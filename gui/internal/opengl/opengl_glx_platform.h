#if !defined(__OPENGL_GLX_PLATFORM_H__)
#define __OPENGL_GLX_PLATFORM_H__



// @defines_____________________________________________________________________
#define glTexImage3D    glTexImage3D__static
#define glTexSubImage3D glTexSubImage3D__static
#define glActiveTexture glActiveTexture__static

#include <GL/gl.h>
#include <GL/glx.h>

#undef glTexImage3D
#undef glTexSubImage3D
#undef glActiveTexture

#define GLX_CONTEXT_MAJOR_VERSION_ARB          0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB          0x2092
#define GLX_CONTEXT_FLAGS_ARB                  0x2094
#define GLX_CONTEXT_DEBUG_BIT_ARB              0x00000001
#define GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB 0x00000002

typedef GLXContext (*GLXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// @types______________________________________________________________________


// @runtime____________________________________________________________________


// @functions__________________________________________________________________


#endif // __OPENGL_GLX_PLATFORM_H__
