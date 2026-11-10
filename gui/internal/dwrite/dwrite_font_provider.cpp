#include "../win32/win32_platform.h"
#include "dwrite_platform.h"
#include "dwrite_font_provider.h"
#include "../../nc_memory.h"
#include "../../cache/nc_font_provider.h"
#include "../../nc_arena.h"
#include "../../nc_file.h"

global IDWriteFontFileLoaderVtbl FP_FONT_FILE_LOADER_VTBL = {
    (IUnknownQueryInterfaceFuncType*) FPIUnknownNoOpQueryInterface,
    (IUnknownAddRefFuncType*) FPIUnknownNoOpAddRef,
    (IUnknownReleaseFuncType*) FPIUnknownNoOpRelease,
    (DWFontFileLoaderCreateStreamFromKeyFuncType*) FPSFontFileLoaderStreamFromKey
};

global IDWriteFontFileLoader FP_FONT_FILE_LOADER = {
    &FP_FONT_FILE_LOADER_VTBL
};

global IDWriteFontFileStreamVtbl FP_FONT_FILE_STREAM_VTBL = {
    (IUnknownQueryInterfaceFuncType*) FPIUnknownNoOpQueryInterface,
    (IUnknownAddRefFuncType*) FPIUnknownNoOpAddRef,
    (IUnknownReleaseFuncType*) FPIUnknownNoOpRelease,
    (DWFontFileStreamReadFileFragmentFuncType*) FPSFontFileStreamReadFileFragment,
    (DWFontFileStreamReleaseFileFragmentFuncType*) FPSFontFileStreamReleaseFileFragment,
    (DWFontFileStreamGetFileSizeFuncType*) FPSFontFileStreamGetFileSize,
    (DWFontFileStreamGetLastWriteTimeFuncType*) FPSFontFileStreamGetLastWriteTime
};

FPFont 
FPFontFromHandle(FPHandle FontHandle)
{
    FPFont Result = {};

    Result.File = (IDWriteFontFile*) PTR_FROM_INT(FontHandle.V[0]);
    Result.Face = (IDWriteFontFace*) PTR_FROM_INT(FontHandle.V[1]);

    return Result;
}

FPHandle 
FPHandleFromFont(FPFont Font)
{
    FPHandle Result = {};

    Result.V[0] = INT_FROM_PTR(Font.File);
    Result.V[1] = INT_FROM_PTR(Font.Face);

    return Result;
}

FPFontFileStreamNode* 
FPFileStreamNodeAlloc(Str8* Data)
{
    FPFontFileStreamNode* Result = NULL;

    for (FPFontFileStreamNode* Node = FP_STATE->HeadStreamNode; Node; Node = Node->Next) {
        if (Node->Stream.Data == Data) {
            Result = Node;
            break;
        }
    }

    if (!Result) {
        Result = FP_STATE->FreeStreamNode;

        if (Result)
            SLL_STACK_POP(FP_STATE->FreeStreamNode);
        else {
            Result = ArenaPushArray(FP_STATE->MemPool, FPFontFileStreamNode, 1);
        }

        MemSet(Result, 0, sizeof(*Result));
        Result->Stream.VTable = &FP_FONT_FILE_STREAM_VTBL;
        Result->Stream.Data = Data;
        DLL_PUSH_BACK(FP_STATE->HeadStreamNode, FP_STATE->TailStreamNode, Result);
    }

    return Result;
}

void 
FPFileStreamNodeRelease(FPFontFileStreamNode* Node)
{
    DLL_REMOVE(FP_STATE->HeadStreamNode, FP_STATE->TailStreamNode, Node);
    SLL_STACK_PUSH(FP_STATE->FreeStreamNode, Node);
}

HRESULT 
FPIUnknownNoOpQueryInterface(void* Obj, REFIID RIID, void* ObjPtr)
{
    return E_NOINTERFACE;
}

ULONG 
FPIUnknownNoOpAddRef(void* Obj)
{
    ULONG Result = 1;

    return Result;
}

ULONG 
FPIUnknownNoOpRelease(void* Obj)
{
    ULONG Result = 1;

    return Result;
}

HRESULT 
FPSFontFileLoaderStreamFromKey(
    FPFontFileLoader* Obj, 
    const void* FontFileRefKey, 
    UINT32 FontFileRefKeySize, 
    OUT IDWriteFontFileStream** Stream
) {
    HRESULT Result = S_OK;
    Str8* Key = *(Str8**) FontFileRefKey;
    FPFontFileStreamNode* Node = FPFileStreamNodeAlloc(Key);

    *Stream = (IDWriteFontFileStream*) &Node->Stream; 

    return Result;
}

HRESULT 
FPSFontFileStreamReadFileFragment(
    FPFontFileStream* Obj, 
    OUT const void** FragmentStart, 
    UINT64 FileOffset, 
    UINT64 FragmentSize, 
    OUT void** FragmentContext
) {
    HRESULT Result = S_OK;

    *FragmentStart = Obj->Data->Str + FileOffset;
    *FragmentContext = NULL;

    return Result;
}

HRESULT 
FPSFontFileStreamReleaseFileFragment(
    FPFontFileStream* Obj, 
    void* FragmentContext
) {
    HRESULT Result = S_OK;

    return Result;
}

HRESULT 
FPSFontFileStreamGetFileSize(
    FPFontFileStream* Obj, 
    OUT UINT64* Size
) {
    HRESULT Result = S_OK;

    *Size = Obj->Data->Size;

    return Result;
}

HRESULT 
FPSFontFileStreamGetLastWriteTime(
    FPFontFileStream* Obj, 
    OUT UINT64* Time
) {
    HRESULT Result = S_OK;

    *Time = 0;

    return Result;
}

EXTERN_C_LINK void 
FontProviderInit(void)
{
    HRESULT Error = 0;
    Arena* MemPool = ArenaAlloc();

    FP_STATE = ArenaPushArrayZero(MemPool, FPState, 1);
    FP_STATE->MemPool = MemPool;

    Error = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED, 
        IID_IDWriteFactory2, 
        (void**) &FP_STATE->Factory
    );

    if (Error == S_OK) {
        FP_STATE->V2IsSupported = TRUE;
    } else {
        Error = DWriteCreateFactory(
            DWRITE_FACTORY_TYPE_SHARED, 
            IID_IDWriteFactory, 
            (void**) &FP_STATE->Factory
        );
    }

    Error = FP_STATE->Factory->Vtbl->RegisterFontFileLoader(
        FP_STATE->Factory, 
        &FP_FONT_FILE_LOADER
    );
    Error = FP_STATE->Factory->Vtbl->CreateRenderingParams(
        FP_STATE->Factory, 
        &FP_STATE->BaseRenderingParams
    );

    {
        FLOAT Gamma = FP_STATE->BaseRenderingParams->Vtbl->GetGamma(
            FP_STATE->BaseRenderingParams
        );

        Gamma = 1.0f;

        FLOAT EnhancedContrast = FP_STATE->BaseRenderingParams->Vtbl->GetEnhancedContrast(
            FP_STATE->BaseRenderingParams
        );

        if (FP_STATE->V2IsSupported) {
            IDWriteFactory2* Factory2 = (IDWriteFactory2*) FP_STATE->Factory;

            Error = Factory2->Vtbl->CreateCustomRenderingParams2(
                Factory2,
                Gamma,
                EnhancedContrast,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_GDI_NATURAL,
                DWRITE_GRID_FIT_MODE_ENABLED,
                (IDWriteRenderingParams2**) &FP_STATE->RenderingParamsSharpHinted
            );
        } else {
            Error = FP_STATE->Factory->Vtbl->CreateCustomRenderingParams(
                FP_STATE->Factory,
                Gamma,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_GDI_NATURAL,
                &FP_STATE->RenderingParamsSharpHinted
            );
        }
    }

    {
        FLOAT Gamma = FP_STATE->BaseRenderingParams->Vtbl->GetGamma(
            FP_STATE->BaseRenderingParams
        );

        Gamma = 1.0f;

        FLOAT EnhancedContrast = FP_STATE->BaseRenderingParams->Vtbl->GetEnhancedContrast(
            FP_STATE->BaseRenderingParams
        );

        if (FP_STATE->V2IsSupported) {
            IDWriteFactory2* Factory2 = (IDWriteFactory2*) FP_STATE->Factory;

            Error = Factory2->Vtbl->CreateCustomRenderingParams2(
                Factory2,
                Gamma,
                EnhancedContrast,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_GDI_NATURAL,
                DWRITE_GRID_FIT_MODE_DISABLED,
                (IDWriteRenderingParams2**) &FP_STATE->RenderingParamsSharpUnhinted
            );
        } else {
            Error = FP_STATE->Factory->Vtbl->CreateCustomRenderingParams(
                FP_STATE->Factory,
                Gamma,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_GDI_NATURAL,
                &FP_STATE->RenderingParamsSharpUnhinted
            );
        }
    }

    {
        FLOAT Gamma = FP_STATE->BaseRenderingParams->Vtbl->GetGamma(
            FP_STATE->BaseRenderingParams
        );

        Gamma = 1.0f;

        FLOAT EnhancedContrast = FP_STATE->BaseRenderingParams->Vtbl->GetEnhancedContrast(
            FP_STATE->BaseRenderingParams
        );

        if (FP_STATE->V2IsSupported) {
            IDWriteFactory2* Factory2 = (IDWriteFactory2*) FP_STATE->Factory;

            Error = Factory2->Vtbl->CreateCustomRenderingParams2(
                Factory2,
                Gamma,
                EnhancedContrast,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                DWRITE_GRID_FIT_MODE_ENABLED,
                (IDWriteRenderingParams2**) &FP_STATE->RenderingParamsSmoothHinted
            );
        } else {
            Error = FP_STATE->Factory->Vtbl->CreateCustomRenderingParams(
                FP_STATE->Factory,
                Gamma,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                &FP_STATE->RenderingParamsSmoothHinted
            );
        }
    }

    {
        FLOAT Gamma = 1.0f;
        FLOAT EnhancedContrast = 0.0f;

        if (FP_STATE->V2IsSupported) {
            IDWriteFactory2* Factory2 = (IDWriteFactory2*) FP_STATE->Factory;

            Error = Factory2->Vtbl->CreateCustomRenderingParams2(
                Factory2,
                Gamma,
                EnhancedContrast,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                DWRITE_GRID_FIT_MODE_DISABLED,
                (IDWriteRenderingParams2**) &FP_STATE->RenderingParamsSmoothUnhinted
            );
        } else {
            Error = FP_STATE->Factory->Vtbl->CreateCustomRenderingParams(
                FP_STATE->Factory,
                Gamma,
                EnhancedContrast,
                0.0f,
                DWRITE_PIXEL_GEOMETRY_FLAT,
                DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
                &FP_STATE->RenderingParamsSmoothUnhinted
            );
        }
    }

    Error = FP_STATE->Factory->Vtbl->GetGdiInterop(
        FP_STATE->Factory, 
        &FP_STATE->GDIInterop
    );
    FP_STATE->BMPRenderTargetDimensions = Vec(2048, 256);
    Error = FP_STATE->GDIInterop->Vtbl->CreateBitmapRenderTarget(
        FP_STATE->GDIInterop,
        NULL,
        FP_STATE->BMPRenderTargetDimensions.X,
        FP_STATE->BMPRenderTargetDimensions.Y,
        &FP_STATE->BMPRenderTarget
    );
    FP_STATE->BMPRenderTarget->Vtbl->SetPixelsPerDip(
        FP_STATE->BMPRenderTarget, 
        1.0f
    );
}

struct PathTask {
    PathTask* Next;
    Str8 Path;
};

EXTERN_C_LINK FPHandle 
OpenFont(Str8 Path)
{
    TempArena Scratch = GetScratch(NULL, 0);
    FPFont Font = {};
    HRESULT Error = 0;

    PathTask StartTask = {};
    PathTask* HeadTask = &StartTask;
    PathTask* TailTask = HeadTask;

    StartTask.Next = NULL;
    StartTask.Path = Path;

    for (PathTask* Task = HeadTask; Task && !Font.File; Task = Task->Next) {
        b32 FileExists = !!(SystemGetFileProperties(Task->Path).Created);
        Str16 WidePath = Str16FromStr8(Scratch.MemPool, Path);

        if (FileExists) {
            Error = FP_STATE->Factory->Vtbl->CreateFontFileReference(
                FP_STATE->Factory,
                (WCHAR*) WidePath.Str,
                NULL,
                &Font.File
            );
        }

        if (Font.File) {
            Error = FP_STATE->Factory->Vtbl->CreateFontFace(
                FP_STATE->Factory,
                DWRITE_FONT_FACE_TYPE_TRUETYPE,
                1,
                &Font.File,
                0,
                DWRITE_FONT_SIMULATIONS_NONE,
                &Font.Face
            );
        }

        if (Task == HeadTask && !Font.File && Task->Path.Size) {
            HKEY RegKey = NULL;
            LSTATUS Status = 0;
            char Name[256] = {};
            char Data[256] = {};
            DWORD NameSize = ARRAY_COUNT(Name);
            DWORD DataSize = ARRAY_COUNT(Data);
            DWORD Type = 0;

            Status = RegOpenKeyExW(
                HKEY_CURRENT_USER, 
                TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Folders\\Fonts"), 
                0, 
                KEY_QUERY_VALUE, 
                &RegKey
            );

            Status = RegEnumValueA(
                RegKey, 
                0, 
                Name, 
                &NameSize, 
                NULL, 
                &Type, 
                (unsigned char*) Data, 
                &DataSize
            );

            Str8 UserFontsPath = CStr8(Data);
            PathTask* FontFindTask = ArenaPushArray(Scratch.MemPool, PathTask, 1);

            FontFindTask->Path = ArenaPushStrFmt(Scratch.MemPool, (char*) "%s/%s", UserFontsPath, Path);
            SLL_QUEUE_PUSH(HeadTask, TailTask, FontFindTask);

            char WindowsPath[256] = {};

            GetWindowsDirectoryA(WindowsPath, ARRAY_COUNT(WindowsPath));

            PathTask* FindSystemFontsTask = ArenaPushArray(Scratch.MemPool, PathTask, 1);

            FindSystemFontsTask->Path = ArenaPushStrFmt(Scratch.MemPool, (char*) "%s/Fonts/%s", WindowsPath, Path);
            SLL_QUEUE_PUSH(HeadTask, TailTask, FindSystemFontsTask);
        }
    }

    FPHandle Result = {};

    if (Font.File)
        Result = FPHandleFromFont(Font);

    ReleaseScratch(Scratch);

    return Result;
}

EXTERN_C_LINK FPHandle 
FontOpen(Str8* Data)
{
    TempArena Scratch = GetScratch(NULL, 0);
    FPFont Font = {};
    HRESULT Error = 0;

    Error = FP_STATE->Factory->Vtbl->CreateCustomFontFileReference(
        FP_STATE->Factory,
        &Data,
        sizeof(Str8*),
        &FP_FONT_FILE_LOADER,
        &Font.File
    );

    Error = FP_STATE->Factory->Vtbl->CreateFontFace(
        FP_STATE->Factory,
        DWRITE_FONT_FACE_TYPE_TRUETYPE,
        1,
        &Font.File,
        0,
        DWRITE_FONT_SIMULATIONS_NONE,
        &Font.Face
    );

    FPHandle Result = FPHandleFromFont(Font);

    ReleaseScratch(Scratch);

    return Result;
}

EXTERN_C_LINK void 
CloseFont(FPHandle FontHandle)
{
    FPFont Font = FPFontFromHandle(FontHandle);

    if (Font.Face)
        Font.Face->Vtbl->Release(Font.Face);

    if (Font.File)
        Font.File->Vtbl->Release(Font.File);
}

EXTERN_C_LINK FPMetrics 
MetricsFromFont(FPHandle FontHandle)
{
    FPFont Font = FPFontFromHandle(FontHandle);
    DWRITE_FONT_METRICS Metrics = {};

    if (Font.Face)
        Font.Face->Vtbl->GetMetrics(Font.Face, &Metrics);

    FPMetrics Result = {};

    {
        Result.DesignUnitsPerEM = (f32) Metrics.DesignUnitsPerEm;
        Result.Ascent = (f32) Metrics.Ascent;
        Result.Descent = (f32) Metrics.Descent;
        Result.LineGap = (f32) Metrics.LineGap;
        Result.CapitalHeight = (f32) Metrics.CapHeight;
    }

    return Result;
}

EXTERN_C_LINK NO_ASAN FPRasterResult 
Raster(
    Arena* MemPool, 
    FPHandle FontHandle, 
    f32 Size, 
    FPRasterKind Kind, 
    Str8 String
) {
    TempArena Scratch = GetScratch(&MemPool, 1);
    HRESULT Error = 0;
    Str32 String32 = Str32FromStr8(Scratch.MemPool, String);
    FPFont Font = FPFontFromHandle(FontHandle);
    COLORREF BackgroundColour = RGB(0, 0, 0);
    COLORREF ForegroundColour = RGB(255, 255, 255);
    DWRITE_FONT_METRICS FontMetrics = {};

    if (Font.Face)
        Font.Face->Vtbl->GetMetrics(Font.Face, &FontMetrics);

    f32 DesignUnitsPerEM = (f32) FontMetrics.DesignUnitsPerEm;
    u16* GlyphIndices = ArenaPushArray(Scratch.MemPool, u16, String32.Size);

    if (Font.Face) {
        Error = Font.Face->Vtbl->GetGlyphIndices(
            Font.Face, 
            String32.Str, 
            String32.Size, 
            GlyphIndices
        );
    }

    u64 GlyphsCount = String32.Size;
    DWRITE_GLYPH_METRICS* GlyphsMetrics = ArenaPushArray(
        Scratch.MemPool, 
        DWRITE_GLYPH_METRICS, 
        GlyphsCount
    );

    if (Font.Face) {
        Error = Font.Face->Vtbl->GetGdiCompatibleGlyphMetrics(
            Font.Face,
            (96.0f / 72.0f) * Size,
            1.0f,
            NULL,
            TRUE,
            GlyphIndices,
            GlyphsCount,
            GlyphsMetrics,
            FALSE
        );
    }

    f32 Advance = 0.0f;
    v2i16 AtlasDimensions = {};
    f32 LeftSideBearing = 0.0f;
    f32 RightSideBearing = 0.0f;

    if (Font.Face) {
        AtlasDimensions.Y = (i16) RoundF32(
            (96.0f / 72.0f) * 
            Size * 
            (
                FontMetrics.Ascent + 
                FontMetrics.Descent + 
                FontMetrics.LineGap
            ) / 
            DesignUnitsPerEM
        ) + 1;

        for (u64 I = 0; I < GlyphsCount; ++I) {
            DWRITE_GLYPH_METRICS* GlyphMetrics = GlyphsMetrics + I;
            f32 GlyphAdvanceWidth = (96.0f / 72.0f) * Size * GlyphMetrics->AdvanceWidth / DesignUnitsPerEM;

            Advance += GlyphAdvanceWidth;
            AtlasDimensions.X = MAX(AtlasDimensions.X, (i16) (Advance + 1));

            if (!I)
                LeftSideBearing = (96.0f / 72.0f) * Size * GlyphMetrics->LeftSideBearing / DesignUnitsPerEM;

            if (I + 1 == GlyphsCount)
                RightSideBearing = (96.0f / 72.0f) * Size * GlyphMetrics->RightSideBearing / DesignUnitsPerEM;
        }

        AtlasDimensions.X -= RightSideBearing;
        AtlasDimensions.X += 2;
        AtlasDimensions.X += 7;
        AtlasDimensions.X -= (AtlasDimensions.X % 8);
    }

    IDWriteBitmapRenderTarget* RenderTarget = NULL;

    if (Font.Face) {
        Error = FP_STATE->GDIInterop->Vtbl->CreateBitmapRenderTarget(
            FP_STATE->GDIInterop, 
            NULL, 
            AtlasDimensions.X, 
            AtlasDimensions.Y, 
            &RenderTarget
        );

        RenderTarget->Vtbl->SetPixelsPerDip(RenderTarget, 1.0f);
    }

    HDC DeviceContext = NULL;

    if (Font.Face) {
        DeviceContext = RenderTarget->Vtbl->GetMemoryDC(RenderTarget);

        HGDIOBJ Original = SelectObject(DeviceContext, GetStockObject(DC_PEN));

        SetDCPenColor(DeviceContext, BackgroundColour);
        SelectObject(DeviceContext, GetStockObject(DC_BRUSH));
        SetDCBrushColor(DeviceContext, BackgroundColour);
        Rectangle(DeviceContext, 0, 0, AtlasDimensions.X, AtlasDimensions.Y);
        SelectObject(DeviceContext, Original);
    }

    v2f32 DrawP = Vec(0.0f, (f32) AtlasDimensions.Y);

    if (Font.Face) {
        f32 Descent = RoundF32((96.0f / 72.0f) * Size * FontMetrics.Descent / DesignUnitsPerEM);
        f32 LineGap = RoundF32((96.0f / 72.0f) * Size * FontMetrics.LineGap / DesignUnitsPerEM);

        DrawP.Y -= Descent;
        DrawP.Y -= LineGap;
    }

    DWRITE_GLYPH_RUN GlyphRun = {};

    if (Font.Face) {
        GlyphRun.FontFace = Font.Face;
        GlyphRun.FontEmSize = Size * 96.0f / 72.0f;
        GlyphRun.GlyphCount = String32.Size;
        GlyphRun.GlyphIndices = GlyphIndices;
    }

    RECT BoundingBox = {};

    if (Font.Face) {
        IDWriteRenderingParams* RenderingParams = FP_STATE->RenderingParamsSharpHinted;

        switch (Kind) {
            default: {} break;

            case 0: {
                RenderingParams = FP_STATE->RenderingParamsSharpUnhinted;
            } break;

            case FP_RASTER_KIND_HINTED: {
                RenderingParams = FP_STATE->RenderingParamsSharpHinted;
            } break;

            case FP_RASTER_KIND_SMOOTH: {
                RenderingParams = FP_STATE->RenderingParamsSmoothUnhinted;
            } break;

            case (FP_RASTER_KIND_SMOOTH | FP_RASTER_KIND_HINTED): {
                RenderingParams = FP_STATE->RenderingParamsSmoothHinted;
            } break;
        }

        Error = RenderTarget->Vtbl->DrawGlyphRun(
            RenderTarget,
            DrawP.X,
            DrawP.Y,
            DWRITE_MEASURING_MODE_NATURAL,
            &GlyphRun,
            RenderingParams,
            ForegroundColour,
            &BoundingBox
        );
    }

    DIBSECTION DIB = {};

    if (Font.Face) {
        HBITMAP Bitmap = (HBITMAP) GetCurrentObject(DeviceContext, OBJ_BITMAP);

        GetObject(Bitmap, sizeof(DIB), &DIB);
    }

    FPRasterResult Result = {};

    if (Font.Face) {
        Result.AtlasDimensions = AtlasDimensions;
        Result.Atlas = ArenaPushArray(MemPool, u8, AtlasDimensions.X * AtlasDimensions.Y * 4);
        Result.Advance = FloorF32(Advance);

        u8* DataIN = (u8*) DIB.dsBm.bmBits;
        u64 PitchIN = (u64) DIB.dsBm.bmWidthBytes;
        u8* DataOUT = (u8*) Result.Atlas;
        u64 PitchOUT = AtlasDimensions.X * 4;
        u8* LineIN = (u8*) DataIN;
        u8* LineOUT = DataOUT;
        u64 ColourSum = 0;

        for (u64 Y = 0; Y < AtlasDimensions.Y; ++Y) {
            u8* PixelIN = LineIN;
            u8* PixelOUT = LineOUT;

            for (u64 X = 0; X < AtlasDimensions.X; ++X) {
                u8 PixelByteIN = *PixelIN;

                PixelOUT[0] = 255;
                PixelOUT[1] = 255;
                PixelOUT[2] = 255;
                PixelOUT[3] = PixelByteIN;
                ColourSum += PixelByteIN;
                PixelIN += 4;
                PixelOUT += 4;
            }

            LineIN += PitchIN;
            LineOUT += PitchOUT;
        }

        if (!ColourSum)
            Result.AtlasDimensions = Vec((i16) 0, (i16) 0);

        RenderTarget->Vtbl->Release(RenderTarget);
    }

    ReleaseScratch(Scratch);

    return Result;
}
