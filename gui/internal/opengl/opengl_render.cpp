#include "../../nc_defines.h"
#include "../../nc_arena.h"
#include "../../nc_log.h"
#include "../../nc_window.h"

#include "opengl_platform.h"
#include "opengl_glx_platform.h"

OGLState* OGL_STATE = NULL;

global OGLAttribute OGL_RECT_INPUT_ATTRIBUTES[] = {
    {0, Str8Lit("c2v_dst_rect"),     GL_FLOAT, 4},
    {1, Str8Lit("c2v_src_rect"),     GL_FLOAT, 4},
    {2, Str8Lit("c2v_colours_0"),    GL_FLOAT, 4},
    {3, Str8Lit("c2v_colours_1"),    GL_FLOAT, 4},
    {4, Str8Lit("c2v_colours_2"),    GL_FLOAT, 4},
    {5, Str8Lit("c2v_colours_3"),    GL_FLOAT, 4},
    {6, Str8Lit("c2v_corner_radii"), GL_FLOAT, 4},
    {7, Str8Lit("c2v_style"),        GL_FLOAT, 4}
};

global OGLAttribute OGL_SINGLE_COLOUR_OUTPUT_ATTRIBUTES[] = {
    {0, Str8Lit("final_colour")}
};

EXTERN_C_LINK_BEGIN
global Str8 OGL_RECT_VSHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "#version 330 core\n"
    "\n"
    "in vec4 c2v_dst_rect;\n"
    "in vec4 c2v_src_rect;\n"
    "in vec4 c2v_colours_0;\n"
    "in vec4 c2v_colours_1;\n"
    "in vec4 c2v_colours_2;\n"
    "in vec4 c2v_colours_3;\n"
    "in vec4 c2v_corner_radii;\n"
    "in vec4 c2v_style;  // x: border_thickness_px, y: softness_px, z: omit_texture, w: shear\n"
    "\n"
    "out vec2 v2p_sdf_sample_pos;\n"
    "out vec2 v2p_texcoord_pct;\n"
    "out vec2 v2p_rect_half_size_px;\n"
    "out vec4 v2p_tint;\n"
    "out float v2p_corner_radius;\n"
    "out float v2p_border_thickness;\n"
    "out float v2p_softness;\n"
    "out float v2p_omit_texture;\n"
    "\n"
    "uniform mat3 u_xform;\n"
    "uniform sampler2D u_tex_colour;\n"
    "uniform vec2 u_viewport_size_px;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "\n"
    "  vec2 vertices[] = vec2[](vec2(-1, -1), vec2(-1, +1), vec2(+1, -1), vec2(+1, +1));\n"
    "  \n"
    "\n"
    "  float shears[] = float[](0, 0, c2v_style.w, c2v_style.w);\n"
    "  \n"
    "\n"
    "  vec2 dst_half_size = (c2v_dst_rect.zw - c2v_dst_rect.xy) / 2;\n"
    "  vec2 dst_center    = (c2v_dst_rect.zw + c2v_dst_rect.xy) / 2;\n"
    "  vec2 dst_position  = vertices[gl_VertexID] * dst_half_size + dst_center;\n"
    "  dst_position.y += shears[gl_VertexID];\n"
    "  dst_position = (u_xform * vec3(dst_position.x, dst_position.y, 1)).xy;\n"
    "  \n"
    "\n"
    "  vec2 src_half_size = (c2v_src_rect.zw - c2v_src_rect.xy) / 2;\n"
    "  vec2 src_center    = (c2v_src_rect.zw + c2v_src_rect.xy) / 2;\n"
    "  vec2 src_position  = vertices[gl_VertexID] * src_half_size + src_center;\n"
    "  \n"
    "\n"
    "  vec4 colours[] = vec4[](c2v_colours_0, c2v_colours_1, c2v_colours_2, c2v_colours_3);\n"
    "  vec4 colour = colours[gl_VertexID];\n"
    "  \n"
    "\n"
    "  float corner_radii[] = float[](c2v_corner_radii.x, c2v_corner_radii.y, c2v_corner_radii.z, c2v_corner_radii.w);\n"
    "  float corner_radius = corner_radii[gl_VertexID];\n"
    "  \n"
    "\n"
    "  vec2 dst_verts_pct = vec2(((gl_VertexID >> 1) != 1) ? 1.f : 0.f,\n"
    "                            ((gl_VertexID & 1) != 0)  ? 0.f : 1.f);\n"
    "  ivec2 u_tex_colour_size_i = textureSize(u_tex_colour, 0);\n"
    "  vec2 u_tex_colour_size = vec2(float(u_tex_colour_size_i.x), float(u_tex_colour_size_i.y));\n"
    "  {\n"
    "    gl_Position = vec4(2 * dst_position.x / u_viewport_size_px.x - 1,\n"
    "                       2 * (1 - dst_position.y / u_viewport_size_px.y) - 1,\n"
    "                       0.0, 1.0);\n"
    "    v2p_sdf_sample_pos    = (2.f * dst_verts_pct - 1.f) * dst_half_size;\n"
    "    v2p_texcoord_pct      = src_position / u_tex_colour_size;\n"
    "    v2p_rect_half_size_px = dst_half_size;\n"
    "    v2p_tint              = colour;\n"
    "    v2p_corner_radius     = corner_radius;\n"
    "    v2p_border_thickness  = c2v_style.x;\n"
    "    v2p_softness          = c2v_style.y;\n"
    "    v2p_omit_texture      = c2v_style.z;\n"
    "  }\n"
    "}\n"
    ""
);

global Str8 OGL_RECT_PSHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "#version 330 core\n"
    "\n"
    "in vec2 v2p_sdf_sample_pos;\n"
    "in vec2 v2p_texcoord_pct;\n"
    "in vec2 v2p_rect_half_size_px;\n"
    "in vec4 v2p_tint;\n"
    "in float v2p_corner_radius;\n"
    "in float v2p_border_thickness;\n"
    "in float v2p_softness;\n"
    "in float v2p_omit_texture;\n"
    "\n"
    "out vec4 final_colour;\n"
    "\n"
    "uniform float u_opacity;\n"
    "uniform sampler2D u_tex_colour;\n"
    "uniform mat4 u_texture_sample_channel_map;\n"
    "\n"
    "float rect_sdf(vec2 sample_pos, vec2 rect_half_size, float r)\n"
    "{\n"
    "  return length(max(abs(sample_pos) - rect_half_size + r, 0.0)) - r;\n"
    "}\n"
    "\n"
    "float linear_from_srgb_f32(float x)\n"
    "{\n"
    "  return x < 0.0404482362771082 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);\n"
    "}\n"
    "\n"
    "vec4 linear_from_srgba(vec4 v)\n"
    "{\n"
    "  vec4 result = vec4(linear_from_srgb_f32(v.x),\n"
    "                     linear_from_srgb_f32(v.y),\n"
    "                     linear_from_srgb_f32(v.z),\n"
    "                     v.w);\n"
    "  return result;\n"
    "}\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "\n"
    "  vec4 albedo_sample = vec4(1, 1, 1, 1);\n"
    "  if(v2p_omit_texture < 1)\n"
    "  {\n"
    "    albedo_sample = u_texture_sample_channel_map * texture(u_tex_colour, v2p_texcoord_pct);\n"
    "    albedo_sample = linear_from_srgba(albedo_sample);\n"
    "  }\n"
    "  \n"
    "\n"
    "  float border_sdf_t = 1;\n"
    "  if(v2p_border_thickness > 0)\n"
    "  {\n"
    "    float border_sdf_s = rect_sdf(v2p_sdf_sample_pos,\n"
    "                                  v2p_rect_half_size_px - vec2(v2p_softness*2.f, v2p_softness*2.f) - v2p_border_thickness,\n"
    "                                  max(v2p_corner_radius-v2p_border_thickness, 0));\n"
    "    border_sdf_t = smoothstep(0, 2*v2p_softness, border_sdf_s);\n"
    "  }\n"
    "  if(border_sdf_t < 0.001f)\n"
    "  {\n"
    "    discard;\n"
    "  }\n"
    "  \n"
    "\n"
    "  float corner_sdf_t = 1;\n"
    "  if(v2p_corner_radius > 0 || v2p_softness > 0.75f)\n"
    "  {\n"
    "    float corner_sdf_s = rect_sdf(v2p_sdf_sample_pos,\n"
    "                                  v2p_rect_half_size_px - vec2(v2p_softness*2.f, v2p_softness*2.f),\n"
    "                                  v2p_corner_radius);\n"
    "    corner_sdf_t = 1-smoothstep(0, 2*v2p_softness, corner_sdf_s);\n"
    "  }\n"
    "  \n"
    "\n"
    "  final_colour = albedo_sample;\n"
    "  final_colour *= v2p_tint;\n"
    "  final_colour.a *= u_opacity;\n"
    "  final_colour.a *= corner_sdf_t;\n"
    "  final_colour.a *= border_sdf_t;\n"
    "}\n"
    ""
);

global Str8 OGL_BLUR_VSHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "#version 330 core\n"
    "\n"
    "uniform vec4 rect;\n"
    "uniform vec4 corner_radii_px;\n"
    "uniform vec2 viewport_size;\n"
    "\n"
    "out vec2 texcoord;\n"
    "out vec2 sdf_sample_pos;\n"
    "out vec2 rect_half_size;\n"
    "out float corner_radius;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  vec2 vertex_positions_scrn[] = vec2[](rect.xw,\n"
    "                                        rect.xy,\n"
    "                                        rect.zw,\n"
    "                                        rect.zy);\n"
    "  float corner_radii[] = float[](corner_radii_px.y,\n"
    "                                 corner_radii_px.x,\n"
    "                                 corner_radii_px.w,\n"
    "                                 corner_radii_px.z);\n"
    "  vec2 cornercoords_pct = vec2((gl_VertexID >> 1) != 0 ? 1.f : 0.f,\n"
    "                               (gl_VertexID & 1) != 0  ? 0.f : 1.f);\n"
    "  \n"
    "  vec2 vertex_position_pct = vertex_positions_scrn[gl_VertexID] / viewport_size;\n"
    "  vec2 vertex_position_scr = 2.f * vertex_position_pct - 1.f;\n"
    "  \n"
    "  vec2 rect_half_size_local = vec2((rect.z-rect.x)/2, (rect.w-rect.y)/2);\n"
    "  \n"
    "  gl_Position = vec4(vertex_position_scr.x, -vertex_position_scr.y, 0.f, 1.f);\n"
    "  texcoord = vec2(vertex_position_pct.x, 1-vertex_position_pct.y);\n"
    "  sdf_sample_pos = (2.f * cornercoords_pct - 1.f) * rect_half_size_local;\n"
    "  rect_half_size = rect_half_size_local - 2.f;\n"
    "  corner_radius = corner_radii[gl_VertexID];\n"
    "}\n"
    ""
);

global Str8 OGL_BLUR_PSHADER_SOURCE = Str8Lit(
    ""
    "\n"
    "#version 330 core\n"
    "\n"
    "uniform sampler2D tex;\n"
    "uniform vec4 kernel[32];\n"
    "uniform int blur_count;\n"
    "uniform vec2 direction;\n"
    "\n"
    "in vec2 texcoord;\n"
    "in vec2 sdf_sample_pos;\n"
    "in vec2 rect_half_size;\n"
    "in float corner_radius;\n"
    "\n"
    "out vec4 final_colour;\n"
    "\n"
    "float rect_sdf(vec2 sample_pos, vec2 rect_half_size, float r)\n"
    "{\n"
    "  return length(max(abs(sample_pos) - rect_half_size + r, 0.0)) - r;\n"
    "}\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "\n"
    "  vec3 colour = kernel[0].x * texture(tex, texcoord).rgb;\n"
    "  for(int i = 1; i < blur_count; i += 1)\n"
    "  {\n"
    "    float weight = kernel[i].x;\n"
    "    float offset = kernel[i].y;\n"
    "    colour += weight * texture(tex, texcoord - offset * direction).rgb;\n"
    "    colour += weight * texture(tex, texcoord + offset * direction).rgb;\n"
    "  }\n"
    "  \n"
    "\n"
    "  float corner_sdf_s = rect_sdf(sdf_sample_pos, rect_half_size, corner_radius);\n"
    "  float corner_sdf_t = 1-smoothstep(0, 2, corner_sdf_s);\n"
    "  \n"
    "\n"
    ""
    "  if(corner_sdf_t < 0.9f)\n"
    "  {\n"
    "    discard;\n"
    "  }\n"
    "  \n"
    "  final_colour = vec4(colour, 1.f);\n"
    "}\n"
    ""
);

Str8 OGL_SHADER_KIND_NAME_TABLE[2] = {
    Str8Lit("rect"),
    Str8Lit("blur")
};

Str8* OGL_SHADER_KIND_VSHADER_SRC_TABLE[2] = {
    &OGL_RECT_VSHADER_SOURCE,
    &OGL_BLUR_VSHADER_SOURCE
};

Str8* OGL_SHADER_KIND_PSHADER_SRC_TABLE[2] = {
    &OGL_RECT_PSHADER_SOURCE,
    &OGL_BLUR_PSHADER_SOURCE
};

OGLAttributeArray OGL_SHADER_KIND_INPUT_ATTRIBUTES_TABLE[2] = {
    {
        OGL_RECT_INPUT_ATTRIBUTES,
        ARRAY_COUNT(OGL_RECT_INPUT_ATTRIBUTES)
    },
    {0}
};

OGLAttributeArray OGL_SHADER_KIND_OUTPUT_ATTRIBUTES_TABLE[2] = {
    {
        OGL_SINGLE_COLOUR_OUTPUT_ATTRIBUTES,
        ARRAY_COUNT(OGL_SINGLE_COLOUR_OUTPUT_ATTRIBUTES)
    },
    {
        OGL_SINGLE_COLOUR_OUTPUT_ATTRIBUTES,
        ARRAY_COUNT(OGL_SINGLE_COLOUR_OUTPUT_ATTRIBUTES)
    }
};
EXTERN_C_LINK_END

RendHandle
RendHandleFromTex2D(OGLTex2D* Tex)
{
    RendHandle Result = {
        INT_FROM_PTR(Tex)
    };

    return Result;
}

OGLTex2D* 
RendTex2DFromHandle(RendHandle Handle)
{
    OGLTex2D* Result = (OGLTex2D*) PTR_FROM_INT(Handle.U64[0]);

    return Result;
}

OGLFormatInfo 
OGLFormatInfoFromTex2DFmt(RendTex2DFmt Format)
{
    OGLFormatInfo Result = {};

    Result.InternalFormat = GL_RGBA;
    Result.Format = GL_RGBA;
    Result.BaseType = GL_UNSIGNED_BYTE;

    return Result;
}

GLuint 
OGLInstanceBufferFromSize(u64 Size)
{
    GLuint Buffer = OGL_STATE->ScratchBuffer64KB;

    if (Size > KB(64)) {
        u64 FlushedBufferSize = Size;

        FlushedBufferSize += MB(1) - 1;
        FlushedBufferSize -= FlushedBufferSize % MB(1);
        glGenBuffers(1, &Buffer);
        glBindBuffer(GL_ARRAY_BUFFER, Buffer);
        glBufferData(GL_ARRAY_BUFFER, FlushedBufferSize, 0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        OGLFlushBuffer* FlushBuffer = ArenaPushArrayZero(
            OGL_STATE->BufferFlushMemPool,
            OGLFlushBuffer,
            1
        );

        FlushBuffer->ID = Buffer;
        SLL_QUEUE_PUSH(
            OGL_STATE->HeadBufferToFlush,
            OGL_STATE->TailBufferToFlush,
            FlushBuffer
        );
    }

    return Buffer;
}

void 
OGLDebugMessageFunc(
    GLenum Source, 
    GLenum Tupe, 
    GLuint ID, 
    GLenum Severity, 
    GLsizei Length, 
    GLchar const* Message, 
    void const* UserParam
) {
    LogError(
        "[OPENGL] %.*s\n",
        (int) Length,
        Message
    );
}

INTERNAL b32
OGLScissor(r2f32 Clip, v2f32 ViewPortLength)
{
    i32 X0 = CLAMP(0, (i32) FloorF32(Clip.X0), (i32) ViewPortLength.X);
    i32 Y0 = CLAMP(0, (i32) FloorF32(Clip.Y0), (i32) ViewPortLength.Y);
    i32 X1 = CLAMP(0, (i32) CeilF32(Clip.X1), (i32) ViewPortLength.X);
    i32 Y1 = CLAMP(0, (i32) CeilF32(Clip.Y1), (i32) ViewPortLength.Y);
    i32 Width = X1 - X0;
    i32 Height = Y1 - Y0;

    if (Width > 0 && Height > 0)
        glScissor(X0, (i32) ViewPortLength.Y - Y1, Width, Height);
    else
        glScissor(0, 0, 0, 0);

    b32 Result = (Width > 0 || Height > 0);

    return Result;
}

void 
RendInit(CommandLine* CLI)
{
    OGLInit(CLI);

    Arena* MemPool = ArenaAlloc();

    OGL_STATE = ArenaPushArrayZero(
        MemPool,
        OGLState,
        1
    );
    OGL_STATE->MemPool = MemPool;

#define X(Name, R, P) Name = (Name##FuncType*) OGLLoadProcedure(#Name);
    OGL_PROC_XLIST
#undef X

    for EACH_ENUM(OGLShaderKind, K, SHADER_KIND_COUNT) {
        struct {
            GLenum Type;
            Str8* Source;
            GLuint Result;
            Str8 Errors;
        } Stages[] = {
            {
                GL_VERTEX_SHADER,
                OGL_SHADER_KIND_VSHADER_SRC_TABLE[K]
            },
            {
                GL_FRAGMENT_SHADER,
                OGL_SHADER_KIND_PSHADER_SRC_TABLE[K]
            }
        };

        for (u64 Index = 0; Index < ARRAY_COUNT(Stages); ++Index) {
            Stages[Index].Result = glCreateShader(Stages[Index].Type);

            GLint SourceSize = Stages[Index].Source->Size;

            glShaderSource(
                Stages[Index].Result, 
                1, 
                (char**) &Stages[Index].Source->Str, 
                &SourceSize
            );
            glCompileShader(Stages[Index].Result);

            GLint InfoLogLength = 0;
            GLint FinalStatus = 0;

            glGetShaderiv(
                Stages[Index].Result, 
                GL_COMPILE_STATUS, 
                &FinalStatus
            );
            glGetShaderiv(
                Stages[Index].Result,
                GL_INFO_LOG_LENGTH,
                &InfoLogLength
            );

            if (InfoLogLength > 0) {
                Stages[Index].Errors.Str = ArenaPushArrayZero(
                    OGL_STATE->MemPool,
                    u8,
                    InfoLogLength + 1
                );
                Stages[Index].Errors.Size = InfoLogLength;
                glGetShaderInfoLog(
                    Stages[Index].Result,
                    InfoLogLength,
                    0,
                    (char*) Stages[Index].Errors.Str
                );
            }
        }

        GLuint Program = glCreateProgram();

        for (u64 Index = 0; Index < ARRAY_COUNT(Stages); ++Index)
            glAttachShader(Program, Stages[Index].Result);

        OGLAttributeArray Inputs = OGL_SHADER_KIND_INPUT_ATTRIBUTES_TABLE[K];

        for (u64 Index = 0; Index < Inputs.Count; ++Index) {
            glBindAttribLocation(
                Program,
                Inputs.V[Index].Index,
                (char*) Inputs.V[Index].Name.Str
            );
        }

        OGLAttributeArray Outputs = OGL_SHADER_KIND_OUTPUT_ATTRIBUTES_TABLE[K];

        for (u64 Index = 0; Index < Outputs.Count; ++Index) {
            glBindFragDataLocation(
                Program,
                Outputs.V[Index].Index,
                (char*) Outputs.V[Index].Name.Str
            );
        }

        glLinkProgram(Program);
        glValidateProgram(Program);
        OGL_STATE->Shaders[K] = Program;
    }

    {
        glGenVertexArrays(1, &OGL_STATE->AllPurposeVAO);

        {
            glGenBuffers(1, &OGL_STATE->ScratchBuffer64KB);
            glBindBuffer(GL_ARRAY_BUFFER, OGL_STATE->ScratchBuffer64KB);
            glBufferData(GL_ARRAY_BUFFER, KB(64), 0, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        {
            glGenTextures(1, &OGL_STATE->WhiteTexture);
            glBindTexture(GL_TEXTURE_2D, OGL_STATE->WhiteTexture);

            u32 WhitePixel = 0xFFFFFFFF;

            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RGBA,
                1,
                1,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                &WhitePixel
            );
        }
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
    OGL_STATE->BufferFlushMemPool = ArenaAlloc();

    b32 DebugMode = CommandLineHasFlag(
        CLI,
        "opengl_debug"_s8
    );

#if defined(NC_DEBUG)
    DebugMode = TRUE;
#endif

    if (DebugMode) {
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(OGLDebugMessageFunc, 0);
    }
}

RendHandle 
RendEquipWindow(Handle WindowHandle)
{
    OGLWindow* Window = OGL_STATE->FreeWindow;

    if (Window)
        SLL_STACK_POP(OGL_STATE->FreeWindow);
    else
        Window = ArenaPushArray(OGL_STATE->MemPool, OGLWindow, 1);

    MemZero(Window, sizeof(*Window));
    Window->Handle = OGLEquipWindow(WindowHandle);

    RendHandle Result = {
        INT_FROM_PTR(Window)
    };

    return Result;
}

void 
RendUnequipWindow(Handle WindowHandle, RendHandle Renderer)
{
    OGLWindow* Window = (OGLWindow*) PTR_FROM_INT(Renderer.U64[0]);

    if (Window) {
        OGLUnequipWindow(WindowHandle, Window->Handle);
        SLL_STACK_PUSH(OGL_STATE->FreeWindow, Window);
    }
}

RendHandle 
Tex2DAlloc(
    RendResourceKind Kind, 
    v2i32 Size, 
    RendTex2DFmt Format, 
    void* Data
) {
    OGLTex2D* Tex2D = OGL_STATE->FreeTex2D;

    if (Tex2D)
        SLL_STACK_POP(OGL_STATE->FreeTex2D);
    else
        Tex2D = ArenaPushArrayZero(OGL_STATE->MemPool, OGLTex2D, 1);

    OGLFormatInfo FormatInfo = OGLFormatInfoFromTex2DFmt(Format);

    {
        glGenTextures(1, &Tex2D->ID);
        glBindTexture(GL_TEXTURE_2D, Tex2D->ID);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            FormatInfo.InternalFormat,
            Size.X,
            Size.Y,
            0,
            FormatInfo.Format,
            FormatInfo.BaseType,
            Data
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    Tex2D->ResourceKind = Kind;
    Tex2D->Format = Format;
    Tex2D->Size = Size;

    RendHandle Result = RendHandleFromTex2D(Tex2D);

    return Result;
}

void 
Tex2DRelease(RendHandle Texture)
{
    OGLTex2D* Tex2D = RendTex2DFromHandle(Texture);

    if (Tex2D) {
        glDeleteTextures(1, &Tex2D->ID);
        SLL_STACK_PUSH(OGL_STATE->FreeTex2D, Tex2D);
    }
}

RendResourceKind 
RendResourceKindFromTex2D(RendHandle Texture)
{
    RendResourceKind Result = RESOURCE_KIND_STATIC;
    OGLTex2D* Tex2D = RendTex2DFromHandle(Texture);

    if (Tex2D)
        Result = Tex2D->ResourceKind;

    return Result;
}

v2i32 
SizeFromTex2D(RendHandle Texture)
{
    v2i32 Result = Vec((i32) 0, (i32) 0);
    OGLTex2D* Tex2D = RendTex2DFromHandle(Texture);

    if (Tex2D)
        Result = Tex2D->Size;

    return Result;
}

RendTex2DFmt 
Tex2DFmtFromTex2D(RendHandle Texture)
{
    RendTex2DFmt Result = TEX2D_FMT_RGBA8;
    OGLTex2D* Tex2D = RendTex2DFromHandle(Texture);

    if (Tex2D)
        Result = Tex2D->Format;

    return Result;
}

void 
FillTex2DRgn(RendHandle Texture, r2i32 SubRect, void* Data)
{
    OGLTex2D* Tex2D = RendTex2DFromHandle(Texture);

    if (Tex2D) {
        OGLFormatInfo FormatInfo = OGLFormatInfoFromTex2DFmt(Tex2D->Format);

        glBindTexture(GL_TEXTURE_2D, Tex2D->ID);

        v2i32 RectLength = Length(SubRect);

        glTexSubImage2D(
            GL_TEXTURE_2D,
            0,
            SubRect.X0,
            SubRect.Y0,
            RectLength.X,
            RectLength.Y,
            FormatInfo.Format,
            FormatInfo.BaseType,
            Data
        );
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

RendHandle 
RendBufferAlloc(RendResourceKind Kind, u64 Size, void* Data)
{
    RendHandle Result = {};

    return Result;
}

void 
RendBufferRelease(RendHandle Buffer)
{}

void 
RendBeginFrame(void)
{}

void 
RendEndFrame(void)
{
    for (
        OGLFlushBuffer* FlushBuffer = OGL_STATE->HeadBufferToFlush;
        FlushBuffer;
        FlushBuffer = FlushBuffer->Next
    ) {
        glDeleteBuffers(1, &FlushBuffer->ID);
    }

    ArenaClear(OGL_STATE->BufferFlushMemPool);
    OGL_STATE->HeadBufferToFlush = NULL;
    OGL_STATE->TailBufferToFlush = NULL;
}

void 
RendBeginWindowFrame(Handle WindowHandle, RendHandle Renderer)
{
    OGLSelectWindow(WindowHandle, Renderer);

    OGLWindow* Window = (OGLWindow*) PTR_FROM_INT(Renderer.U64[0]);
    r2f32 ClientRect = GetWindowClientRect(WindowHandle);
    v2f32 ClientRectLength = Length(ClientRect);

    if (
        ClientRectLength.X != Window->PrevClientRectLength.X ||
        ClientRectLength.Y != Window->PrevClientRectLength.Y
    ) {
        Window->PrevClientRectLength = ClientRectLength;

        OGLRenderTarget* Targets[] = {
            &Window->StageTarget,
            &Window->StageScratchTarget
        };

        for (u64 Index = 0; Index < ARRAY_COUNT(Targets); ++Index) {
            if (Targets[Index]->FBO) {
                glDeleteFramebuffers(1, &Targets[Index]->FBO);
                glDeleteTextures(1, &Targets[Index]->ColourTexture);
            }

            glGenFramebuffers(1, &Targets[Index]->FBO);
            glGenTextures(1, &Targets[Index]->ColourTexture);
            
            GL_BIND_FRAME_BUFFER_SCOPE(GL_FRAMEBUFFER, Targets[Index]->FBO) {
                GL_BIND_TEXTURE_SCOPE(
                    GL_TEXTURE_2D, 
                    Targets[Index]->ColourTexture
                ) {
                    glTexImage2D(
                        GL_TEXTURE_2D,
                        0,
                        GL_RGBA32F,
                        (i32) ClientRectLength.X,
                        (i32) ClientRectLength.Y,
                        0,
                        GL_RGBA,
                        GL_FLOAT,
                        0
                    );
                    glFramebufferTexture2D(
                        GL_FRAMEBUFFER,
                        GL_COLOR_ATTACHMENT0,
                        GL_TEXTURE_2D,
                        Targets[Index]->ColourTexture,
                        0
                    );
                }
            }
        }
    }

    {
        GLuint FBOs[] = {
            Window->StageTarget.FBO,
            Window->StageScratchTarget.FBO,
            0
        };

        for (u64 Index = 0; Index < ARRAY_COUNT(FBOs); ++Index) {
            GL_BIND_FRAME_BUFFER_SCOPE(GL_FRAMEBUFFER, FBOs[Index]) {
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }
    }

    glViewport(0, 0, (i32) ClientRectLength.X, (i32) ClientRectLength.Y);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void 
RendEndWindowFrame(Handle WindowHandle, RendHandle Renderer)
{
    OGLWindow* Window = (OGLWindow*) PTR_FROM_INT(Renderer.U64[0]);

    GL_BIND_FRAME_BUFFER_SCOPE(GL_READ_FRAMEBUFFER, Window->StageTarget.FBO) {
        GL_BIND_FRAME_BUFFER_SCOPE(GL_DRAW_FRAMEBUFFER, 0) {
            glBlitFramebuffer(
                0,
                0,
                (i32) Window->PrevClientRectLength.X,
                (i32) Window->PrevClientRectLength.Y,
                0,
                0,
                (i32) Window->PrevClientRectLength.X,
                (i32) Window->PrevClientRectLength.Y,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST
            );
        }
    }

    OGLSwapWindow(WindowHandle, Renderer);
}

void 
RendSubmitWindow(
    Handle WindowHandle, 
    RendHandle Renderer, 
    RendPassList* Passes
) {
    OGLWindow* Window = (OGLWindow*) PTR_FROM_INT(Renderer.U64[0]);
    r2f32 ViewPortRect = GetWindowClientRect(WindowHandle);
    v2f32 ViewPortRectLength = Length(ViewPortRect);

    for (
        RendPassNode* Node = Passes->Head;
        Node;
        Node = Node->Next
    ) {
        RendPass* Pass = &Node->V;

        switch (Pass->Kind) {
            default: {} break;

            case PASS_KIND_UI: {
                RendPassUIParams* Params = Pass->UIParams;
                RendBatch2DList* Rects = &Params->Rects;
                GLuint Shader = OGL_STATE->Shaders[SHADER_KIND_RECT];

                GL_USE_PROGRAM_SCOPE(Shader) {
                    GL_BIND_VERTEX_ARRAY_SCOPE(OGL_STATE->AllPurposeVAO) {
                        GL_BIND_FRAME_BUFFER_SCOPE(
                            GL_FRAMEBUFFER, 
                            Window->StageTarget.FBO
                        ) {
                            for (
                                RendBatch2DNode* BatchNode = Rects->Head;
                                BatchNode;
                                BatchNode = BatchNode->Next
                            ) {
                                RendBatchList* Batches = &BatchNode->Batches;
                                RendBatch2DParams* BatchParams = &BatchNode->Params;
                                RendTex2DFmt TextureFmt = TEX2D_FMT_RGBA8;
                                GLuint TextureID = OGL_STATE->WhiteTexture;

                                {
                                    OGLTex2D* Tex2D = RendTex2DFromHandle(
                                        BatchParams->Texture
                                    );

                                    if (Tex2D) {
                                        TextureID = Tex2D->ID;
                                        TextureFmt = Tex2D->Format;
                                    }
                                }

                                GLuint Buffer = OGLInstanceBufferFromSize(
                                    Batches->ByteCount
                                );

                                {
                                    glBindBuffer(GL_ARRAY_BUFFER, Buffer);

                                    u64 Offset = 0;

                                    for (
                                        RendBatchNode* BNode = Batches->Head;
                                        BNode;
                                        BNode = BNode->Next
                                    ) {
                                        glBufferSubData(
                                            GL_ARRAY_BUFFER,
                                            Offset,
                                            BNode->V.ByteCount,
                                            BNode->V.V
                                        );
                                        Offset += BNode->V.ByteCount;
                                    }
                                }

                                {
                                    OGLAttributeArray Inputs = OGL_SHADER_KIND_INPUT_ATTRIBUTES_TABLE[SHADER_KIND_RECT];
                                    u64 Offset = 0;

                                    for (
                                        u64 Index = 0; 
                                        Index < Inputs.Count; 
                                        ++Index
                                    ) {
                                        glEnableVertexAttribArray(
                                            Inputs.V[Index].Index
                                        );
                                        glVertexAttribDivisor(
                                            Inputs.V[Index].Index,
                                            1
                                        );
                                        glVertexAttribPointer(
                                            Inputs.V[Index].Index,
                                            Inputs.V[Index].Count,
                                            Inputs.V[Index].Type,
                                            GL_FALSE,
                                            sizeof(Rect2DInstr),
                                            (void*) Offset
                                        );

                                        // WARN(nc): can we guarantee always f32?
                                        Offset += Inputs.V[Index].Count * sizeof(f32);
                                    }
                                }

                                {
                                    glActiveTexture(GL_TEXTURE0);
                                    glBindTexture(GL_TEXTURE_2D, TextureID);
                                    glTexParameteri(
                                        GL_TEXTURE_2D,
                                        GL_TEXTURE_WRAP_S,
                                        GL_REPEAT
                                    );
                                    glTexParameteri(
                                        GL_TEXTURE_2D,
                                        GL_TEXTURE_WRAP_T,
                                        GL_REPEAT
                                    );

                                    switch (BatchParams->SampleKind) {
                                        default:
                                        case TEX2D_SAMPLE_NEAREST: {
                                            glTexParameteri(
                                                GL_TEXTURE_2D,
                                                GL_TEXTURE_MIN_FILTER,
                                                GL_NEAREST
                                            );
                                            glTexParameteri(
                                                GL_TEXTURE_2D,
                                                GL_TEXTURE_MAG_FILTER,
                                                GL_NEAREST
                                            );
                                        } break;

                                        case TEX2D_SAMPLE_LINEAR: {
                                            glTexParameteri(
                                                GL_TEXTURE_2D,
                                                GL_TEXTURE_MIN_FILTER,
                                                GL_LINEAR
                                            );
                                            glTexParameteri(
                                                GL_TEXTURE_2D,
                                                GL_TEXTURE_MAG_FILTER,
                                                GL_LINEAR
                                            );
                                        } break;
                                    }

                                    glTexParameteri(
                                        GL_TEXTURE_2D,
                                        GL_TEXTURE_MAX_LEVEL,
                                        0
                                    );
                                    glUniform1i(
                                        glGetUniformLocation(
                                            Shader, 
                                            "u_tex_colour"
                                        ),
                                        0
                                    );
                                }

                                {
                                    m4f32 TexSampleChannelMap = SampleChannelMapFromTex2DFmt(
                                        TextureFmt
                                    );

                                    glUniformMatrix4fv(
                                        glGetUniformLocation(
                                            Shader,
                                            "u_texture_sample_channel_map"
                                        ),
                                        1,
                                        0,
                                        &TexSampleChannelMap.V[0][0]
                                    );
                                    glUniform2f(
                                        glGetUniformLocation(
                                            Shader,
                                            "u_viewport_size_px"
                                        ),
                                        ViewPortRectLength.X,
                                        ViewPortRectLength.Y
                                    );
                                    glUniform1f(
                                        glGetUniformLocation(
                                            Shader,
                                            "u_opacity"
                                        ),
                                        1.0f - BatchParams->Transparency
                                    );
                                    glUniformMatrix3fv(
                                        glGetUniformLocation(
                                            Shader,
                                            "u_xform"
                                        ),
                                        1,
                                        0,
                                        &BatchParams->XForm.V[0][0]
                                    );
                                }

                                if (
                                    BatchParams->Clip.X0 != 0.0f ||
                                    BatchParams->Clip.X1 != 0.0f ||
                                    BatchParams->Clip.Y0 != 0.0f ||
                                    BatchParams->Clip.Y1 != 0.0f
                                ) {
                                    if (
                                        OGLScissor(
                                            BatchParams->Clip, 
                                            ViewPortRectLength
                                        )
                                    ) {
                                        glEnable(GL_SCISSOR_TEST);
                                    }
                                }

                                {
                                    glDrawArraysInstanced(
                                        GL_TRIANGLE_STRIP,
                                        0,
                                        4,
                                        Batches->ByteCount / Batches->BytesPerInstruction
                                    );
                                }

                                glDisable(GL_SCISSOR_TEST);
                            }
                        }
                    }
                }
            } break;

            case PASS_KIND_BLUR: {
                RendPassBlurParams* Params = Pass->BlurParams;
                GLuint Shader = OGL_STATE->Shaders[SHADER_KIND_BLUR];

                GL_USE_PROGRAM_SCOPE(Shader) {
                    GL_BIND_VERTEX_ARRAY_SCOPE(OGL_STATE->AllPurposeVAO) {
                        v4f32 Kernel[32] = {};
                        u64 BlurCount = 0;

                        {
                            f32 Weights[ARRAY_COUNT(Kernel) * 2] = {};
                            f32 BlurSize = MIN(
                                Params->BlurSize,
                                ARRAY_COUNT(Weights)
                            );

                            BlurCount = (u64) RoundF32(BlurSize);

                            f32 StdDev = (BlurSize - 1.0f) / 2.0f;
                            f32 OneOverRoot2PIStdDev2 = 1.0f / SqrtF32(2 * NC_PI32 * StdDev * StdDev);
                            
                            Weights[0] = 1.0f;

                            if (StdDev > 0.0f) {
                                for (
                                    u64 Index = 0; 
                                    Index < BlurCount; 
                                    ++Index
                                ) {
                                    f32 KernelX = (f32) Index;

                                    Weights[Index] = OneOverRoot2PIStdDev2 * PowF32(NC_EULER32, -KernelX * KernelX / (2.0f * StdDev * StdDev));
                                }
                            }

                            if (Weights[0] > 1.0f) {
                                MemZero(Weights, sizeof(Weights));
                                Weights[0] = 1.0f;
                            } else {
                                for (
                                    u64 Index = 1; 
                                    Index < BlurCount; 
                                    Index += 2
                                ) {
                                    f32 W0 = Weights[Index + 0];
                                    f32 W1 = Weights[Index + 1];
                                    f32 W = W0 + W1;
                                    f32 T = W1 / W;

                                    Kernel[(Index + 1) / 2] = Vec(
                                        W,
                                        (f32) Index + T,
                                        0.0f,
                                        0.0f
                                    );
                                }
                            }

                            Kernel[0].X = Weights[0];
                        }

                        glUniform4fv(
                            glGetUniformLocation(
                                Shader, "rect"
                            ),
                            1,
                            (f32*) (&Params->Rect)
                        );
                        glUniform4fv(
                            glGetUniformLocation(
                                Shader, 
                                "corner_radii_px"
                            ),
                            1,
                            Params->CornerRadii
                        );
                        glUniform2f(
                            glGetUniformLocation(
                                Shader, 
                                "viewport_size"
                            ),
                            ViewPortRectLength.X, 
                            ViewPortRectLength.Y
                        );
                        glUniform1i(
                            glGetUniformLocation(
                                Shader, 
                                "blur_count"
                            ),
                            BlurCount
                        );
                        glUniform4fv(
                            glGetUniformLocation(
                                Shader, 
                                "kernel"
                            ), 
                            ARRAY_COUNT(Kernel), 
                            (f32*) &Kernel[0]
                        );
                        glActiveTexture(GL_TEXTURE0);

                        if (
                            Params->Clip.X0 != 0.0f ||
                            Params->Clip.X1 != 0.0f ||
                            Params->Clip.Y0 != 0.0f ||
                            Params->Clip.Y1 != 0.0f
                        ) {
                            if (
                                OGLScissor(
                                    Params->Clip,
                                    ViewPortRectLength
                                )
                            ) {
                                glEnable(GL_SCISSOR_TEST);
                            }
                        }

                        GL_BIND_FRAME_BUFFER_SCOPE(
                            GL_FRAMEBUFFER,
                            Window->StageScratchTarget.FBO
                        ) {
                            GL_BIND_TEXTURE_SCOPE(
                                GL_TEXTURE_2D,
                                Window->StageTarget.ColourTexture
                            ) {
                                glTexParameteri(
                                    GL_TEXTURE_2D, 
                                    GL_TEXTURE_MIN_FILTER, 
                                    GL_NEAREST
                                );
                                glTexParameteri(
                                    GL_TEXTURE_2D, 
                                    GL_TEXTURE_MAG_FILTER, 
                                    GL_NEAREST
                                );
                                glUniform2f(
                                    glGetUniformLocation(
                                        Shader, 
                                        "direction"
                                    ), 
                                    1.0f / ViewPortRectLength.X, 
                                    0
                                );
                                glUniform1i(
                                    glGetUniformLocation(
                                        Shader, 
                                        "tex"
                                    ),
                                    0
                                );
                                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                            }
                        }

                        GL_BIND_FRAME_BUFFER_SCOPE(
                            GL_FRAMEBUFFER,
                            Window->StageTarget.FBO
                        ) {
                            GL_BIND_TEXTURE_SCOPE(
                                GL_TEXTURE_2D,
                                Window->StageScratchTarget.ColourTexture
                            ) {
                                glTexParameteri(
                                    GL_TEXTURE_2D, 
                                    GL_TEXTURE_MIN_FILTER, 
                                    GL_NEAREST
                                );
                                glTexParameteri(
                                    GL_TEXTURE_2D, 
                                    GL_TEXTURE_MAG_FILTER, 
                                    GL_NEAREST
                                );
                                glUniform2f(
                                    glGetUniformLocation(
                                        Shader, 
                                        "direction"
                                    ),
                                    0,
                                    1.0f / ViewPortRectLength.Y
                                );
                                glUniform1i(
                                    glGetUniformLocation(
                                        Shader, 
                                        "tex"
                                    ),
                                    0
                                );
                                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                            }
                        }

                        glDisable(GL_SCISSOR_TEST);
                    }
                }
            } break;

            case PASS_KIND_GEO3D: {
                RendPassGeo3DParams* Params = Pass->Geo3DParams;
                RendBatch3DMap* MeshMap = &Params->Batches;

                // TODO(nc): this
            } break;
        }
    }
}
