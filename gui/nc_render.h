#if !defined(__NC_RENDER_H__)
#define __NC_RENDER_H__

#include "nc_types.h"
#include "nc_math.h"
#include "nc_memory.h"

struct Arena;
struct CommandLine;
struct Handle;
struct Str8;
struct Scene3D;
struct FrameDrawData;

// @defines____________________________________________________________________
enum RendTex2DFmt {
    TEX2D_FMT_R8,
    TEX2D_FMT_RG8,
    TEX2D_FMT_RGBA8,
    TEX2D_FMT_RGBA8_SRGB,
    TEX2D_FMT_BGRA8,
    TEX2D_FMT_BGRA8_SRGB,
    TEX2D_FMT_R16,
    TEX2D_FMT_RGBA16,
    TEX2D_FMT_R32,
    TEX2D_FMT_RG32,
    TEX2D_FMT_RGBA32,
    TEX2D_FMT_COUNT
};

enum RendResourceKind {
    RESOURCE_KIND_STATIC,
    RESOURCE_KIND_DYNAMIC,
    RESOURCE_KIND_STREAM,
    RESOURCE_KIND_COUNT
};

enum RendTex2DSampleKind {
    TEX2D_SAMPLE_NEAREST,
    TEX2D_SAMPLE_LINEAR,
    TEX2D_SAMPLE_COUNT
};

enum RendGeoTopKind {
    GEO_TOP_LINES,
    GEO_TOP_LINE_STRIP,
    GEO_TOP_TRIANGLES,
    GEO_TOP_TRIANGLE_STRIP,
    GEO_TOP_COUNT
};

enum RendPassKind {
    PASS_KIND_UI,
    PASS_KIND_BLUR,
    PASS_KIND_GEO3D,
    PASS_KIND_COUNT
};

typedef u8 ChannelCode;
enum : u8 {
    CHANNEL_CODE_NULL,
    CHANNEL_CODE_RED,
    CHANNEL_CODE_GREEN,
    CHANNEL_CODE_BLUE,
    CHANNEL_CODE_ALPHA
};

typedef u8 ChannelSize;
enum : u8 {
    CHANNEL_SIZE_NULL,
    CHANNEL_SIZE_2,
    CHANNEL_SIZE_8,
    CHANNEL_SIZE_10,
    CHANNEL_SIZE_11,
    CHANNEL_SIZE_16,
    CHANNEL_SIZE_24,
    CHANNEL_SIZE_32
};

typedef u8 ChannelType;
enum : u8 {
    CHANNEL_TYPE_NULL,
    CHANNEL_TYPE_UINT,
    CHANNEL_TYPE_INT,
    CHANNEL_TYPE_UNORM,
    CHANNEL_TYPE_NORM,
    CHANNEL_TYPE_FLOAT
};

typedef u32 GeoVertFlag;
enum : u32 {
    GEO_VERT_FLAG_TEX_COORD = (1 << 0),
    GEO_VERT_FLAG_NORMALS   = (1 << 1),
    GEO_VERT_FLAG_RGB       = (1 << 2),
    GEO_VERT_FLAG_RGBA      = (1 << 3),
};

#define RendChannel(Index, CodeName, SizeName, TypeName) ((((u64) (CHANNEL_CODE_##CodeName & 0x7)) | ((u64) (CHANNEL_SIZE_##SizeName & 0x7) << 3) | ((u64) (CHANNEL_TYPE_##TypeName & 0x7) << 6)) << ((Index) * 9))
#define ChannelCodeFromTexture2DFormat(Fmt, Index)       ((ChannelCode) (((Fmt) & (0x7 << ((Index) * 9))) >> ((Index) * 9)))
#define ChannelSizeFromTexture2DFormat(Fmt, Index)       ((ChannelSize) (((Fmt) & (0x38 << ((Index) * 9))) >> ((Index) * 9 + 3)))
#define ChannelTypeFromTexture2DFormat(Fmt, Index)       ((ChannelType) (((Fmt) & (0x1C0 << ((Index) * 9))) >> ((Index) * 9 + 6)))

#define TEXTURE_2D_FORMAT_R8    (RendChannel(0, RED, 8, UINT))
#define TEXTURE_2D_FORMAT_RG8   (RendChannel(0, RED, 8, UINT) | RendChannel(1, GREEN, 8, UINT))
#define TEXTURE_2D_FORMAT_RGB8  (RendChannel(0, RED, 8, UINT) | RendChannel(1, GREEN, 8, UINT) | RendChannel(2, BLUE, 8, UINT))
#define TEXTURE_2D_FORMAT_RGBA8 (RendChannel(0, RED, 8, UINT) | RendChannel(1, GREEN, 8, UINT) | RendChannel(2, BLUE, 8, UINT) | RendChannel(3, ALPHA, 8, UINT))

// @types______________________________________________________________________
typedef u64 Texture2DFormat;

union RendHandle {
    u64 U64[1];
    u32 U32[2];
    u16 U16[4];
};

struct Rect2DInstr {
    r2f32 Dst;
    r2f32 Src;
    v4f32 Colours[CORNER_COUNT];
    f32   CornerRadii[CORNER_COUNT];
    f32   BorderThickness;
    f32   EdgeSoftness;
    f32   WhiteTextureOverride;
    f32   Sheer;
};

struct Mesh3DInstr {
    m4f32 XForm;
};

struct RendBatch {
    u8* V;
    u64 ByteCount;
    u64 ByteCapacity;
};

struct RendBatchNode {
    RendBatchNode* Next;
    RendBatch      V;
};

struct RendBatchList {
    RendBatchNode* Head;
    RendBatchNode* Tail;
    u64            BatchCount;
    u64            ByteCount;
    u64            BytesPerInstruction;
};

struct RendBatch2DParams {
    RendHandle          Texture;
    RendTex2DSampleKind SampleKind;
    m3f32               XForm;
    r2f32               Clip;
    f32                 Transparency;
};

struct RendBatch2DNode {
    RendBatch2DNode*  Next;
    RendBatchList     Batches;
    RendBatch2DParams Params;
};

struct RendBatch2DList {
    RendBatch2DNode* Head;
    RendBatch2DNode* Tail;
    u64              Count;
};

struct RendBatch3DParams {
    RendHandle          Vertices;
    RendHandle          Indices;
    RendGeoTopKind      Topology;
    GeoVertFlag         VertexFlags;
    RendHandle          AlbedoTexture;
    RendTex2DSampleKind AlbedoTextureSampleKind;
    m4f32               XForm;
};

struct RendBatch3DMapNode {
    RendBatch3DMapNode* Next;
    u64                 Hash;
    RendBatchList       Batches;
    RendBatch3DParams   Params;
};

struct RendBatch3DMap {
    RendBatch3DMapNode** Slots;
    u64                  SlotsCount;
};

struct RendPassUIParams {
    RendBatch2DList Rects;
};

struct RendPassBlurParams {
    r2f32 Rect;
    r2f32 Clip;
    f32   BlurSize;
    f32   CornerRadii[CORNER_COUNT];
};

struct RendPassGeo3DParams {
    r2f32          ViewPort;
    r2f32          Clip;
    m4f32          View;
    m4f32          Projection;
    RendBatch3DMap Batches;
};

struct RendPass {
    RendPassKind Kind;

    union {
        void*                Params;
        RendPassUIParams*    UIParams;
        RendPassBlurParams*  BlurParams;
        RendPassGeo3DParams* Geo3DParams;
    };
};

struct RendPassNode {
    RendPassNode* Next;
    RendPass      V;
};

struct RendPassList {
    RendPassNode* Head;
    RendPassNode* Tail;
    u64           Count;
};

// @runtime____________________________________________________________________
EXTERN_C_LINK_BEGIN
extern Str8 TEX2D_FMT_DISP_STRINGS[TEX2D_FMT_COUNT];
extern u8   TEX2D_FMT_BYTES_PER_PIXEL[TEX2D_FMT_COUNT];
extern Str8 RESOURCE_KIND_DISP_STRINGS[RESOURCE_KIND_COUNT];
extern Str8 TEX2D_SAMPLE_KIND_DISP_STRINGS[TEX2D_SAMPLE_COUNT];
extern Str8 REND_PASS_KIND_DISP_STRINGS[PASS_KIND_COUNT];
extern u8   REND_PASS_KIND_BATCHES[PASS_KIND_COUNT];
extern u64  REND_PASS_PARAMS_SIZES[PASS_KIND_COUNT];
EXTERN_C_LINK_END

constexpr RendHandle EMPTY_REND_HANDLE_VALUE = {};

#if defined(NC_LANG_CPP)
    INTERNAL b32
    operator==(RendHandle HndlA, RendHandle HndlB)
    {
        return MemCmpStruct(&HndlA, &HndlB);
    }

    INTERNAL b32
    operator!=(RendHandle HndlA, RendHandle HndlB)
    {
        return !(HndlA == HndlB);
    }
#else
    #error nc_render requires C++ for operator overloading
#endif

// @functions__________________________________________________________________
u64 BytesPerPixelFromTexture2DFormat(Texture2DFormat Format);
m4f32 SampleChannelMapFromTexture2DFormat(Texture2DFormat Format);
m4f32 SampleChannelMapFromTex2DFmt(RendTex2DFmt Format);
RendBatchList RendBatchListAlloc(u64 Size);
void* ListPush(Arena* MemPool, RendBatchList* List, u64 BatchCapacity);
RendPass* RendPassFromKind(Arena* MemPool, RendPassList* List, RendPassKind Kind);

EXTERN_C_LINK_BEGIN
void RendInit(CommandLine* CLI);
RendHandle RendEquipWindow(Handle WindowHandle);
void RendUnequipWindow(Handle WindowHandle, RendHandle Renderer);
RendHandle Tex2DAlloc(RendResourceKind Kind, v2i32 Size, RendTex2DFmt Format, void* Data);
void Tex2DRelease(RendHandle Texture);
RendResourceKind RendResourceKindFromTex2D(RendHandle Texture);
v2i32 SizeFromTex2D(RendHandle Texture);
RendTex2DFmt Tex2DFmtFromTex2D(RendHandle Texture);
void FillTex2DRgn(RendHandle Texture, r2i32 SubRect, void* Data);
RendHandle RendBufferAlloc(RendResourceKind Kind, u64 Size, void* Data);
void RendBufferRelease(RendHandle Buffer);
void RendBeginFrame(void);
void RendEndFrame(void);
void RendBeginWindowFrame(Handle WindowHandle, RendHandle Renderer);
void RendEndWindowFrame(Handle WindowHandle, RendHandle Renderer);
void RendSubmitWindow(Handle WindowHandle, RendHandle Renderer, RendPassList* Passes);
EXTERN_C_LINK_END

#endif // __NC_RENDER_H__
