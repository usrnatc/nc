#include "linux_platform.h"

#include "../nc_math.h"
#include "../nc_window.h"
#include "../nc_memory.h"
#include "../nc_print.h"

#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xfixes.h>

struct LinuxWindowClientArea {
    LinuxWindowClientArea* Next;
    r2f32                  Rect;
};

struct LinuxWindow {
    LinuxWindow*           Next;
    LinuxWindow*           Prev;
    Window                 Win;
    XIC                    Xic;
    XID                    CounterXID;
    u64                    CounterValue;
    b32                    ResizeDraw;
    r2f32                  PrevSyncedRect;
    b32                    WaitingForResize;
    b32                    CustomBorder;
    f32                    CustomTitleBarThickness;
    f32                    CustomEdgeThickness;
    LinuxWindowClientArea* HeadClientArea;
    LinuxWindowClientArea* TailClientArea;
};

struct GraphicsState {
    Arena*                 MemPool;
    Display*               Disp;
    Window                 __HIDDEN_WINDOW;                                     // NOTE(nc): we need this because X11 is kind of shit and clipboards die when the owning-window does. so we need a window that lives forever
    XIM                    Xim;
    LinuxWindow*           HeadWindow;
    LinuxWindow*           TailWindow;
    LinuxWindow*           FreeWindow;
    Atom                   DeleteWindowAtom;
    Atom                   SyncRequestAtom;
    Atom                   SyncRequestCounterAtom;
    Atom                   ClipboardAtom;
    Atom                   TargetsAtom;
    Atom                   UTF8StringAtom;
    Atom                   MotifHintsAtom;
    Cursor                 Cursors[CURSOR_KIND_COUNT];
    CursorKind             PrevSetCursor;
    GraphicsProperties     Properties;
    int                    WakeupFD;
    Visual*                WindowVisual;
    int                    WindowDepth;
    Colormap               WindowColourMap;
    LinuxWindowClientArea* FreeClientArea;
    long*                  IconImageData;
    u64                    IconImageDataCount;
    Arena*                 ClipboardMemPool;
    u8*                    ClipboardBuffer;
    u64                    ClipboardBufferSize;
    u64                    KeyDownState[(INPUT_KIND_COUNT + 63) / 64];
    b32                    IsXFixesPresent;
    int                    XFixesSelectionEventCode;
    Window                 PrevTitleBarPressWindow;
    Time                   PrevTitleBarPressTime;
    int                    PrevTitleBarPressX;
    int                    PrevTitleBarPressY;
};

GraphicsState* GRAPHICS_STATE = NULL;

// global readonly char* TEXT_MIME_TYPES[] = {
//     "UTF8_STRING",
//     "text/plain;charset=utf-8",
//     "text/plain",
//     "TEXT",
//     "STRING"
// };

LinuxWindow*
X11WindowToLinuxWindow(Window WindowHandle)
{
    LinuxWindow* Result = NULL;

    for (
        LinuxWindow* Win = GRAPHICS_STATE->HeadWindow; 
        Win; 
        Win = Win->Next
    ) {
        if (Win->Win == WindowHandle) {
            Result = Win;
            break;
        }
    }

    return Result;
}

INTERNAL int
LinuxWindowMoveResizeCodeFromPos(
    LinuxWindow* Win, 
    v2f32 Position, 
    OUT b32* IsInClientArea
) {
    b32 InNonClientArea = FALSE;
    int Result = 0;

    if (Win->CustomBorder) {
        Handle WindowHandle = {
            INT_FROM_PTR(Win)
        };
        r2f32 WindowRect = GetWindowClientRect(WindowHandle);
        r2f32 InsideEdges = Widen(
            WindowRect, 
            -Win->CustomEdgeThickness
        );
        r2f32 InsideEdgesAndTitleBar = InsideEdges;

        InsideEdgesAndTitleBar.Y0 += Win->CustomTitleBarThickness;

        if (!InRange(InsideEdgesAndTitleBar, Position)) {
            InNonClientArea = TRUE;

            for (
                LinuxWindowClientArea* ClientArea = Win->HeadClientArea;
                ClientArea;
                ClientArea = ClientArea->Next
            ) {
                if (InRange(ClientArea->Rect, Position)) {
                    InNonClientArea = FALSE;
                    break;
                }
            }
        }

        if (InNonClientArea) {
            r2f32 TitleBarRect = Rng(
                WindowRect.X0 + Win->CustomEdgeThickness,
                WindowRect.Y0 + Win->CustomEdgeThickness,
                WindowRect.X1 - Win->CustomEdgeThickness,
                WindowRect.Y0 + Win->CustomEdgeThickness + Win->CustomTitleBarThickness
            );

            if (InRange(TitleBarRect, Position)) {
                Result = _NET_WM_MOVERESIZE_MOVE;
            } else if (
                InRange(WindowRect, Position) &&
                !InRange(InsideEdges, Position)
            ) {
                if(
                    Position.X <= InsideEdges.X0 && 
                    Position.Y <= InsideEdges.Y0
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_TOPLEFT;
                } else if(
                    Position.X <= InsideEdges.X0 && 
                    Position.Y >= InsideEdges.Y1
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT;
                } else if(
                    Position.X >= InsideEdges.X1 && 
                    Position.Y <= InsideEdges.Y0
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_TOPRIGHT;
                } else if(
                    Position.X >= InsideEdges.X1 && 
                    Position.Y >= InsideEdges.Y1
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT;
                } else if(
                    Position.X <= InsideEdges.X0
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_LEFT;
                } else if(
                    Position.X >= InsideEdges.X1
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_RIGHT;
                } else if(
                    Position.Y <= InsideEdges.Y0
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_TOP;
                } else if(
                    Position.Y >= InsideEdges.Y1
                ) {
                  Result = _NET_WM_MOVERESIZE_SIZE_BOTTOM;
                } else {
                  InNonClientArea = FALSE;
                }
            }
        }
    }

    *IsInClientArea = !InNonClientArea;

    return Result;
}

INTERNAL KeySym
KeySymFromInputKind(InputKind Input)
{
    KeySym Result = 0;

    switch (Input) {
        default: {} break;

        case INPUT_KIND_ESC: {
            Result = XK_Escape;
        } break;

        case INPUT_KIND_FUNC_1: {
            Result = XK_F1;
        } break;

        case INPUT_KIND_FUNC_2: {
            Result = XK_F2;
        } break;

        case INPUT_KIND_FUNC_3: {
            Result = XK_F3;
        } break;

        case INPUT_KIND_FUNC_4: {
            Result = XK_F4;
        } break;

        case INPUT_KIND_FUNC_5: {
            Result = XK_F5;
        } break;

        case INPUT_KIND_FUNC_6: {
            Result = XK_F6;
        } break;

        case INPUT_KIND_FUNC_7: {
            Result = XK_F7;
        } break;

        case INPUT_KIND_FUNC_8: {
            Result = XK_F8;
        } break;

        case INPUT_KIND_FUNC_9: {
            Result = XK_F9;
        } break;

        case INPUT_KIND_FUNC_10: {
            Result = XK_F10;
        } break;

        case INPUT_KIND_FUNC_11: {
            Result = XK_F11;
        } break;

        case INPUT_KIND_FUNC_12: {
            Result = XK_F12;
        } break;

        case INPUT_KIND_FUNC_13: {
            Result = XK_F13;
        } break;

        case INPUT_KIND_FUNC_14: {
            Result = XK_F14;
        } break;

        case INPUT_KIND_FUNC_15: {
            Result = XK_F15;
        } break;

        case INPUT_KIND_FUNC_16: {
            Result = XK_F16;
        } break;

        case INPUT_KIND_FUNC_17: {
            Result = XK_F17;
        } break;

        case INPUT_KIND_FUNC_18: {
            Result = XK_F18;
        } break;

        case INPUT_KIND_FUNC_19: {
            Result = XK_F19;
        } break;

        case INPUT_KIND_FUNC_20: {
            Result = XK_F20;
        } break;

        case INPUT_KIND_FUNC_21: {
            Result = XK_F21;
        } break;

        case INPUT_KIND_FUNC_22: {
            Result = XK_F22;
        } break;

        case INPUT_KIND_FUNC_23: {
            Result = XK_F23;
        } break;

        case INPUT_KIND_FUNC_24: {
            Result = XK_F24;
        } break;

        case INPUT_KIND_TILDE: {
            Result = '`';
        } break;

        case INPUT_KIND_0: {
            Result = XK_0;
        } break;

        case INPUT_KIND_1: {
            Result = XK_1;
        } break;

        case INPUT_KIND_2: {
            Result = XK_2;
        } break;

        case INPUT_KIND_3: {
            Result = XK_3;
        } break;

        case INPUT_KIND_4: {
            Result = XK_4;
        } break;

        case INPUT_KIND_5: {
            Result = XK_5;
        } break;

        case INPUT_KIND_6: {
            Result = XK_6;
        } break;

        case INPUT_KIND_7: {
            Result = XK_7;
        } break;

        case INPUT_KIND_8: {
            Result = XK_8;
        } break;

        case INPUT_KIND_9: {
            Result = XK_9;
        } break;

        case INPUT_KIND_MINUS: {
            Result = '-';
        } break;

        case INPUT_KIND_EQUAL: {
            Result = '=';
        } break;

        case INPUT_KIND_BACKSPACE: {
            Result = XK_BackSpace;
        } break;

        case INPUT_KIND_TAB: {
            Result = XK_Tab;
        } break;

        case INPUT_KIND_Q: {
            Result = XK_Q;
        } break;

        case INPUT_KIND_W: {
            Result = XK_W;
        } break;

        case INPUT_KIND_E: {
            Result = XK_E;
        } break;

        case INPUT_KIND_R: {
            Result = XK_R;
        } break;

        case INPUT_KIND_T: {
            Result = XK_T;
        } break;

        case INPUT_KIND_Y: {
            Result = XK_Y;
        } break;

        case INPUT_KIND_U: {
            Result = XK_U;
        } break;

        case INPUT_KIND_I: {
            Result = XK_I;
        } break;

        case INPUT_KIND_O: {
            Result = XK_O;
        } break;

        case INPUT_KIND_P: {
            Result = XK_P;
        } break;

        case INPUT_KIND_LEFTBRACKET: {
            Result = '[';
        } break;

        case INPUT_KIND_RIGHTBRACKET: {
            Result = ']';
        } break;

        case INPUT_KIND_BACKSLASH: {
            Result = '\\';
        } break;

        case INPUT_KIND_CAPSLOCK: {
            Result = XK_Caps_Lock;
        } break;

        case INPUT_KIND_A: {
            Result = XK_Alt_L;
        } break;

        case INPUT_KIND_S: {
            Result = XK_Shift_L;
        } break;

        case INPUT_KIND_D: {
            Result = XK_Delete;
        } break;

        case INPUT_KIND_F: {
            Result = XK_F24;
        } break;

        case INPUT_KIND_G: {
            Result = XK_G;
        } break;

        case INPUT_KIND_H: {
            Result = XK_Home;
        } break;

        case INPUT_KIND_J: {
            Result = XK_J;
        } break;

        case INPUT_KIND_K: {
            Result = XK_K;
        } break;

        case INPUT_KIND_L: {
            Result = XK_L;
        } break;

        case INPUT_KIND_SEMICOLON: {
            Result = ';';
        } break;

        case INPUT_KIND_QUOTE: {
            Result = '\'';
        } break;

        case INPUT_KIND_RETURN: {
            Result = XK_Return;
        } break;

        case INPUT_KIND_SHIFT: {
            Result = XK_Shift_L;
        } break;

        case INPUT_KIND_Z: {
            Result = XK_Z;
        } break;

        case INPUT_KIND_X: {
            Result = XK_X;
        } break;

        case INPUT_KIND_C: {
            Result = XK_Caps_Lock;
        } break;

        case INPUT_KIND_V: {
            Result = XK_V;
        } break;

        case INPUT_KIND_B: {
            Result = XK_B;
        } break;

        case INPUT_KIND_N: {
            Result = XK_N;
        } break;

        case INPUT_KIND_M: {
            Result = '-';
        } break;

        case INPUT_KIND_COMMA: {
            Result = ',';
        } break;

        case INPUT_KIND_PERIOD: {
            Result = '.';
        } break;

        case INPUT_KIND_SLASH: {
            Result = '/';
        } break;

        case INPUT_KIND_CTRL: {
            Result = XK_Control_L;
        } break;

        case INPUT_KIND_ALT: {
            Result = XK_Alt_L;
        } break;

        case INPUT_KIND_SPACE: {
            Result = ' ';
        } break;

        case INPUT_KIND_SUPER: {
            Result = XK_Menu;
        } break;

        case INPUT_KIND_SCROLLLOCK: {
            Result = XK_Scroll_Lock;
        } break;

        case INPUT_KIND_PAUSE: {
            Result = XK_Pause;
        } break;

        case INPUT_KIND_INSERT: {
            Result = XK_Insert;
        } break;

        case INPUT_KIND_HOME: {
            Result = XK_Home;
        } break;

        case INPUT_KIND_PAGEUP: {
            Result = XK_Page_Up;
        } break;

        case INPUT_KIND_DELETE: {
            Result = XK_Delete;
        } break;

        case INPUT_KIND_END: {
            Result = XK_End;
        } break;

        case INPUT_KIND_PAGEDOWN: {
            Result = XK_Page_Down;
        } break;

        case INPUT_KIND_UP: {
            Result = XK_Up;
        } break;

        case INPUT_KIND_LEFT: {
            Result = XK_Left;
        } break;

        case INPUT_KIND_DOWN: {
            Result = XK_Down;
        } break;

        case INPUT_KIND_RIGHT: {
            Result = XK_Right;
        } break;

        case INPUT_KIND_NUMLOCK: {
            Result = XK_Num_Lock;
        } break;

        case INPUT_KIND_NUM_SLASH: {
            Result = XK_KP_Divide;
        } break;

        case INPUT_KIND_NUM_STAR: {
            Result = XK_KP_Multiply;
        } break;

        case INPUT_KIND_NUM_MINUS: {
            Result = XK_KP_Subtract;
        } break;

        case INPUT_KIND_NUM_PLUS: {
            Result = XK_KP_Add;
        } break;

        case INPUT_KIND_NUM_PERIOD: {
            Result = XK_KP_Decimal;
        } break;

        case INPUT_KIND_NUM_0: {
            Result = XK_0;
        } break;

        case INPUT_KIND_NUM_1: {
            Result = XK_1;
        } break;

        case INPUT_KIND_NUM_2: {
            Result = XK_2;
        } break;

        case INPUT_KIND_NUM_3: {
            Result = XK_3;
        } break;

        case INPUT_KIND_NUM_4: {
            Result = XK_4;
        } break;

        case INPUT_KIND_NUM_5: {
            Result = XK_5;
        } break;

        case INPUT_KIND_NUM_6: {
            Result = XK_6;
        } break;

        case INPUT_KIND_NUM_7: {
            Result = XK_7;
        } break;

        case INPUT_KIND_NUM_8: {
            Result = XK_8;
        } break;

        case INPUT_KIND_NUM_9: {
            Result = XK_9;
        } break;
    }

    return Result;
}

INTERNAL InputKind
InputKindFromKeySym(KeySym Key, OUT b32* IsRightSided)
{
    InputKind Result = INPUT_KIND_NULL;
    b32 RightSided = FALSE;

    switch (Key) {
        default: {
            if (Key >= XK_F1 && Key <= XK_F24)
                Result = (InputKind) (INPUT_KIND_FUNC_1 + (Key - XK_F1));
            else if (Key >= '0' && Key <= '9')
                Result = INPUT_KIND_0 + (Key - '0');
        } break;

        case XK_Escape: {
            Result = INPUT_KIND_ESC;
        } break;

        case XK_BackSpace: {
            Result = INPUT_KIND_BACKSPACE;
        } break;

        case XK_Delete: {
            Result = INPUT_KIND_DELETE;
        } break;

        case XK_Return: {
            Result = INPUT_KIND_RETURN;
        } break;

        case XK_Pause: {
            Result = INPUT_KIND_PAUSE;
        } break;

        case XK_Tab: {
            Result = INPUT_KIND_TAB;
        } break;

        case XK_Left: {
            Result = INPUT_KIND_LEFT;
        } break;

        case XK_Right: {
            Result = INPUT_KIND_RIGHT;
        } break;

        case XK_Up: {
            Result = INPUT_KIND_UP;
        } break;

        case XK_Down: {
            Result = INPUT_KIND_DOWN;
        } break;

        case XK_Home: {
            Result = INPUT_KIND_HOME;
        } break;

        case XK_End: {
            Result = INPUT_KIND_END;
        } break;

        case XK_Page_Up: {
            Result = INPUT_KIND_PAGEUP;
        } break;

        case XK_Page_Down: {
            Result = INPUT_KIND_PAGEDOWN;
        } break;

        case XK_Alt_L: {
            Result = INPUT_KIND_ALT;
        } break;

        case XK_Alt_R: {
            Result = INPUT_KIND_ALT;
            RightSided = TRUE;
        } break;

        case XK_Shift_L: {
            Result = INPUT_KIND_SHIFT;
        } break;

        case XK_Shift_R: {
            Result = INPUT_KIND_SHIFT;
            RightSided = TRUE;
        } break;

        case XK_Control_L: {
            Result = INPUT_KIND_CTRL;
        } break;

        case XK_Control_R: {
            Result = INPUT_KIND_CTRL;
            RightSided = TRUE;
        } break;

        case XK_Caps_Lock: {
            Result = INPUT_KIND_CAPSLOCK;
        } break;

        case XK_Menu: {
            Result = INPUT_KIND_SUPER;
        } break;

        case XK_Scroll_Lock: {
            Result = INPUT_KIND_SCROLLLOCK;
        } break;

        case XK_Insert: {
            Result = INPUT_KIND_INSERT;
        } break;

        case XK_Num_Lock: {
            Result = INPUT_KIND_NUMLOCK;
        } break;

        case XK_KP_Divide: {
            Result = INPUT_KIND_NUM_SLASH;
        } break;

        case XK_KP_Multiply: {
            Result = INPUT_KIND_NUM_STAR;
        } break;

        case XK_KP_Subtract: {
            Result = INPUT_KIND_NUM_MINUS;
        } break;

        case XK_KP_Add: {
            Result = INPUT_KIND_NUM_PLUS;
        } break;

        case XK_KP_Decimal: {
            Result = INPUT_KIND_NUM_PERIOD;
        } break;

        case XK_KP_0: {
            Result = INPUT_KIND_NUM_0;
        } break;

        case XK_KP_1: {
            Result = INPUT_KIND_NUM_1;
        } break;

        case XK_KP_2: {
            Result = INPUT_KIND_NUM_2;
        } break;

        case XK_KP_3: {
            Result = INPUT_KIND_NUM_3;
        } break;

        case XK_KP_4: {
            Result = INPUT_KIND_NUM_4;
        } break;

        case XK_KP_5: {
            Result = INPUT_KIND_NUM_5;
        } break;

        case XK_KP_6: {
            Result = INPUT_KIND_NUM_6;
        } break;

        case XK_KP_7: {
            Result = INPUT_KIND_NUM_7;
        } break;

        case XK_KP_8: {
            Result = INPUT_KIND_NUM_8;
        } break;

        case XK_KP_9: {
            Result = INPUT_KIND_NUM_9;
        } break;

        case '-': {
            Result = INPUT_KIND_MINUS;
        } break;

        case '=': {
            Result = INPUT_KIND_EQUAL;
        } break;

        case '[': {
            Result = INPUT_KIND_LEFTBRACKET;
        } break;

        case ']': {
            Result = INPUT_KIND_RIGHTBRACKET;
        } break;

        case ';': {
            Result = INPUT_KIND_SEMICOLON;
        } break;

        case '\'': {
            Result = INPUT_KIND_QUOTE;
        } break;

        case '.': {
            Result = INPUT_KIND_PERIOD;
        } break;

        case ',': {
            Result = INPUT_KIND_COMMA;
        } break;

        case '/': {
            Result = INPUT_KIND_SLASH;
        } break;

        case '\\': {
            Result = INPUT_KIND_BACKSLASH;
        } break;

        case '\t': {
            Result = INPUT_KIND_TAB;
        } break;

        case 'a':
        case 'A': {
            Result = INPUT_KIND_A;
        } break;

        case 'b':
        case 'B': {
            Result = INPUT_KIND_B;
        } break;

        case 'c':
        case 'C': {
            Result = INPUT_KIND_C;
        } break;

        case 'd':
        case 'D': {
            Result = INPUT_KIND_D;
        } break;

        case 'e':
        case 'E': {
            Result = INPUT_KIND_E;
        } break;

        case 'f':
        case 'F': {
            Result = INPUT_KIND_F;
        } break;

        case 'g':
        case 'G': {
            Result = INPUT_KIND_G;
        } break;

        case 'h':
        case 'H': {
            Result = INPUT_KIND_H;
        } break;

        case 'i':
        case 'I': {
            Result = INPUT_KIND_I;
        } break;

        case 'j':
        case 'J': {
            Result = INPUT_KIND_J;
        } break;

        case 'k':
        case 'K': {
            Result = INPUT_KIND_K;
        } break;

        case 'l':
        case 'L': {
            Result = INPUT_KIND_L;
        } break;

        case 'm':
        case 'M': {
            Result = INPUT_KIND_M;
        } break;

        case 'n':
        case 'N': {
            Result = INPUT_KIND_N;
        } break;

        case 'o':
        case 'O': {
            Result = INPUT_KIND_O;
        } break;

        case 'p':
        case 'P': {
            Result = INPUT_KIND_P;
        } break;

        case 'q':
        case 'Q': {
            Result = INPUT_KIND_Q;
        } break;

        case 'r':
        case 'R': {
            Result = INPUT_KIND_R;
        } break;

        case 's':
        case 'S': {
            Result = INPUT_KIND_S;
        } break;

        case 't':
        case 'T': {
            Result = INPUT_KIND_T;
        } break;

        case 'u':
        case 'U': {
            Result = INPUT_KIND_U;
        } break;

        case 'v':
        case 'V': {
            Result = INPUT_KIND_V;
        } break;

        case 'w':
        case 'W': {
            Result = INPUT_KIND_W;
        } break;

        case 'x':
        case 'X': {
            Result = INPUT_KIND_X;
        } break;

        case 'y':
        case 'Y': {
            Result = INPUT_KIND_Y;
        } break;

        case 'z':
        case 'Z': {
            Result = INPUT_KIND_Z;
        } break;

        case ' ': {
            Result = INPUT_KIND_SPACE;
        } break;

        case '`': {
            Result = INPUT_KIND_TILDE;
        } break;
    }

    if (IsRightSided)
        *IsRightSided = RightSided;

    return Result;
}

INTERNAL b32
X11WindowHasState(LinuxWindow* SystemWindow, Atom StateAtom)
{
    b32 Result = FALSE;
    Atom ActualType = 0;
    int ActualFormat = 0;
    unsigned long ItemCount = 0;
    unsigned long BytesRemaining = 0;
    u8* Data = NULL;
    Atom WindowStateAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE",
        0
    );

    XGetWindowProperty(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        WindowStateAtom,
        0,
        1024,
        FALSE,
        XA_ATOM,
        &ActualType,
        &ActualFormat,
        &ItemCount,
        &BytesRemaining,
        &Data
    );

    if (Data) {
        Atom* Atoms = (Atom*) Data;

        for (u32 Index = 0; Index < ItemCount; ++Index) {
            if (Atoms[Index] == StateAtom) {
                Result = TRUE;
                break;
            }
        }

        XFree(Data);
    }

    return Result;
}

INTERNAL b32
X11WindowHasState(LinuxWindow* SystemWindow, Atom StateAtomA, Atom StateAtomB)
{
    b32 Result = FALSE;
    Atom ActualType = 0;
    int ActualFormat = 0;
    unsigned long ItemCount = 0;
    unsigned long BytesRemaining = 0;
    u8* Data = NULL;
    Atom WindowStateAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE",
        0
    );

    XGetWindowProperty(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        WindowStateAtom,
        0,
        1024,
        FALSE,
        XA_ATOM,
        &ActualType,
        &ActualFormat,
        &ItemCount,
        &BytesRemaining,
        &Data
    );

    if (Data) {
        Atom* Atoms = (Atom*) Data;
        b32 ResultA = FALSE;
        b32 ResultB = FALSE;

        for (u32 Index = 0; Index < ItemCount; ++Index) {
            if (Atoms[Index] == StateAtomA) {
                ResultA = TRUE;
            }

            if (Atoms[Index] == StateAtomB) {
                ResultB = TRUE;
            }
        }

        XFree(Data);
        Result = ResultA && ResultB;
    }

    return Result;
}

INTERNAL void
X11SendWindowStateEvent(
    LinuxWindow* SystemWindow,
    b32 ShouldSet,
    Atom AtomA,
    Atom AtomB
) {
    XEvent Event = {};
    Atom WindowStateAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE",
        0
    );

    Event.type = ClientMessage;
    Event.xclient.window = SystemWindow->Win;
    Event.xclient.message_type = WindowStateAtom;
    Event.xclient.format = 32;
    Event.xclient.data.l[0] = !!ShouldSet;
    Event.xclient.data.l[1] = AtomA;
    Event.xclient.data.l[2] = AtomB;
    Event.xclient.data.l[3] = 1;
    XSendEvent(
        GRAPHICS_STATE->Disp,
        DefaultRootWindow(GRAPHICS_STATE->Disp),
        FALSE,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &Event
    );
    XFlush(GRAPHICS_STATE->Disp);
}

void
GraphicsInit(void)
{
    Arena* MemPool = ArenaAlloc();

    GRAPHICS_STATE = ArenaPushArrayZero(MemPool, GraphicsState, 1);
    GRAPHICS_STATE->MemPool = MemPool;
    GRAPHICS_STATE->ClipboardMemPool = ArenaAlloc();
    GRAPHICS_STATE->Disp = XOpenDisplay(NULL);

    if (!GRAPHICS_STATE->Disp) {
        char* DisplayEnv = getenv("DISPLAY");

        PrintErr(
            "[ERROR] XOpenDisplay failed (DISPLAY=%s). Is an X server / WSLg running?\n",
            DisplayEnv 
                ? DisplayEnv 
                : "<unset>"
        );
        GraphicalMessage(
            TRUE,
            "Fatal Error"_s8,
            "Could not connect to the X display."_s8
        );
        Abort(1);
    }

    {
        GRAPHICS_STATE->__HIDDEN_WINDOW = XCreateWindow(
            GRAPHICS_STATE->Disp,
            XDefaultRootWindow(GRAPHICS_STATE->Disp),
            -1,
            -1,
            1,
            1,
            0,
            0,
            0,
            0,
            0,
            0
        );
    }

    GRAPHICS_STATE->DeleteWindowAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "WM_DELETE_WINDOW",
        0
    );
    GRAPHICS_STATE->SyncRequestAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_SYNC_REQUEST",
        0
    );
    GRAPHICS_STATE->SyncRequestCounterAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_SYNC_REQUEST_COUNTER",
        0
    );
    GRAPHICS_STATE->ClipboardAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "CLIPBOARD",
        0
    );
    GRAPHICS_STATE->TargetsAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "TARGETS",
        0
    );
    GRAPHICS_STATE->UTF8StringAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "UTF8_STRING",
        0
    );
    GRAPHICS_STATE->MotifHintsAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_MOTIF_WM_HINTS",
        0
    );

    {
        int XFixesVersion = 0;
        int XFixesErr = 0;

        GRAPHICS_STATE->IsXFixesPresent = XQueryExtension(
            GRAPHICS_STATE->Disp,
            "XFIXES",
            &XFixesVersion,
            &GRAPHICS_STATE->XFixesSelectionEventCode,
            &XFixesErr
        );

        if (GRAPHICS_STATE->IsXFixesPresent) {
            XFixesSelectSelectionInput(
                GRAPHICS_STATE->Disp,
                GRAPHICS_STATE->__HIDDEN_WINDOW,
                GRAPHICS_STATE->ClipboardAtom,
                XFixesSetSelectionOwnerNotifyMask
            );
        }
    }

    GRAPHICS_STATE->Xim = XOpenIM(
        GRAPHICS_STATE->Disp,
        NULL,
        NULL,
        NULL
    );

    // NOTE: For some ungodly reason, if a window has stored some selection
    //     : in the clipboard, and that window is closed, the clipboard is
    //     : cleared and the data is lost (why???). This window will be hidden,
    //     : but lives for the lifetime of the application so that the clipboard
    //     : is persistent throughout the program's run as expected.
    // GRAPHICS_STATE->ClipboardXID = XCreateSimpleWindow(
    //     GRAPHICS_STATE->Disp,
    //     XDefaultRootWindow(GRAPHICS_STATE->Disp),
    //     -10,
    //     -10,
    //     1,
    //     1,
    //     0,
    //     0,
    //     0
    // );
    GRAPHICS_STATE->Properties.DoubleClickTime = 0.5f;
    GRAPHICS_STATE->Properties.CaretBlinkTime = 0.5f;
    GRAPHICS_STATE->Properties.DefaultRefreshRate = 60.0f;

    {
        XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
            GRAPHICS_STATE->Disp,
            DefaultRootWindow(GRAPHICS_STATE->Disp)
        );

        for (u64 Index = 0; Index < Resources->noutput; ++Index) {
            XRROutputInfo* OutputInfo = XRRGetOutputInfo(
                GRAPHICS_STATE->Disp,
                Resources,
                Resources->outputs[Index]
            );

            if (
                OutputInfo &&
                OutputInfo->connection == RR_Connected &&
                OutputInfo->crtc != None
            ) {
                XRRCrtcInfo* CRTCInfo = XRRGetCrtcInfo(
                    GRAPHICS_STATE->Disp,
                    Resources,
                    OutputInfo->crtc
                );

                if (CRTCInfo) {
                    for (
                        u64 ModeIndex = 0; 
                        ModeIndex < Resources->nmode; 
                        ++ModeIndex
                    ) {
                        if (
                            Resources->modes[ModeIndex].id == CRTCInfo->mode
                        ) {
                            XRRModeInfo* ModeInfo = &Resources->modes[
                                ModeIndex
                            ];

                            if (ModeInfo->vTotal && ModeInfo->hTotal) {
                                GRAPHICS_STATE->Properties.DefaultRefreshRate = (
                                    (f32) ModeInfo->dotClock /
                                    (
                                        (f32) ModeInfo->hTotal * (f32) ModeInfo->vTotal
                                    )
                                );
                            }
                        }
                    }

                    XRRFreeCrtcInfo(CRTCInfo);
                }

                XRRFreeOutputInfo(OutputInfo);
            }
        }

        XRRFreeScreenResources(Resources);

        if (!GRAPHICS_STATE->Properties.DefaultRefreshRate)
            GRAPHICS_STATE->Properties.DefaultRefreshRate = 60.0f;
    }

    {
        struct {
            CursorKind Kind;
            unsigned int ID;
        } Map[] = {
            {
                CURSOR_KIND_POINTER,
                XC_left_ptr
            },
            {
                CURSOR_KIND_I_BAR,
                XC_xterm
            },
            {
                CURSOR_KIND_LEFT_RIGHT,
                XC_sb_h_double_arrow
            },
            {
                CURSOR_KIND_UP_DOWN,
                XC_sb_v_double_arrow
            },
            {
                CURSOR_KIND_DOWN_RIGHT,
                XC_bottom_right_corner
            },
            {
                CURSOR_KIND_UP_RIGHT,
                XC_top_right_corner
            },
            {
                CURSOR_KIND_UP_DOWN_LEFT_RIGHT,
                XC_fleur
            },
            {
                CURSOR_KIND_HAND_POINT,
                XC_hand1
            },
            {
                CURSOR_KIND_DISABLED,
                XC_X_cursor
            },
        };

        for (u64 Index = 0; Index < ARRAY_COUNT(Map); ++Index) {
            GRAPHICS_STATE->Cursors[Map[Index].Kind] = XCreateFontCursor(
                GRAPHICS_STATE->Disp,
                Map[Index].ID
            );
        }
    }

    GRAPHICS_STATE->WakeupFD = eventfd(0, EFD_CLOEXEC);
}

GraphicsProperties*
GetGraphicsProperties(void)
{
    return &GRAPHICS_STATE->Properties;
}

void
SetClipboardText(Str8 String)
{
    if (GRAPHICS_STATE->ClipboardBuffer) {
        ArenaClear(GRAPHICS_STATE->ClipboardMemPool);
        GRAPHICS_STATE->ClipboardBufferSize = 0;
    }

    GRAPHICS_STATE->ClipboardBuffer = ArenaPushArray(
        GRAPHICS_STATE->ClipboardMemPool,
        u8,
        String.Size + 1
    );
    MemCpy(GRAPHICS_STATE->ClipboardBuffer, String.Str, String.Size);
    GRAPHICS_STATE->ClipboardBuffer[String.Size] = 0;
    GRAPHICS_STATE->ClipboardBufferSize = String.Size;
    XSetSelectionOwner(
        GRAPHICS_STATE->Disp,
        GRAPHICS_STATE->ClipboardAtom,
        GRAPHICS_STATE->__HIDDEN_WINDOW,
        CurrentTime
    );
}

Str8
GetClipboardText(Arena* MemPool)
{
    Str8 Result = {};
    Window OwnerWindow = XGetSelectionOwner(
        GRAPHICS_STATE->Disp,
        GRAPHICS_STATE->ClipboardAtom
    );

    if (OwnerWindow == None)
        return Result;

    // NOTE: if we own the clipboard we can just get the data easily without
    //     : having to fight for ownership
    if (OwnerWindow == GRAPHICS_STATE->__HIDDEN_WINDOW) {
        if (GRAPHICS_STATE->ClipboardBufferSize) {
            Result.Str = ArenaPushArray(
                MemPool, 
                u8, 
                GRAPHICS_STATE->ClipboardBufferSize + 1
            );
            Result.Size = GRAPHICS_STATE->ClipboardBufferSize;
            MemCpy(
                Result.Str, 
                GRAPHICS_STATE->ClipboardBuffer, 
                GRAPHICS_STATE->ClipboardBufferSize
            );
            Result.Str[Result.Size] = 0;
        }

        return Result;
    }

    Atom PropertyAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "NC_CLIPBOARD",
        0
    );

    XConvertSelection(
        GRAPHICS_STATE->Disp,
        GRAPHICS_STATE->ClipboardAtom,
        GRAPHICS_STATE->UTF8StringAtom,
        PropertyAtom,
        GRAPHICS_STATE->__HIDDEN_WINDOW,
        CurrentTime
    );
    XFlush(GRAPHICS_STATE->Disp);

    XEvent Event = {};
    int XFD = ConnectionNumber(GRAPHICS_STATE->Disp);
    pollfd PFD = {};
    b32 GotEvent = FALSE;

    PFD.fd = XFD;
    PFD.events = POLLIN;

    while (poll(&PFD, 1, 1000) > 0) {
        if (
            XCheckTypedWindowEvent(
                GRAPHICS_STATE->Disp,
                GRAPHICS_STATE->__HIDDEN_WINDOW,
                SelectionNotify,
                &Event
            )
        ) {
            GotEvent = TRUE;
            break;
        }
    }

    if (!GotEvent)
        return Result;

    Atom ActualType = 0;
    int ActualFormat = 0;
    unsigned long ItemCount = 0;
    unsigned long BytesRemaining = 0;
    u8* Data = NULL;

    XGetWindowProperty(
        GRAPHICS_STATE->Disp,
        GRAPHICS_STATE->__HIDDEN_WINDOW,
        PropertyAtom,
        0,
        I32_MAX,
        TRUE,
        AnyPropertyType,
        &ActualType,
        &ActualFormat,
        &ItemCount,
        &BytesRemaining,
        &Data
    );

    if (Data && ItemCount) {
        Result.Str = ArenaPushArray(MemPool, u8, ItemCount + 1);
        Result.Size = ItemCount;
        MemCpy(Result.Str, Data, ItemCount);
        Result.Str[Result.Size] = 0;
    }

    if (Data)
        XFree(Data);

    return Result;
}

Handle
WindowOpen(r2f32 Rect, WindowPropertyKind Properties, Str8 Title)
{
    v2f32 Resolution = Length(Rect);
    LinuxWindow* SystemWindow = GRAPHICS_STATE->FreeWindow;

    if (SystemWindow)
        SLL_STACK_POP(GRAPHICS_STATE->FreeWindow);
    else
        SystemWindow = ArenaPushArray(GRAPHICS_STATE->MemPool, LinuxWindow, 1);

    MemSet(SystemWindow, 0, sizeof(*SystemWindow));
    DLL_PUSH_BACK(
        GRAPHICS_STATE->HeadWindow,
        GRAPHICS_STATE->TailWindow,
        SystemWindow
    );

    Visual* Vis = GRAPHICS_STATE->WindowVisual;
    int Depth = GRAPHICS_STATE->WindowDepth;
    Colormap ColourMap = GRAPHICS_STATE->WindowColourMap;
    unsigned long AttrMask = 0;
    XSetWindowAttributes WindowAttributes = {};

    if (!Vis) {
        Vis = (Visual*) CopyFromParent;
        Depth = CopyFromParent;
    } else {
        WindowAttributes.background_pixmap = None;
        WindowAttributes.border_pixel = 0;
        WindowAttributes.colormap = ColourMap;
        AttrMask = CWBackPixmap | CWBorderPixel | CWColormap;
    }

    SystemWindow->Win = XCreateWindow(
        GRAPHICS_STATE->Disp,
        XDefaultRootWindow(GRAPHICS_STATE->Disp),
        0,
        0,
        Resolution.X,
        Resolution.Y,
        0,
        Depth,
        InputOutput,
        Vis,
        AttrMask,
        &WindowAttributes
    );
    XSelectInput(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        (
            ExposureMask |
            PointerMotionMask |
            ButtonPressMask |
            ButtonReleaseMask |
            KeyPressMask |
            KeyReleaseMask |
            StructureNotifyMask |
            FocusChangeMask
        )
    );
    XSetWindowBackgroundPixmap(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        None
    );

    Atom Protocols[] = {
        GRAPHICS_STATE->DeleteWindowAtom,
        GRAPHICS_STATE->SyncRequestAtom
    };

    XSetWMProtocols(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        Protocols,
        ARRAY_COUNT(Protocols)
    );

    {
        XSyncValue InitialValue;

        XSyncIntToValue(&InitialValue, 0);
        SystemWindow->CounterXID = XSyncCreateCounter(
            GRAPHICS_STATE->Disp,
            InitialValue
        );
    }

    XChangeProperty(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        GRAPHICS_STATE->SyncRequestCounterAtom,
        XA_CARDINAL,
        32,
        PropModeReplace,
        (u8*) &SystemWindow->CounterXID,
        1
    );

    {
        Atom NetWMIcon = XInternAtom(GRAPHICS_STATE->Disp, "_NET_WM_ICON", 0);

        XChangeProperty(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            NetWMIcon,
            XA_CARDINAL,
            32,
            PropModeReplace,
            (u8*) GRAPHICS_STATE->IconImageData,
            GRAPHICS_STATE->IconImageDataCount
        );
        XFlush(GRAPHICS_STATE->Disp);
    }

    {
        XClassHint* ClassHints = XAllocClassHint();

        ClassHints->res_name = "nc_gui";
        ClassHints->res_class = "NCGUI";
        XSetClassHint(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            ClassHints
        );
        XFree(ClassHints);
    }

    SystemWindow->Xic = XCreateIC(
        GRAPHICS_STATE->Xim,
        XNInputStyle,
        XIMPreeditNothing | XIMStatusNothing,
        XNClientWindow,
        SystemWindow->Win,
        XNFocusWindow,
        SystemWindow->Win,
        NULL
    );

    TempArena Scratch = GetScratch(NULL, 0);
    Str8 TitleCopy = ArenaPushStrCpy(Scratch.MemPool, Title);

    XStoreName(
        GRAPHICS_STATE->Disp, 
        SystemWindow->Win, 
        (char*) TitleCopy.Str
    );
    ReleaseScratch(Scratch);

    if (Properties & WINDOW_KIND_CUSTOM_BORDER) {
        SystemWindow->CustomBorder = TRUE;

        struct Hints {
            u32 Flags;
            u32 Functions;
            u32 Decorations;
            i32 InputMode;
            u32 CurrStatus;
        };

        Hints WindowHints = {};

        WindowHints.Flags = 2;
        WindowHints.Decorations = 0;

        Atom Property = XInternAtom(
            GRAPHICS_STATE->Disp,
            "_MOTIF_WM_HINTS",
            1
        );

        if (Property != None) {
            XChangeProperty(
                GRAPHICS_STATE->Disp,
                SystemWindow->Win,
                Property,
                Property,
                32,
                PropModeReplace,
                (unsigned char*) &WindowHints,
                5
            );
        }
    }

    Handle Result = {
        INT_FROM_PTR(SystemWindow)
    };

    return Result;
}

void
WindowClose(Handle WindowHandle)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(
        WindowHandle.V[0]
    );

    WindowClearCustomBorderData(WindowHandle);

    if (SystemWindow->Xic)
        XDestroyIC(SystemWindow->Xic);

    XSyncDestroyCounter(GRAPHICS_STATE->Disp, SystemWindow->CounterXID);
    XDestroyWindow(GRAPHICS_STATE->Disp, SystemWindow->Win);
    DLL_REMOVE(
        GRAPHICS_STATE->HeadWindow,
        GRAPHICS_STATE->TailWindow,
        SystemWindow
    );
    SLL_STACK_PUSH(
        GRAPHICS_STATE->FreeWindow,
        SystemWindow
    );
}

void
WindowSetTitle(Handle WindowHandle, Str8 Title)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    TempArena Scratch = GetScratch(NULL, 0);
    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Str8 TitleCopy = ArenaPushStrCpy(Scratch.MemPool, Title);

    XStoreName(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        (char*) TitleCopy.Str
    );
    ReleaseScratch(Scratch);
}

void
WindowFirstPaint(Handle WindowHandle)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    XMapWindow(GRAPHICS_STATE->Disp, SystemWindow->Win);
    XFlush(GRAPHICS_STATE->Disp);
}

void
WindowFocus(Handle WindowHandle)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    // TODO(nc): sometimes, program may open window before the first frame is drawn
    //         : which means XSetInputFocus is called before XMapWindow and we
    //         : server error
    XSetInputFocus(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        RevertToNone,
        CurrentTime
    );
}

b32
WindowIsFocused(Handle WindowHandle)
{
    b32 Result = FALSE;

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Window FocusedWindow = 0;
    int RevertTo = 0;

    XGetInputFocus(
        GRAPHICS_STATE->Disp,
        &FocusedWindow,
        &RevertTo
    );
    Result = SystemWindow->Win == FocusedWindow;

    return Result;
}

b32
WindowIsFullScreen(Handle WindowHandle)
{
    b32 Result = FALSE;

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    {
        LinuxWindow* Win = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
        Atom FullscreenAtom = XInternAtom(
            GRAPHICS_STATE->Disp,
            "_NET_WM_STATE_FULLSCREEN",
            0
        );

        Result = X11WindowHasState(Win, FullscreenAtom);
    }

    return Result;
}

void 
WindowSetFullScreen(Handle WindowHandle, b32 FullScreen)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Atom FullscreenAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE_FULLSCREEN",
        0
    );

    X11SendWindowStateEvent(
        SystemWindow,
        FullScreen,
        FullscreenAtom,
        0
    );
}

b32 
WindowIsMaximised(Handle WindowHandle)
{
    b32 Result = FALSE;

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Atom WindowState = XInternAtom(GRAPHICS_STATE->Disp, "_NET_WM_STATE", 0);
    Atom MaximisedHorizontalAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE_MAXIMIZED_HORZ",
        0
    );
    Atom MaximisedVerticalAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE_MAXIMIZED_VERT",
        0
    );
    unsigned long PropsCount = 0;
    Atom* Props = NULL;

    {
        Atom ActualType = 0;
        int ActualFormat = 0;
        unsigned long PostBytes = 0;
        int __Res = XGetWindowProperty(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            WindowState,
            0,
            1024,
            0,
            AnyPropertyType,
            &ActualType,
            &ActualFormat,
            &PropsCount,
            &PostBytes,
            (unsigned char**) &Props
        );
    }

    b32 IsMaximisedHorizontalSet = FALSE;
    b32 IsMaximisedVerticalSet = FALSE;

    {
        for (
            unsigned long Index = 0;
            Index < PropsCount;
            ++Index
        ) {
            if (Props[Index] == MaximisedHorizontalAtom)
                IsMaximisedHorizontalSet = TRUE;

            if (Props[Index] == MaximisedVerticalAtom)
                IsMaximisedVerticalSet = TRUE;
        }
    }

    XFree(Props);
    Result = (IsMaximisedHorizontalSet && IsMaximisedVerticalSet);

    // Result = X11WindowHasState(
    //     SystemWindow,
    //     MaximisedHorizontalAtom,
    //     MaximisedVerticalAtom
    // );

    return Result;
}

void 
WindowSetMaximised(Handle WindowHandle, b32 Maximised)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Atom MaximisedHorizontalAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE_MAXIMIZED_HORZ",
        0
    );
    Atom MaximisedVerticalAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE_MAXIMIZED_VERT",
        0
    );

    X11SendWindowStateEvent(
        SystemWindow,
        Maximised,
        MaximisedHorizontalAtom,
        MaximisedVerticalAtom
    );
}

// TODO(nc): might need to change this to XGetWindowProperty and compare against IconicState
b32 
WindowIsMinimised(Handle WindowHandle)
{
    b32 Result = FALSE;

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Atom HiddenAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_WM_STATE_HIDDEN",
        0
    );

    Result = X11WindowHasState(
        SystemWindow,
        HiddenAtom
    );

    return Result;
}

void 
WindowSetMinimised(Handle WindowHandle, b32 Minimised)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    if (Minimised) {
        XIconifyWindow(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            DefaultScreen(GRAPHICS_STATE->Disp)
        );
    } else {
        XMapWindow(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win
        );
        XRaiseWindow(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win
        );
    }

    XFlush(GRAPHICS_STATE->Disp);
}

void 
WindowBringToFront(Handle WindowHandle)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    XEvent Event = {};
    Atom BringToFrontAtom = XInternAtom(
        GRAPHICS_STATE->Disp,
        "_NET_ACTIVE_WINDOW",
        0
    );

    Event.type = ClientMessage;
    Event.xclient.window = SystemWindow->Win;
    Event.xclient.message_type = BringToFrontAtom;
    Event.xclient.format = 32;
    Event.xclient.data.l[0] = 1;
    Event.xclient.data.l[1] = CurrentTime;
    Event.xclient.data.l[2] = 0;
    XSendEvent(
        GRAPHICS_STATE->Disp,
        XDefaultRootWindow(GRAPHICS_STATE->Disp),
        FALSE,
        SubstructureRedirectMask | SubstructureNotifyMask,
        &Event
    );
    XFlush(GRAPHICS_STATE->Disp);
}

void 
WindowSetMonitor(Handle WindowHandle, Handle MonitorHandle)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    if (MonitorHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    XID DisplayID = (XID) PTR_FROM_INT(MonitorHandle.V[0]);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        XDefaultRootWindow(GRAPHICS_STATE->Disp)
    );

    if (!Resources)
        return;

    XRRCrtcInfo* DisplayInfo = XRRGetCrtcInfo(
        GRAPHICS_STATE->Disp,
        Resources,
        DisplayID
    );

    if (DisplayInfo) {
        XMoveWindow(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            DisplayInfo->x,
            DisplayInfo->y
        );
        XFlush(GRAPHICS_STATE->Disp);
        XRRFreeCrtcInfo(DisplayInfo);
    }

    XRRFreeScreenResources(Resources);
}

void 
WindowClearCustomBorderData(Handle WindowHandle)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* Win = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    for (
        LinuxWindowClientArea* ClientArea = Win->HeadClientArea, *Next = NULL;
        ClientArea;
        ClientArea = Next
    ) {
        Next = ClientArea->Next;
        SLL_STACK_PUSH(GRAPHICS_STATE->FreeClientArea, ClientArea);
    }

    Win->HeadClientArea = NULL;
    Win->TailClientArea = NULL;
}

void 
WindowPushCustomTitleBar(Handle WindowHandle, f32 Thickness)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* Win = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    Win->CustomTitleBarThickness = Thickness;
}

void 
WindowPushCustomEdges(Handle WindowHandle, f32 Thickness)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* Win = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    Win->CustomEdgeThickness = Thickness;
}

void 
WindowPushCustomTitleBarClientArea(Handle WindowHandle, r2f32 Rect)
{
    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return;

    LinuxWindow* Win = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    LinuxWindowClientArea* ClientArea = GRAPHICS_STATE->FreeClientArea;

    if (ClientArea) {
        SLL_STACK_POP(GRAPHICS_STATE->FreeClientArea);
    } else {
        ClientArea = ArenaPushArrayZero(
            GRAPHICS_STATE->MemPool, 
            LinuxWindowClientArea, 
            1
        );
    }

    ClientArea->Rect = Rect;
    SLL_QUEUE_PUSH(Win->HeadClientArea, Win->TailClientArea, ClientArea);
}

r2f32 
GetWindowRect(Handle WindowHandle)
{
    r2f32 Result = {};

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    if (SystemWindow->ResizeDraw) {
        Result = SystemWindow->PrevSyncedRect;
    } else {
        XWindowAttributes Attributes = {};
        Status Stat = XGetWindowAttributes(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            &Attributes
        );

        Result = Rng(
            (f32) Attributes.x,
            (f32) Attributes.y,
            (f32) (Attributes.x + Attributes.width),
            (f32) (Attributes.y + Attributes.height)
        );
    }

    return Result;
}

r2f32 
GetWindowClientRect(Handle WindowHandle)
{
    r2f32 Result = {};

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);

    if (SystemWindow->ResizeDraw) {
        v2f32 Len = Length(SystemWindow->PrevSyncedRect);

        Result = Rng(
            0.0f,
            0.0f,
            Len.X,
            Len.Y
        );
    } else {
        XWindowAttributes Attributes = {};
        Status Stat = XGetWindowAttributes(
            GRAPHICS_STATE->Disp,
            SystemWindow->Win,
            &Attributes
        );

        Result = Rng(
            0.0f,
            0.0f,
            (f32) Attributes.width,
            (f32) Attributes.height
        );
    }

    return Result;
}

f32 
GetWindowDPI(Handle WindowHandle)
{
    return 96.0f;
}

HandleArray 
ArenaPushMonitorArray(Arena* MemPool)
{
    TempArena Scratch = GetScratch(&MemPool, 1);
    HandleList ResultList = {};
    HandleArray Result = {};
    Window RootWindow = XDefaultRootWindow(GRAPHICS_STATE->Disp);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        RootWindow
    );

    if (!Resources)
        return Result;

    for (u32 Index = 0; Index < Resources->ncrtc; ++Index) {
        XRRCrtcInfo* DisplayInfo = XRRGetCrtcInfo(
            GRAPHICS_STATE->Disp,
            Resources,
            Resources->crtcs[Index]
        );
        Handle DisplayHandle = {
            INT_FROM_PTR(Resources->crtcs[Index])
        };

        if (DisplayInfo && DisplayInfo->mode != None)
            ListPush(Scratch.MemPool, &ResultList, DisplayHandle);

        if (DisplayInfo)
            XRRFreeCrtcInfo(DisplayInfo);
    }

    Result = HandleArrayFromList(MemPool, &ResultList);
    ReleaseScratch(Scratch);

    return Result;
}

Handle 
GetPrimaryMonitor(void)
{
    Handle Result = EMPTY_HANDLE_VALUE;
    Window Root = XDefaultRootWindow(GRAPHICS_STATE->Disp);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        Root
    );

    if (!Resources)
        return Result;

    XID PrimaryID = XRRGetOutputPrimary(
        GRAPHICS_STATE->Disp,
        Root
    );

    if (PrimaryID) {
        XRROutputInfo* PrimaryInfo = XRRGetOutputInfo(
            GRAPHICS_STATE->Disp,
            Resources,
            PrimaryID
        );

        if (PrimaryInfo && PrimaryInfo->crtc != None)
            Result.V[0] = INT_FROM_PTR(PrimaryInfo->crtc);

        if (PrimaryInfo)
            XRRFreeOutputInfo(PrimaryInfo);
    }

    if (Result == EMPTY_HANDLE_VALUE) {
        for (u32 Index = 0; Index < Resources->ncrtc; ++Index) {
            XRRCrtcInfo* DisplayInfo = XRRGetCrtcInfo(
                GRAPHICS_STATE->Disp,
                Resources,
                Resources->crtcs[Index]
            );

            if (DisplayInfo && DisplayInfo->mode != None) {
                Result.V[0] = INT_FROM_PTR(Resources->crtcs[Index]);
                XRRFreeCrtcInfo(DisplayInfo);
                break;
            }

            if (DisplayInfo)
                XRRFreeCrtcInfo(DisplayInfo);
        }
    }

    XRRFreeScreenResources(Resources);

    return Result;
}

// FIXME(nc): this is really dumb, but only way I have found so far
//          : using ranges to check which display the window is in would make it easier
Handle 
MonitorFromWindow(Handle WindowHandle)
{
    Handle Result = EMPTY_HANDLE_VALUE;

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* SystemWindow = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    XWindowAttributes WindowAttributes = {};

    XGetWindowAttributes(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        &WindowAttributes
    );

    int WindowX = 0;
    int WindowY = 0;
    Window ChildWindow = 0;

    XTranslateCoordinates(
        GRAPHICS_STATE->Disp,
        SystemWindow->Win,
        XDefaultRootWindow(GRAPHICS_STATE->Disp),
        0,
        0,
        &WindowX,
        &WindowY,
        &ChildWindow
    );

    i64 WindowCentreX = WindowX + WindowAttributes.width / 2;
    i64 WindowCentreY = WindowY + WindowAttributes.height / 2;
    Window Root = XDefaultRootWindow(GRAPHICS_STATE->Disp);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        Root
    );

    if (!Resources)
        return Result;

    i64 BestOverlap = -1;

    for (i32 Index = 0; Index < Resources->ncrtc; ++Index) {
        XRRCrtcInfo* DisplayInfo = XRRGetCrtcInfo(
            GRAPHICS_STATE->Disp,
            Resources,
            Resources->crtcs[Index]
        );

        if (!DisplayInfo || DisplayInfo->mode == None) {
            if (DisplayInfo)
                XRRFreeCrtcInfo(DisplayInfo);

            continue;
        }

        i64 DisplayRight = DisplayInfo->x + DisplayInfo->width;
        i64 DisplayBottom = DisplayInfo->y + DisplayInfo->height;

        if (
            WindowCentreX >= DisplayInfo->x &&
            WindowCentreX < DisplayRight &&
            WindowCentreY >= DisplayInfo->y &&
            WindowCentreY < DisplayBottom
        ) {
            Result.V[0] = INT_FROM_PTR(Resources->crtcs[Index]);
            XRRFreeCrtcInfo(DisplayInfo);
            break;
        }

        i64 OverlapX0 = (WindowX > DisplayInfo->x) ? WindowX : DisplayInfo->x;
        i64 OverlapY0 = (WindowY > DisplayInfo->y) ? WindowY : DisplayInfo->y;
        i64 OverlapX1 = ((WindowX + WindowAttributes.width) < DisplayRight)
            ? (WindowX + WindowAttributes.width)
            : DisplayRight;
        i64 OverlapY1 = ((WindowY + WindowAttributes.height) < DisplayBottom)
            ? (WindowY + WindowAttributes.height)
            : DisplayBottom;
        i64 Overlap = 0;

        if (OverlapX1 > OverlapX0 && OverlapY1 > OverlapY0)
            Overlap = (OverlapX1 - OverlapX0) * (OverlapY1 - OverlapY0);

        if (Overlap > BestOverlap) {
            BestOverlap = Overlap;
            Result.V[0] = INT_FROM_PTR(Resources->crtcs[Index]);
        }

        XRRFreeCrtcInfo(DisplayInfo);
    }

    XRRFreeScreenResources(Resources);

    return Result;
}

Str8 
NameFromMonitor(Arena* MemPool, Handle MonitorHandle)
{
    Str8 Result = {};

    if (MonitorHandle == EMPTY_HANDLE_VALUE)
        return Result;

    XID DisplayID = (XID) PTR_FROM_INT(MonitorHandle.V[0]);
    Window Root = XDefaultRootWindow(GRAPHICS_STATE->Disp);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        Root
    );

    if (!Resources)
        return Result;

    for (u32 Index = 0; Index < Resources->noutput; ++Index) {
        XRROutputInfo* DisplayInfo = XRRGetOutputInfo(
            GRAPHICS_STATE->Disp,
            Resources,
            Resources->outputs[Index]
        );

        if (!DisplayInfo)
            continue;

        if (DisplayInfo->crtc == DisplayID && DisplayInfo->name) {
            Result.Size = DisplayInfo->nameLen;
            Result.Str = ArenaPushArray(MemPool, u8, Result.Size + 1);
            MemCpy(Result.Str, DisplayInfo->name, DisplayInfo->nameLen);
            Result.Str[DisplayInfo->nameLen] = 0;
            XRRFreeOutputInfo(DisplayInfo);
            break;
        }

        XRRFreeOutputInfo(DisplayInfo);
    }

    XRRFreeScreenResources(Resources);

    return Result;
}

v2f32 
GetMonitorDimensions(Handle MonitorHandle)
{
    v2f32 Result = {};

    if (MonitorHandle == EMPTY_HANDLE_VALUE)
        return Result;

    XID DisplayID = (XID) MonitorHandle.V[0];
    Window Root = XDefaultRootWindow(GRAPHICS_STATE->Disp);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        Root
    );

    if (!Resources)
        return Result;

    XRRCrtcInfo* DisplayInfo = XRRGetCrtcInfo(
        GRAPHICS_STATE->Disp,
        Resources,
        DisplayID
    );

    if (DisplayInfo) {
        Result.X = (f32) DisplayInfo->width;
        Result.Y = (f32) DisplayInfo->height;
        XRRFreeCrtcInfo(DisplayInfo);
    }

    XRRFreeScreenResources(Resources);

    return Result;
}

f32 
GetMonitorDPI(Handle MonitorHandle)
{
    f32 Result = 96.0f;

    if (MonitorHandle == EMPTY_HANDLE_VALUE)
        return Result;

    XID DisplayID = (XID) MonitorHandle.V[0];
    Window Root = XDefaultRootWindow(GRAPHICS_STATE->Disp);
    XRRScreenResources* Resources = XRRGetScreenResourcesCurrent(
        GRAPHICS_STATE->Disp,
        Root
    );

    if (!Resources)
        return Result;

    for (u32 Index = 0; Index < Resources->noutput; ++Index) {
        XRROutputInfo* DisplayInfo = XRRGetOutputInfo(
            GRAPHICS_STATE->Disp,
            Resources,
            Resources->outputs[Index]
        );

        if (!DisplayInfo)
            continue;

        if (DisplayInfo->crtc == DisplayID) {
            if (DisplayInfo->mm_width > 0 && DisplayInfo->mm_height > 0) {
                XRRCrtcInfo* ScreenInfo = XRRGetCrtcInfo(
                    GRAPHICS_STATE->Disp,
                    Resources,
                    DisplayID
                );

                if (ScreenInfo) {
                    f32 DPIX = (f32) (ScreenInfo->width) / ((f32) DisplayInfo->mm_width / 25.4f);
                    f32 DPIY = (f32) (ScreenInfo->height) / ((f32) DisplayInfo->mm_height / 25.4f);

                    Result = (DPIX + DPIY) * 0.5f;
                    XRRFreeCrtcInfo(ScreenInfo);
                }
            }

            XRRFreeOutputInfo(DisplayInfo);
            break;
        }

        XRRFreeOutputInfo(DisplayInfo);
    }

    XRRFreeScreenResources(Resources);

    return Result;
}

void 
SendWakeupEvent(void)
{
    u64 __Temp = 1;
    ssize_t Size = LINUX_RETRY_ON_EINTR(
        write(GRAPHICS_STATE->WakeupFD, &__Temp, sizeof(__Temp))
    );

    ASSERT(Size == sizeof(__Temp));
}

INTERNAL InputModifierKind
InputModifierKindFromX11State(unsigned int State)
{
    InputModifierKind Result = (InputModifierKind) 0;

    if (State & ShiftMask)
        Result |= INPUT_MOD_KIND_SHIFT;

    if (State & ControlMask)
        Result |= INPUT_MOD_KIND_CTRL;

    if (State & Mod1Mask)
        Result |= INPUT_MOD_KIND_ALT;

    return Result;
}

InputEventList 
GetEvents(Arena* MemPool, b32 Wait)
{
    InputEventList Result = {};

    for (;;) {
        if (!XPending(GRAPHICS_STATE->Disp)) {
            struct pollfd PollFDs[2] = {};

            PollFDs[0].fd = ConnectionNumber(GRAPHICS_STATE->Disp);
            PollFDs[0].events = POLLIN;
            PollFDs[1].fd = GRAPHICS_STATE->WakeupFD;
            PollFDs[1].events = POLLIN;

            int Timeout = (Wait && !Result.Count)
                ? -1
                : 0;

            poll(PollFDs, ARRAY_COUNT(PollFDs), Timeout);

            if (PollFDs[1].revents & POLLIN) {
                u64 __Temp = 0;

                LINUX_RETRY_ON_EINTR(
                    read(GRAPHICS_STATE->WakeupFD, &__Temp, sizeof(__Temp))
                );
                Wait = FALSE;
            }
        }

        b32 TryAgain = FALSE;

        while (XPending(GRAPHICS_STATE->Disp)) {
            if (!GRAPHICS_STATE->IsXFixesPresent) {
                XConvertSelection(
                    GRAPHICS_STATE->Disp,
                    GRAPHICS_STATE->ClipboardAtom,
                    GRAPHICS_STATE->UTF8StringAtom,
                    GRAPHICS_STATE->ClipboardAtom,
                    GRAPHICS_STATE->__HIDDEN_WINDOW,
                    CurrentTime
                );
            }

            XEvent Event = {};

            XNextEvent(GRAPHICS_STATE->Disp, &Event);

            b32 SetMouseCursor = FALSE;

            switch (Event.type) {
                default: {
                    if (
                        Event.type == GRAPHICS_STATE->XFixesSelectionEventCode &&
                        GRAPHICS_STATE->IsXFixesPresent
                    ) {
                        XFixesSelectionNotifyEvent* Notification = (XFixesSelectionNotifyEvent*) &Event;

                        if (
                            Notification->subtype == XFixesSelectionNotify &&
                            Notification->owner != GRAPHICS_STATE->__HIDDEN_WINDOW
                        ) {
                            XConvertSelection(
                                GRAPHICS_STATE->Disp,
                                GRAPHICS_STATE->ClipboardAtom,
                                GRAPHICS_STATE->UTF8StringAtom,
                                GRAPHICS_STATE->ClipboardAtom,
                                GRAPHICS_STATE->__HIDDEN_WINDOW,
                                CurrentTime
                            );
                        }
                    }
                } break;

                case SelectionRequest: {
                    XSelectionRequestEvent* Request = &Event.xselectionrequest;
                    Atom DstProperty = (Request->property == None) 
                        ? Request->target 
                        : Request->property;

                    if (Request->target == GRAPHICS_STATE->TargetsAtom) {
                        Atom Formats[] = {
                            GRAPHICS_STATE->UTF8StringAtom,
                            XA_STRING
                        };

                        XChangeProperty(
                            Request->display,
                            Request->requestor,
                            DstProperty,
                            XA_ATOM,
                            32,
                            PropModeReplace,
                            (u8*) Formats,
                            ARRAY_COUNT(Formats)
                        );
                    } else if (
                        Request->target == GRAPHICS_STATE->UTF8StringAtom ||
                        Request->target == XA_STRING
                    ) {
                        XChangeProperty(
                            Request->display,
                            Request->requestor,
                            DstProperty,
                            Request->target,
                            8,
                            PropModeReplace,
                            GRAPHICS_STATE->ClipboardBuffer,
                            GRAPHICS_STATE->ClipboardBufferSize
                        );
                    }

                    XEvent Response = {};

                    Response.type = SelectionNotify;
                    Response.xselection.display = Request->display;
                    Response.xselection.requestor = Request->requestor;
                    Response.xselection.selection = Request->selection;
                    Response.xselection.target = Request->target;
                    Response.xselection.time = Request->time;
                    Response.xselection.property = DstProperty;
                    XSendEvent(
                        Request->display, 
                        Request->requestor, 
                        TRUE, 
                        0, 
                        &Response
                    );
                    XFlush(Request->display);
                    TryAgain = TRUE;
                } break;

                case SelectionNotify: {
                    Atom Type = 0;
                    int Format = 0;
                    u64 ItemsCount = 0;
                    u64 PostBytes = 0;
                    u8* Data = NULL;
                    int Result = XGetWindowProperty(
                        GRAPHICS_STATE->Disp,
                        GRAPHICS_STATE->__HIDDEN_WINDOW,
                        GRAPHICS_STATE->ClipboardAtom,
                        0,
                        MB(256),
                        0,
                        GRAPHICS_STATE->UTF8StringAtom,
                        &Type,
                        &Format,
                        &ItemsCount,
                        &PostBytes,
                        &Data
                    );

                    if (Result == Success && Format == 0) {
                        ArenaClear(GRAPHICS_STATE->ClipboardMemPool);
                        GRAPHICS_STATE->ClipboardBufferSize = ItemsCount;
                        GRAPHICS_STATE->ClipboardBuffer = ArenaPushArrayZero(
                            GRAPHICS_STATE->ClipboardMemPool,
                            u8,
                            GRAPHICS_STATE->ClipboardBufferSize + 1
                        );
                        MemCpy(
                            GRAPHICS_STATE->ClipboardBuffer,
                            Data,
                            GRAPHICS_STATE->ClipboardBufferSize
                        );
                        GRAPHICS_STATE->ClipboardBuffer[GRAPHICS_STATE->ClipboardBufferSize] = 0;
                        XFree(Data);
                        XDeleteProperty(
                            GRAPHICS_STATE->Disp,
                            GRAPHICS_STATE->__HIDDEN_WINDOW,
                            GRAPHICS_STATE->ClipboardAtom
                        );
                    }

                    TryAgain = TRUE;
                } break;

                case KeyPress:
                case KeyRelease: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xkey.window
                    );

                    if (!Win)
                        break;

                    InputModifierKind Modifiers = 0;

                    if (Event.xkey.state & ShiftMask)
                        Modifiers |= INPUT_MOD_KIND_SHIFT;

                    if (Event.xkey.state & ControlMask)
                        Modifiers |= INPUT_MOD_KIND_CTRL;

                    if (Event.xkey.state & Mod1Mask)
                        Modifiers |= INPUT_MOD_KIND_ALT;

                    KeySym Key = 0;
                    u8 Text[256] = {};
                    u64 TextSize = Xutf8LookupString(
                        Win->Xic,
                        &Event.xkey,
                        (char*) Text,
                        sizeof(Text),
                        &Key,
                        NULL
                    );
                    b32 IsRightSided = FALSE;
                    InputKind Input = InputKindFromKeySym(Key, &IsRightSided);

                    if (Event.type == KeyPress)
                        GRAPHICS_STATE->KeyDownState[Input / 64] |= (1ULL << (Input % 64));
                    else
                        GRAPHICS_STATE->KeyDownState[Input / 64] &= ~(1ULL << (Input % 64));

                    if (Event.type == KeyPress && TextSize) {
                        for (u64 Offset = 0; Offset < TextSize; ) {
                            UnicodeDecode Decode = UTF8Decode(
                                Text + Offset, 
                                TextSize - Offset
                            );

                            if (
                                Decode.CodePoint && 
                                Decode.CodePoint != 127 && 
                                Decode.CodePoint >= 32
                            ) {
                                InputEvent* Evt = ListPush(
                                    MemPool, 
                                    &Result, 
                                    EVENT_KIND_TEXT
                                );

                                Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                                Evt->Character = Decode.CodePoint;
                            }

                            if (!Decode.Inc)
                                break;

                            Offset += Decode.Inc;
                        }
                    }

                    {
                        InputEvent* Evt = ListPush(
                            MemPool,
                            &Result,
                            (Event.type == KeyPress) 
                                ? EVENT_KIND_PRESS 
                                : EVENT_KIND_RELEASE
                        );

                        Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                        Evt->Modifier = Modifiers;
                        Evt->Input = Input;
                        Evt->RightSided = IsRightSided;
                    }
                } break;

                case ButtonPress:
                case ButtonRelease: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xbutton.window
                    );

                    if (!Win)
                        break;

                    v2f32 Position = Vec(
                        (f32) Event.xbutton.x, 
                        (f32) Event.xbutton.y
                    );
                    b32 InClientArea = FALSE;
                    int MoveResizeCode = LinuxWindowMoveResizeCodeFromPos(
                        Win, 
                        Position, 
                        &InClientArea
                    );

                    if (!InClientArea) {
                        if (
                            Event.type == ButtonPress && 
                            Event.xbutton.button == Button1
                        ) {
                            Time DoubleClickMSecs = (Time) (
                                GRAPHICS_STATE->Properties.DoubleClickTime * 
                                1000.0f
                            );
                            int DeltaX = Event.xbutton.x_root - GRAPHICS_STATE->PrevTitleBarPressX;
                            int DeltaY = Event.xbutton.y_root - GRAPHICS_STATE->PrevTitleBarPressY;
                            b32 IsDoubleClick = (
                                MoveResizeCode == _NET_WM_MOVERESIZE_MOVE &&
                                GRAPHICS_STATE->PrevTitleBarPressWindow == Win->Win &&
                                GRAPHICS_STATE->PrevTitleBarPressTime &&
                                (
                                    Event.xbutton.time - GRAPHICS_STATE->PrevTitleBarPressTime
                                ) <= DoubleClickMSecs &&
                                DeltaX >= -4 &&
                                DeltaX <= 4 &&
                                DeltaY >= -4 &&
                                DeltaY <= 4
                            );

                            if (IsDoubleClick) {
                                Handle WindowHandle = {
                                    INT_FROM_PTR(Win)
                                };

                                GRAPHICS_STATE->PrevTitleBarPressTime = 0;
                                WindowSetMaximised(
                                    WindowHandle,
                                    !WindowIsMaximised(WindowHandle)
                                );
                            } else {
                                if (MoveResizeCode == _NET_WM_MOVERESIZE_MOVE) {
                                    GRAPHICS_STATE->PrevTitleBarPressWindow = Win->Win;
                                    GRAPHICS_STATE->PrevTitleBarPressTime = Event.xbutton.time;
                                    GRAPHICS_STATE->PrevTitleBarPressX = Event.xbutton.x_root;
                                    GRAPHICS_STATE->PrevTitleBarPressY = Event.xbutton.y_root;
                                }

                                XUngrabPointer(
                                    GRAPHICS_STATE->Disp,
                                    Event.xbutton.time
                                );

                                Atom NetWMMoveResize = XInternAtom(
                                    GRAPHICS_STATE->Disp,
                                    "_NET_WM_MOVERESIZE",
                                    0
                                );
                                XEvent XEvt = {};

                                XEvt.xclient.type = ClientMessage;
                                XEvt.xclient.display = GRAPHICS_STATE->Disp;
                                XEvt.xclient.window = Win->Win;
                                XEvt.xclient.message_type = NetWMMoveResize;
                                XEvt.xclient.format = 32;
                                XEvt.xclient.data.l[0] = Event.xbutton.x_root;
                                XEvt.xclient.data.l[1] = Event.xbutton.y_root;
                                XEvt.xclient.data.l[2] = MoveResizeCode;
                                XEvt.xclient.data.l[3] = Button1;
                                XEvt.xclient.data.l[4] = 1;
                                XSendEvent(
                                    GRAPHICS_STATE->Disp,
                                    DefaultRootWindow(GRAPHICS_STATE->Disp),
                                    0,
                                    SubstructureRedirectMask | SubstructureNotifyMask,
                                    &XEvt
                                );
                                XFlush(GRAPHICS_STATE->Disp);
                            }
                        }
                    } else {
                        InputModifierKind Modifiers = 0;

                        if (Event.xbutton.state & ShiftMask)
                            Modifiers |= INPUT_MOD_KIND_SHIFT;

                        if (Event.xbutton.state & ControlMask)
                            Modifiers |= INPUT_MOD_KIND_CTRL;

                        if (Event.xbutton.state & Mod1Mask)
                            Modifiers |= INPUT_MOD_KIND_ALT;

                        InputKind Input = INPUT_KIND_NULL;

                        switch (Event.xbutton.button) {
                            default: {} break;

                            case Button1: {
                                Input = INPUT_KIND_LEFT_MOUSE_BTN;
                            } break;

                            case Button2: {
                                Input = INPUT_KIND_MIDDLE_MOUSE_BTN;
                            } break;

                            case Button3: {
                                Input = INPUT_KIND_RIGHT_MOUSE_BTN;
                            } break;
                        }

                        if (Input != INPUT_KIND_NULL) {
                            InputEvent* Evt = ListPush(
                                MemPool,
                                &Result,
                                (Event.type == ButtonPress) 
                                    ? EVENT_KIND_PRESS 
                                    : EVENT_KIND_RELEASE
                            );

                            Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                            Evt->Modifier = Modifiers;
                            Evt->Input = Input;
                            Evt->Position = Position;

                            if (Event.type == ButtonPress)
                                GRAPHICS_STATE->KeyDownState[Input / 64] |= (1ull << (Input % 64));
                            else
                                GRAPHICS_STATE->KeyDownState[Input / 64] &= ~(1ull << (Input % 64));
                        } else if (
                            Event.xbutton.button == Button4 ||
                            Event.xbutton.button == Button5 ||
                            Event.xbutton.button == 6 ||
                            Event.xbutton.button == 7
                        ) {
                            InputEvent* Evt = ListPush(
                                MemPool, 
                                &Result, 
                                EVENT_KIND_SCROLL
                            );

                            Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                            Evt->Modifier = Modifiers;
                            Evt->PositionDelta = Vec(
                                (Event.xbutton.button == 6) 
                                    ? -120.0f 
                                    : (Event.xbutton.button == 7) 
                                        ? 120.0f 
                                        : 0.0f,
                                (Event.xbutton.button == Button4) 
                                    ? -120.0f 
                                    : (Event.xbutton.button == Button5) 
                                        ? 120.0f 
                                        : 0.0f
                            );
                            Evt->Position = Position;
                        }
                    }
                } break;

                case MotionNotify: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xmotion.window
                    );

                    if (!Win)
                        break;

                    InputEvent* Evt = ListPush(
                        MemPool, 
                        &Result, 
                        EVENT_KIND_MOUSE_MOVE
                    );

                    Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                    Evt->Position.X = (f32) Event.xmotion.x;
                    Evt->Position.Y = (f32) Event.xmotion.y;
                    SetMouseCursor = TRUE;

                    if (Win->CustomBorder) {
                        b32 InClientArea = FALSE;
                        int MoveResizeCode = LinuxWindowMoveResizeCodeFromPos(
                            Win, 
                            Evt->Position, 
                            &InClientArea
                        );

                        if (!InClientArea) {
                            switch (MoveResizeCode) {
                                default: {} break;

                                case _NET_WM_MOVERESIZE_SIZE_TOPLEFT:
                                case _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT: {
                                    GRAPHICS_STATE->PrevSetCursor = CURSOR_KIND_DOWN_RIGHT;
                                } break;

                                case _NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
                                case _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT: {
                                    GRAPHICS_STATE->PrevSetCursor = CURSOR_KIND_UP_RIGHT;
                                } break;

                                case _NET_WM_MOVERESIZE_SIZE_LEFT:
                                case _NET_WM_MOVERESIZE_SIZE_RIGHT: {
                                    GRAPHICS_STATE->PrevSetCursor = CURSOR_KIND_LEFT_RIGHT;
                                } break;

                                case _NET_WM_MOVERESIZE_SIZE_TOP:
                                case _NET_WM_MOVERESIZE_SIZE_BOTTOM: {
                                    GRAPHICS_STATE->PrevSetCursor = CURSOR_KIND_UP_DOWN;
                                } break;
                            }
                        }
                    }
                } break;

                case FocusIn: {} break;

                case FocusOut: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xfocus.window
                    );

                    if (!Win)
                        break;

                    InputEvent* Evt = ListPush(
                        MemPool, 
                        &Result, 
                        EVENT_KIND_WINDOW_LOSE_FOCUS
                    );

                    Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                } break;

                case ConfigureNotify: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xconfigure.window
                    );

                    if (Win && !Win->ResizeDraw) {
                        Win->PrevSyncedRect = Rng(
                            (f32) Event.xconfigure.x,
                            (f32) Event.xconfigure.y,
                            (f32) (Event.xconfigure.x + Event.xconfigure.width),
                            (f32) (Event.xconfigure.y + Event.xconfigure.height)
                        );
                    }
                } break;

                case Expose: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xexpose.window
                    );

                    if (Win && !XPending(GRAPHICS_STATE->Disp)) {
                        Win->ResizeDraw = Win->WaitingForResize;
                        Update();
                        Win->ResizeDraw = FALSE;

                        if (Win->WaitingForResize) {
                            Win->WaitingForResize = FALSE;

                            XSyncValue SyncValue;

                            XSyncIntsToValue(
                                &SyncValue,
                                (unsigned int) (Win->CounterValue & 0xFFFFFFFFU),
                                (int) (Win->CounterValue >> 32)
                            );
                            XSyncSetCounter(
                                GRAPHICS_STATE->Disp, 
                                Win->CounterXID, 
                                SyncValue
                            );
                            XFlush(GRAPHICS_STATE->Disp);
                        }
                    }
                } break;

                case ClientMessage: {
                    LinuxWindow* Win = X11WindowToLinuxWindow(
                        Event.xclient.window
                    );

                    if (!Win)
                        break;

                    if (
                        (Atom) Event.xclient.data.l[0] == GRAPHICS_STATE->DeleteWindowAtom
                    ) {
                        InputEvent* Evt = ListPush(
                            MemPool, 
                            &Result, 
                            EVENT_KIND_WINDOW_CLOSE
                        );

                        Evt->WindowHandle.V[0] = INT_FROM_PTR(Win);
                    } else if (
                        (Atom) Event.xclient.data.l[0] == GRAPHICS_STATE->SyncRequestAtom
                    ) {
                        u32 CounterLow = (u32) Event.xclient.data.l[2];
                        u32 CounterHigh = (u32) Event.xclient.data.l[3];

                        Win->CounterValue = CounterLow | ((u64) CounterHigh << 32);
                        Win->WaitingForResize = TRUE;
                    }
                } break;
            }

            if (SetMouseCursor) {
                Window RootWin = 0;
                Window ChildWin = 0;
                int RootX = 0;
                int RootY = 0;
                int ChildX = 0;
                int ChildY = 0;
                unsigned int Mask = 0;

                if (
                    XQueryPointer(
                        GRAPHICS_STATE->Disp,
                        XDefaultRootWindow(GRAPHICS_STATE->Disp),
                        &RootWin,
                        &ChildWin,
                        &RootX,
                        &RootY,
                        &ChildX,
                        &ChildY,
                        &Mask
                    )
                ) {
                    XDefineCursor(
                        GRAPHICS_STATE->Disp,
                        RootWin,
                        GRAPHICS_STATE->Cursors[GRAPHICS_STATE->PrevSetCursor]
                    );
                    XFlush(GRAPHICS_STATE->Disp);
                }
            }
        }

        if (!TryAgain && (Result.Count > 0 || !Wait))
            break;
    }

    return Result;
}

InputModifierKind 
GetModifiers(void)
{
    Window Root = 0;
    Window Child = 0;
    int RootX = 0;
    int RootY = 0;
    int WinX = 0;
    int WinY = 0;
    unsigned int Mask = 0;

    XQueryPointer(
        GRAPHICS_STATE->Disp,
        XDefaultRootWindow(GRAPHICS_STATE->Disp),
        &Root,
        &Child,
        &RootX,
        &RootY,
        &WinX,
        &WinY,
        &Mask
    );

    return InputModifierKindFromX11State(Mask);
}

b32 
KeyIsDown(InputKind Input)
{
    b32 Result = FALSE;

    switch (Input) {
        case INPUT_KIND_LEFT_MOUSE_BTN:
        case INPUT_KIND_MIDDLE_MOUSE_BTN:
        case INPUT_KIND_RIGHT_MOUSE_BTN: {
            Window Root = 0;
            Window Child = 0;
            int RootX = 0;
            int RootY = 0;
            int WinX = 0;
            int WinY = 0;
            unsigned int Mask = 0;

            XQueryPointer(
                GRAPHICS_STATE->Disp,
                XDefaultRootWindow(GRAPHICS_STATE->Disp),
                &Root,
                &Child,
                &RootX,
                &RootY,
                &WinX,
                &WinY,
                &Mask
            );

            unsigned int ButtonMask = (
                Input == INPUT_KIND_LEFT_MOUSE_BTN 
                    ? Button1Mask 
                    : Input == INPUT_KIND_MIDDLE_MOUSE_BTN 
                        ? Button2Mask
                        : Button3Mask
            );

            Result = !!(Mask & ButtonMask);
        } break;

        default: {
            KeySym Key = KeySymFromInputKind(Input);

            if (Key) {
                KeyCode Code = XKeysymToKeycode(GRAPHICS_STATE->Disp, Key);

                if (Code) {
                    char Keys[32] = {};

                    XQueryKeymap(GRAPHICS_STATE->Disp, Keys);
                    Result = !!(Keys[Code >> 3] & (1 << (Code & 7)));
                }
            }
        } break;
    }

    return Result;
}

v2f32 
GetMousePosition(Handle WindowHandle)
{
    v2f32 Result = {};

    if (WindowHandle == EMPTY_HANDLE_VALUE)
        return Result;

    LinuxWindow* Win = (LinuxWindow*) PTR_FROM_INT(WindowHandle.V[0]);
    Window Root = 0;
    Window Child = 0;
    int RootX = 0;
    int RootY = 0;
    int WinX = 0;
    int WinY = 0;
    unsigned int Mask = 0;

    if (
        XQueryPointer(
            GRAPHICS_STATE->Disp,
            Win->Win,
            &Root,
            &Child,
            &RootX,
            &RootY,
            &WinX,
            &WinY,
            &Mask
        )
    ) {
        Result = Vec((f32) WinX, (f32) WinY);
    }

    return Result;
}

void 
SetCursorKind(CursorKind Cursor)
{
    if (Cursor >= CURSOR_KIND_COUNT)
        return;

    GRAPHICS_STATE->PrevSetCursor = Cursor;

    for (
        LinuxWindow* Win = GRAPHICS_STATE->HeadWindow; 
        Win; 
        Win = Win->Next
    ) {
        XDefineCursor(
            GRAPHICS_STATE->Disp, 
            Win->Win, 
            GRAPHICS_STATE->Cursors[Cursor]
        );
    }

    XFlush(GRAPHICS_STATE->Disp);
}

void 
GraphicalMessage(b32 IsError, Str8 Title, Str8 Message)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str8 TitleCopy = ArenaPushStrCpy(Scratch.MemPool, Title);
    Str8 MessageCopy = ArenaPushStrCpy(Scratch.MemPool, Message);
    Display* DisplayHandle = XOpenDisplay(NULL);

    if (!DisplayHandle)
        return;

    Window MessageBoxHandle = XCreateSimpleWindow(
        DisplayHandle,
        DefaultRootWindow(DisplayHandle),
        100,
        100,
        400,
        200,
        1,
        BlackPixel(DisplayHandle, 0),
        WhitePixel(DisplayHandle, 0)
    );
    XStoreName(DisplayHandle, MessageBoxHandle, (char*) TitleCopy.Str);
    XSelectInput(DisplayHandle, MessageBoxHandle, ExposureMask | KeyPressMask);
    XMapWindow(DisplayHandle, MessageBoxHandle);

    GC GraphicsCtx = XCreateGC(DisplayHandle, MessageBoxHandle, 0, NULL);
    XEvent Event;

    for (;;) {
        XNextEvent(DisplayHandle, &Event);

        if (Event.type == Expose) {
            XDrawString(
                DisplayHandle,
                MessageBoxHandle,
                GraphicsCtx,
                20,
                50,
                (char*) MessageCopy.Str,
                MessageCopy.Size
            );
        } else if (Event.type == KeyPress) {
            break;
        }
    }

    XCloseDisplay(DisplayHandle);
}

// TODO(nc): add bare X11 dialogue boxes or call to zenity
Str8 
GraphicalFilePicker(Arena* MemPool, Str8 InitialPath)
{
    Str8 Result = ""_s8;

    return Result;
}

INTERNAL void
LinuxSpawnDetached(char* Program, char* Arg)
{
    pid_t PID = fork();

    if (PID == 0) {
        if (fork() == 0) {
            execlp(Program, Program, Arg, (char*) NULL);
            _exit(127);
        }

        _exit(0);
    } else if (PID > 0) {
        int Stat = 0;

        LINUX_RETRY_ON_EINTR(waitpid(PID, &Stat, 0));
    }
}

void 
ShowInFileExplorer(Str8 Path)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str8 Dir = ArenaPushStrCpy(Scratch.MemPool, StrChopLastSlash(Path));

    LinuxSpawnDetached("xdg-open", (char*) Dir.Str);
    ReleaseScratch(Scratch);
}

void 
OpenInBrowser(Str8 URL)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str8 URLCopy = ArenaPushStrCpy(Scratch.MemPool, URL);

    LinuxSpawnDetached("xdg-open", (char*) URLCopy.Str);
    ReleaseScratch(Scratch);
}
