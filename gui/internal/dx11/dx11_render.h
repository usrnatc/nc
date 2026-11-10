#if !defined(__DX11_RENDER_H__)
#define __DX11_RENDER_H__

#include "../../nc_render.h"
#include "../../nc_system.h"
#include "dx11_platform.h"

// @defines____________________________________________________________________
enum DX11VertShaderKind {
    VERT_SHADER_RECT,
    VERT_SHADER_BLUR,
    VERT_SHADER_MESH,
    VERT_SHADER_GEO3D_COMPOSITE,
    VERT_SHADER_FINALISE,
    VERT_SHADER_COUNT
};

enum DX11PixelShaderKind {
    PIXEL_SHADER_RECT,
    PIXEL_SHADER_BLUR,
    PIXEL_SHADER_MESH,
    PIXEL_SHADER_GEO3D_COMPOSITE,
    PIXEL_SHADER_FINALISE,
    PIXEL_SHADER_COUNT
};

enum DX11UniformKind {
    UNIFORM_KIND_RECT,
    UNIFORM_KIND_BLUR,
    UNIFORM_KIND_MESH,
    UNIFORM_KIND_COUNT
};

// @types______________________________________________________________________
struct DX11UniformsRect {
    v2f32 ViewportSize;
    f32   Opacity;
    f32   __Padding0;
    m4f32 TexSampleChannelMap;
    v2f32 Tex2DSize;
    v2f32 Translate;
    v4f32 XForm[3];
    v2f32 XFormScale;
};

struct DX11UniformsBlurPass {
    r2f32 Rect;
    v4f32 CornerRadii;
    v2f32 Direction;
    v2f32 ViewportSize;
    u32   BlurCount;
    u8    __Padding0[204];
};
STATIC_ASSERT(sizeof(DX11UniformsBlurPass) % 256 == 0, DX11UniformsBlurPassAlignedCheck);

struct DX11UniformsBlur {
    DX11UniformsBlurPass Passes[AXIS_2D_COUNT];
    v4f32                Kernel[32];
};

struct DX11UniformsMesh {
    m4f32 XForm;
};

struct DX11Tex2D {
    DX11Tex2D*                Next;
    u64                       Generation;
    ID3D11Texture2D*          Texture;
    ID3D11ShaderResourceView* View;
    RendResourceKind          Kind;
    v2i32                     Size;
    RendTex2DFmt              Format;
};

struct DX11Buffer {
    DX11Buffer*      Next;
    u64              Generation;
    ID3D11Buffer*    Buffer;
    RendResourceKind Kind;
    u64              Size;
};

struct DX11Window {
    DX11Window*               Next;
    u64                       Generation;
    IDXGISwapChain1*          SwapChain;
    ID3D11Texture2D*          FrameBuffer;
    ID3D11RenderTargetView*   FrameBufferRTV;
    ID3D11Texture2D*          StageColour;
    ID3D11RenderTargetView*   StageColourRTV;
    ID3D11ShaderResourceView* StageColourSRV;
    ID3D11Texture2D*          StageScratchColour;
    ID3D11RenderTargetView*   StageScratchColourRTV;
    ID3D11ShaderResourceView* StageScratchColourSRV;
    ID3D11Texture2D*          Geo3DColour;
    ID3D11RenderTargetView*   Geo3DColourRTV;
    ID3D11ShaderResourceView* Geo3DColourSRV;
    ID3D11Texture2D*          Geo3DDepth;
    ID3D11DepthStencilView*   Geo3DDepthDSV;
    ID3D11ShaderResourceView* Geo3DDepthSRV;
    v2i32                     PrevResolution;
};

struct DX11FlushBuffer {
    DX11FlushBuffer* Next;
    ID3D11Buffer*    Buffer;
};

struct DX11State {
    Arena*                   MemPool;
    u64                      WindowCount;
    DX11Window*              FreeWindow;
    DX11Tex2D*               FreeTex2D;
    DX11Buffer*              FreeBuffer;
    DX11Tex2D*               ToFreeTex2D;
    DX11Buffer*              ToFreeBuffer;
    Handle                   RWMtx;
    ID3D11Device*            BaseDevice;
    ID3D11DeviceContext*     BaseDeviceContext;
    ID3D11Device1*           Device;
    ID3D11DeviceContext1*    DeviceContext;
    IDXGIDevice1*            DXGIDevice;
    IDXGIAdapter*            DXGIAdapter;
    IDXGIFactory2*           DXGIFactory;
    ID3D11RasterizerState1*  MainRasteriser;
    ID3D11BlendState*        MainBlendState;
    ID3D11BlendState*        NoBlendState;
    ID3D11SamplerState*      Samplers[TEX2D_SAMPLE_COUNT];
    ID3D11DepthStencilState* NoOpDepthStencil;
    ID3D11DepthStencilState* PlainDepthStencil;
    ID3D11Buffer*            InstanceScratchBuffer64KB;
    RendHandle               BackupTexture;
    ID3D11VertexShader*      VertShaders[VERT_SHADER_COUNT];
    ID3D11InputLayout*       InputLayouts[VERT_SHADER_COUNT];
    ID3D11PixelShader*       PixelShaders[PIXEL_SHADER_COUNT];
    ID3D11Buffer*            UniformBuffers[UNIFORM_KIND_COUNT];
    Arena*                   FlushBufferMemPool;
    DX11FlushBuffer*         HeadFlushBuffer;
    DX11FlushBuffer*         TailFlushBuffer;
};

// @runtime____________________________________________________________________
global DX11State* DX11_STATE = NULL;

global readonly DX11Window DX11_EMPTY_WINDOW_VALUE = {
    (DX11Window*) &DX11_EMPTY_WINDOW_VALUE
};

global readonly DX11Tex2D DX11_EMPTY_TEX2D_VALUE = {
    (DX11Tex2D*) &DX11_EMPTY_TEX2D_VALUE
};

global readonly DX11Buffer DX11_EMPTY_BUFFER_VALUE = {
    (DX11Buffer*) &DX11_EMPTY_BUFFER_VALUE
};

// @functions__________________________________________________________________
DX11Window* DX11WindowFromRendHandle(RendHandle Handle);
DX11Tex2D* DX11Tex2DFromRendHandle(RendHandle Handle);
DX11Buffer* DX11BufferFromRendHandle(RendHandle Handle);
RendHandle RendHandleFromDX11Window(DX11Window* Window);
RendHandle RendHandleFromDX11Tex2D(DX11Tex2D* Texture);
RendHandle RendHandleFromDX11Buffer(DX11Buffer* Buffer);
ID3D11Buffer* ID3D11BufferFromSize(u64 Size);
void UsageAccessFlagsFromResourceKind(RendResourceKind Kind, OUT D3D11_USAGE* Usage, OUT UINT* CPUAccessFlags);

#endif // __DX11_RENDER_H__
