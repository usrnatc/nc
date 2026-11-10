#include "../nc_types.h"
#include "../nc_string.h"
#include "dx11_platform.h"
#include "dx11_render.h"
#include "../nc_render.h"

EXTERN_C_LINK_BEGIN
global readonly Str8 DX11_RECT_SHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "cbuffer Globals : register(b0) {\n"
    "             float2   ViewPortSizePX;\n"
    "             float    Opacity;\n"
    "   row_major float4x4 TextureSampleChannelMap;\n"
    "             float2   TextureT2DSizePX;\n"
    "   row_major float3x3 XForm;\n"
    "             float2   XFormScale;\n"
    "};\n"
    "\n"
    "struct CPU2Vertex {\n"
    "   float4 DstRectPX     : POS;\n"
    "   float4 SrcRectPX     : TEX;\n"
    "   float4 Colour00      : COL0;\n"
    "   float4 Colour01      : COL1;\n"
    "   float4 Colour10      : COL2;\n"
    "   float4 Colour11      : COL3;\n"
    "   float4 CornerRadiiPX : CRAD;\n"
    "   float4 StyleParams   : STY;\n"
    "   uint   VertexID      : SV_VertexID;\n"
    "};\n"
    "\n"
    "struct Vertex2Pixel {\n"
    "                   float4 Position          : SV_POSITION;\n"
    "   nointerpolation float2 RectHalfSizePX    : PSIZE;\n"
    "                   float2 TexCoordPct       : TEX;\n"
    "                   float2 SDFSamplePos      : SDF;\n"
    "                   float4 Tint              : TINT;\n"
    "                   float  CornerRadiusPX    : CRAD;\n"
    "   nointerpolation float  BorderThicknessPX : BTHC;\n"
    "   nointerpolation float  SoftnessPX        : SFT;\n"
    "   nointerpolation float  OmitTexture       : OTX;\n"
    "};\n"
    "\n"
    "Texture2D    MainT2D     : register(t0);\n"
    "SamplerState MainSampler : register(s0);\n"
    "\n"
    "float\n"
    "RectSDF(float2 SamplePos, float2 RectHalfSize, float R)\n"
    "{\n"
    "   return length(max(abs(SamplePos) - RectHalfSize + R, 0.0)) - R;\n"
    "}\n"
    "\n"
    "float\n"
    "LinearFromSRGBF32(float Lin)\n"
    "{\n"
    "   return Lin < 0.0404482362771082 ? Lin / 12.92 : pow((Lin + 0.055) / 1.055, 2.4);\n"
    "}\n"
    "\n"
    "float4\n"
    "LinearFromSRGBA(float4 Lin)\n"
    "{\n"
    "   float4 Result = float4(\n"
    "       LinearFromSRGBF32(Lin.x),\n"
    "       LinearFromSRGBF32(Lin.y),\n"
    "       LinearFromSRGBF32(Lin.z),\n"
    "       Lin.w\n"
    "   );\n"
    "   return Result;\n"
    "}\n"
    "\n"
    "Vertex2Pixel\n"
    "vs_main(CPU2Vertex CPU2Vert)\n"
    "{\n"
    "   float2 DstP0PX = CPU2Vert.DstRectPX.xy;\n"
    "   float2 DstP1PX = CPU2Vert.DstRectPX.zw;\n"
    "   float2 SrcP0PX = CPU2Vert.SrcRectPX.xy;\n"
    "   float2 SrcP1PX = CPU2Vert.SrcRectPX.zw;\n"
    "   float2 DstSizePX = abs(DstP1PX - DstP0PX);\n"
    "   float BorderThicknessPX = CPU2Vert.StyleParams.x;\n"
    "   float SoftnessPX = CPU2Vert.StyleParams.y;\n"
    "   float OmitTexture = CPU2Vert.StyleParams.z;\n"
    "   float ShearPX = CPU2Vert.StyleParams.w;\n"
    "   float2 DstPVertsPX[] = {\n"
    "       float2(DstP0PX.x, DstP1PX.y),\n"
    "       float2(DstP0PX.x, DstP0PX.y),\n"
    "       float2(DstP1PX.x, DstP1PX.y + ShearPX),\n"
    "       float2(DstP1PX.x, DstP0PX.y + ShearPX)\n"
    "   };\n"
    "   float2 SrcPVertsPX[] = {\n"
    "       float2(SrcP0PX.x, SrcP1PX.y),\n"
    "       float2(SrcP0PX.x, SrcP0PX.y),\n"
    "       float2(SrcP1PX.x, SrcP1PX.y),\n"
    "       float2(SrcP1PX.x, SrcP0PX.y)\n"
    "   };\n"
    "   float DstRVertsPX[] = {\n"
    "       CPU2Vert.CornerRadiiPX.y,\n"
    "       CPU2Vert.CornerRadiiPX.x,\n"
    "       CPU2Vert.CornerRadiiPX.w,\n"
    "       CPU2Vert.CornerRadiiPX.z\n"
    "   };\n"
    "   float4 SrcColour[] = {\n"
    "       CPU2Vert.Colour01,\n"
    "       CPU2Vert.Colour00,\n"
    "       CPU2Vert.Colour11,\n"
    "       CPU2Vert.Colour10\n"
    "   };\n"
    "   float2 DstVertsPct = float2(\n"
    "       (CPU2Vert.VertexID >> 1) ? 1.0f : 0.0f,\n"
    "       (CPU2Vert.VertexID & 1) ? 0.0f : 1.0f\n"
    "   );\n"
    "   Vertex2Pixel Vert2Pixel;\n"
    "\n"
    "   {\n"
    "       float2 XFormedPos = mul(XForm, float3(DstPVertsPX[CPU2Vert.VertexID], 1.0f)).xy;\n"
    "\n"
    "       XFormedPos.y = ViewPortSizePX.y - XFormedPos.y;\n"
    "       Vert2Pixel.Position.xy = 2.0f * XFormedPos / ViewPortSizePX - 1.0f;\n"
    "       Vert2Pixel.Position.z = 0.0f;\n"
    "       Vert2Pixel.Position.w = 1.0f;\n"
    "       Vert2Pixel.RectHalfSizePX = DstSizePX / 2.0f * XFormScale;\n"
    "       Vert2Pixel.TexCoordPct = SrcPVertsPX[CPU2Vert.VertexID] / TextureT2DSizePX;\n"
    "       Vert2Pixel.SDFSamplePos = (2.0f * DstVertsPct - 1.0f) * Vert2Pixel.RectHalfSizePX;\n"
    "       Vert2Pixel.Tint = SrcColour[CPU2Vert.VertexID];\n"
    "       Vert2Pixel.CornerRadiusPX = DstRVertsPX[CPU2Vert.VertexID];\n"
    "       Vert2Pixel.BorderThicknessPX = BorderThicknessPX;\n"
    "       Vert2Pixel.SoftnessPX = SoftnessPX;\n"
    "       Vert2Pixel.OmitTexture = OmitTexture;\n"
    "   }\n"
    "\n"
    "   return Vert2Pixel;\n"
    "}\n"
    "\n"
    "float4\n"
    "ps_main(Vertex2Pixel Vert2Pixel) : SV_TARGET\n"
    "{\n"
    "   float4 Tint = Vert2Pixel.Tint;\n"
    "   float4 AlbedoSample = float4(1, 1, 1, 1);\n"
    "\n"
    "   if (Vert2Pixel.OmitTexture < 1) {\n"
    "       AlbedoSample = mul(MainT2D.Sample(MainSampler, Vert2Pixel.TexCoordPct), TextureSampleChannelMap);\n"
    "       AlbedoSample = LinearFromSRGBA(AlbedoSample);\n"
    "   }\n"
    "\n"
    "   float2 SDFSamplePos = Vert2Pixel.SDFSamplePos;\n"
    "   float BorderSDFT = 1;\n"
    "\n"
    "   if (Vert2Pixel.BorderThicknessPX > 0) {\n"
    "       float BorderSDFS = RectSDF(\n"
    "           SDFSamplePos,"
    "           Vert2Pixel.RectHalfSizePX - float2(Vert2Pixel.SoftnessPX * 2.0f, Vert2Pixel.SoftnessPX * 2.0f) - Vert2Pixel.BorderThicknessPX,\n"
    "           max(Vert2Pixel.CornerRadiusPX - Vert2Pixel.BorderThicknessPX, 0)\n"
    "       );\n"
    "\n"
    "       BorderSDFT = smoothstep(0, 2 * Vert2Pixel.SoftnessPX, BorderSDFS);\n"
    "   }\n"
    "\n"
    "   if (BorderSDFT < 0.001f)\n"
    "       discard;\n"
    "\n"
    "   float CornerSDFT = 1;\n"
    "\n"
    "   if (Vert2Pixel.CornerRadiusPX > 0 || Vert2Pixel.SoftnessPX > 0.75f) {\n"
    "       float CornerSDFS = RectSDF("
    "           SDFSamplePos,\n"
    "           Vert2Pixel.RectHalfSizePX - float2(Vert2Pixel.SoftnessPX * 2.0f, Vert2Pixel.SoftnessPX * 2.0f),\n"
    "           Vert2Pixel.CornerRadiusPX"
    "       );\n"
    "\n"
    "       CornerSDFT = 1 - smoothstep(0, 2 * Vert2Pixel.SoftnessPX, CornerSDFS);\n"
    "   }\n"
    "\n"
    "   float4 FinalColour = AlbedoSample;\n"
    "\n"
    "   FinalColour *= Tint;\n"
    "   FinalColour.a *= Opacity;\n"
    "   FinalColour.a *= CornerSDFT;\n"
    "   FinalColour.a *= BorderSDFT;\n"
    "\n"
    "   return FinalColour;\n"
    "}\n"
);

global readonly Str8 DX11_BLUR_SHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "cbuffer Globals : register(b0) {\n"
    "   float4 Rect;\n"
    "   float4 CornerRadiiPX;\n"
    "   float2 Direction;\n"
    "   float2 ViewportSize;\n"
    "   uint   BlurCount;\n"
    "}\n"
    "\n"
    "cbuffer Kernel : register(b1) {\n"
    "   float4 Kernel[32];\n"
    "}\n"
    "\n"
    "struct CPU2Vertex {\n"
    "   uint VertexID : SV_VertexID;\n"
    "};\n"
    "\n"
    "struct Vertex2Pixel {\n"
    "                   float4 Position     : SV_POSITION;\n"
    "                   float2 TexCoord     : TEX;\n"
    "                   float2 SDFSamplePos : SDF;\n"
    "   nointerpolation float2 RectHalfSize : RHS;\n"
    "                   float  CornerRadius : RAD;\n"
    "};\n"
    "\n"
    "Texture2D    StageT2D     : register(t0);\n"
    "SamplerState StageSampler : register(s0);\n"
    "\n"
    "float\n"
    "RectSDF(float2 SamplePos, float2 RectHalfSize, float R)\n"
    "{\n"
    "   return length(max(abs(SamplePos) - RectHalfSize + R, 0.0)) - R;\n"
    "}\n"
    "\n"
    "// vertex shader\n"
    "\n"
    "Vertex2Pixel\n"
    "vs_main(CPU2Vertex C2V)\n"
    "{\n"
    "   float2 VertexPositionsScrn[] = {\n"
    "       Rect.xw,\n"
    "       Rect.xy,\n"
    "       Rect.zw,\n"
    "       Rect.zy\n"
    "   };\n"
    "   float CornerRadiiPXArr[] = {\n"
    "       CornerRadiiPX.y,\n"
    "       CornerRadiiPX.x,\n"
    "       CornerRadiiPX.w,\n"
    "       CornerRadiiPX.z\n"
    "   };\n"
    "   float2 CornerCoordsPct = float2(\n"
    "       (C2V.VertexID >> 1) ? 1.0f : 0.0f,\n"
    "       (C2V.VertexID & 1)  ? 0.0f : 1.0f\n"
    "   );\n"
    "   float2 VertexPositionPct = VertexPositionsScrn[C2V.VertexID] / ViewportSize;\n"
    "   float2 VertexPositionScr = 2.0f * VertexPositionPct - 1.0f;\n"
    "   float2 RectHalfSizeVal = float2((Rect.z - Rect.x) / 2, (Rect.w - Rect.y) / 2);\n"
    "   Vertex2Pixel V2P;\n"
    "\n"
    "   {\n"
    "       V2P.Position = float4(VertexPositionScr.x, -VertexPositionScr.y, 0.0f, 1.0f);\n"
    "       V2P.TexCoord = VertexPositionPct;\n"
    "       V2P.SDFSamplePos = (2.0f * CornerCoordsPct - 1.0f) * RectHalfSizeVal;\n"
    "       V2P.RectHalfSize = RectHalfSizeVal - 2.0f;\n"
    "       V2P.CornerRadius = CornerRadiiPXArr[C2V.VertexID];\n"
    "   }\n"
    "\n"
    "   return V2P;\n"
    "}\n"
    "\n"
    "// pixel shader\n"
    "\n"
    "float4\n"
    "ps_main(Vertex2Pixel V2P) : SV_TARGET\n"
    "{\n"
    "   // blend weighted texture samples into color\n"
    "   float3 Colour = Kernel[0].x * StageT2D.Sample(StageSampler, V2P.TexCoord).rgb;\n"
    "\n"
    "   for (uint I = 1; I < BlurCount; I += 1) {\n"
    "       float Weight = Kernel[I].x;\n"
    "       float Offset = Kernel[I].y;\n"
    "\n"
    "       Colour += Weight * StageT2D.Sample(StageSampler, V2P.TexCoord - Offset * Direction).rgb;\n"
    "       Colour += Weight * StageT2D.Sample(StageSampler, V2P.TexCoord + Offset * Direction).rgb;\n"
    "   }\n"
    "\n"
    "   // sample for corners\n"
    "   float CornerSDFS = RectSDF(V2P.SDFSamplePos, V2P.RectHalfSize, V2P.CornerRadius);\n"
    "   float CornerSDFT = 1 - smoothstep(0, 2, CornerSDFS);\n"
    "\n"
    "   // weight output color by sdf\n"
    "   // this is doing alpha testing, leave blurring only where mostly opaque pixels are\n"
    "   if (CornerSDFT < 0.9f)\n"
    "       discard;\n"
    "\n"
    "   return float4(Colour, 1.0f);\n"
    "}\n"
);

global readonly Str8 DX11_MESH_SHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "cbuffer Uniforms : register(b0) {\n"
    "   row_major float4x4 XForm;\n"
    "}\n"
    "\n"
    "struct CPU2Vertex {\n"
    "   float3 Position : POS;\n"
    "   float3 Normal   : NOR;\n"
    "   float2 TexCoord : TEX;\n"
    "   float3 Colour   : COL;\n"
    "};\n"
    "\n"
    "struct Vertex2Pixel {\n"
    "   float4 Position : SV_POSITION;\n"
    "   float2 TexCoord : TEX;\n"
    "   float4 Colour   : COL;\n"
    "};\n"
    "\n"
    "Vertex2Pixel\n"
    "vs_main(CPU2Vertex C2V)\n"
    "{\n"
    "   Vertex2Pixel V2P;\n"
    "\n"
    "   V2P.Position = mul(float4(C2V.Position, 1.0f), XForm);\n"
    "   V2P.TexCoord = C2V.TexCoord;\n"
    "   V2P.Colour   = float4(C2V.Colour, 1.0f);\n"
    "\n"
    "   return V2P;\n"
    "}\n"
    "\n"
    "float4\n"
    "ps_main(Vertex2Pixel V2P) : SV_TARGET\n"
    "{\n"
    "   return V2P.Colour;\n"
    "}\n"
);

global readonly Str8 DX11_GEO3D_COMPOSITE_SHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "struct CPU2Vertex {\n"
    "   uint VertexID : SV_VertexID;\n"
    "};\n"
    "\n"
    "struct Vertex2Pixel {\n"
    "   float4 Position : SV_POSITION;\n"
    "   float2 TexCoord : TEX;\n"
    "};\n"
    "\n"
    "Texture2D    StageT2D     : register(t0);\n"
    "SamplerState StageSampler : register(s0);\n"
    "\n"
    "// vertex shader\n"
    "\n"
    "Vertex2Pixel\n"
    "vs_main(CPU2Vertex C2V)\n"
    "{\n"
    "   float4 VertexPositionsModl[] = {\n"
    "       float4(0, 0, 0, 1),\n"
    "       float4(0, 1, 0, 1),\n"
    "       float4(1, 0, 0, 1),\n"
    "       float4(1, 1, 0, 1)\n"
    "   };\n"
    "   float4 VertexPositionModl = VertexPositionsModl[C2V.VertexID];\n"
    "   float4 VertexPositionClip = float4(2 * VertexPositionModl.x - 1, 2 * VertexPositionModl.y - 1, 0, 1);\n"
    "   float2 TexCoordVal = float2(VertexPositionModl.x, VertexPositionModl.y);\n"
    "\n"
    "   TexCoordVal.y = 1 - TexCoordVal.y;\n"
    "\n"
    "   Vertex2Pixel V2P;\n"
    "\n"
    "   {\n"
    "       V2P.Position = VertexPositionClip;\n"
    "       V2P.TexCoord = TexCoordVal;\n"
    "   }\n"
    "\n"
    "   return V2P;\n"
    "}\n"
    "\n"
    "// pixel shader\n"
    "\n"
    "float4\n"
    "ps_main(Vertex2Pixel V2P) : SV_TARGET\n"
    "{\n"
    "   float4 FinalColour = StageT2D.Sample(StageSampler, V2P.TexCoord);\n"
    "\n"
    "   return FinalColour;\n"
    "}\n"
);

global readonly Str8 DX11_FINALISER_SHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "struct CPU2Vertex {\n"
    "   uint VertexID : SV_VertexID;\n"
    "};\n"
    "\n"
    "struct Vertex2Pixel {\n"
    "   float4 Position : SV_POSITION;\n"
    "   float2 TexCoord : TEX;\n"
    "};\n"
    "\n"
    "Texture2D    StageT2D     : register(t0);\n"
    "SamplerState StageSampler : register(s0);\n"
    "\n"
    "// vertex shader\n"
    "\n"
    "Vertex2Pixel\n"
    "vs_main(CPU2Vertex C2V)\n"
    "{\n"
    "   float4 VertexPositionsModl[] = {\n"
    "       float4(0, 0, 0, 1),\n"
    "       float4(0, 1, 0, 1),\n"
    "       float4(1, 0, 0, 1),\n"
    "       float4(1, 1, 0, 1)\n"
    "   };\n"
    "   float4 VertexPositionModl = VertexPositionsModl[C2V.VertexID];\n"
    "   float4 VertexPositionClip = float4(2 * VertexPositionModl.x - 1, 2 * VertexPositionModl.y - 1, 0, 1);\n"
    "   float2 TexCoordVal = float2(VertexPositionModl.x, VertexPositionModl.y);\n"
    "\n"
    "   TexCoordVal.y = 1 - TexCoordVal.y;\n"
    "\n"
    "   Vertex2Pixel V2P;\n"
    "\n"
    "   {\n"
    "       V2P.Position = VertexPositionClip;\n"
    "       V2P.TexCoord = TexCoordVal;\n"
    "   }\n"
    "\n"
    "   return V2P;\n"
    "}\n"
    "\n"
    "// pixel shader\n"
    "\n"
    "float4\n"
    "ps_main(Vertex2Pixel V2P) : SV_TARGET\n"
    "{\n"
    "   float4 FinalColour = StageT2D.Sample(StageSampler, V2P.TexCoord);\n"
    "\n"
    "   FinalColour.a = 1;\n"
    "\n"
    "   return FinalColour;\n"
    "}\n"
);
EXTERN_C_LINK_END

global D3D11_INPUT_ELEMENT_DESC DX11_RECT_INPUT_LAYOUTS[] = {
    {"POS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,                            0, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"TEX",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"COL",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"COL",  1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"COL",  2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"COL",  3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"CRAD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1},
    {"STY",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_INSTANCE_DATA, 1}
};

global D3D11_INPUT_ELEMENT_DESC DX11_MESH_INPUT_LAYOUTS[] = {
    {"POS",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,                            D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"NOR",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"TEX",  0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0},
    {"COL",  0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,   0}
};

EXTERN_C_LINK_BEGIN
Str8* DX11_VERT_SHADER_SOURCES[VERT_SHADER_COUNT] = {
    &DX11_RECT_SHADER_SOURCE,
    &DX11_BLUR_SHADER_SOURCE,
    &DX11_MESH_SHADER_SOURCE,
    &DX11_GEO3D_COMPOSITE_SHADER_SOURCE,
    &DX11_FINALISER_SHADER_SOURCE
};

Str8 DX11_VERT_SHADER_NAMES[VERT_SHADER_COUNT] = {
    Str8Lit("DX11_RECT_SHADER_SOURCE"),
    Str8Lit("DX11_BLUR_SHADER_SOURCE"),
    Str8Lit("DX11_MESH_SHADER_SOURCE"),
    Str8Lit("DX11_GEO3D_COMPOSITE_SHADER_SOURCE"),
    Str8Lit("DX11_FINALISER_SHADER_SOURCE")
};

D3D11_INPUT_ELEMENT_DESC* DX11_VERT_SHADER_INPUT_LAYOUTS[VERT_SHADER_COUNT] = {
    DX11_RECT_INPUT_LAYOUTS,
    NULL,
    DX11_MESH_INPUT_LAYOUTS,
    NULL,
    NULL
};

u64 DX11_VERT_SHADER_INPUT_LAYOUT_SIZES[VERT_SHADER_COUNT] = {
    ARRAY_COUNT(DX11_RECT_INPUT_LAYOUTS),
    0,
    ARRAY_COUNT(DX11_MESH_INPUT_LAYOUTS),
    0,
    0
};

Str8* DX11_PIXEL_SHADER_SOURCES[PIXEL_SHADER_COUNT] = {
    &DX11_RECT_SHADER_SOURCE,
    &DX11_BLUR_SHADER_SOURCE,
    &DX11_MESH_SHADER_SOURCE,
    &DX11_GEO3D_COMPOSITE_SHADER_SOURCE,
    &DX11_FINALISER_SHADER_SOURCE
};

Str8 DX11_PIXEL_SHADER_NAMES[PIXEL_SHADER_COUNT] = {
    Str8Lit("DX11_RECT_SHADER_SOURCE"),
    Str8Lit("DX11_BLUR_SHADER_SOURCE"),
    Str8Lit("DX11_MESH_SHADER_SOURCE"),
    Str8Lit("DX11_GEO3D_COMPOSITE_SHADER_SOURCE"),
    Str8Lit("DX11_FINALISER_SHADER_SOURCE")
};

u64 DX11_UNIFORM_SIZES[UNIFORM_KIND_COUNT] = {
    sizeof(DX11UniformsRect),
    sizeof(DX11UniformsBlur),
    sizeof(DX11UniformsMesh)
};
EXTERN_C_LINK_END

DX11Window* 
DX11WindowFromRendHandle(RendHandle Handle)
{
    DX11Window* Result = (DX11Window*) PTR_FROM_INT(*Handle.U64);

    if (!Result)
        Result = &DX11_EMPTY_WINDOW_VALUE;

    return Result;
}

DX11Tex2D* 
DX11Tex2DFromRendHandle(RendHandle Handle)
{
    DX11Tex2D* Result = (DX11Tex2D*) PTR_FROM_INT(*Handle.U64);

    if (!Result)
        Result = &DX11_EMPTY_TEX2D_VALUE;

    return Result;
}

DX11Buffer* 
DX11BufferFromRendHandle(RendHandle Handle)
{
    DX11Buffer* Result = (DX11Buffer*) PTR_FROM_INT(*Handle.U64);

    if (!Result)
        Result = &DX11_EMPTY_BUFFER_VALUE;

    return Result;

}

RendHandle 
RendHandleFromDX11Window(DX11Window* Window)
{
    RendHandle Result = {
        INT_FROM_PTR(Window)
    };

    return Result;
}

DX11Window*
DX11WindowFromHandle(Handle WindowHandle)
{
    DX11Window* Window = (DX11Window*) PTR_FROM_INT(*WindowHandle.V);

    if (!Window)
        Window = &DX11_EMPTY_WINDOW_VALUE;

    return Window;
}

RendHandle 
RendHandleFromDX11Tex2D(DX11Tex2D* Texture)
{
    RendHandle Result = {
        INT_FROM_PTR(Texture)
    };

    return Result;
}

RendHandle 
RendHandleFromDX11Buffer(DX11Buffer* Buffer)
{
    RendHandle Result = {
        INT_FROM_PTR(Buffer)
    };

    return Result;
}

ID3D11Buffer* 
DX11BufferFromSize(u64 Size)
{
    ID3D11Buffer* Result = DX11_STATE->InstanceScratchBuffer64KB;

    if (Size > KB(64)) {
        u64 FlushedBufferSize = Size;

        FlushedBufferSize += MB(1) - 1;
        FlushedBufferSize -= FlushedBufferSize % MB(1);

        D3D11_BUFFER_DESC Description = {};

        Description.ByteWidth = (UINT) FlushedBufferSize;
        Description.Usage = D3D11_USAGE_DYNAMIC;
        Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        HRESULT HRes = DX11_STATE->Device->Vtbl->CreateBuffer(
            DX11_STATE->Device,
            &Description,
            NULL,
            &Result
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create Buffer of size %llu", Size);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create Direct3D 11 Buffer"_s8
            );

            return NULL;
        }

        DX11FlushBuffer* Node = ArenaPushArrayZero(
            DX11_STATE->FlushBufferMemPool, 
            DX11FlushBuffer, 
            1
        );

        Node->Buffer = Result;
        SLL_QUEUE_PUSH(
            DX11_STATE->HeadFlushBuffer, 
            DX11_STATE->TailFlushBuffer, 
            Node
        );
    }

    return Result;
}

void 
UsageAccessFlagsFromResourceKind(RendResourceKind Kind, OUT D3D11_USAGE* Usage, OUT UINT* CPUAccessFlags)
{
    switch (Kind) {
        default: {
            UNREACHABLE();
        } break;

        case RESOURCE_KIND_STATIC: {
            *Usage = D3D11_USAGE_IMMUTABLE;
            *CPUAccessFlags = 0;
        } break;

        case RESOURCE_KIND_DYNAMIC: {
            *Usage = D3D11_USAGE_DEFAULT;
            *CPUAccessFlags = 0;
        } break;

        case RESOURCE_KIND_STREAM: {
            *Usage = D3D11_USAGE_DYNAMIC;
            *CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        } break;
    }
}

EXTERN_C_LINK_BEGIN
void 
RendInit(CommandLine* CLI)
{
    HRESULT HRes = 0;
    Arena* MemPool = ArenaAlloc();

    DX11_STATE = ArenaPushArrayZero(MemPool, DX11State, 1);
    DX11_STATE->MemPool = MemPool;
    DX11_STATE->RWMtx = RWMutexAlloc();

    UINT CreationFlags = (
        D3D11_CREATE_DEVICE_BGRA_SUPPORT
    );

#if defined(NC_DEBUG)
    CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL FeatureLevels[] = {
        D3D_FEATURE_LEVEL_11_0
    };

    HRes = D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        CreationFlags,
        FeatureLevels,
        ARRAY_COUNT(FeatureLevels),
        D3D11_SDK_VERSION,
        &DX11_STATE->BaseDevice,
        NULL,
        &DX11_STATE->BaseDeviceContext
    );

    if (FAILED(HRes)) {
        LogWarn("failed to create device with hardware backend, trying warp"_s8);
        HRes = D3D11CreateDevice(
            NULL,
            D3D_DRIVER_TYPE_WARP,
            NULL,
            CreationFlags,
            FeatureLevels,
            ARRAY_COUNT(FeatureLevels),
            D3D11_SDK_VERSION,
            &DX11_STATE->BaseDevice,
            NULL,
            &DX11_STATE->BaseDeviceContext
        );
    }

    if (FAILED(HRes)) {
        LogPanic("Direct3D 11 device creation failure (%lu)", HRes);
        GraphicalMessage(TRUE, "Fatal Error"_s8, "Direct3D 11 device creation failure"_s8);
        Abort(1);
    }

#if defined(NC_DEBUG)
    ID3D11InfoQueue* Info = NULL;

    HRes = DX11_STATE->BaseDevice->Vtbl->QueryInterface(
        DX11_STATE->BaseDevice,
        IID_ID3D11InfoQueue,
        (void**) &Info
    );

    if (SUCCEEDED(HRes)) {
        HRes = Info->Vtbl->SetBreakOnSeverity(
            Info,
            D3D11_MESSAGE_SEVERITY_CORRUPTION,
            TRUE
        );

        HRes = Info->Vtbl->SetBreakOnSeverity(
            Info,
            D3D11_MESSAGE_SEVERITY_ERROR,
            TRUE
        );

        Info->Vtbl->Release(Info);
    }
#endif

    HRes = DX11_STATE->BaseDevice->Vtbl->QueryInterface(
        DX11_STATE->BaseDevice,
        IID_ID3D11Device1,
        (void**) &DX11_STATE->Device
    );

    if (FAILED(HRes)) {
        LogPanic("unable to query D3D11 Device1"_s8);
        GraphicalMessage(TRUE, "Fatal Exception"_s8, "Unable to query Direct3D 11 Device1"_s8);
        Abort(1);
    }

    HRes = DX11_STATE->BaseDeviceContext->Vtbl->QueryInterface(
        DX11_STATE->BaseDeviceContext,
        IID_ID3D11DeviceContext1,
        (void**) &DX11_STATE->DeviceContext
    );

    if (FAILED(HRes)) {
        LogPanic("unable to query D3D11 DeviceContext1"_s8);
        GraphicalMessage(TRUE, "Fatal Exception"_s8, "Unable to query Direct3D 11 DeviceContext1"_s8);
        Abort(1);
    }

    HRes = DX11_STATE->Device->Vtbl->QueryInterface(
        DX11_STATE->Device,
        IID_IDXGIDevice1,
        (void**) &DX11_STATE->DXGIDevice
    );

    if (FAILED(HRes)) {
        LogPanic("unable to query D3D11 DXGIDevice"_s8);
        GraphicalMessage(TRUE, "Fatal Exception"_s8, "Unable to query Direct3D 11 DXGIDevice"_s8);
        Abort(1);
    }

    HRes = DX11_STATE->DXGIDevice->Vtbl->GetAdapter(
        DX11_STATE->DXGIDevice,
        &DX11_STATE->DXGIAdapter
    );

    if (FAILED(HRes)) {
        LogPanic("unable to query D3D11 DXGIAdapter"_s8);
        GraphicalMessage(TRUE, "Fatal Exception"_s8, "Unable to query Direct3D 11 DXGIAdapter"_s8);
        Abort(1);
    }

    HRes = DX11_STATE->DXGIAdapter->Vtbl->GetParent(
        DX11_STATE->DXGIAdapter,
        IID_IDXGIFactory2,
        (void**) &DX11_STATE->DXGIFactory
    );

    if (FAILED(HRes)) {
        LogPanic("unable to query D3D11 DXGIFactory"_s8);
        GraphicalMessage(TRUE, "Fatal Exception"_s8, "Unable to query Direct3D 11 DXGIFactory"_s8);
        Abort(1);
    }

    HRes = DX11_STATE->DXGIDevice->Vtbl->SetMaximumFrameLatency(
        DX11_STATE->DXGIDevice,
        1
    );

    if (FAILED(HRes)) {
        LogPanic("unable to set D3D11 maximum frame latency"_s8);
        GraphicalMessage(TRUE, "Fatal Exception"_s8, "Unable to set Direct3D 11 maximum frame latency"_s8);
        Abort(1);
    }

    {
        D3D11_RASTERIZER_DESC1 Description = {};

        Description.FillMode = D3D11_FILL_SOLID;
        Description.CullMode = D3D11_CULL_BACK;
        Description.MultisampleEnable = FALSE;
        Description.FrontCounterClockwise = FALSE;
        Description.DepthClipEnable = FALSE;
        Description.ScissorEnable = TRUE;
        HRes = DX11_STATE->Device->Vtbl->CreateRasterizerState1(
            DX11_STATE->Device,
            &Description,
            &DX11_STATE->MainRasteriser
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create RasterizerState1 for MainRasteriser"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create RasterizerState1 for MainRasteriser"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): main blend state
        D3D11_BLEND_DESC Description = {};

        Description.RenderTarget[0].BlendEnable = TRUE;
        Description.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        Description.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        Description.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        Description.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        Description.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        Description.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        Description.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRes = DX11_STATE->Device->Vtbl->CreateBlendState(
            DX11_STATE->Device, 
            &Description, 
            &DX11_STATE->MainBlendState
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create BlendState for MainBlendState"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create BlendState for MainBlendState"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): empty blend state
        D3D11_BLEND_DESC Description = {};

        Description.RenderTarget[0].BlendEnable = FALSE;
        Description.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        HRes = DX11_STATE->Device->Vtbl->CreateBlendState(
            DX11_STATE->Device,
            &Description, 
            &DX11_STATE->NoBlendState
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create BlendState for NoBlendState"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create BlendState for NoBlendState"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): nearest-neighbour sampler
        D3D11_SAMPLER_DESC Description = {};

        Description.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        Description.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.ComparisonFunc = D3D11_COMPARISON_NEVER;
        HRes = DX11_STATE->Device->Vtbl->CreateSamplerState(
            DX11_STATE->Device,
            &Description, 
            &DX11_STATE->Samplers[TEX2D_SAMPLE_NEAREST]
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create SamplerState for NearestNeighbourSampler"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create SamplerState for NearestNeighbourSampler"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): bilinear sampler
        D3D11_SAMPLER_DESC Description = {};

        Description.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        Description.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
        Description.ComparisonFunc = D3D11_COMPARISON_NEVER;
        HRes = DX11_STATE->Device->Vtbl->CreateSamplerState(
            DX11_STATE->Device,
            &Description, 
            &DX11_STATE->Samplers[TEX2D_SAMPLE_LINEAR]
        );
        if (FAILED(HRes)) {
            LogPanic("unable to create SamplerState for BiLinearSampler"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create SamplerState for BiLinearSampler"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): no-op depth / stencil state
        D3D11_DEPTH_STENCIL_DESC Description = {};

        Description.DepthEnable = FALSE;
        Description.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        Description.DepthFunc = D3D11_COMPARISON_LESS;
        HRes = DX11_STATE->Device->Vtbl->CreateDepthStencilState(
            DX11_STATE->Device,
            &Description, 
            &DX11_STATE->NoOpDepthStencil
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create DepthStencilState for NoOpDepthStencil"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create DepthStencilState for NoOpDepthStencil"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): plain depth / stencil state
        D3D11_DEPTH_STENCIL_DESC Description = {};

        Description.DepthEnable = TRUE;
        Description.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        Description.DepthFunc = D3D11_COMPARISON_LESS;
        HRes = DX11_STATE->Device->Vtbl->CreateDepthStencilState(
            DX11_STATE->Device,
            &Description, 
            &DX11_STATE->PlainDepthStencil
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create DepthStencilState for PlainDepthStencil"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create DepthStencilState for PlainDepthStencil"_s8
            );
            Abort(1);
        }
    }

    {   // NOTE(nathan): create buffers
        D3D11_BUFFER_DESC Description = {};

        Description.ByteWidth = KB(64);
        Description.Usage = D3D11_USAGE_DYNAMIC;
        Description.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRes = DX11_STATE->Device->Vtbl->CreateBuffer(
            DX11_STATE->Device,
            &Description, 
            NULL, 
            &DX11_STATE->InstanceScratchBuffer64KB
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create Buffer for InstanceScratchBuffer64KB"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create Buffer for InstanceScratchBuffer64KB"_s8
            );
            Abort(1);
        }
    }

    for (
        DX11VertShaderKind Kind = (DX11VertShaderKind) 0; 
        Kind < VERT_SHADER_COUNT; 
        Kind = (DX11VertShaderKind) (Kind + 1)
    ) {
        Str8 Source = *DX11_VERT_SHADER_SOURCES[Kind];
        Str8 SourceName = DX11_VERT_SHADER_NAMES[Kind];
        D3D11_INPUT_ELEMENT_DESC* ILayElements = DX11_VERT_SHADER_INPUT_LAYOUTS[Kind];
        u64 ILayElementsCount = DX11_VERT_SHADER_INPUT_LAYOUT_SIZES[Kind];
        ID3DBlob* VShaderSourceBlob = NULL;
        ID3DBlob* VShaderSourceErrors = NULL;
        UINT CompileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

    #if defined(NC_DEBUG)
        CompileFlags |= (D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION);
    #endif
        
        HRes = D3DCompile(
            Source.Str,
            Source.Size,
            (char*) SourceName.Str,
            NULL,
            NULL,
            "vs_main",
            "vs_5_0",
            0,
            0,
            &VShaderSourceBlob,
            &VShaderSourceErrors
        );

        if (FAILED(HRes) || !VShaderSourceBlob) {
            Str8 Errors = Str(
                (u8*) VShaderSourceErrors->Vtbl->GetBufferPointer(VShaderSourceErrors), 
                (u64) VShaderSourceErrors->Vtbl->GetBufferSize(VShaderSourceErrors)
            );

            GraphicalMessage(TRUE, Str8Lit("Vertex Shader Compilation Failure"), Errors);
            LogPanic("Failed to compile vertex shader %S", PRINT_STR(SourceName));
            LogPanic("%S", PRINT_STR(Errors));
            Abort(1);
        }

        ID3D11VertexShader* VShader = NULL;

        HRes = DX11_STATE->Device->Vtbl->CreateVertexShader(
            DX11_STATE->Device,
            VShaderSourceBlob->Vtbl->GetBufferPointer(VShaderSourceBlob), 
            VShaderSourceBlob->Vtbl->GetBufferSize(VShaderSourceBlob), 
            NULL, 
            &VShader
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create VertexShader for VShader"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create VertexShader for VShader"_s8
            );
            Abort(1);
        }

        ID3D11InputLayout* ILay = NULL;

        if (ILayElements) {
            HRes = DX11_STATE->Device->Vtbl->CreateInputLayout(
                DX11_STATE->Device,
                ILayElements,
                (UINT) ILayElementsCount,
                VShaderSourceBlob->Vtbl->GetBufferPointer(VShaderSourceBlob),
                VShaderSourceBlob->Vtbl->GetBufferSize(VShaderSourceBlob),
                &ILay
            );

            if (FAILED(HRes)) {
                LogPanic("unable to create InputLayout for ILay"_s8);
                GraphicalMessage(
                    TRUE, 
                    "Fatal Exception"_s8, 
                    "Unable to create InputLayout for ILay"_s8
                );
                Abort(1);
            }
        }

        VShaderSourceBlob->Vtbl->Release(VShaderSourceBlob);

        if (VShaderSourceErrors) {
            VShaderSourceErrors->Vtbl->Release(VShaderSourceErrors);
        }

        DX11_STATE->VertShaders[Kind] = VShader;
        DX11_STATE->InputLayouts[Kind] = ILay;
    }

    for (
        DX11PixelShaderKind Kind = (DX11PixelShaderKind) 0; 
        Kind < PIXEL_SHADER_COUNT; 
        Kind = (DX11PixelShaderKind) (Kind + 1)
    ) {
        Str8 Source = *DX11_PIXEL_SHADER_SOURCES[Kind];
        Str8 SourceName = DX11_PIXEL_SHADER_NAMES[Kind];
        ID3DBlob* PShaderSourceBlob = NULL;
        ID3DBlob* PShaderSourceErrors = NULL;

        HRes = D3DCompile(
            Source.Str,
            Source.Size,
            (char*) SourceName.Str,
            NULL,
            NULL,
            "ps_main",
            "ps_5_0",
            0,
            0,
            &PShaderSourceBlob,
            &PShaderSourceErrors
        );

        if (FAILED(HRes) || !PShaderSourceBlob) {
            Str8 Errors = Str(
                (u8*) PShaderSourceErrors->Vtbl->GetBufferPointer(PShaderSourceErrors),
                (u64) PShaderSourceErrors->Vtbl->GetBufferSize(PShaderSourceErrors)
            );
            GraphicalMessage(TRUE, Str8Lit("Pixel Shader Compilation Failure"), Errors);
            LogPanic("Failed to compile pixel shader %S", PRINT_STR(SourceName));
            LogPanic("%S", PRINT_STR(Errors));
            Abort(1);
        }

        ID3D11PixelShader* PShader = NULL;

        HRes = DX11_STATE->Device->Vtbl->CreatePixelShader(
            DX11_STATE->Device,
            PShaderSourceBlob->Vtbl->GetBufferPointer(PShaderSourceBlob),
            PShaderSourceBlob->Vtbl->GetBufferSize(PShaderSourceBlob), 
            NULL, 
            &PShader
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create PixelShader for PShader"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create PixelShader for PShader"_s8
            );
            Abort(1);
        }

        PShaderSourceBlob->Vtbl->Release(PShaderSourceBlob);

        if (PShaderSourceErrors) {
            PShaderSourceErrors->Vtbl->Release(PShaderSourceErrors);
        }

        DX11_STATE->PixelShaders[Kind] = PShader;
    }

    for (
        DX11UniformKind Kind = (DX11UniformKind) 0; 
        Kind < UNIFORM_KIND_COUNT; 
        Kind = (DX11UniformKind) (Kind + 1)
    ) {
        ID3D11Buffer* Buffer = NULL;
        D3D11_BUFFER_DESC Description = {};

        Description.ByteWidth = (UINT) DX11_UNIFORM_SIZES[Kind];
        Description.ByteWidth += 15;
        Description.ByteWidth -= Description.ByteWidth % 16;
        Description.Usage = D3D11_USAGE_DYNAMIC;
        Description.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        Description.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        HRes = DX11_STATE->Device->Vtbl->CreateBuffer(
            DX11_STATE->Device,
            &Description, 
            NULL, 
            &Buffer
        );

        if (FAILED(HRes)) {
            LogPanic("unable to create Buffer for Uniform Buffer"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to create Buffer for Uniform Buffer"_s8
            );
            Abort(1);
        }

        DX11_STATE->UniformBuffers[Kind] = Buffer;
    }

    u32 BackupTextureData[] = {
        0xFF00FFFF, 0x330033FF,
        0x330033FF, 0xFF00FFFF
    };

    DX11_STATE->BackupTexture = Tex2DAlloc(
        RESOURCE_KIND_STATIC,
        Vec((i32) 2, (i32) 2),
        TEX2D_FMT_RGBA8,
        BackupTextureData
    );
    DX11_STATE->FlushBufferMemPool = ArenaAlloc();
}

RendHandle 
RendEquipWindow(Handle WindowHandle)
{
    RendHandle Result = {};

    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        DX11Window* RendWindow = DX11_STATE->FreeWindow;

        if (!RendWindow) {
            RendWindow = ArenaPushArrayZero(DX11_STATE->MemPool, DX11Window, 1);
        } else {
            u64 Generation = RendWindow->Generation;

            SLL_STACK_POP(DX11_STATE->FreeWindow);
            MemSet(RendWindow, 0, sizeof(*RendWindow));
            RendWindow->Generation = Generation;
        }

        ++RendWindow->Generation;

        HWND Win32WindowHandle = NULL;
        WinWindow* Win32Window = WindowFromHandle(WindowHandle);

        Win32WindowHandle = HWNDFromWindow(Win32Window);

        DXGI_SWAP_CHAIN_DESC1 SwapChainDescription = {};

        SwapChainDescription.Width = 0;
        SwapChainDescription.Height = 0;
        SwapChainDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        SwapChainDescription.Stereo = FALSE;
        SwapChainDescription.SampleDesc.Count = 1;
        SwapChainDescription.SampleDesc.Quality = 0;
        SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        SwapChainDescription.BufferCount = 2;
        SwapChainDescription.Scaling = DXGI_SCALING_NONE;
        SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        SwapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        SwapChainDescription.Flags = 0;

        HRESULT HRes = DX11_STATE->DXGIFactory->Vtbl->CreateSwapChainForHwnd(
            DX11_STATE->DXGIFactory,
            (IUnknown*) DX11_STATE->Device,
            Win32WindowHandle,
            &SwapChainDescription,
            NULL,
            NULL,
            &RendWindow->SwapChain
        );

        if (FAILED(HRes)) {
            LogPanic("DXGI swap chain creation failure (%lX). The process is terminating", HRes);
            GraphicalMessage(TRUE, Str8Lit("Fatal Error"), Str8Lit("DXGI swap chain creation failure"));
            Abort(1);
        }

        DX11_STATE->DXGIFactory->Vtbl->MakeWindowAssociation(
            DX11_STATE->DXGIFactory,
            Win32WindowHandle,
            DXGI_MWA_NO_ALT_ENTER
        );

        D3D11_RENDER_TARGET_VIEW_DESC FrameBufferRTVDescription = {};

        FrameBufferRTVDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        FrameBufferRTVDescription.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        HRes = RendWindow->SwapChain->Vtbl->GetBuffer(
            RendWindow->SwapChain,
            0,
            (IID*) &IID_ID3D11Tex2D,
            (void**) &RendWindow->FrameBuffer
        );

        if (FAILED(HRes)) {
            LogPanic("Failed to get Buffer from RendWindow->SwapChain (0x%lX)", HRes);
            GraphicalMessage(TRUE, "Fatal Exception"_s8, "Failed to get Buffer from RendWindow->SwapChain"_s8);
            Abort(1);
        }

        HRes = DX11_STATE->Device->Vtbl->CreateRenderTargetView(
            DX11_STATE->Device,
            RendWindow->FrameBuffer,
            &FrameBufferRTVDescription,
            &RendWindow->FrameBufferRTV
        );

        if (FAILED(HRes)) {
            LogPanic("Failed to create RenderTargetView FrameBufferRTV (0x%lX)", HRes);
            GraphicalMessage(TRUE, "Fatal Exception"_s8, "Failed to get create RenderTargetView FrameBufferRTV"_s8);
            Abort(1);
        }

        Result = RendHandleFromDX11Window(RendWindow);
        ++DX11_STATE->WindowCount;
        DX11_STATE->DXGIDevice->Vtbl->SetMaximumFrameLatency(
            DX11_STATE->DXGIDevice,
            CLAMP(1, DX11_STATE->WindowCount, 16)
        );
    }

    return Result;
}

void 
RendUnequipWindow(Handle WindowHandle, RendHandle Renderer)
{
    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        DX11Window* RendWindow = DX11WindowFromRendHandle(Renderer);

        if (RendWindow->StageScratchColourSRV)
            RendWindow->StageScratchColourSRV->Vtbl->Release(RendWindow->StageScratchColourSRV);

        if (RendWindow->StageScratchColourRTV)
            RendWindow->StageScratchColourRTV->Vtbl->Release(RendWindow->StageScratchColourRTV);

        if (RendWindow->StageScratchColour)
            RendWindow->StageScratchColour->Vtbl->Release(RendWindow->StageScratchColour);

        if (RendWindow->Geo3DColourSRV)
            RendWindow->Geo3DColourSRV->Vtbl->Release(RendWindow->Geo3DColourSRV);

        if (RendWindow->Geo3DColourRTV)
            RendWindow->Geo3DColourRTV->Vtbl->Release(RendWindow->Geo3DColourRTV);

        if (RendWindow->Geo3DColour)
            RendWindow->Geo3DColour->Vtbl->Release(RendWindow->Geo3DColour);

        if (RendWindow->Geo3DDepthSRV)
            RendWindow->Geo3DDepthSRV->Vtbl->Release(RendWindow->Geo3DDepthSRV);

        if (RendWindow->Geo3DDepthDSV)
            RendWindow->Geo3DDepthDSV->Vtbl->Release(RendWindow->Geo3DDepthDSV);

        if (RendWindow->Geo3DDepth)
            RendWindow->Geo3DDepth->Vtbl->Release(RendWindow->Geo3DDepth);

        if (RendWindow->StageColourSRV)
            RendWindow->StageColourSRV->Vtbl->Release(RendWindow->StageColourSRV);

        if (RendWindow->StageColourRTV)
            RendWindow->StageColourRTV->Vtbl->Release(RendWindow->StageColourRTV);

        if (RendWindow->StageColour)
            RendWindow->StageColour->Vtbl->Release(RendWindow->StageColour);

        if (RendWindow->FrameBufferRTV)
            RendWindow->FrameBufferRTV->Vtbl->Release(RendWindow->FrameBufferRTV);

        if (RendWindow->FrameBuffer)
            RendWindow->FrameBuffer->Vtbl->Release(RendWindow->FrameBuffer);

        if (RendWindow->SwapChain)
            RendWindow->SwapChain->Vtbl->Release(RendWindow->SwapChain);

        ++RendWindow->Generation;
        SLL_STACK_PUSH(DX11_STATE->FreeWindow, RendWindow);
        --DX11_STATE->WindowCount;

        HRESULT HRes = DX11_STATE->DXGIDevice->Vtbl->SetMaximumFrameLatency(
            DX11_STATE->DXGIDevice,
            CLAMP(1, DX11_STATE->WindowCount, 7)
        );

        if (FAILED(HRes)) {
            LogError("unable to set maximum frame latency"_s8);
            GraphicalMessage(
                TRUE, 
                "Fatal Exception"_s8, 
                "Unable to set Direct3D 11 MaximumFrameLatency"_s8
            );
            Abort(1);
        }
    }
}

RendHandle 
Tex2DAlloc(RendResourceKind Kind, v2i32 Size, RendTex2DFmt Format, void* Data)
{
    DX11Tex2D* Texture = NULL;

    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        Texture = DX11_STATE->FreeTex2D;

        if (!Texture) {
            Texture = ArenaPushArrayZero(DX11_STATE->MemPool, DX11Tex2D, 1);
        } else {
            u64 Generation = Texture->Generation;

            SLL_STACK_POP(DX11_STATE->FreeTex2D);
            MemSet(Texture, 0, sizeof(*Texture));
            Texture->Generation = Generation;
        }

        ++Texture->Generation;
    }

    D3D11_USAGE Usage = D3D11_USAGE_DEFAULT;
    UINT CPUAccessFlags = 0;

    UsageAccessFlagsFromResourceKind(Kind, &Usage, &CPUAccessFlags);

    if (Kind == RESOURCE_KIND_STATIC)
        ASSERT(Data && "Static texture must have initial data provided");

    DXGI_FORMAT DXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

    switch (Format) {
        default: {} break;

        case TEX2D_FMT_R8: {
            DXGIFormat = DXGI_FORMAT_R8_UNORM;
        } break;

        case TEX2D_FMT_RG8: {
            DXGIFormat = DXGI_FORMAT_R8G8_UNORM;
        } break;

        case TEX2D_FMT_RGBA8: {
            DXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        } break;

        case TEX2D_FMT_RGBA8_SRGB: {
            DXGIFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        } break;

        case TEX2D_FMT_BGRA8: {
            DXGIFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
        } break;

        case TEX2D_FMT_BGRA8_SRGB: {
            DXGIFormat = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
        } break;

        case TEX2D_FMT_R16: {
            DXGIFormat = DXGI_FORMAT_R16_UNORM;
        } break;

        case TEX2D_FMT_RGBA16: {
            DXGIFormat = DXGI_FORMAT_R16G16B16A16_UNORM;
        } break;

        case TEX2D_FMT_R32: {
            DXGIFormat = DXGI_FORMAT_R32_FLOAT;
        } break;

        case TEX2D_FMT_RG32: {
            DXGIFormat = DXGI_FORMAT_R32G32_FLOAT;
        } break;

        case TEX2D_FMT_RGBA32: {
            DXGIFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
        } break;
    }

    D3D11_SUBRESOURCE_DATA _InitialData = {};
    D3D11_SUBRESOURCE_DATA* InitialData = NULL;

    if (Data) {
        InitialData = &_InitialData;
        InitialData->pSysMem = Data;
        InitialData->SysMemPitch = (UINT) (TEX2D_FMT_BYTES_PER_PIXEL[Format] * Size.X);
    }

    D3D11_TEXTURE2D_DESC TexDescription = {};

    TexDescription.Width = Size.X;
    TexDescription.Height = Size.Y;
    TexDescription.MipLevels = 1;
    TexDescription.ArraySize = 1;
    TexDescription.Format = DXGIFormat;
    TexDescription.SampleDesc.Count = 1;
    TexDescription.Usage = Usage;
    TexDescription.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    TexDescription.CPUAccessFlags = CPUAccessFlags;

    HRESULT HRes = DX11_STATE->Device->Vtbl->CreateTexture2D(
        DX11_STATE->Device,
        &TexDescription, 
        InitialData, 
        &Texture->Texture
    );

    if (FAILED(HRes)) {
        LogError("unable to create Texture2D"_s8);
        GraphicalMessage(
            TRUE, 
            "Fatal Exception"_s8, 
            "Unable to create Direct3D 11 Texture2D"_s8
        );
        Abort(1);
    }

    HRes = DX11_STATE->Device->Vtbl->CreateShaderResourceView(
        DX11_STATE->Device,
        Texture->Texture, 
        NULL, 
        &Texture->View
    );

    if (FAILED(HRes)) {
        LogError("unable to create ShaderResourceView"_s8);
        GraphicalMessage(
            TRUE, 
            "Fatal Exception"_s8, 
            "Unable to create Direct3D 11 ShaderResourceView"_s8
        );
        Abort(1);
    }

    Texture->Kind = Kind;
    Texture->Size = Size;
    Texture->Format = Format;

    RendHandle Result = RendHandleFromDX11Tex2D(Texture);

    return Result;
}

void 
Tex2DRelease(RendHandle TextureHandle)
{
    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        DX11Tex2D* Texture = DX11Tex2DFromRendHandle(TextureHandle);

        if (Texture != &DX11_EMPTY_TEX2D_VALUE)
            SLL_STACK_PUSH(DX11_STATE->ToFreeTex2D, Texture);
    }
}

RendResourceKind 
RendResourceKindFromTex2D(RendHandle TextureHandle)
{
    DX11Tex2D* Texture = DX11Tex2DFromRendHandle(TextureHandle);

    return Texture->Kind;
}

v2i32 
SizeFromTex2D(RendHandle TextureHandle)
{
    DX11Tex2D* Texture = DX11Tex2DFromRendHandle(TextureHandle);

    return Texture->Size;
}

RendTex2DFmt 
Tex2DFmtFromTex2D(RendHandle TextureHandle)
{

    DX11Tex2D* Texture = DX11Tex2DFromRendHandle(TextureHandle);

    return Texture->Format;
}

void 
FillTex2DRgn(RendHandle TextureHandle, r2i32 SubRect, void* Data)
{
    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        DX11Tex2D* Texture = DX11Tex2DFromRendHandle(TextureHandle);

        if (Texture != &DX11_EMPTY_TEX2D_VALUE) {
            ASSERT(Texture->Kind == RESOURCE_KIND_DYNAMIC && "Only dynamic texture can update a region");

            u64 BytesPerPixel = TEX2D_FMT_BYTES_PER_PIXEL[Texture->Format];
            v2i32 Dimensions = Vec(SubRect.X1 - SubRect.X0, SubRect.Y1 - SubRect.Y0);
            D3D11_BOX DstBox = {
                (UINT) SubRect.X0,
                (UINT) SubRect.Y0,
                0,
                (UINT) SubRect.X1,
                (UINT) SubRect.Y1,
                1
            };

            DX11_STATE->DeviceContext->Vtbl->UpdateSubresource(
                DX11_STATE->DeviceContext,
                Texture->Texture,
                0,
                &DstBox,
                Data,
                (UINT) (Dimensions.X * BytesPerPixel),
                0
            );
        }
    }
}

RendHandle 
RendBufferAlloc(
    RendResourceKind Kind, 
    u64 Size, 
    void* Data
) {
    RendHandle Result = EMPTY_REND_HANDLE_VALUE;

    PROFILE(RendBufferAllocTimer) {
        DX11Buffer* Buffer = NULL;

        LOCK_SCOPE_W(DX11_STATE->RWMtx) {
            Buffer = DX11_STATE->FreeBuffer;

            if (!Buffer) {
                Buffer = ArenaPushArray(DX11_STATE->MemPool, DX11Buffer, 1);
            } else {
                u64 Generation = Buffer->Generation;

                SLL_STACK_POP(DX11_STATE->FreeBuffer);
                MemZero(Buffer, sizeof(*Buffer));
                Buffer->Generation = Generation;
            }

            ++Buffer->Generation;
        }

        D3D11_USAGE Usage = D3D11_USAGE_DEFAULT;
        UINT CPUAccessFlags = 0;

        UsageAccessFlagsFromResourceKind(Kind, &Usage, &CPUAccessFlags);

        if (Kind == RESOURCE_KIND_STATIC)
            ASSERT(Data && "Static Buffer must have initial data provided");

        D3D11_SUBRESOURCE_DATA __InitialData = {};
        D3D11_SUBRESOURCE_DATA* InitialData = NULL;

        if (Data) {
            InitialData = &__InitialData;
            InitialData->pSysMem = Data;
        }

        D3D11_BUFFER_DESC Desc = {};

        {
            Desc.ByteWidth = Size;
            Desc.Usage = Usage;
            Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER;
            Desc.CPUAccessFlags = CPUAccessFlags;
        }

        DX11_STATE->Device->Vtbl->CreateBuffer(
            DX11_STATE->Device,
            &Desc,
            InitialData,
            &Buffer->Buffer
        );

        {
            Buffer->Kind = Kind;
            Buffer->Size = Size;
        }

        Result = RendHandleFromDX11Buffer(Buffer);
    }

    return Result;
}

void 
RendBufferRelease(RendHandle BufferHandle)
{
    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        DX11Buffer* Buffer = DX11BufferFromRendHandle(BufferHandle);

        if (Buffer != &DX11_EMPTY_BUFFER_VALUE)
            SLL_STACK_PUSH(DX11_STATE->ToFreeBuffer, Buffer);
    }
}

void 
RendBeginFrame(void)
{
    LOCK_SCOPE_W(DX11_STATE->RWMtx);
}

void 
RendEndFrame(void)
{
    LOCK_SCOPE_W(DX11_STATE->RWMtx) {
        for (
            DX11FlushBuffer* Buffer = DX11_STATE->HeadFlushBuffer;
            Buffer;
            Buffer = Buffer->Next
        ) {
            Buffer->Buffer->Vtbl->Release(Buffer->Buffer);
        }

        for (
            DX11Tex2D* Texture = DX11_STATE->ToFreeTex2D, *Next = NULL;
            Texture;
            Texture = Next
        ) {
            Next = Texture->Next;

            if (Texture->View)
                Texture->View->Vtbl->Release(Texture->View);

            if (Texture->Texture)
                Texture->Texture->Vtbl->Release(Texture->Texture);

            Texture->View = NULL;
            Texture->Texture = NULL;
            ++Texture->Generation;
            SLL_STACK_PUSH(DX11_STATE->FreeTex2D, Texture);
        }

        for (
            DX11Buffer* Buffer = DX11_STATE->ToFreeBuffer, *Next = NULL;
            Buffer;
            Buffer = Next
        ) {
            Next = Buffer->Next;

            if (Buffer->Buffer)
                Buffer->Buffer->Vtbl->Release(Buffer->Buffer);

            ++Buffer->Generation;
            Buffer->Buffer = NULL;
            SLL_STACK_PUSH(DX11_STATE->FreeBuffer, Buffer);
        }

        ArenaClear(DX11_STATE->FlushBufferMemPool);
        DX11_STATE->HeadFlushBuffer = NULL;
        DX11_STATE->TailFlushBuffer = NULL;
        DX11_STATE->ToFreeTex2D = NULL;
        DX11_STATE->ToFreeBuffer = NULL;
    }
}

void 
RendBeginWindowFrame(Handle WindowHandle, RendHandle Renderer)
{
    PROFILE(RendBeginWindowFrameTimer) {
        LOCK_SCOPE_W(DX11_STATE->RWMtx) {
            DX11Window* Wnd = DX11WindowFromRendHandle(Renderer);
            ID3D11DeviceContext1* DevCtx = DX11_STATE->DeviceContext;
            r2f32 ClientRect = GetWindowClientRect(WindowHandle);
            v2i32 Resolution = {
                (i32) (ClientRect.X1 - ClientRect.X0),
                (i32) (ClientRect.Y1 - ClientRect.Y0)
            };
            b32 Resized = FALSE;

            if (
                Wnd->PrevResolution.X != Resolution.X || 
                Wnd->PrevResolution.Y != Resolution.Y
            ) {
                Resized = TRUE;
                Wnd->PrevResolution = Resolution;

                if (Wnd->StageScratchColourSRV) {
                    Wnd->StageScratchColourSRV->Vtbl->Release(
                        Wnd->StageScratchColourSRV
                    );
                }

                if (Wnd->StageScratchColourRTV) {
                    Wnd->StageScratchColourRTV->Vtbl->Release(
                        Wnd->StageScratchColourRTV
                    );
                }

                if (Wnd->StageScratchColour) {
                    Wnd->StageScratchColour->Vtbl->Release(
                        Wnd->StageScratchColour
                    );
                }

                if (Wnd->StageColourSRV) {
                    Wnd->StageColourSRV->Vtbl->Release(
                        Wnd->StageColourSRV
                    );
                }

                if (Wnd->StageColourRTV) {
                    Wnd->StageColourRTV->Vtbl->Release(
                        Wnd->StageColourRTV
                    );
                }

                if (Wnd->StageColour) {
                    Wnd->StageColour->Vtbl->Release(
                        Wnd->StageColour
                    );
                }

                if (Wnd->Geo3DColourSRV) {
                    Wnd->Geo3DColourSRV->Vtbl->Release(
                        Wnd->Geo3DColourSRV
                    );
                }

                if (Wnd->Geo3DColourRTV) {
                    Wnd->Geo3DColourRTV->Vtbl->Release(
                        Wnd->Geo3DColourRTV
                    );
                }

                if (Wnd->Geo3DColour) {
                    Wnd->Geo3DColour->Vtbl->Release(
                        Wnd->Geo3DColour
                    );
                }

                if (Wnd->Geo3DDepthSRV) {
                    Wnd->Geo3DDepthSRV->Vtbl->Release(
                        Wnd->Geo3DDepthSRV
                    );
                }

                if (Wnd->Geo3DDepthDSV) {
                    Wnd->Geo3DDepthDSV->Vtbl->Release(
                        Wnd->Geo3DDepthDSV
                    );
                }

                if (Wnd->Geo3DDepth) {
                    Wnd->Geo3DDepth->Vtbl->Release(
                        Wnd->Geo3DDepth
                    );
                }

                Wnd->FrameBufferRTV->Vtbl->Release(
                    Wnd->FrameBufferRTV
                );
                Wnd->FrameBuffer->Vtbl->Release(
                    Wnd->FrameBuffer
                );
                Wnd->SwapChain->Vtbl->ResizeBuffers(
                    Wnd->SwapChain,
                    0,
                    0,
                    0,
                    DXGI_FORMAT_UNKNOWN,
                    0
                );
                Wnd->SwapChain->Vtbl->GetBuffer(
                    Wnd->SwapChain,
                    0,
                    (REFIID*) &IID_ID3D11Tex2D,
                    (void**) (&Wnd->FrameBuffer)
                );

                D3D11_RENDER_TARGET_VIEW_DESC FrameBufferRTVDesc = {};

                FrameBufferRTVDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
                FrameBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                DX11_STATE->Device->Vtbl->CreateRenderTargetView(
                    DX11_STATE->Device,
                    Wnd->FrameBuffer,
                    &FrameBufferRTVDesc,
                    &Wnd->FrameBufferRTV
                );

                {   // NOTE(nathan): stage colour targets
                    D3D11_TEXTURE2D_DESC ColourDesc = {};

                    {
                        Wnd->FrameBuffer->Vtbl->GetDesc(
                            Wnd->FrameBuffer,
                            &ColourDesc
                        );
                        ColourDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                        ColourDesc.BindFlags = (
                            D3D11_BIND_RENDER_TARGET | 
                            D3D11_BIND_SHADER_RESOURCE
                        );
                    }

                    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc = {};

                    {
                        RTVDesc.Format = ColourDesc.Format;
                        RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    }

                    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

                    {
                        SRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
                        SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        SRVDesc.Texture2D.MipLevels = -1;
                    }

                    DX11_STATE->Device->Vtbl->CreateTexture2D(
                        DX11_STATE->Device,
                        &ColourDesc,
                        NULL,
                        &Wnd->StageColour
                    );
                    DX11_STATE->Device->Vtbl->CreateRenderTargetView(
                        DX11_STATE->Device,
                        Wnd->StageColour,
                        &RTVDesc,
                        &Wnd->StageColourRTV
                    );
                    DX11_STATE->Device->Vtbl->CreateShaderResourceView(
                        DX11_STATE->Device,
                        Wnd->StageColour,
                        &SRVDesc,
                        &Wnd->StageColourSRV
                    );
                    DX11_STATE->Device->Vtbl->CreateTexture2D(
                        DX11_STATE->Device,
                        &ColourDesc,
                        NULL,
                        &Wnd->StageScratchColour
                    );
                    DX11_STATE->Device->Vtbl->CreateRenderTargetView(
                        DX11_STATE->Device,
                        Wnd->StageScratchColour,
                        &RTVDesc,
                        &Wnd->StageScratchColourRTV
                    );
                    DX11_STATE->Device->Vtbl->CreateShaderResourceView(
                        DX11_STATE->Device,
                        Wnd->StageScratchColour,
                        &SRVDesc,
                        &Wnd->StageScratchColourSRV
                    );
                }

                {   // NOTE(nathan): geo 3D targets
                    D3D11_TEXTURE2D_DESC ColourDesc = {};

                    {
                        Wnd->FrameBuffer->Vtbl->GetDesc(
                            Wnd->FrameBuffer,
                            &ColourDesc
                        );
                        ColourDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        ColourDesc.BindFlags = (
                            D3D11_BIND_RENDER_TARGET | 
                            D3D11_BIND_SHADER_RESOURCE
                        );
                    }

                    D3D11_RENDER_TARGET_VIEW_DESC ColourRTVDesc = {};

                    {
                        ColourRTVDesc.Format = ColourDesc.Format;
                        ColourRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    }

                    D3D11_SHADER_RESOURCE_VIEW_DESC ColourSRVDesc = {};

                    {
                        ColourSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                        ColourSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        ColourSRVDesc.Texture2D.MipLevels = -1;
                    }

                    D3D11_TEXTURE2D_DESC DepthDesc = {};

                    {
                        Wnd->FrameBuffer->Vtbl->GetDesc(
                            Wnd->FrameBuffer,
                            &DepthDesc
                        );
                        DepthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
                        DepthDesc.BindFlags = (
                            D3D11_BIND_DEPTH_STENCIL |
                            D3D11_BIND_SHADER_RESOURCE
                        );
                    }

                    D3D11_DEPTH_STENCIL_VIEW_DESC DepthDSVDesc = {};

                    {
                        DepthDSVDesc.Flags = 0;
                        DepthDSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                        DepthDSVDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                        DepthDSVDesc.Texture2D.MipSlice = 0;
                    }

                    D3D11_SHADER_RESOURCE_VIEW_DESC DepthSRVDesc = {};

                    {
                        DepthSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                        DepthSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                        DepthSRVDesc.Texture2D.MostDetailedMip = 0;
                        DepthSRVDesc.Texture2D.MipLevels = -1;
                    }

                    DX11_STATE->Device->Vtbl->CreateTexture2D(
                        DX11_STATE->Device,
                        &ColourDesc,
                        NULL,
                        &Wnd->Geo3DColour
                    );
                    DX11_STATE->Device->Vtbl->CreateRenderTargetView(
                        DX11_STATE->Device,
                        Wnd->Geo3DColour,
                        &ColourRTVDesc,
                        &Wnd->Geo3DColourRTV
                    );
                    DX11_STATE->Device->Vtbl->CreateShaderResourceView(
                        DX11_STATE->Device,
                        Wnd->Geo3DColour,
                        &ColourSRVDesc,
                        &Wnd->Geo3DColourSRV
                    );
                    DX11_STATE->Device->Vtbl->CreateTexture2D(
                        DX11_STATE->Device,
                        &DepthDesc,
                        NULL,
                        &Wnd->Geo3DDepth
                    );
                    DX11_STATE->Device->Vtbl->CreateDepthStencilView(
                        DX11_STATE->Device,
                        Wnd->Geo3DDepth,
                        &DepthDSVDesc,
                        &Wnd->Geo3DDepthDSV
                    );
                    DX11_STATE->Device->Vtbl->CreateShaderResourceView(
                        DX11_STATE->Device,
                        Wnd->Geo3DDepth,
                        &DepthSRVDesc,
                        &Wnd->Geo3DDepthSRV
                    );
                }
            }

            v4f32 ClearColour = Vec(
                0.0f,
                0.0f,
                0.0f,
                0.0f
            );

            DevCtx->Vtbl->ClearRenderTargetView(
                DevCtx,
                Wnd->FrameBufferRTV,
                ClearColour.V
            );
            DevCtx->Vtbl->ClearRenderTargetView(
                DevCtx,
                Wnd->StageColourRTV,
                ClearColour.V
            );

            if (Resized)
                DevCtx->Vtbl->Flush(DevCtx);
        }
    }
}

void 
RendEndWindowFrame(Handle WindowHandle, RendHandle Renderer)
{
    PROFILE(RendEndWindowFrameTimer) {
        LOCK_SCOPE_W(DX11_STATE->RWMtx) {
            DX11Window* Wnd = DX11WindowFromRendHandle(Renderer);
            ID3D11DeviceContext1* DevCtx = DX11_STATE->DeviceContext;

            {
                ID3D11SamplerState* Sampler = DX11_STATE->Samplers[TEX2D_SAMPLE_NEAREST];
                ID3D11VertexShader* VertShader = DX11_STATE->VertShaders[VERT_SHADER_FINALISE];
                ID3D11PixelShader* PixelShader = DX11_STATE->PixelShaders[PIXEL_SHADER_FINALISE];

                DevCtx->Vtbl->OMSetRenderTargets(
                    DevCtx,
                    1,
                    &Wnd->FrameBufferRTV,
                    NULL
                );
                DevCtx->Vtbl->OMSetDepthStencilState(
                    DevCtx,
                    DX11_STATE->NoOpDepthStencil,
                    0
                );
                DevCtx->Vtbl->OMSetBlendState(
                    DevCtx,
                    DX11_STATE->MainBlendState,
                    NULL,
                    U32_MAX
                );

                v2i32 Resolution = Wnd->PrevResolution;
                D3D11_VIEWPORT ViewPort = {
                    0.0f,
                    0.0f,
                    (f32) Resolution.X,
                    (f32) Resolution.Y,
                    0.0f,
                    1.0f
                };

                DevCtx->Vtbl->RSSetViewports(
                    DevCtx,
                    1,
                    &ViewPort
                );
                DevCtx->Vtbl->RSSetState(
                    DevCtx,
                    (ID3D11RasterizerState*) DX11_STATE->MainRasteriser
                );
                DevCtx->Vtbl->IASetPrimitiveTopology(
                    DevCtx,
                    D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
                );
                DevCtx->Vtbl->IASetInputLayout(
                    DevCtx,
                    NULL
                );
                DevCtx->Vtbl->VSSetShader(
                    DevCtx,
                    VertShader,
                    NULL,
                    0
                );
                DevCtx->Vtbl->PSSetShader(
                    DevCtx,
                    PixelShader,
                    NULL,
                    0
                );
                DevCtx->Vtbl->PSSetShaderResources(
                    DevCtx,
                    0,
                    1,
                    &Wnd->StageColourSRV
                );
                DevCtx->Vtbl->PSSetSamplers(
                    DevCtx,
                    0,
                    1,
                    &Sampler
                );

                {
                    D3D11_RECT Rect = {};

                    Rect.left = 0;
                    Rect.right = (LONG) Wnd->PrevResolution.X;
                    Rect.top = 0;
                    Rect.bottom = (LONG) Wnd->PrevResolution.Y;
                    DevCtx->Vtbl->RSSetScissorRects(
                        DevCtx,
                        1,
                        &Rect
                    );
                }

                DevCtx->Vtbl->Draw(DevCtx, 4, 0);
            }

            HRESULT HRes = Wnd->SwapChain->Vtbl->Present(
                Wnd->SwapChain,
                1,
                0
            );

            if (FAILED(HRes)) {
                LogPanic(
                    "D3D11 present failure (%lX). The process is terminating.", 
                    HRes
                );
                GraphicalMessage(
                    TRUE,
                    "Fatal Error"_s8,
                    "Direct3D 11 failed to present."_s8
                );
                Abort(1);
            }

            DevCtx->Vtbl->ClearState(DevCtx);
        }
    }
}

void 
RendSubmitWindow(Handle WindowHandle, RendHandle Renderer, RendPassList* Passes)
{
    PROFILE(RendSubmitWindowTimer) {
        LOCK_SCOPE_W(DX11_STATE->RWMtx) {
            DX11Window* Wnd = DX11WindowFromRendHandle(Renderer);
            ID3D11DeviceContext1* DevCtx = DX11_STATE->DeviceContext;

            for (
                RendPassNode* PassNode = Passes->Head; 
                PassNode; 
                PassNode = PassNode->Next
            ) {
                RendPass* Pass = &PassNode->V;

                switch (Pass->Kind) {
                    default: {} break;

                    case PASS_KIND_UI: {
                        RendPassUIParams* Params = Pass->UIParams;
                        RendBatch2DList* RectBatchGroups = &Params->Rects;
                        v2i32 Resolution = Wnd->PrevResolution;
                        D3D11_VIEWPORT ViewPort = {
                            0.0f,
                            0.0f,
                            (f32) Resolution.X,
                            (f32) Resolution.Y,
                            0.0f,
                            1.0f
                        };

                        DevCtx->Vtbl->RSSetViewports(DevCtx, 1, &ViewPort);
                        DevCtx->Vtbl->RSSetState(
                            DevCtx,
                            (ID3D11RasterizerState*) DX11_STATE->MainRasteriser
                        );

                        for (
                            RendBatch2DNode* GroupNode = RectBatchGroups->Head;
                            GroupNode;
                            GroupNode = GroupNode->Next
                        ) {
                            RendBatchList* Batches = &GroupNode->Batches;
                            RendBatch2DParams* GroupParams = &GroupNode->Params;
                            ID3D11SamplerState* Sampler = DX11_STATE->Samplers[GroupParams->SampleKind];
                            ID3D11VertexShader* VertShader = DX11_STATE->VertShaders[VERT_SHADER_RECT];
                            ID3D11InputLayout* InputLayout = DX11_STATE->InputLayouts[VERT_SHADER_RECT];
                            ID3D11PixelShader* PixelShader = DX11_STATE->PixelShaders[PIXEL_SHADER_RECT];
                            ID3D11Buffer* UniformsBuffer = DX11_STATE->UniformBuffers[UNIFORM_KIND_RECT];
                            ID3D11Buffer* Buffer = DX11BufferFromSize(Batches->ByteCount);

                            {
                                D3D11_MAPPED_SUBRESOURCE SubResource = {};

                                DevCtx->Vtbl->Map(
                                    DevCtx,
                                    Buffer,
                                    0,
                                    D3D11_MAP_WRITE_DISCARD,
                                    0,
                                    &SubResource
                                );

                                u8* DstPtr = (u8*) SubResource.pData;
                                u64 Offset = 0;

                                for (
                                    RendBatchNode* BatchNode = Batches->Head;
                                    BatchNode;
                                    BatchNode = BatchNode->Next
                                ) {
                                    MemCpy(
                                        DstPtr + Offset, 
                                        BatchNode->V.V, 
                                        BatchNode->V.ByteCount
                                    );
                                    Offset += BatchNode->V.ByteCount;
                                }

                                DevCtx->Vtbl->Unmap(DevCtx, Buffer, 0);
                            }

                            RendHandle TextureHandle = GroupParams->Texture;

                            if (TextureHandle == EMPTY_REND_HANDLE_VALUE)
                                TextureHandle = DX11_STATE->BackupTexture;

                            DX11Tex2D* Texture = DX11Tex2DFromRendHandle(TextureHandle);
                            m4f32 TextureSampleChannelMap = SampleChannelMapFromTex2DFmt(
                                Texture->Format
                            );
                            DX11UniformsRect Uniforms = {};

                            {
                                Uniforms.ViewportSize = Vec(
                                    (f32) Resolution.X, 
                                    (f32) Resolution.Y
                                );
                                Uniforms.Opacity = 1.0f - GroupParams->Transparency;
                                Uniforms.TexSampleChannelMap = TextureSampleChannelMap;
                                Uniforms.Tex2DSize = Vec(
                                    (f32) Texture->Size.X,
                                    (f32) Texture->Size.Y
                                );
                                Uniforms.XForm[0] = Vec(
                                    GroupParams->XForm.V[0][0],
                                    GroupParams->XForm.V[1][0],
                                    GroupParams->XForm.V[2][0],
                                    0.0f
                                );
                                Uniforms.XForm[1] = Vec(
                                    GroupParams->XForm.V[0][1],
                                    GroupParams->XForm.V[1][1],
                                    GroupParams->XForm.V[2][1],
                                    0.0f
                                );
                                Uniforms.XForm[2] = Vec(
                                    GroupParams->XForm.V[0][2],
                                    GroupParams->XForm.V[1][2],
                                    GroupParams->XForm.V[2][2],
                                    0.0f
                                );

                                v2f32 XFormCol0 = Vec(
                                    Uniforms.XForm[0].X, 
                                    Uniforms.XForm[1].X
                                );
                                v2f32 XFormCol1 = Vec(
                                    Uniforms.XForm[0].Y,
                                    Uniforms.XForm[1].Y
                                );

                                Uniforms.XFormScale.X = Length(XFormCol0);
                                Uniforms.XFormScale.Y = Length(XFormCol1);
                            }

                            {
                                D3D11_MAPPED_SUBRESOURCE SubResource = {};

                                DevCtx->Vtbl->Map(
                                    DevCtx,
                                    UniformsBuffer,
                                    0,
                                    D3D11_MAP_WRITE_DISCARD,
                                    0,
                                    &SubResource
                                );
                                MemCpy(
                                    (u8*) SubResource.pData,
                                    &Uniforms,
                                    sizeof(Uniforms)
                                );
                                DevCtx->Vtbl->Unmap(DevCtx, UniformsBuffer, 0);
                            }

                            DevCtx->Vtbl->OMSetRenderTargets(
                                DevCtx,
                                1,
                                &Wnd->StageColourRTV,
                                NULL
                            );
                            DevCtx->Vtbl->OMSetDepthStencilState(
                                DevCtx,
                                DX11_STATE->NoOpDepthStencil,
                                0
                            );
                            DevCtx->Vtbl->OMSetBlendState(
                                DevCtx,
                                DX11_STATE->MainBlendState,
                                NULL,
                                U32_MAX
                            );

                            u32 Stride = Batches->BytesPerInstruction;
                            u32 Offset = 0;

                            DevCtx->Vtbl->IASetPrimitiveTopology(
                                DevCtx,
                                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
                            );
                            DevCtx->Vtbl->IASetInputLayout(
                                DevCtx,
                                InputLayout
                            );
                            DevCtx->Vtbl->IASetVertexBuffers(
                                DevCtx,
                                0,
                                1,
                                &Buffer,
                                &Stride,
                                &Offset
                            );
                            DevCtx->Vtbl->VSSetShader(
                                DevCtx,
                                VertShader,
                                NULL,
                                0
                            );
                            DevCtx->Vtbl->VSSetConstantBuffers(
                                DevCtx,
                                0,
                                1,
                                &UniformsBuffer
                            );
                            DevCtx->Vtbl->PSSetShader(
                                DevCtx,
                                PixelShader,
                                NULL,
                                0
                            );
                            DevCtx->Vtbl->PSSetConstantBuffers(
                                DevCtx,
                                0,
                                1,
                                &UniformsBuffer
                            );
                            DevCtx->Vtbl->PSSetShaderResources(
                                DevCtx,
                                0,
                                1,
                                &Texture->View
                            );
                            DevCtx->Vtbl->PSSetSamplers(
                                DevCtx,
                                0,
                                1,
                                &Sampler
                            );

                            {
                                r2f32 Clip = GroupParams->Clip;
                                D3D11_RECT Rect = {};

                                {
                                    if (
                                        Clip.X0 == 0.0f &&
                                        Clip.Y0 == 0.0f &&
                                        Clip.X1 == 0.0f &&
                                        Clip.Y1 == 0.0f
                                    ) {
                                        Rect.right = (LONG) Wnd->PrevResolution.X;
                                        Rect.bottom = (LONG) Wnd->PrevResolution.Y;
                                    } else if (
                                        Clip.X0 <= Clip.X1 && 
                                        Clip.Y0 <= Clip.Y1
                                    ) {
                                        Rect.left = (LONG) Clip.X0;
                                        Rect.right = (LONG) Clip.X1;
                                        Rect.top = (LONG) Clip.Y0;
                                        Rect.bottom = (LONG) Clip.Y1;
                                    }
                                }

                                DevCtx->Vtbl->RSSetScissorRects(
                                    DevCtx,
                                    1,
                                    &Rect
                                );
                            }

                            DevCtx->Vtbl->DrawInstanced(
                                DevCtx,
                                4,
                                Batches->ByteCount / Batches->BytesPerInstruction,
                                0,
                                0
                            );
                        }
                    } break;

                    case PASS_KIND_BLUR: {
                        RendPassBlurParams* Params = Pass->BlurParams;
                        ID3D11SamplerState* Sampler = DX11_STATE->Samplers[TEX2D_SAMPLE_LINEAR];
                        ID3D11VertexShader* VertShader = DX11_STATE->VertShaders[VERT_SHADER_BLUR];
                        ID3D11PixelShader* PixelShader = DX11_STATE->PixelShaders[PIXEL_SHADER_BLUR];
                        ID3D11Buffer* UniformsBuffer = DX11_STATE->UniformBuffers[UNIFORM_KIND_BLUR];

                        DevCtx->Vtbl->OMSetDepthStencilState(
                            DevCtx,
                            DX11_STATE->NoOpDepthStencil,
                            0
                        );
                        DevCtx->Vtbl->OMSetBlendState(
                            DevCtx,
                            DX11_STATE->NoBlendState,
                            NULL,
                            U32_MAX
                        );

                        v2i32 Resolution = Wnd->PrevResolution;
                        D3D11_VIEWPORT ViewPort = {
                            0.0f,
                            0.0f,
                            (f32) Resolution.X,
                            (f32) Resolution.Y,
                            0.0f,
                            1.0f
                        };

                        DevCtx->Vtbl->RSSetViewports(
                            DevCtx,
                            1,
                            &ViewPort
                        );
                        DevCtx->Vtbl->RSSetState(
                            DevCtx,
                            (ID3D11RasterizerState*) DX11_STATE->MainRasteriser
                        );
                        DevCtx->Vtbl->IASetPrimitiveTopology(
                            DevCtx,
                            D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
                        );
                        DevCtx->Vtbl->IASetInputLayout(
                            DevCtx,
                            NULL
                        );
                        DevCtx->Vtbl->VSSetShader(
                            DevCtx,
                            VertShader,
                            NULL,
                            0
                        );
                        DevCtx->Vtbl->VSSetConstantBuffers(
                            DevCtx,
                            0,
                            1,
                            &UniformsBuffer
                        );
                        DevCtx->Vtbl->PSSetShader(
                            DevCtx,
                            PixelShader,
                            NULL,
                            0
                        );
                        DevCtx->Vtbl->PSSetSamplers(
                            DevCtx,
                            0,
                            1,
                            &Sampler
                        );

                        {
                            D3D11_RECT Rect = {};

                            Rect.right = (LONG) Wnd->PrevResolution.X;
                            Rect.bottom = (LONG) Wnd->PrevResolution.Y;
                            DevCtx->Vtbl->RSSetScissorRects(
                                DevCtx,
                                1,
                                &Rect
                            );
                        }

                        DX11UniformsBlur Uniforms = {};

                        {
                            f32 Weights[ARRAY_COUNT(Uniforms.Kernel) * 2] = {};
                            f32 BlurSize = MIN(Params->BlurSize, ARRAY_COUNT(Weights));
                            u64 BlurCount = (u64) RoundF32(BlurSize);
                            f32 StdDev = (BlurSize - 1.0f) / 2.0f;
                            f32 CoEff = 1.0f / SqrtF32(2.0f * NC_PI32 * StdDev * StdDev);

                            Weights[0] = 1.0f;

                            if (StdDev > 0.0f) {
                                for (u64 Index = 0; Index < BlurCount; ++Index) {
                                    f32 KernelX = (f32) Index;

                                    Weights[Index] = (
                                        CoEff * (
                                            PowF32(
                                                NC_EULER32,
                                                -KernelX * KernelX / (2.0f * StdDev * StdDev)
                                            )
                                        )
                                    );
                                }
                            }

                            if (Weights[0] > 1.0f) {
                                MemZero(Weights, sizeof(Weights));
                                Weights[0] = 1.0f;
                            } else {
                                for (u64 Index = 1; Index < BlurCount; Index += 2) {
                                    f32 Weight0 = Weights[Index];
                                    f32 Weight1 = Weights[Index + 1];
                                    f32 Weight = Weight0 + Weight1;
                                    f32 T = Weight1 / Weight;

                                    Uniforms.Kernel[(Index + 1) / 2] = Vec(
                                        Weight,
                                        (f32) Index + T,
                                        0.0f,
                                        0.0f
                                    );
                                }
                            }

                            Uniforms.Kernel[0].X = Weights[0];
                            Uniforms.Passes[AXIS_2D_X].ViewportSize = Vec(
                                (f32) Resolution.X,
                                (f32) Resolution.Y
                            );
                            Uniforms.Passes[AXIS_2D_X].Rect = Params->Rect;
                            Uniforms.Passes[AXIS_2D_X].Direction = Vec(
                                1.0f / Resolution.X,
                                0.0f
                            );
                            Uniforms.Passes[AXIS_2D_X].BlurCount = 1 + BlurCount / 2;
                            MemCpyArray(
                                Uniforms.Passes[AXIS_2D_X].CornerRadii.V,
                                Params->CornerRadii
                            );
                            Uniforms.Passes[AXIS_2D_Y].ViewportSize = Vec(
                                (f32) Resolution.X,
                                (f32) Resolution.Y
                            );
                            Uniforms.Passes[AXIS_2D_Y].Rect = Params->Rect;
                            Uniforms.Passes[AXIS_2D_Y].Direction = Vec(
                                0.0f,
                                1.0f / Resolution.Y
                            );
                            Uniforms.Passes[AXIS_2D_Y].BlurCount = 1 + BlurCount / 2;
                            MemCpyArray(
                                Uniforms.Passes[AXIS_2D_Y].CornerRadii.V,
                                Params->CornerRadii
                            );

                            D3D11_MAPPED_SUBRESOURCE SubResource = {};

                            DevCtx->Vtbl->Map(
                                DevCtx,
                                UniformsBuffer,
                                0,
                                D3D11_MAP_WRITE_DISCARD,
                                0,
                                &SubResource
                            );
                            MemCpy(
                                (u8*) SubResource.pData,
                                &Uniforms,
                                sizeof(Uniforms)
                            );
                            DevCtx->Vtbl->Unmap(
                                DevCtx,
                                UniformsBuffer,
                                0
                            );
                        }

                        ID3D11Buffer* UniformsBuffers[] = {
                            UniformsBuffer,
                            UniformsBuffer
                        };
                        u32 UniformOffset[AXIS_2D_COUNT][2] = {
                            {
                                0 * sizeof(DX11UniformsBlurPass) / 16,
                                (u32) OFFSETOF(DX11UniformsBlur, Kernel) / 16
                            },
                            {
                                1 * sizeof(DX11UniformsBlurPass) / 16,
                                (u32) OFFSETOF(DX11UniformsBlur, Kernel) / 16
                            }
                        };
                        u32 UniformCount[AXIS_2D_COUNT][2] = {
                            {
                                sizeof(DX11UniformsBlurPass) / 16,
                                sizeof(Uniforms.Kernel) / 16
                            },
                            {
                                sizeof(DX11UniformsBlurPass) / 16,
                                sizeof(Uniforms.Kernel) / 16
                            }
                        };


                        {
                            r2f32 Clip = Params->Clip;
                            D3D11_RECT Rect = {};

                            {
                                if (
                                    Clip.X0 == 0.0f &&
                                    Clip.Y0 == 0.0f &&
                                    Clip.X1 == 0.0f &&
                                    Clip.Y1 == 0.0f
                                ) {
                                    Rect.right = (LONG) Wnd->PrevResolution.X;
                                    Rect.bottom = (LONG) Wnd->PrevResolution.Y;
                                } else if (
                                    Clip.X0 <= Clip.X1 && 
                                    Clip.Y0 <= Clip.Y1
                                ) {
                                    Rect.left = (LONG) Clip.X0;
                                    Rect.right = (LONG) Clip.X1;
                                    Rect.top = (LONG) Clip.Y0;
                                    Rect.bottom = (LONG) Clip.Y1;
                                }
                            }

                            DevCtx->Vtbl->RSSetScissorRects(
                                DevCtx,
                                1,
                                &Rect
                            );
                        }

                        ID3D11ShaderResourceView* SRV = NULL;

                        DevCtx->Vtbl->OMSetRenderTargets(
                            DevCtx,
                            1,
                            &Wnd->StageScratchColourRTV,
                            NULL
                        );
                        DevCtx->Vtbl->PSSetConstantBuffers1(
                            DevCtx,
                            0,
                            ARRAY_COUNT(UniformsBuffers),
                            UniformsBuffers,
                            UniformOffset[AXIS_2D_X],
                            UniformCount[AXIS_2D_X]
                        );
                        DevCtx->Vtbl->PSSetShaderResources(
                            DevCtx,
                            0,
                            1,
                            &Wnd->StageColourSRV
                        );
                        DevCtx->Vtbl->Draw(
                            DevCtx,
                            4,
                            0
                        );
                        DevCtx->Vtbl->PSSetShaderResources(
                            DevCtx,
                            0,
                            1,
                            &SRV
                        );
                        DevCtx->Vtbl->OMSetRenderTargets(
                            DevCtx,
                            1,
                            &Wnd->StageColourRTV,
                            NULL
                        );
                        DevCtx->Vtbl->PSSetConstantBuffers1(
                            DevCtx,
                            0,
                            ARRAY_COUNT(UniformsBuffers),
                            UniformsBuffers,
                            UniformOffset[AXIS_2D_Y],
                            UniformCount[AXIS_2D_Y]
                        );
                        DevCtx->Vtbl->PSSetShaderResources(
                            DevCtx,
                            0,
                            1,
                            &Wnd->StageScratchColourSRV
                        );
                        DevCtx->Vtbl->Draw(
                            DevCtx,
                            4,
                            0
                        );
                        DevCtx->Vtbl->PSSetShaderResources(
                            DevCtx,
                            0,
                            1,
                            &SRV
                        );
                    } break;

                    case PASS_KIND_GEO3D: {
                        RendPassGeo3DParams* Params = Pass->Geo3DParams;
                        RendBatch3DMap* MeshMap = &Params->Batches;
                        v2f32 ViewPortLength = Length(Params->ViewPort);
                        D3D11_VIEWPORT ViewPort = {
                            Params->ViewPort.X0,
                            Params->ViewPort.Y0,
                            ViewPortLength.X,
                            ViewPortLength.Y,
                            0.0f,
                            1.0f
                        };

                        DevCtx->Vtbl->RSSetViewports(
                            DevCtx,
                            1,
                            &ViewPort
                        );
                        DevCtx->Vtbl->RSSetState(
                            DevCtx,
                            (ID3D11RasterizerState*) DX11_STATE->MainRasteriser
                        );

                        {
                            v4f32 BackgroundColour = Vec(0.0f, 0.0f, 0.0f, 0.0f);

                            DevCtx->Vtbl->ClearRenderTargetView(
                                DevCtx,
                                Wnd->Geo3DColourRTV,
                                BackgroundColour.V
                            );
                            DevCtx->Vtbl->ClearDepthStencilView(
                                DevCtx,
                                Wnd->Geo3DDepthDSV,
                                D3D11_CLEAR_DEPTH,
                                1.0f,
                                0
                            );
                        }

                        {
                            ID3D11VertexShader* VertShader = DX11_STATE->VertShaders[VERT_SHADER_MESH];
                            ID3D11InputLayout* InputLayout = DX11_STATE->InputLayouts[VERT_SHADER_MESH];
                            ID3D11PixelShader* PixelShader = DX11_STATE->PixelShaders[PIXEL_SHADER_MESH];
                            ID3D11Buffer* UniformsBuffer = DX11_STATE->UniformBuffers[VERT_SHADER_MESH];

                            DevCtx->Vtbl->OMSetRenderTargets(
                                DevCtx,
                                1,
                                &Wnd->Geo3DColourRTV,
                                Wnd->Geo3DDepthDSV
                            );
                            DevCtx->Vtbl->OMSetDepthStencilState(
                                DevCtx,
                                DX11_STATE->PlainDepthStencil,
                                0
                            );
                            DevCtx->Vtbl->OMSetBlendState(
                                DevCtx,
                                DX11_STATE->MainBlendState,
                                NULL,
                                U32_MAX
                            );

                            for (
                                u64 SlotIndex = 0;
                                SlotIndex < MeshMap->SlotsCount;
                                ++SlotIndex
                            ) {
                                for (
                                    RendBatch3DMapNode* Node = MeshMap->Slots[SlotIndex];
                                    Node;
                                    Node = Node->Next
                                ) {
                                    RendBatchList* Batches = &Node->Batches;
                                    RendBatch3DParams* BatchParams = &Node->Params;
                                    DX11Buffer* Vertices = DX11BufferFromRendHandle(
                                        BatchParams->Vertices
                                    );
                                    DX11Buffer* Indices = DX11BufferFromRendHandle(
                                        BatchParams->Indices
                                    );
                                    u32 Stride = 11 * sizeof(f32);
                                    u32 Offset = 0;

                                    DevCtx->Vtbl->IASetPrimitiveTopology(
                                        DevCtx,
                                        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST
                                    );
                                    DevCtx->Vtbl->IASetInputLayout(
                                        DevCtx,
                                        InputLayout
                                    );
                                    DevCtx->Vtbl->IASetVertexBuffers(
                                        DevCtx,
                                        0,
                                        1,
                                        &Vertices->Buffer,
                                        &Stride,
                                        &Offset
                                    );
                                    DevCtx->Vtbl->IASetIndexBuffer(
                                        DevCtx,
                                        Indices->Buffer,
                                        DXGI_FORMAT_R32_UINT,
                                        0
                                    );

                                    DX11UniformsMesh Uniforms = {};

                                    Uniforms.XForm = (
                                        Params->Projection * Params->View
                                    );

                                    {
                                        D3D11_MAPPED_SUBRESOURCE SubResource = {};

                                        DevCtx->Vtbl->Map(
                                            DevCtx,
                                            UniformsBuffer,
                                            0,
                                            D3D11_MAP_WRITE_DISCARD,
                                            0,
                                            &SubResource
                                        );
                                        MemCpy(
                                            (u8*) SubResource.pData,
                                            &Uniforms,
                                            sizeof(Uniforms)
                                        );
                                        DevCtx->Vtbl->Unmap(
                                            DevCtx,
                                            UniformsBuffer,
                                            0
                                        );
                                    }

                                    DevCtx->Vtbl->VSSetShader(
                                        DevCtx,
                                        VertShader,
                                        NULL,
                                        0
                                    );
                                    DevCtx->Vtbl->VSSetConstantBuffers(
                                        DevCtx,
                                        0,
                                        1,
                                        &UniformsBuffer
                                    );
                                    DevCtx->Vtbl->PSSetShader(
                                        DevCtx,
                                        PixelShader,
                                        NULL,
                                        0
                                    );
                                    DevCtx->Vtbl->PSSetConstantBuffers(
                                        DevCtx,
                                        0,
                                        1,
                                        &UniformsBuffer
                                    );

                                    {
                                        D3D11_RECT Rect = {};

                                        {
                                            Rect.right = (LONG) Wnd->PrevResolution.X;
                                            Rect.bottom = (LONG) Wnd->PrevResolution.Y;
                                        }

                                        DevCtx->Vtbl->RSSetScissorRects(
                                            DevCtx,
                                            1,
                                            &Rect
                                        );
                                    }

                                    DevCtx->Vtbl->DrawIndexed(
                                        DevCtx,
                                        Indices->Size / sizeof(u32),
                                        0,
                                        0
                                    );
                                }
                            }
                        }

                        {
                            ID3D11SamplerState* Sampler = DX11_STATE->Samplers[TEX2D_SAMPLE_NEAREST];
                            ID3D11VertexShader* VertShader = DX11_STATE->VertShaders[VERT_SHADER_GEO3D_COMPOSITE];
                            ID3D11PixelShader* PixelShader = DX11_STATE->PixelShaders[PIXEL_SHADER_GEO3D_COMPOSITE];

                            DevCtx->Vtbl->OMSetRenderTargets(
                                DevCtx,
                                1,
                                &Wnd->StageColourRTV,
                                NULL
                            );
                            DevCtx->Vtbl->OMSetDepthStencilState(
                                DevCtx,
                                DX11_STATE->NoOpDepthStencil,
                                0
                            );
                            DevCtx->Vtbl->OMSetBlendState(
                                DevCtx,
                                DX11_STATE->MainBlendState,
                                NULL,
                                U32_MAX
                            );

                            v2i32 Resolution = Wnd->PrevResolution;
                            D3D11_VIEWPORT ViewPort = {
                                0.0f,
                                0.0f,
                                (f32) Resolution.X,
                                (f32) Resolution.Y,
                                0.0f,
                                1.0f
                            };

                            DevCtx->Vtbl->RSSetViewports(
                                DevCtx,
                                1,
                                &ViewPort
                            );
                            DevCtx->Vtbl->RSSetState(
                                DevCtx,
                                (ID3D11RasterizerState*) DX11_STATE->MainRasteriser
                            );
                            DevCtx->Vtbl->IASetPrimitiveTopology(
                                DevCtx,
                                D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP
                            );
                            DevCtx->Vtbl->IASetInputLayout(
                                DevCtx,
                                NULL
                            );
                            DevCtx->Vtbl->VSSetShader(
                                DevCtx,
                                VertShader,
                                NULL,
                                0
                            );
                            DevCtx->Vtbl->PSSetShader(
                                DevCtx,
                                PixelShader,
                                0,
                                0
                            );
                            DevCtx->Vtbl->PSSetShaderResources(
                                DevCtx,
                                0,
                                1,
                                &Wnd->Geo3DColourSRV
                            );
                            DevCtx->Vtbl->PSSetSamplers(
                                DevCtx,
                                0,
                                1,
                                &Sampler
                            );

                            {
                                r2f32 Clip = Params->Clip;
                                D3D11_RECT Rect = {};

                                if (
                                    Clip.X0 == 0.0f &&
                                    Clip.Y0 == 0.0f &&
                                    Clip.X1 == 0.0f &&
                                    Clip.Y1 == 0.0f
                                ) {
                                    Rect.right = (LONG) Wnd->PrevResolution.X;
                                    Rect.bottom = (LONG) Wnd->PrevResolution.Y;
                                } else if (
                                    Clip.X0 <= Clip.X1 && 
                                    Clip.Y0 <= Clip.Y1
                                ) {
                                    Rect.left = (LONG) Clip.X0;
                                    Rect.right = (LONG) Clip.X1;
                                    Rect.top = (LONG) Clip.Y0;
                                    Rect.bottom = (LONG) Clip.Y1;
                                }

                                DevCtx->Vtbl->RSSetScissorRects(
                                    DevCtx,
                                    1,
                                    &Rect
                                );
                            }

                            DevCtx->Vtbl->Draw(
                                DevCtx,
                                4,
                                0
                            );
                        }
                    } break;
                }
            }
        }
    }
}
EXTERN_C_LINK_END
