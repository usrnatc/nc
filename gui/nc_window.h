#if !defined(__NC_WINDOW_H__)
#define __NC_WINDOW_H__

#include "nc_types.h"
#include "nc_system.h"
#include "nc_keyboard.h"

// @defines____________________________________________________________________
struct GraphicsState;

typedef u32 InputModifierKind;
enum : u32 {
    INPUT_MOD_KIND_CTRL  = (1 << 0),
    INPUT_MOD_KIND_SHIFT = (1 << 1),
    INPUT_MOD_KIND_ALT   = (1 << 2)
};

typedef u32 WindowPropertyKind;
enum : u32 {
    WINDOW_KIND_CUSTOM_BORDER        = (1 << 0),
    WINDOW_KIND_USE_DEFAULT_POSITION = (1 << 1)
};

enum CursorKind {
    CURSOR_KIND_POINTER,
    CURSOR_KIND_I_BAR,
    CURSOR_KIND_LEFT_RIGHT,
    CURSOR_KIND_UP_DOWN,
    CURSOR_KIND_DOWN_RIGHT,
    CURSOR_KIND_UP_RIGHT,
    CURSOR_KIND_UP_DOWN_LEFT_RIGHT,
    CURSOR_KIND_HAND_POINT,
    CURSOR_KIND_DISABLED,
    CURSOR_KIND_COUNT
};

enum InputEventKind {
    EVENT_KIND_NULL,
    EVENT_KIND_PRESS,
    EVENT_KIND_RELEASE,
    EVENT_KIND_MOUSE_MOVE,
    EVENT_KIND_TEXT,
    EVENT_KIND_SCROLL,
    EVENT_KIND_WINDOW_LOSE_FOCUS,
    EVENT_KIND_WINDOW_CLOSE,
    EVENT_KIND_FILE_DROP,
    EVENT_KIND_WAKEUP,
    EVENT_KIND_COUNT
};

EXTERN_C_LINK_BEGIN
extern Str8 KEYBOARD_CONFIG_STR_TABLE[143];
EXTERN_C_LINK_END

// @types______________________________________________________________________
struct GraphicsProperties {
    f32 DoubleClickTime;
    f32 CaretBlinkTime;
    f32 DefaultRefreshRate;
};

struct InputEvent {
    InputEvent*       Next;
    InputEvent*       Prev;
    u64               TimeStampUSecs;
    Handle            WindowHandle;
    InputEventKind    Kind;
    InputModifierKind Modifier;
    InputKind         Input;
    b32               IsRepeat;
    b32               RightSided;
    u32               Character;
    u32               RepeatCount;
    v2f32             Position;
    v2f32             PositionDelta;
    Str8List          Strings;
};

struct InputEventList {
    u64         Count;
    InputEvent* Head;
    InputEvent* Tail;
};

// @runtime____________________________________________________________________
extern GraphicsState* GRAPHICS_STATE;

#if defined(NC_OS_WIN)
    extern InputEventList EVENT_LIST;
    extern Arena* GRAPHICS_MEMPOOL;
    extern b32 WINDOW_RESIZING;
    extern b32 NEW_WINDOW_CUSTOM_BORDER;
#endif

// @functions__________________________________________________________________
EXTERN_C_LINK_BEGIN
u32 CodePointFromInput(InputModifierKind Mods, InputKind Input);
EXTERN_C_LINK_END

INTERNAL b32 Update(void);

Str8 StrFromEventKind(InputEventKind Kind);
Str8 StrFromInputModifierInput(Arena* MemPool, InputModifierKind Modifiers, InputKind Input);
Str8List StrListFromInputModKind(Arena* MemPool, InputModifierKind Modifiers);
INTERNAL b32 Frame(void);
void ConsumeEvent(InputEventList* List, InputEvent* Event);
b32 InputPress(InputEventList* List, Handle WindowHandle, InputModifierKind Modifier, InputKind Input);
b32 InputRelease(InputEventList* List, Handle WindowHandle, InputModifierKind Modifier, InputKind Input);
b32 Text(InputEventList* List, Handle WindowHandle, u32 Character);
InputEventList EventListCopy(Arena* MemPool, InputEventList* List);
void EventListCat(InputEventList* Dst, InputEventList* Src);
InputEvent* ListPush(Arena* MemPool, InputEventList* List, InputEventKind Event);
void GraphicsInit(void);
GraphicsProperties* GetGraphicsProperties(void);
void SetClipboardText(Str8 String);
Str8 GetClipboardText(Arena* MemPool);
Handle WindowOpen(r2f32 Rect, WindowPropertyKind Properties, Str8 Title);
void WindowClose(Handle WindowHandle);
void WindowSetTitle(Handle WindowHandle, Str8 Title);
void WindowFirstPaint(Handle WindowHandle);
void WindowFocus(Handle WindowHandle);
b32 WindowIsFocused(Handle WindowHandle);
void WindowShowCursor(Handle WindowHandle, b32 Show);
void WindowCaptureCursor(Handle WindowHandle, b32 Capture);
void WindowSetCursorPosition(Handle WindowHandle, v2f32 Position);
v2f32 WindowClientPositionToScreen(Handle WindowHandle, v2f32 ClientPosition);
v2f32 WindowScreenPositionToClient(Handle WindowHandle, v2f32 ClientPosition);
b32 WindowIsFullScreen(Handle WindowHandle);
void WindowSetFullScreen(Handle WindowHandle, b32 FullScreen);
b32 WindowIsMaximised(Handle WindowHandle);
void WindowSetMaximised(Handle WindowHandle, b32 Maximised);
b32 WindowIsMinimised(Handle WindowHandle);
void WindowSetMinimised(Handle WindowHandle, b32 Minimised);
void WindowBringToFront(Handle WindowHandle);
void WindowSetMonitor(Handle WindowHandle, Handle MonitorHandle);
void WindowClearCustomBorderData(Handle WindowHandle);
void WindowPushCustomTitleBar(Handle WindowHandle, f32 Thickness);
void WindowPushCustomEdges(Handle WindowHandle, f32 Thickness);
void WindowPushCustomTitleBarClientArea(Handle WindowHandle, r2f32 Rect);
r2f32 GetWindowRect(Handle WindowHandle);
r2f32 GetWindowClientRect(Handle WindowHandle);
f32 GetWindowDPI(Handle WindowHandle);
HandleArray ArenaPushMonitorArray(Arena* MemPool);
Handle GetPrimaryMonitor(void);
Handle MonitorFromWindow(Handle WindowHandle);
Str8 NameFromMonitor(Arena* MemPool, Handle MonitorHandle);
v2f32 GetMonitorDimensions(Handle MonitorHandle);
f32 GetMonitorDPI(Handle MonitorHandle);
void SendWakeupEvent(void);
InputEventList GetEvents(Arena* MemPool, b32 Wait);
InputModifierKind GetModifiers(void);
b32 KeyIsDown(InputKind Input);
v2f32 GetMousePosition(Handle WindowHandle);
void SetCursorKind(CursorKind Cursor);
void GraphicalMessage(b32 IsError, Str8 Title, Str8 Message);
Str8 GraphicalFilePicker(Arena* MemPool, Str8 InitialPath);
void ShowInFileExplorer(Str8 Path);
void OpenInBrowser(Str8 URL);

#endif // __NC_WINDOW_H__
