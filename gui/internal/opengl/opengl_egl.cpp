OGLEGLState* OGL_EGL_STATE = NULL;

vproc* 
OGLLoadProcedure(char* Name)
{
    vproc* Result = (vproc*) eglGetProcAddress(Name);

    return Result;
}

void 
OGLInit(CommandLine* CLI)
{
    Arena* MemPool = ArenaAlloc();

    OGL_EGL_STATE = ArenaPushArray(
        MemPool,
        OGLEGLState,
        1
    );
    OGL_EGL_STATE->MemPool = MemPool;
    OGL_EGL_STATE->DisplayHandle = eglGetDisplay(
        (EGLNativeDisplayType) OGL_EGL_STATE->DisplayHandle
    );

    if (OGL_EGL_STATE->DisplayHandle == EGL_NO_DISPLAY) {
        GraphicalMessage(
            TRUE, 
            "Fatal Error"_s8, 
            "Failed to get EGL display."_s8
        );
        Abort(1);
    }

    EGLint EGLVersionMajor = 0;
    EGLint EGLVersionMinor = 0;

    if (
        !eglInitialize(
            OGL_EGL_STATE->DisplayHandle,
            &EGLVersionMajor,
            &EGLVersionMinor
        )
    ) {
        GraphicalMessage(
            TRUE,
            "Fatal Error"_s8,
            "Could not initialise EGL display."_s8
        );
        Abort(1);
    }

    if (EGLVersionMajor < 1 || (EGLVersionMajor == 1 && EGLVersionMinor < 5)) {
        TempArena Scratch = GetScratch(NULL, 0);
        Str8 Message = ArenaPushStrFmt(
            Scratch.MemPool,
            "Unsupported EGL version (%i.%i, need at least 1.5)",
            EGLVersionMajor,
            EGLVersionMinor
        );
        GraphicalMessage(
            TRUE,
            "Fatal Error"_s8,
            Message
        );
        Abort(1);
        ReleaseScratch(Scratch);
    }

    if (!eglBindAPI(EGL_OPENGL_API)) {
        GraphicalMessage(
            TRUE,
            "Fatal Error"_s8,
            "Could not initialise EGL API to OpenGL"_s8
        );
        Abort(1);
    }

    {
        EGLint ConfigFilter[] = {
            EGL_SURFACE_TYPE,      EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
            EGL_CONFORMANT,        EGL_OPENGL_BIT,
            EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
            EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
            EGL_RED_SIZE,          8,
            EGL_GREEN_SIZE,        8,
            EGL_BLUE_SIZE,         8,
            EGL_DEPTH_SIZE,        24,
            EGL_STENCIL_SIZE,      8,
            EGL_NONE
        };
        EGLConfig Configs[256] = {};
        EGLint ConfigsCount = 0;

        if (
            !eglChooseConfig(
                OGL_EGL_STATE->DisplayHandle,
                ConfigFilter,
                Configs,
                ARRAY_COUNT(Configs),
                &ConfigsCount
            ) ||
            !ConfigsCount
        ) {
            GraphicalMessage(
                TRUE,
                "Fatal Error"_s8,
                "Could not choose EGL configuration."_s8
            );
            Abort(1);
        }

        int Screen = DefaultScreen(GRAPHICS_STATE->Disp);
        XVisualInfo VisualInfoTemplate = {};
        XVisualInfo* VisualInfo = NULL;
        int VisualInfoCount = 0;

        for (EGLint Index = 0; Index < ConfigsCount && !VisualInfo; ++Index) {
            EGLint NativeVisualID = 0;

            if (
                !eglGetConfigAttrib(
                    OGL_EGL_STATE->DisplayHandle,
                    Configs[Index],
                    EGL_NATIVE_VISUAL_ID,
                    &NativeVisualID
                ) ||
                !NativeVisualID
            ) {
                continue;
            }

            VisualInfoTemplate.visualid = NativeVisualID;
            VisualInfoTemplate.screen = Screen;
            VisualInfo = XGetVisualInfo(
                GRAPHICS_STATE->Disp,
                VisualIDMask | VisualScreenMask,
                &VisualInfoTemplate,
                &VisualInfoCount
            );

            if (VisualInfo && VisualInfoCount) {
                OGL_EGL_STATE->Config = Configs[Index];
                break;
            }

            if (VisualInfo) {
                XFree(VisualInfo);
                VisualInfo = NULL;
            }
        }

        if (!VisualInfo || OGL_EGL_STATE->Config == 0) {
            GraphicalMessage(
                TRUE, 
                "Fatal Error"_s8, 
                "Could not find an EGL config with a matching X11 visual."_s8
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

        EGLint Options[] = {
            EGL_CONTEXT_MAJOR_VERSION,                         3,
            EGL_CONTEXT_MINOR_VERSION,                         3,
            EGL_CONTEXT_OPENGL_PROFILE_MASK,                   EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
            (DebugMode) ? EGL_CONTEXT_OPENGL_DEBUG : EGL_NONE, EGL_TRUE,
            EGL_NONE
        };
        OGL_EGL_STATE->Context = eglCreateContext(
            OGL_EGL_STATE->DisplayHandle,
            OGL_EGL_STATE->Config,
            EGL_NO_CONTEXT,
            Options
        );

        if (OGL_EGL_STATE->Context == EGL_NO_CONTEXT) {
            GraphicalMessage(
                TRUE, 
                "Fatal Error"_s8, 
                "Could not create OpenGL context with EGL."_s8
            );
            Abort(1);
        }
    }

    {
        EGLint PBufferAttribs[] = {
            EGL_WIDTH,  1,
            EGL_HEIGHT, 1,
            EGL_NONE
        };
        EGLSurface InitSurface = eglCreatePbufferSurface(
            OGL_EGL_STATE->DisplayHandle,
            OGL_EGL_STATE->Config,
            PBufferAttribs
        );

        if (
            InitSurface == EGL_NO_SURFACE || 
            !eglMakeCurrent(
                OGL_EGL_STATE->DisplayHandle, 
                InitSurface, 
                InitSurface,
                OGL_EGL_STATE->Context
            )
        ) {
            eglMakeCurrent(
                OGL_EGL_STATE->DisplayHandle,
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                OGL_EGL_STATE->Context
            );
        }
    }
}

RendHandle 
OGLEquipWindow(Handle WindowHandle)
{
    LinuxWindow* Window = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    OGLEGLWindow* RendWindow = OGL_EGL_STATE->FreeWindow;

    if (RendWindow) {
        SLL_STACK_POP(OGL_EGL_STATE->FreeWindow);
    } else {
        RendWindow = ArenaPushArrayZero(
            OGL_EGL_STATE->MemPool, 
            OGLEGLWindow, 
            1
        );
    }

    {
        EGLint SurfaceOptions[] = {
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_NONE
        };

        RendWindow->Surface = eglCreateWindowSurface(
            OGL_EGL_STATE->DisplayHandle,
            OGL_EGL_STATE->Config,
            Window->Win,
            SurfaceOptions
        );

        if (RendWindow->Surface == EGL_NO_SURFACE) {
            EGLint SurfaceOptionsLinear[] = {
                EGL_NONE
            };

            RendWindow->Surface = eglCreateWindowSurface(
                OGL_EGL_STATE->DisplayHandle,
                OGL_EGL_STATE->Config,
                Window->Win,
                SurfaceOptionsLinear
            );
        }

        if (RendWindow->Surface == EGL_NO_SURFACE) {
            GraphicalMessage(
                TRUE, 
                "Fatal Error"_s8, 
                "Could not create EGL surface."_s8
            );
            Abort(1);
        }
    }

    RendHandle Result = {
        INT_FROM_PTR(RendWindow)
    };

    return Result;
}

void 
OGLUnequipWindow(Handle WindowHandle, RendHandle Renderer)
{
    OGLEGLWindow* RendWindow = (OGLEGLWindow*) PTR_FROM_INT(Renderer.U64[0]);

    if (RendWindow)
        SLL_STACK_PUSH(OGL_EGL_STATE->FreeWindow, RendWindow);
}

void 
OGLSelectWindow(Handle WindowHandle, RendHandle Renderer)
{
    OGLWindow* Window = (OGLWindow*) PTR_FROM_INT(Renderer.U64[0]);
    OGLEGLWindow* RendWindow = (OGLEGLWindow*) PTR_FROM_INT(
        Window->Handle.U64[0]
    );

    eglMakeCurrent(
        OGL_EGL_STATE->DisplayHandle,
        RendWindow->Surface,
        RendWindow->Surface,
        OGL_EGL_STATE->Context
    );
}

void 
OGLSwapWindow(Handle WindowHandle, RendHandle Renderer)
{
    OGLWindow* Window = (OGLWindow*) PTR_FROM_INT(Renderer.U64[0]);
    OGLEGLWindow* RendWindow = (OGLEGLWindow*) PTR_FROM_INT(
        Window->Handle.U64[0]
    );

    eglSwapBuffers(
        OGL_EGL_STATE->DisplayHandle,
        RendWindow->Surface
    );
}
