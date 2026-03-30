#if !defined(__NC_HASHSTORE_H__)
#define __NC_HASHSTORE_H__

#include "../nc_types.h"
#include "../nc_system.h"
#include "../nc_string.h"

struct Arena;

// @defines____________________________________________________________________
#define HS_KEY_HASH_HISTORY_COUNT            64
#define HS_KEY_HASH_HISTORY_STRONG_REF_COUNT  2

// @types______________________________________________________________________
struct HSRoot {
    u64 V[1];
};

struct HSID {
    u128 V[1];
};

struct HSKey {
    HSRoot Root;
    u64    __Padding;
    HSID   ID;
};

struct HSRootIDChunkNode {
    HSRootIDChunkNode* Next;
    HSID*              V;
    u64                Count;
    u64                Capacity;
};

struct HSRootIDChunkList {
    HSRootIDChunkNode* Head;
    HSRootIDChunkNode* Tail;
    u64                ChunkCount;
    u64                TotalCount;
};

struct HSRootNode {
    HSRootNode*       Next;
    HSRootNode*       Prev;
    Arena*            MemPool;
    HSRoot            Root;
    HSRootIDChunkList IDs;
};

struct HSRootSlot {
    HSRootNode* Head;
    HSRootNode* Tail;
};

struct HSKeyNode {
    HSKeyNode* Next;
    HSKeyNode* Prev;
    HSKey      Key;
    u128       HashHistory[HS_KEY_HASH_HISTORY_COUNT];
    u64        HashHistoryGeneration;
};

struct HSKeySlot {
    HSKeyNode* Head;
    HSKeyNode* Tail;
};

struct HSNode {
    HSNode* Next;
    HSNode* Prev;
    u128    Hash;
    Arena*  MemPool;
    Str8    Data;
    u64     ScopeRefCount;
    u64     KeyRefCount;
    u64     DownstreamRefCount;
};

struct HSSlot {
    HSNode* Head;
    HSNode* Tail;
};

struct HSStripe {
    Arena* MemPool;
    Handle RWMtx;
    Handle CondVar;
};

struct HSTouch {
    HSTouch* Next;
    u128     Hash;
};

struct HSScope {
    HSScope* Next;
    HSTouch* HeadTouch;
};

struct HSTCTX {
    Arena*   MemPool;
    HSScope* FreeScope;
    HSTouch* FreeTouch;
};

struct HSShared {
    Arena*       MemPool;
    u64          SlotsCount;
    u64          StripesCount;
    HSSlot*      Slots;
    HSStripe*    Stripes;
    HSNode**     StripesFreeNodes;
    u64          KeySlotsCount;
    u64          KeyStripesCount;
    HSKeySlot*   KeySlots;
    HSStripe*    KeyStripes;
    HSKeyNode**  KeyStripesFreeNodes;
    u64          RootSlotsCount;
    u64          RootStripesCount;
    HSRootSlot*  RootSlots;
    HSStripe*    RootStripes;
    HSRootNode** RootStripesFreeNodes;
    u64          RootIDGeneration;
    Handle       EvictorThread;
};

// @runtime____________________________________________________________________
extern HSShared* HS_SHARED;

// @functions__________________________________________________________________
u64 HSSmallHash(Str8 Data);
u128 HSHash(Str8 Data);
HSID HSIDAlloc(u64 Val0, u64 Val1);
HSKey HSKeyAlloc(HSRoot Root, HSID ID);
void HSInit(void);
HSRoot HSRootAlloc(void);
void HSRootRelease(HSRoot Root);
u128 HSSubmitData(HSKey Key, Arena** DataMemPool, Str8 Data);
HSScope* HSOpenScope(void);
void HSCloseScope(HSScope* Scope);
void HSScopeTouchNodeSafe(HSScope* Scope, HSNode* Node);
void HSHashDownstreamInc(u128 Hash);
void HSHashDownStreamDec(u128 Hash);
u128 HSHashFromKey(HSKey Key, u64 RewindCount);
Str8 HSDataFromHash(HSScope* Scope, u128 Hash);
void HSEvictorThreadEntryPoint(void* Params);

#if defined(NC_LANG_CPP)
    b32 operator==(HSID IDA, HSID IDB);
    b32 operator!=(HSID IDA, HSID IDB);
    b32 operator==(HSKey KeyA, HSKey KeyB);
    b32 operator!=(HSKey KeyA, HSKey KeyB);
#endif

#endif // __NC_HASHSTORE_H__
