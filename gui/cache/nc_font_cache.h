#if !defined(__NC_FONT_CACHE_H__)
#define __NC_FONT_CACHE_H__

#include "../nc_types.h"
#include "../nc_math.h"
#include "../nc_render.h"
#include "../nc_string.h"
#include "nc_font_provider.h"

// @defines____________________________________________________________________
typedef u32 FontRasterKind;
enum : u32 {
    FONT_RASTER_KIND_SMOOTH = (1 << 0),
    FONT_RASTER_KIND_HINTED = (1 << 1)
};

typedef u32 FontAtlasRegionNodeKind;
enum : u32 {
    FONT_ATLAS_REGION_NODE_KIND_TAKEN = (1 << 0)
};

// @types______________________________________________________________________
struct FontTag {
    u64 V[2];
};

struct FontPiece {
    RendHandle Texture;
    r2i16      SubRect;
    v2i16      Offset;
    f32        Advance;
    u16        DecodeSize;
};

struct FontPieceChunkNode {
    FontPieceChunkNode* Next;
    FontPiece*          V;
    u64                 Count;
    u64                 Capacity;
};

struct FontPieceChunkList {
    FontPieceChunkNode* Head;
    FontPieceChunkNode* Tail;
    u64                 NodeCount;
    u64                 TotalPieceCount;
};

struct FontPieceArray {
    FontPiece* V;
    u64        Count;
};

struct FontRun {
    FontPieceArray Pieces;
    v2f32          Dimensions;
    f32            Ascent;
    f32            Descent;
};

struct FontHashNode {
    FontHashNode* Next;
    FontTag       Tag;
    FPHandle      Handle;
    FPMetrics     Metrics;
    Str8          Path;
};

struct FontHashSlot {
    FontHashNode* Head;
    FontHashNode* Tail;
};

struct FontRasterCacheInfo {
    r2i16 SubRect;
    v2i16 RasterDimensions;
    i16   AtlasNumber;
    f32   Advance;
};

struct FontHash2InfoRasterCacheNode {
    FontHash2InfoRasterCacheNode* Next;
    FontHash2InfoRasterCacheNode* Prev;
    u64                           Hash;
    FontRasterCacheInfo           Info;
};

struct FontHash2InfoRasterCacheSlot {
    FontHash2InfoRasterCacheNode* Head;
    FontHash2InfoRasterCacheNode* Tail;
};

struct FontRunCacheNode {
    FontRunCacheNode* Next;
    Str8              String;
    FontRun           Run;
};

struct FontRunCacheSlot {
    FontRunCacheNode* Head;
    FontRunCacheNode* Tail;
};

struct FontHash2StyleRasterCacheNode {
    FontHash2StyleRasterCacheNode* Next;
    FontHash2StyleRasterCacheNode* Prev;
    u64                            StyleHash;
    f32                            Ascent;
    f32                            Descent;
    f32                            ColumnWidth;
    FontRasterCacheInfo*           UTF8Class1DirectMap;
    u64                            UTF8Class1DirectMapMask[4];
    u64                            Hash2InfoSlotsCount;
    FontHash2InfoRasterCacheSlot*  Hash2InfoSlots;
    u64                            RunSlotsCount;
    FontRunCacheSlot*              RunSlots;
    u64                            RunSlotsFrameIndex;
};

struct FontHash2StyleRasterCacheSlot {
    FontHash2StyleRasterCacheNode* Head;
    FontHash2StyleRasterCacheNode* Tail;
};

struct FontAtlasRegionNode {
    FontAtlasRegionNode*    Parent;
    FontAtlasRegionNode*    Children[CORNER_COUNT];
    v2i16                   MaxFreeSize[CORNER_COUNT];
    FontAtlasRegionNodeKind Kind;
    u64                     NumAllocatedDescendants;
};

struct FontAtlas {
    FontAtlas*           Next;
    FontAtlas*           Prev;
    RendHandle           Texture;
    v2i16                RootDimensions;
    FontAtlasRegionNode* Root;
};

struct FontMetrics {
    f32 Ascent;
    f32 Descent;
    f32 LineGap;
    f32 CapitalHeight;
};

struct FontState {
    Arena*                         PermanentMemPool;
    Arena*                         RasterMemPool;
    Arena*                         FrameMemPool;
    u64                            FrameIndex;
    u64                            FontHashTableSize;
    FontHashSlot*                  FontHashTable;
    u64                            Hash2StyleSlotsCount;
    FontHash2StyleRasterCacheSlot* Hash2StyleSlots;
    FontAtlas*                     HeadAtlas;
    FontAtlas*                     TailAtlas;
};

// @runtime____________________________________________________________________
constexpr FontTag EMPTY_FONT_TAG_VALUE = {};

extern FontState* FONT_STATE;

#if defined(NC_LANG_CPP)
    INTERNAL b32
    operator==(FontTag TagA, FontTag TagB)
    {
        return (TagA.V[0] == TagB.V[0] && TagA.V[1] == TagB.V[1]);
    }

    INTERNAL b32
    operator!=(FontTag TagA, FontTag TagB)
    {
        return !(TagA == TagB);
    }
#else
    #error nc_font_cache.h requires C++ for operator overloading
#endif

// @functions__________________________________________________________________
u128 FontHash(Str8 String);
u64 FontHash(u64 Seed, Str8 String);
v2i32 FontVertexFromCorner(Corner Cnr);
FPHandle FPHandleFromTag(FontTag Tag);
FPMetrics FPMetricsFromTag(FontTag Tag);
EXTERN_C_LINK FontTag FontTagFromPath(Str8 Path);
EXTERN_C_LINK FontTag FontTagFromData(Str8* Data);
Str8 PathFromFontTag(FontTag Tag);
r2i16 FontAtlasRegionAlloc(Arena* MemPool, FontAtlas* Atlas, v2i16 SizeRequired);
void FontAtlasRegionRelease(FontAtlas* Atlas, r2i16 Region);
FontPiece* ListPush(Arena* MemPool, FontPieceChunkList* List, u64 Capacity);
void ListPush(Arena* MemPool, FontPieceChunkList* List, u64 Capacity, FontPiece* Piece);
FontPieceArray FontPieceArrayFromChunkList(Arena* MemPool, FontPieceChunkList* List);
FontPieceArray CopyArray(Arena* MemPool, FontPieceArray* Array);
FontHash2StyleRasterCacheNode* FontHash2StyleFromTagSizeKind(FontTag Tag, f32 Size, FontRasterKind Kind);
FontRun FontRunFromStr(FontTag Tag, f32 Size, f32 BaseAlignPX, f32 TabSizePX, FontRasterKind Kind, Str8 String);
Str8List FontWrappedStringLinesFromFontSizeStringMax(Arena* MemPool, FontTag Tag, f32 Size, f32 BaseAlignPX, f32 TabSizePX, Str8 String, f32 Max);
v2f32 FontDimensionsFromTagSizeString(FontTag Tag, f32 Size, f32 BaseAlignPX, f32 TabSizePX, Str8 String);
v2f32 FontDimensionsFromTagSizeStringList(FontTag Tag, f32 Size, f32 BaseAlignPX, f32 TabSizePX, Str8List List);
f32 FontColumnSizeFromTagSize(FontTag Tag, f32 Size);
u64 FontCharPositionFromTagSizeString(FontTag Tag, f32 Size, f32 BaseAlignPX, f32 TabSizePX, Str8 String, f32 P);
FontMetrics FontMetricsFromTagSize(FontTag Tag, f32 Size);
f32 FontLineHeightFromMetrics(FontMetrics* Metrics);
void FontInit(void);
void FontReset(void);
void FontFrame(void);

#endif // __NC_FONT_CACHE_H__
