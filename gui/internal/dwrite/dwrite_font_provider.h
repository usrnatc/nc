#if !defined(__DWRITE_FONT_PROVIDER_H__)
#define __DWRITE_FONT_PROVIDER_H__

#include "../../cache/nc_font_provider.h"
#include "dwrite_platform.h"
#include "../../nc_string.h"

struct FPFontFileLoader;
struct FPFontFileStream;

// @types______________________________________________________________________
struct FPFontFileStream {
    IDWriteFontFileStreamVtbl* VTable;
    Str8*                      Data;
};

struct FPFontFileStreamNode {
    FPFontFileStreamNode* Next;
    FPFontFileStreamNode* Prev;
    FPFontFileStream      Stream;
};

struct FPState {
    Arena*                     MemPool;
    b32                        V2IsSupported;
    IDWriteFactory*            Factory;
    IDWriteRenderingParams*    BaseRenderingParams;
    IDWriteRenderingParams*    RenderingParamsSharpHinted;
    IDWriteRenderingParams*    RenderingParamsSharpUnhinted;
    IDWriteRenderingParams*    RenderingParamsSmoothHinted;
    IDWriteRenderingParams*    RenderingParamsSmoothUnhinted;
    IDWriteGDIInterop*         GDIInterop;
    v2i32                      BMPRenderTargetDimensions;
    IDWriteBitmapRenderTarget* BMPRenderTarget;
    FPFontFileStreamNode*      HeadStreamNode;
    FPFontFileStreamNode*      TailStreamNode;
    FPFontFileStreamNode*      FreeStreamNode;
};

struct FPFont {
    IDWriteFontFile* File;
    IDWriteFontFace* Face;
};

// @runtime____________________________________________________________________
global FPState* FP_STATE = NULL;

// @functions__________________________________________________________________
FPFont FPFontFromHandle(FPHandle FontHandle);
FPHandle FPHandleFromFont(FPFont Font);
FPFontFileStreamNode* FPFileStreamNodeAlloc(Str8* Data);
void FPFileStreamNodeRelease(FPFontFileStreamNode* Node);
HRESULT FPIUnknownNoOpQueryInterface(void* Obj, REFIID RIID, void* ObjPtr);
ULONG FPIUnknownNoOpAddRef(void* Obj);
ULONG FPIUnknownNoOpRelease(void* Obj);
HRESULT FPSFontFileLoaderStreamFromKey(FPFontFileLoader* Obj, const void* FontFileRefKey, UINT32 FontFileRefKeySize, OUT IDWriteFontFileStream** Stream);
HRESULT FPSFontFileStreamReadFileFragment(FPFontFileStream* Obj, OUT const void** FragmentStart, UINT64 FileOffset, UINT64 FragmentSize, OUT void** FragmentContext);
HRESULT FPSFontFileStreamReleaseFileFragment(FPFontFileStream* Obj, void* FragmentContext);
HRESULT FPSFontFileStreamGetFileSize(FPFontFileStream* Obj, OUT UINT64* Size);
HRESULT FPSFontFileStreamGetLastWriteTime(FPFontFileStream* Obj, OUT UINT64* Time);
FPRasterResult Raster(Arena* MemPool, FPHandle FontHandle, f32 Size, FPRasterKind Kind, Str8 String);
FPHandle OpenFont(Str8 Path);
FPHandle FontOpen(Str8* Path);

#endif // __DWRITE_FONT_PROVIDER_H__
