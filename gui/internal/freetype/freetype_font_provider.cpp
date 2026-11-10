#include "../../nc_string.h"
#include "../../nc_arena.h"
#include "../../nc_profile.h"

#include "freetype_font_provider.h"

FPFont 
FPFontFromHandle(FPHandle Handle)
{
    FPFont Result = {
        (FT_Face) PTR_FROM_INT(Handle.V[0])
    };

    return Result;
}

FPHandle 
FPHandleFromFont(FPFont Font)
{
    FPHandle Result = {
        INT_FROM_PTR(Font.Face)
    };

    return Result;
}

EXTERN_C_LINK_BEGIN
void 
FontProviderInit(void)
{
    Arena* MemPool = ArenaAlloc();

    FP_STATE = ArenaPushArrayZero(
        MemPool,
        FPState,
        1
    );
    FP_STATE->MemPool = MemPool;
    FT_Init_FreeType(&FP_STATE->Library);
}

FPHandle 
OpenFont(Str8 Path)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str8 PathCopy = ArenaPushStrCpy(Scratch.MemPool, Path);
    FPFont Font = {};

    FT_New_Face(
        FP_STATE->Library,
        (char*) PathCopy.Str,
        0,
        &Font.Face
    );

    FPHandle Result = FPHandleFromFont(Font);

    ReleaseScratch(Scratch);

    return Result;
}

FPHandle 
FontOpen(Str8* Data)
{
    FPFont Font = {};

    FT_New_Memory_Face(
        FP_STATE->Library,
        Data->Str,
        (FT_Long) Data->Size,
        0,
        &Font.Face
    );

    FPHandle Result = FPHandleFromFont(Font);

    return Result;
}

void 
CloseFont(FPHandle FontHandle)
{
    FPFont Font = FPFontFromHandle(FontHandle);

    if (Font.Face)
        FT_Done_Face(Font.Face);
}

FPMetrics 
MetricsFromFont(FPHandle FontHandle)
{
    FPFont Font = FPFontFromHandle(FontHandle);
    FPMetrics Result = {};

    if (Font.Face) {
        Result.DesignUnitsPerEM = (f32) (Font.Face->units_per_EM);
        Result.Ascent = (f32) Font.Face->ascender;
        Result.Descent = -(f32) Font.Face->descender;
        Result.LineGap = (f32) (
            Font.Face->height - 
            Font.Face->ascender + 
            Font.Face->descender
        );
        Result.CapitalHeight = (f32) (Font.Face->ascender);
    }

    return Result;
}

NO_ASAN FPRasterResult 
Raster(
    Arena* MemPool, 
    FPHandle FontHandle, 
    f32 Size, 
    FPRasterKind Kind, 
    Str8 String
) {
    FPRasterResult Result = {};

    PROFILE(RasterTimer) {
        FPFont Font = FPFontFromHandle(FontHandle);

        if (Font.Face) {
            TempArena Scratch = GetScratch(&MemPool, 1);
            FT_Face Face = Font.Face;

            FT_Set_Pixel_Sizes(Face, 0, (FT_UInt) ((96.0f / 72.0f) * Size));

            i64 Ascent = Face->size->metrics.ascender >> 6;
            i64 Descent = Abs64(Face->size->metrics.descender >> 6);
            i64 Height = Face->size->metrics.height >> 6;
            Str32 StringWide = Str32FromStr8(Scratch.MemPool, String);
            i32 TotalWidth = 0;

            for (u64 Index = 0; Index < StringWide.Size; ++Index) {
                FT_Load_Char(Face, StringWide.Str[Index], FT_LOAD_RENDER);
                TotalWidth += (Face->glyph->advance.x >> 6);
            }

            v2i16 Len = Vec(
                (i16) (TotalWidth + 1),
                (i16) (Height + 1)
            );
            u64 AtlasLen = Len.X * Len.Y * 4;
            u8* Atlas = ArenaPushArrayZero(
                MemPool,
                u8,
                AtlasLen
            );
            i32 Baseline = Ascent;
            i32 AtlasWriteX = 0;

            for (u64 Index = 0; Index < StringWide.Size; ++Index) {
                FT_Load_Char(Face, StringWide.Str[Index], FT_LOAD_RENDER);

                FT_Bitmap* BMP = &Face->glyph->bitmap;
                i32 Top = Face->glyph->bitmap_top;
                i32 Left = Face->glyph->bitmap_left;

                for (i32 Row = 0; Row < (i32) BMP->rows; ++Row) {
                    i32 Y = Baseline - Top + Row;

                    for (i32 Col = 0; Col < (i32) BMP->width; ++Col) {
                        i32 X = AtlasWriteX + Left + Col;
                        u64 Offset = (Y * Len.X + X) * 4;

                        if (Offset + 4 <= AtlasLen) {
                            Atlas[Offset + 0] = U8_MAX;
                            Atlas[Offset + 1] = U8_MAX;
                            Atlas[Offset + 2] = U8_MAX;
                            Atlas[Offset + 3] = BMP->buffer[
                                Row * BMP->pitch + Col
                            ];
                        }
                    }
                }

                AtlasWriteX += (Face->glyph->advance.x >> 6);
            }

            Result.AtlasDimensions = Len;
            Result.Advance = (f32) TotalWidth;
            Result.Atlas = Atlas;
            ReleaseScratch(Scratch);
        }
    }

    return Result;
}
EXTERN_C_LINK_END
