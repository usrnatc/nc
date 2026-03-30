#include "libnc.h"

void
AsyncThreadEntryPoint(void* Params)
{
    LaneContext LaneCtx = *(LaneContext*) Params;

    SetLaneContext(LaneCtx);
    GetTLS()->IS_ASYNC_THREAD = TRUE;
    ThreadSetName("[ASYNC %llu]", LaneIndex());

    for (;;) {
        if (!LaneIndex()) {
            if (!AtomicLoadU32(&ASYNC_LOOP_REPEAT, MEM_ORDER_SEQ_CST)) {
                LOCK_SCOPE(ASYNC_TICK_BEGIN_MTX) {
                    CondVarWait(
                        ASYNC_TICK_BEGIN_COND_VAR, 
                        ASYNC_TICK_BEGIN_MTX, 
                        TimeNow() + SECONDS(1)
                    );
                }
            }

            AtomicExchangeU32(&ASYNC_LOOP_REPEAT, 0, MEM_ORDER_SEQ_CST);
            AtomicExchangeU32(&ASYNC_LOOP_REPEAT_HIGH_PRIORITY, 0, MEM_ORDER_SEQ_CST);
        }

        LaneSync();
        OCAsyncTick();
        CAsyncTick();
        FSAsyncTick();
        LaneSync();

        b32 ShouldQuit = FALSE;

        if (!LaneIndex())
            ShouldQuit = AtomicLoadU32(&GLOBAL_ASYNC_EXIT, MEM_ORDER_SEQ_CST);

        LaneSyncU64(&ShouldQuit, 0);

        if (ShouldQuit)
            break;
    }
}

void
MainThreadBaseEntryPoint(int ArgC, char** ArgV)
{
    ThreadSetName("[MAIN]"_s8);

    TempArena Scratch = GetScratch(NULL, 0);

    ASYNC_TICK_BEGIN_COND_VAR = CondVarAlloc();
    ASYNC_TICK_BEGIN_MTX = MutexAlloc();
    ASYNC_TICK_END_MTX = MutexAlloc();

    Str8List CLIStrings = StrListFromCLIArgs(Scratch.MemPool, ArgC, ArgV);
    CommandLine CLI = CommandLineFromStrList(Scratch.MemPool, CLIStrings);

    OCInit();
    CInit();
    FSInit();

    Handle* AsyncThreads = NULL;
    u64 LaneBroadcastValue = 0;
    u64 MainThreadCount = 1;
    u64 AsyncThreadsCount = GetSystemProperties()->LogicalProcessorCount;
    u64 MainThreadCountClamped = MIN(AsyncThreadsCount, MainThreadCount);

    AsyncThreadsCount -= MainThreadCountClamped;

    Str8 AsyncThreadsCountString = CommandLineString(&CLI, "threads"_s8);

    if (AsyncThreadsCountString.Size)
        AsyncThreadsCount = U64FromStr(AsyncThreadsCountString);

    AsyncThreadsCount = MAX(1, AsyncThreadsCount);

    Handle Barrier = BarrierAlloc(AsyncThreadsCount);
    LaneContext* LaneCtxs = ArenaPushArrayZero(
        Scratch.MemPool, 
        LaneContext, 
        AsyncThreadsCount
    );

    ASYNC_THREADS_COUNT = AsyncThreadsCount;
    LogInfo("%llu async threads launched", ASYNC_THREADS_COUNT);
    AsyncThreads = ArenaPushArrayZero(Scratch.MemPool, Handle, ASYNC_THREADS_COUNT);

    for (u64 Index = 0; Index < ASYNC_THREADS_COUNT; ++Index) {
        LaneCtxs[Index].Index = Index;
        LaneCtxs[Index].Count = ASYNC_THREADS_COUNT;
        LaneCtxs[Index].Barrier = Barrier;
        LaneCtxs[Index].BroadcastMemory = &LaneBroadcastValue;
        AsyncThreads[Index] = ThreadLaunch(AsyncThreadEntryPoint, &LaneCtxs[Index]);
    }

    EntryPoint(&CLI);
    ReleaseScratch(Scratch);
}

void
EntryPoint(CommandLine* CLI)
{}
