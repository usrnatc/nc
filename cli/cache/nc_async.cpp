AsyncShared* ASYNC_SHARED = NULL;

void 
AsyncInit(void)
{
    Arena* MemPool = ArenaAlloc();

    ASYNC_SHARED = ArenaPushArrayZero(MemPool, AsyncShared, 1);
    ASYNC_SHARED->MemPool = MemPool;

    for (AsyncPriority P = 0; P < ASYNC_PRIORITY_COUNT; ++P) {
        AsyncRing* Ring = &ASYNC_SHARED->Rings[P];

        Ring->RingSize = MB(8);
        Ring->RingBase = ArenaPushArray(MemPool, u8, Ring->RingSize);
        Ring->RingMtx = MutexAlloc();
        Ring->RingCondVar = CondVarAlloc();
    }

    ASYNC_SHARED->RingMtx = MutexAlloc();
    ASYNC_SHARED->RingCondVar = CondVarAlloc();
    ASYNC_SHARED->WorkThreadsCount = MAX(4, GetSystemProperties()->LogicalProcessorCount - 1);
    ASYNC_SHARED->WorkThreadsCount = MAX(4, ASYNC_SHARED->WorkThreadsCount);
    ASYNC_SHARED->WorkThreads = ArenaPushArrayZero(MemPool, Handle, ASYNC_SHARED->WorkThreadsCount);

    for (u64 I = 0; I < ASYNC_SHARED->WorkThreadsCount; ++I)
        ASYNC_SHARED->WorkThreads[I] = ThreadLaunch(AsyncWorkThreadEntryPoint, PTR_FROM_INT(I), NULL);
}

u64 
AsyncThreadCount(void)
{
    return ASYNC_SHARED->WorkThreadsCount;
}

b32 
__AsyncPushWork(AsyncWorkFuncType* WorkFunc, AsyncWorkParameters* Params)
{
    AsyncRing* Ring = &ASYNC_SHARED->Rings[Params->Priority];
    AsyncWork Work = {};

    Work.WorkFunc = WorkFunc;
    Work.Input = Params->Input;
    Work.Output = Params->Output;
    Work.Semaphore = Params->Semaphore;
    Work.CompletionCounter = Params->CompletionCounter;
    Work.WorkingCounter = Params->WorkingCounter;

    b32 QueuedInRingBuffer = FALSE;
    b32 NeedToExecuteOnThisThread = FALSE;

    LOCK_SCOPE(Ring->RingMtx) {
        u64 NumAvailableWorkThreads = (ASYNC_SHARED->WorkThreadsCount - AtomicLoadU64(&ASYNC_SHARED->WorkThreadsLiveCount, MEM_ORDER_SEQ_CST));

        if (!NumAvailableWorkThreads && GetTLS()->ASYNC_WORK_THREAD_DEPTH > 0) {
            NeedToExecuteOnThisThread = TRUE;
            continue;
        }

        u64 UnConsumedSize = Ring->RingWritePosition - Ring->RingReadPosition;
        u64 AvailableSize = Ring->RingSize - UnConsumedSize;

        if (AvailableSize >= sizeof(Work)) {
            QueuedInRingBuffer = TRUE;

            if (Params->Semaphore != EMPTY_HANDLE_VALUE)
                SemaphoreLock(Params->Semaphore, U64_MAX);

            Ring->RingWritePosition += RingWriteStruct(Ring->RingBase, Ring->RingSize, Ring->RingWritePosition, &Work);
            continue;
        }

        if (TimeNow() >= Params->EndTimeUSecs)
            continue;

        CondVarWait(Ring->RingCondVar, Ring->RingMtx, Params->EndTimeUSecs);
    }

    if (QueuedInRingBuffer) {
        CondVarBroadcast(Ring->RingCondVar);
        CondVarBroadcast(ASYNC_SHARED->RingCondVar);
    }

    if (NeedToExecuteOnThisThread)
        AsyncExecuteWork(Work);

    b32 Result = (QueuedInRingBuffer || NeedToExecuteOnThisThread);

    return Result;
}

void 
ListPush(Arena* MemPool, AsyncTaskList* List, AsyncTask* Task)
{
    AsyncTaskNode* Node = ArenaPushArrayZero(MemPool, AsyncTaskNode, 1);

    SLL_QUEUE_PUSH(List->Head, List->Tail, Node);
    Node->V = Task;
    ++List->Count;
}

AsyncTask* 
__AsyncTaskLaunch(Arena* MemPool, AsyncWorkFuncType* WorkFunc, AsyncWorkParameters* Params)
{
    AsyncTask* Task = ArenaPushArrayZero(MemPool, AsyncTask, 1);

    Task->Semaphore = SemaphoreAlloc(1, 1, {});

    AsyncWorkParameters ParamsRefined = {};

    MemCpy(&ParamsRefined, Params, sizeof(ParamsRefined));

    ParamsRefined.EndTimeUSecs = U64_MAX;
    ParamsRefined.Semaphore = Task->Semaphore;

    if (!ParamsRefined.Output)
        ParamsRefined.Output = &Task->Output;

    __AsyncPushWork(WorkFunc, &ParamsRefined);

    return Task;
}

void* 
AsyncTaskJoin(AsyncTask* Task)
{
    void* Result = NULL;

    if (Task && Task->Semaphore != EMPTY_HANDLE_VALUE) {
        SemaphoreLock(Task->Semaphore, U64_MAX);
        SemaphoreDestroy(Task->Semaphore, {});
        MemSet(&Task->Semaphore, 0, sizeof(Task->Semaphore));
        Result = (void*) AtomicLoadU64(&Task->Output, MEM_ORDER_SEQ_CST);
    }

    return Result;
}

AsyncWork 
AsyncPopWork(void)
{
    AsyncWork Work = {};
    b32 Done = FALSE;
    AsyncPriority TakenPriority = ASYNC_PRIORITY_LOW;

    LOCK_SCOPE(ASYNC_SHARED->RingMtx) {
        for (AsyncPriority Priority = ASYNC_PRIORITY_HIGH; ; --Priority) {
            AsyncRing* Ring = &ASYNC_SHARED->Rings[Priority];

            DEFER(MutexLock(Ring->RingMtx), MutexUnlock(Ring->RingMtx)) {
                u64 UnConsumedSize = Ring->RingWritePosition - Ring->RingReadPosition;

                if (UnConsumedSize >= sizeof(Work)) {
                    Ring->RingReadPosition += RingReadStruct(
                        Ring->RingBase, 
                        Ring->RingSize, 
                        Ring->RingReadPosition, 
                        &Work
                    );
                    Done = TRUE;
                    TakenPriority = Priority;
                }
            }

            if (Done)
                break;

            if (Priority == ASYNC_PRIORITY_LOW)
                break;
        }

        if (!Done)
            CondVarWait(ASYNC_SHARED->RingCondVar, ASYNC_SHARED->RingMtx, U64_MAX);
    }

    CondVarBroadcast(ASYNC_SHARED->RingCondVar);
    CondVarBroadcast(ASYNC_SHARED->Rings[TakenPriority].RingCondVar);

    return Work;
}

void 
AsyncExecuteWork(AsyncWork Work)
{
    if (!Work.WorkFunc)
        return;

    ThreadLocalStorage* TLS = GetTLS();

    ++TLS->ASYNC_WORK_THREAD_DEPTH;

    void* WorkOutput = Work.WorkFunc(TLS->ASYNC_WORK_THREAD_INDEX, Work.Input);

    --TLS->ASYNC_WORK_THREAD_DEPTH;

    if (Work.Output)
        AtomicStoreU64((u64*) Work.Output, (u64) WorkOutput, MEM_ORDER_SEQ_CST);

    if (Work.Semaphore != EMPTY_HANDLE_VALUE)
        SemaphoreUnlock(Work.Semaphore);

    if (Work.CompletionCounter)
        AtomicIncFetchU64(Work.CompletionCounter, MEM_ORDER_SEQ_CST);

    if (Work.WorkingCounter)
        AtomicDecFetchU64(Work.WorkingCounter, MEM_ORDER_SEQ_CST);
}

AsyncRoot* 
AsyncRootAlloc(void)
{
    Arena* MemPool = ArenaAlloc();
    AsyncRoot* Root = ArenaPushArrayZero(MemPool, AsyncRoot, 1);

    Root->MemPools = ArenaPushArrayZero(MemPool, Arena*, AsyncThreadCount());
    Root->MemPools[0] = MemPool;

    for (u64 I = 1; I < AsyncThreadCount(); ++I)
        Root->MemPools[I] = ArenaAlloc();

    return Root;
}

void 
AsyncRootRelease(AsyncRoot* Root)
{
    for (u64 I = 1; I < AsyncThreadCount(); ++I)
        ArenaRelease(Root->MemPools[I]);

    ArenaRelease(Root->MemPools[0]);
}

Arena* 
AsyncRootThreadMemPool(AsyncRoot* Root)
{
    ThreadLocalStorage* TLS = GetTLS();

    return Root->MemPools[TLS->ASYNC_WORK_THREAD_INDEX];
}

void 
AsyncWorkThreadEntryPoint(void* Param)
{
    u64 ThreadIndex = INT_FROM_PTR(Param);
    ThreadLocalStorage* TLS = GetTLS();

    ThreadSetName((char*) "[ASYNC] work thread #%llu", ThreadIndex);
    TLS->ASYNC_WORK_THREAD_INDEX = ThreadIndex;

    for (;;) {
        AsyncWork Work = AsyncPopWork();

        AtomicIncFetchU64(&ASYNC_SHARED->WorkThreadsLiveCount, MEM_ORDER_SEQ_CST);
        AsyncExecuteWork(Work);
        AtomicDecFetchU64(&ASYNC_SHARED->WorkThreadsLiveCount, MEM_ORDER_SEQ_CST);
    }
}

