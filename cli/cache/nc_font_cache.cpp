#include "nc_font_cache.h"
#include "nc_font_provider.h"
#include "../nc_arena.h"

#if !defined(XXH_IMPLEMENTATION)
    #define XXH_IMPLEMENTATION
    #define XXH_STATIC_LINKING_ONLY
    #define XXH_INLINE_ALL
    #define XXH_NO_STDLIB
    #include "xxHash/xxhash.h"
#endif

FontState* FONT_STATE = NULL;

u128 
FontHash(Str8 String)
{
    union {
        XXH128_hash_t xxHash;
        u128 V;
    } Result;

    Result.xxHash = XXH3_128bits(String.Str, String.Size);

    return Result.V;
}

u64 
FontHash(u64 Seed, Str8 String)
{
    u64 Result = XXH3_64bits_withSeed(String.Str, String.Size, Seed);

    return Result;
}

v2i32 
FontVertexFromCorner(Corner Cnr)
{
    v2i32 Result = {};

    switch (Cnr) {
        default: {} break;

        case CORNER_00: {
            Result = Vec(0, 0);
        } break;

        case CORNER_01: {
            Result = Vec(0, 1);
        } break;

        case CORNER_10: {
            Result = Vec(1, 0);
        } break;

        case CORNER_11: {
            Result = Vec(1, 1);
        } break;
    }

    return Result;
}

FPHandle 
FPHandleFromTag(FontTag Tag)
{
    u64 SlotIndex = Tag.V[1] % FONT_STATE->FontHashTableSize;
    FontHashNode* FoundNode = NULL;

    for (
        FontHashNode* Node = FONT_STATE->FontHashTable[SlotIndex].Head; 
        Node; 
        Node = Node->Next
    ) {
        if (MemCmpStruct(&Tag, &Node->Tag)) {
            FoundNode = Node;
            break;
        }
    }

    FPHandle Result = {};

    if (FoundNode)
        Result = FoundNode->Handle;

    return Result;
}

FPMetrics 
FPMetricsFromTag(FontTag Tag)
{
    u64 SlotIndex = Tag.V[1] % FONT_STATE->FontHashTableSize;
    FontHashNode* FoundNode = NULL;

    for (
        FontHashNode* Node = FONT_STATE->FontHashTable[SlotIndex].Head;
        Node;
        Node = Node->Next
    ) {
        if (MemCmpStruct(&Tag, &Node->Tag)) {
            FoundNode = Node;
            break;
        }
    }

    FPMetrics Result = {};

    if (FoundNode)
        Result = FoundNode->Metrics;

    return Result;
}

FontTag 
FontTagFromPath(Str8 Path)
{
    FontTag Result = {};
    u128 Hash = FontHash(Path);

    MemCpy(&Result, &Hash, sizeof(Result));
    Result.V[1] |= (1ULL << 63);

    u64 SlotIndex = Result.V[1] % FONT_STATE->FontHashTableSize;
    FontHashNode* FoundNode = NULL;

    for (
        FontHashNode* Node = FONT_STATE->FontHashTable[SlotIndex].Head;
        Node;
        Node = Node->Next
    ) {
        if (MemCmpStruct(&Result, &Node->Tag)) {
            FoundNode = Node;
            break;
        }
    }

    if (!FoundNode) {
        FPHandle FHandle = OpenFont(Path);
        FontHashSlot* Slot = &FONT_STATE->FontHashTable[SlotIndex];

        FoundNode = ArenaPushArray(FONT_STATE->PermanentMemPool, FontHashNode, 1);
        FoundNode->Tag = Result;
        FoundNode->Handle = FHandle;
        FoundNode->Metrics = MetricsFromFont(FoundNode->Handle);
        FoundNode->Path = ArenaPushStrCpy(FONT_STATE->PermanentMemPool, Path);
        SLL_QUEUE_PUSH(Slot->Head, Slot->Tail, FoundNode);
    }

    if (FoundNode->Handle == EMPTY_FPHANDLE_VALUE)
        MemSet(&Result, 0, sizeof(Result));

    return Result;
}

EXTERN_C_LINK FontTag 
FontTagFromData(Str8* Data)
{
    FontTag Result = {};
    u128 Hash = FontHash(Str((u8*) &Data, sizeof(Str8*)));

    MemCpy(&Result, &Hash, sizeof(Result));
    Result.V[1] &= ~(1ULL << 63);

    u64 SlotIndex = Result.V[1] % FONT_STATE->FontHashTableSize;
    FontHashNode* FoundNode = NULL;

    for (
        FontHashNode* Node = FONT_STATE->FontHashTable[SlotIndex].Head;
        Node;
        Node = Node->Next
    ) {
        if (MemCmpStruct(&Result, &Node->Tag)) {
            FoundNode = Node;
            break;
        }
    }

    FontHashNode* NewNode = NULL;

    if (!FoundNode) {
        FontHashSlot* Slot = &FONT_STATE->FontHashTable[SlotIndex];

        NewNode = ArenaPushArrayZero(FONT_STATE->PermanentMemPool, FontHashNode, 1);
        NewNode->Tag = Result;
        NewNode->Handle = FontOpen(Data);
        NewNode->Metrics = MetricsFromFont(NewNode->Handle);
        NewNode->Path = ""_s8;
        SLL_QUEUE_PUSH(Slot->Head, Slot->Tail, NewNode);
    }

    return Result;
}

Str8 
PathFromFontTag(FontTag Tag)
{
    u64 SlotIndex = Tag.V[1] % FONT_STATE->FontHashTableSize;
    FontHashNode* FoundNode = NULL;

    for (
        FontHashNode* Node = FONT_STATE->FontHashTable[SlotIndex].Head;
        Node;
        Node = Node->Next
    ) {
        if (MemCmpStruct(&Tag, &Node->Tag)) {
            FoundNode = Node;
            break;
        }
    }

    Str8 Result = {};

    if (FoundNode)
        Result = FoundNode->Path;

    return Result;
}

r2i16 
FontAtlasRegionAlloc(
    Arena* MemPool, 
    FontAtlas* Atlas, 
    v2i16 SizeRequired
) {
    v2i16 RegionP0 = {};
    v2i16 RegionSize = {};
    Corner NodeCorner = CORNER_INVALID;
    FontAtlasRegionNode* Node = NULL;
    v2i16 NodeSupportedSize = Atlas->RootDimensions;

    for (
        FontAtlasRegionNode* RNode = Atlas->Root, *RNext = NULL;
        RNode;
        RNode = RNext, RNext = NULL
    ) {
        if (RNode->Kind & FONT_ATLAS_REGION_NODE_KIND_TAKEN)
            break;

        b32 NodeCanBeAllocated = !RNode->NumAllocatedDescendants;

        if (NodeCanBeAllocated)
            RegionSize = NodeSupportedSize;

        v2i16 ChildSize = Vec((i16) (NodeSupportedSize.X / 2), (i16) (NodeSupportedSize.Y / 2));
        FontAtlasRegionNode* BestChild = NULL;

        if (ChildSize.X >= SizeRequired.X && ChildSize.Y >= SizeRequired.Y) {
            for (Corner Cnr = (Corner) 0; Cnr < CORNER_COUNT; Cnr = (Corner) (Cnr + 1)) {
                if (!RNode->Children[Cnr]) {
                    RNode->Children[Cnr] = ArenaPushArrayZero(MemPool, FontAtlasRegionNode, 1);
                    RNode->Children[Cnr]->Parent = RNode;
                    RNode->Children[Cnr]->MaxFreeSize[CORNER_00] = Vec((i16) (ChildSize.X / 2), (i16) (ChildSize.Y / 2));
                    RNode->Children[Cnr]->MaxFreeSize[CORNER_01] = Vec((i16) (ChildSize.X / 2), (i16) (ChildSize.Y / 2));
                    RNode->Children[Cnr]->MaxFreeSize[CORNER_10] = Vec((i16) (ChildSize.X / 2), (i16) (ChildSize.Y / 2));
                    RNode->Children[Cnr]->MaxFreeSize[CORNER_11] = Vec((i16) (ChildSize.X / 2), (i16) (ChildSize.Y / 2));
                }

                if (RNode->MaxFreeSize[Cnr].X >= SizeRequired.X && RNode->MaxFreeSize[Cnr].Y >= SizeRequired.Y) {
                    BestChild = RNode->Children[Cnr];
                    NodeCorner = Cnr;

                    v2i32 SideVertex = FontVertexFromCorner(Cnr);

                    RegionP0.X += SideVertex.X * ChildSize.X;
                    RegionP0.Y += SideVertex.Y * ChildSize.Y;
                    break;
                }
            }
        }

        if (NodeCanBeAllocated && !BestChild) {
            Node = RNode;
        } else {
            RNext = BestChild;
            NodeSupportedSize = ChildSize;
        }
    }

    if (Node) {
        Node->Kind = (FontAtlasRegionNodeKind) ((u32) Node->Kind | (u32) FONT_ATLAS_REGION_NODE_KIND_TAKEN);

        if (Node->Parent)
            MemSet(&Node->Parent->MaxFreeSize[NodeCorner], 0, sizeof(Node->Parent->MaxFreeSize[NodeCorner]));

        for (FontAtlasRegionNode* PNode = Node->Parent; PNode; PNode = PNode->Parent) {
            ++PNode->NumAllocatedDescendants;

            FontAtlasRegionNode* Parent = PNode->Parent;

            if (Parent) {
                Corner PCnr = CORNER_INVALID;

                if (PNode == Parent->Children[CORNER_00])
                    PCnr = CORNER_00;
                else if (PNode == Parent->Children[CORNER_01])
                    PCnr = CORNER_01;
                else if (PNode == Parent->Children[CORNER_10])
                    PCnr = CORNER_10;
                else if (PNode == Parent->Children[CORNER_11])
                    PCnr = CORNER_11;

                if (PCnr == CORNER_INVALID)
                    UNREACHABLE();

                Parent->MaxFreeSize[PCnr].X = MAX(
                    MAX(
                        PNode->MaxFreeSize[CORNER_00].X,
                        PNode->MaxFreeSize[CORNER_01].X
                    ),
                    MAX(
                        PNode->MaxFreeSize[CORNER_10].X,
                        PNode->MaxFreeSize[CORNER_11].X
                    )
                );

                Parent->MaxFreeSize[PCnr].Y = MAX(
                    MAX(
                        PNode->MaxFreeSize[CORNER_00].Y,
                        PNode->MaxFreeSize[CORNER_01].Y
                    ),
                    MAX(
                        PNode->MaxFreeSize[CORNER_10].Y,
                        PNode->MaxFreeSize[CORNER_11].Y
                    )
                );
            }
        }
    } else {
        RegionSize = {};
    }

    r2i16 Result = {};

    Result.Point0 = RegionP0;
    Result.Point1 = RegionP0 + RegionSize;

    return Result;
}

void 
FontAtlasRegionRelease(FontAtlas* Atlas, r2i16 Region)
{
    v2i16 RegionSize = Vec((i16) (Region.X1 - Region.X0), (i16) (Region.Y1 - Region.Y0));
    v2i16 CalculatedRegionSize = {};
    FontAtlasRegionNode* Node = NULL;
    Corner NodeCnr = CORNER_INVALID;
    v2i16 NodeP0 = Vec((i16) 0, (i16) 0);
    v2i16 NodeSize = Atlas->RootDimensions;

    for (
        FontAtlasRegionNode* RNode = Atlas->Root, *RNext = NULL;
        RNode;
        RNode = RNext
    ) {
        if (
            NodeP0.X <= Region.Point0.X &&
            NodeP0.X + NodeSize.X > Region.Point0.X &&
            NodeP0.Y <= Region.Point0.Y &&
            NodeP0.Y + NodeSize.Y > Region.Point0.Y
        ) {
            if (
                Region.Point0.X == NodeP0.X &&
                Region.Point0.Y == NodeP0.Y &&
                RegionSize.X == NodeSize.X &&
                RegionSize.Y == NodeSize.Y
            ) {
                Node = RNode;
                CalculatedRegionSize = NodeSize;
                break;
            } else {
                v2i16 RMidPoint = Vec(
                    (i16) (Region.Point0.X + RegionSize.X / 2),
                    (i16) (Region.Point0.Y + RegionSize.Y / 2)
                );

                v2i16 NMidPoint = Vec(
                    (i16) (NodeP0.X + NodeSize.X / 2),
                    (i16) (NodeP0.Y + NodeSize.Y / 2)
                );

                Corner NextCnr = CORNER_INVALID;

                if (RMidPoint.X <= NMidPoint.X && RMidPoint.Y <= NMidPoint.Y)
                    NextCnr = CORNER_00;
                else if (RMidPoint.X <= NMidPoint.X && RMidPoint.Y > NMidPoint.Y)
                    NextCnr = CORNER_01;
                else if (RMidPoint.X > NMidPoint.X && RMidPoint.Y <= NMidPoint.Y)
                    NextCnr = CORNER_10;
                else if (RMidPoint.X > NMidPoint.X && RMidPoint.Y > NMidPoint.Y)
                    NextCnr = CORNER_11;

                RNext = RNode->Children[NextCnr];
                NodeCnr = NextCnr;
                NodeSize.X /= 2;
                NodeSize.Y /= 2;

                v2i32 SideVertex = FontVertexFromCorner(NodeCnr);

                NodeP0.X += SideVertex.X * NodeSize.X;
                NodeP0.Y += SideVertex.Y * NodeSize.Y;
            }
        } else {
            break;
        }
    }

    if (Node && NodeCnr != CORNER_INVALID) {
        Node->Kind &= ~FONT_ATLAS_REGION_NODE_KIND_TAKEN;

        if (Node->Parent)
            Node->Parent->MaxFreeSize[NodeCnr] = CalculatedRegionSize;

        for (FontAtlasRegionNode* PNode = Node->Parent; PNode; PNode = PNode->Parent) {
            --PNode->NumAllocatedDescendants;

            FontAtlasRegionNode* Parent = PNode->Parent;

            if (Parent) {
                Corner PCnr = CORNER_INVALID;

                if (PNode == Parent->Children[CORNER_00])
                    PCnr = CORNER_00;
                else if (PNode == Parent->Children[CORNER_01])
                    PCnr = CORNER_01;
                else if (PNode == Parent->Children[CORNER_10])
                    PCnr = CORNER_10;
                else if (PNode == Parent->Children[CORNER_11])
                    PCnr = CORNER_11;

                if (PCnr == CORNER_INVALID)
                    UNREACHABLE();

                Parent->MaxFreeSize[PCnr].X = MAX(
                    MAX(
                        PNode->MaxFreeSize[CORNER_00].X,
                        PNode->MaxFreeSize[CORNER_01].X
                    ),
                    MAX(
                        PNode->MaxFreeSize[CORNER_10].X,
                        PNode->MaxFreeSize[CORNER_11].X
                    )
                );

                Parent->MaxFreeSize[PCnr].Y = MAX(
                    MAX(
                        PNode->MaxFreeSize[CORNER_00].Y,
                        PNode->MaxFreeSize[CORNER_01].Y
                    ),
                    MAX(
                        PNode->MaxFreeSize[CORNER_10].Y,
                        PNode->MaxFreeSize[CORNER_11].Y
                    )
                );
            }
        }
    }
}

FontPiece* 
ListPush(Arena* MemPool, FontPieceChunkList* List, u64 Capacity)
{
    FontPieceChunkNode* Node = List->Tail;

    if (!Node || Node->Count >= Node->Capacity) {
        Node = ArenaPushArrayZero(MemPool, FontPieceChunkNode, 1);
        Node->V = ArenaPushArray(MemPool, FontPiece, Capacity);
        Node->Capacity = Capacity;
        SLL_QUEUE_PUSH(List->Head, List->Tail, Node);
        ++List->NodeCount;
    }

    FontPiece* Result = Node->V + Node->Count;

    ++Node->Count;
    ++List->TotalPieceCount;

    return Result;
}

void 
ListPush(
    Arena* MemPool, 
    FontPieceChunkList* List, 
    u64 Capacity, 
    FontPiece* Piece
) {
    FontPiece* NewPiece = ListPush(MemPool, List, Capacity);

    MemCpy(NewPiece, Piece, sizeof(*NewPiece));
}

FontPieceArray 
FontPieceArrayFromChunkList(
    Arena* MemPool, 
    FontPieceChunkList* List
) {
    FontPieceArray Result = {};

    Result.Count = List->TotalPieceCount;
    Result.V = ArenaPushArray(MemPool, FontPiece, Result.Count);

    u64 WriteIndex = 0;

    for (FontPieceChunkNode* Node = List->Head; Node; Node = Node->Next) {
        MemCpy(Result.V + WriteIndex, Node->V, Node->Count * sizeof(FontPiece));
        WriteIndex += Node->Count;
    }

    return Result;
}

FontPieceArray 
CopyArray(Arena* MemPool, FontPieceArray* Array)
{
    FontPieceArray Result = {};

    Result.Count = Array->Count;
    Result.V = ArenaPushArray(MemPool, FontPiece, Result.Count);
    MemCpy(Result.V, Array->V, sizeof(FontPiece) * Result.Count);

    return Result;
}

FontHash2StyleRasterCacheNode* 
FontHash2StyleFromTagSizeKind(
    FontTag Tag, 
    f32 Size, 
    FontRasterKind Kind
) {
    u64 StyleHash = 0;
    f32 SizeRounded = FloorF32(Size);
    u32 SizeBits = 0;

    MemCpy(&SizeBits, &SizeRounded, sizeof(SizeBits));

    u64 Buffer[] = {
        Tag.V[0],
        Tag.V[1],
        (u64) SizeBits,
        (u64) Kind
    };

    StyleHash = FontHash(5381, Str((u8*) Buffer, sizeof(Buffer)));

    FontHash2StyleRasterCacheNode* H2SNode = NULL;
    u64 SlotIndex = StyleHash % FONT_STATE->Hash2StyleSlotsCount;
    FontHash2StyleRasterCacheSlot* Slot = &FONT_STATE->Hash2StyleSlots[SlotIndex];

    for (
        FontHash2StyleRasterCacheNode* Node = Slot->Head;
        Node;
        Node = Node->Next
    ) {
        if (Node->StyleHash == StyleHash) {
            H2SNode = Node;
            break;
        }
    }

    if (UNLIKELY(!H2SNode)) {
        FontMetrics Metrics = FontMetricsFromTagSize(Tag, Size);

        H2SNode = ArenaPushArrayZero(FONT_STATE->RasterMemPool, FontHash2StyleRasterCacheNode, 1);
        DLL_PUSH_BACK(Slot->Head, Slot->Tail, H2SNode);
        H2SNode->StyleHash = StyleHash;
        H2SNode->Ascent = Metrics.Ascent;
        H2SNode->Descent = Metrics.Descent;
        H2SNode->UTF8Class1DirectMap = ArenaPushArray(FONT_STATE->RasterMemPool, FontRasterCacheInfo, 256);
        H2SNode->Hash2InfoSlotsCount = 1024;
        H2SNode->Hash2InfoSlots = ArenaPushArrayZero(FONT_STATE->RasterMemPool, FontHash2InfoRasterCacheSlot, H2SNode->Hash2InfoSlotsCount);
    }

    return H2SNode;
}

FontRun 
FontRunFromStr(
    FontTag Tag, 
    f32 Size, 
    f32 BaseAlignPX, 
    f32 TabSizePX, 
    FontRasterKind Kind, 
    Str8 String
) {
    FontHash2StyleRasterCacheNode* H2SNode = FontHash2StyleFromTagSizeKind(Tag, Size, Kind);

    if (H2SNode->RunSlotsFrameIndex != FONT_STATE->FrameIndex || !H2SNode->RunSlots) {
        H2SNode->RunSlotsCount = 1024;
        H2SNode->RunSlots = ArenaPushArrayZero(FONT_STATE->FrameMemPool, FontRunCacheSlot, H2SNode->RunSlotsCount);
        H2SNode->RunSlotsFrameIndex = FONT_STATE->FrameIndex;
    }

    u64 RunHash = FontHash(5381, String);
    u64 RunSlotIndex = RunHash % H2SNode->RunSlotsCount;
    FontRunCacheSlot* RunSlot = &H2SNode->RunSlots[RunSlotIndex];
    FontRunCacheNode* RunNode = NULL;

    for (
        FontRunCacheNode* Node = RunSlot->Head;
        Node;
        Node = Node->Next
    ) {
        if (StrMatch(Node->String, String, 0)) {
            RunNode = Node;
            break;
        }
    }

    b32 RunIsCacheable = TRUE;
    FontRun Run = {};

    if (RunNode) {
        Run = RunNode->Run;
    } else {
        FontPieceChunkList PieceChunks = {};
        v2f32 Dimensions = {};
        b32 FontHandleMappedOnMiss = FALSE;
        FPHandle FontHandle = {};
        u64 PieceSubStringStartIndex = 0;
        u64 PieceSubStringEndIndex = 0;

        for (u64 I = 0; I <= String.Size;) {
            u8 Byte = (I < String.Size) ? String.Str[I] : 0;
            b32 NeedAnotherCodepoint = FALSE;

            if (!Byte) {
                ++I;
            } else {
                switch (UTF8_CLASS[Byte >> 3]) {
                    case 1: {
                        ++I;
                        ++PieceSubStringEndIndex;
                        NeedAnotherCodepoint = FALSE;
                    } break;

                    default: {
                        UnicodeDecode Decode = UTF8Decode(String.Str + I, String.Size - I);

                        I += Decode.Inc;
                        PieceSubStringEndIndex += Decode.Inc;
                        NeedAnotherCodepoint = FALSE;
                    } break;
                }
            }

            if (NeedAnotherCodepoint || PieceSubStringEndIndex == PieceSubStringStartIndex)
                continue;

            if (Byte && Byte != '\t')
                continue;

            Str8 PieceSubString = StrSub(String, Rng(PieceSubStringStartIndex, PieceSubStringEndIndex));

            PieceSubStringStartIndex = I;
            PieceSubStringEndIndex = I;

            b32 IsTab = (PieceSubString.Size == 1 && *PieceSubString.Str == '\t');

            if (IsTab) {
                RunIsCacheable = FALSE;
                PieceSubString = " "_s8;
            }

            FontRasterCacheInfo* Info = NULL;
            u64 PieceHash = 0;

            if (
                PieceSubString.Size == 1 && 
                H2SNode->UTF8Class1DirectMapMask[PieceSubString.Str[0] / 64] & (1UL << (PieceSubString.Str[0] % 64))
            ) {
                Info = &H2SNode->UTF8Class1DirectMap[PieceSubString.Str[0]];
            }

            if (PieceSubString.Size > 1) {
                PieceHash = FontHash(5381, PieceSubString);

                u64 SlotIndex = PieceHash % H2SNode->Hash2InfoSlotsCount;
                FontHash2InfoRasterCacheSlot* Slot = &H2SNode->Hash2InfoSlots[SlotIndex];

                for (
                    FontHash2InfoRasterCacheNode* Node = Slot->Head; 
                    Node; 
                    Node = Node->Next
                ) {
                    if (Node->Hash == PieceHash) {
                        Info = &Node->Info;
                        break;
                    }
                }
            }

            if (!Info) {
                TempArena Scratch = GetScratch(NULL, 0);

                if (!FontHandleMappedOnMiss) {
                    FontHandleMappedOnMiss = TRUE;

                    u64 FontSlotIndex = Tag.V[1] % FONT_STATE->FontHashTableSize;
                    FontHashNode* FoundNode = NULL;

                    for (
                        FontHashNode* Node = FONT_STATE->FontHashTable[FontSlotIndex].Head;
                        Node;
                        Node = Node->Next
                    ) {
                        if (MemCmpStruct(&Tag, &Node->Tag)) {
                            FoundNode = Node;
                            break;
                        }
                    }

                    if (FoundNode)
                        FontHandle = FoundNode->Handle;
                }

                FPRasterResult FPRaster = {};

                if (Size > 0.0f) {
                    FPRasterKind FPKind = 0;

                    if (Kind & FONT_RASTER_KIND_SMOOTH)
                        FPKind |= FP_RASTER_KIND_SMOOTH;

                    if (Kind & FONT_RASTER_KIND_HINTED)
                        FPKind |= FP_RASTER_KIND_HINTED;

                    FPRaster = Raster(Scratch.MemPool, FontHandle, FloorF32(Size), FPKind, PieceSubString);
                }

                i16 ChosenAtlasNum = 0;
                FontAtlas* ChosenAtlas = NULL;
                r2i16 ChosenAtlasRegion = {};

                if (FPRaster.AtlasDimensions.X != 0 && FPRaster.AtlasDimensions.Y != 0) {
                    u64 NumAtlases = 0;

                    for (FontAtlas* Atlas = FONT_STATE->HeadAtlas; ; Atlas = Atlas->Next, ++NumAtlases) {
                        if (!Atlas && NumAtlases < 64) {
                            Atlas = ArenaPushArrayZero(FONT_STATE->RasterMemPool, FontAtlas, 1);
                            DLL_PUSH_BACK(FONT_STATE->HeadAtlas, FONT_STATE->TailAtlas, Atlas);
                            Atlas->RootDimensions = Vec((i16) 1024, (i16) 1024);
                            Atlas->Root = ArenaPushArrayZero(FONT_STATE->RasterMemPool, FontAtlasRegionNode, 1);
                            Atlas->Root->MaxFreeSize[CORNER_00] = Vec(
                                (i16) (Atlas->RootDimensions.X / 2), 
                                (i16) (Atlas->RootDimensions.Y / 2)
                            );
                            Atlas->Root->MaxFreeSize[CORNER_01] = Vec(
                                (i16) (Atlas->RootDimensions.X / 2), 
                                (i16) (Atlas->RootDimensions.Y / 2)
                            );
                            Atlas->Root->MaxFreeSize[CORNER_10] = Vec(
                                (i16) (Atlas->RootDimensions.X / 2), 
                                (i16) (Atlas->RootDimensions.Y / 2)
                            );
                            Atlas->Root->MaxFreeSize[CORNER_11] = Vec(
                                (i16) (Atlas->RootDimensions.X / 2), 
                                (i16) (Atlas->RootDimensions.Y / 2)
                            );
                            Atlas->Texture = Tex2DAlloc(
                                RESOURCE_KIND_DYNAMIC, 
                                Vec((i32) Atlas->RootDimensions.X, (i32) Atlas->RootDimensions.Y),
                                TEX2D_FMT_RGBA8,
                                NULL
                            );
                        }

                        if (Atlas) {
                            v2i16 DimensionsRequired = Vec(
                                (i16) (FPRaster.AtlasDimensions.X + 2), 
                                (i16) (FPRaster.AtlasDimensions.Y + 2)
                            );

                            ChosenAtlasRegion = FontAtlasRegionAlloc(FONT_STATE->RasterMemPool, Atlas, DimensionsRequired);

                            if (ChosenAtlasRegion.X1 != ChosenAtlasRegion.X0) {
                                ChosenAtlas = Atlas;
                                ChosenAtlasNum = (i32) NumAtlases;
                                break;
                            }
                        } else {
                            break;
                        }
                    }
                }

                if (ChosenAtlas) {
                    r2i32 SubRegion = {
                        ChosenAtlasRegion.X0,
                        ChosenAtlasRegion.Y0,
                        ChosenAtlasRegion.X0 + FPRaster.AtlasDimensions.X,
                        ChosenAtlasRegion.Y0 + FPRaster.AtlasDimensions.Y
                    };

                    FillTex2DRgn(ChosenAtlas->Texture, SubRegion, FPRaster.Atlas);
                }

                if (PieceSubString.Size == 1) {
                    Info = &H2SNode->UTF8Class1DirectMap[PieceSubString.Str[0]];
                    H2SNode->UTF8Class1DirectMapMask[PieceSubString.Str[0] / 64] |= (1ULL << (PieceSubString.Str[0] % 64));
                } else {
                    u64 SlotIndex = PieceHash % H2SNode->Hash2InfoSlotsCount;
                    FontHash2InfoRasterCacheSlot* Slot = &H2SNode->Hash2InfoSlots[SlotIndex];
                    FontHash2InfoRasterCacheNode* Node = ArenaPushArray(FONT_STATE->RasterMemPool, FontHash2InfoRasterCacheNode, 1);

                    DLL_PUSH_BACK(Slot->Head, Slot->Tail, Node);
                    Node->Hash = PieceHash;
                    Info = &Node->Info;
                }

                if (Info) {
                    Info->SubRect = ChosenAtlasRegion;
                    Info->AtlasNumber = ChosenAtlasNum;
                    Info->RasterDimensions = FPRaster.AtlasDimensions;
                    Info->Advance = FPRaster.Advance;
                }

                ReleaseScratch(Scratch);
            }

            if (Info) {
                FontAtlas* Atlas = NULL;

                if (Info->SubRect.X1 != 0 && Info->SubRect.Y1 != 0) {
                    i32 Num = 0;

                    for (FontAtlas* A = FONT_STATE->HeadAtlas; A; A = A->Next, ++Num) {
                        if (Info->AtlasNumber == Num) {
                            Atlas = A;
                            break;
                        }
                    }
                }

                f32 Advance = Info->Advance;

                if (IsTab)
                    Advance = FloorF32(TabSizePX) - FMod(FloorF32(BaseAlignPX), FloorF32(TabSizePX));

                FontPiece* Piece = ListPush(FONT_STATE->FrameMemPool, &PieceChunks, String.Size);

                Piece->Texture = (Atlas) ? Atlas->Texture : EMPTY_REND_HANDLE_VALUE;
                Piece->SubRect = Rng(
                    Vec((i16) Info->SubRect.X0, (i16) Info->SubRect.Y0),
                    Vec((i16) (Info->SubRect.X0 + Info->RasterDimensions.X), (i16) (Info->SubRect.Y0 + Info->RasterDimensions.Y))
                );
                Piece->Advance = Advance;
                Piece->DecodeSize = PieceSubString.Size;
                Piece->Offset = Vec((i16) 0, (i16) -(H2SNode->Ascent + H2SNode->Descent));
                BaseAlignPX += Advance;
                Dimensions.X += Piece->Advance;
                Dimensions.Y = MAX(Dimensions.Y, Info->RasterDimensions.Y);
            }
        }

        if (PieceChunks.NodeCount == 1) {
            Run.Pieces.V = PieceChunks.Head->V;
            Run.Pieces.Count = PieceChunks.Head->Count;
        } else {
            Run.Pieces = FontPieceArrayFromChunkList(FONT_STATE->FrameMemPool, &PieceChunks);
        }

        Run.Dimensions = Dimensions;
        Run.Ascent = H2SNode->Ascent;
        Run.Descent = H2SNode->Descent;
    }

    if (RunIsCacheable) {
        RunNode = ArenaPushArrayZero(FONT_STATE->FrameMemPool, FontRunCacheNode, 1);
        SLL_QUEUE_PUSH(RunSlot->Head, RunSlot->Tail, RunNode);
        RunNode->String = ArenaPushStrCpy(FONT_STATE->FrameMemPool, String);
        RunNode->Run = Run;
    }

    return Run;
}

Str8List 
FontWrappedStringLinesFromFontSizeStringMax(
    Arena* MemPool, 
    FontTag Tag, 
    f32 Size, 
    f32 BaseAlignPX, 
    f32 TabSizePX, 
    Str8 String, 
    f32 Max
) {
    Str8List Result = {};
    TempArena Scratch = GetScratch(&MemPool, 1);
    FontRun Run = FontRunFromStr(Tag, Size, BaseAlignPX, TabSizePX, 0, String);
    f32 OffPX = 0.0f;
    u64 OffBytes = 0;
    u64 LineStartOffBytes = 0;
    u64 LineEndOffBytes = 0;
    b32 SeekingWordEnd = FALSE;
    f32 WordStartOffPX = 0.0f;
    FontPiece* LastWordStartPiece = NULL;
    u64 LastWordStartOffBytes = 0;
    FontPiece* PiecesFirst = Run.Pieces.V;
    FontPiece* PiecesEnd = Run.Pieces.V + Run.Pieces.Count;

    for (
        FontPiece* Piece = PiecesFirst, *Next = NULL; 
        Piece && Piece <= PiecesEnd; 
        Piece = Next
    ) {
        if (Piece)
            Next = Piece + 1;

        u8 Byte = (OffBytes < String.Size) ? String.Str[OffBytes] : 0;
        f32 Advance = (Piece) ? Piece->Advance : 0.0f;
        u64 DecodeSize = (Piece) ? Piece->DecodeSize : 0;
        b32 IsFirstByteOfWord = FALSE;
        b32 IsFirstSpaceAfterWord = FALSE;

        if (!SeekingWordEnd && !IsSpace(Byte)) {
            SeekingWordEnd = TRUE;
            IsFirstByteOfWord = TRUE;
            LastWordStartOffBytes = OffBytes;
            LastWordStartPiece = Piece;
            WordStartOffPX = OffPX;
        } else if (SeekingWordEnd && IsSpace(Byte)) {
            SeekingWordEnd = FALSE;
            IsFirstSpaceAfterWord = TRUE;
        } else if (SeekingWordEnd && !Byte) {
            IsFirstSpaceAfterWord = TRUE;
        }

        b32 IsIllegal = (OffPX >= Max);
        b32 IsNextIllegal = (OffPX + Advance >= Max);
        b32 IsEnd = !Byte;

        if (IsFirstSpaceAfterWord && !IsIllegal)
            LineEndOffBytes = OffBytes;

        if (IsNextIllegal && WordStartOffPX == 0.0f) {
            Str8 Line = Str(String.Str + LineStartOffBytes, OffBytes - LineStartOffBytes);

            Line = StrSkipChopWhitespace(Line);

            if (Line.Size)
                ListPush(MemPool, &Result, Line);

            OffPX = Advance;
            LineStartOffBytes = OffBytes;
            LineEndOffBytes = OffBytes;
            WordStartOffPX = 0;
            LastWordStartPiece = Piece;
            LastWordStartOffBytes = OffBytes;
            OffBytes += DecodeSize;
        } else if (IsFirstSpaceAfterWord && (IsIllegal || IsEnd)) {
            Str8 Line = Str(String.Str + LineStartOffBytes, LineEndOffBytes - LineStartOffBytes);

            Line = StrSkipChopWhitespace(Line);

            if (Line.Size)
                ListPush(MemPool, &Result, Line);

            LineStartOffBytes = LineEndOffBytes;

            if (IsIllegal) {
                OffPX = 0;
                WordStartOffPX = 0;
                OffBytes = LastWordStartOffBytes;
                Next = LastWordStartPiece;
            }
        } else {
            OffPX += Advance;
            OffBytes += DecodeSize;
        }

        if (!Piece && !Next)
            break;
    }

    ReleaseScratch(Scratch);

    return Result;
}

v2f32 
FontDimensionsFromTagSizeString(
    FontTag Tag, 
    f32 Size, 
    f32 BaseAlignPX, 
    f32 TabSizePX, 
    Str8 String
) {
    TempArena Scratch = GetScratch(NULL, 0);
    v2f32 Result = {};
    FontRun Run = FontRunFromStr(Tag, Size, BaseAlignPX, TabSizePX, 0, String);

    Result = Run.Dimensions;
    ReleaseScratch(Scratch);

    return Result;
}

v2f32 
FontDimensionsFromTagSizeStringList(
    FontTag Tag, 
    f32 Size, 
    f32 BaseAlignPX, 
    f32 TabSizePX, 
    Str8List List
) {
    v2f32 Result = {};

    for (Str8Node* Node = List.Head; Node; Node = Node->Next) {
        v2f32 StrDimensions = FontDimensionsFromTagSizeString(Tag, Size, BaseAlignPX, TabSizePX, Node->String);

        Result.X += StrDimensions.X;
        Result.Y = MAX(Result.Y, StrDimensions.Y);
    }

    return Result;
}

f32 
FontColumnSizeFromTagSize(FontTag Tag, f32 Size)
{
    f32 Result = FontDimensionsFromTagSizeString(Tag, Size, 0, 0, "H"_s8).X;

    return Result;
}

u64 
FontCharPositionFromTagSizeString(
    FontTag Tag, 
    f32 Size, 
    f32 BaseAlignPX, 
    f32 TabSizePX, 
    Str8 String, 
    f32 P
) {
    TempArena Scratch = GetScratch(NULL, 0);
    u64 BestOffsetBytes = 0;
    f32 BestOffsetPX = INF32;
    u64 OffsetBytes = 0;
    f32 OffsetPX = 0.0f;
    FontRun Run = FontRunFromStr(Tag, Size, BaseAlignPX, TabSizePX, 0, String);

    for (u64 I = 0; I <= Run.Pieces.Count; ++I) {
        f32 ThisPieceOffsetPX = AbsF32(OffsetPX - P);

        if (ThisPieceOffsetPX < BestOffsetPX) {
            BestOffsetBytes = OffsetBytes;
            BestOffsetPX = ThisPieceOffsetPX;
        }

        if (I < Run.Pieces.Count) {
            FontPiece* Piece = &Run.Pieces.V[I];

            OffsetPX += Piece->Advance;
            OffsetBytes += Piece->DecodeSize;
        }
    }

    ReleaseScratch(Scratch);

    return BestOffsetBytes;
}

FontMetrics 
FontMetricsFromTagSize(FontTag Tag, f32 Size)
{
    FPMetrics Metrics = FPMetricsFromTag(Tag);
    FontMetrics Result = {};
    const f32 SizeFloored = FloorF32(Size);

    Result.Ascent = SizeFloored * Metrics.Ascent / Metrics.DesignUnitsPerEM;
    Result.Descent = SizeFloored * Metrics.Descent / Metrics.DesignUnitsPerEM;
    Result.LineGap = SizeFloored * Metrics.LineGap / Metrics.DesignUnitsPerEM;
    Result.CapitalHeight = SizeFloored * Metrics.CapitalHeight / Metrics.DesignUnitsPerEM;

    return Result;
}

f32 
FontLineHeightFromMetrics(FontMetrics* Metrics)
{
    return Metrics->Ascent + Metrics->Descent + Metrics->LineGap;
}

void 
FontInit(void)
{
    Arena* MemPool = ArenaAlloc();

    FONT_STATE = ArenaPushArrayZero(MemPool, FontState, 1);
    FONT_STATE->PermanentMemPool = MemPool;
    FONT_STATE->RasterMemPool = ArenaAlloc();
    FONT_STATE->FrameMemPool = ArenaAlloc();
    FONT_STATE->FontHashTableSize = 64;
    FONT_STATE->FontHashTable = ArenaPushArrayZero(
        FONT_STATE->PermanentMemPool, 
        FontHashSlot, 
        FONT_STATE->FontHashTableSize
    );
    FontReset();
}

void 
FontReset(void)
{
    for (FontAtlas* Atlas = FONT_STATE->HeadAtlas; Atlas; Atlas = Atlas->Next) {
        Tex2DRelease(Atlas->Texture);
        MemSet(Atlas, 0, sizeof(*Atlas));
    }

    FONT_STATE->HeadAtlas = NULL;
    FONT_STATE->TailAtlas = NULL;
    ArenaClear(FONT_STATE->RasterMemPool);
    FONT_STATE->Hash2StyleSlotsCount = 1024;
    FONT_STATE->Hash2StyleSlots = ArenaPushArrayZero(
        FONT_STATE->RasterMemPool, 
        FontHash2StyleRasterCacheSlot, 
        FONT_STATE->Hash2StyleSlotsCount
    );
}

void 
FontFrame(void)
{
    ++FONT_STATE->FrameIndex;
    ArenaClear(FONT_STATE->FrameMemPool);
}
