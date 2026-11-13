#if !defined(__NC_APPLICATION_H__)
#define __NC_APPLICATION_H__

#include "nc_types.h"
#include "nc_string.h"
#include "nc_system.h"
#include "nc_ui.h"
#include "nc_content.h"
#include "nc_metadesk.h"
#include "nc_config.h"
#include "nc_eval.h"

// @defines____________________________________________________________________
typedef u64 AppEvalSpaceKind;
enum : u64 {
    APP_EVAL_SPACE_KIND_META_QUERY = EVAL_SPACE_KIND_FIRST_USER_DEFINED,
    APP_EVAL_SPACE_KIND_META_CFG,
    APP_EVAL_SPACE_KIND_META_CMD,
    APP_EVAL_SPACE_KIND_META_THEME,
    APP_EVAL_SPACE_KIND_META_VIEW
};

#define APP_VIEW_UI_FUNC_SIG(X)  void X(EvalEval Eval, r2f32 Rect)
#define APP_VIEW_UI_FUNC_NAME(X) __AppViewUI##X
#define APP_VIEW_UI_FUNC_DEF(X)  APP_VIEW_UI_FUNC_SIG(APP_VIEW_UI_FUNC_NAME(X))
typedef APP_VIEW_UI_FUNC_SIG(AppViewUIFuncType);

enum AppDragDropState {
    APP_DRAG_DROP_STATE_NULL,
    APP_DRAG_DROP_STATE_DRAGGING,
    APP_DRAG_DROP_STATE_DROPPING,
    APP_DRAG_DROP_STATE_COUNT
};

enum AppRegistersSlot {
    APP_REGISTERS_SLOT_NULL,
    APP_REGISTERS_SLOT_WINDOW,
    APP_REGISTERS_SLOT_PANEL,
    APP_REGISTERS_SLOT_TAB,
    APP_REGISTERS_SLOT_VIEW,
    APP_REGISTERS_SLOT_PREV_TAB,
    APP_REGISTERS_SLOT_DST_PANEL,
    APP_REGISTERS_SLOT_CONFIG,
    APP_REGISTERS_SLOT_CONFIG_LIST,
    APP_REGISTERS_SLOT_EVAL_SPACE,
    APP_REGISTERS_SLOT_FILE_PATH,
    APP_REGISTERS_SLOT_CURSOR,
    APP_REGISTERS_SLOT_MARK,
    APP_REGISTERS_SLOT_TEXT_KEY,
    APP_REGISTERS_SLOT_EXPRESSION,
    APP_REGISTERS_SLOT_UI_KEY,
    APP_REGISTERS_SLOT_SRC_UI_KEY,
    APP_REGISTERS_SLOT_OFFSET_PX,
    APP_REGISTERS_SLOT_REG_SLOT,
    APP_REGISTERS_SLOT_CONTEXT_EXPRESSION,
    APP_REGISTERS_SLOT_DO_IMPLICIT_ROOT,
    APP_REGISTERS_SLOT_DO_LISTER,
    APP_REGISTERS_SLOT_DO_SMALL,
    APP_REGISTERS_SLOT_DO_BIG_ROWS,
    APP_REGISTERS_SLOT_ALL_WINDOWS,
    APP_REGISTERS_SLOT_ACTIVATE_WITH_SINGLE_CLICK,
    APP_REGISTERS_SLOT_FORCE_CONFIRM,
    APP_REGISTERS_SLOT_STRING,
    APP_REGISTERS_SLOT_COMMAND_NAME,
    APP_REGISTERS_SLOT_DIRECTION,
    APP_REGISTERS_SLOT_OS_EVENT,
    APP_REGISTERS_SLOT_COUNT
};

enum AppCommandKind {
    APP_COMMAND_KIND_NULL,
    APP_COMMAND_KIND_EXIT,
    APP_COMMAND_KIND_OPEN_PALETTE,
    APP_COMMAND_KIND_RUN_COMMAND,
    APP_COMMAND_KIND_STATE,
    APP_COMMAND_KIND_EVAL,
    APP_COMMAND_KIND_OS_EVENT,
    APP_COMMAND_KIND_INC_WINDOW_FONT_SIZE,
    APP_COMMAND_KIND_DEC_WINDOW_FONT_SIZE,
    APP_COMMAND_KIND_OPEN_WINDOW,
    APP_COMMAND_KIND_WINDOW_SETTINGS,
    APP_COMMAND_KIND_CLOSE_WINDOW,
    APP_COMMAND_KIND_TOGGLE_FULLSCREEN,
    APP_COMMAND_KIND_BRING_TO_FRONT,
    APP_COMMAND_KIND_POP_UP_ACCEPT,
    APP_COMMAND_KIND_POP_UP_CANCEL,
    APP_COMMAND_KIND_RESET_TO_DEFAULT_BINDINGS,
    APP_COMMAND_KIND_RESET_TO_DEFAULT_PANELS,
    APP_COMMAND_KIND_RESET_TO_COMPACT_PANELS,
    APP_COMMAND_KIND_RESET_TO_SIMPLE_PANELS,
    APP_COMMAND_KIND_NEW_PANEL_LEFT,
    APP_COMMAND_KIND_NEW_PANEL_UP,
    APP_COMMAND_KIND_NEW_PANEL_RIGHT,
    APP_COMMAND_KIND_NEW_PANEL_DOWN,
    APP_COMMAND_KIND_SPLIT_PANEL,
    APP_COMMAND_KIND_ROTATE_PANEL_COLUMNS,
    APP_COMMAND_KIND_NEXT_PANEL,
    APP_COMMAND_KIND_PREV_PANEL,
    APP_COMMAND_KIND_FOCUS_PANEL,
    APP_COMMAND_KIND_FOCUS_PANEL_RIGHT,
    APP_COMMAND_KIND_FOCUS_PANEL_LEFT,
    APP_COMMAND_KIND_FOCUS_PANEL_UP,
    APP_COMMAND_KIND_FOCUS_PANEL_DOWN,
    APP_COMMAND_KIND_UNDO,
    APP_COMMAND_KIND_REDO,
    APP_COMMAND_KIND_GO_BACK,
    APP_COMMAND_KIND_GO_FORWARD,
    APP_COMMAND_KIND_CLOSE_PANEL,
    APP_COMMAND_KIND_FOCUS_TAB,
    APP_COMMAND_KIND_NEXT_TAB,
    APP_COMMAND_KIND_PREV_TAB,
    APP_COMMAND_KIND_MOVE_TAB_RIGHT,
    APP_COMMAND_KIND_MOVE_TAB_LEFT,
    APP_COMMAND_KIND_OPEN_TAB,
    APP_COMMAND_KIND_BUILD_TAB,
    APP_COMMAND_KIND_DUPLICATE_TAB,
    APP_COMMAND_KIND_COPY_TAB_FULL_PATH,
    APP_COMMAND_KIND_CLOSE_TAB,
    APP_COMMAND_KIND_MOVE_VIEW,
    APP_COMMAND_KIND_TAB_BAR_TOP,
    APP_COMMAND_KIND_TAB_BAR_BOTTOM,
    APP_COMMAND_KIND_TAB_SETTINGS,
    APP_COMMAND_KIND_SET_CURRENT_PATH,
    APP_COMMAND_KIND_OPEN,
    APP_COMMAND_KIND_SWITCH,
    APP_COMMAND_KIND_SHOW_FILE_IN_EXPLORER,
    APP_COMMAND_KIND_NEW_SESSION,
    APP_COMMAND_KIND_OPEN_SESSION,
    APP_COMMAND_KIND_OPEN_RECENT_SESSION,
    APP_COMMAND_KIND_SAVE_SESSION,
    APP_COMMAND_KIND_WRITE_SESSION_DATA,
    APP_COMMAND_KIND_SESSION_SETTINGS,
    APP_COMMAND_KIND_EDIT,
    APP_COMMAND_KIND_ACCEPT,
    APP_COMMAND_KIND_CANCEL,
    APP_COMMAND_KIND_MOVE_LEFT,
    APP_COMMAND_KIND_MOVE_RIGHT,
    APP_COMMAND_KIND_MOVE_UP,
    APP_COMMAND_KIND_MOVE_DOWN,
    APP_COMMAND_KIND_MOVE_LEFT_SELECT,
    APP_COMMAND_KIND_MOVE_RIGHT_SELECT,
    APP_COMMAND_KIND_MOVE_UP_SELECT,
    APP_COMMAND_KIND_MOVE_DOWN_SELECT,
    APP_COMMAND_KIND_MOVE_LEFT_CHUNK,
    APP_COMMAND_KIND_MOVE_RIGHT_CHUNK,
    APP_COMMAND_KIND_MOVE_UP_CHUNK,
    APP_COMMAND_KIND_MOVE_DOWN_CHUNK,
    APP_COMMAND_KIND_MOVE_LEFT_CHUNK_SELECT,
    APP_COMMAND_KIND_MOVE_RIGHT_CHUNK_SELECT,
    APP_COMMAND_KIND_MOVE_UP_CHUNK_SELECT,
    APP_COMMAND_KIND_MOVE_DOWN_CHUNK_SELECT,
    APP_COMMAND_KIND_MOVE_UP_PAGE,
    APP_COMMAND_KIND_MOVE_DOWN_PAGE,
    APP_COMMAND_KIND_MOVE_UP_WHOLE,
    APP_COMMAND_KIND_MOVE_DOWN_WHOLE,
    APP_COMMAND_KIND_MOVE_UP_PAGE_SELECT,
    APP_COMMAND_KIND_MOVE_DOWN_PAGE_SELECT,
    APP_COMMAND_KIND_MOVE_UP_WHOLE_SELECT,
    APP_COMMAND_KIND_MOVE_DOWN_WHOLE_SELECT,
    APP_COMMAND_KIND_MOVE_UP_REORDER,
    APP_COMMAND_KIND_MOVE_DOWN_REORDER,
    APP_COMMAND_KIND_MOVE_HOME,
    APP_COMMAND_KIND_MOVE_END,
    APP_COMMAND_KIND_MOVE_HOME_SELECT,
    APP_COMMAND_KIND_MOVE_END_SELECT,
    APP_COMMAND_KIND_SELECT_ALL,
    APP_COMMAND_KIND_DELETE_SINGLE,
    APP_COMMAND_KIND_DELETE_CHUNK,
    APP_COMMAND_KIND_BACKSPACE_SINGLE,
    APP_COMMAND_KIND_BACKSPACE_CHUNK,
    APP_COMMAND_KIND_COPY,
    APP_COMMAND_KIND_CUT,
    APP_COMMAND_KIND_PASTE,
    APP_COMMAND_KIND_INSERT_TEXT,
    APP_COMMAND_KIND_MOVE_NEXT,
    APP_COMMAND_KIND_MOVE_PREV,
    APP_COMMAND_KIND_CENTRE_CURSOR,
    APP_COMMAND_KIND_CONTAIN_CURSOR,
    APP_COMMAND_KIND_FIND_NEXT,
    APP_COMMAND_KIND_FIND_PREV,
    APP_COMMAND_KIND_ENABLE_CONFIG,
    APP_COMMAND_KIND_DISABLE_CONFIG,
    APP_COMMAND_KIND_SELECT_CONFIG,
    APP_COMMAND_KIND_DESELECT_CONFIG,
    APP_COMMAND_KIND_REMOVE_CONFIG,
    APP_COMMAND_KIND_NAME_CONFIG,
    APP_COMMAND_KIND_DUPLICATE_CONFIG,
    APP_COMMAND_KIND_SAVE_TO_SESSION,
    APP_COMMAND_KIND_EDIT_SESSION_THEME,
    APP_COMMAND_KIND_ADD_THEME_COLOUR,
    APP_COMMAND_KIND_FORK_THEME,
    APP_COMMAND_KIND_SAVE_THEME,
    APP_COMMAND_KIND_SAVE_AND_SET_THEME,
    APP_COMMAND_KIND_PICK_FILE,
    APP_COMMAND_KIND_PICK_FOLDER,
    APP_COMMAND_KIND_PICK_FILE_OR_FOLDER,
    APP_COMMAND_KIND_TOGGLE_DEV_MENU,
    APP_COMMAND_KIND_RECORD_SESSION_AS_LAST_OPENED,
    APP_COMMAND_KIND_RECORD_RECENT_SESSION,
    APP_COMMAND_KIND_QUERY,
    APP_COMMAND_KIND_PUSH_QUERY,
    APP_COMMAND_KIND_INC_VIEW_FONT_SIZE,
    APP_COMMAND_KIND_DEC_VIEW_FONT_SIZE,
    APP_COMMAND_KIND_RECORD_FILE_IN_SESSION,
    APP_COMMAND_KIND_COMPLETE_QUERY,
    APP_COMMAND_KIND_CANCEL_QUERY,
    APP_COMMAND_KIND_UPDATE_QUERY,
    APP_COMMAND_KIND_OPEN_WATCH,
    APP_COMMAND_KIND_OPEN_FILE_PATH_MAPS,
    APP_COMMAND_KIND_OPEN_TYPE_VIEWS,
    APP_COMMAND_KIND_OPEN_TEXT,
    APP_COMMAND_KIND_OPEN_BITMAP,
    APP_COMMAND_KIND_OPEN_COLOUR,
    APP_COMMAND_KIND_OPEN_GEO3D,
    APP_COMMAND_KIND_COUNT,
    APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD = APP_COMMAND_KIND_OPEN_WATCH
};

typedef u32 AppQueryKind;
enum : u32 {
    APP_QUERY_KIND_ALLOW_FILES      = (1 << 0),
    APP_QUERY_KIND_ALLOW_FOLDERS    = (1 << 1),
    APP_QUERY_KIND_KEEP_OLD_INPUT   = (1 << 2),
    APP_QUERY_KIND_SELECT_OLD_INPUT = (1 << 3),
    APP_QUERY_KIND_FLOATING         = (1 << 4),
    APP_QUERY_KIND_REQUIRED         = (1 << 5),
};

typedef u32 AppCommandKindFlags;
enum : u32 {
    APP_COMMAND_KIND_FLAG_LIST_IN_UI         = (1 << 0),
    APP_COMMAND_KIND_FLAG_LIST_IN_TAB        = (1 << 1),
    APP_COMMAND_KIND_FLAG_LIST_IN_TEXT_POINT = (1 << 2),
    APP_COMMAND_KIND_FLAG_LIST_IN_TEXT_RANGE = (1 << 3),
};

typedef u32 AppCommandBindingButtonFlag;
enum : u32 {
    APP_COMMAND_BINDING_BTN_FLAG_ADD_NEW = (1 << 0),
    APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT = (1 << 1)
};

enum AppIconKind {
    APP_ICON_KIND_NULL,
    APP_ICON_KIND_FOLDER_OPEN_OUTLINE,
    APP_ICON_KIND_FOLDER_CLOSED_OUTLINE,
    APP_ICON_KIND_FOLDER_OPEN_FILLED,
    APP_ICON_KIND_FOLDER_CLOSED_FILLED,
    APP_ICON_KIND_FILE_OUTLINE,
    APP_ICON_KIND_FILE_FILLED,
    APP_ICON_KIND_INFO,
    APP_ICON_KIND_WARNING_SMALL,
    APP_ICON_KIND_WARNING_BIG,
    APP_ICON_KIND_UNLOCKED,
    APP_ICON_KIND_LOCKED,
    APP_ICON_KIND_LEFT_ARROW,
    APP_ICON_KIND_RIGHT_ARROW,
    APP_ICON_KIND_UP_ARROW,
    APP_ICON_KIND_DOWN_ARROW,
    APP_ICON_KIND_GEAR,
    APP_ICON_KIND_PENCIL,
    APP_ICON_KIND_TRASH,
    APP_ICON_KIND_PIN,
    APP_ICON_KIND_RADIO_HOLLOW,
    APP_ICON_KIND_RADIO_FILLED,
    APP_ICON_KIND_CHECK_HOLLOW,
    APP_ICON_KIND_CHECK_FILLED,
    APP_ICON_KIND_CHECK,
    APP_ICON_KIND_LEFT_CARET,
    APP_ICON_KIND_RIGHT_CARET,
    APP_ICON_KIND_UP_CARET,
    APP_ICON_KIND_DOWN_CARET,
    APP_ICON_KIND_UP_SCROLL,
    APP_ICON_KIND_DOWN_SCROLL,
    APP_ICON_KIND_LEFT_SCROLL,
    APP_ICON_KIND_RIGHT_SCROLL,
    APP_ICON_KIND_ADD,
    APP_ICON_KIND_MINUS,
    APP_ICON_KIND_MACHINE,
    APP_ICON_KIND_CIRCLE_FILLED,
    APP_ICON_KIND_X,
    APP_ICON_KIND_REFRESH,
    APP_ICON_KIND_UNDO,
    APP_ICON_KIND_REDO,
    APP_ICON_KIND_SAVE,
    APP_ICON_KIND_WINDOW,
    APP_ICON_KIND_CLIPBOARD,
    APP_ICON_KIND_XSPLIT,
    APP_ICON_KIND_YSPLIT,
    APP_ICON_KIND_CLOSE_PANEL,
    APP_ICON_KIND_FIND,
    APP_ICON_KIND_PALETTE,
    APP_ICON_KIND_THUMBNAILS,
    APP_ICON_KIND_GLASSES,
    APP_ICON_KIND_BINOCULARS,
    APP_ICON_KIND_LIST,
    APP_ICON_KIND_GRID,
    APP_ICON_KIND_QUESTION_MARK,
    APP_ICON_KIND_PERSON,
    APP_ICON_KIND_BRIEFCASE,
    APP_ICON_KIND_DOT,
    APP_ICON_KIND_BITMAP,
    APP_ICON_KIND_CUBE,
    APP_ICON_KIND_WINDOW_RESTORE,
    APP_ICON_KIND_WINDOW_MINIMIZE,
    APP_ICON_KIND_DUPLICATE,
    APP_ICON_KIND_COUNT
};

typedef u32 AppThemePreset;
enum : u32 {
    APP_THEME_PRESET_DEFAULT_DARK,
    APP_THEME_PRESET_DEFAULT_LIGHT,
    APP_THEME_PRESET_VISUAL_STUDIO_DARK,
    APP_THEME_PRESET_VISUAL_STUDIO_LIGHT,
    APP_THEME_PRESET_SOLARISED_DARK,
    APP_THEME_PRESET_SOLARISED_LIGHT,
    APP_THEME_PRESET_HANDMADE_HERO,
    APP_THEME_PRESET_NAYSAYER,
    APP_THEME_PRESET_FOUR_CODER,
    APP_THEME_PRESET_GROVE,
    APP_THEME_PRESET_FAR_MANAGER,
    APP_THEME_PRESET_AYU_DARK,
    APP_THEME_PRESET_GRUVBOX_DARK,
    APP_THEME_PRESET_TOKYO_NIGHT,
    APP_THEME_PRESET_DRACULA,
    APP_THEME_PRESET_NORD,
    APP_THEME_PRESET_CATPUCCIN_MOCHA,
    APP_THEME_PRESET_MONOKAI,
    APP_THEME_PRESET_COUNT
};

enum AppFontSlot {
    APP_FONT_SLOT_MAIN,
    APP_FONT_SLOT_CODE,
    APP_FONT_SLOT_ICONS,
    APP_FONT_SLOT_COUNT
};

enum AppCodeColourSlotKind {
    APP_CODE_COLOUR_SLOT_CODE_DEFAULT,
    APP_CODE_COLOUR_SLOT_CODE_SYMBOL,
    APP_CODE_COLOUR_SLOT_CODE_TYPE,
    APP_CODE_COLOUR_SLOT_CODE_LOCAL,
    APP_CODE_COLOUR_SLOT_CODE_REGISTER,
    APP_CODE_COLOUR_SLOT_CODE_KEYWORD,
    APP_CODE_COLOUR_SLOT_CODE_DELIMITER_OR_OPERATOR,
    APP_CODE_COLOUR_SLOT_CODE_NUMERIC,
    APP_CODE_COLOUR_SLOT_CODE_NUMERIC_ALT_DIGIT_GROUP,
    APP_CODE_COLOUR_SLOT_CODE_STRING,
    APP_CODE_COLOUR_SLOT_CODE_META,
    APP_CODE_COLOUR_SLOT_CODE_COMMENT,
    APP_CODE_COLOUR_SLOT_CODE_LINE_NUMBERS,
    APP_CODE_COLOUR_SLOT_CODE_LINE_NUMBERS_SELECTED,
    APP_CODE_COLOUR_SLOT_COUNT
};

#define AppPluralFromCodeName(X)                          (AppVocabInfoFromCodeName(X)->CodeNamePlural)
#define AppDisplayFromCodeName(X)                         (AppVocabInfoFromCodeName(X)->DisplayName)
#define AppDisplayPluralFromCodeName(X)                   (AppVocabInfoFromCodeName(X)->DisplayNamePlural)
#define AppIconKindFromCodeName(X)                        (AppVocabInfoFromCodeName(X)->IconKind)
#define AppSingularFromCodeNamePlural(X)                  (AppVocabInfoFromCodeNamePlural(X)->CodeName)
#define GetAppViewState(X) (X*)                           AppViewStateFromSize(sizeof(X))
#define AppStoreViewParameterF32(X, Y)                    AppStoreViewParameter((X), "%ff", (Y))
#define AppStoreViewParameterI64(X, Y)                    AppStoreViewParameter((X), "%lld", (Y))
#define AppStoreViewParameterU64(X, Y)                    AppStoreViewParameter((X), "0x%llX", (Y))
#define AppFont(X)                                        UIFont(AppFontFromSlot(X)) UITextRasterFlags(AppRasterKindFromSlot((X)))
#define Registers()                                       (&APP_STATE->HeadRegisters->V)
#define BaseRegisters()                                   (&APP_STATE->BaseRegisters.V)

#define APP_REGISTERS_DEFAULT_INIT(X)                                 \
    X.Window = Registers()->Window,                                   \
    X.Panel = Registers()->Panel,                                     \
    X.Tab = Registers()->Tab,                                         \
    X.View = Registers()->View,                                       \
    X.PrevTab = Registers()->PrevTab,                                 \
    X.DstPanel = Registers()->DstPanel,                               \
    X.Config = Registers()->Config,                                   \
    X.ConfigList = Registers()->ConfigList,                           \
    X.Space = Registers()->Space,                                     \
    X.FilePath = Registers()->FilePath,                               \
    X.Cursor = Registers()->Cursor,                                   \
    X.Mark = Registers()->Mark,                                       \
    X.TextKey = Registers()->TextKey,                                 \
    X.Expression = Registers()->Expression,                           \
    X.UI = Registers()->UI,                                           \
    X.SrcUI = Registers()->SrcUI,                                     \
    X.OffsetPX = Registers()->OffsetPX,                               \
    X.RegSlot = Registers()->RegSlot,                                 \
    X.DoImplicitRoot = Registers()->DoImplicitRoot,                   \
    X.DoSmall = Registers()->DoSmall,                                 \
    X.DoLister = Registers()->DoLister,                               \
    X.DoBigRows = Registers()->DoBigRows,                             \
    X.AllWindows = Registers()->AllWindows,                           \
    X.ActivateWithSingleClick = Registers()->ActivateWithSingleClick, \
    X.String = Registers()->String,                                   \
    X.CommandName = Registers()->CommandName,                         \
    X.ForceConfirm = Registers()->ForceConfirm,                       \
    X.OSEvent = Registers()->OSEvent                                  \


#define AppPushRegisters(...) [&]() -> AppRegisters* { \
    AppRegisters __Registers = {};                     \
    APP_REGISTERS_DEFAULT_INIT(__Registers);           \
    __VA_ARGS__;                                       \
    return __AppPushRegisters(&__Registers);           \
}()

#define AppRegistersScope(...) DEFER(AppPushRegisters(__VA_ARGS__), AppPopRegisters())

#define AppCmd(X, ...) [&](){                                        \
    AppRegisters __Registers = {};                                   \
    APP_REGISTERS_DEFAULT_INIT(__Registers);                         \
    __VA_ARGS__;                                                     \
    AppPushCmd(APP_COMMAND_KIND_INFO_TABLE[X].String, &__Registers); \
}()

enum AppQueryExpressionKind {
    APP_QUERY_EXPR_KIND_NULL,
    APP_QUERY_EXPR_KIND_COMMANDS,
    APP_QUERY_EXPR_KIND_TAB_COMMANDS,
    APP_QUERY_EXPR_KIND_RECENT_FILES,
    APP_QUERY_EXPR_KIND_SOURCE_FILES,
    APP_QUERY_EXPR_KIND_RECENT_SESSIONS,
    APP_QUERY_EXPR_KIND_THEMES,
    APP_QUERY_EXPR_KIND_TYPES,
    APP_QUERY_EXPR_KIND_FILE_PATH_MAPS,
    APP_QUERY_EXPR_KIND_OUTPUT,
    APP_QUERY_EXPR_KIND_SESSION_SETTINGS,
    APP_QUERY_EXPR_KIND_CONFIG,
    APP_QUERY_EXPR_KIND_CONFIG_CHILD,
    APP_QUERY_EXPR_KIND_FILE_PATH,
    APP_QUERY_EXPR_KIND_COUNT
};

typedef u32 AppCellKind;
enum : u32 {
    APP_CELL_KIND_EXPANDER              = (1 << 0),
    APP_CELL_KIND_EXPANDER_SPACE        = (1 << 1),
    APP_CELL_KIND_EXPANDER_PLACEHOLDER  = (1 << 2),
    APP_CELL_KIND_TOGGLE_SWITCH         = (1 << 3),
    APP_CELL_KIND_SLIDER                = (1 << 4),
    APP_CELL_KIND_BINDINGS              = (1 << 5),
    APP_CELL_KIND_EMPTY_EDIT_BTN        = (1 << 6),
    APP_CELL_KIND_REVERT_BTN            = (1 << 7),
    APP_CELL_KIND_DISABLE_EDIT          = (1 << 8),
    APP_CELL_KIND_KEYBOARD_CLICKABLE    = (1 << 9),
    APP_CELL_KIND_SINGLE_CLICK_ACTIVATE = (1 << 10),
    APP_CELL_KIND_CODE_CONTENTS         = (1 << 11),                            // WARN(nathan): to be removed / replaced
    APP_CELL_KIND_BORDER                = (1 << 12),
    APP_CELL_KIND_NO_BACKGROUND         = (1 << 13),
    APP_CELL_KIND_BTN                   = (1 << 14)
};

enum AppWatchCellKind {
    APP_WATCH_CELL_KIND_EVAL,
    APP_WATCH_CELL_KIND_VIEW_UI
};

typedef u32 AppWatchCellFlag;
enum : u32 {
    APP_WATCH_CELL_FLAG_EXPR                       = (1 <<  0),
    APP_WATCH_CELL_FLAG_NO_EVAL                    = (1 <<  1),
    APP_WATCH_CELL_FLAG_BTN                        = (1 <<  2),
    APP_WATCH_CELL_FLAG_BACKGROUND                 = (1 <<  3),
    APP_WATCH_CELL_FLAG_ACTIVATE_WITH_SINGLE_CLICK = (1 <<  4),
    APP_WATCH_CELL_FLAG_IS_NONE_CODE               = (1 <<  5),
    APP_WATCH_CELL_FLAG_CAN_EDIT                   = (1 <<  6),
    APP_WATCH_CELL_FLAG_IS_ERRORED                 = (1 <<  7),
    APP_WATCH_CELL_FLAG_INDENTED                   = (1 <<  8)
};

#define WatchCellPushNew(X, Y, Z, W, ...) [&]() -> AppWatchCell* { \
    AppWatchCell __Cell = {};                                      \
    __Cell.Kind = (Z);                                             \
    __Cell.Eval = (W);                                             \
    __VA_ARGS__;                                                   \
    return __WatchCellPushNew((X), (Y), &__Cell);                  \
}()

#define AppSetAutoCompleteRegisters(X, ...) [&]() {   \
    AppRegisters __Registers = {};                    \
    APP_REGISTERS_DEFAULT_INIT(__Registers);          \
    __VA_ARGS__;                                      \
    __AppSetAutoCompleteRegisters((X), &__Registers); \
}()

// @types______________________________________________________________________
struct AppViewUIRule {
    Str8               Name;
    AppViewUIFuncType* ViewUIFunc;
};

struct AppViewUIRuleNode {
    AppViewUIRuleNode* Next;
    AppViewUIRule      V;
};

struct AppViewUIRuleSlot {
    AppViewUIRuleNode* Head;
    AppViewUIRuleNode* Tail;
};

struct AppViewUIRuleMap {
    AppViewUIRuleSlot* Slots;
    u64                SlotsCount;
};

struct AppLocation {
    Str8      FilePath;
    TextPoint Point;
    Str8      Expression;
};

struct AppAutoCompleteCursorInfo {
    Str8    ListExpr;
    Str8    Filter;
    r1u64   ReplacedRange;
    Str8    CalleeExpr;
    MDNode* ArgSchema;
};

struct AppRegisters {
    ConfigID             Window;
    ConfigID             Panel;
    ConfigID             Tab;
    ConfigID             View;
    ConfigID             PrevTab;
    ConfigID             DstPanel;
    ConfigID             Config;
    ConfigIDList         ConfigList;
    EvalSpace            Space;
    Str8                 FilePath;
    TextPoint            Cursor;
    TextPoint            Mark;
    CKey                 TextKey;
    Str8                 Expression;
    UIKey                UI;
    UIKey                SrcUI;
    v2f32                OffsetPX;
    AppRegistersSlot     RegSlot;
    Str8                 ContextExpression;
    b32                  DoImplicitRoot;
    b32                  DoLister;
    b32                  DoSmall;
    b32                  DoBigRows;
    b32                  AllWindows;
    b32                  ActivateWithSingleClick;
    b32                  ForceConfirm;
    Str8                 String;
    Str8                 CommandName;
    Direction2D          Direction;
    InputEvent*          OSEvent;
};

struct AppRegistersNode {
    AppRegistersNode* Next;
    AppRegisters      V;
};

struct AppQuery {
    AppQueryKind     Kind;
    AppRegistersSlot Slot;
    Str8             Expression;
    Str8             ViewName;
};

struct AppCommandKindInfo {
    Str8                String;
    Str8                Description;
    Str8                SearchTags;
    Str8                ContextFilter;
    AppCommandKindFlags Flags;
    AppQuery            Query;
};

struct AppVocabInfo {
    Str8        CodeName;
    Str8        CodeNamePlural;
    Str8        DisplayName;
    Str8        DisplayNamePlural;
    AppIconKind IconKind;
};

struct AppVocabInfoMapNode {
    AppVocabInfoMapNode* SingleNext;
    AppVocabInfoMapNode* PluralNext;
    AppVocabInfo         V;
};

struct AppVocabInfoMapSlot {
    AppVocabInfoMapNode* Head;
    AppVocabInfoMapNode* Tail;
};

struct AppVocabInfoMap {
    u64                  SingleSlotsCount;
    AppVocabInfoMapSlot* SingleSlots;
    u64                  PluralSlotsCount;
    AppVocabInfoMapSlot* PluralSlots;
};

struct AppNameSchemaInfo {
    Str8 Name;
    b32  IsView;
    Str8 Schema;
};

struct AppCommand {
    Str8          Name;
    AppRegisters* Regs;
};

struct AppCommandNode {
    AppCommandNode* Next;
    AppCommandNode* Prev;
    AppCommand      Command;
};

struct AppCommandList {
    AppCommandNode* Head;
    AppCommandNode* Tail;
    u64             Count;
};

struct AppArenaExt {
    AppArenaExt* Next;
    Arena*       MemPool;
};

struct AppWatchPoint {
    EVKey ParentKey;
    EVKey Key;
    u64   CellID;
};

struct AppWatchViewTextEditState {
    AppWatchViewTextEditState* PointHashNext;
    AppWatchPoint              Point;
    TextPoint                  Cursor;
    TextPoint                  Mark;
    u8                         InputBuffer[1024];
    u64                        InputSize;
    u8                         InitialBuffer[1024];
    u64                        InitialSize;
};

struct AppWatchViewState {
    b32                         Initialised;
    Arena*                      FilterMemPool;
    Str8                        PrevFilter;
    AppWatchPoint               Cursor;
    AppWatchPoint               Mark;
    AppWatchPoint               NextCursor;
    AppWatchPoint               NextMark;
    Arena*                      TextEditMemPool;
    u64                         TextEditStateSlotsCount;
    AppWatchViewTextEditState   DummyTextEditState;
    AppWatchViewTextEditState** TextEditStateSlots;
    b32                         IsTextEditing;
};

struct AppViewState {
    AppViewState*     HashNext;
    AppViewState*     HashPrev;
    ConfigID          CfgID;
    u64               LastFrameIndexTouched;
    u64               LastFrameIndexBuilt;
    f32               LoadingT;
    f32               LoadingTTarget;
    u64               LoadingProgressV;
    u64               LoadingProgressVTarget;
    UIScrollPoint2D   ScrollPosition;
    EVView*           View;
    Arena*            MemPool;
    u64               MemPoolResetPosition;
    AppArenaExt*      HeadArenaExt;
    AppArenaExt*      TailArenaExt;
    void*             UserData;
    b32               QueryIsOpen;
    TextPoint         QueryCursor;
    TextPoint         QueryMark;
    u8                QueryBuffer[KB(1)];
    u64               QueryStringSize;
    // u64               QuerySelectedIndex;
    // i64               QueryHoveredIndex;
    Str8              QueryCommandName;
    Str8              QueryExpression;
    b32               ContentsAreFocused;
    AppWatchViewState WatchState;
};

struct AppViewStateSlot {
    AppViewState* Head;
    AppViewState* Tail;
};

// struct AppCommandPaletteState {
//     b32             IsOpen;
//     u64             InputSize;
//     u8              InputBuffer[KB(1)];
//     TextPoint       Cursor;
//     TextPoint       Mark;
//     u64             SelectedIndex;
//     UIScrollPoint2D Scroll;
// };

struct AppDropCompletionTask {
    AppDropCompletionTask* Next;
    b32                    Cfg;
    Str8List               Paths;
};

struct AppWindowState {
    AppWindowState*           OrderNext;
    AppWindowState*           OrderPrev;
    AppWindowState*           HashNext;
    AppWindowState*           HashPrev;
    ConfigID                  CfgID;
    u64                       FramesAlive;
    u64                       LastFrameIndexTouched;
    Arena*                    MemPool;
    Handle                    WinHandle;
    RendHandle                RendHandle;
    UIState*                  UI;
    f32                       LastDPI;
    b32                       WindowLayoutReset;
    r2f32                     LastWindowRect;
    UITheme*                  Theme;
    v4f32                     ThemeCodeColours[APP_CODE_COLOUR_SLOT_COUNT];
    FontRasterKind            FontSlotRasterKind[APP_FONT_SLOT_COUNT];
    b32                       DevMenuIsOpen;
    b32                       MenuBarFocused;
    b32                       MenuBarFocusedOnPress;
    // AppCommandPaletteState    CommandPalette;
    b32                       MenuBarKeyHeld;
    b32                       MenuBarFocusPressStarted;
    Arena*                    DropCompletionMemPool;
    ConfigID                  DropCompletionPanel;
    AppDropCompletionTask*    HeadDropCompletionTask;
    b32                       QueryIsActive;
    Arena*                    QueryMemPool;
    AppRegisters*             QueryRegs;
    // ConfigID                  QueryViewID;
    ConfigID                  QueryLastViewID;
    b32                       HoverEvalFocused;
    Arena*                    HoverEvalMemPool;
    v2f32                     HoverEvalSpawnPosition;
    Str8                      HoverEvalString;
    u64                       HoverEvalFirstTimeUSecs;
    u64                       HoverEvalLastTimeUSecs;
    // u8                        ErrorBuffer[512];
    // u64                       ErrorStrSize;
    f32                       ErrorT;
    UIEventList               UIEvents;
    DBucket*                  DrawBucket;
    u64                       AutoCompletePrevFrameIndex;
    Arena*                    AutoCompleteMemPool;
    AppRegisters*             AutoCompleteRegs;
    AppAutoCompleteCursorInfo AutoCompleteCursorInfo;
};

struct AppWindowStateSlot {
    AppWindowState* Head;
    AppWindowState* Tail;
};

struct AppNameChunkNode {
    AppNameChunkNode* Next;
    u64               Size;
};

struct AppState {
    Arena*              MemPool;
    b32                 Quit;
    u64                 FrameIndex;
    i32                 FrameDepth;
    f32                 FrameDeltaTime;
    Access*             FrameAccess;
    Arena*              FrameMemPools[2];
    u64                 FrameTimeUSecsHistory[64];
    u64                 NumFramesRequested;
    u64                 FrameEvalMemReadEndTimeUSecs;
    f64                 TimeSecs;
    u64                 TimeUSecs;
    Arena*              SessionPathMemPool;
    Str8                SessionPath;
    Arena*              CommandMemPools[2];
    AppCommandList      Commands[2];
    u64                 CommandsGeneration;
    Arena*              CommandOutputMemPool;
    Str8List            CommandOutputs;
    AppRegistersNode    BaseRegisters;
    AppRegistersNode*   HeadRegisters;
    u64                 WindowStateSlotsCount;
    AppWindowStateSlot* WindowStateSlots;
    ConfigID            LastFocusedWindow;
    AppWindowState*     HeadWindowState;
    AppWindowState*     TailWindowState;
    AppWindowState*     FreeWindowState;
    ConfigID            WindowStateLastAccessID;
    AppWindowState*     WindowStateLastAccessed;
    u64                 ViewStateSlotsCount;
    AppViewStateSlot*   ViewStateSlots;
    AppViewState*       FreeViewState;
    ConfigID            ViewStateLastAccessedID;
    AppViewState*       ViewStateLastAccessed;
    ConfigInputMap*     KeyMap;
    f32                 CatchAllAnimationRate;
    f32                 MenuAnimationRate;
    f32                 MenuAnimationRateSlow;
    f32                 ScrollingAnimationRate;
    f32                 TooltipAnimationRate;
    f32                 RichHoverAnimationRate;
    CKey                SessionConfigStringKey;
    CKey                TransientConfigStringKey;
    MDNodePtrList*      Schemas;
    MDNode*             ThemePresetTrees[APP_THEME_PRESET_COUNT];
    AppVocabInfoMap     VocabInfoMap;
    UIKey               PopUpKey;
    b32                 PopUpActive;
    f32                 PopUpT;
    Arena*              PopUpMemPool;
    AppCommandList      PopUpCommands;
    Str8                PopUpTitle;
    Str8                PopUpDescription;
    b32                 TextEditMode;
    AppRegisters*       HoverRegisters;
    AppRegistersSlot    HoverRegistersSlot;
    AppRegisters*       NextHoverRegisters;
    AppRegistersSlot    NextHoverRegistersSlot;
    RendHandle          IconTexture;
    UIKey               DropCompletionKey;
    UIKey               ContextMenuKey;
    Arena*              DragDropMemPool;
    AppRegisters*       DragDropRegisters;
    AppRegistersSlot    DragDropRegistersSlot;
    AppDragDropState    DragDropState;
    // AppNameChunkNode*   FreeNameChunks[ARRAY_COUNT(CFG_STRING_BUCKET_CHUNK_SIZES)];
    ConfigState*        Config;
    ConfigSchemaTable*  CfgSchemaTable;
    ConfigNode*         RootConfig;
    u64                 ConfigIDSlotsCount;
    ConfigNodePtrSlot*  ConfigIDSlots;
    u64                 ConfigIDGeneration;
    ConfigID            ConfigLastAccessedID;
    ConfigNode*         ConfigLastAccessed;
    u64                 ConfigChangeGeneration;
    Arena*              BindChangeMemPool;
    b32                 BindChangeActive;
    ConfigID            BindChangeBindingID;
    Str8                BindChangeCommandName;
    FontTag             FontSlotTable[APP_FONT_SLOT_COUNT];
    Arena*              ThemePathMemPool;
    b32                 AltMenuBarEnabled;
    EVStringFlag        EvalVisualBaseStringFlags;
    EvalCache*          ECache;
    AppViewUIRuleMap*   ViewUIRuleMap;
    EvalStr2TypeKeyMap* MetaName2TypeMap;
};

struct BindingTableEntry {
    Str8          String;
    ConfigBinding Binding;
};

// struct AppQueryExpression {
//     AppQueryExpressionKind Kind;
//     ConfigID               ConfigID;
//     Str8                   ChildPath;
//     Str8                   RawExpression;
// };

struct AppQueryResult {
    Str8                 String;
    Str8                 DisplayString;
    Str8                 Description;
    Str8                 SearchTags;
    AppIconKind          IconKind;
    ConfigID             CfgID;
    AppCommandKindInfo*  CommandInfo;
    void*                UserData;
};

struct AppQueryResultNode {
    AppQueryResultNode* Next;
    AppQueryResultNode* Prev;
    AppQueryResult      V;
};

struct AppQueryResultList {
    AppQueryResultNode* Head;
    AppQueryResultNode* Tail;
    u64                 Count;
};

struct AppCellParams {
    AppCellKind  Kind;
    i32          Depth;
    Str8         PreEditValue;
    FancyStrList MetaFancyStrings;
    FancyStrList ValueFancyStrings;
    FancyStrList NoteFancyStrings;
    UISize       NoteWidth;
    Str8         SearchNeedle;
    Str8         Description;
    b32*         Expanded;
    b32*         Toggled;
    f32*         SliderValue;
    Str8         BindingsName;
    b32*         Revert;
    TextPoint*   Cursor;
    TextPoint*   Mark;
    u8*          EditBuffer;
    u64          EditBufferSize;
    u64*         EditStringSize;
    UIKey*       LineEditKey;
};

struct AppViewWatchState {
    Arena* MemPool;
    u64*   ExpandedKeys;
    u64    ExpandedKeysCapacity;
};

struct AppViewWatchRow {
    AppQueryResult* Result;
};

struct AppWatchCell {
    AppWatchCell*    Next;
    AppWatchCellKind Kind;
    AppWatchCellFlag Flags;
    u64              Index;
    EvalEval         Eval;
    f32              DefaultPercent;
    f32              Percent;
    f32              PX;
};

struct AppWatchCellList {
    AppWatchCell* Head;
    AppWatchCell* Tail;
    u64           Count;
    f32           TotalPercent;
};

struct AppWatchRowInfo {
    u64              ProjectIndex;
    b32              CanExpand;
    b32              ExprIsEditable;
    Str8             GroupCfgName;
    ConfigNode*      GroupCfgParent;
    ConfigNode*      GroupCfgChild;
    Str8             CellStyleKey;
    AppWatchCellList Cells;
    AppViewUIRule*   ViewUIRule;
};

struct AppWatchRowCellInfo {
    AppWatchCellFlag     Flags;
    ConfigNode*          Config;
    Str8                 CommandName;
    Str8                 FilePath;
    FancyStrList         ExprFStrings;
    FancyStrList         EvalFStrings;
    Str8                 Description;
    Str8                 ErrorTooltip;
    Str8                 InheritanceTooltip;
    AppViewUIRule*       ViewUIRule;
};

struct AppWatchRowExtrasDrawData {
    b32 BreaksFromPrev;
};

struct AppSchemaIRExt {
    ConfigNode*   Config;
    MDNodePtrList Schemas;
};

struct AppSchemaExpandAccel {
    Str8Array Commands;
    MDNode**  Children;
    u64       ChildrenCount;
};

struct AppConfigsIRExt {
    Str8               ConfigName;
    Str8Array          Commands;
    ConfigNodePtrArray Configs;
    r1u64              CommandIndexRange;
    r1u64              ConfigsIndexRange;
};

struct AppBitmapTopology {
    v2i16        Dimensions;
    RendTex2DFmt Fmt;
};

struct AppBitmapBoxDrawData {
    r2f32      Src;
    RendHandle Texture;
    f32        LoadedT;
    b32        Hovered;
    v2i32      MousePX;
    f32        UIPerBMPPX;
};

struct AppBitmapCanvasBoxDrawData {
    v2f32 ViewCentrePosition;
    f32   Zoom;
};

struct AppEvalColour {
    v4f32    Colour;
    EvalEval ColourEvals[4];
};

struct AppGeo3DViewState {
    f32 Yaw;
    f32 Pitch;
    f32 Zoom;
};

struct AppGeo3DBoxDrawData {
    f32        Yaw;
    f32        Pitch;
    f32        Zoom;
    RendHandle VertBuffer;
    RendHandle IndexBuffer;
};

struct AppWatchesAccel {
    ConfigNodePtrArray Configs;
};

// @runtime____________________________________________________________________
extern AppState* APP_STATE;
extern ConfigID APP_LAST_DRAG_DROP_PANEL;
extern ConfigID APP_LAST_DRAG_DROP_PREV_TAB;

EXTERN_C_LINK_BEGIN
extern AppCommandKindInfo APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_COUNT];
extern BindingTableEntry APP_DEFAULT_BINDING_TABLE[85];
extern Str8 APP_BINDING_VERSION_REMAP_OLD_NAME_TABLE[2];
extern Str8 APP_BINDING_VERSION_REMAP_NEW_NAME_TABLE[2];
extern Str8 APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_COUNT];
extern Str8 APP_THEME_PRESET_DISPLAY_STR_TABLE[APP_THEME_PRESET_COUNT];
extern Str8 APP_THEME_PRESET_CODE_STR_TABLE[APP_THEME_PRESET_COUNT];
extern Str8 APP_THEME_PRESET_CONFIG_STR_TABLE[APP_THEME_PRESET_COUNT];
extern AppNameSchemaInfo APP_NAME_SCHEMA_INFO_TABLE[27];
extern Str8 APP_REGISTERS_SLOT_CODE_NAME_TABLE[APP_REGISTERS_SLOT_COUNT];
extern r1u64 APP_REGISTERS_SLOT_RANGE_TABLE[APP_REGISTERS_SLOT_COUNT];
extern Str8 APP_CODE_COLOUR_SLOT_NAME_TABLE[14];
extern Str8 APP_TAB_FAST_PATH_VIEW_NAME_TABLE[APP_COMMAND_KIND_COUNT - APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD];
extern Str8 APP_TAB_FAST_PATH_QUERY_NAME[APP_COMMAND_KIND_COUNT - APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD];
extern AppVocabInfo APP_VOCAB_INFO_TABLE[232];
extern const Str8 APP_ICON_FONT_BYTES;
extern const Str8 APP_DEFAULT_CODE_FONT_BYTES;
extern const Str8 APP_DEFAULT_MAIN_FONT_BYTES;
EXTERN_C_LINK_END

APP_VIEW_UI_FUNC_DEF(Null);

global readonly AppViewUIRule APP_NULL_VIEW_UI_RULE = {
    {},
    APP_VIEW_UI_FUNC_NAME(Null)
};

// @functions__________________________________________________________________
AppVocabInfo* AppVocabInfoFromCodeName(Str8 CodeName);
AppVocabInfo* AppVocabInfoFromCodeNamePlural(Str8 CodeName);
void ListPush(Arena* MemPool, ConfigIDList* List, ConfigID ID);
ConfigIDList ListCpy(Arena* MemPool, ConfigIDList* List);
void CopyRegisters(Arena* MemPool, AppRegisters* Dst, AppRegisters* Src);
AppRegisters* CopyRegisters(Arena* MemPool, AppRegisters* Regs);
void ListPush(Arena* MemPool, AppCommandList* List, Str8 Name, AppRegisters* Regs);
b32 AppDragIsActive(void);
void AppDragBegin(AppRegistersSlot Slot);
b32 AppDragDrop(void);
void AppDragKill(void);
void AppSetHoverRegisters(AppRegistersSlot Slot);
AppRegisters* AppGetHoverRegisters(void);
// u64 AppNameBucketNumberFromStrSize(u64 Size);
// Str8 AppNameAlloc(Str8 String);
// void AppNameRelease(Str8 String);
b32 AppIsProjectFiltered(ConfigNode* Config);
v4f32 AppHSVAFromConfig(ConfigNode* Config);
v4f32 AppColourFromConfig(ConfigNode* Config);
b32 AppIsDisabled(ConfigNode* Config);
Str8 LabelFromConfig(ConfigNode* Config);
Str8 ExpressionFromConfig(ConfigNode* Config);
Str8 PathFromConfig(ConfigNode* Config);
MDNodePtrList AppSchemasFromName(Str8 Name);
Str8 AppDefaultSettingFromNames(Str8 SchemaName, Str8 SettingName);
Str8 AppSettingFromNameStr8(Str8 Name);
b32 AppSettingFromNameB32(Str8 Name);
u64 AppSettingFromNameU64(Str8 Name);
f32 AppSettingFromNameF32(Str8 Name);
ConfigNode* AppImmediateConfigFromKey(Str8 String);
ConfigNode* AppImmediateConfigFromKey(char* Fmt, ...);
// Str8 AppMappedFromFilePath(Arena* MemPool, Str8 FilePath);
Str8List AppPossibleOverridesFromFilePath(Arena* MemPool, Str8 FilePath);
AppViewState* AppViewStateFromConfig(ConfigNode* Config);
extern void AppViewUI(r2f32 Rect);
Arena* AppViewMemPool(void);
UIScrollPoint2D AppViewScrollPosition(void);
Str8 AppViewQueryCommand(void);
Str8 AppViewQueryInput(void);
Str8 AppViewSettingFromNameStr8(Str8 Name);
b32 AppViewSettingFromNameb32(Str8 Name);
u64 AppViewSettingFromNameU64(Str8 Name);
f32 AppViewSettingFromNameF32(Str8 Name);
void* AppViewStateFromSize(u64 Size);
Arena* AppPushViewMemPool(void);
void AppStoreViewExprStr(Str8 String);
Str8 AppEvalStrFromFilePath(Arena* MemPool, Str8 String);
void AppStoreViewLoadingInfo(b32 IsLoading, u64 Progress, u64 ProgressTarget);
void AppStoreViewScrollPosition(UIScrollPoint2D Position);
void AppStoreViewParameter(Str8 Key, Str8 Value);
void AppStoreViewParameter(Str8 Key, char* Fmt, ...);
Str8 AppPushWindowTitle(Arena* MemPool);
ConfigNode* AppWindowFromConfig(ConfigNode* Config);
AppWindowState* AppWindowStateFromConfig(ConfigNode* Config);
AppWindowState* AppWindowStateFromHandle(Handle WinHandle);
void AppWindowFrame(void);
f32 AppFontSize(void);
FontTag AppFontFromSlot(AppFontSlot Slot);
FontRasterKind AppRasterKindFromSlot(AppFontSlot Slot);
void AppRequestFrame(void);
Arena* AppFrameMemPool(void);
void AppRegistersFillSlotFromStr(AppRegistersSlot Slot, Str8 QueryExpr, Str8 String);
AppCommandKind AppCommandKindFromStr(Str8 String);
AppCommandKindInfo* AppCommandKindInfoFromStr(Str8 String);
void AppPushCmd(Str8 Name, AppRegisters* Regs);
b32 AppNextCommand(AppCommand** Command);
b32 AppNextViewCommand(AppCommand** Command);
void AppInit(CommandLine* CLI);
void AppFrame(void);
AppRegisters* __AppPushRegisters(AppRegisters* Regs);
AppRegisters* AppPopRegisters(void);
void ListPush(Arena* MemPool, ConfigNodePtrList* List, ConfigNode* Config);
// AppQueryExpression AppParseQueryExpression(Str8 Expression);
// Str8 AppQueryNameFromExpression(Str8 Expression);
void ListPush(Arena* MemPool, AppQueryResultList* List, AppQueryResult Entry);
// AppQueryResultList AppQueryResultsFromExpression(Arena* MemPool, Str8 Expression);
// AppQueryResultList AppQueryResultsFiltered(Arena* MemPool, AppQueryResultList* Source, Str8 Filter);
// b32 AppQueryResultMatchesFilter(AppQueryResult* Entry, Str8 Filter);
// void AppQueryListerUI(ConfigNode* View);
// void AppQueryInputUI(ConfigNode* View);
// void AppQueryViewUI(r2f32 Rect);
Str8 AppExpressionFromConfig(ConfigNode* Cfg);
void AppCommandBindingButtons(Str8 Name, Str8 Filter, u64 Limit, AppCommandBindingButtonFlag Flags);
UISignal AppCell(AppCellParams* Params, Str8 String);
UISignal AppCell(AppCellParams* Params, char* Fmt, ...);
b32 AppViewWatchRowIsExpanded(AppViewWatchState* WatchState, u64 Key);
void AppViewWatchRowSetExpanded(AppViewWatchState* WatchState, u64 Key, b32 IsExpanded);
ConfigNode* AppConfigFromEvalSpace(EvalSpace Space);
EvalSpace AppEvalSpaceFromConfig(ConfigNode* Cfg);
Str8 AppCommandNameFromEval(EvalEval Eval);
u64 AppEvalSpaceGen(EvalSpace Space);
b32 AppEvalSpaceRead(EvalSpace Space, OUT void* Data, OUT EvalSpaceRangeInfo* RangeInfo, r1u64 Range);
b32 AppEvalSpaceWrite(EvalSpace Space, IN void* Data, r1u64 Range);
CKey AppKeyFromEvalSpaceRange(EvalSpace Space, r1u64 Range, b32 IsZeroTerminated);
r1u64 AppWholeRangeFromEvalSpace(EvalSpace Space);
b32 AppCommitEvalValueStr(EvalEval Eval, Str8 String);
Str8 AppFilePathFromEval(Arena* MemPool, EvalEval Eval);
Str8 AppFilePathFromEvalStr(Arena* MemPool, Str8 String);
Str8 AppEvalStrFromFilePath(Arena* MemPool, Str8 String);
Str8 AppQueryFromEvalStr(Arena* MemPool, Str8 String);
r1u64 AppSpaceRangeFromEval(EvalEval Eval);
Str8 AppLabelFromConfig(ConfigNode* Config);
EvalValue AppViewSettingValueFromName(Str8 String);
AppWatchRowCellInfo AppInfoFromWatchRowCell(Arena* MemPool, EVRow* Row, EVStringFlag Flags, AppWatchRowInfo* RowInfo, AppWatchCell* Cell, FontTag Font, f32 FontSize, f32 MaxSizePX);
AppWatchRowInfo AppWatchRowInfoFromRow(Arena* MemPool, EVRow* Row);
v2i64 AppTableFromWatchPoint(EVBlockRangeList* BlockRanges, AppWatchPoint Point);
AppWatchPoint AppWatchPointFromTable(EVBlockRangeList* BlockRanges, v2i64 Table);
u64 AppIDFromWatchCell(AppWatchCell* Cell);
AppWatchViewTextEditState* AppWatchViewTextEditStateFromPoint(AppWatchViewState* WV, AppWatchPoint Point);
EVView* AppViewEvalView(void);
AppViewUIRule* AppViewUIRuleFomStr(Str8 String);
FancyStrList AppTitleFStrFromConfig(Arena* MemPool, ConfigNode* Config, b32 IncludeExtras);
MDNode* AppThemeTreeFromName(Arena* MemPool, Access* Acc, Str8 Name);
AppWatchCell* __WatchCellPushNew(Arena* MemPool, AppWatchCellList* List, AppWatchCell* Params);
ConfigNode* AppViewFromEval(ConfigNode* Parent, EvalEval Eval);
void __AppSetAutoCompleteRegisters(EvalEval DstEval, AppRegisters* Regs);
void AppSetHoverEval(v2f32 Position, Str8 String);
AppRegistersSlot AppRegistersFillSlotFromCtxEval(EvalEval Eval);
AppRegistersSlot AppRegistersFillSlotFromCellCtx(Str8 RootCtxExpr, EVRow* Row);
UISignal AppIconButton(AppIconKind Kind, FMRangeList* Matches, Str8 String);
void AppCommandListMenuButtons(Str8* CommandNames, u64 CommandNamesCount, u32* FastPointCodePoints);

EVAL_TYPE_ACCESS_FUNC_DEF(Commands);
EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Commands);
EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Commands);

EVAL_TYPE_ACCESS_FUNC_DEF(Themes);
EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Themes);
EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Themes);

EVAL_TYPE_ACCESS_FUNC_DEF(Views);
EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Views);
EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Views);

EVAL_TYPE_IR_EXT_FUNC_DEF(Schema);
EVAL_TYPE_ACCESS_FUNC_DEF(Schema);
EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Schema);
EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Schema);

EVAL_TYPE_ACCESS_FUNC_DEF(Configs);
EVAL_TYPE_IR_EXT_FUNC_DEF(ConfigsSlice);
EVAL_TYPE_EXPAND_INFO_FUNC_DEF(ConfigsQuery);
EVAL_TYPE_ACCESS_FUNC_DEF(ConfigsSlice);
EVAL_TYPE_EXPAND_INFO_FUNC_DEF(ConfigsSlice);
EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(ConfigsSlice);
EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_DEF(ConfigsSlice);
EVAL_TYPE_EXPAND_NUM_FROM_ID_FUNC_DEF(ConfigsSlice);

EV_EXPAND_RULE_INFO_FUNC_DEF(Text);
EV_EXPAND_RULE_INFO_FUNC_DEF(Bitmap);
EV_EXPAND_RULE_INFO_FUNC_DEF(Colour);
EV_EXPAND_RULE_INFO_FUNC_DEF(Geo3D);

APP_VIEW_UI_FUNC_DEF(Text);
APP_VIEW_UI_FUNC_DEF(Bitmap);
APP_VIEW_UI_FUNC_DEF(Colour);
APP_VIEW_UI_FUNC_DEF(Geo3D);

#if defined(NC_LANG_CPP)
    b32 operator==(MDToken TokenA, MDToken TokenB);
    b32 operator!=(MDToken TokenA, MDToken TokenB);

    INTERNAL b32 
    operator==(const AppWatchPoint& PointA, const AppWatchPoint& PointB)
    {
        return (
            PointA.ParentKey == PointB.ParentKey &&
            PointA.Key == PointB.Key &&
            PointA.CellID == PointB.CellID
        );
    }

    INTERNAL b32 
    operator!=(const AppWatchPoint& PointA, const AppWatchPoint& PointB)
    {
        return !(PointA == PointB);
    }
#endif

#endif // __NC_APPLICATION_H__
