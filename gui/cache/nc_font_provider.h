#if !defined(__NC_FONT_PROVIDER_H__)
#define __NC_FONT_PROVIDER_H__

#include "../nc_types.h"
#include "../nc_math.h"

struct Str8;

// @defines____________________________________________________________________
typedef u32 FPRasterKind;
enum : u32 {
    FP_RASTER_KIND_SMOOTH = (1 << 0),
    FP_RASTER_KIND_HINTED = (1 << 1)
};

// @types______________________________________________________________________
struct FPHandle {
    u64 V[2];
};

struct FPMetrics {
    f32 DesignUnitsPerEM;
    f32 Ascent;
    f32 Descent;
    f32 LineGap;
    f32 CapitalHeight;
};

struct FPRasterResult {
    v2i16 AtlasDimensions;
    void* Atlas;
    f32 Advance;
};

// @runtime____________________________________________________________________
constexpr FPHandle EMPTY_FPHANDLE_VALUE = {};

#if defined(NC_LANG_CPP)
    INTERNAL b32
    operator==(FPHandle HandleA, FPHandle HandleB)
    {
        return (HandleA.V[0] == HandleB.V[0] && HandleA.V[1] == HandleB.V[1]);
    }

    INTERNAL b32
    operator!=(FPHandle HandleA, FPHandle HandleB)
    {
        return !(HandleA == HandleB);
    }
#endif

// @functions__________________________________________________________________
EXTERN_C_LINK void FontProviderInit(void);
EXTERN_C_LINK FPHandle OpenFont(Str8 Path);
EXTERN_C_LINK FPHandle FontOpen(Str8* Data);
EXTERN_C_LINK void CloseFont(FPHandle FontHandle);
EXTERN_C_LINK FPMetrics MetricsFromFont(FPHandle FontHandle);
EXTERN_C_LINK NO_ASAN FPRasterResult Raster(Arena* MemPool, FPHandle FontHandle, f32 Size, FPRasterKind Kind, Str8 String);

#endif // __NC_FONT_PROVIDER_H__
