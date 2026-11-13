#if !defined(__OPENGL_GLX_H__)
#define __OPENGL_GLX_H__

#include "../../nc_types.h"
#include "../../nc_cli.h"
#include "../../nc_render.h"
#include "opengl_glx_platform.h"

// @defines____________________________________________________________________


// @types______________________________________________________________________


// @runtime____________________________________________________________________
extern GLXContext OGLContext;

// @functions__________________________________________________________________
vproc* OGLLoadProcedure(char* Name);
void OGLInit(CommandLine* CLI);
RendHandle OGLEquipWindow(Handle WindowHandle);
void OGLUnequipWindow(Handle WindowHandle, RendHandle Renderer);
void OGLSelectWindow(Handle WindowHandle, RendHandle Renderer);
void OGLSwapWindow(Handle WindowHandle, RendHandle Renderer);

#endif // __OPENGL_GLX_H__
