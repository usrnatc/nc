EditShared* EDIT_SHARED = NULL;

void 
EditInit(void)
{
    Arena* MemPool = ArenaAlloc();

    EDIT_SHARED = ArenaPushArrayZero(
        MemPool,
        EditShared,
        1
    );
    EDIT_SHARED->MemPool = MemPool;
    EDIT_SHARED->SlotsCount = 256;
    EDIT_SHARED->StripesCount = MIN(
        EDIT_SHARED->SlotsCount,
        GetSystemProperties()->LogicalProcessorCount
    );
    EDIT_SHARED->Slots = ArenaPushArrayZero(
        MemPool,
        EditSlot,
        EDIT_SHARED->SlotsCount
    );
    EDIT_SHARED->Stripes = ArenaPushArrayZero(
        MemPool,
        EditStripe,
        EDIT_SHARED->StripesCount
    );

    for (u64 Index = 0; Index < EDIT_SHARED->StripesCount; ++Index) {
        EDIT_SHARED->Stripes[Index].MemPool = ArenaAlloc();
        EDIT_SHARED->Stripes[Index].RWMtx = RWMutexAlloc();
    }

    EDIT_SHARED->MutateThreadsCount = MIN(
        GetSystemProperties()->LogicalProcessorCount,
        4
    );
    EDIT_SHARED->MutateThreads = ArenaPushArrayZero(
        MemPool,
        EditMutateThread,
        EDIT_SHARED->MutateThreadsCount
    );

    for (u64 Index = 0; Index < EDIT_SHARED->MutateThreadsCount; ++Index) {
        EDIT_SHARED->MutateThreads[Index].RingSize = KB(64);
        EDIT_SHARED->MutateThreads[Index].RingBase = ArenaPushArray(
            MemPool,
            u8,
            EDIT_SHARED->MutateThreads[Index].RingSize
        );
        EDIT_SHARED->MutateThreads[Index].CondVar = CondVarAlloc();
        EDIT_SHARED->MutateThreads[Index].Mtx = MutexAlloc();
        EDIT_SHARED->MutateThreads[Index].Thread = ThreadLaunch(
            EditMutateThreadEntryPoint,
            &EDIT_SHARED->MutateThreads[Index]
        );
    }
}

void 
EditPushOp(CKey BufferKey, EditOp Op)
{
    u64 BufferHash = Hash(Str8Struct(&BufferKey));
    EditMutateThread* Thread = &EDIT_SHARED->MutateThreads[
        BufferHash % EDIT_SHARED->MutateThreadsCount
    ];

    EditEnqueueOp(Thread, BufferKey, Op);
}

void 
EditEnqueueOp(EditMutateThread* Thread, CKey BufferKey, EditOp Op)
{
    LOCK_SCOPE(Thread->Mtx) {
        for (;;) {
            u64 UnconsumedSize = Thread->RingWritePosition - Thread->RingReadPosition;
            u64 AvailableSize = Thread->RingSize - UnconsumedSize;
            u64 RequiredSize = (
                sizeof(BufferKey) + 
                sizeof(Op.Range) + 
                sizeof(Op.Replace.Size) + 
                Op.Replace.Size
            );

            if (AvailableSize >= RequiredSize) {
                Thread->RingWritePosition += RingWriteStruct(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingWritePosition,
                    &BufferKey
                );
                Thread->RingWritePosition += RingWriteStruct(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingWritePosition,
                    &Op.Range
                );
                Thread->RingWritePosition += RingWriteStruct(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingWritePosition,
                    &Op.Replace.Size
                );
                Thread->RingWritePosition += RingWrite(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingWritePosition,
                    Op.Replace.Str,
                    Op.Replace.Size
                );
                break;
            }

            CondVarWait(Thread->CondVar, Thread->Mtx, U64_MAX);
        }
    }

    CondVarBroadcast(Thread->CondVar);
}

void 
EditDequeueOp(
    Arena* MemPool, 
    EditMutateThread* Thread, 
    OUT CKey* BufferKey, 
    OUT EditOp* Op
) {
    LOCK_SCOPE(Thread->Mtx) {
        for (;;) {
            u64 UnconsumedSize = Thread->RingWritePosition - Thread->RingReadPosition;

            if (
                UnconsumedSize >= (
                    sizeof(*BufferKey) + 
                    sizeof(Op->Range) + 
                    sizeof(Op->Replace.Size)
                )
            ) {
                Thread->RingReadPosition += RingReadStruct(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingReadPosition,
                    BufferKey
                );
                Thread->RingReadPosition += RingReadStruct(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingReadPosition,
                    &Op->Range
                );
                Thread->RingReadPosition += RingReadStruct(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingReadPosition,
                    &Op->Replace.Size
                );
                Op->Replace.Str = ArenaPushArray(
                    MemPool,
                    u8,
                    Op->Replace.Size
                );
                Thread->RingReadPosition += RingRead(
                    Thread->RingBase,
                    Thread->RingSize,
                    Thread->RingReadPosition,
                    Op->Replace.Str,
                    Op->Replace.Size
                );
                break;
            }

            CondVarWait(Thread->CondVar, Thread->Mtx, U64_MAX);
        }
    }

    CondVarBroadcast(Thread->CondVar);
}

void 
EditMutateThreadEntryPoint(void* Params)
{
    EditMutateThread* MutateThread = (EditMutateThread*) Params;

    ThreadSetName(
        "[EDIT_MUTATE_THREAD_%llu]",
        (u64) (MutateThread - EDIT_SHARED->MutateThreads)
    );

    for (;;) {
        TempArena Scratch = GetScratch(NULL, 0);
        Access* Acc = AccessOpen();
        CKey BufferKey = {};
        EditOp Op = {};

        EditDequeueOp(Scratch.MemPool, MutateThread, &BufferKey, &Op);

        u128 BufferHash = CHashFromKey(BufferKey, 0);
        Str8 Data = CDataFromHash(Acc, BufferHash);

        Op.Range.Min = MIN(Op.Range.Min, Data.Size);
        Op.Range.Max = MAX(Op.Range.Max, Data.Size);

        if (
            Op.Range.Max != Op.Range.Min ||
            Op.Replace.Size
        ) {
            u64 NewDataSize = Data.Size + Op.Replace.Size - Length(Op.Range);
            Arena* MemPool = ArenaAlloc(
                __Params.CommitSize = NewDataSize + ARENA_HEADER_SIZE,
                __Params.ReserveSize = NewDataSize + ARENA_HEADER_SIZE
            );
            u8* NewDataBase = ArenaPushArray(
                MemPool,
                u8,
                NewDataSize
            );
            Str8 PreReplaceData = StrSub(Data, Rng(0ULL, Op.Range.Min));
            Str8 PostReplaceData = StrSub(Data, Rng(Op.Range.Max, Data.Size));

            if (PreReplaceData.Size) {
                MemCpy(
                    NewDataBase,
                    PreReplaceData.Str,
                    PreReplaceData.Size
                );
            }

            if (Op.Replace.Size) {
                MemCpy(
                    NewDataBase + PreReplaceData.Size,
                    Op.Replace.Str,
                    Op.Replace.Size
                );
            }

            if (PostReplaceData.Size) {
                MemCpy(
                    NewDataBase + PreReplaceData.Size + Op.Replace.Size,
                    PostReplaceData.Str,
                    PostReplaceData.Size
                );
            }

            Str8 NewData = Str(NewDataBase, NewDataSize);

            CSubmitData(BufferKey, &MemPool, NewData);
        }

        AccessClose(Acc);
        ReleaseScratch(Scratch);
    }
}
