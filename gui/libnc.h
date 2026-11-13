#if !defined(__LIBNC_H__)
#define __LIBNC_H__

#include "nc_defines.h"
#include "nc_types.h"
#include "nc_system.h"
#include "nc_memory.h"

#if defined(NC_CC_GNU)
    #include "linux/linux_system.cpp"
#endif

#include "nc_tls.h"
#include "nc_math.h"
#include "nc_arena.h"
#include "nc_string.h"
#include "nc_time.h"
#include "nc_print.h"
#include "nc_ring.h"
#include "nc_log.h"
#include "nc_profile.h"
#include "nc_atomics.h"
#include "nc_cli.h"
#include "nc_file.h"
#include "nc_net.h"
#include "nc_http.h"
#include "nc_proc.h"
#include "nc_simd.h"
#include "nc_sync.h"
#include "nc_thread.h"
#include "nc_object_cache.h"
#include "nc_content.h"
#include "nc_file_stream.h"
#include "nc_edit.h"
#include "nc_base64.h"
#include "nc_sock.h"
#include "nc_keyboard.h"
#include "nc_font_provider.h"
#include "nc_font_cache.h"
#include "nc_render.h"
#include "nc_draw.h"
#include "nc_ui.h"
#include "nc_ui_components.h"
#include "nc_window.h"
#include "nc_config.h"
#include "nc_metadesk.h"
#include "nc_sort.h"
#include "nc_application.h"
#include "nc_eval.h"

#include "nc_math.cpp"
#include "nc_memory.cpp"
#include "nc_arena.cpp"
#include "nc_string.cpp"
#include "nc_system.cpp"
#include "nc_time.cpp"
#include "nc_ring.cpp"
#include "nc_log.cpp"
#include "nc_profile.cpp"
#include "nc_cli.cpp"
#include "nc_file.cpp"
#include "nc_http.cpp"
#include "nc_proc.cpp"
#include "nc_thread.cpp"
#include "nc_object_cache.cpp"
#include "nc_content.cpp"
#include "nc_file_stream.cpp"
#include "nc_edit.cpp"
#include "nc_base64.cpp"
#include "nc_keyboard.cpp"
#include "nc_font_cache.cpp"
#include "nc_render.cpp"
#include "nc_draw.cpp"
#include "nc_ui.cpp"
#include "nc_ui_components.cpp"
#include "nc_window.cpp"
#include "nc_metadesk.cpp"
#include "nc_config.cpp"
#include "nc_application.cpp"
#include "nc_eval.cpp"

#if defined(NC_OS_WIN)
    #include "win32/win32_platform.h"
    #include "win32/win32_system.cpp"
    #include "win32/win32_memory.cpp"
    #include "win32/win32_file.cpp"
    #include "win32/win32_keyboard.h"
    #include "win32/win32_net.cpp"
    #include "win32/win32_http.cpp"
    #include "win32/win32_proc.cpp"
    #include "win32/win32_sync.cpp"
    #include "win32/win32_thread.cpp"
    #include "win32/win32_time.cpp"
    #include "win32/win32_window.cpp"
    #include "win32/win32_print.cpp"
    #include "win32/win32_sock.cpp"
    #include "win32/win32_entry.cpp"
#elif defined(NC_OS_LINUX)
    #include "linux/linux_platform.h"
    #include "linux/linux_memory.cpp"
    #include "linux/linux_time.cpp"
    #include "linux/linux_file.cpp"
    #include "linux/linux_net.cpp"
    #include "linux/linux_proc.cpp"
    #include "linux/linux_sync.cpp"
    #include "linux/linux_thread.cpp"
    #include "linux/linux_print.cpp"
    #include "linux/linux_window.cpp"
    #include "linux/linux_sock.cpp"
    #include "linux/linux_entry.cpp"
#elif defined(NC_OS_MAC)
    #include "macos/macos_system.cpp"
    #include "macos/macos_memory.cpp"
    #include "macos/macos_file.cpp"
    #include "macos/macos_keyboard.h"
    #include "macos/macos_net.cpp"
    #include "macos/macos_proc.cpp"
    #include "macos/macos_sync.cpp"
    #include "macos/macos_thread.cpp"
    #include "macos/macos_time.cpp"
    #include "macos/macos_window.cpp"
    #include "macos/macos_print.cpp"
#endif

#if defined(NC_OS_WIN) && defined(NC_USE_DX11)
    #include "dx11/dx11_platform.h"
    #include "dx11/dx11_render.h"
    #include "dx11/dx11_render.cpp"
#elif defined(NC_USE_OPENGL)
    #include "opengl/opengl_platform.h"
    #include "opengl/opengl_render.h"

    #include "opengl/opengl_render.cpp"

    #if defined(NC_USE_OPENGL_GLX)
        #include "opengl/opengl_glx_platform.h"
        #include "opengl/opengl_glx.h"

        #include "opengl/opengl_glx.cpp"
    #elif defined(NC_USE_OPENGL_EGL)
        #include "opengl/opengl_egl_platform.h"
        #include "opengl/opengl_egl.h"

        #include "opengl/opengl_egl.cpp"
    #else
        #error Need to specify backend of OpenGL to use [GLX | EGL]
    #endif
#elif defined(NC_USE_VULKAN)
    #error NC_USE_VULKAN defined but no vulkan backend defined
#elif defined(NC_OS_MAC) && defined(NC_USE_METAL)
    #error NC_USE_METAL defined but no metal backend defined
#else
    #error NC_GUI defined but no gfx backend defined should be NC_USE_[DX11 | OPENGL | VULKAN | METAL]
#endif

#if defined(NC_OS_WIN) && defined(NC_USE_DWRITE)
    #include "dwrite/dwrite_platform.h"
    #include "dwrite/dwrite_font_provider.h"
    #include "dwrite/dwrite_font_provider.cpp"
#elif defined(NC_USE_FREETYPE)
    #include "freetype/freetype_platform.h"
    #include "freetype/freetype_font_provider.h"
    #include "freetype/freetype_font_provider.cpp"
#else
    #error NC_GUI defined but no font backend defined should be NC_USE_[DWRITE | FREETYPE]
#endif

#include "rad_lzb_simple/rad_lzb_simple.h"
#include "rad_lzb_simple/rad_lzb_simple.c"

INTERNAL b32
Update(void)
{
    b32 Result = FALSE;

    AtomicIncFetchU64(&GLOBAL_UPDATE_TICK_INDEX, MEM_ORDER_SEQ_CST);
    FontFrame();
    Result = Frame();

    return Result;
}
#else
    #error libnc.h should only be included once in any file of your choosing
#endif // __LIBNC_H__
