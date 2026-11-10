#if !defined(__FREETYPE_FONT_PROVIDER_H__)
#define __FREETYPE_FONT_PROVIDER_H__

#include "../../nc_defines.h"
#include "../../nc_arena.h"
#include "../../cache/nc_font_provider.h"

#undef internal

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#define internal static

// @defines____________________________________________________________________


// @types______________________________________________________________________
struct FPFont {
    FT_Face Face;
};

struct FPState {
    Arena*     MemPool;
    FT_Library Library;
};

// @runtime____________________________________________________________________
global FPState* FP_STATE = NULL;

// @functions__________________________________________________________________
FPFont FPFontFromHandle(FPHandle Handle);
FPHandle FPHandleFromFont(FPFont Font);

#endif // __FREETYPE_FONT_PROVIDER_H__
