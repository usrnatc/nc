#include "nc_hashstore.h"

#if !defined(XXH_IMPLEMENTATION)
    #define XXH_IMPLEMENTATION
    #define XXH_STATIC_LINKING_ONLY
    #include "xxHash/xxhash.h"
#endif

HSShared* HS_SHARED = NULL;

u64
HSSmallHash(Str8 Data)
{
    u64 Result = XXH3_64bits(Data.Str, Data.Size);

    return Result;
}

u128
HSHash(Str8 Data)
{
    u128 Result = {};
    XXH128_hash_t DataHash = XXH3_128bits(Data.Str, Data.Size);

    MemCpy(&Result, &DataHash, sizeof(Result));

    return Result;
}

HSID
HSIDAlloc(u64 Val0, u64 Val1)
{
    HSID Result;

    Result.V[0].U64[0] = Val0;
    Result.V[0].U64[1] = Val1;

    return Result;
}

HSKey
HSKeyAlloc(HSRoot Root, HSID ID)
{
    HSKey Key = {};

    Key.Root = Root;
    Key.__Padding = 0;
    Key.ID = ID;

    return Key;
}

void
HSInit(void)
{
    Arena* MemPool = ArenaAlloc();

    HS_SHARED = ArenaPushArrayZero(MemPool, HSShared, 1);
    HS_SHARED->MemPool = MemPool;
    HS_SHARED->SlotsCount = 4096;
    HS_SHARED->StripesCount = MIN(HS_SHARED->SlotsCount, GetSystemProperties()->LogicalProcessorCount);
    HS_SHARED->Slots = ArenaPushArrayZero(MemPool, HSSlot, HS_SHARED->SlotsCount);
    HS_SHARED->Stripes = ArenaPushArrayZero(MemPool, HSStripe, HS_SHARED->StripesCount);
    HS_SHARED->StripesFreeNodes = ArenaPushArrayZero(MemPool, HSNode*, HS_SHARED->StripesCount);

    for (u64 I = 0; I < HS_SHARED->StripesCount; ++I) {
        HSStripe* Stripe = &HS_SHARED->Stripes[I];

        Stripe->MemPool = ArenaAlloc();
        Stripe->RWMtx = RWMutexAlloc();
        Stripe->CondVar = CondVarAlloc();
    }

    HS_SHARED->KeySlotsCount = 4096;
    HS_SHARED->KeyStripesCount = MIN(HS_SHARED->KeySlotsCount, GetSystemProperties()->LogicalProcessorCount);
    HS_SHARED->KeySlots = ArenaPushArrayZero(MemPool, HSKeySlot, HS_SHARED->KeySlotsCount);
    HS_SHARED->KeyStripes = ArenaPushArrayZero(MemPool, HSStripe, HS_SHARED->KeyStripesCount);
    HS_SHARED->KeyStripesFreeNodes = ArenaPushArrayZero(MemPool, HSKeyNode*, HS_SHARED->KeyStripesCount);

    for (u64 I = 0; I < HS_SHARED->KeyStripesCount; ++I) {
        HSStripe* KeyStripe = &HS_SHARED->KeyStripes[I];

        KeyStripe->MemPool = ArenaAlloc();
        KeyStripe->RWMtx = RWMutexAlloc();
        KeyStripe->CondVar = CondVarAlloc();
    }

    HS_SHARED->RootSlotsCount = 4096;
    HS_SHARED->RootStripesCount = MIN(HS_SHARED->RootSlotsCount, GetSystemProperties()->LogicalProcessorCount);
    HS_SHARED->RootSlots = ArenaPushArrayZero(MemPool, HSRootSlot, HS_SHARED->RootSlotsCount);
    HS_SHARED->RootStripes = ArenaPushArrayZero(MemPool, HSStripe, HS_SHARED->RootStripesCount);
    HS_SHARED->RootStripesFreeNodes = ArenaPushArrayZero(MemPool, HSRootNode*, HS_SHARED->RootStripesCount);

    for (u64 I = 0; I < HS_SHARED->RootStripesCount; ++I) {
        HSStripe* RootStripe = &HS_SHARED->RootStripes[I];

        RootStripe->MemPool = ArenaAlloc();
        RootStripe->RWMtx = RWMutexAlloc();
        RootStripe->CondVar = CondVarAlloc();
    }

    HS_SHARED->EvictorThread = ThreadLaunch(HSEvictorThreadEntryPoint, NULL, NULL);
}

HSRoot
HSRootAlloc(void)
{
    HSRoot Result = {};

    *Result.V = AtomicIncFetchU64(&HS_SHARED->RootIDGeneration, MEM_ORDER_SEQ_CST);

    u64 SlotIndex = *Result.V % HS_SHARED->RootSlotsCount;
    u64 StripeIndex = SlotIndex % HS_SHARED->RootStripesCount;
    HSRootSlot* Slot = &HS_SHARED->RootSlots[SlotIndex];
    HSStripe* Stripe = &HS_SHARED->RootStripes[StripeIndex];

    LOCK_SCOPE_W(Stripe->RWMtx) {
        HSRootNode* Node = HS_SHARED->RootStripesFreeNodes[StripeIndex];

        if (Node)
            SLL_STACK_POP(HS_SHARED->RootStripesFreeNodes[StripeIndex]);
        else
            Node = ArenaPushArrayZero(Stripe->MemPool, HSRootNode, 1);

        DLL_PUSH_BACK(Slot->Head, Slot->Tail, Node);
        Node->Root = Result;
        Node->MemPool = ArenaAlloc();
    }

    return Result;
}

void
HSRootRelease(HSRoot Root)
{
    u64 SlotIndex = *Root.V % HS_SHARED->RootSlotsCount;
    u64 StripeIndex = SlotIndex % HS_SHARED->RootStripesCount;
    HSRootSlot* Slot = &HS_SHARED->RootSlots[SlotIndex];
    HSStripe* Stripe = &HS_SHARED->RootStripes[StripeIndex];
    Arena* RootMemPool = NULL;
    HSRootIDChunkList RootIDS = {};

    LOCK_SCOPE_W(Stripe->RWMtx) {
        for (HSRootNode* Node = Slot->Head; Node; Node = Node->Next) {
            if (MemCmp(&Root, &Node->Root, sizeof(Root))) {
                DLL_REMOVE(Slot->Head, Slot->Tail, Node);
                RootMemPool = Node->MemPool;
                RootIDS = Node->IDs;
                SLL_STACK_PUSH(HS_SHARED->RootStripesFreeNodes[StripeIndex], Node);
                break;
            }
        }
    }

    for (
        HSRootIDChunkNode* IDChunkNode = RootIDS.Head; 
        IDChunkNode; 
        IDChunkNode = IDChunkNode->Next
    ) {
        for (u64 ChunkIndex = 0; ChunkIndex < IDChunkNode->Count; ++ChunkIndex) {
            HSID ID = IDChunkNode->V[ChunkIndex];
            HSKey Key = HSKeyAlloc(Root, ID);
            u64 KeyHash = HSSmallHash(Str((u8*) &Key, sizeof(Key)));
            u64 KeySlotIndex = KeyHash % HS_SHARED->KeySlotsCount;
            u64 KeyStripeIndex = KeySlotIndex % HS_SHARED->KeyStripesCount;
            HSKeySlot* KeySlot = &HS_SHARED->KeySlots[KeySlotIndex];
            HSStripe* KeyStripe = &HS_SHARED->KeyStripes[KeyStripeIndex];

            LOCK_SCOPE_W(KeyStripe->RWMtx) {
                for (HSKeyNode* Node = KeySlot->Head; Node; Node = Node->Next) {
                    if (Node->Key == Key) {
                        for (
                            u64 HistoryIndex = 0;
                            (
                                HistoryIndex < HS_KEY_HASH_HISTORY_STRONG_REF_COUNT && 
                                HistoryIndex < Node->HashHistoryGeneration
                            );
                            ++HistoryIndex
                        ) {
                            u128 Hash = Node->HashHistory[(Node->HashHistoryGeneration + HistoryIndex) % ARRAY_COUNT(Node->HashHistory)];
                            u64 HashSlotIndex = Hash.U64[1] % HS_SHARED->SlotsCount;
                            u64 HashStripeIndex = HashSlotIndex % HS_SHARED->StripesCount;
                            HSSlot* HashSlot = &HS_SHARED->Slots[HashSlotIndex];
                            HSStripe* HashStripe = &HS_SHARED->Stripes[HashStripeIndex];

                            LOCK_SCOPE_R(HashStripe->RWMtx) {
                                for (HSNode* N = HashSlot->Head; N; N = N->Next) {
                                    if (N->Hash == Hash) {
                                        AtomicDecFetchU64(&N->KeyRefCount, MEM_ORDER_SEQ_CST);
                                        break;
                                    }
                                }
                            }
                        }

                        DLL_REMOVE(KeySlot->Head, KeySlot->Tail, Node);
                        SLL_STACK_PUSH(HS_SHARED->KeyStripesFreeNodes[KeyStripeIndex], Node);
                        break;
                    }
                }
            }
        }
    }
}

u128
HSSubmitData(HSKey Key, Arena** DataMemPool, Str8 Data)
{
    u64 KeyHash = HSSmallHash(Str((u8*) &Key, sizeof(Key)));
    u64 KeySlotIndex = KeyHash % HS_SHARED->KeySlotsCount;
    u64 KeyStripeIndex = KeySlotIndex % HS_SHARED->KeyStripesCount;
    HSKeySlot *KeySlot = &HS_SHARED->KeySlots[KeySlotIndex];
    HSStripe *KeyStripe = &HS_SHARED->KeyStripes[KeyStripeIndex];
    u128 Result = HSHash(Data);
    u64 SlotIndex = Result.U64[1] % HS_SHARED->SlotsCount;
    u64 StripeIndex = SlotIndex % HS_SHARED->StripesCount;
    HSSlot *Slot = &HS_SHARED->Slots[SlotIndex];
    HSStripe *Stripe = &HS_SHARED->Stripes[StripeIndex];

    LOCK_SCOPE_W(Stripe->RWMtx) {
        HSNode* FoundNode = NULL;

        for (HSNode* Node = Slot->Head; Node; Node = Node->Next) {
            if (Node->Hash == Result) {
                FoundNode = Node;
                break;
            }
        }

        if (!FoundNode) {
            HSNode* Node = HS_SHARED->StripesFreeNodes[StripeIndex];

            if (Node)
                SLL_STACK_POP(HS_SHARED->StripesFreeNodes[StripeIndex]);
            else
                Node = ArenaPushArrayZero(Stripe->MemPool, HSNode, 1);

            Node->Hash = Result;

            if (DataMemPool)
                Node->MemPool = *DataMemPool;

            Node->Data = Data;
            Node->ScopeRefCount = 0;
            Node->KeyRefCount = 1;
            DLL_PUSH_BACK(Slot->Head, Slot->Tail, Node);
        } else {
            ++FoundNode->KeyRefCount;

            if (DataMemPool)
                ArenaRelease(*DataMemPool);
        }

        if (DataMemPool)
            *DataMemPool = NULL;
    }

    u128 KeyExpiredHash = {};

    LOCK_SCOPE_W(KeyStripe->RWMtx) {
        b32 KeyIsNew = FALSE;
        HSKeyNode* KeyNode = NULL;

        for (HSKeyNode* Node = KeySlot->Head; Node; Node = Node->Next) {
            if (Node->Key == Key) {
                KeyNode = Node;
                break;
            }
        }

        if (!KeyNode) {
            KeyIsNew = TRUE;
            KeyNode = HS_SHARED->KeyStripesFreeNodes[KeyStripeIndex];

            if (KeyNode)
                SLL_STACK_POP(HS_SHARED->KeyStripesFreeNodes[KeyStripeIndex]);
            else
                KeyNode = ArenaPushArrayZero(KeyStripe->MemPool, HSKeyNode, 1);

            KeyNode->Key = Key;
            DLL_PUSH_BACK(KeySlot->Head, KeySlot->Tail, KeyNode);
        }

        if (KeyNode) {
            if (KeyNode->HashHistoryGeneration >= HS_KEY_HASH_HISTORY_STRONG_REF_COUNT)
                KeyExpiredHash = KeyNode->HashHistory[(KeyNode->HashHistoryGeneration - HS_KEY_HASH_HISTORY_STRONG_REF_COUNT) % ARRAY_COUNT(KeyNode->HashHistory)];

            KeyNode->HashHistory[KeyNode->HashHistoryGeneration % ARRAY_COUNT(KeyNode->HashHistory)] = Result;
            ++KeyNode->HashHistoryGeneration;
        }

        if (KeyIsNew) {
            u64 RootHash = HSSmallHash(Str((u8*) &Key.Root, sizeof(Key.Root)));
            u64 RootSlotIndex = RootHash % HS_SHARED->RootSlotsCount;
            u64 RootStripeIndex = RootSlotIndex % HS_SHARED->RootStripesCount;
            HSRootSlot* RootSlot = &HS_SHARED->RootSlots[RootSlotIndex];
            HSStripe* RootStripe = &HS_SHARED->RootStripes[RootStripeIndex];

            LOCK_SCOPE_W(RootStripe->RWMtx) {
                for (HSRootNode* N = RootSlot->Head; N; N = N->Next) {
                    if (MemCmp(&N->Root, &Key.Root, sizeof(N->Root))) {
                        HSRootIDChunkNode* Chunk = N->IDs.Tail;

                        if (!Chunk || Chunk->Count >= Chunk->Capacity) {
                            Chunk = ArenaPushArrayZero(N->MemPool, HSRootIDChunkNode, 1);
                            SLL_QUEUE_PUSH(N->IDs.Head, N->IDs.Tail, Chunk);
                            ++N->IDs.ChunkCount;
                            Chunk->Capacity = 1024;
                            Chunk->V = ArenaPushArray(N->MemPool, HSID, Chunk->Capacity);
                        }

                        Chunk->V[Chunk->Count] = Key.ID;
                        ++Chunk->Count;
                        ++N->IDs.TotalCount;
                        break;
                    }
                }
            }
        }
    }

    if (KeyExpiredHash != u128{}) {
        u64 OldHashSlotIndex = KeyExpiredHash.U64[1] % HS_SHARED->SlotsCount;
        u64 OldHashStripeIndex = OldHashSlotIndex % HS_SHARED->StripesCount;
        HSSlot* OldHashSlot = &HS_SHARED->Slots[OldHashSlotIndex];
        HSStripe* OldHashStripe = &HS_SHARED->Stripes[OldHashStripeIndex];

        LOCK_SCOPE_R(OldHashStripe->RWMtx) {
            for (HSNode* Node = OldHashSlot->Head; Node; Node = Node->Next) {
                if (Node->Hash == KeyExpiredHash) {
                    AtomicDecFetchU64(&Node->KeyRefCount, MEM_ORDER_SEQ_CST);
                    break;
                }
            }
        }
    }

    return Result;
}

HSScope*
HSOpenScope(void)
{
    ThreadLocalStorage* TLS = GetTLS();

    if (!TLS->HS_CTX) {
        Arena* MemPool = ArenaAlloc();

        TLS->HS_CTX = ArenaPushArrayZero(MemPool, HSTCTX, 1);
        TLS->HS_CTX->MemPool = MemPool;
    }

    HSScope* Result = TLS->HS_CTX->FreeScope;

    if (Result)
        SLL_STACK_POP(TLS->HS_CTX->FreeScope);
    else
        Result = ArenaPushArrayZero(TLS->HS_CTX->MemPool, HSScope, 1);

    MemSet(Result, 0, sizeof(*Result));

    return Result;
}

void
HSCloseScope(HSScope* Scope)
{
    ThreadLocalStorage* TLS = GetTLS();

    for (HSTouch* Touch = Scope->HeadTouch, *Next = NULL; Touch; Touch = Next) {
        u128 Hash = Touch->Hash;

        Next = Touch->Next;

        u64 SlotIndex = Hash.U64[1] % HS_SHARED->SlotsCount;
        u64 StripeIndex = SlotIndex % HS_SHARED->StripesCount;
        HSSlot* Slot = &HS_SHARED->Slots[SlotIndex];
        HSStripe* Stripe = &HS_SHARED->Stripes[StripeIndex];

        LOCK_SCOPE_R(Stripe->RWMtx) {
            for (HSNode* Node = Slot->Head; Node; Node = Node->Next) {
                if (Hash == Node->Hash) {
                    AtomicDecFetchU64(&Node->ScopeRefCount, MEM_ORDER_SEQ_CST);
                    break;
                }
            }
        }

        SLL_STACK_PUSH(TLS->HS_CTX->FreeTouch, Touch);
    }

    SLL_STACK_PUSH(TLS->HS_CTX->FreeScope, Scope);
}

void
HSScopeTouchNodeSafe(HSScope* Scope, HSNode* Node)
{
    ThreadLocalStorage* TLS = GetTLS();
    HSTouch* Touch = TLS->HS_CTX->FreeTouch;

    AtomicIncFetchU64(&Node->ScopeRefCount, MEM_ORDER_SEQ_CST);

    if (Touch)
        SLL_STACK_POP(TLS->HS_CTX->FreeTouch);
    else
        Touch = ArenaPushArray(TLS->HS_CTX->MemPool, HSTouch, 1);

    MemSet(Touch, 0, sizeof(*Touch));
    Touch->Hash = Node->Hash;
    SLL_STACK_PUSH(Scope->HeadTouch, Touch);
}

void
HSHashDownstreamInc(u128 Hash)
{
    u64 SlotIndex = Hash.U64[1] % HS_SHARED->SlotsCount;
    u64 StripeIndex = SlotIndex % HS_SHARED->StripesCount;
    HSSlot* Slot = &HS_SHARED->Slots[SlotIndex];
    HSStripe* Stripe = &HS_SHARED->Stripes[StripeIndex];

    LOCK_SCOPE_R(Stripe->RWMtx) {
        for (HSNode* Node = Slot->Head; Node; Node = Node->Next) {
            if (Hash == Node->Hash) {
                AtomicIncFetchU64(&Node->DownstreamRefCount, MEM_ORDER_SEQ_CST);
                break;
            }
        }
    }
}

void
HSHashDownStreamDec(u128 Hash)
{
    u64 SlotIndex = Hash.U64[1] % HS_SHARED->SlotsCount;
    u64 StripeIndex = SlotIndex % HS_SHARED->StripesCount;
    HSSlot* Slot = &HS_SHARED->Slots[SlotIndex];
    HSStripe* Stripe = &HS_SHARED->Stripes[StripeIndex];

    LOCK_SCOPE_R(Stripe->RWMtx) {
        for (HSNode* Node = Slot->Head; Node; Node = Node->Next) {
            if (Hash == Node->Hash) {
                AtomicDecFetchU64(&Node->DownstreamRefCount, MEM_ORDER_SEQ_CST);
                break;
            }
        }
    }
}

u128
HSHashFromKey(HSKey Key, u64 RewindCount)
{
    u128 Result = {};
    u64 KeyHash = HSSmallHash(Str((u8*) &Key, sizeof(Key)));
    u64 KeySlotIndex = KeyHash % HS_SHARED->KeySlotsCount;
    u64 KeyStripeIndex = KeySlotIndex % HS_SHARED->KeyStripesCount;
    HSKeySlot* KeySlot = &HS_SHARED->KeySlots[KeySlotIndex];
    HSStripe* KeyStripe = &HS_SHARED->KeyStripes[KeyStripeIndex];

    LOCK_SCOPE_R(KeyStripe->RWMtx) {
        for (HSKeyNode* Node = KeySlot->Head; Node; Node = Node->Next) {
            if ((Node->Key == Key) && Node->HashHistoryGeneration > 0 && Node->HashHistoryGeneration - 1 >= RewindCount) {
                Result = Node->HashHistory[(Node->HashHistoryGeneration - 1 - RewindCount) % ARRAY_COUNT(Node->HashHistory)];
                break;
            }
        }
    }

    return Result;
}

Str8
HSDataFromHash(HSScope* Scope, u128 Hash)
{
    Str8 Result = {};
    u64 SlotIndex = Hash.U64[1] % HS_SHARED->SlotsCount;
    u64 StripeIndex = SlotIndex % HS_SHARED->StripesCount;
    HSSlot* Slot = &HS_SHARED->Slots[SlotIndex];
    HSStripe* Stripe = &HS_SHARED->Stripes[StripeIndex];   

    LOCK_SCOPE_R(Stripe->RWMtx) {
        for (HSNode* Node = Slot->Head; Node; Node = Node->Next) {
            if (Node->Hash == Hash) {
                Result = Node->Data;
                HSScopeTouchNodeSafe(Scope, Node);
                break;
            }
        }
    }

    return Result;
}

void
HSEvictorThreadEntryPoint(void* Params)
{
    ThreadSetName((char*) "[HS] evictor thread");

    for (;;) {
        for (u64 SlotIndex = 0; SlotIndex < HS_SHARED->SlotsCount; ++SlotIndex) {
            u64 StripeIndex = SlotIndex % HS_SHARED->StripesCount;
            HSSlot* Slot = &HS_SHARED->Slots[SlotIndex];
            HSStripe* Stripe = &HS_SHARED->Stripes[StripeIndex];
            b32 SlotHasWork = FALSE;

            LOCK_SCOPE_R(Stripe->RWMtx) {
                for (HSNode* Node = Slot->Head; Node; Node = Node->Next) {
                    u64 KeyRefCount = AtomicLoadU64(&Node->KeyRefCount, MEM_ORDER_SEQ_CST);
                    u64 ScopeRefCount = AtomicLoadU64(&Node->ScopeRefCount, MEM_ORDER_SEQ_CST);
                    u64 DownstreamRefCount = AtomicLoadU64(&Node->DownstreamRefCount, MEM_ORDER_SEQ_CST);

                    if (!KeyRefCount && !ScopeRefCount && !DownstreamRefCount) {
                        SlotHasWork = TRUE;
                        break;
                    }
                }
            }

            if (SlotHasWork) {
                LOCK_SCOPE_W(Stripe->RWMtx) {
                    for (HSNode* Node = Slot->Head, *Next = NULL; Node; Node = Next) {
                        Next = Node->Next;

                        u64 KeyRefCount = AtomicLoadU64(&Node->KeyRefCount, MEM_ORDER_SEQ_CST);
                        u64 ScopeRefCount = AtomicLoadU64(&Node->ScopeRefCount, MEM_ORDER_SEQ_CST);
                        u64 DownstreamRefCount = AtomicLoadU64(&Node->DownstreamRefCount, MEM_ORDER_SEQ_CST);

                        if (!KeyRefCount && !ScopeRefCount && !DownstreamRefCount) {
                            DLL_REMOVE(Slot->Head, Slot->Tail, Node);
                            SLL_STACK_PUSH(HS_SHARED->StripesFreeNodes[StripeIndex], Node);

                            if (Node->MemPool)
                                ArenaRelease(Node->MemPool);
                        }
                    }
                }
            }
        }

        SleepMSecs(1000);
    }
}

#if defined(NC_LANG_CPP)
    b32
    operator==(HSID IDA, HSID IDB)
    {
        return MemCmp(&IDA, &IDB, sizeof(HSID));
    }

    b32
    operator!=(HSID IDA, HSID IDB)
    {
        return !(IDA == IDB);
    }

    b32
    operator==(HSKey KeyA, HSKey KeyB)
    {
        return ((KeyA.ID == KeyB.ID) && MemCmp(&KeyA.Root, &KeyB.Root, sizeof(KeyA.Root)));
    }

    b32
    operator!=(HSKey KeyA, HSKey KeyB)
    {
        return !(KeyA == KeyB);
    }
#endif
