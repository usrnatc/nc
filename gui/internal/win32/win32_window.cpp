#include "win32_platform.h"

struct WinTitleBarClientArea {
    WinTitleBarClientArea* Next;
    r2f32                  Rect;
};

struct WinWindow {
    WinWindow*             Next;
    WinWindow*             Prev;
    Arena*                 PaintMemPool;
    WinTitleBarClientArea* HeadTitleBarClientArea;
    WinTitleBarClientArea* TailTitleBarClientArea;
    HWND                   WindowHandle;
    HDC                    WindowDescriptor;
    WINDOWPLACEMENT        LastWindowPlacement;
    f32                    DPI;
    f32                    CustomBorderTitleThickness;
    f32                    CustomBorderEdgeThickness;
    b32                    FirstPaintCompleted;
    b32                    Maximised;
    b32                    CustomBorder;
    b32                    CustomBorderCompositionEnabled;
};

struct WinMonitorGatherBundle {
    Arena*      MemPool;
    HandleList* List;
};

struct GraphicsState {
    Arena*             MemPool;
    u32                ThreadID;
    HINSTANCE          Instance;
    HCURSOR            Cursor;
    GraphicsProperties Properties;
    WinWindow*         HeadWindow;
    WinWindow*         TailWindow;
    WinWindow*         FreeWindow;
    InputKind          InputKindFromCache[256];
};

EXTERN_C_LINK_BEGIN

Str8 KEYBOARD_CONFIG_STR_TABLE[143] = {
    Str8Lit("null"),
    Str8Lit("esc"),
    Str8Lit("f1"),
    Str8Lit("f2"),
    Str8Lit("f3"),
    Str8Lit("f4"),
    Str8Lit("f5"),
    Str8Lit("f6"),
    Str8Lit("f7"),
    Str8Lit("f8"),
    Str8Lit("f9"),
    Str8Lit("f10"),
    Str8Lit("f11"),
    Str8Lit("f12"),
    Str8Lit("f13"),
    Str8Lit("f14"),
    Str8Lit("f15"),
    Str8Lit("f16"),
    Str8Lit("f17"),
    Str8Lit("f18"),
    Str8Lit("f19"),
    Str8Lit("f20"),
    Str8Lit("f21"),
    Str8Lit("f22"),
    Str8Lit("f23"),
    Str8Lit("f24"),
    Str8Lit("tick"),
    Str8Lit("0"),
    Str8Lit("1"),
    Str8Lit("2"),
    Str8Lit("3"),
    Str8Lit("4"),
    Str8Lit("5"),
    Str8Lit("6"),
    Str8Lit("7"),
    Str8Lit("8"),
    Str8Lit("9"),
    Str8Lit("minus"),
    Str8Lit("equal"),
    Str8Lit("backspace"),
    Str8Lit("tab"),
    Str8Lit("q"),
    Str8Lit("w"),
    Str8Lit("e"),
    Str8Lit("r"),
    Str8Lit("t"),
    Str8Lit("y"),
    Str8Lit("u"),
    Str8Lit("i"),
    Str8Lit("o"),
    Str8Lit("p"),
    Str8Lit("left_bracket"),
    Str8Lit("right_bracket"),
    Str8Lit("backslash"),
    Str8Lit("caps_lock"),
    Str8Lit("a"),
    Str8Lit("s"),
    Str8Lit("d"),
    Str8Lit("f"),
    Str8Lit("g"),
    Str8Lit("h"),
    Str8Lit("j"),
    Str8Lit("k"),
    Str8Lit("l"),
    Str8Lit("semicolon"),
    Str8Lit("quote"),
    Str8Lit("return"),
    Str8Lit("shift"),
    Str8Lit("z"),
    Str8Lit("x"),
    Str8Lit("c"),
    Str8Lit("v"),
    Str8Lit("b"),
    Str8Lit("n"),
    Str8Lit("m"),
    Str8Lit("comma"),
    Str8Lit("period"),
    Str8Lit("slash"),
    Str8Lit("ctrl"),
    Str8Lit("alt"),
    Str8Lit("space"),
    Str8Lit("menu"),
    Str8Lit("scroll_lock"),
    Str8Lit("pause"),
    Str8Lit("insert"),
    Str8Lit("home"),
    Str8Lit("page_up"),
    Str8Lit("delete"),
    Str8Lit("end"),
    Str8Lit("page_down"),
    Str8Lit("up"),
    Str8Lit("left"),
    Str8Lit("down"),
    Str8Lit("right"),
    Str8Lit("ex0"),
    Str8Lit("ex1"),
    Str8Lit("ex2"),
    Str8Lit("ex3"),
    Str8Lit("ex4"),
    Str8Lit("ex5"),
    Str8Lit("ex6"),
    Str8Lit("ex7"),
    Str8Lit("ex8"),
    Str8Lit("ex9"),
    Str8Lit("ex10"),
    Str8Lit("ex11"),
    Str8Lit("ex12"),
    Str8Lit("ex13"),
    Str8Lit("ex14"),
    Str8Lit("ex15"),
    Str8Lit("ex16"),
    Str8Lit("ex17"),
    Str8Lit("ex18"),
    Str8Lit("ex19"),
    Str8Lit("ex20"),
    Str8Lit("ex21"),
    Str8Lit("ex22"),
    Str8Lit("ex23"),
    Str8Lit("ex24"),
    Str8Lit("ex25"),
    Str8Lit("ex26"),
    Str8Lit("ex27"),
    Str8Lit("ex28"),
    Str8Lit("ex29"),
    Str8Lit("num_lock"),
    Str8Lit("numpad_slash"),
    Str8Lit("numpad_star"),
    Str8Lit("numpad_minus"),
    Str8Lit("numpad_plus"),
    Str8Lit("numpad_period"),
    Str8Lit("numpad_0"),
    Str8Lit("numpad_1"),
    Str8Lit("numpad_2"),
    Str8Lit("numpad_3"),
    Str8Lit("numpad_4"),
    Str8Lit("numpad_5"),
    Str8Lit("numpad_6"),
    Str8Lit("numpad_7"),
    Str8Lit("numpad_8"),
    Str8Lit("numpad_9"),
    Str8Lit("left_mouse"),
    Str8Lit("middle_mouse"),
    Str8Lit("right_mouse")
};

EXTERN_C_LINK_END

#define WIN_DPI_AWARENESS_PER_MONITOR_AWARE ((void*) -4)

typedef BOOL WinSetProcessDPIAwarenessFuncType(void* Value);
typedef UINT WinGetDPIForWindowFuncType(HWND WindowHandle);
typedef HRESULT WinGetDPIForMonitorFuncType(HMONITOR MonitorHandle, MONITOR_DPI_TYPE DPIType, UINT* DPIX, UINT* DPIY);
typedef INT WinGetSystemMetricsForDPIFuncType(INT Index, UINT DPI);

global WinGetDPIForWindowFuncType* WinGetDPIForWindowFunc = NULL;
global WinGetDPIForMonitorFuncType* WinGetDPIForMonitorFunc = NULL;
global WinGetSystemMetricsForDPIFuncType* WinGetSystemMetricsForDPIFunc = NULL;

GraphicsState* GRAPHICS_STATE = NULL;
InputEventList EVENT_LIST = {};
Arena* GRAPHICS_MEMPOOL = NULL;
b32 WINDOW_RESIZING = FALSE;
b32 NEW_WINDOW_CUSTOM_BORDER = FALSE;

Handle 
HandleFromWindow(WinWindow* Window)
{
    Handle Result = {};

    *Result.V = INT_FROM_PTR(Window);

    return Result;
}

WinWindow*
WindowFromHandle(Handle WindowHandle)
{
    WinWindow* Result = (WinWindow*) PTR_FROM_INT(*WindowHandle.V);

    return Result;
}

WinWindow*
WindowFromHWND(HWND WindowHandle)
{
    WinWindow* Result = NULL;

    for (WinWindow* Window = GRAPHICS_STATE->HeadWindow; Window; Window = Window->Next) {
        if (Window->WindowHandle == WindowHandle) {
            Result = Window;
            break;
        }
    }

    return Result;
}

HWND 
HWNDFromWindow(WinWindow* Window)
{
    return Window->WindowHandle;
}

WinWindow* 
WindowAlloc(void)
{
    WinWindow* Result = GRAPHICS_STATE->FreeWindow;

    if (Result)
        SLL_STACK_POP(GRAPHICS_STATE->FreeWindow);
    else
        Result = ArenaPushArray(GRAPHICS_STATE->MemPool, WinWindow, 1);

    MemSet(Result, 0, sizeof(*Result));

    if (Result)
        DLL_PUSH_BACK(GRAPHICS_STATE->HeadWindow, GRAPHICS_STATE->TailWindow, Result);

    Result->LastWindowPlacement.length = sizeof(WINDOWPLACEMENT);

    return Result;
}

void 
WindowRelease(WinWindow* Window)
{
    if (Window->PaintMemPool)
        ArenaRelease(Window->PaintMemPool);

    ReleaseDC(Window->WindowHandle, Window->WindowDescriptor);
    DestroyWindow(Window->WindowHandle);
    DLL_REMOVE(GRAPHICS_STATE->HeadWindow, GRAPHICS_STATE->TailWindow, Window);
    SLL_STACK_PUSH(GRAPHICS_STATE->FreeWindow, Window);
}

InputEvent* 
PushEvent(InputEventKind Kind, WinWindow* Window)
{
    InputEvent* Result = ListPush(GRAPHICS_MEMPOOL, &EVENT_LIST, Kind);

    Result->WindowHandle = HandleFromWindow(Window);
    Result->Modifier = GetModifiers();

    return Result;
}

InputKind 
InputKindFromVKey(WPARAM VKey)
{
    local b32 First = TRUE;
    local InputKind InputKindTable[256];

    if (First) {
        First = FALSE;

        MemSet(InputKindTable, 0, ARRAY_COUNT(InputKindTable) * sizeof(*InputKindTable));

        InputKindTable[(unsigned int) 'A'] = INPUT_KIND_A;
        InputKindTable[(unsigned int) 'B'] = INPUT_KIND_B;
        InputKindTable[(unsigned int) 'C'] = INPUT_KIND_C;
        InputKindTable[(unsigned int) 'D'] = INPUT_KIND_D;
        InputKindTable[(unsigned int) 'E'] = INPUT_KIND_E;
        InputKindTable[(unsigned int) 'F'] = INPUT_KIND_F;
        InputKindTable[(unsigned int) 'G'] = INPUT_KIND_G;
        InputKindTable[(unsigned int) 'H'] = INPUT_KIND_H;
        InputKindTable[(unsigned int) 'I'] = INPUT_KIND_I;
        InputKindTable[(unsigned int) 'J'] = INPUT_KIND_J;
        InputKindTable[(unsigned int) 'K'] = INPUT_KIND_K;
        InputKindTable[(unsigned int) 'L'] = INPUT_KIND_L;
        InputKindTable[(unsigned int) 'M'] = INPUT_KIND_M;
        InputKindTable[(unsigned int) 'N'] = INPUT_KIND_N;
        InputKindTable[(unsigned int) 'O'] = INPUT_KIND_O;
        InputKindTable[(unsigned int) 'P'] = INPUT_KIND_P;
        InputKindTable[(unsigned int) 'Q'] = INPUT_KIND_Q;
        InputKindTable[(unsigned int) 'R'] = INPUT_KIND_R;
        InputKindTable[(unsigned int) 'S'] = INPUT_KIND_S;
        InputKindTable[(unsigned int) 'T'] = INPUT_KIND_T;
        InputKindTable[(unsigned int) 'U'] = INPUT_KIND_U;
        InputKindTable[(unsigned int) 'V'] = INPUT_KIND_V;
        InputKindTable[(unsigned int) 'W'] = INPUT_KIND_W;
        InputKindTable[(unsigned int) 'X'] = INPUT_KIND_X;
        InputKindTable[(unsigned int) 'Y'] = INPUT_KIND_Y;
        InputKindTable[(unsigned int) 'Z'] = INPUT_KIND_Z;
        
        for (u64 I = '0', J = INPUT_KIND_0; I <= '9'; ++I, ++J)
            InputKindTable[I] = (InputKind) J;

        for (u64 I = VK_NUMPAD0, J = INPUT_KIND_0; I <= VK_NUMPAD9; ++I, ++J)
            InputKindTable[I] = (InputKind) J;

        for (u64 I = VK_F1, J = INPUT_KIND_FUNC_1; I <= VK_F24; ++I, ++J)
            InputKindTable[I] = (InputKind) J;
        
        InputKindTable[VK_SPACE]     = INPUT_KIND_SPACE;
        InputKindTable[VK_OEM_3]     = INPUT_KIND_TILDE;
        InputKindTable[VK_OEM_MINUS] = INPUT_KIND_MINUS;
        InputKindTable[VK_OEM_PLUS]  = INPUT_KIND_EQUAL;
        InputKindTable[VK_OEM_4]     = INPUT_KIND_LEFTBRACKET;
        InputKindTable[VK_OEM_6]     = INPUT_KIND_RIGHTBRACKET;
        InputKindTable[VK_OEM_1]     = INPUT_KIND_SEMICOLON;
        InputKindTable[VK_OEM_7]     = INPUT_KIND_QUOTE;
        InputKindTable[VK_OEM_COMMA] = INPUT_KIND_COMMA;
        InputKindTable[VK_OEM_PERIOD]= INPUT_KIND_PERIOD;
        InputKindTable[VK_OEM_2]     = INPUT_KIND_SLASH;
        InputKindTable[VK_OEM_5]     = INPUT_KIND_BACKSLASH;
        InputKindTable[VK_TAB]       = INPUT_KIND_TAB;
        InputKindTable[VK_PAUSE]     = INPUT_KIND_PAUSE;
        InputKindTable[VK_ESCAPE]    = INPUT_KIND_ESC;
        InputKindTable[VK_UP]        = INPUT_KIND_UP;
        InputKindTable[VK_LEFT]      = INPUT_KIND_LEFT;
        InputKindTable[VK_DOWN]      = INPUT_KIND_DOWN;
        InputKindTable[VK_RIGHT]     = INPUT_KIND_RIGHT;
        InputKindTable[VK_BACK]      = INPUT_KIND_BACKSPACE;
        InputKindTable[VK_RETURN]    = INPUT_KIND_RETURN;
        InputKindTable[VK_DELETE]    = INPUT_KIND_DELETE;
        InputKindTable[VK_INSERT]    = INPUT_KIND_INSERT;
        InputKindTable[VK_PRIOR]     = INPUT_KIND_PAGEUP;
        InputKindTable[VK_NEXT]      = INPUT_KIND_PAGEDOWN;
        InputKindTable[VK_HOME]      = INPUT_KIND_HOME;
        InputKindTable[VK_END]       = INPUT_KIND_END;
        InputKindTable[VK_CAPITAL]   = INPUT_KIND_CAPSLOCK;
        InputKindTable[VK_NUMLOCK]   = INPUT_KIND_NUMLOCK;
        InputKindTable[VK_SCROLL]    = INPUT_KIND_SCROLLLOCK;
        InputKindTable[VK_APPS]      = INPUT_KIND_SUPER;
        InputKindTable[VK_CONTROL]   = INPUT_KIND_CTRL;
        InputKindTable[VK_LCONTROL]  = INPUT_KIND_CTRL;
        InputKindTable[VK_RCONTROL]  = INPUT_KIND_CTRL;
        InputKindTable[VK_SHIFT]     = INPUT_KIND_SHIFT;
        InputKindTable[VK_LSHIFT]    = INPUT_KIND_SHIFT;
        InputKindTable[VK_RSHIFT]    = INPUT_KIND_SHIFT;
        InputKindTable[VK_MENU]      = INPUT_KIND_ALT;
        InputKindTable[VK_LMENU]     = INPUT_KIND_ALT;
        InputKindTable[VK_RMENU]     = INPUT_KIND_ALT;
        InputKindTable[VK_DIVIDE]    = INPUT_KIND_NUM_SLASH;
        InputKindTable[VK_MULTIPLY]  = INPUT_KIND_NUM_STAR;
        InputKindTable[VK_SUBTRACT]  = INPUT_KIND_NUM_MINUS;
        InputKindTable[VK_ADD]       = INPUT_KIND_NUM_PLUS;
        InputKindTable[VK_DECIMAL]   = INPUT_KIND_NUM_PERIOD;
        
        for (u32 I = 0; I < 10; ++I)
            InputKindTable[VK_NUMPAD0 + I] = (InputKind) ((u64) INPUT_KIND_NUM_0 + I);
        
        for (u64 I = 0xDF, J = 0; I < 0xFF; ++I, ++J)
            InputKindTable[I] = (InputKind) ((u64) INPUT_KIND_EX0 + J);
    }

    InputKind Result = InputKindTable[VKey & 0x000000FF];

    return Result;
}

WPARAM 
VKeyFromInputKind(InputKind Input)
{
    WPARAM Result = 0;

    {
        local b32 Initialized = FALSE;
        local WPARAM VKeyTable[INPUT_KIND_COUNT] = {};

        if(!Initialized) {
            Initialized = TRUE;
            VKeyTable[INPUT_KIND_ESC] = VK_ESCAPE;

            for(InputKind Key = INPUT_KIND_FUNC_1; Key <= INPUT_KIND_FUNC_24; Key = (InputKind) (Key + 1))
                VKeyTable[Key] = VK_F1 + (Key - INPUT_KIND_FUNC_1);

            VKeyTable[INPUT_KIND_TILDE] = VK_OEM_3;

            for(InputKind Key = INPUT_KIND_0; Key <= INPUT_KIND_9; Key = (InputKind) (Key + 1))
                VKeyTable[Key] = '0' + (Key - INPUT_KIND_0);

            VKeyTable[INPUT_KIND_MINUS]            = VK_OEM_MINUS;
            VKeyTable[INPUT_KIND_EQUAL]            = VK_OEM_PLUS;
            VKeyTable[INPUT_KIND_BACKSPACE]        = VK_BACK;
            VKeyTable[INPUT_KIND_TAB]              = VK_TAB;
            VKeyTable[INPUT_KIND_Q]                = 'Q';
            VKeyTable[INPUT_KIND_W]                = 'W';
            VKeyTable[INPUT_KIND_E]                = 'E';
            VKeyTable[INPUT_KIND_R]                = 'R';
            VKeyTable[INPUT_KIND_T]                = 'T';
            VKeyTable[INPUT_KIND_Y]                = 'Y';
            VKeyTable[INPUT_KIND_U]                = 'U';
            VKeyTable[INPUT_KIND_I]                = 'I';
            VKeyTable[INPUT_KIND_O]                = 'O';
            VKeyTable[INPUT_KIND_P]                = 'P';
            VKeyTable[INPUT_KIND_LEFTBRACKET]      = VK_OEM_4;
            VKeyTable[INPUT_KIND_RIGHTBRACKET]     = VK_OEM_6;
            VKeyTable[INPUT_KIND_BACKSLASH]        = VK_OEM_5;
            VKeyTable[INPUT_KIND_CAPSLOCK]         = VK_CAPITAL;
            VKeyTable[INPUT_KIND_A]                = 'A';
            VKeyTable[INPUT_KIND_S]                = 'S';
            VKeyTable[INPUT_KIND_D]                = 'D';
            VKeyTable[INPUT_KIND_F]                = 'F';
            VKeyTable[INPUT_KIND_G]                = 'G';
            VKeyTable[INPUT_KIND_H]                = 'H';
            VKeyTable[INPUT_KIND_J]                = 'J';
            VKeyTable[INPUT_KIND_K]                = 'K';
            VKeyTable[INPUT_KIND_L]                = 'L';
            VKeyTable[INPUT_KIND_SEMICOLON]        = VK_OEM_1;
            VKeyTable[INPUT_KIND_QUOTE]            = VK_OEM_7;
            VKeyTable[INPUT_KIND_RETURN]           = VK_RETURN;
            VKeyTable[INPUT_KIND_SHIFT]            = VK_SHIFT;
            VKeyTable[INPUT_KIND_Z]                = 'Z';
            VKeyTable[INPUT_KIND_X]                = 'X';
            VKeyTable[INPUT_KIND_C]                = 'C';
            VKeyTable[INPUT_KIND_V]                = 'V';
            VKeyTable[INPUT_KIND_B]                = 'B';
            VKeyTable[INPUT_KIND_N]                = 'N';
            VKeyTable[INPUT_KIND_M]                = 'M';
            VKeyTable[INPUT_KIND_COMMA]            = VK_OEM_COMMA;
            VKeyTable[INPUT_KIND_PERIOD]           = VK_OEM_PERIOD;
            VKeyTable[INPUT_KIND_SLASH]            = VK_OEM_2;
            VKeyTable[INPUT_KIND_CTRL]             = VK_CONTROL;
            VKeyTable[INPUT_KIND_ALT]              = VK_MENU;
            VKeyTable[INPUT_KIND_SPACE]            = VK_SPACE;
            VKeyTable[INPUT_KIND_SUPER]            = VK_APPS;
            VKeyTable[INPUT_KIND_SCROLLLOCK]       = VK_SCROLL;
            VKeyTable[INPUT_KIND_PAUSE]            = VK_PAUSE;
            VKeyTable[INPUT_KIND_INSERT]           = VK_INSERT;
            VKeyTable[INPUT_KIND_HOME]             = VK_HOME;
            VKeyTable[INPUT_KIND_PAGEUP]           = VK_PRIOR;
            VKeyTable[INPUT_KIND_DELETE]           = VK_DELETE;
            VKeyTable[INPUT_KIND_END]              = VK_END;
            VKeyTable[INPUT_KIND_PAGEDOWN]         = VK_NEXT;
            VKeyTable[INPUT_KIND_UP]               = VK_UP;
            VKeyTable[INPUT_KIND_LEFT]             = VK_LEFT;
            VKeyTable[INPUT_KIND_DOWN]             = VK_DOWN;
            VKeyTable[INPUT_KIND_RIGHT]            = VK_RIGHT;

            for(InputKind Key = INPUT_KIND_EX0; Key <= INPUT_KIND_EX29; Key = (InputKind) (Key + 1))
                VKeyTable[Key] = 0xDF + (Key - INPUT_KIND_EX0);

            VKeyTable[INPUT_KIND_NUMLOCK]          = VK_NUMLOCK;
            VKeyTable[INPUT_KIND_NUM_SLASH]        = VK_DIVIDE;
            VKeyTable[INPUT_KIND_NUM_STAR]         = VK_MULTIPLY;
            VKeyTable[INPUT_KIND_NUM_MINUS]        = VK_SUBTRACT;
            VKeyTable[INPUT_KIND_NUM_PLUS]         = VK_ADD;
            VKeyTable[INPUT_KIND_NUM_PERIOD]       = VK_DECIMAL;
            VKeyTable[INPUT_KIND_NUM_0]            = VK_NUMPAD0;
            VKeyTable[INPUT_KIND_NUM_1]            = VK_NUMPAD1;
            VKeyTable[INPUT_KIND_NUM_2]            = VK_NUMPAD2;
            VKeyTable[INPUT_KIND_NUM_3]            = VK_NUMPAD3;
            VKeyTable[INPUT_KIND_NUM_4]            = VK_NUMPAD4;
            VKeyTable[INPUT_KIND_NUM_5]            = VK_NUMPAD5;
            VKeyTable[INPUT_KIND_NUM_6]            = VK_NUMPAD6;
            VKeyTable[INPUT_KIND_NUM_7]            = VK_NUMPAD7;
            VKeyTable[INPUT_KIND_NUM_8]            = VK_NUMPAD8;
            VKeyTable[INPUT_KIND_NUM_9]            = VK_NUMPAD9;
            VKeyTable[INPUT_KIND_LEFT_MOUSE_BTN]   = VK_LBUTTON;
            VKeyTable[INPUT_KIND_MIDDLE_MOUSE_BTN] = VK_MBUTTON;
            VKeyTable[INPUT_KIND_RIGHT_MOUSE_BTN]  = VK_RBUTTON;
        }

        Result = VKeyTable[Input];
    }

    return Result;
}

LRESULT 
WinWindowFunc(HWND WindowHandle, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    b32 Success = TRUE;

    if (!GRAPHICS_MEMPOOL) {
        Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
        Success = FALSE;

        return Result;
    }

    WinWindow* Window = WindowFromHWND(WindowHandle);
    Handle WinHandle = HandleFromWindow(Window);
    b32 Release = FALSE;

    switch (Message) {
        default: {
            Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
        } break;

        case WM_ENTERSIZEMOVE: {
            WINDOW_RESIZING = TRUE;
        } break;

        case WM_EXITSIZEMOVE: {
            WINDOW_RESIZING = FALSE;
        } break;

        case WM_SIZE:
        case WM_PAINT: {
            PAINTSTRUCT PaintStruct = {};

            BeginPaint(WindowHandle, &PaintStruct);
            Update();
            EndPaint(WindowHandle, &PaintStruct);
        } break;

        case WM_CLOSE: {
            PushEvent(EVENT_KIND_WINDOW_CLOSE, Window);
        } break;

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP: {
            Release = TRUE;
        }

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN: {
            InputEvent* Event = NULL;

            if (Release)
                Event = PushEvent(EVENT_KIND_RELEASE, Window);
            else
                Event = PushEvent(EVENT_KIND_PRESS, Window);

            switch (Message) {
                case WM_LBUTTONUP:
                case WM_LBUTTONDOWN: {
                    Event->Input = INPUT_KIND_LEFT_MOUSE_BTN;
                } break;

                case WM_MBUTTONUP:
                case WM_MBUTTONDOWN: {
                    Event->Input = INPUT_KIND_MIDDLE_MOUSE_BTN;
                } break;

                case WM_RBUTTONUP:
                case WM_RBUTTONDOWN: {
                    Event->Input = INPUT_KIND_RIGHT_MOUSE_BTN;
                } break;
            }

            Event->Position.X = (f32) (i16) LOWORD(LParam);
            Event->Position.Y = (f32) (i16) HIWORD(LParam);

            if (Release)
                ReleaseCapture();
            else
                SetCapture(WindowHandle);
        } break;

        case WM_MOUSEMOVE: {
            InputEvent* Event = PushEvent(EVENT_KIND_MOUSE_MOVE, Window);

            Event->Position.X = (f32) (i16) LOWORD(LParam);
            Event->Position.Y = (f32) (i16) HIWORD(LParam);
        } break;

        case WM_MOUSEWHEEL: {
            i16 WheelDelta = HIWORD(WParam);
            InputEvent* Event = PushEvent(EVENT_KIND_SCROLL, Window);
            POINT Point = {};

            Point.x = (i32)(i16) LOWORD(LParam);
            Point.y = (i32)(i16) HIWORD(LParam);
            ScreenToClient(Window->WindowHandle, &Point);
            Event->Position.X = (f32) Point.x;
            Event->Position.Y = (f32) Point.y;
            Event->PositionDelta = Vec(0.0f, -(f32) WheelDelta);
        } break;

        case WM_MOUSEHWHEEL: {
            i16 WheelDelta = HIWORD(WParam);
            InputEvent* Event = PushEvent(EVENT_KIND_SCROLL, Window);
            POINT Point = {};

            Point.x = (i32)(i16) LOWORD(LParam);
            Point.y = (i32)(i16) HIWORD(LParam);
            ScreenToClient(Window->WindowHandle, &Point);
            Event->Position.X = (f32) Point.x;
            Event->Position.Y = (f32) Point.y;
            Event->PositionDelta = Vec((f32) WheelDelta, 0.0f);
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP: {
            if (WParam != VK_MENU && (WParam < VK_F1 || WParam > VK_F24 || WParam == VK_F4))
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
        }

        case WM_KEYDOWN:
        case WM_KEYUP: {
            b32 WasDown = (LParam & (1 << 30));
            b32 IsDown = !(LParam & (1 << 31));
            b32 IsRepeat = FALSE;

            if (!IsDown)
                Release = TRUE;
            else if (WasDown)
                IsRepeat = TRUE;

            b32 RightSided = FALSE;

            if (
                (LParam & (1 << 24)) &&
                (
                    WParam == VK_CONTROL ||
                    WParam == VK_RCONTROL ||
                    WParam == VK_MENU ||
                    WParam == VK_RMENU ||
                    WParam == VK_SHIFT ||
                    WParam == VK_RSHIFT
                )
            ) {
                RightSided = TRUE;
            }

            InputEvent* Event;

            if (Release)
                Event = PushEvent(EVENT_KIND_RELEASE, Window);
            else
                Event = PushEvent(EVENT_KIND_PRESS, Window);

            Event->Input = InputKindFromVKey(WParam);
            Event->RepeatCount = (LParam & 0x0000FFFF);
            Event->IsRepeat = IsRepeat;
            Event->RightSided = RightSided;

            if (Event->Input == INPUT_KIND_ALT && Event->Modifier & INPUT_MOD_KIND_ALT)
                Event->Modifier &= ~INPUT_MOD_KIND_ALT;

            if (Event->Input == INPUT_KIND_CTRL && Event->Modifier & INPUT_MOD_KIND_CTRL)
                Event->Modifier &= ~INPUT_MOD_KIND_CTRL;

            if (Event->Input == INPUT_KIND_SHIFT && Event->Modifier & INPUT_MOD_KIND_SHIFT)
                Event->Modifier &= ~INPUT_MOD_KIND_SHIFT;
        } break;

        case WM_SYSCHAR: {
            WORD VKCode = LOWORD(WParam);

            if (VKCode == VK_SPACE)
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            else
                Result = 0;
        } break;

        case WM_CHAR: {
            u32 Character = WParam;

            if (Character >= 32 && Character != 127) {
                InputEvent* Event = PushEvent(EVENT_KIND_TEXT, Window);

                if (LParam & (1 << 28))
                    Event->Modifier |= INPUT_MOD_KIND_ALT;

                Event->Character = Character;
            }
        } break;

        case WM_KILLFOCUS: {
            PushEvent(EVENT_KIND_WINDOW_LOSE_FOCUS, Window);
            ReleaseCapture();
        } break;

        case WM_SETCURSOR: {
            r2f32 WindowRect = GetWindowClientRect(WinHandle);
            v2f32 Mouse = GetMousePosition(WinHandle);
            b32 OnBorder = FALSE;
            DWORD WindowStyle = 0;

            if (Window)
                WindowStyle = GetWindowLong(Window->WindowHandle, GWL_STYLE);

            b32 IsFullScreen = !(WindowStyle & WS_OVERLAPPEDWINDOW);

            if (Window && Window->CustomBorder && !IsFullScreen) {
                b32 OnBorderX = (
                    Mouse.X <= Window->CustomBorderEdgeThickness || 
                    WindowRect.X1 - Window->CustomBorderEdgeThickness <= Mouse.X
                );
                b32 OnBorderY = (
                    Mouse.Y <= Window->CustomBorderEdgeThickness || 
                    WindowRect.Y1 - Window->CustomBorderEdgeThickness <= Mouse.Y
                );

                OnBorder = (OnBorderX || OnBorderY);
            }

            if (!WINDOW_RESIZING && !OnBorder && InRange(WindowRect, Mouse))
                SetCursor(GRAPHICS_STATE->Cursor);
            else
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
        } break;

        case WM_DPICHANGED: {
            f32 NewDPI = (f32) (WParam & 0xFFFF);

            Window->DPI = NewDPI;
        } break;

        case WM_DROPFILES: {
            HDROP Drop = (HDROP) WParam;
            POINT DropPoint = {};

            DragQueryPoint(Drop, &DropPoint);

            u64 NumFilesDropped = DragQueryFile(Drop, U32_MAX, NULL, 0);
            InputEvent* Event = PushEvent(EVENT_KIND_FILE_DROP, Window);

            Event->Position = Vec((f32) DropPoint.x, (f32) DropPoint.y);

            for (u64 Index = 0; Index < NumFilesDropped; ++Index) {
                u64 NameSize = DragQueryFile(Drop, Index, NULL, 0) + 1;
                u8* NamePtr = ArenaPushArray(GRAPHICS_MEMPOOL, u8, NameSize);

                DragQueryFileA(Drop, Index, (char*) NamePtr, NameSize);
                ListPush(GRAPHICS_MEMPOOL, &Event->Strings, Str(NamePtr, NameSize - 1));
            }

            DragFinish(Drop);
        } break;

        case WM_NCPAINT: {
            if (
                NEW_WINDOW_CUSTOM_BORDER ||
                (
                    Window && 
                    Window->CustomBorder && 
                    !Window->CustomBorderCompositionEnabled
                )
            ) {
                Result = 0;
            } else {
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            }
        } break;

        case WM_DWMCOMPOSITIONCHANGED: {
            if (Window && Window->CustomBorder) {
                BOOL Enabled = FALSE;

                DwmIsCompositionEnabled(&Enabled);
                Window->CustomBorderCompositionEnabled = Enabled;

                if (Enabled) {
                    MARGINS Margins = {
                        0,
                        0,
                        1,
                        0
                    };

                    DwmExtendFrameIntoClientArea(WindowHandle, &Margins);

                    DWORD DWMCRPEnabled = DWMNCRP_ENABLED;

                    DwmSetWindowAttribute(
                        WindowHandle, 
                        DWMWA_NCRENDERING_POLICY, 
                        &Enabled, 
                        sizeof(DWMCRPEnabled)
                    );
                }
            } else {
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            }
        } break;

        case WM_WINDOWPOSCHANGED: {
            Result = 0;
        } break;

        case WM_NCUAHDRAWCAPTION:
        case WM_NCUAHDRAWFRAME: {
            if (NEW_WINDOW_CUSTOM_BORDER || (Window && Window->CustomBorder))
                Result = 0;
            else
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
        } break;

        case WM_SETICON:
        case WM_SETTEXT: {
            if (
                NEW_WINDOW_CUSTOM_BORDER ||
                (
                    Window && Window->CustomBorder && !Window->CustomBorderCompositionEnabled
                )
            ) {
                LONG_PTR OldStyle = GetWindowLongPtrW(WindowHandle, GWL_STYLE);

                SetWindowLongPtrW(WindowHandle, GWL_STYLE, OldStyle & ~WS_VISIBLE);
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
                SetWindowLongPtrW(WindowHandle, GWL_STYLE, OldStyle);
            } else {
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            }
        } break;

        case WM_NCACTIVATE: {
            if (!NEW_WINDOW_CUSTOM_BORDER && (!Window || !Window->CustomBorder))
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            else
                Result = DefWindowProcW(WindowHandle, Message, WParam, -1);
        } break;

        case WM_NCCALCSIZE: {
            if (NEW_WINDOW_CUSTOM_BORDER || (Window && Window->CustomBorder)) {
                f32 DPI = WinGetDPIForWindowFunc 
                    ? (f32) WinGetDPIForWindowFunc(WindowHandle) 
                    : 96.0f;
                i32 FrameX = (WinGetSystemMetricsForDPIFunc)
                    ? WinGetSystemMetricsForDPIFunc(SM_CXFRAME, DPI) 
                    : GetSystemMetrics(SM_CXFRAME);
                i32 FrameY = (WinGetSystemMetricsForDPIFunc)
                    ? WinGetSystemMetricsForDPIFunc(SM_CYFRAME, DPI) 
                    : GetSystemMetrics(SM_CYFRAME);
                i32 Padding = (WinGetSystemMetricsForDPIFunc)
                    ? WinGetSystemMetricsForDPIFunc(SM_CXPADDEDBORDER, DPI) 
                    : GetSystemMetrics(SM_CXPADDEDBORDER);
                DWORD WindowStyle = GetWindowLong(WindowHandle, GWL_STYLE);
                b32 IsFullScreen = !(WindowStyle & WS_OVERLAPPEDWINDOW);

                if (!IsFullScreen) {
                    RECT* Rect = (WParam == 0) ? (RECT*) LParam : ((NCCALCSIZE_PARAMS*) LParam)->rgrc;

                    Rect->right -= FrameX + Padding;
                    Rect->left += FrameX + Padding;
                    Rect->bottom -= FrameY + Padding;

                    if (IsMaximized(WindowHandle)) {
                        Rect->top += FrameY + Padding;
                        Rect->bottom -= 1;
                    }
                }

                Result = (WParam != 0) ? WVR_REDRAW : 0;
            } else {
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            }
        } break;

        case WM_NCHITTEST: {
            DWORD WindowStyle = (Window) ? GetWindowLong(Window->WindowHandle, GWL_STYLE) : 0;
            b32 IsFullScreen = !(WindowStyle & WS_OVERLAPPEDWINDOW);

            if (!Window || !Window->CustomBorder || IsFullScreen) {
                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);
            } else {
                b32 IsDefaultHandled = FALSE;

                Result = DefWindowProcW(WindowHandle, Message, WParam, LParam);

                switch (Result) {
                    case HTNOWHERE:
                    case HTRIGHT:
                    case HTLEFT:
                    case HTTOPLEFT:
                    case HTTOPRIGHT:
                    case HTBOTTOMRIGHT:
                    case HTBOTTOM:
                    case HTBOTTOMLEFT: {
                        IsDefaultHandled = TRUE;
                    } break;
                }

                if (!IsDefaultHandled) {
                    POINT MonitorPosition  = {};

                    MonitorPosition.x = GET_X_LPARAM(LParam);
                    MonitorPosition.y = GET_Y_LPARAM(LParam);

                    POINT ClientPosition = MonitorPosition;

                    ScreenToClient(WindowHandle, &ClientPosition);

                    f32 DPI = (WinGetDPIForWindowFunc) 
                        ? (f32) WinGetDPIForWindowFunc(WindowHandle) 
                        : 96.0f;
                    i32 FrameY = (WinGetSystemMetricsForDPIFunc) 
                        ? WinGetSystemMetricsForDPIFunc(SM_CYFRAME, DPI) 
                        : GetSystemMetrics(SM_CYFRAME);
                    b32 IsOverTopResize = (
                        ClientPosition.y >= 0 && 
                        ClientPosition.y < FrameY
                    );
                    b32 IsOverTitleBar = (
                        ClientPosition.y >= 0 && 
                        ClientPosition.y < Window->CustomBorderTitleThickness
                    );
                    b32 IsOverTitleBarClientArea = FALSE;

                    for (
                        WinTitleBarClientArea* Area = Window->HeadTitleBarClientArea; 
                        Area; 
                        Area = Area->Next
                    ) {
                        r2f32 Rect = Area->Rect;

                        if (
                            Rect.X0 <= ClientPosition.x && 
                            ClientPosition.x < Rect.X1 &&
                            Rect.Y0 <= ClientPosition.y &&
                            ClientPosition.y < Rect.Y1
                        ) {
                            IsOverTitleBarClientArea = TRUE;
                            break;
                        }
                    }

                    if (IsMaximized(WindowHandle)) {
                        if (IsOverTitleBarClientArea)
                            Result = HTCLIENT;
                        else if (IsOverTitleBar)
                            Result = HTCAPTION;
                        else
                            Result = HTCLIENT;
                    } else {
                        if (IsOverTitleBarClientArea)
                            Result = HTCLIENT;
                        else if (IsOverTopResize)
                            Result = HTTOP;
                        else if (IsOverTitleBar)
                            Result = HTCAPTION;
                        else
                            Result = HTCLIENT;
                    }
                }
            }
        } break;
    }

    return Result;
}

BOOL 
WinGatherMonitorsFunc(HMONITOR MonitorHandle, HDC MonitorDescriptor, LPRECT Rect, LPARAM BundlePtr)
{
    WinMonitorGatherBundle* Bundle = (WinMonitorGatherBundle*) BundlePtr;
    Handle Monitor = {};

    *Monitor.V = INT_FROM_PTR(MonitorHandle);

    ListPush(Bundle->MemPool, Bundle->List, Monitor);

    return TRUE;
}

r2f32 
Rng(RECT Rect)
{
    r2f32 Result = {};

    Result.X0 = (f32) Rect.left;
    Result.X1 = (f32) Rect.right;
    Result.Y0 = (f32) Rect.top;
    Result.Y1 = (f32) Rect.bottom;

    return Result;
}

Str8 
StrFromEventKind(InputEventKind Kind)
{
    Str8 Result = {};

    switch (Kind) {
        case EVENT_KIND_NULL:
        case EVENT_KIND_COUNT: {} break;

        case EVENT_KIND_PRESS: {
            Result = Str8Lit("Press");
        } break;

        case EVENT_KIND_RELEASE: {
            Result = Str8Lit("Release");
        } break;

        case EVENT_KIND_MOUSE_MOVE: {
            Result = Str8Lit("MouseMove");
        } break;

        case EVENT_KIND_TEXT: {
            Result = Str8Lit("Text");
        } break;

        case EVENT_KIND_SCROLL: {
            Result = Str8Lit("Scroll");
        } break;

        case EVENT_KIND_WINDOW_LOSE_FOCUS: {
            Result = Str8Lit("WindowLoseFocus");
        } break;

        case EVENT_KIND_WINDOW_CLOSE: {
            Result = Str8Lit("WindowClose");
        } break;

        case EVENT_KIND_FILE_DROP: {
            Result = Str8Lit("FileDrop");
        } break;

        case EVENT_KIND_WAKEUP: {
            Result = Str8Lit("Wakeup");
        } break;
    }

    return Result;
}

void 
GraphicsInit(void)
{
    Arena* MemPool = ArenaAlloc();

    GRAPHICS_STATE = ArenaPushArray(MemPool, GraphicsState, 1);
    GRAPHICS_STATE->MemPool = MemPool;
    GRAPHICS_STATE->ThreadID = (u32) GetCurrentThreadId();
    GRAPHICS_STATE->Instance = GetModuleHandle(NULL);

    WinSetProcessDPIAwarenessFuncType* WinSetProcessDPIAwarenessFunc = NULL;
    HMODULE Module = LoadLibraryA("user32.dll");

    if (Module) {
        WinSetProcessDPIAwarenessFunc = (WinSetProcessDPIAwarenessFuncType*) GetProcAddress(
            Module, 
            "SetProcessDpiAwarenessContext"
        );
        WinGetDPIForWindowFunc = (WinGetDPIForWindowFuncType*) GetProcAddress(
            Module, 
            "GetDpiForWindow"
        );
        WinGetDPIForMonitorFunc = (WinGetDPIForMonitorFuncType*) GetProcAddress(
            Module, 
            "GetDpiForMonitor"
        );
        WinGetSystemMetricsForDPIFunc = (WinGetSystemMetricsForDPIFuncType*) GetProcAddress(
            Module, 
            "GetSystemMetricsForDpi"
        );
        FreeLibrary(Module);
    }

    if (WinSetProcessDPIAwarenessFunc) {
        WinSetProcessDPIAwarenessFunc(WIN_DPI_AWARENESS_PER_MONITOR_AWARE);
    } else {
        HMODULE SHCore = LoadLibraryA("shcore.dll");

        if (SHCore) {
            typedef HRESULT (WINAPI* SetProcessDPIAwarenessFuncType)(int);

            SetProcessDPIAwarenessFuncType SetProcessDPIAwarenessFunc = (SetProcessDPIAwarenessFuncType) GetProcAddress(
                SHCore, 
                "SetProcessDpiAwareness"
            );

            if (SetProcessDPIAwarenessFunc)
                SetProcessDPIAwarenessFunc(2);

            FreeLibrary(SHCore);
        }

        SetProcessDPIAware();
    }

    WNDCLASSEXW WindowClass = {};

    WindowClass.cbSize = sizeof(WindowClass);
    WindowClass.lpfnWndProc = WinWindowFunc;
    WindowClass.hInstance = GRAPHICS_STATE->Instance;
    WindowClass.lpszClassName = TEXT("GraphicalWindow");
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hIcon = LoadIcon(GRAPHICS_STATE->Instance, MAKEINTRESOURCE(1));
    WindowClass.style = CS_VREDRAW | CS_HREDRAW | CS_OWNDC;

    ATOM WindowAtom = RegisterClassExW(&WindowClass);

    UNUSED(WindowAtom);
    GRAPHICS_STATE->Properties.DoubleClickTime = GetDoubleClickTime() / 1000.0f;
    GRAPHICS_STATE->Properties.CaretBlinkTime = GetCaretBlinkTime() / 1000.0f;

    DEVMODEW DevModeW = {};

    DevModeW.dmSize = sizeof(DevModeW);

    if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &DevModeW)) {
        if (
            DevModeW.dmFields & DM_DISPLAYFREQUENCY &&
            DevModeW.dmDisplayFrequency > 0 &&
            DevModeW.dmDisplayFrequency < 360
        ) {
            GRAPHICS_STATE->Properties.DefaultRefreshRate = (f32) DevModeW.dmDisplayFrequency;
        } else {
            GRAPHICS_STATE->Properties.DefaultRefreshRate = (f32) 60.0f;
        }
    } else {
        GRAPHICS_STATE->Properties.DefaultRefreshRate = (f32) 60.0f;
    }

    LogInfo(
        "[WIN32] :: DefaultRefreshRate = %.6f", 
        GRAPHICS_STATE->Properties.DefaultRefreshRate
    );

    SetCursorKind(CURSOR_KIND_POINTER);

    {
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'A'] = INPUT_KIND_A;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'B'] = INPUT_KIND_B;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'C'] = INPUT_KIND_C;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'D'] = INPUT_KIND_D;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'E'] = INPUT_KIND_E;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'F'] = INPUT_KIND_F;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'G'] = INPUT_KIND_G;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'H'] = INPUT_KIND_H;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'I'] = INPUT_KIND_I;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'J'] = INPUT_KIND_J;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'K'] = INPUT_KIND_K;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'L'] = INPUT_KIND_L;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'M'] = INPUT_KIND_M;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'N'] = INPUT_KIND_N;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'O'] = INPUT_KIND_O;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'P'] = INPUT_KIND_P;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'Q'] = INPUT_KIND_Q;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'R'] = INPUT_KIND_R;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'S'] = INPUT_KIND_S;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'T'] = INPUT_KIND_T;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'U'] = INPUT_KIND_U;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'V'] = INPUT_KIND_V;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'W'] = INPUT_KIND_W;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'X'] = INPUT_KIND_X;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'Y'] = INPUT_KIND_Y;
        GRAPHICS_STATE->InputKindFromCache[(unsigned int) 'Z'] = INPUT_KIND_Z;
        
        for(u64 I = '0', J = INPUT_KIND_0; I <= '9'; ++I, ++J)
            GRAPHICS_STATE->InputKindFromCache[I] = (InputKind) J;

        for(u64 I = VK_NUMPAD0, J = INPUT_KIND_0; I <= VK_NUMPAD9; ++I, ++J)
            GRAPHICS_STATE->InputKindFromCache[I] = (InputKind) J;

        for(u64 I = VK_F1, J = INPUT_KIND_FUNC_1; I <= VK_F24; ++I, ++J)
            GRAPHICS_STATE->InputKindFromCache[I] = (InputKind) J;
        
        GRAPHICS_STATE->InputKindFromCache[VK_SPACE]      = INPUT_KIND_SPACE;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_3]      = INPUT_KIND_TILDE;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_MINUS]  = INPUT_KIND_MINUS;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_PLUS]   = INPUT_KIND_EQUAL;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_4]      = INPUT_KIND_LEFTBRACKET;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_6]      = INPUT_KIND_RIGHTBRACKET;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_1]      = INPUT_KIND_SEMICOLON;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_7]      = INPUT_KIND_QUOTE;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_COMMA]  = INPUT_KIND_COMMA;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_PERIOD] = INPUT_KIND_PERIOD;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_2]      = INPUT_KIND_SLASH;
        GRAPHICS_STATE->InputKindFromCache[VK_OEM_5]      = INPUT_KIND_BACKSLASH;
        GRAPHICS_STATE->InputKindFromCache[VK_TAB]        = INPUT_KIND_TAB;
        GRAPHICS_STATE->InputKindFromCache[VK_PAUSE]      = INPUT_KIND_PAUSE;
        GRAPHICS_STATE->InputKindFromCache[VK_ESCAPE]     = INPUT_KIND_ESC;
        GRAPHICS_STATE->InputKindFromCache[VK_UP]         = INPUT_KIND_UP;
        GRAPHICS_STATE->InputKindFromCache[VK_LEFT]       = INPUT_KIND_LEFT;
        GRAPHICS_STATE->InputKindFromCache[VK_DOWN]       = INPUT_KIND_DOWN;
        GRAPHICS_STATE->InputKindFromCache[VK_RIGHT]      = INPUT_KIND_RIGHT;
        GRAPHICS_STATE->InputKindFromCache[VK_BACK]       = INPUT_KIND_BACKSPACE;
        GRAPHICS_STATE->InputKindFromCache[VK_RETURN]     = INPUT_KIND_RETURN;
        GRAPHICS_STATE->InputKindFromCache[VK_DELETE]     = INPUT_KIND_DELETE;
        GRAPHICS_STATE->InputKindFromCache[VK_INSERT]     = INPUT_KIND_INSERT;
        GRAPHICS_STATE->InputKindFromCache[VK_PRIOR]      = INPUT_KIND_PAGEUP;
        GRAPHICS_STATE->InputKindFromCache[VK_NEXT]       = INPUT_KIND_PAGEDOWN;
        GRAPHICS_STATE->InputKindFromCache[VK_HOME]       = INPUT_KIND_HOME;
        GRAPHICS_STATE->InputKindFromCache[VK_END]        = INPUT_KIND_END;
        GRAPHICS_STATE->InputKindFromCache[VK_CAPITAL]    = INPUT_KIND_CAPSLOCK;
        GRAPHICS_STATE->InputKindFromCache[VK_NUMLOCK]    = INPUT_KIND_NUMLOCK;
        GRAPHICS_STATE->InputKindFromCache[VK_SCROLL]     = INPUT_KIND_SCROLLLOCK;
        GRAPHICS_STATE->InputKindFromCache[VK_APPS]       = INPUT_KIND_SUPER;
        GRAPHICS_STATE->InputKindFromCache[VK_CONTROL]    = INPUT_KIND_CTRL;
        GRAPHICS_STATE->InputKindFromCache[VK_LCONTROL]   = INPUT_KIND_CTRL;
        GRAPHICS_STATE->InputKindFromCache[VK_RCONTROL]   = INPUT_KIND_CTRL;
        GRAPHICS_STATE->InputKindFromCache[VK_SHIFT]      = INPUT_KIND_SHIFT;
        GRAPHICS_STATE->InputKindFromCache[VK_LSHIFT]     = INPUT_KIND_SHIFT;
        GRAPHICS_STATE->InputKindFromCache[VK_RSHIFT]     = INPUT_KIND_SHIFT;
        GRAPHICS_STATE->InputKindFromCache[VK_MENU]       = INPUT_KIND_ALT;
        GRAPHICS_STATE->InputKindFromCache[VK_LMENU]      = INPUT_KIND_ALT;
        GRAPHICS_STATE->InputKindFromCache[VK_RMENU]      = INPUT_KIND_ALT;
        GRAPHICS_STATE->InputKindFromCache[VK_DIVIDE]     = INPUT_KIND_NUM_SLASH;
        GRAPHICS_STATE->InputKindFromCache[VK_MULTIPLY]   = INPUT_KIND_NUM_STAR;
        GRAPHICS_STATE->InputKindFromCache[VK_SUBTRACT]   = INPUT_KIND_NUM_MINUS;
        GRAPHICS_STATE->InputKindFromCache[VK_ADD]        = INPUT_KIND_NUM_PLUS;
        GRAPHICS_STATE->InputKindFromCache[VK_DECIMAL]    = INPUT_KIND_NUM_PERIOD;
        
        for(u32 I = 0; I < 10; ++I)
            GRAPHICS_STATE->InputKindFromCache[VK_NUMPAD0 + I] = (InputKind) ((u64) INPUT_KIND_NUM_0 + I);
        
        for(u64 I = 0xDF, J = 0; I < 0xFF; ++I, ++J)
            GRAPHICS_STATE->InputKindFromCache[I] = (InputKind) ((u64) INPUT_KIND_EX0 + J);
    }

    // TODO(nc): parse icon
}

GraphicsProperties* 
GetGraphicsProperties(void)
{
    return &GRAPHICS_STATE->Properties;
}

void 
SetClipboardText(Str8 String)
{
    if (OpenClipboard(NULL)) {
        EmptyClipboard();

        HANDLE StringCopyHandle = GlobalAlloc(GMEM_MOVEABLE, String.Size + 1);

        if (StringCopyHandle) {
            u8* CopyBuffer = (u8*) GlobalLock(StringCopyHandle);

            MemCpy(CopyBuffer, String.Str, String.Size);
            CopyBuffer[String.Size] = 0;
            GlobalUnlock(StringCopyHandle);
            SetClipboardData(CF_TEXT, StringCopyHandle);
        }

        CloseClipboard();
    }
}

Str8 
GetClipboardText(Arena* MemPool)
{
    Str8 Result = {};

    if (IsClipboardFormatAvailable(CF_TEXT) && OpenClipboard(NULL)) {
        HANDLE DataHandle = GetClipboardData(CF_TEXT);

        if (DataHandle) {
            u8* Buffer = (u8*) GlobalLock(DataHandle);

            if (Buffer) {
                u64 Size = StrLen(Buffer);

                Result = ArenaPushStrCpy(MemPool, Str(Buffer, Size));
                GlobalUnlock(DataHandle);
            }
        }

        CloseClipboard();
    }

    return Result;
}

Handle 
WindowOpen(r2f32 Rect, WindowPropertyKind Properties, Str8 Title)
{
    b32 CustomBorder = !!(Properties & WINDOW_KIND_CUSTOM_BORDER);
    b32 UseDefaultPosition = !!(Properties & WINDOW_KIND_USE_DEFAULT_POSITION);
    v2f32 Position = Rect.Point0;
    v2f32 Dimensions = Length(Rect);
    HWND WindowHandle = NULL;

    {
        TempArena Scratch = GetScratch(NULL, 0);
        Str16 WideTitle = Str16FromStr8(Scratch.MemPool, Title);

        NEW_WINDOW_CUSTOM_BORDER = CustomBorder;
        WindowHandle = CreateWindowExW(
            WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP,
            TEXT("GraphicalWindow"),
            (LPCWSTR) WideTitle.Str,
            WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
            (UseDefaultPosition) ? CW_USEDEFAULT : (i32) Position.X,
            (UseDefaultPosition) ? CW_USEDEFAULT : (i32) Position.Y,
            (i32) Dimensions.X,
            (i32) Dimensions.Y,
            NULL,
            NULL,
            GRAPHICS_STATE->Instance,
            NULL
        );
        DragAcceptFiles(WindowHandle, TRUE);
        NEW_WINDOW_CUSTOM_BORDER = FALSE;
        ReleaseScratch(Scratch);
    }

    WinWindow* Window = WindowAlloc();

    {
        Window->WindowHandle = WindowHandle;
        Window->WindowDescriptor = GetDC(WindowHandle);

        if (WinGetDPIForWindowFunc)
            Window->DPI = WinGetDPIForWindowFunc(WindowHandle);
        else
            Window->DPI = 96.0f;
    }

    {
        BOOL Enabled = FALSE;

        DwmIsCompositionEnabled(&Enabled);
        Window->CustomBorderCompositionEnabled = Enabled;
    }

    if (Properties & WINDOW_KIND_CUSTOM_BORDER) {
        Window->CustomBorder = TRUE;
        Window->PaintMemPool = ArenaAlloc();
    }

    Handle Result = HandleFromWindow(Window);

    return Result;
}

void 
WindowClose(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    WindowRelease(Window);
}

void 
WindowSetTitle(Handle WindowHandle, Str8 Title)
{
    TempArena Scratch = GetScratch(NULL, 0);
    WinWindow* Window = WindowFromHandle(WindowHandle);
    Str16 WideTitle = Str16FromStr8(Scratch.MemPool, Title);

    SetWindowTextW(Window->WindowHandle, (WCHAR*) WideTitle.Str);
    ReleaseScratch(Scratch);
}

void 
WindowFirstPaint(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    Window->FirstPaintCompleted = TRUE;
    ShowWindow(Window->WindowHandle, SW_SHOW);

    if (Window->Maximised)
        ShowWindow(Window->WindowHandle, SW_MAXIMIZE);
}

void 
WindowFocus(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    SetForegroundWindow(Window->WindowHandle);
    SetFocus(Window->WindowHandle);
}

b32 
WindowIsFocused(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);
    HWND ActiveHandle = GetActiveWindow();

    if (!ActiveHandle)
        return FALSE;

    return (ActiveHandle == Window->WindowHandle);
}

void
WindowShowCursor(Handle WindowHandle, b32 Show)
{
    UNUSED(WindowHandle);
    ShowCursor(Show);
}

void
WindowCaptureCursor(Handle WindowHandle, b32 Capture)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);
    HWND WindowHWND = HWNDFromWindow(Window);

    if (Capture)
        SetCapture(WindowHWND);
    else
        ReleaseCapture();
}

void
WindowSetCursorPosition(Handle WindowHandle, v2f32 Position)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);
    HWND WinHandle = HWNDFromWindow(Window);

    SetCursorPos((i32) Position.X, (i32) Position.Y);
}

v2f32
WindowClientPositionToScreen(Handle WindowHandle, v2f32 ClientPosition)
{
    HWND WinHandle = HWNDFromWindow(WindowFromHandle(WindowHandle));
    POINT Point = {
        (LONG) ClientPosition.X,
        (LONG) ClientPosition.Y
    };

    ClientToScreen(WinHandle, &Point);

    v2f32 Result = {
        (f32) Point.x,
        (f32) Point.y
    };

    return Result;
}

v2f32
WindowScreenPositionToClient(Handle WindowHandle, v2f32 ScreenPosition)
{
    HWND WinHandle = HWNDFromWindow(WindowFromHandle(WindowHandle));
    POINT Point = {
        (LONG) ScreenPosition.X,
        (LONG) ScreenPosition.Y
    };

    ScreenToClient(WinHandle, &Point);

    v2f32 Result = {
        (f32) Point.x,
        (f32) Point.y
    };

    return Result;
}

b32 
WindowIsFullScreen(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (!Window || !Window->WindowHandle)
        return FALSE;

    DWORD WindowStyle = GetWindowLong(Window->WindowHandle, GWL_STYLE);

    return !(WindowStyle & WS_OVERLAPPEDWINDOW);
}

void 
WindowSetFullScreen(Handle WindowHandle, b32 FullScreen)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (!Window->WindowHandle)
        return;

    DWORD WindowStyle = GetWindowLong(Window->WindowHandle, GWL_STYLE);
    b32 IsAlreadyFullScreen = WindowIsFullScreen(WindowHandle);

    if (FullScreen) {
        if (!IsAlreadyFullScreen)
            GetWindowPlacement(Window->WindowHandle, &Window->LastWindowPlacement);

        MONITORINFO MonitorInfo = {};

        MonitorInfo.cbSize = sizeof(MonitorInfo);

        if (GetMonitorInfo(MonitorFromWindow(Window->WindowHandle, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
            SetWindowLong(Window->WindowHandle, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(
                Window->WindowHandle,
                HWND_TOP,
                MonitorInfo.rcMonitor.left,
                MonitorInfo.rcMonitor.top,
                MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED
            );
        }
    } else {
        SetWindowLong(Window->WindowHandle, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window->WindowHandle, &Window->LastWindowPlacement);
        SetWindowPos(
            Window->WindowHandle,
            0,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED
        );
    }
}

b32 
WindowIsMaximised(Handle WindowHandle)
{
    b32 Result = FALSE;
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window)
        Result = !!(IsZoomed(Window->WindowHandle));

    return Result;
}

void 
WindowSetMaximised(Handle WindowHandle, b32 Maximised)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window) {
        if (Window->FirstPaintCompleted) {
            if (Maximised)
                ShowWindow(Window->WindowHandle, SW_MAXIMIZE);
            else
                ShowWindow(Window->WindowHandle, SW_RESTORE);
        } else {
            Window->Maximised = Maximised;
        }
    }
}

b32 
WindowIsMinimised(Handle WindowHandle)
{
    b32 Result = FALSE;
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window)
        Result = !!(IsIconic(Window->WindowHandle));

    return Result;
}

void 
WindowSetMinimised(Handle WindowHandle, b32 Minimised)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window && Minimised != WindowIsMinimised(WindowHandle)) {
        if (Minimised)
            ShowWindow(Window->WindowHandle, SW_MINIMIZE);
        else
            ShowWindow(Window->WindowHandle, SW_RESTORE);
    }
}

void 
WindowBringToFront(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window)
        BringWindowToTop(Window->WindowHandle);
}

void 
WindowSetMonitor(Handle WindowHandle, Handle MonitorHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);
    HMONITOR Monitor = (HMONITOR) *MonitorHandle.V;

    {
        MONITORINFOEXW Info = {};

        Info.cbSize = sizeof(Info);

        if (GetMonitorInfoW(Monitor, (MONITORINFO*) &Info)) {
            r2f32 ExistingRect = GetWindowRect(WindowHandle);
            v2f32 WindowSize = Length(ExistingRect);

            SetWindowPos(
                Window->WindowHandle,
                HWND_TOP,
                (Info.rcWork.left + Info.rcWork.right) / 2 - (i32) WindowSize.X / 2,
                (Info.rcWork.top + Info.rcWork.bottom) / 2 - (i32) WindowSize.Y / 2,
                (i32) WindowSize.X,
                (i32) WindowSize.Y,
                0
            );
        }
    }
}

void 
WindowClearCustomBorderData(Handle WindowHandle)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window && Window->CustomBorder) {
        ArenaClear(Window->PaintMemPool);
        Window->HeadTitleBarClientArea = NULL;
        Window->TailTitleBarClientArea = NULL;
        Window->CustomBorderTitleThickness = 0.0f;
        Window->CustomBorderEdgeThickness = 0.0f;
    }
}

void 
WindowPushCustomTitleBar(Handle WindowHandle, f32 Thickness)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window)
        Window->CustomBorderTitleThickness = Thickness;
}

void 
WindowPushCustomEdges(Handle WindowHandle, f32 Thickness)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window)
        Window->CustomBorderEdgeThickness = Thickness;
}

void 
WindowPushCustomTitleBarClientArea(Handle WindowHandle, r2f32 Rect)
{
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window && Window->CustomBorder) {
        WinTitleBarClientArea* Area = ArenaPushArray(Window->PaintMemPool, WinTitleBarClientArea, 1);

        if (Area) {
            Area->Rect = Rect;
            SLL_QUEUE_PUSH(Window->HeadTitleBarClientArea, Window->TailTitleBarClientArea, Area);
        }
    }
}

r2f32 
GetWindowRect(Handle WindowHandle)
{
    r2f32 Result = {};
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window) {
        RECT Rect = {};

        GetWindowRect(HWNDFromWindow(Window), &Rect);
        Result = Rng(Rect);
    }

    return Result;
}

r2f32 
GetWindowClientRect(Handle WindowHandle)
{
    r2f32 Result = {};
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window) {
        RECT Rect = {};

        GetClientRect(HWNDFromWindow(Window), &Rect);
        Result = Rng(Rect);
    }

    return Result;
}

f32 
GetWindowDPI(Handle WindowHandle)
{
    f32 Result = 96.0f;
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window)
        Result = Window->DPI;

    return Result;
}

HandleArray 
ArenaPushMonitorArray(Arena* MemPool)
{
    TempArena Scratch = GetScratch(&MemPool, 1);
    HandleList List = {};

    {
        WinMonitorGatherBundle Bundle = {};

        Bundle.MemPool = MemPool;
        Bundle.List = &List;
        EnumDisplayMonitors(NULL, NULL, WinGatherMonitorsFunc, (LPARAM) &Bundle);
    }

    HandleArray Result = HandleArrayFromList(MemPool, &List);

    ReleaseScratch(Scratch);

    return Result;
}

Handle 
GetPrimaryMonitor(void)
{
    POINT ZeroPoint = {};

    ZeroPoint.x = 0;
    ZeroPoint.y = 0;

    HMONITOR Monitor = MonitorFromPoint(ZeroPoint, MONITOR_DEFAULTTOPRIMARY);
    Handle Result = {};

    *Result.V = INT_FROM_PTR(Monitor);

    return Result;
}

Handle 
MonitorFromWindow(Handle WindowHandle)
{
    Handle Result = {};
    WinWindow* Window = WindowFromHandle(WindowHandle);

    if (Window && Window->WindowHandle) {
        HMONITOR Monitor = MonitorFromWindow(Window->WindowHandle, MONITOR_DEFAULTTOPRIMARY);

        *Result.V = INT_FROM_PTR(Monitor);
    }

    return Result;
}

Str8 
NameFromMonitor(Arena* MemPool, Handle MonitorHandle)
{
    Str8 Result = {};
    HMONITOR Monitor = (HMONITOR) *MonitorHandle.V;
    MONITORINFOEXW Info = {};

    Info.cbSize = sizeof(Info);

    if (GetMonitorInfoW(Monitor, (MONITORINFO*) &Info)) {
        Str16 WideResult = CStr16((u16*) Info.szDevice);

        Result = Str8FromStr16(MemPool, WideResult);
    }

    return Result;
}

v2f32 
GetMonitorDimensions(Handle MonitorHandle)
{
    v2f32 Result = {};
    HMONITOR Monitor = (HMONITOR) *MonitorHandle.V;
    MONITORINFO Info = {};

    Info.cbSize = sizeof(Info);

    if (GetMonitorInfoW(Monitor, &Info)) {
        Result.X = Info.rcWork.right - Info.rcWork.left;
        Result.Y = Info.rcWork.bottom - Info.rcWork.top;
    }

    return Result;
}

f32 
GetMonitorDPI(Handle MonitorHandle)
{
    f32 Result = 96.0f;
    HMONITOR Monitor = (HMONITOR) *MonitorHandle.V;

    if (WinGetDPIForMonitorFunc) {
        UINT DPIX = 0;
        UINT DPIY = 0;
        HRESULT Stat = WinGetDPIForMonitorFunc(Monitor, MDT_EFFECTIVE_DPI, &DPIX, &DPIY);

        Result = (f32) DPIX;
    }

    return Result;
}

void 
SendWakeupEvent(void)
{
    PostThreadMessageA(GRAPHICS_STATE->ThreadID, 0x401, 0, 0);                  // TODO: Should be PostThreadMessageW for a UNICODE program?
}

InputEventList 
GetEvents(Arena* MemPool, b32 Wait)
{
    GRAPHICS_MEMPOOL = MemPool;

    MemSet(&EVENT_LIST, 0, sizeof(EVENT_LIST));

    MSG Message = {};

    if (!Wait || GetMessage(&Message, NULL, 0, 0)) {
        b32 FirstWait = Wait;

        for (
            ; 
            FirstWait || PeekMessageA(&Message, NULL, 0, 0, PM_REMOVE);
            FirstWait = FALSE
        ) {
            TranslateMessage(&Message);
            DispatchMessage(&Message);

            if (Message.message == WM_QUIT)
                PushEvent(EVENT_KIND_WINDOW_CLOSE, NULL);
        }
    }

    return EVENT_LIST;
}

InputModifierKind 
GetModifiers(void)
{
    InputModifierKind Result = (InputModifierKind) 0;

    if (GetKeyState(VK_CONTROL) & 0x8000)
        Result |= INPUT_MOD_KIND_CTRL;

    if (GetKeyState(VK_SHIFT) & 0x8000)
        Result |= INPUT_MOD_KIND_SHIFT;

    if (GetKeyState(VK_MENU) & 0x8000)
        Result |= INPUT_MOD_KIND_ALT;

    return Result;
}

b32 
KeyIsDown(InputKind Input)
{
    b32 Result = FALSE;
    WPARAM VKey = VKeyFromInputKind(Input);

    if (GetKeyState(VKey) &0x8000)
        Result = TRUE;

    return Result;
}

v2f32 
GetMousePosition(Handle WindowHandle)
{
    v2f32 Result = {};
    POINT Point;

    if (GetCursorPos(&Point)) {
        WinWindow* Window = WindowFromHandle(WindowHandle);

        ScreenToClient(Window->WindowHandle, &Point);
        Result.X = (f32) Point.x;
        Result.Y = (f32) Point.y;
    }

    return Result;
}

#define WIN_CURSOR_LIST(X)                \
    X(POINTER,              IDC_ARROW)    \
    X(I_BAR,                IDC_IBEAM)    \
    X(LEFT_RIGHT,           IDC_SIZEWE)   \
    X(UP_DOWN,              IDC_SIZENS)   \
    X(DOWN_RIGHT,           IDC_SIZENWSE) \
    X(UP_RIGHT,             IDC_SIZENESW) \
    X(UP_DOWN_LEFT_RIGHT,   IDC_SIZEALL)  \
    X(HAND_POINT,           IDC_HAND)     \
    X(DISABLED,             IDC_NO)

#define CURSOR_CASE(X, Y) case CURSOR_KIND_##X: {   \
    local HCURSOR _Cursor = NULL;                   \
                                                    \
    if (!_Cursor)                                   \
        _Cursor = LoadCursor(NULL, Y);              \
                                                    \
    CursorHandle = _Cursor;                         \
} break;

void 
SetCursorKind(CursorKind Cursor)
{
    b32 ValidCursor = TRUE;
    HCURSOR CursorHandle = NULL;

    switch (Cursor) {
        WIN_CURSOR_LIST(CURSOR_CASE)
    }

#undef CURSOR_CASE
#undef WIN_CURSOR_LIST

    if (ValidCursor && !WINDOW_RESIZING) {
        if (CursorHandle != GRAPHICS_STATE->Cursor) {
            PostMessage(NULL, WM_SETCURSOR, 0, 0);

            POINT Point = {};

            GetCursorPos(&Point);
            SetCursorPos(Point.x, Point.y);
        }

        GRAPHICS_STATE->Cursor = CursorHandle;
    }
}

void 
GraphicalMessage(b32 IsError, Str8 Title, Str8 Message)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str16 WideTitle = Str16FromStr8(Scratch.MemPool, Title);
    Str16 WideMessage = Str16FromStr8(Scratch.MemPool, Message);

    MessageBoxW(0, (WCHAR*) WideMessage.Str, (WCHAR*) WideTitle.Str, MB_OK | (!!IsError * MB_ICONERROR));
    ReleaseScratch(Scratch);
}

Str8 
GraphicalFilePicker(Arena* MemPool, Str8 InitialPath)
{
    Str8 Result = {};
    TempArena Scratch = GetScratch(&MemPool, 1);
    u64 BufferSize = 4096;
    u16* Buffer = ArenaPushArrayZero(Scratch.MemPool, u16, BufferSize);
    OPENFILENAMEW Params = {};

    Params.lStructSize = sizeof(OPENFILENAMEW);
    Params.lpstrFile = (WCHAR*) Buffer;
    Params.nMaxFile = BufferSize;
    Params.lpstrFilter = TEXT("All Files (*.*)\0*.*\0\0");
    Params.nFilterIndex = 1;
    Params.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    Params.hwndOwner = NULL;

    if (InitialPath.Str && InitialPath.Size) {
        Params.lpstrInitialDir = (WCHAR*) Str16FromStr8(
            Scratch.MemPool, 
            InitialPath
        ).Str;
    }

    if (GetOpenFileNameW(&Params)) {
        Result = Str8FromStr16(MemPool, Str((u16*) Buffer));
    } else {
        DWORD Error = CommDlgExtendedError();

        if (Error != 0) {
            LogError("GetOpenFileNameW call failed. (0x%X)", Error);
        }
    }

    ReleaseScratch(Scratch);

    return Result;
}

void 
ShowInFileExplorer(Str8 Path)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str8 PathCopy = ArenaPushStrCpy(Scratch.MemPool, Path);

    for (u64 Index = 0; Index < PathCopy.Size; ++Index)
        if (PathCopy.Str[Index] == '/')
            PathCopy.Str[Index] = '\\';

    Str16 WidePath = Str16FromStr8(Scratch.MemPool, PathCopy);
    SFGAOF Flags = 0;
    PIDLIST_ABSOLUTE List = NULL;

    if (WidePath.Size && SUCCEEDED(SHParseDisplayName((WCHAR*) WidePath.Str, NULL, &List, 0, &Flags))) {
        HRESULT Result = SHOpenFolderAndSelectItems(List, 0, NULL, 0);

        CoTaskMemFree((LPVOID) List);
        UNUSED(Result);
    }

    ReleaseScratch(Scratch);
}

void 
OpenInBrowser(Str8 URL)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Str16 WideURL = Str16FromStr8(Scratch.MemPool, URL);

    ShellExecuteW(NULL, TEXT("open"), (WCHAR*) WideURL.Str, NULL, NULL, SW_SHOWNORMAL);
    ReleaseScratch(Scratch);
}
