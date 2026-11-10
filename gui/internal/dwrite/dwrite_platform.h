#if !defined(__DWRITE_PLATFORM_H__)
#define __DWRITE_PLATFORM_H__

#include "../dx11/dx11_platform.h"

struct IDWriteFactory;
struct IDWriteFactory1;
struct IDWriteFactory2;
struct IDWriteRenderingParams;
struct IDWriteRenderingParams1;
struct IDWriteRenderingParams2;
struct IDWriteFontFileLoader;
struct IDWriteFontFileStream;
struct IDWriteFontFile;
struct IDWriteFontFace;
struct IDWriteFontFace1;
struct IDWriteFontFace2;
struct IDWriteGDIInterop;
struct IDWriteBitmapRenderTarget;
struct IDWriteBitmapRenderTarget1;
struct IDWriteFactoryVtbl;
struct IDWriteFactory1Vtbl;
struct IDWriteFactory2Vtbl;
struct IDWriteRenderingParamsVtbl;
struct IDWriteRenderingParams1Vtbl;
struct IDWriteRenderingParams2Vtbl;
struct IDWriteFontFileLoaderVtbl;
struct IDWriteFontFileStreamVtbl;
struct IDWriteFontFileVtbl;
struct IDWriteFontFaceVtbl;
struct IDWriteFontFace1Vtbl;
struct IDWriteFontFace2Vtbl;
struct IDWriteGDIInteropVtbl;
struct IDWriteBitmapRenderTargetVtbl;
struct IDWriteBitmapRenderTarget1Vtbl;

// @defines____________________________________________________________________
typedef enum _DWRITE_FACTORY_TYPE : i32 {
    DWRITE_FACTORY_TYPE_SHARED,
    DWRITE_FACTORY_TYPE_ISOLATED
} DWRITE_FACTORY_TYPE;
STATIC_ASSERT(sizeof(DWRITE_FACTORY_TYPE) == 4, DWriteFactoryTypeSizeCheck);

typedef enum _DWRITE_PIXEL_GEOMETRY : i32 {
    DWRITE_PIXEL_GEOMETRY_FLAT,
    DWRITE_PIXEL_GEOMETRY_RGB,
    DWRITE_PIXEL_GEOMETRY_BGR
} DWRITE_PIXEL_GEOMETRY;
STATIC_ASSERT(sizeof(DWRITE_PIXEL_GEOMETRY) == 4, DWritePixelGeometrySizeCheck);

typedef enum _DWRITE_RENDERING_MODE : i32 {
    DWRITE_RENDERING_MODE_DEFAULT,
    DWRITE_RENDERING_MODE_ALIASED,
    DWRITE_RENDERING_MODE_GDI_CLASSIC,
    DWRITE_RENDERING_MODE_GDI_NATURAL,
    DWRITE_RENDERING_MODE_NATURAL,
    DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
    DWRITE_RENDERING_MODE_OUTLINE,
    DWRITE_RENDERING_MODE_CLEARTYPE_GDI_CLASSIC,
    DWRITE_RENDERING_MODE_CLEARTYPE_GDI_NATURAL,
    DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL,
    DWRITE_RENDERING_MODE_CLEARTYPE_NATURAL_SYMMETRIC
} DWRITE_RENDERING_MODE;
STATIC_ASSERT(sizeof(DWRITE_RENDERING_MODE) == 4, DWriteRenderingModeSizeCheck);

typedef enum _DWRITE_FONT_SIMULATIONS : i32 {
    DWRITE_FONT_SIMULATIONS_NONE    = 0x0000,
    DWRITE_FONT_SIMULATIONS_BOLD    = 0x0001,
    DWRITE_FONT_SIMULATIONS_OBLIQUE = 0x0002
} DWRITE_FONT_SIMULATIONS;
STATIC_ASSERT(sizeof(DWRITE_FONT_SIMULATIONS) == 4, DWriteFontSimulationsSizeCheck);

typedef enum _DWRITE_FONT_FACE_TYPE : i32 {
    DWRITE_FONT_FACE_TYPE_CFF,
    DWRITE_FONT_FACE_TYPE_TRUETYPE,
    DWRITE_FONT_FACE_TYPE_OPENTYPE_COLLECTION,
    DWRITE_FONT_FACE_TYPE_TYPE1,
    DWRITE_FONT_FACE_TYPE_VECTOR,
    DWRITE_FONT_FACE_TYPE_BITMAP,
    DWRITE_FONT_FACE_TYPE_UNKNOWN,
    DWRITE_FONT_FACE_TYPE_RAW_CFF,
    DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION
} DWRITE_FONT_FACE_TYPE;
STATIC_ASSERT(sizeof(DWRITE_FONT_FACE_TYPE) == 4, DWriteFontFaceTypeSizeCheck);

typedef enum _DWRITE_GRID_FIT_MODE : i32 {
    DWRITE_GRID_FIT_MODE_DEFAULT,
    DWRITE_GRID_FIT_MODE_DISABLED,
    DWRITE_GRID_FIT_MODE_ENABLED
} DWRITE_GRID_FIT_MODE;
STATIC_ASSERT(sizeof(DWRITE_GRID_FIT_MODE) == 4, DWriteGridFitModeSizeCheck);

typedef enum _DWRITE_MEASURING_MODE : i32 {
    DWRITE_MEASURING_MODE_NATURAL,
    DWRITE_MEASURING_MODE_GDI_CLASSIC,
    DWRITE_MEASURING_MODE_GDI_NATURAL
} DWRITE_MEASURING_MODE;
STATIC_ASSERT(sizeof(DWRITE_MEASURING_MODE) == 4, DWriteMeasuringModeSizeCheck);

typedef enum _DWRITE_TEXTURE_TYPE : i32 {
    DWRITE_TEXTURE_ALIASED_1x1,
    DWRITE_TEXTURE_CLEARTYPE_3x1
} DWRITE_TEXTURE_TYPE;
STATIC_ASSERT(sizeof(DWRITE_TEXTURE_TYPE) == 4, DWriteTextureTypeSizeCheck);

// @types______________________________________________________________________
struct DWRITE_GLYPH_METRICS {
    INT32  LeftSideBearing;
    UINT32 AdvanceWidth;
    INT32  RightSideBearing;
    INT32  TopSideBearing;
    UINT32 AdvanceHeight;
    INT32  BottomSideBearing;
    INT32  VerticalOriginY;
};

struct DWRITE_GLYPH_OFFSET {
    FLOAT AdvanceOffset;
    FLOAT AscenderOffset;
};

struct DWRITE_GLYPH_RUN {
    IDWriteFontFace*           FontFace;
    FLOAT                      FontEmSize;
    UINT32                     GlyphCount;
    UINT16 const*              GlyphIndices;
    FLOAT const*               GlyphAdvances;
    DWRITE_GLYPH_OFFSET const* GlyphOffsets;
    BOOL                       IsSideways;
    UINT32                     BidiLevel;
};

struct DWRITE_FONT_METRICS {
    UINT16 DesignUnitsPerEm;
    UINT16 Ascent;
    UINT16 Descent;
    INT16  LineGap;
    UINT16 CapHeight;
    UINT16 XHeight;
    INT16  UnderlinePosition;
    UINT16 UnderlineThickness;
    INT16  StrikethroughPosition;
    UINT16 StrikethroughThickness;
};

struct DWRITE_MATRIX {
    FLOAT M11;
    FLOAT M12;
    FLOAT M21;
    FLOAT M22;
    FLOAT Dx;
    FLOAT Dy;
};

struct IDWriteFactory {
    IDWriteFactoryVtbl* Vtbl;
};

struct IDWriteFactory1 {
    IDWriteFactory1Vtbl* Vtbl;
};

struct IDWriteFactory2 {
    IDWriteFactory2Vtbl* Vtbl;
};

struct IDWriteRenderingParams {
    IDWriteRenderingParamsVtbl* Vtbl;
};

struct IDWriteRenderingParams1 {
    IDWriteRenderingParams1Vtbl* Vtbl;
};

struct IDWriteRenderingParams2 {
    IDWriteRenderingParams2Vtbl* Vtbl;
};

struct IDWriteFontFileLoader {
    IDWriteFontFileLoaderVtbl* Vtbl;
};

struct IDWriteFontFileStream {
    IDWriteFontFileStreamVtbl* Vtbl;
};

struct IDWriteFontFile {
    IDWriteFontFileVtbl* Vtbl;
};

struct IDWriteFontFace {
    IDWriteFontFaceVtbl* Vtbl;
};

struct IDWriteFontFace1 {
    IDWriteFontFace1Vtbl* Vtbl;
};

struct IDWriteFontFace2 {
    IDWriteFontFace2Vtbl* Vtbl;
};

struct IDWriteGDIInterop {
    IDWriteGDIInteropVtbl* Vtbl;
};

struct IDWriteBitmapRenderTarget {
    IDWriteBitmapRenderTargetVtbl* Vtbl;
};

struct IDWriteBitmapRenderTarget1 {
    IDWriteBitmapRenderTarget1Vtbl* Vtbl;
};

// @callbacks__________________________________________________________________
typedef HRESULT                 WINAPI DWFactGetSystemFontCollectionFuncType(THIS, void** FontCollection, BOOL CheckForUpdates);
typedef HRESULT                 WINAPI DWFactCreateCustomFontCollectionFuncType(THIS, void* CollectionLoader, void const* CollectionKey, UINT32 CollectionKeySize, void** FontCollection);
typedef HRESULT                 WINAPI DWFactRegisterFontCollectionLoaderFuncType(THIS, void* FontCollectionLoader);
typedef HRESULT                 WINAPI DWFactUnregisterFontCollectionLoaderFuncType(THIS, void* FontCollectionLoader);
typedef HRESULT                 WINAPI DWFactCreateFontFileReferenceFuncType(THIS, WCHAR const* FilePath, FILETIME const* LastWriteTime, IDWriteFontFile** FontFile);
typedef HRESULT                 WINAPI DWFactCreateCustomFontFileReferenceFuncType(THIS, void const* FontFileReferenceKey, UINT32 FontFileReferenceKeySize, IDWriteFontFileLoader* FontFileLoader, IDWriteFontFile** FontFile);
typedef HRESULT                 WINAPI DWFactCreateFontFaceFuncType(THIS, DWRITE_FONT_FACE_TYPE FontFaceType, UINT32 NumberOfFiles, IDWriteFontFile* const* FontFiles, UINT32 FaceIndex, DWRITE_FONT_SIMULATIONS FontFaceSimulationFlags, IDWriteFontFace** FontFace);
typedef HRESULT                 WINAPI DWFactCreateRenderingParamsFuncType(THIS, IDWriteRenderingParams** RenderingParams);
typedef HRESULT                 WINAPI DWFactCreateMonitorRenderingParamsFuncType(THIS, HMONITOR Monitor, IDWriteRenderingParams** RenderingParams);
typedef HRESULT                 WINAPI DWFactCreateCustomRenderingParamsFuncType(THIS, FLOAT Gamma, FLOAT EnhancedContrast, FLOAT ClearTypeLevel, DWRITE_PIXEL_GEOMETRY PixelGeometry, DWRITE_RENDERING_MODE RenderingMode, IDWriteRenderingParams** RenderingParams);
typedef HRESULT                 WINAPI DWFactRegisterFontFileLoaderFuncType(THIS, IDWriteFontFileLoader* FontFileLoader);
typedef HRESULT                 WINAPI DWFactUnregisterFontFileLoaderFuncType(THIS, IDWriteFontFileLoader* FontFileLoader);
typedef HRESULT                 WINAPI DWFactCreateTextFormatFuncType(THIS, WCHAR const* FontFamilyName, void* FontCollection, i32 FontWeight, i32 FontStyle, i32 FontStretch, FLOAT FontSize, WCHAR const* LocaleName, void** TextFormat);
typedef HRESULT                 WINAPI DWFactCreateTypographyFuncType(THIS, void** Typography);
typedef HRESULT                 WINAPI DWFactGetGdiInteropFuncType(THIS, IDWriteGDIInterop** GdiInterop);
typedef HRESULT                 WINAPI DWFactCreateTextLayoutFuncType(THIS, WCHAR const* String, UINT32 StringLength, void* TextFormat, FLOAT MaxWidth, FLOAT MaxHeight, void** TextLayout);
typedef HRESULT                 WINAPI DWFactCreateGdiCompatibleTextLayoutFuncType(THIS, WCHAR const* String, UINT32 StringLength, void* TextFormat, FLOAT LayoutWidth, FLOAT LayoutHeight, FLOAT PixelsPerDip, DWRITE_MATRIX const* Transform, BOOL UseGdiNatural, void** TextLayout);
typedef HRESULT                 WINAPI DWFactCreateEllipsisTrimmingSignFuncType(THIS, void* TextFormat, void** TrimmingSign);
typedef HRESULT                 WINAPI DWFactCreateTextAnalyzerFuncType(THIS, void** TextAnalyzer);
typedef HRESULT                 WINAPI DWFactCreateNumberSubstitutionFuncType(THIS, i32 SubstitutionMethod, WCHAR const* LocaleName, BOOL IgnoreUserOverride, void** NumberSubstitution);
typedef HRESULT                 WINAPI DWFactCreateGlyphRunAnalysisFuncType(THIS, DWRITE_GLYPH_RUN const* GlyphRun, FLOAT PixelsPerDip, DWRITE_MATRIX const* Transform, i32 RenderingMode, DWRITE_MEASURING_MODE MeasuringMode, FLOAT BaselineOriginX, FLOAT BaselineOriginY, void** GlyphRunAnalysis);
typedef HRESULT                 WINAPI DWFact1GetEudcFontCollectionFuncType(THIS, void** FontCollection, BOOL CheckForUpdates);
typedef HRESULT                 WINAPI DWFact1CreateCustomRenderingParamsFuncType(THIS, FLOAT Gamma, FLOAT EnhancedContrast, FLOAT EnhancedContrastGrayscale, FLOAT ClearTypeLevel, DWRITE_PIXEL_GEOMETRY PixelGeometry, DWRITE_RENDERING_MODE RenderingMode, IDWriteRenderingParams1** RenderingParams);
typedef HRESULT                 WINAPI DWFact2GetSystemFontFallbackFuncType(THIS, void** FontFallback);
typedef HRESULT                 WINAPI DWFact2CreateFontFallbackBuilderFuncType(THIS, void** FontFallbackBuilder);
typedef HRESULT                 WINAPI DWFact2TranslateColorGlyphRunFuncType(THIS, FLOAT BaselineOriginX, FLOAT BaselineOriginY, DWRITE_GLYPH_RUN const* GlyphRun, void const* GlyphRunDescription, DWRITE_MEASURING_MODE MeasuringMode, DWRITE_MATRIX const* WorldToDeviceTransform, UINT32 ColorPaletteIndex, void** ColorLayers);
typedef HRESULT                 WINAPI DWFact2CreateCustomRenderingParamsFuncType(THIS, FLOAT Gamma, FLOAT EnhancedContrast, FLOAT GrayscaleEnhancedContrast, FLOAT ClearTypeLevel, DWRITE_PIXEL_GEOMETRY PixelGeometry, DWRITE_RENDERING_MODE RenderingMode, DWRITE_GRID_FIT_MODE GridFitMode, IDWriteRenderingParams2** RenderingParams);
typedef FLOAT                   WINAPI DWRenderParamsGetGammaFuncType(THIS);
typedef FLOAT                   WINAPI DWRenderParamsGetEnhancedContrastFuncType(THIS);
typedef FLOAT                   WINAPI DWRenderParamsGetClearTypeLevelFuncType(THIS);
typedef DWRITE_PIXEL_GEOMETRY   WINAPI DWRenderParamsGetPixelGeometryFuncType(THIS);
typedef DWRITE_RENDERING_MODE   WINAPI DWRenderParamsGetRenderingModeFuncType(THIS);
typedef FLOAT                   WINAPI DWRenderParams1GetGrayscaleEnhancedContrastFuncType(THIS);
typedef DWRITE_GRID_FIT_MODE    WINAPI DWRenderParams2GetGridFitModeFuncType(THIS);
typedef HRESULT                 WINAPI DWFontFileLoaderCreateStreamFromKeyFuncType(THIS, void const* FontFileReferenceKey, UINT32 FontFileReferenceKeySize, IDWriteFontFileStream** FontFileStream);
typedef HRESULT                 WINAPI DWFontFileStreamReadFileFragmentFuncType(THIS, void const** FragmentStart, UINT64 FileOffset, UINT64 FragmentSize, void** FragmentContext);
typedef void                    WINAPI DWFontFileStreamReleaseFileFragmentFuncType(THIS, void* FragmentContext);
typedef HRESULT                 WINAPI DWFontFileStreamGetFileSizeFuncType(THIS, UINT64* FileSize);
typedef HRESULT                 WINAPI DWFontFileStreamGetLastWriteTimeFuncType(THIS, UINT64* LastWriteTime);
typedef HRESULT                 WINAPI DWFontFileGetReferenceKeyFuncType(THIS, void const** FontFileReferenceKey, UINT32* FontFileReferenceKeySize);
typedef HRESULT                 WINAPI DWFontFileGetLoaderFuncType(THIS, IDWriteFontFileLoader** FontFileLoader);
typedef HRESULT                 WINAPI DWFontFileAnalyzeFuncType(THIS, BOOL* IsSupportedFontType, DWRITE_FONT_FACE_TYPE* FontFaceType, UINT32* NumberOfFaces);
typedef DWRITE_FONT_FACE_TYPE   WINAPI DWFontFaceGetTypeFuncType(THIS);
typedef HRESULT                 WINAPI DWFontFaceGetFilesFuncType(THIS, UINT32* NumberOfFiles, IDWriteFontFile** FontFiles);
typedef UINT32                  WINAPI DWFontFaceGetIndexFuncType(THIS);
typedef DWRITE_FONT_SIMULATIONS WINAPI DWFontFaceGetSimulationsFuncType(THIS);
typedef BOOL                    WINAPI DWFontFaceIsSymbolFontFuncType(THIS);
typedef void                    WINAPI DWFontFaceGetMetricsFuncType(THIS, DWRITE_FONT_METRICS* FontFaceMetrics);
typedef UINT16                  WINAPI DWFontFaceGetGlyphCountFuncType(THIS);
typedef HRESULT                 WINAPI DWFontFaceGetDesignGlyphMetricsFuncType(THIS, UINT16 const* GlyphIndices, UINT32 GlyphCount, DWRITE_GLYPH_METRICS* GlyphMetrics, BOOL IsSideways);
typedef HRESULT                 WINAPI DWFontFaceGetGlyphIndicesFuncType(THIS, UINT32 const* CodePoints, UINT32 CodePointCount, UINT16* GlyphIndices);
typedef HRESULT                 WINAPI DWFontFaceTryGetFontTableFuncType(THIS, UINT32 OpenTypeTableTag, void const** TableData, UINT32* TableSize, void** TableContext, BOOL* Exists);
typedef void                    WINAPI DWFontFaceReleaseFontTableFuncType(THIS, void* TableContext);
typedef HRESULT                 WINAPI DWFontFaceGetGlyphRunOutlineFuncType(THIS, FLOAT EmSize, UINT16 const* GlyphIndices, FLOAT const* GlyphAdvances, DWRITE_GLYPH_OFFSET const* GlyphOffsets, UINT32 GlyphCount, BOOL IsSideways, BOOL IsRightToLeft, void* GeometrySink);
typedef HRESULT                 WINAPI DWFontFaceGetRecommendedRenderingModeFuncType(THIS, FLOAT EmSize, FLOAT PixelsPerDip, DWRITE_MEASURING_MODE MeasuringMode, IDWriteRenderingParams* RenderingParams, DWRITE_RENDERING_MODE* RenderingMode);
typedef HRESULT                 WINAPI DWFontFaceGetGdiCompatibleMetricsFuncType(THIS, FLOAT EmSize, FLOAT PixelsPerDip, DWRITE_MATRIX const* Transform, DWRITE_FONT_METRICS* FontFaceMetrics);
typedef HRESULT                 WINAPI DWFontFaceGetGdiCompatibleGlyphMetricsFuncType(THIS, FLOAT EmSize, FLOAT PixelsPerDip, DWRITE_MATRIX const* Transform, BOOL UseGdiNatural, UINT16 const* GlyphIndices, UINT32 GlyphCount, DWRITE_GLYPH_METRICS* GlyphMetrics, BOOL IsSideways);
typedef HRESULT                 WINAPI DWGdiInteropCreateFontFromLOGFONTFuncType(THIS, void const* LogFont, void** Font);
typedef HRESULT                 WINAPI DWGdiInteropConvertFontToLOGFONTFuncType(THIS, void* Font, void* LogFont, BOOL* IsSystemFont);
typedef HRESULT                 WINAPI DWGdiInteropConvertFontFaceToLOGFONTFuncType(THIS, IDWriteFontFace* FontFace, void* LogFont);
typedef HRESULT                 WINAPI DWGdiInteropCreateFontFaceFromHdcFuncType(THIS, HDC Hdc, IDWriteFontFace** FontFace);
typedef HRESULT                 WINAPI DWGdiInteropCreateBitmapRenderTargetFuncType(THIS, HDC Hdc, UINT32 Width, UINT32 Height, IDWriteBitmapRenderTarget** RenderTarget);
typedef HRESULT                 WINAPI DWBitmapRenderTargetDrawGlyphRunFuncType(THIS, FLOAT BaselineOriginX, FLOAT BaselineOriginY, DWRITE_MEASURING_MODE MeasuringMode, DWRITE_GLYPH_RUN const* GlyphRun, IDWriteRenderingParams* RenderingParams, COLORREF TextColor, RECT* BlackBoxRect);
typedef HDC                     WINAPI DWBitmapRenderTargetGetMemoryDCFuncType(THIS);
typedef FLOAT                   WINAPI DWBitmapRenderTargetGetPixelsPerDipFuncType(THIS);
typedef HRESULT                 WINAPI DWBitmapRenderTargetSetPixelsPerDipFuncType(THIS, FLOAT PixelsPerDip);
typedef HRESULT                 WINAPI DWBitmapRenderTargetGetCurrentTransformFuncType(THIS, DWRITE_MATRIX* Transform);
typedef HRESULT                 WINAPI DWBitmapRenderTargetSetCurrentTransformFuncType(THIS, DWRITE_MATRIX const* Transform);
typedef HRESULT                 WINAPI DWBitmapRenderTargetGetSizeFuncType(THIS, SIZE_T* Size);
typedef HRESULT                 WINAPI DWBitmapRenderTargetResizeFuncType(THIS, UINT32 Width, UINT32 Height);
typedef i32                     WINAPI DWBitmapRenderTarget1GetTextAntialiasModeFuncType(THIS);
typedef HRESULT                 WINAPI DWBitmapRenderTarget1SetTextAntialiasModeFuncType(THIS, i32 AntialiasMode);

EXTERN_C_LINK DLL_IMPORT HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE FactoryKind, REFIID iid, void** Factory);

// @vtbls______________________________________________________________________
struct IDWriteFactoryVtbl {
    IUnknownQueryInterfaceFuncType*               QueryInterface;
    IUnknownAddRefFuncType*                       AddRef;
    IUnknownReleaseFuncType*                      Release;
    DWFactGetSystemFontCollectionFuncType*        GetSystemFontCollection;
    DWFactCreateCustomFontCollectionFuncType*     CreateCustomFontCollection;
    DWFactRegisterFontCollectionLoaderFuncType*   RegisterFontCollectionLoader;
    DWFactUnregisterFontCollectionLoaderFuncType* UnregisterFontCollectionLoader;
    DWFactCreateFontFileReferenceFuncType*        CreateFontFileReference;
    DWFactCreateCustomFontFileReferenceFuncType*  CreateCustomFontFileReference;
    DWFactCreateFontFaceFuncType*                 CreateFontFace;
    DWFactCreateRenderingParamsFuncType*          CreateRenderingParams;
    DWFactCreateMonitorRenderingParamsFuncType*   CreateMonitorRenderingParams;
    DWFactCreateCustomRenderingParamsFuncType*    CreateCustomRenderingParams;
    DWFactRegisterFontFileLoaderFuncType*         RegisterFontFileLoader;
    DWFactUnregisterFontFileLoaderFuncType*       UnregisterFontFileLoader;
    DWFactCreateTextFormatFuncType*               CreateTextFormat;
    DWFactCreateTypographyFuncType*               CreateTypography;
    DWFactGetGdiInteropFuncType*                  GetGdiInterop;
    DWFactCreateTextLayoutFuncType*               CreateTextLayout;
    DWFactCreateGdiCompatibleTextLayoutFuncType*  CreateGdiCompatibleTextLayout;
    DWFactCreateEllipsisTrimmingSignFuncType*     CreateEllipsisTrimmingSign;
    DWFactCreateTextAnalyzerFuncType*             CreateTextAnalyzer;
    DWFactCreateNumberSubstitutionFuncType*       CreateNumberSubstitution;
    DWFactCreateGlyphRunAnalysisFuncType*         CreateGlyphRunAnalysis;
};

struct IDWriteFactory1Vtbl {
    IUnknownQueryInterfaceFuncType*               QueryInterface;
    IUnknownAddRefFuncType*                       AddRef;
    IUnknownReleaseFuncType*                      Release;
    DWFactGetSystemFontCollectionFuncType*        GetSystemFontCollection;
    DWFactCreateCustomFontCollectionFuncType*     CreateCustomFontCollection;
    DWFactRegisterFontCollectionLoaderFuncType*   RegisterFontCollectionLoader;
    DWFactUnregisterFontCollectionLoaderFuncType* UnregisterFontCollectionLoader;
    DWFactCreateFontFileReferenceFuncType*        CreateFontFileReference;
    DWFactCreateCustomFontFileReferenceFuncType*  CreateCustomFontFileReference;
    DWFactCreateFontFaceFuncType*                 CreateFontFace;
    DWFactCreateRenderingParamsFuncType*          CreateRenderingParams;
    DWFactCreateMonitorRenderingParamsFuncType*   CreateMonitorRenderingParams;
    DWFactCreateCustomRenderingParamsFuncType*    CreateCustomRenderingParams;
    DWFactRegisterFontFileLoaderFuncType*         RegisterFontFileLoader;
    DWFactUnregisterFontFileLoaderFuncType*       UnregisterFontFileLoader;
    DWFactCreateTextFormatFuncType*               CreateTextFormat;
    DWFactCreateTypographyFuncType*               CreateTypography;
    DWFactGetGdiInteropFuncType*                  GetGdiInterop;
    DWFactCreateTextLayoutFuncType*               CreateTextLayout;
    DWFactCreateGdiCompatibleTextLayoutFuncType*  CreateGdiCompatibleTextLayout;
    DWFactCreateEllipsisTrimmingSignFuncType*     CreateEllipsisTrimmingSign;
    DWFactCreateTextAnalyzerFuncType*             CreateTextAnalyzer;
    DWFactCreateNumberSubstitutionFuncType*       CreateNumberSubstitution;
    DWFactCreateGlyphRunAnalysisFuncType*         CreateGlyphRunAnalysis;
    DWFact1GetEudcFontCollectionFuncType*         GetEudcFontCollection;
    DWFact1CreateCustomRenderingParamsFuncType*   CreateCustomRenderingParams1;
};

struct IDWriteFactory2Vtbl {
    IUnknownQueryInterfaceFuncType*               QueryInterface;
    IUnknownAddRefFuncType*                       AddRef;
    IUnknownReleaseFuncType*                      Release;
    DWFactGetSystemFontCollectionFuncType*        GetSystemFontCollection;
    DWFactCreateCustomFontCollectionFuncType*     CreateCustomFontCollection;
    DWFactRegisterFontCollectionLoaderFuncType*   RegisterFontCollectionLoader;
    DWFactUnregisterFontCollectionLoaderFuncType* UnregisterFontCollectionLoader;
    DWFactCreateFontFileReferenceFuncType*        CreateFontFileReference;
    DWFactCreateCustomFontFileReferenceFuncType*  CreateCustomFontFileReference;
    DWFactCreateFontFaceFuncType*                 CreateFontFace;
    DWFactCreateRenderingParamsFuncType*          CreateRenderingParams;
    DWFactCreateMonitorRenderingParamsFuncType*   CreateMonitorRenderingParams;
    DWFactCreateCustomRenderingParamsFuncType*    CreateCustomRenderingParams;
    DWFactRegisterFontFileLoaderFuncType*         RegisterFontFileLoader;
    DWFactUnregisterFontFileLoaderFuncType*       UnregisterFontFileLoader;
    DWFactCreateTextFormatFuncType*               CreateTextFormat;
    DWFactCreateTypographyFuncType*               CreateTypography;
    DWFactGetGdiInteropFuncType*                  GetGdiInterop;
    DWFactCreateTextLayoutFuncType*               CreateTextLayout;
    DWFactCreateGdiCompatibleTextLayoutFuncType*  CreateGdiCompatibleTextLayout;
    DWFactCreateEllipsisTrimmingSignFuncType*     CreateEllipsisTrimmingSign;
    DWFactCreateTextAnalyzerFuncType*             CreateTextAnalyzer;
    DWFactCreateNumberSubstitutionFuncType*       CreateNumberSubstitution;
    DWFactCreateGlyphRunAnalysisFuncType*         CreateGlyphRunAnalysis;
    DWFact1GetEudcFontCollectionFuncType*         GetEudcFontCollection;
    DWFact1CreateCustomRenderingParamsFuncType*   CreateCustomRenderingParams1;
    DWFact2GetSystemFontFallbackFuncType*         GetSystemFontFallback;
    DWFact2CreateFontFallbackBuilderFuncType*     CreateFontFallbackBuilder;
    DWFact2TranslateColorGlyphRunFuncType*        TranslateColorGlyphRun;
    DWFact2CreateCustomRenderingParamsFuncType*   CreateCustomRenderingParams2;
};

struct IDWriteRenderingParamsVtbl {
    IUnknownQueryInterfaceFuncType*            QueryInterface;
    IUnknownAddRefFuncType*                    AddRef;
    IUnknownReleaseFuncType*                   Release;
    DWRenderParamsGetGammaFuncType*            GetGamma;
    DWRenderParamsGetEnhancedContrastFuncType* GetEnhancedContrast;
    DWRenderParamsGetClearTypeLevelFuncType*   GetClearTypeLevel;
    DWRenderParamsGetPixelGeometryFuncType*    GetPixelGeometry;
    DWRenderParamsGetRenderingModeFuncType*    GetRenderingMode;
};

struct IDWriteRenderingParams1Vtbl {
    IUnknownQueryInterfaceFuncType*                      QueryInterface;
    IUnknownAddRefFuncType*                              AddRef;
    IUnknownReleaseFuncType*                             Release;
    DWRenderParamsGetGammaFuncType*                      GetGamma;
    DWRenderParamsGetEnhancedContrastFuncType*           GetEnhancedContrast;
    DWRenderParamsGetClearTypeLevelFuncType*             GetClearTypeLevel;
    DWRenderParamsGetPixelGeometryFuncType*              GetPixelGeometry;
    DWRenderParamsGetRenderingModeFuncType*              GetRenderingMode;
    DWRenderParams1GetGrayscaleEnhancedContrastFuncType* GetGrayscaleEnhancedContrast;
};

struct IDWriteRenderingParams2Vtbl {
    IUnknownQueryInterfaceFuncType*                      QueryInterface;
    IUnknownAddRefFuncType*                              AddRef;
    IUnknownReleaseFuncType*                             Release;
    DWRenderParamsGetGammaFuncType*                      GetGamma;
    DWRenderParamsGetEnhancedContrastFuncType*           GetEnhancedContrast;
    DWRenderParamsGetClearTypeLevelFuncType*             GetClearTypeLevel;
    DWRenderParamsGetPixelGeometryFuncType*              GetPixelGeometry;
    DWRenderParamsGetRenderingModeFuncType*              GetRenderingMode;
    DWRenderParams1GetGrayscaleEnhancedContrastFuncType* GetGrayscaleEnhancedContrast;
    DWRenderParams2GetGridFitModeFuncType*               GetGridFitMode;
};

struct IDWriteFontFileLoaderVtbl {
    IUnknownQueryInterfaceFuncType*              QueryInterface;
    IUnknownAddRefFuncType*                      AddRef;
    IUnknownReleaseFuncType*                     Release;
    DWFontFileLoaderCreateStreamFromKeyFuncType* CreateStreamFromKey;
};

struct IDWriteFontFileStreamVtbl {
    IUnknownQueryInterfaceFuncType*              QueryInterface;
    IUnknownAddRefFuncType*                      AddRef;
    IUnknownReleaseFuncType*                     Release;
    DWFontFileStreamReadFileFragmentFuncType*    ReadFileFragment;
    DWFontFileStreamReleaseFileFragmentFuncType* ReleaseFileFragment;
    DWFontFileStreamGetFileSizeFuncType*         GetFileSize;
    DWFontFileStreamGetLastWriteTimeFuncType*    GetLastWriteTime;
};

struct IDWriteFontFileVtbl {
    IUnknownQueryInterfaceFuncType*    QueryInterface;
    IUnknownAddRefFuncType*            AddRef;
    IUnknownReleaseFuncType*           Release;
    DWFontFileGetReferenceKeyFuncType* GetReferenceKey;
    DWFontFileGetLoaderFuncType*       GetLoader;
    DWFontFileAnalyzeFuncType*         Analyze;
};

struct IDWriteFontFaceVtbl {
    IUnknownQueryInterfaceFuncType*                 QueryInterface;
    IUnknownAddRefFuncType*                         AddRef;
    IUnknownReleaseFuncType*                        Release;
    DWFontFaceGetTypeFuncType*                      GetType;
    DWFontFaceGetFilesFuncType*                     GetFiles;
    DWFontFaceGetIndexFuncType*                     GetIndex;
    DWFontFaceGetSimulationsFuncType*               GetSimulations;
    DWFontFaceIsSymbolFontFuncType*                 IsSymbolFont;
    DWFontFaceGetMetricsFuncType*                   GetMetrics;
    DWFontFaceGetGlyphCountFuncType*                GetGlyphCount;
    DWFontFaceGetDesignGlyphMetricsFuncType*        GetDesignGlyphMetrics;
    DWFontFaceGetGlyphIndicesFuncType*              GetGlyphIndices;
    DWFontFaceTryGetFontTableFuncType*              TryGetFontTable;
    DWFontFaceReleaseFontTableFuncType*             ReleaseFontTable;
    DWFontFaceGetGlyphRunOutlineFuncType*           GetGlyphRunOutline;
    DWFontFaceGetRecommendedRenderingModeFuncType*  GetRecommendedRenderingMode;
    DWFontFaceGetGdiCompatibleMetricsFuncType*      GetGdiCompatibleMetrics;
    DWFontFaceGetGdiCompatibleGlyphMetricsFuncType* GetGdiCompatibleGlyphMetrics;
};

struct IDWriteFontFace1Vtbl {
    IUnknownQueryInterfaceFuncType*                 QueryInterface;
    IUnknownAddRefFuncType*                         AddRef;
    IUnknownReleaseFuncType*                        Release;
    DWFontFaceGetTypeFuncType*                      GetType;
    DWFontFaceGetFilesFuncType*                     GetFiles;
    DWFontFaceGetIndexFuncType*                     GetIndex;
    DWFontFaceGetSimulationsFuncType*               GetSimulations;
    DWFontFaceIsSymbolFontFuncType*                 IsSymbolFont;
    DWFontFaceGetMetricsFuncType*                   GetMetrics;
    DWFontFaceGetGlyphCountFuncType*                GetGlyphCount;
    DWFontFaceGetDesignGlyphMetricsFuncType*        GetDesignGlyphMetrics;
    DWFontFaceGetGlyphIndicesFuncType*              GetGlyphIndices;
    DWFontFaceTryGetFontTableFuncType*              TryGetFontTable;
    DWFontFaceReleaseFontTableFuncType*             ReleaseFontTable;
    DWFontFaceGetGlyphRunOutlineFuncType*           GetGlyphRunOutline;
    DWFontFaceGetRecommendedRenderingModeFuncType*  GetRecommendedRenderingMode;
    DWFontFaceGetGdiCompatibleMetricsFuncType*      GetGdiCompatibleMetrics;
    DWFontFaceGetGdiCompatibleGlyphMetricsFuncType* GetGdiCompatibleGlyphMetrics;
    void*                                           GetMetrics1;
    void*                                           GetGdiCompatibleMetrics1;
    void*                                           GetCaretMetrics;
    void*                                           GetUnicodeRanges;
    void*                                           IsMonospacedFont;
    void*                                           GetDesignGlyphAdvances;
    void*                                           GetGdiCompatibleGlyphAdvances;
    void*                                           GetKerningPairAdjustments;
    void*                                           HasKerningPairs;
    void*                                           GetRecommendedRenderingMode1;
    void*                                           GetVerticalGlyphVariants;
    void*                                           HasVerticalGlyphVariants;
};

struct IDWriteFontFace2Vtbl {
    IUnknownQueryInterfaceFuncType*                 QueryInterface;
    IUnknownAddRefFuncType*                         AddRef;
    IUnknownReleaseFuncType*                        Release;
    DWFontFaceGetTypeFuncType*                      GetType;
    DWFontFaceGetFilesFuncType*                     GetFiles;
    DWFontFaceGetIndexFuncType*                     GetIndex;
    DWFontFaceGetSimulationsFuncType*               GetSimulations;
    DWFontFaceIsSymbolFontFuncType*                 IsSymbolFont;
    DWFontFaceGetMetricsFuncType*                   GetMetrics;
    DWFontFaceGetGlyphCountFuncType*                GetGlyphCount;
    DWFontFaceGetDesignGlyphMetricsFuncType*        GetDesignGlyphMetrics;
    DWFontFaceGetGlyphIndicesFuncType*              GetGlyphIndices;
    DWFontFaceTryGetFontTableFuncType*              TryGetFontTable;
    DWFontFaceReleaseFontTableFuncType*             ReleaseFontTable;
    DWFontFaceGetGlyphRunOutlineFuncType*           GetGlyphRunOutline;
    DWFontFaceGetRecommendedRenderingModeFuncType*  GetRecommendedRenderingMode;
    DWFontFaceGetGdiCompatibleMetricsFuncType*      GetGdiCompatibleMetrics;
    DWFontFaceGetGdiCompatibleGlyphMetricsFuncType* GetGdiCompatibleGlyphMetrics;
    void*                                           GetMetrics1;
    void*                                           GetGdiCompatibleMetrics1;
    void*                                           GetCaretMetrics;
    void*                                           GetUnicodeRanges;
    void*                                           IsMonospacedFont;
    void*                                           GetDesignGlyphAdvances;
    void*                                           GetGdiCompatibleGlyphAdvances;
    void*                                           GetKerningPairAdjustments;
    void*                                           HasKerningPairs;
    void*                                           GetRecommendedRenderingMode1;
    void*                                           GetVerticalGlyphVariants;
    void*                                           HasVerticalGlyphVariants;
    void*                                           IsColorFont;
    void*                                           GetColorPaletteCount;
    void*                                           GetPaletteEntryCount;
    void*                                           GetPaletteEntries;
    void*                                           GetRecommendedRenderingMode2;
};

struct IDWriteGDIInteropVtbl {
    IUnknownQueryInterfaceFuncType*               QueryInterface;
    IUnknownAddRefFuncType*                       AddRef;
    IUnknownReleaseFuncType*                      Release;
    DWGdiInteropCreateFontFromLOGFONTFuncType*    CreateFontFromLOGFONT;
    DWGdiInteropConvertFontToLOGFONTFuncType*     ConvertFontToLOGFONT;
    DWGdiInteropConvertFontFaceToLOGFONTFuncType* ConvertFontFaceToLOGFONT;
    DWGdiInteropCreateFontFaceFromHdcFuncType*    CreateFontFaceFromHdc;
    DWGdiInteropCreateBitmapRenderTargetFuncType* CreateBitmapRenderTarget;
};

struct IDWriteBitmapRenderTargetVtbl {
    IUnknownQueryInterfaceFuncType*                   QueryInterface;
    IUnknownAddRefFuncType*                           AddRef;
    IUnknownReleaseFuncType*                          Release;
    DWBitmapRenderTargetDrawGlyphRunFuncType*         DrawGlyphRun;
    DWBitmapRenderTargetGetMemoryDCFuncType*          GetMemoryDC;
    DWBitmapRenderTargetGetPixelsPerDipFuncType*      GetPixelsPerDip;
    DWBitmapRenderTargetSetPixelsPerDipFuncType*      SetPixelsPerDip;
    DWBitmapRenderTargetGetCurrentTransformFuncType*  GetCurrentTransform;
    DWBitmapRenderTargetSetCurrentTransformFuncType*  SetCurrentTransform;
    DWBitmapRenderTargetGetSizeFuncType*              GetSize;
    DWBitmapRenderTargetResizeFuncType*               Resize;
};

struct IDWriteBitmapRenderTarget1Vtbl {
    IUnknownQueryInterfaceFuncType*                    QueryInterface;
    IUnknownAddRefFuncType*                            AddRef;
    IUnknownReleaseFuncType*                           Release;
    DWBitmapRenderTargetDrawGlyphRunFuncType*          DrawGlyphRun;
    DWBitmapRenderTargetGetMemoryDCFuncType*           GetMemoryDC;
    DWBitmapRenderTargetGetPixelsPerDipFuncType*       GetPixelsPerDip;
    DWBitmapRenderTargetSetPixelsPerDipFuncType*       SetPixelsPerDip;
    DWBitmapRenderTargetGetCurrentTransformFuncType*   GetCurrentTransform;
    DWBitmapRenderTargetSetCurrentTransformFuncType*   SetCurrentTransform;
    DWBitmapRenderTargetGetSizeFuncType*               GetSize;
    DWBitmapRenderTargetResizeFuncType*                Resize;
    DWBitmapRenderTarget1GetTextAntialiasModeFuncType* GetTextAntialiasMode;
    DWBitmapRenderTarget1SetTextAntialiasModeFuncType* SetTextAntialiasMode;
};

// @runtime____________________________________________________________________
constexpr IID IID_IDWriteFactory = {
    0xB859EE5A,
    0xD838,
    0x4B5B,
    {
        0xA2, 
        0xE8, 
        0x1A, 
        0xDC, 
        0x7D, 
        0x93, 
        0xDB, 
        0x48 
    }
};

constexpr IID IID_IDWriteFactory1 = {
    0x30572F99,
    0xDAC6,
    0x41DB,
    {
        0xA1, 
        0x6E, 
        0x04, 
        0x86, 
        0x30, 
        0x7E, 
        0x60, 
        0x6A 
    }
};

constexpr IID IID_IDWriteFactory2 = {
    0x0439FC60,
    0xCA44,
    0x4994,
    {
        0x8D,
        0xEE,
        0x3A,
        0x9A,
        0xF7,
        0xB7,
        0x32,
        0xEC
    }
};

// @functions__________________________________________________________________
EXTERN_C_LINK DLL_IMPORT HRESULT WINAPI DWriteCreateFactory(DWRITE_FACTORY_TYPE FactoryType, REFIID IID, void** Factory);

#endif // __DWRITE_PLATFORM_H__
