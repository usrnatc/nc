#if !defined(__NC_TEXTURE_CACHE_H__)
#define __NC_TEXTURE_CACHE_H__

#include "../nc_types.h"
#include "../nc_math.h"
#include "../nc_render.h"
#include "../nc_thread.h"
#include "nc_hashstore.h"
#include "nc_async.h"

struct Arena;

// @types______________________________________________________________________
struct TexTopology {
    v2i16 Dim;
    RendTex2DFmt Format;
};

struct TexNode {
    TexNode* Next;
    TexNode* Prev;
    u128 Hash;
    TexTopology Topology;
    RendHandle Texture;
    b32 IsWorking;
    u64 ScopeRefCount;
    u64 LastTimeTouchedUSecs;
    u64 LastUserClockIndexTouched;
    u64 LoadCount;
};

struct TexSlot {
    TexNode* Head;
    TexNode* Tail;
};

struct TexStripe {
    Arena* MemPool;
    Handle RWMutex;
    Handle CondVar;
};

struct TexTouch {
    TexTouch* Next;
    u128 Hash;
    TexTopology Topology;
};

struct TexScope {
    TexScope* Next;
    TexTouch* HeadTouch;
};

struct TexTCTX {
    Arena* MemPool;
    TexScope* FreeScope;
    TexTouch* FreeTouch;
};

struct TexShared {
    Arena* MemPool;
    u64 SlotsCount;
    u64 StripesCount;
    TexSlot* Slots;
    TexStripe* Stripes;
    TexNode** StripesFreeNodes;
    u64 RingSize;
    u8* RingBase;
    u64 RingWritePosition;
    u64 RingReadPosition;
    Handle RingCondVar;
    Handle RingMutex;
    Handle EvictorThread;
};

// @runtime____________________________________________________________________
extern TexShared* TEX_SHARED;

// @functions__________________________________________________________________
TexTopology TexTopologyAlloc(v2i32 Dim, RendTex2DFmt Format);
void TexInit(void);
void TexTCTXEnsureInitialised(void);
TexScope* TexScopeOpen(void);
void TexScopeClose(TexScope* Scope);
void TexScopeTouchNodeR(TexScope* Scope, TexNode* Node);
RendHandle TexTextureFromHashTopology(TexScope* Scope, u128 Hash, TexTopology Topology);
RendHandle TexTextureFromKeyTopology(TexScope* Scope, HSKey Key, TexTopology Topology, OUT u128* Hash);
b32 TexEnqueueRequest(u128 Hash, TexTopology Topology, u64 EndTimeUSecs);
void TexDequeueRequest(OUT u128* Hash, OUT TexTopology* Topology);
void TexEvictorThreadEntryPoint(void* Param);

NC_ASYNC_FUNC_DEF(TexTransferWork);

#endif // __NC_TEXTURE_CACHE_H__
