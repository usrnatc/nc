TexShared* TEX_SHARED = NULL;

TexTopology 
TexTopologyAlloc(v2i32 Dim, RendTex2DFmt Format)
{
    TexTopology Result = {};

    Result.Dim.X = (i16) CLAMP(0, Dim.X, I32_MAX);
    Result.Dim.Y = (i16) CLAMP(0, Dim.Y, I32_MAX);
    Result.Format = Format;

    return Result;
}

void 
TexInit(void)
{
    Arena* MemPool = ArenaAlloc();

    TEX_SHARED = ArenaPushArrayZero(MemPool, TexShared, 1);
    TEX_SHARED->MemPool = MemPool;
    TEX_SHARED->SlotsCount = 1024;
    TEX_SHARED->StripesCount = MIN(TEX_SHARED->SlotsCount, GetSystemProperties()->LogicalProcessorCount);
    TEX_SHARED->Slots = ArenaPushArrayZero(MemPool, TexSlot, TEX_SHARED->SlotsCount);
    TEX_SHARED->Stripes = ArenaPushArrayZero(MemPool, TexStripe, TEX_SHARED->StripesCount);
    TEX_SHARED->StripesFreeNodes = ArenaPushArrayZero(MemPool, TexNode*, TEX_SHARED->StripesCount);

    for (u64 Index = 0; Index < TEX_SHARED->StripesCount; ++Index) {
        TEX_SHARED->Stripes[Index].MemPool = ArenaAlloc();
        TEX_SHARED->Stripes[Index].RWMutex = RWMutexAlloc();
        TEX_SHARED->Stripes[Index].CondVar = CondVarAlloc();
    }

    TEX_SHARED->RingSize = KB(64);
    TEX_SHARED->RingBase = ArenaPushArray(MemPool, u8, TEX_SHARED->RingSize);
    TEX_SHARED->RingCondVar = CondVarAlloc();
    TEX_SHARED->RingMutex = MutexAlloc();
    TEX_SHARED->EvictorThread = ThreadLaunch(TexEvictorThreadEntryPoint, NULL, NULL);
}

void 
TexTCTXEnsureInitialised(void)
{
    ThreadLocalStorage* TLS = GetTLS();

    if (!TLS->TEX_TCTX) {
        Arena* MemPool = ArenaAlloc();

        TLS->TEX_TCTX = ArenaPushArrayZero(MemPool, TexTCTX, 1);
        TLS->TEX_TCTX->MemPool = MemPool;
    }
}

TexScope*
TexScopeOpen(void)
{
    TexTCTXEnsureInitialised();

    ThreadLocalStorage* TLS = GetTLS();
    TexScope* Result = TLS->TEX_TCTX->FreeScope;

    if (Result)
        SLL_STACK_POP(TLS->TEX_TCTX->FreeScope);
    else
        Result = ArenaPushArray(TLS->TEX_TCTX->MemPool, TexScope, 1);

    MemSet(Result, 0, sizeof(*Result));

    return Result;
}

void 
TexScopeClose(TexScope* Scope)
{
    ThreadLocalStorage* TLS = GetTLS();

    for (
        TexTouch* Touch = Scope->HeadTouch, *Next = NULL;
        Touch;
        Touch = Next
    ) {
        u128 Hash = Touch->Hash;

        Next = Touch->Next;

        u64 SlotIndex = Hash.U64[1] % TEX_SHARED->SlotsCount;
        u64 StripeIndex = SlotIndex % TEX_SHARED->StripesCount;
        TexSlot* Slot = &TEX_SHARED->Slots[SlotIndex];
        TexStripe* Stripe = &TEX_SHARED->Stripes[StripeIndex];

        LOCK_SCOPE_R(Stripe->RWMutex) {
            for (TexNode* N = Slot->Head; N; N = N->Next) {
                if ((Hash == N->Hash) && MemCmpStruct(&Touch->Topology, &N->Topology)) {
                    AtomicDecFetchU64(&N->ScopeRefCount, MEM_ORDER_SEQ_CST);
                    break;
                }
            }
        }

        SLL_STACK_PUSH(TLS->TEX_TCTX->FreeTouch, Touch);
    }

    SLL_STACK_PUSH(TLS->TEX_TCTX->FreeScope, Scope);
}

void 
TexScopeTouchNodeR(TexScope* Scope, TexNode* Node)
{
    ThreadLocalStorage* TLS = GetTLS();
    TexTouch* Touch = TLS->TEX_TCTX->FreeTouch;

    AtomicIncFetchU64(&Node->ScopeRefCount, MEM_ORDER_SEQ_CST);
    AtomicExchangeU64(&Node->LastTimeTouchedUSecs, TimeNow(), MEM_ORDER_SEQ_CST);
    AtomicExchangeU64(&Node->LastUserClockIndexTouched, UpdateTickIndex(), MEM_ORDER_SEQ_CST);

    if (Touch)
        SLL_STACK_POP(TLS->TEX_TCTX->FreeTouch);
    else
        Touch = ArenaPushArray(TLS->TEX_TCTX->MemPool, TexTouch, 1);

    MemSet(Touch, 0, sizeof(*Touch));
    Touch->Hash = Node->Hash;
    Touch->Topology = Node->Topology;
    SLL_STACK_PUSH(Scope->HeadTouch, Touch);
}

RendHandle 
TexTextureFromHashTopology(TexScope* Scope, u128 Hash, TexTopology Topology)
{
    RendHandle Result = {};
    u64 SlotIndex = Hash.U64[1] % TEX_SHARED->SlotsCount;
    u64 StripeIndex = SlotIndex % TEX_SHARED->StripesCount;
    TexSlot* Slot = &TEX_SHARED->Slots[SlotIndex];
    TexStripe* Stripe = &TEX_SHARED->Stripes[StripeIndex];
    b32 Found = FALSE;
    b32 Stale = FALSE;

    LOCK_SCOPE_R(Stripe->RWMutex) {
        for (
            TexNode* N = Slot->Head;
            N;
            N = N->Next
        ) {
            if ((Hash == N->Hash) && MemCmpStruct(&Topology, &N->Topology)) {
                Result = N->Texture;
                Found = (Result != EMPTY_REND_HANDLE_VALUE);
                TexScopeTouchNodeR(Scope, N);
                break;
            }
        }
    }

    b32 NodeIsNew = FALSE;

    if (!Found) {
        LOCK_SCOPE_W(Stripe->RWMutex) {
            TexNode* Node = NULL;

            for (
                TexNode* N = Slot->Head;
                N;
                N = N->Next
            ) {
                if ((Hash == N->Hash) && MemCmpStruct(&Topology, &N->Topology)) {
                    Node = N;
                    break;
                }
            }

            if (!Node) {
                Node = TEX_SHARED->StripesFreeNodes[StripeIndex];

                if (Node)
                    SLL_STACK_POP(TEX_SHARED->StripesFreeNodes[StripeIndex]);
                else
                    Node = ArenaPushArray(Stripe->MemPool, TexNode, 1);

                MemSet(Node, 0, sizeof(*Node));
                DLL_PUSH_BACK(Slot->Head, Slot->Tail, Node);
                Node->Hash = Hash;
                MemCpy(&Node->Topology, &Topology, sizeof(Node->Topology));
                NodeIsNew = TRUE;
            }
        }
    }

    if (NodeIsNew) {
        TexEnqueueRequest(Hash, Topology, U64_MAX);
        AsyncPushWork(TexTransferWork);
    }

    return Result;
}

RendHandle 
TexTextureFromKeyTopology(
    TexScope* Scope, 
    HSKey Key, 
    TexTopology Topology, 
    OUT u128* Hash
) {
    RendHandle Result = {};

    for (
        u64 RewindIndex = 0;
        RewindIndex < HS_KEY_HASH_HISTORY_COUNT;
        ++RewindIndex
    ) {
        u128 KeyHash = HSHashFromKey(Key, RewindIndex);

        Result = TexTextureFromHashTopology(Scope, KeyHash, Topology);

        if (Result != EMPTY_REND_HANDLE_VALUE) {
            if (Hash)
                *Hash = KeyHash;

            break;
        }
    }

    return Result;
}

b32 
TexEnqueueRequest(u128 Hash, TexTopology Topology, u64 EndTimeUSecs)
{
    b32 IsOkay = FALSE;

    LOCK_SCOPE(TEX_SHARED->RingMutex) {
        for (;;) {
            u64 UnconsumedSize = TEX_SHARED->RingWritePosition - TEX_SHARED->RingReadPosition;
            u64 AvailableSize = TEX_SHARED->RingSize - UnconsumedSize;

            if (AvailableSize >= sizeof(Hash) + sizeof(Topology)) {
                IsOkay = TRUE;
                TEX_SHARED->RingWritePosition += RingWriteStruct(
                    TEX_SHARED->RingBase, 
                    TEX_SHARED->RingSize, 
                    TEX_SHARED->RingWritePosition, 
                    &Hash
                );
                TEX_SHARED->RingWritePosition += RingWriteStruct(
                    TEX_SHARED->RingBase, 
                    TEX_SHARED->RingSize, 
                    TEX_SHARED->RingWritePosition, 
                    &Topology
                );

                break;
            }

            if (TimeNow() >= EndTimeUSecs)
                break;

            CondVarWait(
                TEX_SHARED->RingCondVar, 
                TEX_SHARED->RingMutex, 
                EndTimeUSecs
            );
        }
    }

    if (IsOkay)
        CondVarBroadcast(TEX_SHARED->RingCondVar);

    return IsOkay;
}

void 
TexDequeueRequest(OUT u128* Hash, OUT TexTopology* Topology)
{
    LOCK_SCOPE(TEX_SHARED->RingMutex) {
        for (;;) {
            u64 UnconsumedSize = TEX_SHARED->RingWritePosition - TEX_SHARED->RingReadPosition;

            if (UnconsumedSize >= sizeof(*Hash) + sizeof(*Topology)) {
                TEX_SHARED->RingReadPosition += RingReadStruct(
                    TEX_SHARED->RingBase, 
                    TEX_SHARED->RingSize, 
                    TEX_SHARED->RingReadPosition, 
                    Hash
                );

                TEX_SHARED->RingReadPosition += RingReadStruct(
                    TEX_SHARED->RingBase,
                    TEX_SHARED->RingSize,
                    TEX_SHARED->RingReadPosition,
                    Topology
                );

                break;
            }

            CondVarWait(TEX_SHARED->RingCondVar, TEX_SHARED->RingMutex, U64_MAX);
        }
    }

    CondVarBroadcast(TEX_SHARED->RingCondVar);
}

NC_ASYNC_FUNC_DEF(TexTransferWork)
{
    HSScope* Scope = HSOpenScope();
    u128 Hash = {};
    TexTopology Topology = {};

    TexDequeueRequest(&Hash, &Topology);

    u64 SlotIndex = Hash.U64[1] % TEX_SHARED->SlotsCount;
    u64 StripeIndex = SlotIndex % TEX_SHARED->StripesCount;
    TexSlot* Slot = &TEX_SHARED->Slots[SlotIndex];
    TexStripe* Stripe = &TEX_SHARED->Stripes[StripeIndex];
    b32 HaveTask = FALSE;

    LOCK_SCOPE_R(Stripe->RWMutex) {
        for (
            TexNode* N = Slot->Head;
            N;
            N = N->Next
        ) {
            if ((Hash == N->Hash) && MemCmpStruct(&Topology, &N->Topology)) {
                HaveTask = !AtomicCASU32(
                    &N->IsWorking, 
                    1, 
                    0, 
                    MEM_ORDER_SEQ_CST, 
                    MEM_ORDER_SEQ_CST
                );
                break;
            }
        }
    }

    Str8 Data = {};

    if (HaveTask)
        Data = HSDataFromHash(Scope, Hash);

    RendHandle Texture = {};

    if (
        HaveTask &&
        Topology.Dim.X > 0 &&
        Topology.Dim.Y > 0 &&
        Data.Size >= (u64) Topology.Dim.X * (u64) Topology.Dim.Y * (u64) TEX2D_FMT_BYTES_PER_PIXEL[Topology.Format]
    ) {
        Texture = Tex2DAlloc(
            RESOURCE_KIND_STATIC, 
            Vec((i32) Topology.Dim.X, (i32) Topology.Dim.Y), 
            Topology.Format, 
            Data.Str
        );
    }

    if (HaveTask) {
        LOCK_SCOPE_W(Stripe->RWMutex) {
            for (
                TexNode* N = Slot->Head;
                N;
                N = N->Next
            ) {
                if ((Hash == N->Hash) && MemCmpStruct(&Topology, &N->Topology)) {
                    N->Texture = Texture;
                    AtomicExchangeU32(&N->IsWorking, 0, MEM_ORDER_SEQ_CST);
                    AtomicIncFetchU64(&N->LoadCount, MEM_ORDER_SEQ_CST);
                    break;
                }
            }
        }
    }

    HSCloseScope(Scope);

    return NULL;
}

void
TexEvictorThreadEntryPoint(void* Param)
{
    ThreadSetName("[TEX] Evictor Thread"_s8);

    for (;;) {
        u64 CheckTimeUSecs = TimeNow();
        u64 CheckTimeUserClocks = UpdateTickIndex();
        u64 EvictThresholdUSecs = MILLION(10);
        u64 EvictThresholdUserClocks = 10;

        for (
            u64 SlotIndex = 0;
            SlotIndex < TEX_SHARED->SlotsCount;
            ++SlotIndex
        ) {
            u64 StripeIndex = SlotIndex % TEX_SHARED->StripesCount;
            TexSlot* Slot = &TEX_SHARED->Slots[SlotIndex];
            TexStripe* Stripe = &TEX_SHARED->Stripes[StripeIndex];
            b32 SlotHasWork = FALSE;

            LOCK_SCOPE_R(Stripe->RWMutex) {
                for (
                    TexNode* N = Slot->Head;
                    N;
                    N = N->Next
                ) {
                    if (
                        !N->ScopeRefCount &&
                        N->LastTimeTouchedUSecs + EvictThresholdUSecs <= CheckTimeUSecs &&
                        N->LastUserClockIndexTouched + EvictThresholdUserClocks <= CheckTimeUserClocks &&
                        !N->IsWorking
                    ) {
                        SlotHasWork = TRUE;
                        break;
                    }
                }
            }

            if (SlotHasWork) {
                LOCK_SCOPE_W(Stripe->RWMutex) {
                    for (
                        TexNode* N = Slot->Head, *Next = NULL;
                        N;
                        N = Next
                    ) {
                        Next = N->Next;

                        if (
                            !N->ScopeRefCount &&
                            N->LastTimeTouchedUSecs + EvictThresholdUSecs <= CheckTimeUSecs &&
                            N->LastUserClockIndexTouched + EvictThresholdUserClocks <= CheckTimeUserClocks &&
                            N->LoadCount &&
                            !N->IsWorking
                        ) {
                            DLL_REMOVE(Slot->Head, Slot->Tail, N);

                            if (N->Texture != EMPTY_REND_HANDLE_VALUE)
                                Tex2DRelease(N->Texture);

                            SLL_STACK_PUSH(TEX_SHARED->StripesFreeNodes[StripeIndex], N);
                        }
                    }
                }
            }

            SleepMSecs(5);
        }

        SleepMSecs(THOUSAND(1));
    }
}
