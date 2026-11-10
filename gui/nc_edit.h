#if !defined(__NC_EDIT_H__)
#define __NC_EDIT_H__

// @defines____________________________________________________________________


// @types______________________________________________________________________
struct EditNode {
    EditNode* Next;
    EditNode* Prev;
    u128      Key;
};

struct EditSlot {
    EditNode* Head;
    EditNode* Tail;
};

struct EditStripe {
    Arena*    MemPool;
    EditNode* FreeNode;
    Handle    RWMtx;
};

struct EditOp {
    r1u64 Range;
    Str8  Replace;
};

struct EditMutateThread {
    u64    RingSize;
    u8*    RingBase;
    u64    RingReadPosition;
    u64    RingWritePosition;
    Handle CondVar;
    Handle Mtx;
    Handle Thread;
};

struct EditShared {
    Arena*            MemPool;
    u64               SlotsCount;
    u64               StripesCount;
    EditSlot*         Slots;
    EditStripe*       Stripes;
    u64               MutateThreadsCount;
    EditMutateThread* MutateThreads;
};

// @runtime____________________________________________________________________
extern EditShared* EDIT_SHARED;

// @functions__________________________________________________________________
void EditInit(void);
void EditPushOp(CKey BufferKey, EditOp Op);
void EditEnqueueOp(EditMutateThread* Thread, CKey BufferKey, EditOp Op);
void EditDequeueOp(Arena* MemPool, EditMutateThread* Thread, OUT CKey* BufferKey, OUT EditOp* Op);
void EditMutateThreadEntryPoint(void* Params);

#endif // __NC_EDIT_H__
