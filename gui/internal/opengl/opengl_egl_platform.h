#if !defined(__OPENGL_EGL_PLATFORM_H__)
#define __OPENGL_EGL_PLATFORM_H__

#include "../nc_types.h"

// @defines____________________________________________________________________
#define glTexImage3D         glTexImage3D__static
#define glTexImageSubImage3D glTexImageSubImage3D__static
#define glActiveTexture      glActiveTexture__static

#include <GL/gl.h>
#include <EGL/egl.h>

#undef glTexImage3D
#undef glTexImageSubImage3D
#undef glActiveTexture

// @types______________________________________________________________________
#if defined(NC_CC_MSVC)
#elif defined(NC_CC_CLANG) || defined(NC_CC_GNU)
    struct OGLWindow {
        OGLWindow*  Next;
        EGLSurface* Surface;
    };

    struct OGLEGLState {
        Arena*      MemPool;
        EGLDisplay* DisplayHandle;
        EGLConfig*  Config;
        EGLContext* Context;
        OGLWindow*  FreeWindow;
    };
#endif

// @runtime____________________________________________________________________
#if defined(NC_CC_MSVC)
#elif defined(NC_CC_CLANG) || defined(NC_CC_GCC)
    extern OGLEGLState* OGL_EGL_STATE;
#endif

// @functions__________________________________________________________________

#endif // __OPENGL_EGL_PLATFORM_H__
