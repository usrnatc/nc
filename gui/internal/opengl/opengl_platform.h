#if !defined(__OPENGL_PLATFORM_H__)
#define __OPENGL_PLATFORM_H__

#include "../nc_types.h"
#include "../nc_render.h"
#include "../nc_string.h"

#define NC_USE_OPENGL_GLX

// @defines____________________________________________________________________
typedef char GLchar;
typedef dptr GLsizeiptr;
typedef dptr GLintptr;

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_TEXTURE_MAX_LEVEL              0x813D
#define GL_R8                             0x8229
#define GL_ARRAY_BUFFER                   0x8892
#define GL_STREAM_DRAW                    0x88E0
#define GL_STREAM_READ                    0x88E1
#define GL_STREAM_COPY                    0x88E2
#define GL_STATIC_DRAW                    0x88E4
#define GL_STATIC_READ                    0x88E5
#define GL_STATIC_COPY                    0x88E6
#define GL_DYNAMIC_DRAW                   0x88E8
#define GL_DYNAMIC_READ                   0x88E9
#define GL_DYNAMIC_COPY                   0x88EA
#define GL_FRAGMENT_SHADER                0x8B30
#define GL_VERTEX_SHADER                  0x8B31
#define GL_TESS_EVALUATION_SHADER         0x8E87
#define GL_TESS_CONTROL_SHADER            0x8E88
#define GL_INFO_LOG_LENGTH                0x8B84
#define GL_TEXTURE_2D_ARRAY               0x8C1A
#define GL_COMPILE_STATUS                 0x8B81
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF
#define GL_DEBUG_OUTPUT                   0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS       0x8242

enum OGLShaderKind {
    SHADER_KIND_RECT,
    SHADER_KIND_BLUR,
    SHADER_KIND_COUNT
};

#if defined(NC_USE_OPENGL_EGL)
    #include "opengl_egl_platform.h"
#elif defined(NC_USE_OPENGL_GLX)
    #include "opengl_glx_platform.h"
#endif

#define OGL_PROC_XLIST                                                                                                                                                                                \
    X(glGenBuffers, void, (GLsizei N, GLuint* Buffers))                                                                                                                                               \
    X(glBindBuffer, void, (GLenum Target, GLuint Buffer))                                                                                                                                             \
    X(glDeleteBuffers, void, (GLsizei N, GLuint* Buffers))                                                                                                                                            \
    X(glGenVertexArrays, void, (GLsizei N, GLuint* Arrays))                                                                                                                                           \
    X(glBindVertexArray, void, (GLuint Array))                                                                                                                                                        \
    X(glCreateProgram, GLuint, (void))                                                                                                                                                                \
    X(glGenFramebuffers, void, (GLsizei N, GLuint* FBOs))                                                                                                                                             \
    X(glDeleteFramebuffers, void, (GLsizei N, GLuint* FBOs))                                                                                                                                          \
    X(glCreateShader, GLuint, (GLenum Type))                                                                                                                                                          \
    X(glShaderSource, void, (GLuint Shader, GLsizei Count, char** String, GLint* Length))                                                                                                             \
    X(glCompileShader, void, (GLuint Shader))                                                                                                                                                         \
    X(glGetShaderiv, void, (GLuint Shader, GLenum PName, GLint* Params))                                                                                                                              \
    X(glGetShaderInfoLog, void, (GLuint Shader, GLsizei BufferSize, GLsizei* Length, char* InfoLog))                                                                                                  \
    X(glGetProgramiv, void, (GLuint Program, GLenum PName, GLint* Params))                                                                                                                            \
    X(glGetProgramInfoLog, void, (GLuint Program, GLsizei BufferSize, GLsizei* Length, char* InfoLog))                                                                                                \
    X(glAttachShader, void, (GLuint Program, GLuint Shader))                                                                                                                                          \
    X(glLinkProgram, void, (GLuint Program))                                                                                                                                                          \
    X(glValidateProgram, void, (GLuint Program))                                                                                                                                                      \
    X(glDeleteShader, void, (GLuint Shader))                                                                                                                                                          \
    X(glUseProgram, void, (GLuint Program))                                                                                                                                                           \
    X(glGetUniformLocation, GLint, (GLuint Program, char* Name))                                                                                                                                      \
    X(glGetAttribLocation, GLint, (GLuint Program, char* Name))                                                                                                                                       \
    X(glEnableVertexAttribArray, void, (GLuint Index))                                                                                                                                                \
    X(glVertexAttribPointer, void, (GLuint Index, GLint Size, GLenum Type, GLboolean Normalised, GLsizei Stride, void const* Pointer))                                                                \
    X(glBufferData, void, (GLenum Target, dptr Size, void* Data, GLenum Usage))                                                                                                                       \
    X(glBufferSubData, void, (GLenum Target, dptr Offset, dptr Size, void const* Data))                                                                                                               \
    X(glBlendFuncSeparate, void, (GLenum SFactorRGB, GLenum DFactorRGB, GLenum SFactorAlpha, GLenum DFactorAlpha))                                                                                    \
    X(glUniform1f, void, (GLint Location, GLfloat V0))                                                                                                                                                \
    X(glUniform2f, void, (GLint Location, GLfloat V0, GLfloat V1))                                                                                                                                    \
    X(glUniform3f, void, (GLint Location, GLfloat V0, GLfloat V1, GLfloat V2))                                                                                                                        \
    X(glUniform4f, void, (GLint Location, GLfloat V0, GLfloat V1, GLfloat V2, GLfloat V3))                                                                                                            \
    X(glUniform4fv, void, (GLint Location, GLsizei Count, GLfloat const* Value))                                                                                                                      \
    X(glUniformMatrix3fv, void, (GLint Location, GLsizei Count, GLboolean Transpose, GLfloat const* Value))                                                                                           \
    X(glUniformMatrix4fv, void, (GLint Location, GLsizei Count, GLboolean Transpose, GLfloat const* Value))                                                                                           \
    X(glUniform1i, void, (GLint Location, GLint V0))                                                                                                                                                  \
    X(glTexImage3D, void, (GLenum Target, GLint Level, GLint InternalFormat, GLsizei Width, GLsizei Height, GLsizei Depth, GLint Border, GLenum Format, GLenum Type, void const* Pixels))             \
    X(glTexSubImage3D, void, (GLenum Target, GLint Level, GLint XOffset, GLint YOffset, GLint ZOffset, GLsizei Width, GLsizei Height, GLsizei Depth, GLenum Format, GLenum Type, void const* Pixels)) \
    X(glGenerateMipmap, void, (GLenum Target))                                                                                                                                                        \
    X(glBindAttribLocation, void, (GLuint ProgramObj, GLuint Index, char* Name))                                                                                                                      \
    X(glBindFramebuffer, void, (GLenum Target, GLuint FBO))                                                                                                                                           \
    X(glBindFragDataLocation, void, (GLuint Program, GLuint Colour, char* Name))                                                                                                                      \
    X(glActiveTexture, void, (GLenum Texture))                                                                                                                                                        \
    X(glVertexAttribDivisor, void, (GLuint Index, GLuint Divisor))                                                                                                                                    \
    X(glDrawArraysInstanced, void, (GLenum Mode, GLint First, GLsizei Count, GLsizei InstanceCount))                                                                                                  \
    X(glDebugMessageCallback, void, (void (*)(GLenum Source, GLenum Type, GLuint ID, GLenum Severity, GLsizei Length, GLchar const* Message, void const* UserParam), void* UserData))                 \
    X(glFramebufferTexture2D, void, (GLenum Target, GLenum Attachment, GLenum TexTarget, GLuint Texture, GLint Level))                                                                                \
    X(glBlitFramebuffer, void, (GLint SrcX0, GLint SrcY0, GLint SrcX1, GLint SrcY1, GLint DstX0, GLint DstY0, GLint DstX1, GLint DstY1, GLbitfield Mask, GLenum filter))                              \

#define X(Y, Z, W) typedef Z Y##FuncType W;
OGL_PROC_XLIST
#undef X

#define GL_USE_PROGRAM_SCOPE(...)          DEFER(glUseProgram(__VA_ARGS__), glUseProgram(0))
#define GL_BIND_VERTEX_ARRAY_SCOPE(...)    DEFER(glBindVertexArray(__VA_ARGS__), glBindVertexArray(0))
#define GL_BIND_FRAME_BUFFER_SCOPE(X, ...) DEFER(glBindFramebuffer((X), __VA_ARGS__), glBindFramebuffer((X), 0))
#define GL_BIND_TEXTURE_SCOPE(X, ...)      DEFER(glBindTexture((X), __VA_ARGS__), glBindTexture((X), 0))
#define GL_ENABLE_SCOPE(...)               DEFER(glEnable(__VA_ARGS__), glDisable(__VA_ARGS__))


// @types______________________________________________________________________
struct OGLAttribute {
    u64    Index;
    Str8   Name;
    GLenum Type;
    u64    Count;
};

struct OGLAttributeArray {
    OGLAttribute* V;
    u64           Count;
};

struct OGLFormatInfo {
    GLint  InternalFormat;
    GLenum Format;
    GLenum BaseType;
};

struct OGLTex2D {
    OGLTex2D*        Next;
    GLuint           ID;
    RendResourceKind ResourceKind;
    RendTex2DFmt     Format;
    v2i32            Size;
};

struct OGLFlushBuffer {
    OGLFlushBuffer* Next;
    GLuint          ID;
};

struct OGLRenderTarget {
    GLuint FBO;
    GLuint ColourTexture;
};

struct OGLWindow {
    OGLWindow*      Next;
    RendHandle      Handle;
    v2f32           PrevClientRectLength;
    OGLRenderTarget StageTarget;
    OGLRenderTarget StageScratchTarget;
};

struct OGLState {
    Arena*          MemPool;
    OGLWindow*      FreeWindow;
    OGLTex2D*       FreeTex2D;
    GLuint          Shaders[SHADER_KIND_COUNT];
    GLuint          AllPurposeVAO;
    GLuint          ScratchBuffer64KB;
    GLuint          WhiteTexture;
    Arena*          BufferFlushMemPool;
    OGLFlushBuffer* HeadBufferToFlush;
    OGLFlushBuffer* TailBufferToFlush;
};

// @runtime____________________________________________________________________
#define X(Y, Z, W) global Y##FuncType* Y = NULL;
OGL_PROC_XLIST
#undef X

EXTERN_C_LINK_BEGIN
extern Str8 OGL_SHADER_KIND_NAME_TABLE[2];
extern Str8* OGL_SHADER_KIND_VSHADER_SRC_TABLE[2];
extern Str8* OGL_SHADER_KIND_PSHADER_SRC_TABLE[2];
extern OGLAttributeArray OGL_SHADER_KIND_INPUT_ATTRIBUTES_TABLE[2];
extern OGLAttributeArray OGL_SHADER_KIND_OUTPUT_ATTRIBUTES_TABLE[2];
EXTERN_C_LINK_END

extern OGLState* OGL_STATE;

// @functions__________________________________________________________________
extern vproc* OGLLoadProcedure(char* Name);
extern void OGLInit(CommandLine* CLI);
extern RendHandle OGLEquipWindow(Handle WindowHandle);
extern void OGLUnequipWindow(Handle WindowHandle, RendHandle Renderer);
extern void OGLSelectWindow(Handle WindowHandle, RendHandle Renderer);
extern void OGLSwapWindow(Handle WindowHandle, RendHandle Renderer);

RendHandle RendHandleFromTex2D(OGLTex2D* Tex);
OGLTex2D* RendTex2DFromHandle(RendHandle Handle);
OGLFormatInfo OGLFormatInfoFromTex2DFmt(RendTex2DFmt Format);
GLuint OGLInstanceBufferFromSize(u64 Size);
void OGLDebugMessageFunc(GLenum Source, GLenum Tupe, GLuint ID, GLenum Severity, GLsizei Length, GLchar const* Message, void const* UserParam);

#endif // __OPENGL_PLATFORM_H__
