#if !defined(__NC_ASYNC_H__)
#define __NC_ASYNC_H__

#include "../nc_types.h"
#include "../nc_system.h"

// @defines____________________________________________________________________
#define NC_ASYNC_FUNC_PROT(X) void* X(u64 ThreadIndex, void* Input)
#define NC_ASYNC_FUNC_DEF(X) NC_ASYNC_FUNC_PROT(X)

typedef NC_ASYNC_FUNC_PROT(AsyncWorkFuncType);

typedef u32 AsyncPriority;
enum : u32 {
    ASYNC_PRIORITY_LOW,
    ASYNC_PRIORITY_HIGH,
    ASYNC_PRIORITY_COUNT
};

#define AsyncPushWork(X, ...) [&]() -> b32 {                        \
    AsyncWorkParameters __Params = {U64_MAX, ASYNC_PRIORITY_HIGH};  \
    __VA_ARGS__;                                                    \
    return __AsyncPushWork((X), &__Params);                         \
}()

#define AsyncTaskLaunch(X, Y, ...) [&]() -> AsyncTask* {            \
    AsyncWorkParameters __Params = {U64_MAX, ASYNC_PRIORITY_HIGH};  \
    __VA_ARGS__;                                                    \
    return __AsyncTaskLaunch((X), (Y), &__Params);                  \
}()

#define AsyncTaskJoinStruct(X, Y) (Y*) AsyncTaskJoin((X))
#define RingWriteStruct(X, Y, Z, W)    RingWrite((X), (Y), (Z), (W), sizeof(*(W)))
#define RingReadStruct(X, Y, Z, W)     RingRead((X), (Y), (Z), (W), sizeof(*(W)))

// @types______________________________________________________________________
struct AsyncWorkParameters {
    u64 EndTimeUSecs;
    AsyncPriority Priority;
    void* Input;
    void** Output;
    Handle Semaphore;
    u64* CompletionCounter;
    u64* WorkingCounter;
};

struct AsyncWork {
    AsyncWorkFuncType* WorkFunc;
    void* Input;
    void** Output;
    Handle Semaphore;
    u64* CompletionCounter;
    u64* WorkingCounter;
};

struct AsyncTask {
    Handle Semaphore;
    void* Output;
};

struct AsyncTaskNode {
    AsyncTaskNode* Next;
    AsyncTask* V;
};

struct AsyncTaskList {
    AsyncTaskNode* Head;
    AsyncTaskNode* Tail;
    u64 Count;
};

struct AsyncRoot {
    Arena** MemPools;
};

struct AsyncRing {
    u64 RingSize;
    u8* RingBase;
    u64 RingWritePosition;
    u64 RingReadPosition;
    Handle RingMtx;
    Handle RingCondVar;
};

struct AsyncShared {
    Arena* MemPool;
    AsyncRing Rings[ASYNC_PRIORITY_COUNT];
    Handle RingMtx;
    Handle RingCondVar;
    Handle* WorkThreads;
    u64 WorkThreadsCount;
    u64 WorkThreadsLiveCount;
};

// @runtime____________________________________________________________________
extern AsyncShared* ASYNC_SHARED;

// @functions__________________________________________________________________
void AsyncInit(void);
u64 AsyncThreadCount(void);
b32 __AsyncPushWork(AsyncWorkFuncType* WorkFunc, AsyncWorkParameters* Params);
void ListPush(Arena* MemPool, AsyncTaskList* List, AsyncTask* Task);
AsyncTask* __AsyncTaskLaunch(Arena* MemPool, AsyncWorkFuncType* WorkFunc, AsyncWorkParameters* Params);
void* AsyncTaskJoin(AsyncTask* Task);
AsyncWork AsyncPopWork(void);
void AsyncExecuteWork(AsyncWork Work);
AsyncRoot* AsyncRootAlloc(void);
void AsyncRootRelease(AsyncRoot* Root);
Arena* AsyncRootThreadMemPool(AsyncRoot* Root);
void AsyncWorkThreadEntryPoint(void* Param);

#endif // __NC_ASYNC_H__
