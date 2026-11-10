#include "nc_render.h"
#include "nc_string.h"
#include "nc_arena.h"

EXTERN_C_LINK_BEGIN
Str8 TEX2D_FMT_DISP_STRINGS[TEX2D_FMT_COUNT] = {
    Str8Lit("R8"),
    Str8Lit("RG8"),
    Str8Lit("RGBA8"),
    Str8Lit("RGBA8_SRGB"),
    Str8Lit("BGRA8"),
    Str8Lit("BGRA8_SRGB"),
    Str8Lit("R16"),
    Str8Lit("RGBA16"),
    Str8Lit("R32"),
    Str8Lit("RG32"),
    Str8Lit("RGBA32")
};

u8 TEX2D_FMT_BYTES_PER_PIXEL[TEX2D_FMT_COUNT] = {
     1,
     2,
     4,
     4,
     4,
     4,
     2,
     8,
     4,
     8,
    16,
};

Str8 RESOURCE_KIND_DISP_STRINGS[RESOURCE_KIND_COUNT] = {
    Str8Lit("Static"),
    Str8Lit("Dynamic"),
    Str8Lit("Stream ")
};

Str8 TEX2D_SAMPLE_KIND_DISP_STRINGS[TEX2D_SAMPLE_COUNT] = {
    Str8Lit("Nearest"),
    Str8Lit("Linear")
};

Str8 REND_PASS_KIND_DISP_STRINGS[PASS_KIND_COUNT] = {
    Str8Lit("UI"),
    Str8Lit("Blur"),
    Str8Lit("Geo3D")
};

u8 REND_PASS_KIND_BATCHES[PASS_KIND_COUNT] = {
    1,
    0,
    1
};

u64 REND_PASS_PARAMS_SIZES[PASS_KIND_COUNT] = {
    sizeof(RendPassUIParams),
    sizeof(RendPassBlurParams),
    sizeof(RendPassGeo3DParams)
};
EXTERN_C_LINK_END

u64 
BytesPerPixelFromTexture2DFormat(Texture2DFormat Format)
{
    u64 ResultBits = 0;

    for (u64 ChannelIndex = 0; ChannelIndex < 4; ++ChannelIndex) {
        ChannelSize Size = ChannelSizeFromTexture2DFormat(Format, ChannelIndex);

        switch (Size) {
            default: {} break;

            case CHANNEL_SIZE_2: {
                ResultBits += 2;
            } break;

            case CHANNEL_SIZE_8: {
                ResultBits += 8;
            } break;

            case CHANNEL_SIZE_10: {
                ResultBits += 10;
            } break;

            case CHANNEL_SIZE_11: {
                ResultBits += 11;
            } break;

            case CHANNEL_SIZE_16: {
                ResultBits += 16;
            } break;

            case CHANNEL_SIZE_24: {
                ResultBits += 24;
            } break;

            case CHANNEL_SIZE_32: {
                ResultBits += 32;
            } break;
        }
    }

    u64 ResultRounded = ResultBits + 7;

    ResultRounded -= ResultRounded % 8;

    u64 Result = ResultRounded / 8;

    return Result;
}

API_INTERNAL m4f32 
SampleChannelMapFromTexture2DFormat(Texture2DFormat Format)
{
    UNUSED(Format);

    return m4f32{
        {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };
}

m4f32 
SampleChannelMapFromTex2DFmt(RendTex2DFmt Format)
{
    m4f32 Result = {
        {
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f}
        }
    };

    switch (Format) {
        default: {} break;

        case TEX2D_FMT_R8: {
            MemSet(Result.V[0], 0, ARRAY_COUNT(Result.V[0]));
            Result.V[0][0] = 1.0f;
            Result.V[0][1] = 1.0f;
            Result.V[0][2] = 1.0f;
            Result.V[0][3] = 1.0f;
        } break;
    }

    return Result;
}

RendBatchList 
RendBatchListAlloc(u64 Size)
{
    RendBatchList Result = {};

    Result.BytesPerInstruction = Size;

    return Result;
}

void* 
ListPush(Arena* MemPool, RendBatchList* List, u64 BatchCapacity)
{
    void* Result = NULL;
    RendBatchNode* Node = List->Tail;

    if (!Node || Node->V.ByteCount + List->BytesPerInstruction > Node->V.ByteCapacity) {
        Node = ArenaPushArrayZero(MemPool, RendBatchNode, 1);
        Node->V.ByteCapacity = BatchCapacity * List->BytesPerInstruction;
        Node->V.V = ArenaPushArray(MemPool, u8, Node->V.ByteCapacity);
        SLL_QUEUE_PUSH(List->Head, List->Tail, Node);
        ++List->BatchCount;
    }

    Result = Node->V.V + Node->V.ByteCount;
    Node->V.ByteCount += List->BytesPerInstruction;
    List->ByteCount += List->BytesPerInstruction;

    return Result;
}

RendPass* 
RendPassFromKind(Arena* MemPool, RendPassList* List, RendPassKind Kind)
{
    RendPassNode* Node = List->Tail;

    if (!REND_PASS_KIND_BATCHES[Kind]) {
        Node = NULL;
    }

    if (!Node || Node->V.Kind != Kind) {
        Node = ArenaPushArrayZero(MemPool, RendPassNode, 1);
        SLL_QUEUE_PUSH(List->Head, List->Tail, Node);
        ++List->Count;
        Node->V.Kind = Kind;
        Node->V.Params = ArenaPushArrayZero(MemPool, u8, REND_PASS_PARAMS_SIZES[Kind]);
    }

    return &Node->V;
}
