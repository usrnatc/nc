#include "../../nc_system.h"
#include "../../nc_render.h"
#include "../../nc_string.h"

#include "opengl_glx_platform.h"
#include "opengl_glx.h"

GLXContext OGLContext = NULL;

vproc* 
OGLLoadProcedure(char* Name)
{
    vproc* Result = (vproc*) glXGetProcAddressARB((u8*) Name);

    return Result;
}

void 
OGLInit(CommandLine* CLI)
{
    int GLXVersionMajor = 0;
    int GLXVersionMinor = 0;

    if (
        !glXQueryVersion(
            GRAPHICS_STATE->Disp,
            &GLXVersionMajor,
            &GLXVersionMinor
        ) ||
        (
            GLXVersionMajor == 1 && GLXVersionMinor < 3
        ) ||
        GLXVersionMajor < 1
    ) {
        TempArena Scratch = GetScratch(NULL, 0);
        Str8 Message = ArenaPushStrFmt(
            Scratch.MemPool,
            "Unsupported GLX version (%i.%i, need at least 1.3)",
            GLXVersionMajor,
            GLXVersionMinor
        );

        GraphicalMessage(TRUE, "Fatal Error"_s8, Message);
        Abort(1);
        ReleaseScratch(Scratch);
    }

    local int FramebufferConfigOptions[] = {
        GLX_X_RENDERABLE,   TRUE,
        GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,    GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
        GLX_RED_SIZE,        8,
        GLX_GREEN_SIZE,      8,
        GLX_BLUE_SIZE,       8,
        GLX_ALPHA_SIZE,      8,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,    8,
        GLX_DOUBLEBUFFER,   TRUE,
        None
    };
    int FramebufferConfigsCount = 0;
    GLXFBConfig* FramebufferConfigs = glXChooseFBConfig(
        GRAPHICS_STATE->Disp,
        DefaultScreen(GRAPHICS_STATE->Disp),
        FramebufferConfigOptions,
        &FramebufferConfigsCount
    );

    if (!FramebufferConfigs) {
        GraphicalMessage(
            TRUE,
            "Fatal Error"_s8,
            "Could not find a suitable framebuffer configuration."_s8
        );
        Abort(1);
    }

    GLXFBConfig FramebufferConfig = *FramebufferConfigs;

    XFree(FramebufferConfigs);

    {
        XVisualInfo* VisualInfo = glXGetVisualFromFBConfig(
            GRAPHICS_STATE->Disp,
            FramebufferConfig
        );

        if (!VisualInfo) {
            GraphicalMessage(
                TRUE,
                "Fatal Error"_s8,
                "Could not get visual from GLX framebuffer config."_s8
            );
            Abort(1);
        }

        GRAPHICS_STATE->WindowVisual = VisualInfo->visual;
        GRAPHICS_STATE->WindowDepth = VisualInfo->depth;
        GRAPHICS_STATE->WindowColourMap = XCreateColormap(
            GRAPHICS_STATE->Disp,
            XRootWindow(GRAPHICS_STATE->Disp, VisualInfo->screen),
            VisualInfo->visual,
            AllocNone
        );
        XFree(VisualInfo);
    }

    {
        b32 DebugMode = CommandLineHasFlag(CLI, "opengl_debug"_s8);

    #if defined(NC_DEBUG)
        DebugMode = TRUE;
    #endif

        GLXCreateContextAttribsARBProc GLXCreateContextAttribsARBFunc = NULL;

        GLXCreateContextAttribsARBFunc = (GLXCreateContextAttribsARBProc) glXGetProcAddressARB(
            (u8*) "glXCreateContextAttribsARB"
        );

        int ContextOptions[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 3,
            GLX_CONTEXT_FLAGS_ARB,         !!DebugMode * GLX_CONTEXT_DEBUG_BIT_ARB,
            None
        };

        OGLContext = GLXCreateContextAttribsARBFunc(
            GRAPHICS_STATE->Disp,
            FramebufferConfig,
            0,
            1,
            ContextOptions
        );
    }

    glXMakeCurrent(GRAPHICS_STATE->Disp, 0, OGLContext);
}

RendHandle 
OGLEquipWindow(Handle WindowHandle)
{
    RendHandle Result = {};

    return Result;
}

void 
OGLUnequipWindow(Handle WindowHandle, RendHandle Renderer)
{}

void 
OGLSelectWindow(Handle WindowHandle, RendHandle Renderer)
{
    LinuxWindow* Window = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    glXMakeCurrent(
        GRAPHICS_STATE->Disp,
        Window->Win,
        OGLContext
    );
    glDrawBuffer(GL_BACK);
}

void 
OGLSwapWindow(Handle WindowHandle, RendHandle Renderer)
{
    LinuxWindow* Window = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    glXSwapBuffers(GRAPHICS_STATE->Disp, Window->Win);
}

