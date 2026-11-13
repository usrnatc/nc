#if !defined(__OPENGL_EGL_H__)
#define __OPENGL_EGL_H__

#include "../../nc_types.h"
#include "../../nc_system.h"
#include "../../nc_render.h"

// @defines____________________________________________________________________


// @types______________________________________________________________________


// @runtime____________________________________________________________________


// @functions__________________________________________________________________
vproc* OGLLoadProcedure(char* Name);
void OGLInit(CommandLine* CLI);
RendHandle OGLEquipWindow(Handle WindowHandle);
void OGLUnequipWindow(Handle WindowHandle, RendHandle Renderer);
void OGLSelectWindow(Handle WindowHandle, RendHandle Renderer);
void OGLSwapWindow(Handle WindowHandle, RendHandle Renderer);

#endif // __OPENGL_EGL_H__
