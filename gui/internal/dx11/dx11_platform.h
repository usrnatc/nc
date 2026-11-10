#if !defined(__DX11_PLATFORM_H__)
#define __DX11_PLATFORM_H__

#include "../win32/win32_platform.h"

struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3DBlob;
struct ID3D11ShaderResourceView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct IDXGISwapChain1;
struct ID3D11Device1;
struct ID3D11DeviceContext1;
struct IDXGIFactory2;
struct ID3D11RasterizerState1;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11DepthStencilState;
struct ID3D11InputLayout;
struct IDXGIAdapter;
struct ID3D11InfoQueue;
struct IDXGIOutput;
struct IUnknownVtbl;
struct ID3D11VertexShaderVtbl;
struct ID3D11PixelShaderVtbl;
struct ID3DBlobVtbl;
struct ID3D11ShaderResourceViewVtbl;
struct ID3D11DepthStencilViewVtbl;
struct ID3D11BufferVtbl;
struct IDXGISwapChain1Vtbl;
struct ID3D11Device1Vtbl;
struct ID3D11DeviceContext1Vtbl;
struct IDXGIFactory2Vtbl;
struct ID3D11RasterizerState1Vtbl;
struct ID3D11RasterizerStateVtbl;
struct ID3D11BlendStateVtbl;
struct ID3D11SamplerStateVtbl;
struct ID3D11DepthStencilStateVtbl;
struct ID3D11InputLayoutVtbl;
struct ID3D11DeviceVtbl;
struct ID3D11DeviceContextVtbl;
struct IDXGISwapChainVtbl;
struct ID3D11RenderTargetViewVtbl;
struct ID3D11Texture2DVtbl;
struct IDXGIDevice1Vtbl;
struct IDXGIAdapterVtbl;
struct ID3D11InfoQueueVtbl;
struct IDXGIOutputVtbl;
struct ID3D11DebugVtbl;
struct IDXGIDebugVtbl;

// @defines____________________________________________________________________
#define STDMETHODCALLTYPE            __stdcall
#define THIS_                        void* self,
#define THIS                         void* self
#define PURE                         = 0
#define	D3D11_SDK_VERSION            7
#define D3D11_APPEND_ALIGNED_ELEMENT 0xFFFFFFFF

#define DXGI_MWA_NO_WINDOW_CHANGES 0x1
#define DXGI_MWA_NO_ALT_ENTER      0x2
#define DXGI_MWA_NO_PRINT_SCREEN   0x4
#define DXGI_MWA_VALID             0x7

#define D3D11_DEFAULT_STENCIL_READ_MASK 0xFF
#define D3D11_DEFAULT_STENCIL_WRITE_MASK 0xFF

typedef RECT D3D11_RECT;

typedef enum _D3D_FEATURE_LEVEL : i32 {
  D3D_FEATURE_LEVEL_1_0_GENERIC,
  D3D_FEATURE_LEVEL_1_0_CORE                          = 0x1000,
  D3D_FEATURE_LEVEL_9_1                               = 0x9100,
  D3D_FEATURE_LEVEL_9_2                               = 0x9200,
  D3D_FEATURE_LEVEL_9_3                               = 0x9300,
  D3D_FEATURE_LEVEL_10_0                              = 0xA000,
  D3D_FEATURE_LEVEL_10_1                              = 0xA100,
  D3D_FEATURE_LEVEL_11_0                              = 0xB000,
  D3D_FEATURE_LEVEL_11_1                              = 0xB100,
  D3D_FEATURE_LEVEL_12_0                              = 0xC000,
  D3D_FEATURE_LEVEL_12_1                              = 0xC100,
  D3D_FEATURE_LEVEL_12_2                              = 0xC200
} D3D_FEATURE_LEVEL;
STATIC_ASSERT(sizeof(D3D_FEATURE_LEVEL) == 4, D3DFeatureLevelSizeCheck);

typedef enum _D3D11_RESOURCE_MISC_FLAG : i32 {
  D3D11_RESOURCE_MISC_GENERATE_MIPS                   = 0x0001L,
  D3D11_RESOURCE_MISC_SHARED                          = 0x0002L,
  D3D11_RESOURCE_MISC_TEXTURECUBE                     = 0x0004L,
  D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS               = 0x0010L,
  D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS          = 0x0020L,
  D3D11_RESOURCE_MISC_BUFFER_STRUCTURED               = 0x0040L,
  D3D11_RESOURCE_MISC_RESOURCE_CLAMP                  = 0x0080L,
  D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX               = 0x0100L,
  D3D11_RESOURCE_MISC_GDI_COMPATIBLE                  = 0x0200L,
  D3D11_RESOURCE_MISC_SHARED_NTHANDLE                 = 0x0800L,
  D3D11_RESOURCE_MISC_RESTRICTED_CONTENT              = 0x1000L,
  D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE        = 0x2000L,
  D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER = 0x4000L,
  D3D11_RESOURCE_MISC_GUARDED                         = 0x8000L,
  D3D11_RESOURCE_MISC_TILE_POOL                       = 0x20000L,
  D3D11_RESOURCE_MISC_TILED                           = 0x40000L,
  D3D11_RESOURCE_MISC_HW_PROTECTED                    = 0x80000L,
  D3D11_RESOURCE_MISC_SHARED_DISPLAYABLE,
  D3D11_RESOURCE_MISC_SHARED_EXCLUSIVE_WRITER,
  D3D11_RESOURCE_MISC_NO_SHADER_ACCESS
} D3D11_RESOURCE_MISC_FLAG;
STATIC_ASSERT(sizeof(D3D11_RESOURCE_MISC_FLAG) == 4, D3D11ResourceMiscFlagSizeCheck);

typedef enum _D3D_DRIVER_TYPE : i32 {
    D3D_DRIVER_TYPE_UNKNOWN = 0,
    D3D_DRIVER_TYPE_HARDWARE,
    D3D_DRIVER_TYPE_REFERENCE,
    D3D_DRIVER_TYPE_NULL,
    D3D_DRIVER_TYPE_SOFTWARE,
    D3D_DRIVER_TYPE_WARP
} D3D_DRIVER_TYPE;
STATIC_ASSERT(sizeof(D3D_DRIVER_TYPE) == 4, D3DDriverTypeSizeCheck);

typedef enum _DXGI_FORMAT : i32 {
    DXGI_FORMAT_UNKNOWN                                 = 0,
    DXGI_FORMAT_R32G32B32A32_TYPELESS                   = 1,
    DXGI_FORMAT_R32G32B32A32_FLOAT                      = 2,
    DXGI_FORMAT_R32G32B32A32_UINT                       = 3,
    DXGI_FORMAT_R32G32B32A32_SINT                       = 4,
    DXGI_FORMAT_R32G32B32_TYPELESS                      = 5,
    DXGI_FORMAT_R32G32B32_FLOAT                         = 6,
    DXGI_FORMAT_R32G32B32_UINT                          = 7,
    DXGI_FORMAT_R32G32B32_SINT                          = 8,
    DXGI_FORMAT_R16G16B16A16_TYPELESS                   = 9,
    DXGI_FORMAT_R16G16B16A16_FLOAT                      = 10,
    DXGI_FORMAT_R16G16B16A16_UNORM                      = 11,
    DXGI_FORMAT_R16G16B16A16_UINT                       = 12,
    DXGI_FORMAT_R16G16B16A16_SNORM                      = 13,
    DXGI_FORMAT_R16G16B16A16_SINT                       = 14,
    DXGI_FORMAT_R32G32_TYPELESS                         = 15,
    DXGI_FORMAT_R32G32_FLOAT                            = 16,
    DXGI_FORMAT_R32G32_UINT                             = 17,
    DXGI_FORMAT_R32G32_SINT                             = 18,
    DXGI_FORMAT_R32G8X24_TYPELESS                       = 19,
    DXGI_FORMAT_D32_FLOAT_S8X24_UINT                    = 20,
    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS                = 21,
    DXGI_FORMAT_X32_TYPELESS_G8X24_UINT                 = 22,
    DXGI_FORMAT_R10G10B10A2_TYPELESS                    = 23,
    DXGI_FORMAT_R10G10B10A2_UNORM                       = 24,
    DXGI_FORMAT_R10G10B10A2_UINT                        = 25,
    DXGI_FORMAT_R11G11B10_FLOAT                         = 26,
    DXGI_FORMAT_R8G8B8A8_TYPELESS                       = 27,
    DXGI_FORMAT_R8G8B8A8_UNORM                          = 28,
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB                     = 29,
    DXGI_FORMAT_R8G8B8A8_UINT                           = 30,
    DXGI_FORMAT_R8G8B8A8_SNORM                          = 31,
    DXGI_FORMAT_R8G8B8A8_SINT                           = 32,
    DXGI_FORMAT_R16G16_TYPELESS                         = 33,
    DXGI_FORMAT_R16G16_FLOAT                            = 34,
    DXGI_FORMAT_R16G16_UNORM                            = 35,
    DXGI_FORMAT_R16G16_UINT                             = 36,
    DXGI_FORMAT_R16G16_SNORM                            = 37,
    DXGI_FORMAT_R16G16_SINT                             = 38,
    DXGI_FORMAT_R32_TYPELESS                            = 39,
    DXGI_FORMAT_D32_FLOAT                               = 40,
    DXGI_FORMAT_R32_FLOAT                               = 41,
    DXGI_FORMAT_R32_UINT                                = 42,
    DXGI_FORMAT_R32_SINT                                = 43,
    DXGI_FORMAT_R24G8_TYPELESS                          = 44,
    DXGI_FORMAT_D24_UNORM_S8_UINT                       = 45,
    DXGI_FORMAT_R24_UNORM_X8_TYPELESS                   = 46,
    DXGI_FORMAT_X24_TYPELESS_G8_UINT                    = 47,
    DXGI_FORMAT_R8G8_TYPELESS                           = 48,
    DXGI_FORMAT_R8G8_UNORM                              = 49,
    DXGI_FORMAT_R8G8_UINT                               = 50,
    DXGI_FORMAT_R8G8_SNORM                              = 51,
    DXGI_FORMAT_R8G8_SINT                               = 52,
    DXGI_FORMAT_R16_TYPELESS                            = 53,
    DXGI_FORMAT_R16_FLOAT                               = 54,
    DXGI_FORMAT_D16_UNORM                               = 55,
    DXGI_FORMAT_R16_UNORM                               = 56,
    DXGI_FORMAT_R16_UINT                                = 57,
    DXGI_FORMAT_R16_SNORM                               = 58,
    DXGI_FORMAT_R16_SINT                                = 59,
    DXGI_FORMAT_R8_TYPELESS                             = 60,
    DXGI_FORMAT_R8_UNORM                                = 61,
    DXGI_FORMAT_R8_UINT                                 = 62,
    DXGI_FORMAT_R8_SNORM                                = 63,
    DXGI_FORMAT_R8_SINT                                 = 64,
    DXGI_FORMAT_A8_UNORM                                = 65,
    DXGI_FORMAT_R1_UNORM                                = 66,
    DXGI_FORMAT_R9G9B9E5_SHAREDEXP                      = 67,
    DXGI_FORMAT_R8G8_B8G8_UNORM                         = 68,
    DXGI_FORMAT_G8R8_G8B8_UNORM                         = 69,
    DXGI_FORMAT_BC1_TYPELESS                            = 70,
    DXGI_FORMAT_BC1_UNORM                               = 71,
    DXGI_FORMAT_BC1_UNORM_SRGB                          = 72,
    DXGI_FORMAT_BC2_TYPELESS                            = 73,
    DXGI_FORMAT_BC2_UNORM                               = 74,
    DXGI_FORMAT_BC2_UNORM_SRGB                          = 75,
    DXGI_FORMAT_BC3_TYPELESS                            = 76,
    DXGI_FORMAT_BC3_UNORM                               = 77,
    DXGI_FORMAT_BC3_UNORM_SRGB                          = 78,
    DXGI_FORMAT_BC4_TYPELESS                            = 79,
    DXGI_FORMAT_BC4_UNORM                               = 80,
    DXGI_FORMAT_BC4_SNORM                               = 81,
    DXGI_FORMAT_BC5_TYPELESS                            = 82,
    DXGI_FORMAT_BC5_UNORM                               = 83,
    DXGI_FORMAT_BC5_SNORM                               = 84,
    DXGI_FORMAT_B5G6R5_UNORM                            = 85,
    DXGI_FORMAT_B5G5R5A1_UNORM                          = 86,
    DXGI_FORMAT_B8G8R8A8_UNORM                          = 87,
    DXGI_FORMAT_B8G8R8X8_UNORM                          = 88,
    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM              = 89,
    DXGI_FORMAT_B8G8R8A8_TYPELESS                       = 90,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB                     = 91,
    DXGI_FORMAT_B8G8R8X8_TYPELESS                       = 92,
    DXGI_FORMAT_B8G8R8X8_UNORM_SRGB                     = 93,
    DXGI_FORMAT_BC6H_TYPELESS                           = 94,
    DXGI_FORMAT_BC6H_UF16                               = 95,
    DXGI_FORMAT_BC6H_SF16                               = 96,
    DXGI_FORMAT_BC7_TYPELESS                            = 97,
    DXGI_FORMAT_BC7_UNORM                               = 98,
    DXGI_FORMAT_BC7_UNORM_SRGB                          = 99,
    DXGI_FORMAT_AYUV                                    = 100,
    DXGI_FORMAT_Y410                                    = 101,
    DXGI_FORMAT_Y416                                    = 102,
    DXGI_FORMAT_NV12                                    = 103,
    DXGI_FORMAT_P010                                    = 104,
    DXGI_FORMAT_P016                                    = 105,
    DXGI_FORMAT_420_OPAQUE                              = 106,
    DXGI_FORMAT_YUY2                                    = 107,
    DXGI_FORMAT_Y210                                    = 108,
    DXGI_FORMAT_Y216                                    = 109,
    DXGI_FORMAT_NV11                                    = 110,
    DXGI_FORMAT_AI44                                    = 111,
    DXGI_FORMAT_IA44                                    = 112,
    DXGI_FORMAT_P8                                      = 113,
    DXGI_FORMAT_A8P8                                    = 114,
    DXGI_FORMAT_B4G4R4A4_UNORM                          = 115,
    DXGI_FORMAT_P208                                    = 130,
    DXGI_FORMAT_V208                                    = 131,
    DXGI_FORMAT_V408                                    = 132,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE         = 189,
    DXGI_FORMAT_SAMPLER_FEEDBACK_MIP_REGION_USED_OPAQUE = 190,
    DXGI_FORMAT_FORCE_UINT                              = I32_MAX
} DXGI_FORMAT;
STATIC_ASSERT(sizeof(DXGI_FORMAT) == 4, DXGIFormatSizeCheck);

typedef enum _DXGI_MODE_SCALING : i32 {
    DXGI_MODE_SCALING_UNSPECIFIED = 0
} DXGI_MODE_SCALING;
STATIC_ASSERT(sizeof(DXGI_MODE_SCALING) == 4, DXGIModeScalingSizeCheck);

typedef enum _DXGI_MODE_SCANLINE_ORDER : i32 {
    DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED = 0
} DXGI_MODE_SCANLINE_ORDER;
STATIC_ASSERT(sizeof(DXGI_MODE_SCANLINE_ORDER) == 4, DXGIModeScanlineOrderSizeCheck);

typedef enum _D3D11_INPUT_CLASSIFICATION : i32 {
  D3D11_INPUT_PER_VERTEX_DATA = 0,
  D3D11_INPUT_PER_INSTANCE_DATA = 1
} D3D11_INPUT_CLASSIFICATION;
STATIC_ASSERT(sizeof(D3D11_INPUT_CLASSIFICATION) == 4, D3D11InputClassificationSizeCheck);

typedef enum _D3D11_USAGE : i32 {
    D3D11_USAGE_DEFAULT   = 0,
    D3D11_USAGE_IMMUTABLE = 1,
    D3D11_USAGE_DYNAMIC   = 2,
    D3D11_USAGE_STAGING   = 3
} D3D11_USAGE;
STATIC_ASSERT(sizeof(D3D11_USAGE) == 4, D3D11UsageSizeCheck);

typedef enum _D3D11_BIND_FLAG : i32 {
    D3D11_BIND_VERTEX_BUFFER   = 0x1,
    D3D11_BIND_INDEX_BUFFER    = 0x2,
    D3D11_BIND_CONSTANT_BUFFER = 0x4,
    D3D11_BIND_SHADER_RESOURCE = 0x8,
    D3D11_BIND_RENDER_TARGET   = 0x20,
    D3D11_BIND_DEPTH_STENCIL   = 0x40
} D3D11_BIND_FLAG;
STATIC_ASSERT(sizeof(D3D11_BIND_FLAG) == 4, D3D11BindFlagSizeCheck);

typedef enum _D3D11_CPU_ACCESS_FLAG : i32 {
    D3D11_CPU_ACCESS_WRITE = 0x10000,
    D3D11_CPU_ACCESS_READ  = 0x20000
} D3D11_CPU_ACCESS_FLAG;
STATIC_ASSERT(sizeof(D3D11_CPU_ACCESS_FLAG) == 4, D3D11CPUAccessFlagSizeCheck);

typedef enum _D3D11_MAP : i32 {
    D3D11_MAP_READ               = 1,
    D3D11_MAP_WRITE              = 2,
    D3D11_MAP_READ_WRITE         = 3,
    D3D11_MAP_WRITE_DISCARD      = 4,
    D3D11_MAP_WRITE_NO_OVERWRITE = 5
} D3D11_MAP;
STATIC_ASSERT(sizeof(D3D11_MAP) == 4, D3D11MapSizeCheck);

typedef enum _D3D11_RESOURCE_DIMENSION : i32 {
    D3D11_RESOURCE_DIMENSION_BUFFER    = 1,
    D3D11_RESOURCE_DIMENSION_TEXTURE2D = 3
} D3D11_RESOURCE_DIMENSION;
STATIC_ASSERT(sizeof(D3D11_RESOURCE_DIMENSION) == 4, D3D11ResourceDimensionSizeCheck);

typedef enum _D3D11_PRIMITIVE_TOPOLOGY : i32 {
    D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED                  = 0,
    D3D11_PRIMITIVE_TOPOLOGY_POINTLIST                  = 1,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST                   = 2,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP                  = 3,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST               = 4,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP              = 5,
    D3D11_PRIMITIVE_TOPOLOGY_LINELIST_ADJ               = 10,
    D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ              = 11,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ           = 12,
    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ          = 13,
    D3D11_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST  = 33,
    D3D11_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST  = 34,
    D3D11_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST  = 35,
    D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST  = 36,
    D3D11_PRIMITIVE_TOPOLOGY_5_CONTROL_POINT_PATCHLIST  = 37,
    D3D11_PRIMITIVE_TOPOLOGY_6_CONTROL_POINT_PATCHLIST  = 38,
    D3D11_PRIMITIVE_TOPOLOGY_7_CONTROL_POINT_PATCHLIST  = 39,
    D3D11_PRIMITIVE_TOPOLOGY_8_CONTROL_POINT_PATCHLIST  = 40,
    D3D11_PRIMITIVE_TOPOLOGY_9_CONTROL_POINT_PATCHLIST  = 41,
    D3D11_PRIMITIVE_TOPOLOGY_10_CONTROL_POINT_PATCHLIST = 42,
    D3D11_PRIMITIVE_TOPOLOGY_11_CONTROL_POINT_PATCHLIST = 43,
    D3D11_PRIMITIVE_TOPOLOGY_12_CONTROL_POINT_PATCHLIST = 44,
    D3D11_PRIMITIVE_TOPOLOGY_13_CONTROL_POINT_PATCHLIST = 45,
    D3D11_PRIMITIVE_TOPOLOGY_14_CONTROL_POINT_PATCHLIST = 46,
    D3D11_PRIMITIVE_TOPOLOGY_15_CONTROL_POINT_PATCHLIST = 47,
    D3D11_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST = 48,
    D3D11_PRIMITIVE_TOPOLOGY_17_CONTROL_POINT_PATCHLIST = 49,
    D3D11_PRIMITIVE_TOPOLOGY_18_CONTROL_POINT_PATCHLIST = 50,
    D3D11_PRIMITIVE_TOPOLOGY_19_CONTROL_POINT_PATCHLIST = 51,
    D3D11_PRIMITIVE_TOPOLOGY_20_CONTROL_POINT_PATCHLIST = 52,
    D3D11_PRIMITIVE_TOPOLOGY_21_CONTROL_POINT_PATCHLIST = 53,
    D3D11_PRIMITIVE_TOPOLOGY_22_CONTROL_POINT_PATCHLIST = 54,
    D3D11_PRIMITIVE_TOPOLOGY_23_CONTROL_POINT_PATCHLIST = 55,
    D3D11_PRIMITIVE_TOPOLOGY_24_CONTROL_POINT_PATCHLIST = 56,
    D3D11_PRIMITIVE_TOPOLOGY_25_CONTROL_POINT_PATCHLIST = 57,
    D3D11_PRIMITIVE_TOPOLOGY_26_CONTROL_POINT_PATCHLIST = 58,
    D3D11_PRIMITIVE_TOPOLOGY_27_CONTROL_POINT_PATCHLIST = 59,
    D3D11_PRIMITIVE_TOPOLOGY_28_CONTROL_POINT_PATCHLIST = 60,
    D3D11_PRIMITIVE_TOPOLOGY_29_CONTROL_POINT_PATCHLIST = 61,
    D3D11_PRIMITIVE_TOPOLOGY_30_CONTROL_POINT_PATCHLIST = 62,
    D3D11_PRIMITIVE_TOPOLOGY_31_CONTROL_POINT_PATCHLIST = 63,
    D3D11_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST = 64
} D3D11_PRIMITIVE_TOPOLOGY;
STATIC_ASSERT(sizeof(D3D11_PRIMITIVE_TOPOLOGY) == 4, D3DPrimitiveTopologySizeCheck);

typedef enum _D3D11_CULL_MODE : i32 {
    D3D11_CULL_NONE  = 1,
    D3D11_CULL_FRONT = 2,
    D3D11_CULL_BACK  = 3
} D3D11_CULL_MODE;
STATIC_ASSERT(sizeof(D3D11_CULL_MODE) == 4, D3D11CullModeSizeCheck);

typedef enum _D3D11_FILL_MODE : i32 {
    D3D11_FILL_WIREFRAME = 2,
    D3D11_FILL_SOLID     = 3
} D3D11_FILL_MODE;
STATIC_ASSERT(sizeof(D3D11_FILL_MODE) == 4, D3D11FillModeSizeCheck);

typedef enum _D3D11_COMPARISON_FUNC : i32 {
    D3D11_COMPARISON_NEVER         = 1,
    D3D11_COMPARISON_LESS          = 2,
    D3D11_COMPARISON_EQUAL         = 3,
    D3D11_COMPARISON_LESS_EQUAL    = 4,
    D3D11_COMPARISON_GREATER       = 5,
    D3D11_COMPARISON_NOT_EQUAL     = 6,
    D3D11_COMPARISON_GREATER_EQUAL = 7,
    D3D11_COMPARISON_ALWAYS        = 8
} D3D11_COMPARISON_FUNC;
STATIC_ASSERT(sizeof(D3D11_COMPARISON_FUNC) == 4, D3D11ComparisonFuncSizeCheck);

typedef enum _D3D11_FILTER : i32 {
    D3D11_FILTER_MIN_MAG_MIP_POINT                          = 0,
    D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR                   = 0x1,
    D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT             = 0x4,
    D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR                   = 0x5,
    D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT                   = 0x10,
    D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR            = 0x11,
    D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT                   = 0x14,
    D3D11_FILTER_MIN_MAG_MIP_LINEAR                         = 0x15,
    D3D11_FILTER_ANISOTROPIC                                = 0x55,
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT               = 0x80,
    D3D11_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR        = 0x81,
    D3D11_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT  = 0x84,
    D3D11_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR        = 0x85,
    D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT        = 0x90,
    D3D11_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR = 0x91,
    D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT        = 0x94,
    D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR              = 0x95,
    D3D11_FILTER_COMPARISON_ANISOTROPIC                     = 0xD5,
    D3D11_FILTER_MINIMUM_MIN_MAG_MIP_POINT                  = 0x100,
    D3D11_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR           = 0x101,
    D3D11_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     = 0x104,
    D3D11_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR           = 0x105,
    D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT           = 0x110,
    D3D11_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    = 0x111,
    D3D11_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT           = 0x114,
    D3D11_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR                 = 0x115,
    D3D11_FILTER_MINIMUM_ANISOTROPIC                        = 0x155,
    D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_POINT                  = 0x180,
    D3D11_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR           = 0x181,
    D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT     = 0x184,
    D3D11_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR           = 0x185,
    D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT           = 0x190,
    D3D11_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR    = 0x191,
    D3D11_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT           = 0x194,
    D3D11_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR                 = 0x195,
    D3D11_FILTER_MAXIMUM_ANISOTROPIC                        = 0x1d5
} D3D11_FILTER;
STATIC_ASSERT(sizeof(D3D11_FILTER) == 4, D3D11FilterSizeCheck);

typedef enum _D3D11_TEXTURE_ADDRESS_MODE : i32 {
    D3D11_TEXTURE_ADDRESS_WRAP        = 1,
    D3D11_TEXTURE_ADDRESS_MIRROR      = 2,
    D3D11_TEXTURE_ADDRESS_CLAMP       = 3,
    D3D11_TEXTURE_ADDRESS_BORDER      = 4,
    D3D11_TEXTURE_ADDRESS_MIRROR_ONCE = 5
} D3D11_TEXTURE_ADDRESS_MODE;
STATIC_ASSERT(sizeof(D3D11_TEXTURE_ADDRESS_MODE) == 4, D3D11TextureAddressModeSizeCheck);

typedef enum _D3D11_BLEND : i32 {
    D3D11_BLEND_ZERO             = 1,
    D3D11_BLEND_ONE              = 2,
    D3D11_BLEND_SRC_COLOR        = 3,
    D3D11_BLEND_INV_SRC_COLOR    = 4,
    D3D11_BLEND_SRC_ALPHA        = 5,
    D3D11_BLEND_INV_SRC_ALPHA    = 6,
    D3D11_BLEND_DEST_ALPHA       = 7,
    D3D11_BLEND_INV_DEST_ALPHA   = 8,
    D3D11_BLEND_DEST_COLOR       = 9,
    D3D11_BLEND_INV_DEST_COLOR   = 10,
    D3D11_BLEND_SRC_ALPHA_SAT    = 11,
    D3D11_BLEND_BLEND_FACTOR     = 14,
    D3D11_BLEND_INV_BLEND_FACTOR = 15,
    D3D11_BLEND_SRC1_COLOR       = 16,
    D3D11_BLEND_INV_SRC1_COLOR   = 17,
    D3D11_BLEND_SRC1_ALPHA       = 18,
    D3D11_BLEND_INV_SRC1_ALPHA   = 19
} D3D11_BLEND;
STATIC_ASSERT(sizeof(D3D11_BLEND) == 4, D3D11BlendSizeCheck);

typedef enum _D3D11_BLEND_OP : i32 {
    D3D11_BLEND_OP_ADD          = 1,
    D3D11_BLEND_OP_SUBTRACT     = 2,
    D3D11_BLEND_OP_REV_SUBTRACT = 3,
    D3D11_BLEND_OP_MIN          = 4,
    D3D11_BLEND_OP_MAX          = 5
} D3D11_BLEND_OP;
STATIC_ASSERT(sizeof(D3D11_BLEND_OP) == 4, D3D11BlendOpSizeCheck);

typedef enum _D3D11_DEPTH_WRITE_MASK : i32 {
    D3D11_DEPTH_WRITE_MASK_ZERO = 0,
    D3D11_DEPTH_WRITE_MASK_ALL  = 1
} D3D11_DEPTH_WRITE_MASK;
STATIC_ASSERT(sizeof(D3D11_DEPTH_WRITE_MASK) == 4, D3D11DepthWriteMaskSizeCheck);

typedef enum _D3D11_STENCIL_OP : i32 {
  D3D11_STENCIL_OP_KEEP     = 1,
  D3D11_STENCIL_OP_ZERO     = 2,
  D3D11_STENCIL_OP_REPLACE  = 3,
  D3D11_STENCIL_OP_INCR_SAT = 4,
  D3D11_STENCIL_OP_DECR_SAT = 5,
  D3D11_STENCIL_OP_INVERT   = 6,
  D3D11_STENCIL_OP_INCR     = 7,
  D3D11_STENCIL_OP_DECR     = 8
} D3D11_STENCIL_OP;
STATIC_ASSERT(sizeof(D3D11_STENCIL_OP) == 4, D3D11StencilOpSizeCheck);

typedef enum _DXGI_USAGE : i32 {
    DXGI_USAGE_RENDER_TARGET_OUTPUT = 32,
} DXGI_USAGE;
STATIC_ASSERT(sizeof(DXGI_USAGE) == 4, DXGIUsageSizeCheck);

typedef enum _D3D11_CREATE_DEVICE_FLAG : i32 {
    D3D11_CREATE_DEVICE_SINGLETHREADED                                = 0x001,
    D3D11_CREATE_DEVICE_DEBUG                                         = 0x002,
    D3D11_CREATE_DEVICE_SWITCH_TO_REF                                 = 0x004,
    D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS      = 0x008,
    D3D11_CREATE_DEVICE_BGRA_SUPPORT                                  = 0x020,
    D3D11_CREATE_DEVICE_DEBUGGABLE                                    = 0x040,
    D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY = 0x080,
    D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT                           = 0x100,
    D3D11_CREATE_DEVICE_VIDEO_SUPPORT                                 = 0x800
} D3D11_CREATE_DEVICE_FLAG;
STATIC_ASSERT(sizeof(D3D11_CREATE_DEVICE_FLAG) == 4, D3D11CreateDeviceFlagSizeCheck);

typedef enum _D3D11_COLOR_WRITE_ENABLE : i32 {
    D3D11_COLOR_WRITE_ENABLE_RED   = (1 << 0),
    D3D11_COLOR_WRITE_ENABLE_GREEN = (1 << 1),
    D3D11_COLOR_WRITE_ENABLE_BLUE  = (1 << 2),
    D3D11_COLOR_WRITE_ENABLE_ALPHA = (1 << 3),
    D3D11_COLOR_WRITE_ENABLE_ALL   = (D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_ALPHA)
} D3D11_COLOR_WRITE_ENABLE;
STATIC_ASSERT(sizeof(D3D11_COLOR_WRITE_ENABLE) == 4, D3D11ColorWriteEnableSizeCheck);

typedef enum _D3D11_SRV_DIMENSION : i32 { 
    D3D11_SRV_DIMENSION_UNKNOWN          = 0,
    D3D11_SRV_DIMENSION_BUFFER           = 1,
    D3D11_SRV_DIMENSION_TEXTURE1D        = 2,
    D3D11_SRV_DIMENSION_TEXTURE1DARRAY   = 3,
    D3D11_SRV_DIMENSION_TEXTURE2D        = 4,
    D3D11_SRV_DIMENSION_TEXTURE2DARRAY   = 5,
    D3D11_SRV_DIMENSION_TEXTURE2DMS      = 6,
    D3D11_SRV_DIMENSION_TEXTURE2DMSARRAY = 7,
    D3D11_SRV_DIMENSION_TEXTURE3D        = 8,
    D3D11_SRV_DIMENSION_TEXTURECUBE      = 9,
    D3D11_SRV_DIMENSION_TEXTURECUBEARRAY = 10,
    D3D11_SRV_DIMENSION_BUFFEREX         = 11
} D3D11_SRV_DIMENSION;
STATIC_ASSERT(sizeof(D3D11_SRV_DIMENSION) == 4, D3D11SRVDimensionSizeCheck);

typedef enum _D3D11_DSV_DIMENSION : i32 {
    D3D11_DSV_DIMENSION_UNKNOWN          = 0,
    D3D11_DSV_DIMENSION_TEXTURE1D        = 1,
    D3D11_DSV_DIMENSION_TEXTURE1DARRAY   = 2,
    D3D11_DSV_DIMENSION_TEXTURE2D        = 3,
    D3D11_DSV_DIMENSION_TEXTURE2DARRAY   = 4,
    D3D11_DSV_DIMENSION_TEXTURE2DMS      = 5,
    D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY = 6
} D3D11_DSV_DIMENSION;
STATIC_ASSERT(sizeof(D3D11_DSV_DIMENSION) == 4, D3D11DSVDimensionSizeCheck);

typedef enum _D3D11_MESSAGE_SEVERITY : i32 {
    D3D11_MESSAGE_SEVERITY_CORRUPTION = 0,
    D3D11_MESSAGE_SEVERITY_ERROR,
    D3D11_MESSAGE_SEVERITY_WARNING,
    D3D11_MESSAGE_SEVERITY_INFO,
    D3D11_MESSAGE_SEVERITY_MESSAGE
} D3D11_MESSAGE_SEVERITY;
STATIC_ASSERT(sizeof(D3D11_MESSAGE_SEVERITY) == 4, D3D11MessageSeveritySizeCheck);

typedef enum _DXGI_SCALING : i32 {
    DXGI_SCALING_STRETCH              = 0,
    DXGI_SCALING_NONE                 = 1,
    DXGI_SCALING_ASPECT_RATIO_STRETCH = 2
} DXGI_SCALING;
STATIC_ASSERT(sizeof(DXGI_SCALING) == 4, DXGIScalingSizeCheck);

typedef enum _DXGI_SWAP_EFFECT : i32 {
    DXGI_SWAP_EFFECT_DISCARD         = 0,
    DXGI_SWAP_EFFECT_SEQUENTIAL      = 1,
    DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL = 3,
    DXGI_SWAP_EFFECT_FLIP_DISCARD    = 4
} DXGI_SWAP_EFFECT;
STATIC_ASSERT(sizeof(DXGI_SWAP_EFFECT) == 4, DXGISwapEffectSizeCheck);

typedef enum _DXGI_SWAP_CHAIN_FLAG : i32 {
  DXGI_SWAP_CHAIN_FLAG_NONPREROTATED                          = 1,
  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH                      = 2,
  DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE                         = 4,
  DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT                     = 8,
  DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER        = 16,
  DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY                           = 32,
  DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT          = 64,
  DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER                       = 128,
  DXGI_SWAP_CHAIN_FLAG_FULLSCREEN_VIDEO                       = 256,
  DXGI_SWAP_CHAIN_FLAG_YUV_VIDEO                              = 512,
  DXGI_SWAP_CHAIN_FLAG_HW_PROTECTED                           = 1024,
  DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING                          = 2048,
  DXGI_SWAP_CHAIN_FLAG_RESTRICTED_TO_ALL_HOLOGRAPHIC_DISPLAYS = 4096
} DXGI_SWAP_CHAIN_FLAG;
STATIC_ASSERT(sizeof(DXGI_SWAP_CHAIN_FLAG) == 4, DXGISwapChainFlagSizeCheck);

typedef enum _DXGI_ALPHA_MODE : i32 {
    DXGI_ALPHA_MODE_UNSPECIFIED   = 0,
    DXGI_ALPHA_MODE_PREMULTIPLIED = 1,
    DXGI_ALPHA_MODE_STRAIGHT      = 2,
    DXGI_ALPHA_MODE_IGNORE        = 3,
    DXGI_ALPHA_MODE_FORCE_DWORD   = I32_MAX
} DXGI_ALPHA_MODE;
STATIC_ASSERT(sizeof(DXGI_ALPHA_MODE) == 4, DXGIAlphaModeSizeCheck);

typedef enum _D3D11_RTV_DIMENSION : i32 {
    D3D11_RTV_DIMENSION_UNKNOWN          = 0,
    D3D11_RTV_DIMENSION_BUFFER           = 1,
    D3D11_RTV_DIMENSION_TEXTURE1D        = 2,
    D3D11_RTV_DIMENSION_TEXTURE1DARRAY   = 3,
    D3D11_RTV_DIMENSION_TEXTURE2D        = 4,
    D3D11_RTV_DIMENSION_TEXTURE2DARRAY   = 5,
    D3D11_RTV_DIMENSION_TEXTURE2DMS      = 6,
    D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY = 7,
    D3D11_RTV_DIMENSION_TEXTURE3D        = 8
} D3D11_RTV_DIMENSION;
STATIC_ASSERT(sizeof(D3D11_RTV_DIMENSION) == 4, D3D11RTVDimensionSizeCheck);

typedef enum _D3D11_CLEAR_FLAG : i32 {
    D3D11_CLEAR_DEPTH   = 0x1L,
    D3D11_CLEAR_STENCIL = 0x2L
} D3D11_CLEAR_FLAG;
STATIC_ASSERT(sizeof(D3D11_CLEAR_FLAG) == 4, D3D11ClearFlagSizeCheck);

typedef enum _DXGI_DEBUG_RLO_FLAGS {
    DXGI_DEBUG_RLO_SUMMARY         = 0x1,
    DXGI_DEBUG_RLO_DETAIL          = 0x2,
    DXGI_DEBUG_RLO_IGNORE_INTERNAL = 0x4,
    DXGI_DEBUG_RLO_ALL             = 0x7
} DXGI_DEBUG_RLO_FLAGS;

typedef enum _D3D11_RLDO_FLAGS {
    D3D11_RLDO_SUMMARY         = 0x1,
    D3D11_RLDO_DETAIL          = 0x2,
    D3D11_RLDO_IGNORE_INTERNAL = 0x4
} D3D11_RLDO_FLAGS;

#define D3DCOMPILE_DEBUG                                (1 << 0)
#define D3DCOMPILE_SKIP_VALIDATION                      (1 << 1)
#define D3DCOMPILE_SKIP_OPTIMIZATION                    (1 << 2)
#define D3DCOMPILE_PACK_MATRIX_ROW_MAJOR                (1 << 3)
#define D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR             (1 << 4)
#define D3DCOMPILE_PARTIAL_PRECISION                    (1 << 5)
#define D3DCOMPILE_FORCE_VS_SOFTWARE_NO_OPT             (1 << 6)
#define D3DCOMPILE_FORCE_PS_SOFTWARE_NO_OPT             (1 << 7)
#define D3DCOMPILE_NO_PRESHADER                         (1 << 8)
#define D3DCOMPILE_AVOID_FLOW_CONTROL                   (1 << 9)
#define D3DCOMPILE_PREFER_FLOW_CONTROL                  (1 << 10)
#define D3DCOMPILE_ENABLE_STRICTNESS                    (1 << 11)
#define D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY       (1 << 12)
#define D3DCOMPILE_IEEE_STRICTNESS                      (1 << 13)
#define D3DCOMPILE_OPTIMIZATION_LEVEL0                  (1 << 14)
#define D3DCOMPILE_OPTIMIZATION_LEVEL1                  0
#define D3DCOMPILE_OPTIMIZATION_LEVEL2                  ((1 << 14) | (1 << 15))
#define D3DCOMPILE_OPTIMIZATION_LEVEL3                  (1 << 15)
#define D3DCOMPILE_RESERVED16                           (1 << 16)
#define D3DCOMPILE_RESERVED17                           (1 << 17)
#define D3DCOMPILE_WARNINGS_ARE_ERRORS                  (1 << 18)
#define D3DCOMPILE_RESOURCES_MAY_ALIAS                  (1 << 19)
#define D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES   (1 << 20)
#define D3DCOMPILE_ALL_RESOURCES_BOUND                  (1 << 21)
#define D3DCOMPILE_DEBUG_NAME_FOR_SOURCE                (1 << 22)
#define D3DCOMPILE_DEBUG_NAME_FOR_BINARY                (1 << 23)

// @types______________________________________________________________________
typedef GUID IID;
typedef GUID REFIID;

typedef struct D3D11_BOX {
    UINT left;
    UINT top;
    UINT front;
    UINT right;
    UINT bottom;
    UINT back;
} D3D11_BOX;

typedef struct D3D11_BUFFER_RTV {
    union {
        UINT FirstElement;
        UINT ElementOffset;
    };

    union {
        UINT NumElements;
        UINT ElementWidth;
    };
} D3D11_BUFFER_RTV;

typedef struct D3D11_TEX1D_RTV {
    UINT MipSlice;
} D3D11_TEX1D_RTV;

typedef struct D3D11_TEX1D_ARRAY_RTV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX1D_ARRAY_RTV;

typedef struct D3D11_TEX2D_RTV {
    UINT MipSlice;
} D3D11_TEX2D_RTV;

typedef struct D3D11_TEX2D_ARRAY_RTV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_RTV;

typedef struct D3D11_TEX2DMS_RTV {
    UINT UnusedField_NothingToDefine;
} D3D11_TEX2DMS_RTV;

typedef struct D3D11_TEX2DMS_ARRAY_RTV {
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2DMS_ARRAY_RTV;

typedef struct D3D11_TEX3D_RTV {
    UINT MipSlice;
    UINT FirstWSlice;
    UINT WSize;
} D3D11_TEX3D_RTV;

typedef struct DXGI_RATIONAL {
    UINT Numerator;
    UINT Denominator;
} DXGI_RATIONAL;

typedef struct DXGI_MODE_DESC {
    UINT Width;
    UINT Height;
    DXGI_RATIONAL RefreshRate;
    DXGI_FORMAT Format;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING Scaling;
} DXGI_MODE_DESC;

typedef struct DXGI_SAMPLE_DESC {
    UINT Count;
    UINT Quality;
} DXGI_SAMPLE_DESC;

typedef struct DXGI_SWAP_CHAIN_DESC {
    DXGI_MODE_DESC BufferDesc;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE BufferUsage;
    UINT BufferCount;
    HWND OutputWindow;
    BOOL Windowed;
    UINT SwapEffect;
    UINT Flags;
} DXGI_SWAP_CHAIN_DESC;

typedef struct DXGI_SWAP_CHAIN_DESC1 {
    UINT             Width;
    UINT             Height;
    DXGI_FORMAT      Format;
    BOOL             Stereo;
    DXGI_SAMPLE_DESC SampleDesc;
    DXGI_USAGE       BufferUsage;
    UINT             BufferCount;
    DXGI_SCALING     Scaling;
    DXGI_SWAP_EFFECT SwapEffect;
    DXGI_ALPHA_MODE  AlphaMode;
    UINT             Flags;
} DXGI_SWAP_CHAIN_DESC1;

typedef struct D3D11_VIEWPORT {
    FLOAT TopLeftX;
    FLOAT TopLeftY;
    FLOAT Width;
    FLOAT Height;
    FLOAT MinDepth;
    FLOAT MaxDepth;
} D3D11_VIEWPORT;

typedef struct D3D11_INPUT_ELEMENT_DESC {
  LPCSTR                     SemanticName;
  UINT                       SemanticIndex;
  DXGI_FORMAT                Format;
  UINT                       InputSlot;
  UINT                       AlignedByteOffset;
  D3D11_INPUT_CLASSIFICATION InputSlotClass;
  UINT                       InstanceDataStepRate;
} D3D11_INPUT_ELEMENT_DESC;

typedef struct D3D11_BUFFER_DESC {
    UINT ByteWidth;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
    UINT StructureByteStride;
} D3D11_BUFFER_DESC;

typedef struct D3D11_SUBRESOURCE_DATA {
    const void* pSysMem;
    UINT SysMemPitch;
    UINT SysMemSlicePitch;
} D3D11_SUBRESOURCE_DATA;

typedef struct D3D11_MAPPED_SUBRESOURCE {
    void* pData;
    UINT RowPitch;
    UINT DepthPitch;
} D3D11_MAPPED_SUBRESOURCE;

typedef struct D3D11_TEXTURE2D_DESC {
    UINT Width;
    UINT Height;
    UINT MipLevels;
    UINT ArraySize;
    DXGI_FORMAT Format;
    DXGI_SAMPLE_DESC SampleDesc;
    D3D11_USAGE Usage;
    UINT BindFlags;
    UINT CPUAccessFlags;
    UINT MiscFlags;
} D3D11_TEXTURE2D_DESC;

typedef struct D3D11_BUFFER_SRV {
    union {
        UINT FirstElement;
        UINT ElementOffset;
    };

    union {
        UINT NumElements;
        UINT ElementWidth;
    };
} D3D11_BUFFER_SRV;

typedef struct D3D11_TEX1D_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEX1D_SRV;

typedef struct D3D11_TEX1D_ARRAY_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX1D_ARRAY_SRV;

typedef struct D3D11_TEX2D_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEX2D_SRV;

typedef struct D3D11_TEX2D_ARRAY_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_SRV;

typedef struct D3D11_TEX2DMS_SRV {
    UINT UnusedField_NothingToDefine;
} D3D11_TEX2DMS_SRV;

typedef struct D3D11_TEX2DMS_ARRAY_SRV {
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2DMS_ARRAY_SRV;

typedef struct D3D11_TEX3D_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEX3D_SRV;

typedef struct D3D11_TEXCUBE_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
} D3D11_TEXCUBE_SRV;

typedef struct D3D11_TEXCUBE_ARRAY_SRV {
    UINT MostDetailedMip;
    UINT MipLevels;
    UINT First2DArrayFace;
    UINT NumCubes;
} D3D11_TEXCUBE_ARRAY_SRV;

typedef struct D3D11_BUFFEREX_SRV {
    UINT FirstElement;
    UINT NumElements;
    UINT Flags;
} D3D11_BUFFEREX_SRV;

typedef struct D3D11_SHADER_RESOURCE_VIEW_DESC {
    DXGI_FORMAT         Format;
    D3D11_SRV_DIMENSION ViewDimension;

    union {
        D3D11_BUFFER_SRV        Buffer;
        D3D11_TEX1D_SRV         Texture1D;
        D3D11_TEX1D_ARRAY_SRV   Texture1DArray;
        D3D11_TEX2D_SRV         Texture2D;
        D3D11_TEX2D_ARRAY_SRV   Texture2DArray;
        D3D11_TEX2DMS_SRV       Texture2DMS;
        D3D11_TEX2DMS_ARRAY_SRV Texture2DMSArray;
        D3D11_TEX3D_SRV         Texture3D;
        D3D11_TEXCUBE_SRV       TextureCube;
        D3D11_TEXCUBE_ARRAY_SRV TextureCubeArray;
        D3D11_BUFFEREX_SRV      BufferEx;
    };
} D3D11_SHADER_RESOURCE_VIEW_DESC;

typedef struct D3D11_TEX1D_DSV {
    UINT MipSlice;
} D3D11_TEX1D_DSV;

typedef struct D3D11_TEX1D_ARRAY_DSV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX1D_ARRAY_DSV;

typedef struct D3D11_TEX2D_DSV {
    UINT MipSlice;
} D3D11_TEX2D_DSV;

typedef struct D3D11_TEX2D_ARRAY_DSV {
    UINT MipSlice;
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2D_ARRAY_DSV;

typedef struct D3D11_TEX2DMS_DSV {
    UINT UnusedField_NothingToDefine;
} D3D11_TEX2DMS_DSV;

typedef struct D3D11_TEX2DMS_ARRAY_DSV {
    UINT FirstArraySlice;
    UINT ArraySize;
} D3D11_TEX2DMS_ARRAY_DSV;

typedef struct D3D11_DEPTH_STENCIL_VIEW_DESC {
    DXGI_FORMAT         Format;
    D3D11_DSV_DIMENSION ViewDimension;
    UINT                Flags;

    union {
        D3D11_TEX1D_DSV         Texture1D;
        D3D11_TEX1D_ARRAY_DSV   Texture1DArray;
        D3D11_TEX2D_DSV         Texture2D;
        D3D11_TEX2D_ARRAY_DSV   Texture2DArray;
        D3D11_TEX2DMS_DSV       Texture2DMS;
        D3D11_TEX2DMS_ARRAY_DSV Texture2DMSArray;
    };
} D3D11_DEPTH_STENCIL_VIEW_DESC;

typedef struct D3D11_RASTERIZER_DESC {
    D3D11_FILL_MODE FillMode;
    D3D11_CULL_MODE CullMode;
    BOOL            FrontCounterClockwise;
    INT             DepthBias;
    FLOAT           DepthBiasClamp;
    FLOAT           SlopeScaledDepthBias;
    BOOL            DepthClipEnable;
    BOOL            ScissorEnable;
    BOOL            MultisampleEnable;
    BOOL            AntialiasedLineEnable;
} D3D11_RASTERIZER_DESC;

typedef struct D3D11_RASTERIZER_DESC1 {
    D3D11_FILL_MODE FillMode;
    D3D11_CULL_MODE CullMode;
    BOOL            FrontCounterClockwise;
    INT             DepthBias;
    FLOAT           DepthBiasClamp;
    FLOAT           SlopeScaledDepthBias;
    BOOL            DepthClipEnable;
    BOOL            ScissorEnable;
    BOOL            MultisampleEnable;
    BOOL            AntialiasedLineEnable;
    UINT            ForcedSampleCount;
} D3D11_RASTERIZER_DESC1;
STATIC_ASSERT(sizeof(D3D11_RASTERIZER_DESC1) == 44, D3D11RasterizerDesc1SizeCheck);

typedef struct D3D11_RENDER_TARGET_BLEND_DESC {
    b32            BlendEnable;
    D3D11_BLEND    SrcBlend;
    D3D11_BLEND    DestBlend;
    D3D11_BLEND_OP BlendOp;
    D3D11_BLEND    SrcBlendAlpha;
    D3D11_BLEND    DestBlendAlpha;
    D3D11_BLEND_OP BlendOpAlpha;
    BYTE           RenderTargetWriteMask;
} D3D11_RENDER_TARGET_BLEND_DESC;

typedef struct D3D11_BLEND_DESC {
    b32                            AlphaToCoverageEnable;
    b32                            IndependentBlendEnable;
    D3D11_RENDER_TARGET_BLEND_DESC RenderTarget[8];
} D3D11_BLEND_DESC;
STATIC_ASSERT(sizeof(D3D11_BLEND_DESC) == 264, D3D11BlendDescSizeCheck);

typedef struct D3D11_SAMPLER_DESC {
    D3D11_FILTER               Filter;
    D3D11_TEXTURE_ADDRESS_MODE AddressU;
    D3D11_TEXTURE_ADDRESS_MODE AddressV;
    D3D11_TEXTURE_ADDRESS_MODE AddressW;
    FLOAT                      MipLODBias;
    UINT                       MaxAnisotropy;
    D3D11_COMPARISON_FUNC      ComparisonFunc;
    FLOAT                      BorderColor[4];
    FLOAT                      MinLOD;
    FLOAT                      MaxLOD;
} D3D11_SAMPLER_DESC;
STATIC_ASSERT(sizeof(D3D11_SAMPLER_DESC) == 52, D3D11SamplerDescSizeCheck);

typedef struct D3D11_DEPTH_STENCILOP_DESC {
    D3D11_STENCIL_OP      StencilFailOp;
    D3D11_STENCIL_OP      StencilDepthFailOp;
    D3D11_STENCIL_OP      StencilPassOp;
    D3D11_COMPARISON_FUNC StencilFunc;
} D3D11_DEPTH_STENCILOP_DESC;

typedef struct D3D11_DEPTH_STENCIL_DESC {
    b32                        DepthEnable;
    D3D11_DEPTH_WRITE_MASK     DepthWriteMask;
    D3D11_COMPARISON_FUNC      DepthFunc;
    b32                        StencilEnable;
    BYTE                       StencilReadMask;
    BYTE                       StencilWriteMask;
    D3D11_DEPTH_STENCILOP_DESC FrontFace;
    D3D11_DEPTH_STENCILOP_DESC BackFace;
} D3D11_DEPTH_STENCIL_DESC;
STATIC_ASSERT(sizeof(D3D11_DEPTH_STENCIL_DESC) == 52, D3D11DepthStencilDescSizeCheck);

typedef struct DXGI_SWAP_CHAIN_FULLSCREEN_DESC {
    DXGI_RATIONAL            RefreshRate;
    DXGI_MODE_SCANLINE_ORDER ScanlineOrdering;
    DXGI_MODE_SCALING        Scaling;
    b32                      Windowed;
} DXGI_SWAP_CHAIN_FULLSCREEN_DESC;

typedef struct D3D11_RENDER_TARGET_VIEW_DESC {
    DXGI_FORMAT         Format;
    D3D11_RTV_DIMENSION ViewDimension;

    union {
        D3D11_BUFFER_RTV        Buffer;
        D3D11_TEX1D_RTV         Texture1D;
        D3D11_TEX1D_ARRAY_RTV   Texture1DArray;
        D3D11_TEX2D_RTV         Texture2D;
        D3D11_TEX2D_ARRAY_RTV   Texture2DArray;
        D3D11_TEX2DMS_RTV       Texture2DMS;
        D3D11_TEX2DMS_ARRAY_RTV Texture2DMSArray;
        D3D11_TEX3D_RTV         Texture3D;
    };
} D3D11_RENDER_TARGET_VIEW_DESC;

struct IUnknown {
    IUnknownVtbl* Vtbl;
};

struct ID3D11Debug {
    ID3D11DebugVtbl* Vtbl;
};

struct IDXGIDebug {
    IDXGIDebugVtbl* Vtbl;
};

struct ID3D11Device { 
    ID3D11DeviceVtbl* Vtbl; 
};

struct ID3D11DeviceContext {
    ID3D11DeviceContextVtbl* Vtbl; 
};

struct IDXGISwapChain {
    IDXGISwapChainVtbl* Vtbl; 
};

struct ID3D11RenderTargetView {
    ID3D11RenderTargetViewVtbl* Vtbl; 
};

struct ID3D11Texture2D {
    ID3D11Texture2DVtbl* Vtbl;
};

struct ID3D11VertexShader {
    ID3D11VertexShaderVtbl* Vtbl; 
};

struct ID3D11PixelShader {
    ID3D11PixelShaderVtbl* Vtbl; 
};

struct ID3DBlob {
    ID3DBlobVtbl* Vtbl; 
};

struct ID3D11ShaderResourceView {
    ID3D11ShaderResourceViewVtbl* Vtbl; 
};

struct ID3D11DepthStencilView {
    ID3D11DepthStencilViewVtbl* Vtbl; 
};

struct ID3D11Buffer {
    ID3D11BufferVtbl* Vtbl; 
};

struct IDXGISwapChain1 {
    IDXGISwapChain1Vtbl* Vtbl; 
};

struct ID3D11Device1 {
    ID3D11Device1Vtbl* Vtbl; 
};

struct ID3D11DeviceContext1 {
    ID3D11DeviceContext1Vtbl* Vtbl; 
};

struct IDXGIFactory2 {
    IDXGIFactory2Vtbl* Vtbl; 
};

struct ID3D11RasterizerState1 {
    ID3D11RasterizerState1Vtbl* Vtbl; 
};

struct ID3D11RasterizerState {
    ID3D11RasterizerStateVtbl* Vtbl; 
};

struct ID3D11BlendState {
    ID3D11BlendStateVtbl* Vtbl; 
};

struct ID3D11SamplerState {
    ID3D11SamplerStateVtbl* Vtbl; 
};

struct ID3D11DepthStencilState {
    ID3D11DepthStencilStateVtbl* Vtbl; 
};

struct ID3D11InputLayout {
    ID3D11InputLayoutVtbl* Vtbl; 
};

struct IDXGIDevice1 {
    IDXGIDevice1Vtbl* Vtbl;
};

struct IDXGIAdapter {
    IDXGIAdapterVtbl* Vtbl;
};

struct ID3D11InfoQueue {
    ID3D11InfoQueueVtbl* Vtbl;
};

struct IDXGIOutput {
    IDXGIOutputVtbl* Vtbl;
};

// @callbacks__________________________________________________________________
typedef HRESULT WINAPI IUnknownQueryInterfaceFuncType(THIS, REFIID riid, void** ppv);
typedef ULONG   WINAPI IUnknownAddRefFuncType(THIS);
typedef ULONG   WINAPI IUnknownReleaseFuncType(THIS);
typedef HRESULT WINAPI DX11SwapChain_PresentFuncType(THIS, UINT sync, UINT flags);
typedef HRESULT WINAPI DX11SwapChainGetBufferFuncType(THIS, UINT buffer, REFIID* riid, void** surface);
typedef void*   WINAPI DX11BlobGetBufferPointerFuncType(THIS);
typedef SIZE_T  WINAPI DX11BlobGetBufferSizeFuncType(THIS);
typedef HRESULT WINAPI DX11DevCreateRenderTargetViewFuncType(THIS, void* resource, void* desc, ID3D11RenderTargetView** rtv);
typedef HRESULT WINAPI DX11DevCreateBufferFuncType(THIS, const D3D11_BUFFER_DESC* desc, const D3D11_SUBRESOURCE_DATA* data, ID3D11Buffer** buffer);
typedef HRESULT WINAPI DX11DevCreateTexture2DFuncType(THIS, const D3D11_TEXTURE2D_DESC* desc, const D3D11_SUBRESOURCE_DATA* data, ID3D11Texture2D** texture);
typedef HRESULT WINAPI DX11DevCreateShaderResourceViewFuncType(THIS, void* resource, const D3D11_SHADER_RESOURCE_VIEW_DESC* desc, ID3D11ShaderResourceView** srv);
typedef HRESULT WINAPI DX11DevCreateDepthStencilViewFuncType(THIS, void* resource, const D3D11_DEPTH_STENCIL_VIEW_DESC* desc, ID3D11DepthStencilView** dsv);
typedef HRESULT WINAPI DX11DevCreateVertexShaderFuncType(THIS, const void* bytecode, SIZE_T size, void* linker, ID3D11VertexShader** shader);
typedef HRESULT WINAPI DX11DevCreatePixelShaderFuncType(THIS, const void* bytecode, SIZE_T size, void* linker, ID3D11PixelShader** shader);
typedef HRESULT WINAPI DX11DevCreateInputLayoutFuncType(THIS, const D3D11_INPUT_ELEMENT_DESC* elements, UINT count, const void* bytecode, SIZE_T size, ID3D11InputLayout** layout);
typedef HRESULT WINAPI DX11DevCreateSamplerStateFuncType(THIS, const D3D11_SAMPLER_DESC* desc, ID3D11SamplerState** sampler);
typedef HRESULT WINAPI DX11DevCreateRasterizerStateFuncType(THIS, const D3D11_RASTERIZER_DESC* desc, ID3D11RasterizerState** state);
typedef HRESULT WINAPI DX11DevCreateBlendStateFuncType(THIS, const D3D11_BLEND_DESC* desc, ID3D11BlendState** state);
typedef HRESULT WINAPI DX11DevCreateDepthStencilStateFuncType(THIS, const D3D11_DEPTH_STENCIL_DESC* desc, ID3D11DepthStencilState** state);
typedef void    WINAPI DX11DevCtxClearRenderTargetViewFuncType(THIS, ID3D11RenderTargetView* rtv, const FLOAT* color);
typedef void    WINAPI DX11DevCtxOMSetRenderTargetsFuncType(THIS, UINT num, ID3D11RenderTargetView** rtvs, void* dsv);
typedef void    WINAPI DX11DevCtxRSSetViewportsFuncType(THIS, UINT num, const D3D11_VIEWPORT* viewports);
typedef void    WINAPI DX11DevCtxVSSetShaderFuncType(THIS, ID3D11VertexShader* shader, void** instances, UINT count);
typedef void    WINAPI DX11DevCtxPSSetShaderFuncType(THIS, ID3D11PixelShader* shader, void** instances, UINT count);
typedef void    WINAPI DX11DevCtxPSSetShaderResourcesFuncType(THIS, UINT start, UINT count, ID3D11ShaderResourceView** srvs);
typedef void    WINAPI DX11DevCtxPSSetSamplersFuncType(THIS, UINT start, UINT count, ID3D11SamplerState** samplers);
typedef void    WINAPI DX11DevCtxIASetVertexBuffersFuncType(THIS, UINT start, UINT count, ID3D11Buffer** buffers, const UINT* strides, const UINT* offsets);
typedef void    WINAPI DX11DevCtxIASetIndexBufferFuncType(THIS, ID3D11Buffer* buffer, DXGI_FORMAT format, UINT offset);
typedef void    WINAPI DX11DevCtxIASetInputLayoutFuncType(THIS, ID3D11InputLayout* layout);
typedef void    WINAPI DX11DevCtxIASetPrimitiveTopologyFuncType(THIS, D3D11_PRIMITIVE_TOPOLOGY topology);
typedef void    WINAPI DX11DevCtxDrawFuncType(THIS, UINT count, UINT start);
typedef void    WINAPI DX11DevCtxDrawIndexedFuncType(THIS, UINT count, UINT start, INT base);
typedef void    WINAPI DX11DevCtxUpdateSubresourceFuncType(THIS, void* resource, UINT subresource, const void* box, const void* data, UINT pitch, UINT depth);
typedef HRESULT WINAPI DX11DevCtxMapFuncType(THIS, void* resource, UINT subresource, D3D11_MAP maptype, UINT flags, D3D11_MAPPED_SUBRESOURCE* mapped);
typedef void    WINAPI DX11DevCtxUnmapFuncType(THIS, void* resource, UINT subresource);
typedef void    WINAPI DX11DevCtxVSSetConstantBuffersFuncType(THIS, UINT start, UINT count, ID3D11Buffer** buffers);
typedef void    WINAPI DX11DevCtxPSSetConstantBuffersFuncType(THIS, UINT start, UINT count, ID3D11Buffer** buffers);
typedef void    WINAPI DX11DevCtxRSSetStateFuncType(THIS, ID3D11RasterizerState* state);
typedef void    WINAPI DX11DevCtxOMSetBlendStateFuncType(THIS, ID3D11BlendState* state, const FLOAT* factor, UINT mask);
typedef void    WINAPI DX11DevCtxOMSetDepthStencilStateFuncType(THIS, ID3D11DepthStencilState* state, UINT stencil);
typedef HRESULT WINAPI DXGIDevGetAdapterFuncType(THIS, IDXGIAdapter** pAdapter);
typedef HRESULT WINAPI DXGIObjGetParentFuncType(THIS, REFIID riid, void** ppParent);
typedef HRESULT WINAPI DX11Dev1CreateRasteriserState1FuncType(THIS, D3D11_RASTERIZER_DESC1 const* pRasteriserDesc, ID3D11RasterizerState1** ppRasteriserState);
typedef HRESULT WINAPI DX11InfoQueueSetBreakOnSeverityFuncType(THIS, D3D11_MESSAGE_SEVERITY Severity, BOOL bEnable);
typedef HRESULT WINAPI DXGIDev1SetMaximumFrameLatencyFuncType(THIS, UINT MaxLatency);
typedef HRESULT WINAPI DXGIFact2CreateSwapChainForHwndFuncType(THIS, IUnknown* pDevice, HWND hWnd, DXGI_SWAP_CHAIN_DESC1 const* pDesc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC const* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain);
typedef HRESULT WINAPI DXGIFactMakeWindowAssociationFuncType(THIS, HWND WindowHandle, UINT Flags);
typedef HRESULT WINAPI DXGISwapChainResizeBuffersFuncType(THIS, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags);
typedef void    WINAPI DX11Tex2DGetDescFuncType(THIS, D3D11_TEXTURE2D_DESC* pDesc);
typedef void    WINAPI DX11DevCtxFlushFuncType(THIS);
typedef void    WINAPI DX11DevCtxRSSetScissorRectsFuncType(THIS, UINT NumRects, D3D11_RECT const* pRects);
typedef void    WINAPI DX11DevCtxClearStateFuncType(THIS);
typedef void    WINAPI DX11DevCtxClearDepthStencilViewFuncType(THIS, ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil);
typedef void    WINAPI DX11DevCtx1PSSetConstantBuffers1FuncType(THIS, UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers, UINT const* pFirstConstant, UINT const* pNumConstants);
typedef void    WINAPI DX11DevCtxDrawInstancedFuncType(THIS, UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation);
typedef void    WINAPI DX11DevCtxVSSetShaderResourcesFuncType(THIS, UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews);
typedef void    WINAPI DX11DevCtxDrawIndexedInstancedFuncType(THIS, UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation);
typedef void    WINAPI DX11DevCtxCopyResourceFuncType(THIS, void* pDstResource, void* pSrcResource);
typedef HRESULT WINAPI DX11DebugReportLiveDeviceObjectsFuncType(THIS, D3D11_RLDO_FLAGS Flags);
typedef HRESULT WINAPI DXGIDebugReportLiveObjectsFuncType(THIS, GUID apiid, DXGI_DEBUG_RLO_FLAGS flags);

// @vtbls______________________________________________________________________
struct IUnknownVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
};

struct ID3D11DebugVtbl {
    IUnknownQueryInterfaceFuncType*           QueryInterface;
    IUnknownAddRefFuncType*                   AddRef;
    IUnknownReleaseFuncType*                  Release;
    void*                                     GetFeatureMask;
    void*                                     GetPresentPerRenderOpDelay;
    void*                                     GetSwapChain;
    DX11DebugReportLiveDeviceObjectsFuncType* ReportLiveDeviceObjects;
    void*                                     SetFeatureMask;
    void*                                     SetPresentPerRenderOpDelay;
    void*                                     SetSwapChain;
    void*                                     ValidateContext;
    void*                                     ValidateContextForDispatch;
};

struct IDXGIDebugVtbl {
    IUnknownQueryInterfaceFuncType*     QueryInterface;
    IUnknownAddRefFuncType*             AddRef;
    IUnknownReleaseFuncType*            Release;
    DXGIDebugReportLiveObjectsFuncType* ReportLiveObjects;
};

struct ID3DBlobVtbl {
    IUnknownQueryInterfaceFuncType*   QueryInterface;
    IUnknownAddRefFuncType*           AddRef;
    IUnknownReleaseFuncType*          Release;
    DX11BlobGetBufferPointerFuncType* GetBufferPointer;
    DX11BlobGetBufferSizeFuncType*    GetBufferSize;
};

struct ID3D11DeviceChildVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
};

struct ID3D11VertexShaderVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
};

struct ID3D11PixelShaderVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
};

struct ID3D11ShaderResourceViewVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetResource;
    void*                           GetDesc;
};

struct ID3D11RenderTargetViewVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetResource;
    void*                           GetDesc;
};

struct ID3D11DepthStencilViewVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetResource;
    void*                           GetDesc;
};

struct ID3D11BufferVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetType;
    void*                           SetEvictionPriority;
    void*                           GetEvictionPriority;
    void*                           GetDesc;
};

struct ID3D11Texture2DVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetType;
    void*                           SetEvictionPriority;
    void*                           GetEvictionPriority;
    DX11Tex2DGetDescFuncType*       GetDesc;
};

struct ID3D11InputLayoutVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
};

struct ID3D11RasterizerStateVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetDesc;
};

struct ID3D11RasterizerState1Vtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetDesc;
    void*                           GetDesc1;
};

struct ID3D11BlendStateVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetDesc;
};

struct ID3D11DepthStencilStateVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetDesc;
};

struct ID3D11SamplerStateVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           GetDevice;
    void*                           GetPrivateData;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetDesc;
};

struct ID3D11DeviceVtbl {
    IUnknownQueryInterfaceFuncType*          QueryInterface;
    IUnknownAddRefFuncType*                  AddRef;
    IUnknownReleaseFuncType*                 Release;
    DX11DevCreateBufferFuncType*             CreateBuffer;
    void*                                    CreateTexture1D;
    DX11DevCreateTexture2DFuncType*          CreateTexture2D;
    void*                                    CreateTexture3D;
    DX11DevCreateShaderResourceViewFuncType* CreateShaderResourceView;
    void*                                    CreateUnorderedAccessView;
    DX11DevCreateRenderTargetViewFuncType*   CreateRenderTargetView;
    DX11DevCreateDepthStencilViewFuncType*   CreateDepthStencilView;
    DX11DevCreateInputLayoutFuncType*        CreateInputLayout;
    DX11DevCreateVertexShaderFuncType*       CreateVertexShader;
    void*                                    CreateGeometryShader;
    void*                                    CreateGeometryShaderWithStreamOutput;
    DX11DevCreatePixelShaderFuncType*        CreatePixelShader;
    void*                                    CreateHullShader;
    void*                                    CreateDomainShader;
    void*                                    CreateComputeShader;
    void*                                    CreateClassLinkage;
    DX11DevCreateBlendStateFuncType*         CreateBlendState;
    DX11DevCreateDepthStencilStateFuncType*  CreateDepthStencilState;
    DX11DevCreateRasterizerStateFuncType*    CreateRasterizerState;
    DX11DevCreateSamplerStateFuncType*       CreateSamplerState;
    void*                                    CreateQuery;
    void*                                    CreatePredicate;
    void*                                    CreateCounter;
    void*                                    CreateDeferredContext;
    void*                                    OpenSharedResource;
    void*                                    CheckFormatSupport;
    void*                                    CheckMultisampleQualityLevels;
    void*                                    CheckCounterInfo;
    void*                                    CheckCounter;
    void*                                    CheckFeatureSupport;
    void*                                    GetPrivateData;
    void*                                    SetPrivateData;
    void*                                    SetPrivateDataInterface;
    void*                                    GetFeatureLevel;
    void*                                    GetCreationFlags;
    void*                                    GetDeviceRemovedReason;
    void*                                    GetImmediateContext;
    void*                                    SetExceptionMode;
    void*                                    GetExceptionMode;
};

struct ID3D11Device1Vtbl {
    IUnknownQueryInterfaceFuncType*          QueryInterface;
    IUnknownAddRefFuncType*                  AddRef;
    IUnknownReleaseFuncType*                 Release;
    DX11DevCreateBufferFuncType*             CreateBuffer;
    void*                                    CreateTexture1D;
    DX11DevCreateTexture2DFuncType*          CreateTexture2D;
    void*                                    CreateTexture3D;
    DX11DevCreateShaderResourceViewFuncType* CreateShaderResourceView;
    void*                                    CreateUnorderedAccessView;
    DX11DevCreateRenderTargetViewFuncType*   CreateRenderTargetView;
    DX11DevCreateDepthStencilViewFuncType*   CreateDepthStencilView;
    DX11DevCreateInputLayoutFuncType*        CreateInputLayout;
    DX11DevCreateVertexShaderFuncType*       CreateVertexShader;
    void*                                    CreateGeometryShader;
    void*                                    CreateGeometryShaderWithStreamOutput;
    DX11DevCreatePixelShaderFuncType*        CreatePixelShader;
    void*                                    CreateHullShader;
    void*                                    CreateDomainShader;
    void*                                    CreateComputeShader;
    void*                                    CreateClassLinkage;
    DX11DevCreateBlendStateFuncType*         CreateBlendState;
    DX11DevCreateDepthStencilStateFuncType*  CreateDepthStencilState;
    DX11DevCreateRasterizerStateFuncType*    CreateRasterizerState;
    DX11DevCreateSamplerStateFuncType*       CreateSamplerState;
    void*                                    CreateQuery;
    void*                                    CreatePredicate;
    void*                                    CreateCounter;
    void*                                    CreateDeferredContext;
    void*                                    OpenSharedResource;
    void*                                    CheckFormatSupport;
    void*                                    CheckMultisampleQualityLevels;
    void*                                    CheckCounterInfo;
    void*                                    CheckCounter;
    void*                                    CheckFeatureSupport;
    void*                                    GetPrivateData;
    void*                                    SetPrivateData;
    void*                                    SetPrivateDataInterface;
    void*                                    GetFeatureLevel;
    void*                                    GetCreationFlags;
    void*                                    GetDeviceRemovedReason;
    void*                                    GetImmediateContext;
    void*                                    SetExceptionMode;
    void*                                    GetExceptionMode;
    void*                                    GetImmediateContext1;
    void*                                    CreateDeferredContext1;
    void*                                    CreateBlendState1;
    DX11Dev1CreateRasteriserState1FuncType*  CreateRasterizerState1;
    void*                                    CreateDeviceContextState;
    void*                                    OpenSharedResource1;
    void*                                    OpenSharedResourceByName;
};

struct ID3D11DeviceContextVtbl {
    IUnknownQueryInterfaceFuncType*           QueryInterface;
    IUnknownAddRefFuncType*                   AddRef;
    IUnknownReleaseFuncType*                  Release;
    void*                                     GetDevice;
    void*                                     GetPrivateData;
    void*                                     SetPrivateData;
    void*                                     SetPrivateDataInterface;
    DX11DevCtxVSSetConstantBuffersFuncType*   VSSetConstantBuffers;
    DX11DevCtxPSSetShaderResourcesFuncType*   PSSetShaderResources;
    DX11DevCtxPSSetShaderFuncType*            PSSetShader;
    DX11DevCtxPSSetSamplersFuncType*          PSSetSamplers;
    DX11DevCtxVSSetShaderFuncType*            VSSetShader;
    DX11DevCtxDrawIndexedFuncType*            DrawIndexed;
    DX11DevCtxDrawFuncType*                   Draw;
    DX11DevCtxMapFuncType*                    Map;
    DX11DevCtxUnmapFuncType*                  Unmap;
    DX11DevCtxPSSetConstantBuffersFuncType*   PSSetConstantBuffers;
    DX11DevCtxIASetInputLayoutFuncType*       IASetInputLayout;
    DX11DevCtxIASetVertexBuffersFuncType*     IASetVertexBuffers;
    DX11DevCtxIASetIndexBufferFuncType*       IASetIndexBuffer;
    DX11DevCtxDrawIndexedInstancedFuncType*   DrawIndexedInstanced;
    DX11DevCtxDrawInstancedFuncType*          DrawInstanced;
    void*                                     GSSetConstantBuffers;
    void*                                     GSSetShader;
    DX11DevCtxIASetPrimitiveTopologyFuncType* IASetPrimitiveTopology;
    DX11DevCtxVSSetShaderResourcesFuncType*   VSSetShaderResources;
    void*                                     VSSetSamplers;
    void*                                     Begin;
    void*                                     End;
    void*                                     GetData;
    void*                                     SetPredication;
    void*                                     GSSetShaderResources;
    void*                                     GSSetSamplers;
    DX11DevCtxOMSetRenderTargetsFuncType*     OMSetRenderTargets;
    void*                                     OMSetRenderTargetsAndUnorderedAccessViews;
    DX11DevCtxOMSetBlendStateFuncType*        OMSetBlendState;
    DX11DevCtxOMSetDepthStencilStateFuncType* OMSetDepthStencilState;
    void*                                     SOSetTargets;
    void*                                     DrawAuto;
    void*                                     DrawIndexedInstancedIndirect;
    void*                                     DrawInstancedIndirect;
    void*                                     Dispatch;
    void*                                     DispatchIndirect;
    DX11DevCtxRSSetStateFuncType*             RSSetState;
    DX11DevCtxRSSetViewportsFuncType*         RSSetViewports;
    DX11DevCtxRSSetScissorRectsFuncType*      RSSetScissorRects;
    void*                                     CopySubresourceRegion;
    DX11DevCtxCopyResourceFuncType*           CopyResource;
    DX11DevCtxUpdateSubresourceFuncType*      UpdateSubresource;
    void*                                     CopyStructureCount;
    DX11DevCtxClearRenderTargetViewFuncType*  ClearRenderTargetView;
    void*                                     ClearUnorderedAccessViewUint;
    void*                                     ClearUnorderedAccessViewFloat;
    DX11DevCtxClearDepthStencilViewFuncType*  ClearDepthStencilView;
    void*                                     GenerateMips;
    void*                                     SetResourceMinLOD;
    void*                                     GetResourceMinLOD;
    void*                                     ResolveSubresource;
    void*                                     ExecuteCommandList;
    void*                                     HSSetShaderResources;
    void*                                     HSSetShader;
    void*                                     HSSetSamplers;
    void*                                     HSSetConstantBuffers;
    void*                                     DSSetShaderResources;
    void*                                     DSSetShader;
    void*                                     DSSetSamplers;
    void*                                     DSSetConstantBuffers;
    void*                                     CSSetShaderResources;
    void*                                     CSSetUnorderedAccessViews;
    void*                                     CSSetShader;
    void*                                     CSSetSamplers;
    void*                                     CSSetConstantBuffers;
    void*                                     VSGetConstantBuffers;
    void*                                     PSGetShaderResources;
    void*                                     PSGetShader;
    void*                                     PSGetSamplers;
    void*                                     VSGetShader;
    void*                                     PSGetConstantBuffers;
    void*                                     IAGetInputLayout;
    void*                                     IAGetVertexBuffers;
    void*                                     IAGetIndexBuffer;
    void*                                     GSGetConstantBuffers;
    void*                                     GSGetShader;
    void*                                     IAGetPrimitiveTopology;
    void*                                     VSGetShaderResources;
    void*                                     VSGetSamplers;
    void*                                     GetPredication;
    void*                                     GSGetShaderResources;
    void*                                     GSGetSamplers;
    void*                                     OMGetRenderTargets;
    void*                                     OMGetRenderTargetsAndUnorderedAccessViews;
    void*                                     OMGetBlendState;
    void*                                     OMGetDepthStencilState;
    void*                                     SOGetTargets;
    void*                                     RSGetState;
    void*                                     RSGetViewports;
    void*                                     RSGetScissorRects;
    void*                                     HSGetShaderResources;
    void*                                     HSGetShader;
    void*                                     HSGetSamplers;
    void*                                     HSGetConstantBuffers;
    void*                                     DSGetShaderResources;
    void*                                     DSGetShader;
    void*                                     DSGetSamplers;
    void*                                     DSGetConstantBuffers;
    void*                                     CSGetShaderResources;
    void*                                     CSGetUnorderedAccessViews;
    void*                                     CSGetShader;
    void*                                     CSGetSamplers;
    void*                                     CSGetConstantBuffers;
    DX11DevCtxClearStateFuncType*             ClearState;
    DX11DevCtxFlushFuncType*                  Flush;
    void*                                     GetType;
    void*                                     GetContextFlags;
    void*                                     FinishCommandList;
};

struct ID3D11DeviceContext1Vtbl {
    IUnknownQueryInterfaceFuncType*           QueryInterface;
    IUnknownAddRefFuncType*                   AddRef;
    IUnknownReleaseFuncType*                  Release;
    void*                                     GetDevice;
    void*                                     GetPrivateData;
    void*                                     SetPrivateData;
    void*                                     SetPrivateDataInterface;
    DX11DevCtxVSSetConstantBuffersFuncType*   VSSetConstantBuffers;
    DX11DevCtxPSSetShaderResourcesFuncType*   PSSetShaderResources;
    DX11DevCtxPSSetShaderFuncType*            PSSetShader;
    DX11DevCtxPSSetSamplersFuncType*          PSSetSamplers;
    DX11DevCtxVSSetShaderFuncType*            VSSetShader;
    DX11DevCtxDrawIndexedFuncType*            DrawIndexed;
    DX11DevCtxDrawFuncType*                   Draw;
    DX11DevCtxMapFuncType*                    Map;
    DX11DevCtxUnmapFuncType*                  Unmap;
    DX11DevCtxPSSetConstantBuffersFuncType*   PSSetConstantBuffers;
    DX11DevCtxIASetInputLayoutFuncType*       IASetInputLayout;
    DX11DevCtxIASetVertexBuffersFuncType*     IASetVertexBuffers;
    DX11DevCtxIASetIndexBufferFuncType*       IASetIndexBuffer;
    DX11DevCtxDrawIndexedInstancedFuncType*   DrawIndexedInstanced;
    DX11DevCtxDrawInstancedFuncType*          DrawInstanced;
    void*                                     GSSetConstantBuffers;
    void*                                     GSSetShader;
    DX11DevCtxIASetPrimitiveTopologyFuncType* IASetPrimitiveTopology;
    DX11DevCtxVSSetShaderResourcesFuncType*   VSSetShaderResources;
    void*                                     VSSetSamplers;
    void*                                     Begin;
    void*                                     End;
    void*                                     GetData;
    void*                                     SetPredication;
    void*                                     GSSetShaderResources;
    void*                                     GSSetSamplers;
    DX11DevCtxOMSetRenderTargetsFuncType*     OMSetRenderTargets;
    void*                                     OMSetRenderTargetsAndUnorderedAccessViews;
    DX11DevCtxOMSetBlendStateFuncType*        OMSetBlendState;
    DX11DevCtxOMSetDepthStencilStateFuncType* OMSetDepthStencilState;
    void*                                     SOSetTargets;
    void*                                     DrawAuto;
    void*                                     DrawIndexedInstancedIndirect;
    void*                                     DrawInstancedIndirect;
    void*                                     Dispatch;
    void*                                     DispatchIndirect;
    DX11DevCtxRSSetStateFuncType*             RSSetState;
    DX11DevCtxRSSetViewportsFuncType*         RSSetViewports;
    DX11DevCtxRSSetScissorRectsFuncType*      RSSetScissorRects;
    void*                                     CopySubresourceRegion;
    DX11DevCtxCopyResourceFuncType*           CopyResource;
    DX11DevCtxUpdateSubresourceFuncType*      UpdateSubresource;
    void*                                     CopyStructureCount;
    DX11DevCtxClearRenderTargetViewFuncType*  ClearRenderTargetView;
    void*                                     ClearUnorderedAccessViewUint;
    void*                                     ClearUnorderedAccessViewFloat;
    DX11DevCtxClearDepthStencilViewFuncType*  ClearDepthStencilView;
    void*                                     GenerateMips;
    void*                                     SetResourceMinLOD;
    void*                                     GetResourceMinLOD;
    void*                                     ResolveSubresource;
    void*                                     ExecuteCommandList;
    void*                                     HSSetShaderResources;
    void*                                     HSSetShader;
    void*                                     HSSetSamplers;
    void*                                     HSSetConstantBuffers;
    void*                                     DSSetShaderResources;
    void*                                     DSSetShader;
    void*                                     DSSetSamplers;
    void*                                     DSSetConstantBuffers;
    void*                                     CSSetShaderResources;
    void*                                     CSSetUnorderedAccessViews;
    void*                                     CSSetShader;
    void*                                     CSSetSamplers;
    void*                                     CSSetConstantBuffers;
    void*                                     VSGetConstantBuffers;
    void*                                     PSGetShaderResources;
    void*                                     PSGetShader;
    void*                                     PSGetSamplers;
    void*                                     VSGetShader;
    void*                                     PSGetConstantBuffers;
    void*                                     IAGetInputLayout;
    void*                                     IAGetVertexBuffers;
    void*                                     IAGetIndexBuffer;
    void*                                     GSGetConstantBuffers;
    void*                                     GSGetShader;
    void*                                     IAGetPrimitiveTopology;
    void*                                     VSGetShaderResources;
    void*                                     VSGetSamplers;
    void*                                     GetPredication;
    void*                                     GSGetShaderResources;
    void*                                     GSGetSamplers;
    void*                                     OMGetRenderTargets;
    void*                                     OMGetRenderTargetsAndUnorderedAccessViews;
    void*                                     OMGetBlendState;
    void*                                     OMGetDepthStencilState;
    void*                                     SOGetTargets;
    void*                                     RSGetState;
    void*                                     RSGetViewports;
    void*                                     RSGetScissorRects;
    void*                                     HSGetShaderResources;
    void*                                     HSGetShader;
    void*                                     HSGetSamplers;
    void*                                     HSGetConstantBuffers;
    void*                                     DSGetShaderResources;
    void*                                     DSGetShader;
    void*                                     DSGetSamplers;
    void*                                     DSGetConstantBuffers;
    void*                                     CSGetShaderResources;
    void*                                     CSGetUnorderedAccessViews;
    void*                                     CSGetShader;
    void*                                     CSGetSamplers;
    void*                                     CSGetConstantBuffers;
    DX11DevCtxClearStateFuncType*             ClearState;
    DX11DevCtxFlushFuncType*                  Flush;
    void*                                     GetType;
    void*                                     GetContextFlags;
    void*                                     FinishCommandList;
    void*                                     CopySubresourceRegion1;
    void*                                     UpdateSubresource1;
    void*                                     DiscardResource;
    void*                                     DiscardView;
    void*                                     VSSetConstantBuffers1;
    void*                                     HSSetConstantBuffers1;
    void*                                     DSSetConstantBuffers1;
    void*                                     GSSetConstantBuffers1;
    DX11DevCtx1PSSetConstantBuffers1FuncType* PSSetConstantBuffers1;
    void*                                     CSSetConstantBuffers1;
    void*                                     VSGetConstantBuffers1;
    void*                                     HSGetConstantBuffers1;
    void*                                     DSGetConstantBuffers1;
    void*                                     GSGetConstantBuffers1;
    void*                                     PSGetConstantBuffers1;
    void*                                     CSGetConstantBuffers1;
    void*                                     SwapDeviceContextState;
    void*                                     ClearView;
    void*                                     DiscardView1;
};

struct IDXGISwapChainVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetPrivateData;
    DXGIObjGetParentFuncType*       GetParent;
    void*                           GetDevice;
    DX11SwapChain_PresentFuncType*  Present;
    DX11SwapChainGetBufferFuncType* GetBuffer;
    void*                           SetFullscreenState;
    void*                           GetFullscreenState;
    void*                           GetDesc;
    DXGISwapChainResizeBuffersFuncType*  ResizeBuffers;
    void*                           ResizeTarget;
    void*                           GetContainingOutput;
    void*                           GetFrameStatistics;
    void*                           GetLastPresentCount;
};

struct IDXGISwapChain1Vtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetPrivateData;
    DXGIObjGetParentFuncType*       GetParent;
    void*                           GetDevice;
    DX11SwapChain_PresentFuncType*  Present;
    DX11SwapChainGetBufferFuncType* GetBuffer;
    void*                           SetFullscreenState;
    void*                           GetFullscreenState;
    void*                           GetDesc;
    DXGISwapChainResizeBuffersFuncType*  ResizeBuffers;
    void*                           ResizeTarget;
    void*                           GetContainingOutput;
    void*                           GetFrameStatistics;
    void*                           GetLastPresentCount;
    void*                           GetDesc1;
    void*                           GetFullscreenDesc;
    void*                           GetHwnd;
    void*                           GetCoreWindow;
    void*                           Present1;
    void*                           IsTemporaryMonoSupported;
    void*                           GetRestrictToOutput;
    void*                           SetBackgroundColor;
    void*                           GetBackgroundColor;
    void*                           SetRotation;
    void*                           GetRotation;
};

struct IDXGIFactory2Vtbl {
    IUnknownQueryInterfaceFuncType*          QueryInterface;
    IUnknownAddRefFuncType*                  AddRef;
    IUnknownReleaseFuncType*                 Release;
    void*                                    SetPrivateData;
    void*                                    SetPrivateDataInterface;
    void*                                    GetPrivateData;
    DXGIObjGetParentFuncType*                GetParent;
    void*                                    EnumAdapters;
    DXGIFactMakeWindowAssociationFuncType*   MakeWindowAssociation;
    void*                                    GetWindowAssociation;
    void*                                    CreateSwapChain;
    void*                                    CreateSoftwareAdapter;
    void*                                    EnumAdapters1;
    void*                                    IsCurrent;
    void*                                    IsWindowedStereoEnabled;
    DXGIFact2CreateSwapChainForHwndFuncType* CreateSwapChainForHwnd;
    void*                                    CreateSwapChainForCoreWindow;
    void*                                    GetSharedResourceAdapterLuid;
    void*                                    RegisterStereoStatusWindow;
    void*                                    RegisterStereoStatusEvent;
    void*                                    UnregisterStereoStatus;
    void*                                    RegisterOcclusionStatusWindow;
    void*                                    RegisterOcclusionStatusEvent;
    void*                                    UnregisterOcclusionStatus;
    void*                                    CreateSwapChainForComposition;
};

struct IDXGIDevice1Vtbl {
    IUnknownQueryInterfaceFuncType*         QueryInterface;
    IUnknownAddRefFuncType*                 AddRef;
    IUnknownReleaseFuncType*                Release;
    void*                                   SetPrivateData;
    void*                                   SetPrivateDataInterface;
    void*                                   GetPrivateData;
    DXGIObjGetParentFuncType*               GetParent;
    DXGIDevGetAdapterFuncType*              GetAdapter;
    void*                                   CreateSurface;
    void*                                   QueryResourceResidency;
    void*                                   SetGPUThreadPriority;
    void*                                   GetGPUThreadPriority;
    DXGIDev1SetMaximumFrameLatencyFuncType* SetMaximumFrameLatency;
    void*                                   GetMaximumFrameLatency;
};

struct IDXGIAdapterVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetPrivateData;
    DXGIObjGetParentFuncType*       GetParent;
    void*                           EnumOutputs;
    void*                           GetDesc;
    void*                           CheckInterfaceSupport;
};

struct ID3D11InfoQueueVtbl {
    IUnknownQueryInterfaceFuncType*          QueryInterface;
    IUnknownAddRefFuncType*                  AddRef;
    IUnknownReleaseFuncType*                 Release;
    void*                                    AddApplicationMessage;
    void*                                    AddMessage;
    void*                                    AddRetrievalFilterEntries;
    void*                                    AddStorageFilterEntries;
    void*                                    ClearRetrievalFilter;
    void*                                    ClearStorageFilter;
    void*                                    ClearStoredMessages;
    void*                                    GetBreakOnCategory;
    void*                                    GetBreakOnID;
    void*                                    GetBreakOnSeverity;
    void*                                    GetMessage;
    void*                                    GetMessageCountLimit;
    void*                                    GetMuteDebugOutput;
    void*                                    GetNumMessagesAllowedByStorageFilter;
    void*                                    GetNumMessagesDeniedByStorageFilter;
    void*                                    GetNumMessagesDiscardedByMessageCountLimit;
    void*                                    GetNumStoredMessages;
    void*                                    GetNumStoredMessagesAllowedByRetrievalFilter;
    void*                                    GetRetrievalFilter;
    void*                                    GetRetrievalFilterStackSize;
    void*                                    GetStorageFilter;
    void*                                    GetStorageFilterStackSize;
    void*                                    PopRetrievalFilter;
    void*                                    PopStorageFilter;
    void*                                    PushCopyOfRetrievalFilter;
    void*                                    PushCopyOfStorageFilter;
    void*                                    PushEmptyRetrievalFilter;
    void*                                    PushEmptyStorageFilter;
    void*                                    PushRetrievalFilter;
    void*                                    PushStorageFilter;
    void*                                    SetBreakOnCategory;
    void*                                    SetBreakOnID;
    DX11InfoQueueSetBreakOnSeverityFuncType* SetBreakOnSeverity;
    void*                                    SetMessageCountLimit;
    void*                                    SetMuteDebugOutput;
};

struct IDXGIOutputVtbl {
    IUnknownQueryInterfaceFuncType* QueryInterface;
    IUnknownAddRefFuncType*         AddRef;
    IUnknownReleaseFuncType*        Release;
    void*                           SetPrivateData;
    void*                           SetPrivateDataInterface;
    void*                           GetPrivateData;
    DXGIObjGetParentFuncType*       GetParent;
    void*                           GetDesc;
    void*                           GetDisplayModeList;
    void*                           FindClosestMatchingMode;
    void*                           WaitForVBlank;
    void*                           TakeOwnership;
    void*                           ReleaseOwnership;
    void*                           GetGammaControlCapabilities;
    void*                           SetGammaControl;
    void*                           GetGammaControl;
    void*                           SetDisplaySurface;
    void*                           GetDisplaySurfaceData;
    void*                           GetFrameStatistics;
};

EXTERN_C_LINK DLL_IMPORT HRESULT WINAPI D3D11CreateDeviceAndSwapChain(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContex);
EXTERN_C_LINK DLL_IMPORT HRESULT WINAPI D3DCompile(void const* pSrcData, SIZE_T SrcDataSize, LPCSTR pSourceName, void const* pDefines, void* pInclude, LPCSTR pEntrypoint, LPCSTR pTarget, UINT Flags1, UINT Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);
EXTERN_C_LINK DLL_IMPORT HRESULT WINAPI D3D11CreateDevice(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, D3D_FEATURE_LEVEL const* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel, ID3D11DeviceContext** ppImmediateContext);

// @runtime____________________________________________________________________
constexpr IID IID_ID3D11Tex2D = {
    0x6F15AAF2,
    0xD208,
    0x4E89,
    {
        0x9A,
        0xB4,
        0x48,
        0x95,
        0x35,
        0xD3,
        0x4F,
        0x9C
    }
};

constexpr IID IID_ID3D11Device1 = {
    0xA04BFB29,
    0x08EF,
    0x43D6,
    {
        0xA4,
        0x9C,
        0xA9,
        0xBD,
        0xBD,
        0xCB,
        0xE6,
        0x86
    }
};

constexpr IID IID_ID3D11DeviceContext1 = {
    0xBB2C6FAA,
    0xB5FB,
    0x4082,
    {
        0x8E,
        0x6B,
        0x38,
        0x8B,
        0x8C,
        0xFA,
        0x90,
        0xE1
    }
};

constexpr IID IID_IDXGIDevice1 = {
    0x77DB970F,
    0x6276,
    0x48BA,
    {
        0xBA,
        0x28,
        0x7,
        0x1,
        0x43,
        0xB4,
        0x39,
        0x2C
    }
};

constexpr IID IID_IDXGIFactory2 = {
    0x50C83A1C,
    0xE072,
    0x4C48,
    {
        0x87,
        0xB0,
        0x36,
        0x30,
        0xFA,
        0x36,
        0xA6,
        0xD0
    }
};

constexpr IID IID_ID3D11InfoQueue = {
    0x6543DBB6,
    0x1B48,
    0x42F5,
    {
        0xAB,
        0x82,
        0xE9,
        0x7E,
        0xC7,
        0x43,
        0x26,
        0xF6
    }
};

constexpr IID IID_ID3D11Debug {
    0x79CF2233, 
    0x7536, 
    0x4948, 
    {
        0x9D, 
        0x36, 
        0x1E, 
        0x46, 
        0x92, 
        0xDC, 
        0x57, 
        0x60 
    }
};

constexpr IID IID_IDXGIDebug {
    0x119E7452, 
    0xDE9E, 
    0x40FE, 
    {
        0x88, 
        0x06, 
        0x88, 
        0xF9, 
        0x0C, 
        0x12, 
        0xB4, 
        0x41
    }
};

constexpr IID IID_IDXGIDebugAll {
    0xE48AE283, 
    0xDA80, 
    0x490B, 
    {
        0x87, 
        0xE6, 
        0x43, 
        0xE9, 
        0xA9, 
        0xCF, 
        0xDA, 
        0x8
    }
};

#endif // __DX11_PLATFORM_H__
