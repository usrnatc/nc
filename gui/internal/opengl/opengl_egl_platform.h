#if !defined(__OPENGL_EGL_PLATFORM_H__)
#define __OPENGL_EGL_PLATFORM_H__

// @defines____________________________________________________________________
#define glTexImage3D    glTexImage3D__static
#define glTexSubImage3D glTexSubImage3D__static
#define glActiveTexture glActiveTexture__static

#include <GL/gl.h>
#include <EGL/egl.h>

#undef glTexImage3D
#undef glTexSubImage3D
#undef glActiveTexture

// @types______________________________________________________________________
struct OGLEGLWindow {
    OGLEGLWindow*  Next;
    EGLSurface* Surface;
};

struct OGLEGLState {
    Arena*      MemPool;
    EGLDisplay* DisplayHandle;
    EGLConfig   Config;
    EGLContext* Context;
    OGLEGLWindow*  FreeWindow;
};

// @runtime____________________________________________________________________
extern OGLEGLState* OGL_EGL_STATE;

// @functions__________________________________________________________________

#endif // __OPENGL_EGL_PLATFORM_H__
