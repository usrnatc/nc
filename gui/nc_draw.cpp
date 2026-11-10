#include "nc_render.h"
#include "nc_types.h"
#include "nc_arena.h"
#include "nc_math.h"
#include "nc_font_cache.h"
#include "nc_string.h"
#include "nc_draw.h"
#include "nc_system.h"
#include "nc_tls.h"

RendTex2DSampleKind
DPushTex2DSampleKind(RendTex2DSampleKind Kind)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucket* Bucket = DTopBucket();
    RendTex2DSampleKind OldValue = Bucket->HeadTex2DSampleKind->V;
    Texture2DSampleKindNode* Node = ArenaPushArray(
        TLS->D_THREAD_CTX->MemPool, 
        Texture2DSampleKindNode, 
        1
    );

    Node->V = Kind;
    SLL_STACK_PUSH(Bucket->HeadTex2DSampleKind, Node);
    ++Bucket->StackGeneration;

    return OldValue;
}

m3f32
DPushXForm2D(m3f32 Mat)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucket* Bucket = DTopBucket();
    m3f32 OldValue = Bucket->HeadXForm2D->V;
    XForm2DNode* Node = ArenaPushArray(TLS->D_THREAD_CTX->MemPool, XForm2DNode, 1);

    Node->V = Mat;
    SLL_STACK_PUSH(Bucket->HeadXForm2D, Node);
    ++Bucket->StackGeneration;

    return OldValue;
}

r2f32
DPushClip(r2f32 Clip)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucket* Bucket = DTopBucket();
    r2f32 OldValue = Bucket->HeadClip->V;
    ClipNode* Node = ArenaPushArray(TLS->D_THREAD_CTX->MemPool, ClipNode, 1);

    Node->V = Clip;
    SLL_STACK_PUSH(Bucket->HeadClip, Node);
    ++Bucket->StackGeneration;

    return OldValue;
}

f32
DPushTransparency(f32 Transparency)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucket* Bucket = DTopBucket();
    f32 OldValue = Bucket->HeadTransparency->V;
    TransparencyNode* Node = ArenaPushArray(
        TLS->D_THREAD_CTX->MemPool, 
        TransparencyNode, 
        1
    );

    Node->V = Transparency;
    SLL_STACK_PUSH(Bucket->HeadTransparency, Node);
    ++Bucket->StackGeneration;

    return OldValue;
}

RendTex2DSampleKind
DPopTex2DSampleKind(void)
{
    DBucket* Bucket = DTopBucket();
    RendTex2DSampleKind PoppedVal = Bucket->HeadTex2DSampleKind->V;

    SLL_STACK_POP(Bucket->HeadTex2DSampleKind);
    ++Bucket->StackGeneration;

    return PoppedVal;
}

m3f32
DPopXForm2D(void)
{
    DBucket* Bucket = DTopBucket();
    m3f32 PoppedVal = Bucket->HeadXForm2D->V;

    SLL_STACK_POP(Bucket->HeadXForm2D);
    ++Bucket->StackGeneration;

    return PoppedVal;
}

r2f32
DPopClip(void)
{
    DBucket* Bucket = DTopBucket();
    r2f32 PoppedVal = Bucket->HeadClip->V;

    SLL_STACK_POP(Bucket->HeadClip);
    ++Bucket->StackGeneration;

    return PoppedVal;
}

f32
DPopTransparency(void)
{
    DBucket* Bucket = DTopBucket();
    f32 PoppedVal = Bucket->HeadTransparency->V;

    SLL_STACK_POP(Bucket->HeadTransparency);
    ++Bucket->StackGeneration;

    return PoppedVal;
}

RendTex2DSampleKind
DTopTex2DSampleKind(void)
{
    DBucket* Bucket = DTopBucket();
    RendTex2DSampleKind TopVal = Bucket->HeadTex2DSampleKind->V;

    return TopVal;
}

m3f32
DTopXForm2D(void)
{
    DBucket* Bucket = DTopBucket();
    m3f32 TopVal = Bucket->HeadXForm2D->V;

    return TopVal;
}

r2f32
DTopClip(void)
{
    DBucket* Bucket = DTopBucket();
    r2f32 TopVal = Bucket->HeadClip->V;

    return TopVal;
}

f32
DTopTransparency(void)
{
    DBucket* Bucket = DTopBucket();
    f32 TopVal = Bucket->HeadTransparency->V;

    return TopVal;
}

u64 
DHash(Str8 String)
{
    u64 Result = 5381;

    for (u64 I = 0; I < String.Size; ++I)
        Result = ((Result << 5) + Result) + String.Str[I];

    return Result;
}

void 
ListPush(Arena* MemPool, FancyStrList* List, FancyStr* String)
{
    FancyStrNode* Node = ArenaPushArray(MemPool, FancyStrNode, 1);

    MemCpy(&Node->V, String, sizeof(Node->V));
    SLL_QUEUE_PUSH(List->Head, List->Tail, Node);
    ++List->NodeCount;
    List->TotalSize += String->String.Size;
}

void 
ListPush(
    Arena* MemPool, 
    FancyStrList* List, 
    FancyStrParams* Params, 
    FancyStrParams* Overrides, 
    Str8 String
) {
    FancyStr FString = {};

    FString.String = String;
    FString.Params = *Params;

    if (Overrides) {
        if (Overrides->Font != EMPTY_FONT_TAG_VALUE)
            FString.Params.Font = Overrides->Font;

        if (Overrides->RasterKind != 0)
            FString.Params.RasterKind = Overrides->RasterKind;

        if (
            Overrides->Colour.X != 0.0f ||
            Overrides->Colour.Y != 0.0f ||
            Overrides->Colour.Z != 0.0f ||
            Overrides->Colour.W != 0.0f
        ) {
            FString.Params.Colour = Overrides->Colour;
        }

        if (Overrides->Size > 0.0f)
            FString.Params.Size = Overrides->Size;

        if (Overrides->UnderlineThickness > 0.0f)
            FString.Params.UnderlineThickness = Overrides->UnderlineThickness;

        if (Overrides->StrikethroughThickness > 0.0f)
            FString.Params.StrikethroughThickness = Overrides->StrikethroughThickness;
    }

    ListPush(MemPool, List, &FString);
}

void 
ListCat(FancyStrList* Dst, FancyStrList* Src)
{
    if (Dst->Tail && Src->Head) {
        Dst->Tail->Next = Src->Head;
        Dst->Tail = Src->Tail;
        Dst->TotalSize += Src->TotalSize;
        Dst->NodeCount += Src->NodeCount;
    } else if (Src->Head) {
        MemCpy(Dst, Src, sizeof(*Dst));
    }

    MemSet(Src, 0, sizeof(*Src));
}

FancyStrList 
ListCpy(Arena* MemPool, FancyStrList* List)
{
    FancyStrList Result = {};

    for (FancyStrNode* Node = List->Head; Node; Node = Node->Next) {
        FancyStr FString = Node->V;

        FString.String = ArenaPushStrCpy(MemPool, FString.String);
        ListPush(MemPool, &Result, &FString);
    }

    return Result;
}

Str8 
Str8FromFancyStrList(Arena* MemPool, FancyStrList* List)
{
    ThreadLocalStorage* TLS = GetTLS();
    Str8 Result = {};
    TempArena Scratch = GetScratch(&MemPool, 1);
    Str8List Parts = {};

    for (FancyStrNode* Node = List->Head; Node; Node = Node->Next) {
        if (Node->V.Params.Font != TLS->D_THREAD_CTX->IconFont)
            ListPush(Scratch.MemPool, &Parts, Node->V.String);
    }

    Result = StrListJoin(MemPool, &Parts, NULL);
    Result = StrSkipChopWhitespace(Result);
    ReleaseScratch(Scratch);

    return Result;
}

FancyRunList 
FancyRunListFromFancyStrList(Arena* MemPool, f32 TabSizePX, FancyStrList* List)
{
    FancyRunList Result = {};
    f32 BaseAlignPX = 0.0f;

    for (FancyStrNode* Node = List->Head; Node; Node = Node->Next) {
        FancyRunNode* Dst = ArenaPushArrayZero(MemPool, FancyRunNode, 1);

        Dst->V.Run = FontRunFromStr(
            Node->V.Params.Font, 
            Node->V.Params.Size, 
            BaseAlignPX, 
            TabSizePX, 
            Node->V.Params.RasterKind, 
            Node->V.String
        );

        Dst->V.Colour = Node->V.Params.Colour;
        Dst->V.UnderlineThickness = Node->V.Params.UnderlineThickness;
        Dst->V.StrikethroughThickness = Node->V.Params.StrikethroughThickness;
        Dst->V.Icon = (Node->V.Params.Font == GetTLS()->D_THREAD_CTX->IconFont);
        SLL_QUEUE_PUSH(Result.Head, Result.Tail, Dst);
        ++Result.NodeCount;
        Result.Dimensions.X += Dst->V.Run.Dimensions.X;
        Result.Dimensions.Y = MAX(Result.Dimensions.Y, Dst->V.Run.Dimensions.Y);
        BaseAlignPX = Dst->V.Run.Dimensions.X;
    }

    return Result;
}

v2f32 
DDimensionsFromFancyStrList(f32 TabSizePX, FancyStrList* List)
{
    TempArena Scratch = GetScratch(NULL, 0);
    FancyRunList Runs = FancyRunListFromFancyStrList(Scratch.MemPool, TabSizePX, List);
    v2f32 Result = Runs.Dimensions;

    ReleaseScratch(Scratch);

    return Result;
}

void 
DBeginFrame(FontTag IconFont)
{
    ThreadLocalStorage* TLS = GetTLS();

    if (!TLS->D_THREAD_CTX) {
        Arena* MemPool = ArenaAlloc(
            __Params.ReserveSize = GB(64), 
            __Params.CommitSize = MB(8)
        );

        TLS->D_THREAD_CTX = ArenaPushArrayZero(MemPool, DThreadContext, 1);
        TLS->D_THREAD_CTX->MemPool = MemPool;
        TLS->D_THREAD_CTX->MemPoolFrameStartPosition = ArenaGetPosition(MemPool);
    }

    ArenaPopTo(
        TLS->D_THREAD_CTX->MemPool, 
        TLS->D_THREAD_CTX->MemPoolFrameStartPosition
    );
    TLS->D_THREAD_CTX->FreeBucketSelection = NULL;
    TLS->D_THREAD_CTX->HeadBucket = NULL;
    TLS->D_THREAD_CTX->IconFont = IconFont;
}

void 
DSubmitBucket(Handle WindowHandle, RendHandle RendHandle, DBucket* Bucket)
{    
    RendSubmitWindow(WindowHandle, RendHandle, &Bucket->Passes);
}

DBucket* 
DBucketAlloc(void)
{
    DBucket* Bucket = ArenaPushArrayZero(
        GetTLS()->D_THREAD_CTX->MemPool, 
        DBucket, 
        1
    );

    DBucketStackInit(Bucket);

    return Bucket;
}

void 
DPushBucket(DBucket* Bucket)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucketSelectionNode* Node = TLS->D_THREAD_CTX->FreeBucketSelection;

    if (Node) {
        SLL_STACK_POP(TLS->D_THREAD_CTX->FreeBucketSelection);
    } else {
        Node = ArenaPushArray(TLS->D_THREAD_CTX->MemPool, DBucketSelectionNode, 1);
    }

    SLL_STACK_PUSH(TLS->D_THREAD_CTX->HeadBucket, Node);
    Node->Bucket = Bucket;
}

void 
DPopBucket(void)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucketSelectionNode* Node = TLS->D_THREAD_CTX->HeadBucket;
    SLL_STACK_POP(TLS->D_THREAD_CTX->HeadBucket);
    SLL_STACK_PUSH(TLS->D_THREAD_CTX->FreeBucketSelection, Node);
}

DBucket* 
DTopBucket(void)
{
    ThreadLocalStorage* TLS = GetTLS();
    DBucket* Result = NULL;

    if (TLS->D_THREAD_CTX->HeadBucket)
        Result = TLS->D_THREAD_CTX->HeadBucket->Bucket;

    return Result;
}

Rect2DInstr*
DrawRect(
    r2f32 Destination, 
    v4f32 Colour, 
    f32 CornerRadius, 
    f32 BorderThickness, 
    f32 EdgeSoftness
) {
    Arena* MemPool = GetTLS()->D_THREAD_CTX->MemPool;
    DBucket* Bucket = DTopBucket();
    RendPass* Pass = RendPassFromKind(MemPool, &Bucket->Passes, PASS_KIND_UI);
    RendPassUIParams* Params = Pass->UIParams;
    RendBatch2DList* Rects = &Params->Rects;
    RendBatch2DNode* Node = Rects->Tail;

    if (
        !Node || 
        Bucket->StackGeneration != Bucket->LastCommandStackgeneration
    ) {
        Node = ArenaPushArrayZero(MemPool, RendBatch2DNode, 1);
        SLL_QUEUE_PUSH(Rects->Head, Rects->Tail, Node);
        ++Rects->Count;
        Node->Batches = RendBatchListAlloc(sizeof(Rect2DInstr));
        Node->Params.Texture = EMPTY_REND_HANDLE_VALUE;
        Node->Params.SampleKind = Bucket->HeadTex2DSampleKind->V;
        Node->Params.XForm = Bucket->HeadXForm2D->V;
        Node->Params.Clip = Bucket->HeadClip->V;
        Node->Params.Transparency = Bucket->HeadTransparency->V;
    }

    Rect2DInstr* Instruction = (Rect2DInstr*) ListPush(
        MemPool,
        &Node->Batches,
        256
    );

    Instruction->Dst = Destination;
    Instruction->Src = Rng(Vec(0.0f, 0.0f), Vec(0.0f, 0.0f));
    Instruction->Colours[CORNER_00] = Colour;
    Instruction->Colours[CORNER_01] = Colour;
    Instruction->Colours[CORNER_10] = Colour;
    Instruction->Colours[CORNER_11] = Colour;
    Instruction->CornerRadii[CORNER_00] = CornerRadius;
    Instruction->CornerRadii[CORNER_01] = CornerRadius;
    Instruction->CornerRadii[CORNER_10] = CornerRadius;
    Instruction->CornerRadii[CORNER_11] = CornerRadius;
    Instruction->BorderThickness = BorderThickness;
    Instruction->EdgeSoftness = EdgeSoftness;
    Instruction->WhiteTextureOverride = 1.0f;
    Instruction->Sheer = 0.0f;
    Bucket->LastCommandStackgeneration = Bucket->StackGeneration;

    return Instruction;
}

Rect2DInstr*
DrawImage(
    r2f32 Destination, 
    r2f32 Source, 
    RendHandle Texture, 
    v4f32 Colour, 
    f32 CornerRadius, 
    f32 BorderThickness, 
    f32 EdgeSoftness
) {
    ThreadLocalStorage* TLS = GetTLS();
    Arena* MemPool = TLS->D_THREAD_CTX->MemPool;
    DBucket* Bucket = DTopBucket();
    RendPass* Pass = RendPassFromKind(MemPool, &Bucket->Passes, PASS_KIND_UI);
    RendPassUIParams* Params = Pass->UIParams;
    RendBatch2DList* Rects = &Params->Rects;
    RendBatch2DNode* Node = Rects->Tail;

    if (
        Node && 
        Bucket->StackGeneration == Bucket->LastCommandStackgeneration && 
        (Node->Params.Texture == EMPTY_REND_HANDLE_VALUE)
    ) {
        Node->Params.Texture = Texture;
    } else if (
        !Node || 
        Bucket->StackGeneration != Bucket->LastCommandStackgeneration || 
        (Texture != Node->Params.Texture)
    ) {
        Node = ArenaPushArrayZero(MemPool, RendBatch2DNode, 1);
        SLL_QUEUE_PUSH(Rects->Head, Rects->Tail, Node);
        ++Rects->Count;
        Node->Batches = RendBatchListAlloc(sizeof(Rect2DInstr));
        Node->Params.Texture = Texture;
        Node->Params.SampleKind = Bucket->HeadTex2DSampleKind->V;
        Node->Params.XForm = Bucket->HeadXForm2D->V;
        Node->Params.Clip = Bucket->HeadClip->V;
        Node->Params.Transparency = Bucket->HeadTransparency->V;
    }

    Rect2DInstr* Instruction = (Rect2DInstr*) ListPush(MemPool, &Node->Batches, 256);

    Instruction->Dst = Destination;
    Instruction->Src = Source;
    Instruction->Colours[CORNER_00] = Colour;
    Instruction->Colours[CORNER_01] = Colour;
    Instruction->Colours[CORNER_10] = Colour;
    Instruction->Colours[CORNER_11] = Colour;
    Instruction->CornerRadii[CORNER_00] = CornerRadius;
    Instruction->CornerRadii[CORNER_01] = CornerRadius;
    Instruction->CornerRadii[CORNER_10] = CornerRadius;
    Instruction->CornerRadii[CORNER_11] = CornerRadius;
    Instruction->BorderThickness = BorderThickness;
    Instruction->EdgeSoftness = EdgeSoftness;
    Instruction->WhiteTextureOverride = 0.0f;
    Instruction->Sheer = 0.0f;
    Bucket->LastCommandStackgeneration = Bucket->StackGeneration;

    return Instruction;
}

RendPassBlurParams*
DrawBlur(r2f32 Rect, f32 BlurSize, f32 CornerRadius)
{
    Arena* MemPool = GetTLS()->D_THREAD_CTX->MemPool;
    DBucket* Bucket = DTopBucket();
    RendPass* Pass = RendPassFromKind(MemPool, &Bucket->Passes, PASS_KIND_BLUR);
    RendPassBlurParams* Params = Pass->BlurParams;

    Params->Rect = Rect;
    Params->Clip = DTopClip();
    Params->BlurSize = BlurSize;
    Params->CornerRadii[CORNER_00] = CornerRadius;
    Params->CornerRadii[CORNER_01] = CornerRadius;
    Params->CornerRadii[CORNER_10] = CornerRadius;
    Params->CornerRadii[CORNER_11] = CornerRadius;

    return Params;
}

RendPassGeo3DParams*
DrawGeo3DBegin(r2f32 ViewPort, m4f32 View, m4f32 Projection)
{
    Arena* MemPool = GetTLS()->D_THREAD_CTX->MemPool;
    DBucket* Bucket = DTopBucket();
    RendPass* Pass = RendPassFromKind(MemPool, &Bucket->Passes, PASS_KIND_GEO3D);
    RendPassGeo3DParams* Params = Pass->Geo3DParams;

    Params->ViewPort = ViewPort;
    Params->View = View;
    Params->Projection = Projection;

    return Params;
}

Mesh3DInstr*
DrawMesh(
    RendHandle Vertices, 
    RendHandle Indices, 
    RendGeoTopKind Topology, 
    GeoVertFlag VertexFlags, 
    RendHandle AlbedoTexture, 
    m4f32 XForm
) {
    Arena* MemPool = GetTLS()->D_THREAD_CTX->MemPool;
    DBucket* Bucket = DTopBucket();
    RendPass* Pass = RendPassFromKind(MemPool, &Bucket->Passes, PASS_KIND_GEO3D);
    RendPassGeo3DParams* Params = Pass->Geo3DParams;

    if (!Params->Batches.SlotsCount) {
        Params->Batches.SlotsCount = 64;
        Params->Batches.Slots = ArenaPushArrayZero(
            MemPool,
            RendBatch3DMapNode*,
            Params->Batches.SlotsCount
        );
    }

    u64 MeshHash = 0;
    u64 SlotIndex = 0;

    {
        u64 Buffer[] = {
            Vertices.U64[0],
            Vertices.U64[1],
            Indices.U64[0],
            Indices.U64[1],
            (u64) Topology,
            (u64) VertexFlags,
            AlbedoTexture.U64[0],
            AlbedoTexture.U64[1],
            (u64) DTopTex2DSampleKind()
        };

        MeshHash = Hash(Str((u8*) Buffer, sizeof(Buffer)));
        SlotIndex = MeshHash % Params->Batches.SlotsCount;
    }

    RendBatch3DMapNode* Node = NULL;

    {
        for (
            RendBatch3DMapNode* N = Params->Batches.Slots[SlotIndex];
            N;
            N = N->Next
        ) {
            if (N->Hash == MeshHash) {
                Node = N;
                break;
            }
        }
    }

    if (!Node) {
        Node = ArenaPushArrayZero(MemPool, RendBatch3DMapNode, 1);
        SLL_STACK_PUSH(Params->Batches.Slots[SlotIndex], Node);
        Node->Hash = MeshHash;
        Node->Batches = RendBatchListAlloc(sizeof(Mesh3DInstr));
        Node->Params.Vertices = Vertices;
        Node->Params.Indices = Indices;
        Node->Params.Topology = Topology;
        Node->Params.VertexFlags = VertexFlags;
        Node->Params.AlbedoTexture = AlbedoTexture;
        Node->Params.AlbedoTextureSampleKind = DTopTex2DSampleKind();
        Node->Params.XForm = Mat4(1.0f);
    }

    Mesh3DInstr* Instruction = (Mesh3DInstr*) ListPush(
        MemPool, 
        &Node->Batches, 
        256
    );

    Instruction->XForm = XForm;

    return Instruction;
}

void 
DrawSubBucket(DBucket* Bucket)
{
    ThreadLocalStorage* TLS = GetTLS();
    Arena* MemPool = TLS->D_THREAD_CTX->MemPool;
    DBucket* Source = Bucket;
    DBucket* Destination = DTopBucket();
    r2f32 DestinationClip = DTopClip();
    b32 DestinationClipIsSet = !(
        DestinationClip.X0 == 0.0f &&
        DestinationClip.X1 == 0.0f &&
        DestinationClip.Y0 == 0.0f &&
        DestinationClip.Y1 == 0.0f
    );

    for (RendPassNode* Node = Source->Passes.Head; Node; Node = Node->Next) {
        RendPass* SourcePass = &Node->V;
        RendPass* DestinationPass = RendPassFromKind(
            MemPool, 
            &Destination->Passes, 
            SourcePass->Kind
        );

        switch (DestinationPass->Kind) {
            default: {
                DestinationPass->Params = SourcePass->Params;
            } break;

            case PASS_KIND_UI: {
                RendPassUIParams* SourceUIParams = SourcePass->UIParams;
                RendPassUIParams* DestinationUIParams = DestinationPass->UIParams;

                for (
                    RendBatch2DNode* SourceGroupNode = SourceUIParams->Rects.Head; 
                    SourceGroupNode; 
                    SourceGroupNode = SourceGroupNode->Next
                ) {
                    RendBatch2DNode* DestinationGroupNode = ArenaPushArrayZero(
                        MemPool, 
                        RendBatch2DNode, 
                        1
                    );

                    SLL_QUEUE_PUSH(
                        DestinationUIParams->Rects.Head, 
                        DestinationUIParams->Rects.Tail, 
                        DestinationGroupNode
                    );
                    ++DestinationUIParams->Rects.Count;
                    MemCpy(
                        &DestinationGroupNode->Params, 
                        &SourceGroupNode->Params, 
                        sizeof(DestinationGroupNode->Params)
                    );
                    DestinationGroupNode->Batches = SourceGroupNode->Batches;
                    DestinationGroupNode->Params.XForm = DTopXForm2D();

                    if (DestinationClipIsSet) {
                        b32 ClipIsSet = !(
                            DestinationGroupNode->Params.Clip.X0 == 0.0f &&
                            DestinationGroupNode->Params.Clip.X1 == 0.0f &&
                            DestinationGroupNode->Params.Clip.Y0 == 0.0f &&
                            DestinationGroupNode->Params.Clip.Y1 == 0.0f
                        );

                        if (ClipIsSet) {
                            DestinationGroupNode->Params.Clip = Intersect(
                                DestinationClip, 
                                DestinationGroupNode->Params.Clip
                            );
                        } else {
                            DestinationGroupNode->Params.Clip = DestinationClip;
                        }
                    }
                }
            } break;
        }
    }
}

void 
DrawTruncatedFancyRunList(
    v2f32 P, 
    FancyRunList* List, 
    f32 MaxX, 
    FontRun TrailerRun
) {
    b32 TrailerEnabled = (
        List->Dimensions.X > MaxX && 
        TrailerRun.Dimensions.X < MaxX
    );
    f32 Advance = 0.0f;
    b32 TrailerFound = FALSE;
    v4f32 LastColour = {};
    u64 ByteOffset = 0;

    for (FancyRunNode* Node = List->Head; Node; Node = Node->Next) {
        FancyRun* FRun = &Node->V;
        r1f32 PixelRange = {};

        PixelRange.Min = 100000.0f;
        PixelRange.Max = 0.0f;

        FontPiece* StartPiece = FRun->Run.Pieces.V;
        FontPiece* EndPiece = StartPiece + FRun->Run.Pieces.Count;
        f32 PreAdvance = Advance;

        LastColour = FRun->Colour;

        for (FontPiece* Piece = StartPiece; Piece < EndPiece; ++Piece) {
            if (
                TrailerEnabled && 
                Advance + Piece->Advance > (MaxX - TrailerRun.Dimensions.X)
            ) {
                TrailerFound = TRUE;
                break;
            }

            if (!TrailerEnabled && Advance + Piece->Advance > MaxX)
                goto END_DRAW;

            RendHandle Texture = Piece->Texture;
            r2f32 Source = Rng(
                (f32) Piece->SubRect.X0,
                (f32) Piece->SubRect.Y0,
                (f32) Piece->SubRect.X1,
                (f32) Piece->SubRect.Y1
            );
            v2f32 Size = Length(Source);
            r2f32 Destination = Rng(
                P.X + Piece->Offset.X + Advance, 
                P.Y + Piece->Offset.Y,
                P.X + Piece->Offset.X + Advance + Size.X, 
                P.Y + Piece->Offset.Y + Size.Y
            );

            if (Texture != EMPTY_REND_HANDLE_VALUE) {
                DrawImage(
                    Destination, 
                    Source, 
                    Texture, 
                    FRun->Colour, 
                    0.0f, 
                    0.0f, 
                    0.0f
                );
            }

            Advance += Piece->Advance;
            PixelRange.Min = MIN(PreAdvance, PixelRange.Min);
            PixelRange.Max = MAX(Advance, PixelRange.Max);
        }

        if (FRun->UnderlineThickness > 0.0f) {
            DrawRect(
                Rng(
                    P.X + PixelRange.Min,
                    P.Y + FRun->Run.Descent + FRun->Run.Descent / 8,
                    P.X + PixelRange.Max,
                    P.Y + FRun->Run.Descent + FRun->Run.Descent / 8 + FRun->UnderlineThickness
                ),
                FRun->Colour,
                0.0f,
                0.0f,
                0.8f
            );
        }

        if (FRun->StrikethroughThickness > 0.0f) {
            DrawRect(
                Rng(
                    P.X + PreAdvance, 
                    P.Y + FRun->Run.Descent - FRun->Run.Ascent / 2,
                    P.X + Advance,
                    P.Y + FRun->Run.Descent - FRun->Run.Ascent / 2 + FRun->StrikethroughThickness
                ),
                FRun->Colour,
                0.0f,
                0.0f,
                1.0f
            );
        }

        if (TrailerFound)
            break;
    }

END_DRAW:
    if (TrailerFound) {
        FontPiece* StartPiece = TrailerRun.Pieces.V;
        FontPiece* EndPiece = StartPiece + TrailerRun.Pieces.Count;
        f32 PreAdvance = Advance;
        v4f32 TrailerPieceColour = LastColour;

        for (FontPiece* Piece = StartPiece; Piece < EndPiece; ++Piece) {
            RendHandle Texture = Piece->Texture;
            r2f32 Source = Rng(
                (f32) Piece->SubRect.X0,
                (f32) Piece->SubRect.Y0,
                (f32) Piece->SubRect.X1,
                (f32) Piece->SubRect.Y1
            );
            v2f32 Size = Length(Source);
            r2f32 Destination = Rng(
                P.X + Piece->Offset.X + Advance, 
                P.Y + Piece->Offset.Y,
                P.X + Piece->Offset.X + Advance + Size.X, 
                P.Y + Piece->Offset.Y + Size.Y
            );

            if (Texture != EMPTY_REND_HANDLE_VALUE) {
                DrawImage(
                    Destination, 
                    Source, 
                    Texture, 
                    TrailerPieceColour, 
                    0.0f, 
                    0.0f, 
                    0.0f
                );
                TrailerPieceColour.W *= 0.5f;
            }

            Advance += Piece->Advance;
        }
    }
}

void 
DrawTextRun(v2f32 P, v4f32 Colour, FontRun Run)
{
    f32 Advance = 0.0f;
    FontPiece* StartPiece = Run.Pieces.V;
    FontPiece* EndPiece = StartPiece + Run.Pieces.Count;

    for (FontPiece* Piece = StartPiece; Piece < EndPiece; ++Piece) {
        RendHandle Texture = Piece->Texture;
        r2f32 Source = Rng(
            (f32) Piece->SubRect.X0,
            (f32) Piece->SubRect.Y0,
            (f32) Piece->SubRect.X1,
            (f32) Piece->SubRect.Y1
        );
        v2f32 Size = Length(Source);
        r2f32 Destination = Rng(
            P.X + Piece->Offset.X + Advance, 
            P.Y + Piece->Offset.Y,
            P.X + Piece->Offset.X + Advance + Size.X, 
            P.Y + Piece->Offset.Y + Size.Y
        );

        if (
            Size.X != 0.0f && 
            Size.Y != 0.0f && 
            Texture != EMPTY_REND_HANDLE_VALUE
        ) {
            DrawImage(
                Destination, 
                Source, 
                Texture, 
                Colour, 
                0.0f, 
                0.0f, 
                0.0f
            );
        }

        Advance += Piece->Advance;
    }
}

void 
NC_DrawText(
    FontTag Font, 
    f32 Size, 
    f32 BaseAlignPX, 
    f32 TabAlignPX, 
    FontRasterKind Kind, 
    v2f32 P, 
    v4f32 Colour, 
    Str8 String
) {
    TempArena Scratch = GetScratch(NULL, 0);
    FontRun Run = FontRunFromStr(
        Font, 
        Size, 
        BaseAlignPX, 
        TabAlignPX, 
        Kind, 
        String
    );

    DrawTextRun(P, Colour, Run);
    ReleaseScratch(Scratch);
}

FMRangeList 
FuzzyFindFStrs(Arena* MemPool, FancyStrList* FancyStrings, Str8 Needle)
{
    TempArena Scratch = GetScratch(&MemPool, 1);
    Str8 FancyStringsString = {};

    FancyStringsString.Size = FancyStrings->TotalSize;
    FancyStringsString.Str = ArenaPushArrayZero(MemPool, u8, FancyStringsString.Size);

    u64 Offset = 0;

    for (FancyStrNode* Node = FancyStrings->Head; Node; Node = Node->Next) {
        if (Node->V.Params.Font != GetTLS()->D_THREAD_CTX->IconFont) {
            MemCpy(
                FancyStringsString.Str + Offset, 
                Node->V.String.Str, 
                Node->V.String.Size
            );
            Offset += Node->V.String.Size;
        }
    }

    FMRangeList Result = FuzzyFind(MemPool, Needle, FancyStringsString);

    ReleaseScratch(Scratch);

    return Result;
}

void 
DrawTruncatedFancyRunFuzzyMatches(
    v2f32 Position, 
    FancyRunList* List, 
    f32 MaxX, 
    FMRangeList* Ranges, 
    v4f32 Colour
) {
    for (
        FMRangeNode* MatchNode = Ranges->Head;
        MatchNode;
        MatchNode = MatchNode->Next
    ) {
        r1u64 ByteRange = MatchNode->Range;
        r1f32 PixelRange = {};

        {
            PixelRange.Min = 100000.0f;
            PixelRange.Max = 0.0f;
        }

        f32 PrevPieceEndPadding = 0.0f;
        u64 ByteOffset = 0;
        f32 Advance = 0.0f;
        f32 Ascent = 0.0f;
        f32 Descent = 0.0f;

        for (
            FancyRunNode* FancyNode = List->Head;
            FancyNode;
            FancyNode = FancyNode->Next
        ) {
            FancyRun* FRun = &FancyNode->V;
            FontRun* Run = &FRun->Run;

            Ascent = Run->Ascent;
            Descent = Run->Descent;

            for (
                u64 PieceIndex = 0; 
                PieceIndex < Run->Pieces.Count; 
                ++PieceIndex
            ) {
                FontPiece* Piece = &Run->Pieces.V[PieceIndex];

                if (InRange(ByteRange, ByteOffset)) {
                    f32 PreAdvance = Advance + Piece->Offset.X;
                    f32 PostAdvance = Advance + Piece->Advance;

                    PixelRange.Min = MIN(PreAdvance, PixelRange.Min);
                    PixelRange.Max = MAX(PostAdvance, PixelRange.Max);
                }

                if (!FRun->Icon)
                    ByteOffset += Piece->DecodeSize;

                Advance += Piece->Advance;
            }
        }

        if (PixelRange.Min < PixelRange.Max) {
            r2f32 Rect = Rng(
                Position.X + PixelRange.Min - Ascent / 4.0f,
                Position.Y - Descent - Ascent - Ascent / 8.0f,
                Position.X + PixelRange.Max + Ascent / 4.0f,
                Position.Y - Descent - Ascent + Ascent / 8.0f + List->Dimensions.Y
            );

            Rect.X0 = MIN(Rect.X0, Position.X + MaxX);
            Rect.X1 = MIN(Rect.X1, Position.X + MaxX);
            DrawRect(
                Rect,
                Colour,
                (Descent + Ascent) / 4.0f,
                0.0f,
                1.0f
            );
        }
    }
}
