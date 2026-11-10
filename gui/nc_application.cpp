#include <stdarg.h>

#include "nc_application.h"
#include "nc_arena.h"
#include "nc_atomics.h"
#include "nc_config.h"
#include "nc_content.h"
#include "nc_defines.h"
#include "nc_eval.h"
#include "nc_file.h"
#include "nc_math.h"
#include "nc_metadesk.h"
#include "nc_render.h"
#include "nc_string.h"
#include "nc_system.h"
#include "nc_thread.h"
#include "nc_tls.h"
#include "nc_ui.h"
#include "nc_ui_components.h"
#include "nc_file_stream.h"
#include "fonts/nc_fonts.h"
#include "nc_profile.h"
#include "nc_object_cache.h"
#include "nc_draw.h"
#include "nc_window.h"
#include "nc_log.h"

#if !defined(NC_DATA_DIR)
    #define NC_DATA_DIR "nc_gui"
#endif

#if !defined(STB_IMAGE_IMPLEMENTATION)
    #define STBI_NO_STDIO
    #define STB_IMAGE_IMPLEMENTATION
    #include "stb_image.h"
#endif

// FIXME(nc): Memory leak in all floating views, when open we leak memory like
// FIXME    : a motherfucker (~2 MB chunks at a time)

// TODO(nc): Change CAD references and names to Diagram, CAD view is okay but
// TODO    : within the running application it should be refered to as Diagram

// TODO(nc): add canvas, cad, diagram, component, connection p&id colours to 
// TODO    : to themes so users can customise and colours are not hard coded

AppViewState              __EMPTY_APP_VIEW_STATE_VALUE;
AppViewState* const       EMPTY_APP_VIEW_STATE_VALUE            = &__EMPTY_APP_VIEW_STATE_VALUE;
AppWindowState            __EMPTY_APP_WINDOW_STATE_VALUE;
AppWindowState* const     EMPTY_APP_WINDOW_STATE_VALUE          = &__EMPTY_APP_WINDOW_STATE_VALUE;
AppCommandKindInfo        __EMPTY_APP_COMMAND_KIND_INFO_VALUE;
AppCommandKindInfo* const EMPTY_APP_COMMAND_KIND_INFO_VALUE     = &__EMPTY_APP_COMMAND_KIND_INFO_VALUE;
AppVocabInfo              __EMPTY_APP_VOCAB_INFO_VALUE;
AppVocabInfo* const       EMPTY_APP_VOCAB_INFO_VALUE            = &__EMPTY_APP_VOCAB_INFO_VALUE;
AppViewUIRule             __EMPTY_APP_VIEW_UI_RULE_VALUE;
AppViewUIRule* const      EMPTY_APP_VIEW_UI_RULE_VALUE          = &__EMPTY_APP_VIEW_UI_RULE_VALUE;
AppState*                 APP_STATE                             = NULL;
ConfigID                  APP_LAST_DRAG_DROP_PANEL              = 0;
ConfigID                  APP_LAST_DRAG_DROP_PREV_TAB           = 0;

EXTERN_C_LINK_BEGIN
AppCommandKindInfo APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_COUNT] = {
    {},
    {
        Str8Lit("exit"),
        Str8Lit("Exits the application."),
        Str8Lit("quit,close,end,abort"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("open_palette"), 
        Str8Lit("Opens the palette."), 
        Str8Lit("help,cmd,lister"), 
        Str8Lit(""), 
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("run_command"), 
        Str8Lit("Runs a command from the command palette."), 
        Str8Lit("help,cmd"), 
        Str8Lit(""), 
        0, 
        {
            APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED, 
            APP_REGISTERS_SLOT_COMMAND_NAME, 
            Str8Lit("query:commands")
        }
    },
    {
        Str8Lit("state"),
        Str8Lit(""),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            0,
            APP_REGISTERS_SLOT_NULL,
            Str8Lit("")
        }
    },
    {
        Str8Lit("eval"),
        Str8Lit(""),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            0,
            APP_REGISTERS_SLOT_EXPRESSION,
            Str8Lit("")
        }
    },
    {
        Str8Lit("os_event")
    },
    {
        Str8Lit("inc_window_font_size"),
        Str8Lit("Increases the window's font size by one point."),
        Str8Lit(""),
        Str8Lit(""),
        0, // APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            0,
            APP_REGISTERS_SLOT_NULL,
            Str8Lit("")
        }
    },
    {
        Str8Lit("dec_window_font_size"),
        Str8Lit("Decreases the window's font size by one point."),
        Str8Lit(""),
        Str8Lit(""),
        0, // APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            0,
            APP_REGISTERS_SLOT_NULL,
            Str8Lit("")
        }
    },
    {
        Str8Lit("open_window"),
        Str8Lit("Opens a new window."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("window_settings"),
        Str8Lit("Opens settings for a window."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("close_window"),
        Str8Lit("Closes an opened window."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("toggle_fullscreen"),
        Str8Lit("Toggles fullscreen view on the active window."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("bring_to_front"),
        Str8Lit("Brings all windows to the front, and focuses the most recently focused window."),
        Str8Lit("top")
    },
    {
        Str8Lit("popup_accept"),
        Str8Lit("Accepts the active popup prompt.")
    },
    {
        Str8Lit("popup_cancel"),
        Str8Lit("Cancels the active popup prompt.")
    },
    {
        Str8Lit("reset_to_default_bindings"),
        Str8Lit("Resets all keybindings to their defaults."),
        Str8Lit(""),
        Str8Lit(""),
        0, // APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("reset_to_default_panels"),
        Str8Lit("Resets the window to the default panel layout."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("reset_to_compact_panels"),
        Str8Lit("Resets the window to the compact panel layout."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("reset_to_simple_panels"),
        Str8Lit("Resets the window to the simple panel layout."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("new_panel_left"),
        Str8Lit("Creates a new panel to the left of the active panel."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("new_panel_up"),
        Str8Lit("Creates a new panel at the top of the active panel."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("new_panel_right"),
        Str8Lit("Creates a new panel to the right of the active panel."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("new_panel_down"),
        Str8Lit("Creates a new panel at the bottom of the active panel."),
        Str8Lit("panel"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("split_panel"),
        Str8Lit("Creates a new panel in a given direction, and moves a tab to it, if specified.")
    },
    {
        Str8Lit("rotate_panel_columns"),
        Str8Lit("Rotates all panels at the closest column level of the panel hierarchy."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("next_panel"),
        Str8Lit("Cycles the active panel forward."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("prev_panel"),
        Str8Lit("Cycles the active panel backwards."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("focus_panel"),
        Str8Lit("Focuses a new panel.")
    },
    {
        Str8Lit("focus_panel_right"),
        Str8Lit("Focuses a panel rightward of the currently focused panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("focus_panel_left"),
        Str8Lit("Focuses a panel leftward of the currently focused panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("focus_panel_up"),
        Str8Lit("Focuses a panel upward of the currently focused panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("focus_panel_down"),
        Str8Lit("Focuses a panel downward of the currently focused panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("undo"),
        Str8Lit("Undoes the previous action.")
    },
    {
        Str8Lit("redo"),
        Str8Lit("Redoes the first previously undone action.")
    },
    {
        Str8Lit("go_back"),
        Str8Lit("Returns to the previously selected panel and tab in recorded history.")
    },
    {
        Str8Lit("go_forward"),
        Str8Lit("Returns to the next selected panel and tab in recorded history.")
    },
    {
        Str8Lit("close_panel"),
        Str8Lit("Closes the currently active panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("focus_tab"),
        Str8Lit("Focuses the passed tab within its containing panel.")
    },
    {
        Str8Lit("next_tab"),
        Str8Lit("Focuses the next tab on the active panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("prev_tab"),
        Str8Lit("Focuses the previous tab on the active panel."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_tab_right"),
        Str8Lit("Moves the selected tab right one slot."),
        Str8Lit(""),
        Str8Lit("$tab,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_tab_left"),
        Str8Lit("Moves the selected tab left one slot."),
        Str8Lit(""),
        Str8Lit("$tab,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("open_tab"),
        Str8Lit("Opens a new tab."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_COMMAND_NAME,
            Str8Lit("query:tab_commands"),
            Str8Lit("commands")
        }
    },
    {
        Str8Lit("build_tab"),
        Str8Lit("Opens a new tab with the parameterized view specification.")
    },
    {
        Str8Lit("duplicate_tab"),
        Str8Lit("Duplicates a tab."),
        Str8Lit(""),
        Str8Lit("$tab,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("copy_tab_full_path"),
        Str8Lit("Copies the full path of the file being viewed by this tab."),
        Str8Lit(""),
        Str8Lit("$tab,")
    },
    {
        Str8Lit("close_tab"),
        Str8Lit("Closes the currently opened tab."),
        Str8Lit(""),
        Str8Lit("$tab,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_view"),
        Str8Lit("Moves a view to a new panel.")
    },
    {
        Str8Lit("tab_bar_top"),
        Str8Lit("Anchors a panel's tab bar to the top of the panel."),
        Str8Lit(""),
        Str8Lit("$tab,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("tab_bar_bottom"),
        Str8Lit("Anchors a panel's tab bar to the bottom of the panel."),
        Str8Lit(""),
        Str8Lit("$tab,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("tab_settings"),
        Str8Lit("Opens settings for a tab."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("set_current_path"),
        Str8Lit("Sets the application's current path, which is used as a starting point when browsing for files.")
    },
    {
        Str8Lit("open"),
        Str8Lit("Opens a file."),
        Str8Lit("code,source,file"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("switch"),
        Str8Lit("Switches to a recent file."),
        Str8Lit("code,source,file"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_CONFIG,
            Str8Lit("query:recent_files, query:source_files")
        }
    },
    {
        Str8Lit("show_file_in_explorer"),
        Str8Lit("Opens the operating system's file explorer and shows the selected file."),
        Str8Lit(""),
        Str8Lit("$file"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("new_session"),
        Str8Lit("Creates a new session file, and sets the current session path as that file's path."),
        Str8Lit("new,session,layout"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("open_session"),
        Str8Lit("Opens a session file path, immediately loading it, and begins autosaving to it."),
        Str8Lit("session"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("open_recent_session"),
        Str8Lit("Opens a recently used session file."),
        Str8Lit("session"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_CONFIG,
            Str8Lit("query:recent_sessions")
        }
    },
    {
        Str8Lit("save_session"),
        Str8Lit("Saves session data to a file, and sets the current session path as that path."),
        Str8Lit("session"),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("write_session_data"),
        Str8Lit("Writes session data to the active session file.")
    },
    {
        Str8Lit("session_settings"),
        Str8Lit("Opens session settings."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("edit"),
        Str8Lit("Edits the current selection."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("accept"),
        Str8Lit("Accepts current changes, or answers prompts in the affirmative."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("cancel"),
        Str8Lit("Rejects current changes, exits temporary menus, or answers prompts in the negative."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_left"),
        Str8Lit("Moves the cursor or selection left."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_right"),
        Str8Lit("Moves the cursor or selection right."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up"),
        Str8Lit("Moves the cursor or selection up."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down"),
        Str8Lit("Moves the cursor or selection down."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_left_select"),
        Str8Lit("Moves the cursor or selection left, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_right_select"),
        Str8Lit("Moves the cursor or selection right, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_select"),
        Str8Lit("Moves the cursor or selection up, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_select"),
        Str8Lit("Moves the cursor or selection down, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_left_chunk"),
        Str8Lit("Moves the cursor or selection left one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_right_chunk"),
        Str8Lit("Moves the cursor or selection right one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_chunk"),
        Str8Lit("Moves the cursor or selection up one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_chunk"),
        Str8Lit("Moves the cursor or selection down one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_left_chunk_select"),
        Str8Lit("Moves the cursor or selection left one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_right_chunk_select"),
        Str8Lit("Moves the cursor or selection right one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_chunk_select"),
        Str8Lit("Moves the cursor or selection up one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_chunk_select"),
        Str8Lit("Moves the cursor or selection down one chunk."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_page"),
        Str8Lit("Moves the cursor or selection up one page."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_page"),
        Str8Lit("Moves the cursor or selection down one page."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_whole"),
        Str8Lit("Moves the cursor or selection to the beginning of the relevant content."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_whole"),
        Str8Lit("Moves the cursor or selection to the end of the relevant content."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_page_select"),
        Str8Lit("Moves the cursor or selection up one page, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_page_select"),
        Str8Lit("Moves the cursor or selection down one page, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_whole_select"),
        Str8Lit("Moves the cursor or selection to the beginning of the relevant content, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_whole_select"),
        Str8Lit("Moves the cursor or selection to the end of the relevant content, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_up_reorder"),
        Str8Lit("Moves the cursor or selection up, while swapping the currently selected element with that upward."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_down_reorder"),
        Str8Lit("Moves the cursor or selection down, while swapping the currently selected element with that downward."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_home"),
        Str8Lit("Moves the cursor to the beginning of the line."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_end"),
        Str8Lit("Moves the cursor to the end of the line."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_home_select"),
        Str8Lit("Moves the cursor to the beginning of the line, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_end_select"),
        Str8Lit("Moves the cursor to the end of the line, while selecting."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("select_all"),
        Str8Lit("Selects everything possible."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("delete_single"),
        Str8Lit("Deletes a single element to the right of the cursor, or the active selection."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("delete_chunk"),
        Str8Lit("Deletes a chunk to the right of the cursor, or the active selection."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("backspace_single"),
        Str8Lit("Deletes a single element to the left of the cursor, or the active selection."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("backspace_chunk"),
        Str8Lit("Deletes a chunk to the left of the cursor, or the active selection."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("copy"),
        Str8Lit("Copies the active selection to the clipboard."),
        Str8Lit(""),
        Str8Lit("$text_rng,"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI | APP_COMMAND_KIND_FLAG_LIST_IN_TEXT_RANGE
    },
    {
        Str8Lit("cut"),
        Str8Lit("Copies the active selection to the clipboard, then deletes it."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("paste"),
        Str8Lit("Pastes the current contents of the clipboard."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("insert_text"),
        Str8Lit("Inserts the text that was used to cause this command.")
    },
    {
        Str8Lit("move_next"),
        Str8Lit("Moves the cursor or selection to the next element."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("move_prev"),
        Str8Lit("Moves the cursor or selection to the previous element."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("centre_cursor"),
        Str8Lit("Snaps the current code view to centre the cursor."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("contain_cursor"),
        Str8Lit("Snaps the current code view to contain the cursor."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("find_next"),
        Str8Lit("Searches the current code file forward (from the cursor) for the last searched string."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_KEEP_OLD_INPUT,
            APP_REGISTERS_SLOT_NULL
        }
    },
    {
        Str8Lit("find_prev"),
        Str8Lit("Searches the current code file backwards (from the cursor) for the last searched string."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI,
        {
            APP_QUERY_KIND_KEEP_OLD_INPUT,
            APP_REGISTERS_SLOT_NULL
        }
    },
    {
        Str8Lit("enable_cfg"),
        Str8Lit("Enables a config tree.")
    },
    {
        Str8Lit("disable_cfg"),
        Str8Lit("Disables a config tree.")
    },
    {
        Str8Lit("select_cfg"),
        Str8Lit("Selects a config tree, disabling all others of the same kind.")
    },
    {
        Str8Lit("deselect_cfg"),
        Str8Lit("Deselects a config tree.")
    },
    {
        Str8Lit("remove_cfg"),
        Str8Lit("Removes a config tree.")
    },
    {
        Str8Lit("name_cfg"),
        Str8Lit("Equips a config tree with a label.")
    },
    {
        Str8Lit("duplicate_cfg"),
        Str8Lit("Duplicates a config tree.")
    },
    {
        Str8Lit("save_cfg_to_session"),
        Str8Lit("Saves the config tree to the session.")
    },
    {
        Str8Lit("edit_session_theme"),
        Str8Lit("Edits the current session theme."),
        Str8Lit("colour"),
        Str8Lit(""),
        0,
        {
            0,
            APP_REGISTERS_SLOT_STRING,
            Str8Lit("query:themes")
        }
    },
    {
        Str8Lit("add_theme_colour"),
        Str8Lit("Adds a new theme colour")
    },
    {
        Str8Lit("fork_theme"),
        Str8Lit("Imports all colours from the current theme so they can be individually edited")
    },
    {
        Str8Lit("save_theme"),
        Str8Lit("Saves all theme colours to a new theme file."),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_STRING
        }
    },
    {
        Str8Lit("save_and_set_theme"),
        Str8Lit("Saves all theme colours to a new theme file, and then sets that theme as the selected theme."),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_STRING
        }
    },
    {
        Str8Lit("pick_file"),
        Str8Lit("Opens the file browser to pick a file."),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("pick_folder"),
        Str8Lit("Opens the file browser to pick a folder."),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            APP_QUERY_KIND_ALLOW_FOLDERS | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("pick_file_or_folder"),
        Str8Lit("Opens the file browser to pick a file or folder."),
        Str8Lit(""),
        Str8Lit(""),
        0,
        {
            APP_QUERY_KIND_ALLOW_FILES | APP_QUERY_KIND_ALLOW_FOLDERS | APP_QUERY_KIND_FLOATING | APP_QUERY_KIND_REQUIRED,
            APP_REGISTERS_SLOT_FILE_PATH,
            Str8Lit("folder:\"$input\"")
        }
    },
    {
        Str8Lit("toggle_dev_menu"),
        Str8Lit("Opens and closes the developer menu."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("record_session_as_last_opened"),
        Str8Lit("Records a file path as the last opened session.")
    },
    {
        Str8Lit("record_recent_session"),
        Str8Lit("Records a file path as a recent session.")
    },
    {
        Str8Lit("complete_query"),
        Str8Lit("Completes and closes a query")
    },
    {
        Str8Lit("push_query"),
        Str8Lit("Opens a new temporary query interface")
    },
    {
        Str8Lit("inc_view_font_size"),
        Str8Lit("Increases the view's font size by one point."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("dec_view_font_size"),
        Str8Lit("Decreases the view's font size by one point."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("record_file_in_session"),
        Str8Lit("Records the passed file path as a recent file in the currently loaded session")
    },
    {
        Str8Lit("query"),
        Str8Lit("Submits the active query")
    },
    {
        Str8Lit("cancel_query"),
        Str8Lit("Cancels the active query")
    },
    {
        Str8Lit("update_query"),
        Str8Lit("Updates the query input buffer")
    },
    {
        Str8Lit("watches"),
        Str8Lit("Opens a Watch tab."),
        Str8Lit(""),
        Str8Lit("{tab_commands}"),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    { 
        Str8Lit("file_path_maps"), 
        Str8Lit("Opens a File Path Map tab."), 
        Str8Lit(""), 
        Str8Lit("{tab_commands}"), 
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    { 
        Str8Lit("type_views"), 
        Str8Lit("Opens a Type Views tab."), 
        Str8Lit(""), 
        Str8Lit("{tab_commands}"), 
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    {
        Str8Lit("text"),
        Str8Lit("Opens a Text tab."),
        Str8Lit(""),
        Str8Lit(""),
        APP_COMMAND_KIND_FLAG_LIST_IN_UI | APP_COMMAND_KIND_FLAG_LIST_IN_TAB
    },
    { 
        Str8Lit("bitmap"), 
        Str8Lit("Opens a Bitmap tab."), 
        Str8Lit(""), 
        Str8Lit("{tab_commands}"), 
        APP_COMMAND_KIND_FLAG_LIST_IN_UI | APP_COMMAND_KIND_FLAG_LIST_IN_TAB
    },
    { 
        Str8Lit("colour"), 
        Str8Lit("Opens a Colour tab."), 
        Str8Lit(""), 
        Str8Lit("{tab_commands}"), 
        APP_COMMAND_KIND_FLAG_LIST_IN_UI
    },
    { 
        Str8Lit("geo3d"), 
        Str8Lit("Opens a Geometry (3D) tab."), 
        Str8Lit(""), 
        Str8Lit("{tab_commands}"), 
        APP_COMMAND_KIND_FLAG_LIST_IN_UI | APP_COMMAND_KIND_FLAG_LIST_IN_TAB
    }
};

BindingTableEntry APP_DEFAULT_BINDING_TABLE[85] = {
    {
        Str8Lit("inc_window_font_size"),
        {
            INPUT_KIND_EQUAL,
            INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("dec_window_font_size"),
        {
            INPUT_KIND_MINUS,
            INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("window"),
        {
            INPUT_KIND_N,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("toggle_fullscreen"),
        {
            INPUT_KIND_RETURN,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("new_panel_right"),
        {
            INPUT_KIND_P,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("new_panel_down"),
        {
            INPUT_KIND_MINUS,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("rotate_panel_columns"),
        {
            INPUT_KIND_2,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("next_panel"),
        {
            INPUT_KIND_COMMA,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("prev_panel"),
        {
            INPUT_KIND_COMMA,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("focus_panel_right"),
        {
            INPUT_KIND_RIGHT,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("focus_panel_left"),
        {
            INPUT_KIND_LEFT,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("focus_panel_up"),
        {
            INPUT_KIND_UP,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("focus_panel_down"),
        {
            INPUT_KIND_DOWN,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("undo"),
        {
            INPUT_KIND_Z,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("redo"),
        {
            INPUT_KIND_Y,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("go_back"),
        {
            INPUT_KIND_LEFT,
            INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("go_forward"),
        {
            INPUT_KIND_RIGHT,
            INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("close_panel"),
        {
            INPUT_KIND_P,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_ALT | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("next_tab"),
        {
            INPUT_KIND_PAGEDOWN,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("prev_tab"),
        {
            INPUT_KIND_PAGEUP,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_tab_right"),
        {
            INPUT_KIND_PAGEDOWN,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_tab_left"),
        {
            INPUT_KIND_PAGEUP,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("close_tab"),
        {
            INPUT_KIND_W,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("tab_bar_top"),
        {
            INPUT_KIND_UP,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("tab_bar_bottom"),
        {
            INPUT_KIND_DOWN,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("open_tab"),
        {
            INPUT_KIND_T,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("open"),
        {
            INPUT_KIND_O,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("switch"),
        {
            INPUT_KIND_I,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("open_session"),
        {
            INPUT_KIND_RETURN,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("save_session"),
        {
            INPUT_KIND_S,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("edit"),
        {
            INPUT_KIND_FUNC_2,
            0
        }
    },
    {
        Str8Lit("accept"),
        {
            INPUT_KIND_RETURN,
            0
        }
    },
    {
        Str8Lit("accept"),
        {
            INPUT_KIND_SPACE,
            0
        }
    },
    {
        Str8Lit("cancel"),
        {
            INPUT_KIND_ESC,
            0
        }
    },
    {
        Str8Lit("move_left"),
        {
            INPUT_KIND_LEFT,
            0
        }
    },
    {
        Str8Lit("move_right"),
        {
            INPUT_KIND_RIGHT,
            0
        }
    },
    {
        Str8Lit("move_up"),
        {
            INPUT_KIND_UP,
            0
        }
    },
    {
        Str8Lit("move_down"),
        {
            INPUT_KIND_DOWN,
            0
        }
    },
    {
        Str8Lit("move_left_select"),
        {
            INPUT_KIND_LEFT,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_right_select"),
        {
            INPUT_KIND_RIGHT,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_up_select"),
        {
            INPUT_KIND_UP,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_down_select"),
        {
            INPUT_KIND_DOWN,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_left_chunk"),
        {
            INPUT_KIND_LEFT,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_right_chunk"),
        {
            INPUT_KIND_RIGHT,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_up_chunk"),
        {
            INPUT_KIND_UP,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_down_chunk"),
        {
            INPUT_KIND_DOWN,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_left_chunk_select"),
        {
            INPUT_KIND_LEFT,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_right_chunk_select"),
        {
            INPUT_KIND_RIGHT,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_up_chunk_select"),
        {
            INPUT_KIND_UP,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_down_chunk_select"),
        {
            INPUT_KIND_DOWN,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_up_page"),
        {
            INPUT_KIND_PAGEUP,
            0
        }
    },
    {
        Str8Lit("move_down_page"),
        {
            INPUT_KIND_PAGEDOWN,
            0
        }
    },
    {
        Str8Lit("move_up_whole"),
        {
            INPUT_KIND_HOME,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_down_whole"),
        {
            INPUT_KIND_END,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("move_up_page_select"),
        {
            INPUT_KIND_PAGEUP,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_down_page_select"),
        {
            INPUT_KIND_PAGEDOWN,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_up_whole_select"),
        {
            INPUT_KIND_HOME,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_down_whole_select"),
        {
            INPUT_KIND_END,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_up_reorder"),
        {
            INPUT_KIND_UP,
            INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("move_down_reorder"),
        {
            INPUT_KIND_DOWN,
            INPUT_MOD_KIND_ALT
        }
    },
    {
        Str8Lit("move_home"),
        {
            INPUT_KIND_HOME,
            0
        }
    },
    {
        Str8Lit("move_end"),
        {
            INPUT_KIND_END,
            0
        }
    },
    {
        Str8Lit("move_home_select"),
        {
            INPUT_KIND_HOME,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("move_end_select"),
        {
            INPUT_KIND_END,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("select_all"),
        {
            INPUT_KIND_A,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("delete_single"),
        {
            INPUT_KIND_DELETE,
            0
        }
    },
    {
        Str8Lit("delete_chunk"),
        {
            INPUT_KIND_DELETE,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("backspace_single"),
        {
            INPUT_KIND_BACKSPACE,
            0
        }
    },
    {
        Str8Lit("backspace_chunk"),
        {
            INPUT_KIND_BACKSPACE,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("copy"),
        {
            INPUT_KIND_C,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("copy"),
        {
            INPUT_KIND_INSERT,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("cut"),
        {
            INPUT_KIND_X,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("paste"),
        {
            INPUT_KIND_V,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("paste"),
        {
            INPUT_KIND_INSERT,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("insert_text"),
        {
            INPUT_KIND_NULL,
            0
        }
    },
    {
        Str8Lit("move_next"),
        {
            INPUT_KIND_TAB,
            0
        }
    },
    {
        Str8Lit("move_prev"),
        {
            INPUT_KIND_TAB,
            INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("goto_line"),
        {
            INPUT_KIND_G,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("search"),
        {
            INPUT_KIND_F,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("search_backwards"),
        {
            INPUT_KIND_R,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("find_next"),
        {
            INPUT_KIND_FUNC_3,
            0
        }
    },
    {
        Str8Lit("find_prev"),
        {
            INPUT_KIND_FUNC_3,
            INPUT_MOD_KIND_CTRL
        }
    },
    {
        Str8Lit("open_palette"),
        {
            INPUT_KIND_FUNC_1,
            0
        }
    },
    {
        Str8Lit("open_palette"),
        {
            INPUT_KIND_P,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT
        }
    },
    {
        Str8Lit("toggle_dev_menu"),
        {
            INPUT_KIND_D,
            INPUT_MOD_KIND_CTRL | INPUT_MOD_KIND_SHIFT | INPUT_MOD_KIND_ALT
        }
    }
};

Str8 APP_BINDING_VERSION_REMAP_OLD_NAME_TABLE[2] = {
    Str8Lit("commands"),
    Str8Lit("load_project")
};

Str8 APP_BINDING_VERSION_REMAP_NEW_NAME_TABLE[2] = {
    Str8Lit("run_command"),
    Str8Lit("open_session")
};

Str8 APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_COUNT] = {
    Str8Lit(""),
    Str8Lit("b"),
    Str8Lit("c"),
    Str8Lit("B"),
    Str8Lit("C"),
    Str8Lit("f"),
    Str8Lit("F"),
    Str8Lit("i"),
    Str8Lit("w"),
    Str8Lit("W"),
    Str8Lit("k"),
    Str8Lit("K"),
    Str8Lit("L"),
    Str8Lit("R"),
    Str8Lit("U"),
    Str8Lit("D"),
    Str8Lit("G"),
    Str8Lit("P"),
    Str8Lit("3"),
    Str8Lit("p"),
    Str8Lit("O"),
    Str8Lit("o"),
    Str8Lit("!"),
    Str8Lit("1"),
    Str8Lit("V"),
    Str8Lit("<"),
    Str8Lit(">"),
    Str8Lit("^"),
    Str8Lit("v"),
    Str8Lit("9"),
    Str8Lit("0"),
    Str8Lit("7"),
    Str8Lit("8"),
    Str8Lit("+"),
    Str8Lit("-"),
    Str8Lit("M"),
    Str8Lit("."),
    Str8Lit("x"),
    Str8Lit("q"),
    Str8Lit("j"),
    Str8Lit("u"),
    Str8Lit("m"),
    Str8Lit("n"),
    Str8Lit("a"),
    Str8Lit("X"),
    Str8Lit("Y"),
    Str8Lit("S"),
    Str8Lit("N"),
    Str8Lit("E"),
    Str8Lit("H"),
    Str8Lit("e"),
    Str8Lit("I"),
    Str8Lit("J"),
    Str8Lit("A"),
    Str8Lit("?"),
    Str8Lit("4"),
    Str8Lit("5"),
    Str8Lit("6"),
    Str8Lit("&"),
    Str8Lit("*"),
    Str8Lit("("),
    Str8Lit(")"),
    Str8Lit("#")
};

// TODO(nc): change to PID
Str8 APP_CODE_COLOUR_SLOT_NAME_TABLE[14] = {
    Str8Lit("code_default"),
    Str8Lit("code_symbol"),
    Str8Lit("code_type"),
    Str8Lit("code_local"),
    Str8Lit("code_register"),
    Str8Lit("code_keyword"),
    Str8Lit("code_delimiter_or_operator"),
    Str8Lit("code_numeric"),
    Str8Lit("code_numeric_alt_digit_group"),
    Str8Lit("code_string"),
    Str8Lit("code_meta"),
    Str8Lit("code_comment"),
    Str8Lit("code_line_numbers"),
    Str8Lit("code_line_numbers_selected")
};

Str8 APP_THEME_PRESET_DISPLAY_STR_TABLE[APP_THEME_PRESET_COUNT] = {
    Str8Lit("Default Dark"),
    Str8Lit("Default Light"),
    Str8Lit("VS Dark"),
    Str8Lit("VS Light"),
    Str8Lit("Solarized Dark"),
    Str8Lit("Solarized Light"),
    Str8Lit("Handmade Hero"),
    Str8Lit("Naysayer"),
    Str8Lit("4coder"),
    Str8Lit("Grove"),
    Str8Lit("Far Manager"),
    Str8Lit("Ayu Dark"),
    Str8Lit("Gruvbox Dark"),
    Str8Lit("Tokyo Night"),
    Str8Lit("Dracula"),
    Str8Lit("Nord"),
    Str8Lit("Catppuccin Mocha"),
    Str8Lit("Monokai")
};

Str8 APP_THEME_PRESET_CODE_STR_TABLE[APP_THEME_PRESET_COUNT] = {
    Str8Lit("default_dark"),
    Str8Lit("default_light"),
    Str8Lit("vs_dark"),
    Str8Lit("vs_light"),
    Str8Lit("solarized_dark"),
    Str8Lit("solarized_light"),
    Str8Lit("handmade_hero"),
    Str8Lit("naysayer"),
    Str8Lit("four_coder"),
    Str8Lit("grove"),
    Str8Lit("far_manager"),
    Str8Lit("ayu_dark"),
    Str8Lit("gruvbox_dark"),
    Str8Lit("tokyo_night"),
    Str8Lit("dracula"),
    Str8Lit("nord"),
    Str8Lit("catppuccin_mocha"),
    Str8Lit("monokai")
};

Str8 APP_THEME_PRESET_CONFIG_STR_TABLE[APP_THEME_PRESET_COUNT] = {
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x1b1b1bff}\n  theme_colour:{tags: \"alt background\", value: 0x222222ff}\n  theme_colour:{tags: \"pop background\", value: 0x355b6eff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour: {tags:\"match background\",                       value: 0x31393dff }\n  theme_colour: {tags:\"border\",                                 value: 0x404040ff }\n  theme_colour: {tags:\"text\",                                   value: 0xe5e5e5ff }\n  theme_colour: {tags:\"weak text\",                              value: 0xa4a4a4ff }\n  theme_colour: {tags:\"good text\",                              value: 0x32a852ff }\n  theme_colour: {tags:\"neutral text\",                           value: 0x3a90bbff }\n  theme_colour: {tags:\"bad text\",                               value: 0xcf5242ff }\n  theme_colour: {tags:\"hover\",                                  value: 0xffffffff }\n  theme_colour: {tags:\"focus overlay\",                          value: 0xfda20012 }\n  theme_colour: {tags:\"focus border\",                           value: 0xfda200ff }\n  theme_colour: {tags:\"cursor\",                                 value: 0x8aff00ff }\n  theme_colour: {tags:\"selection\",                              value: 0x99ccff0f }\n  theme_colour: {tags:\"inactive background\",                    value: 0x0000002f }\n  theme_colour: {tags:\"drop_shadow\",                            value: 0x0000007f }\n  theme_colour: {tags:\"good_pop background\",                    value: 0x2c5b36ff }\n  theme_colour: {tags:\"good_pop border\",                        value: 0x568761ff }\n  theme_colour: {tags:\"good_pop hover\",                         value: 0xe3f5d3ff }\n  theme_colour: {tags:\"good_pop weak text\",                     value: 0xe3f5d3ff }\n  theme_colour: {tags:\"bad_pop background\",                     value: 0x803425ff }\n  theme_colour: {tags:\"bad_pop hover\",                          value: 0xff825cff }\n  theme_colour: {tags:\"code_default\",                           value: 0xcbcbcbff }\n  theme_colour: {tags:\"code_symbol\",                            value: 0x42a2cfff }\n  theme_colour: {tags:\"code_type\",                              value: 0xfec746ff }\n  theme_colour: {tags:\"code_local\",                             value: 0x98bc80ff }\n  theme_colour: {tags:\"code_register\",                          value: 0xb7afd5ff }\n  theme_colour: {tags:\"code_keyword\",                           value: 0xb38d4cff }\n  theme_colour: {tags:\"code_delimiter_or_operator\",             value: 0x767676ff }\n  theme_colour: {tags:\"code_numeric\",                           value: 0x98abb1ff }\n  theme_colour: {tags:\"code_numeric_alt_digit_group\",           value: 0x738287ff }\n  theme_colour: {tags:\"code_string\",                            value: 0x98abb1ff }\n  theme_colour: {tags:\"code_meta\",                              value: 0xd96759ff }\n  theme_colour: {tags:\"code_comment\",                           value: 0x717171ff }\n  theme_colour: {tags:\"line_info_0\",                            value: 0x4f3022ff }\n  theme_colour: {tags:\"line_info_1\",                            value: 0x4f3e15ff }\n  theme_colour: {tags:\"line_info_2\",                            value: 0x434e2aff }\n  theme_colour: {tags:\"line_info_3\",                            value: 0x36241fff }\n  theme_colour: {tags:\"line_info_4\",                            value: 0x4f3022ff }\n  theme_colour: {tags:\"line_info_5\",                            value: 0x4f3e15ff }\n  theme_colour: {tags:\"line_info_6\",                            value: 0x434e2aff }\n  theme_colour: {tags:\"line_info_7\",                            value: 0x36241fff }\n  theme_colour: {tags:\"thread_0\",                               value: 0xffcb7fff }\n  theme_colour: {tags:\"thread_1\",                               value: 0xb2ff65ff }\n  theme_colour: {tags:\"thread_2\",                               value: 0xff99e5ff }\n  theme_colour: {tags:\"thread_3\",                               value: 0x6598ffff }\n  theme_colour: {tags:\"thread_4\",                               value: 0x65ffcbff }\n  theme_colour: {tags:\"thread_5\",                               value: 0xff9819ff }\n  theme_colour: {tags:\"thread_6\",                               value: 0x9932ffff }\n  theme_colour: {tags:\"thread_7\",                               value: 0x65ff4cff }\n  theme_colour: {tags:\"thread_unwound\",                         value: 0xb2ccd8ff }\n  theme_colour: {tags:\"thread_error\",                           value: 0xb23219ff }\n  theme_colour: {tags:\"breakpoint\",                             value: 0xa72911ff }\n  theme_colour: {tags:\"floating background\",                    value: 0x1b1b1baf }\n  theme_colour: {tags:\"floating background alt\",                value: 0x0000005f }\n  theme_colour: {tags:\"floating background fresh\",              value: 0x31393d5f }\n  theme_colour: {tags:\"floating border\",                        value: 0xbfbfbf1f }\n  theme_colour: {tags:\"floating scroll_bar background\",         value: 0x3b3b3b5f }\n  theme_colour: {tags:\"floating scroll_bar border\",             value: 0x5f5f5f5f }\n  theme_colour: {tags:\"menu_bar background\",                    value: 0x2b3740ff }\n  theme_colour: {tags:\"menu_bar border\",                        value: 0x3e4c57ff }\n  theme_colour: {tags:\"scroll_bar background\",                  value: 0x2b2b2bff }\n  theme_colour: {tags:\"scroll_bar border\",                      value: 0x3f3f3fff }\n  theme_colour: {tags:\"implicit background\",                    value: 0x00000000 }\n  theme_colour: {tags:\"implicit border\",                        value: 0x00000000 }\n  theme_colour: {tags:\"hollow background\",                      value: 0x00000000 }\n  theme_colour: {tags:\"hollow border\",                          value: 0xffffff1f }\n  theme_colour: {tags:\"tab background\",                         value: 0x6f5135ff }\n  theme_colour: {tags:\"tab border\",                             value: 0x8a6e54ff }\n  theme_colour: {tags:\"tab inactive background\",                value: 0x2b3740ff }\n  theme_colour: {tags:\"tab inactive border\",                    value: 0x3e4c57ff }\n  theme_colour: {tags:\"tab auto background\",                    value: 0x693847ff }\n  theme_colour: {tags:\"tab auto border\",                        value: 0x9e6274ff }\n  theme_colour: {tags:\"tab auto inactive background\",           value: 0x2f2633ff }\n  theme_colour: {tags:\"tab auto inactive border\",               value: 0x685073ff }\n  theme_colour: {tags:\"drop_site background\",                   value: 0xffffff05 }\n  theme_colour: {tags:\"drop_site border\",                       value: 0xffffff0f }\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0xffffffff}\n  theme_colour:{tags: \"alt background\", value: 0xf8f8f8ff}\n  theme_colour:{tags: \"pop background\", value: 0xcbe4f2ff}\n  theme_colour:{tags: \"menu_bar pop background\", value: 0x5aabd9ff}\n  theme_colour:{tags: \"fresh background\", value: 0xeaddceff}\n  theme_colour:{  tags: \"match background\"  , value: 0xc1e9c4ff }\n  theme_colour:{  tags: border  , value: 0xcbcbcbff }\n  theme_colour:{  tags: text  , value: 0xff }\n  theme_colour:{  tags: \"weak text\"  , value: 0x727272ff }\n  theme_colour:{  tags: \"good text\"  , value: 0x217538ff }\n  theme_colour:{  tags: \"neutral text\"  , value: 0x1a5b7cff }\n  theme_colour:{  tags: \"bad text\"  , value: 0x972717ff }\n  theme_colour:{  tags: hover  , value: 0xff }\n  theme_colour:{  tags: \"focus overlay\"  , value: 0x67ff4b }\n  theme_colour:{  tags: \"focus border\"  , value: 0x67ffff }\n  theme_colour:{  tags: cursor  , value: 0xff }\n  theme_colour:{  tags: selection  , value: 0x283d5166 }\n  theme_colour:{  tags: \"inactive background\"  , value: 0x8 }\n  theme_colour:{  tags: drop_shadow  , value: 0xb }\n  theme_colour:{  tags: \"good_pop background\"  , value: 0x90c09aff }\n  theme_colour:{  tags: \"good_pop border\"  , value: 0x1e7231ff }\n  theme_colour:{  tags: \"good_pop hover\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"good_pop weak text\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"good_pop text\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"bad_pop background\"  , value: 0xa93620ff }\n  theme_colour:{  tags: \"bad_pop text\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"bad_pop text weak\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"menu_bar bad_pop background\"  , value: 0xff2a00ff }\n  theme_colour:{  tags: \"menu_bar bad_pop text\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"bad_pop hover\"  , value: 0xff825cff }\n  theme_colour:{  tags: code_default  , value: 0x80808ff }\n  theme_colour:{  tags: code_symbol  , value: 0x4ac3ff }\n  theme_colour:{  tags: code_type  , value: 0xf46200ff }\n  theme_colour:{  tags: code_local  , value: 0x317c00ff }\n  theme_colour:{  tags: code_register  , value: 0x9a00ffff }\n  theme_colour:{  tags: code_keyword  , value: 0xff0600ff }\n  theme_colour:{  tags: code_delimiter_or_operator  , value: 0x8a8a8aff }\n  theme_colour:{  tags: code_numeric  , value: 0x7d49ff }\n  theme_colour:{  tags: code_numeric_alt_digit_group  , value: 0xb56aff }\n  theme_colour:{  tags: code_string  , value: 0x63549fff }\n  theme_colour:{  tags: code_meta  , value: 0xd96759ff }\n  theme_colour:{  tags: code_comment  , value: 0x717171ff }\n  theme_colour:{  tags: line_info_0  , value: 0xe6d5cdff }\n  theme_colour:{  tags: line_info_1  , value: 0xdbcfb2ff }\n  theme_colour:{  tags: line_info_2  , value: 0xddeac1ff }\n  theme_colour:{  tags: line_info_3  , value: 0xddc4bdff }\n  theme_colour:{  tags: line_info_4  , value: 0xba917eff }\n  theme_colour:{  tags: thread_0  , value: 0xffa700ff }\n  theme_colour:{  tags: thread_1  , value: 0xb41fff }\n  theme_colour:{  tags: thread_2  , value: 0xff99e5ff }\n  theme_colour:{  tags: thread_3  , value: 0x6598ffff }\n  theme_colour:{  tags: thread_4  , value: 0x65ffcbff }\n  theme_colour:{  tags: thread_5  , value: 0xff9819ff }\n  theme_colour:{  tags: thread_6  , value: 0x9932ffff }\n  theme_colour:{  tags: thread_7  , value: 0x65ff4cff }\n  theme_colour:{  tags: thread_unwound  , value: 0xb2ccd8ff }\n  theme_colour:{  tags: thread_error  , value: 0xb23219ff }\n  theme_colour:{  tags: breakpoint  , value: 0xff2800ff }\n  theme_colour:{  tags: \"floating background\"  , value: 0xffffffc7 }\n  theme_colour:{  tags: \"floating background alt\"  , value: 0x23 }\n  theme_colour:{  tags: \"floating background fresh\"  , value: 0xeaddceff }\n  theme_colour:{  tags: \"floating border\"  , value: 0x88888884 }\n  theme_colour:{  tags: \"scroll_bar background\"  , value: 0xe9e9e9ff }\n  theme_colour:{  tags: \"scroll_bar border\"  , value: 0x5f5f5f5f }\n  theme_colour:{  tags: \"floating scroll_bar background\"  , value: 0xe9e9e9ff }\n  theme_colour:{  tags: \"floating scroll_bar border\"  , value: 0x5f5f5f5f }\n  theme_colour:{  tags: \"menu_bar background\"  , value: 0x2b6b9aff }\n  theme_colour:{  tags: \"menu_bar border\"  , value: 0x4d }\n  theme_colour:{  tags: \"menu_bar text\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"menu_bar text weak\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"good menu_bar text\"  , value: 0x70db32dff }\n  theme_colour:{  tags: \"bad menu_bar text\"  , value: 0xffa79bff }\n  theme_colour:{  tags: \"neutral menu_bar text\"  , value: 0xc4dbe7ff }\n  theme_colour:{  tags: \"implicit background\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"implicit border\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"hollow background\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"hollow border\"  , value: 0xffffff1f }\n  theme_colour:{  tags: \"tab text\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"tab text weak\"  , value: 0xffffffff }\n  theme_colour:{  tags: \"tab background\"  , value: 0xb67e48ff }\n  theme_colour:{  tags: \"tab border\"  , value: 0x875b31ff }\n  theme_colour:{  tags: \"tab inactive background\"  , value: 0xcacacaff }\n  theme_colour:{  tags: \"tab inactive border\"  , value: 0xb5b5b5ff }\n  theme_colour:{  tags: \"tab auto background\"  , value: 0xc41c69ff }\n  theme_colour:{  tags: \"tab auto border\"  , value: 0x981039ff }\n  theme_colour:{  tags: \"tab auto inactive background\"  , value: 0x9b328a3ff }\n  theme_colour:{  tags: \"tab auto inactive border\"  , value: 0x373737ff }\n  theme_colour:{  tags: \"drop_site background\"  , value: 0xffffff05 }\n  theme_colour:{  tags: \"drop_site border\"  , value: 0xffffff0f }\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x1f1f1fff}\n  theme_colour:{tags: \"alt background\", value: 0x222222ff}\n  theme_colour:{tags: \"pop background\", value: 0x383167ff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour:{  tags: \"match background\"  , value: 0x31393dff }\n  theme_colour:{  tags: border  , value: 0x404040ff }\n  theme_colour:{  tags: text  , value: 0xe5e5e5ff }\n  theme_colour:{  tags: \"weak text\"  , value: 0xa4a4a4ff }\n  theme_colour:{  tags: \"good text\"  , value: 0x32a852ff }\n  theme_colour:{  tags: \"neutral text\"  , value: 0x3a90bbff }\n  theme_colour:{  tags: \"bad text\"  , value: 0xcf5242ff }\n  theme_colour:{  tags: hover  , value: 0xffffffff }\n  theme_colour:{  tags: \"focus overlay\"  , value: 0x7160e81e }\n  theme_colour:{  tags: \"focus border\"  , value: 0x7160e8ff }\n  theme_colour:{  tags: cursor  , value: 0x8aff00ff }\n  theme_colour:{  tags: selection  , value: 0x99ccff0f }\n  theme_colour:{  tags: \"inactive background\"  , value: 0x0000002f }\n  theme_colour:{  tags: drop_shadow  , value: 0x0000007f }\n  theme_colour:{  tags: \"good_pop background\"  , value: 0x2c5b36ff }\n  theme_colour:{  tags: \"good_pop border\"  , value: 0x568761ff }\n  theme_colour:{  tags: \"good_pop hover\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"good_pop weak text\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"bad_pop background\"  , value: 0x803425ff }\n  theme_colour:{  tags: \"bad_pop hover\"  , value: 0xff825cff }\n  theme_colour:{  tags: code_default  , value: 0xe0e0e0ff }\n  theme_colour:{  tags: code_symbol  , value: 0xdcdcaaff }\n  theme_colour:{  tags: code_type  , value: 0x4ec9b0ff }\n  theme_colour:{  tags: code_local  , value: 0x9cdcfeff }\n  theme_colour:{  tags: code_register  , value: 0xb7afd5ff }\n  theme_colour:{  tags: code_keyword  , value: 0x569cd6ff }\n  theme_colour:{  tags: code_delimiter_or_operator  , value: 0x767676ff }\n  theme_colour:{  tags: code_numeric  , value: 0xb5cea8ff }\n  theme_colour:{  tags: code_numeric_alt_digit_group  , value: 0x7c986dff }\n  theme_colour:{  tags: code_string  , value: 0xd69d85ff }\n  theme_colour:{  tags: code_meta  , value: 0x9b9b9bff }\n  theme_colour:{  tags: code_comment  , value: 0x51a644ff }\n  theme_colour:{  tags: line_info_0  , value: 0x4f3022ff }\n  theme_colour:{  tags: line_info_1  , value: 0x4f3e15ff }\n  theme_colour:{  tags: line_info_2  , value: 0x434e2aff }\n  theme_colour:{  tags: line_info_3  , value: 0x36241fff }\n  theme_colour:{  tags: line_info_4  , value: 0x4f3022ff }\n  theme_colour:{  tags: line_info_5  , value: 0x4f3e15ff }\n  theme_colour:{  tags: line_info_6  , value: 0x434e2aff }\n  theme_colour:{  tags: line_info_7  , value: 0x36241fff }\n  theme_colour:{  tags: thread_0  , value: 0xffdc48ff }\n  theme_colour:{  tags: thread_1  , value: 0xb2ff65ff }\n  theme_colour:{  tags: thread_2  , value: 0xff99e5ff }\n  theme_colour:{  tags: thread_3  , value: 0x6598ffff }\n  theme_colour:{  tags: thread_4  , value: 0x65ffcbff }\n  theme_colour:{  tags: thread_5  , value: 0xff9819ff }\n  theme_colour:{  tags: thread_6  , value: 0x9932ffff }\n  theme_colour:{  tags: thread_7  , value: 0x65ff4cff }\n  theme_colour:{  tags: thread_unwound  , value: 0xb2ccd8ff }\n  theme_colour:{  tags: thread_error  , value: 0xb23219ff }\n  theme_colour:{  tags: breakpoint  , value: 0xa72911ff }\n  theme_colour:{  tags: \"floating background\"  , value: 0x1b1b1baf }\n  theme_colour:{  tags: \"floating background alt\"  , value: 0x0000005f }\n  theme_colour:{  tags: \"floating background fresh\"  , value: 0x31393d5f }\n  theme_colour:{  tags: \"floating border\"  , value: 0xbfbfbf1f }\n  theme_colour:{  tags: \"floating scroll_bar background\"  , value: 0x3b3b3b5f }\n  theme_colour:{  tags: \"floating scroll_bar border\"  , value: 0x5f5f5f5f }\n  theme_colour:{  tags: \"scroll_bar background\"  , value: 0x2b2b2bff }\n  theme_colour:{  tags: \"scroll_bar border\"  , value: 0x3f3f3fff }\n  theme_colour:{  tags: \"implicit background\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"implicit border\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"hollow background\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"hollow border\"  , value: 0xffffff1f }\n  theme_colour:{  tags: \"tab background\"  , value: 0x333333ff }\n  theme_colour:{  tags: \"tab border\"  , value: 0x7160e8ff }\n  theme_colour:{  tags: \"tab inactive background\"  , value: 0x171717ff }\n  theme_colour:{  tags: \"tab inactive border\"  , value: 0x3e4c57ff }\n  theme_colour:{  tags: \"tab auto background\"  , value: 0x3f386dff }\n  theme_colour:{  tags: \"tab auto border\"  , value: 0x7160e8ff }\n  theme_colour:{  tags: \"tab auto inactive background\"  , value: 0x2f2633ff }\n  theme_colour:{  tags: \"tab auto inactive border\"  , value: 0x685073ff }\n  theme_colour:{  tags: \"drop_site background\"  , value: 0xffffff05 }\n  theme_colour:{  tags: \"drop_site border\"  , value: 0xffffff0f }\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0xffffffff}\n  theme_colour:{tags: \"alt background\", value: 0xefefefff}\n  theme_colour:{tags: \"pop background\", value: 0xe3eaf2ff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0xeccbbeff}\n  theme_colour:{tags: \"match background\", value: 0xedcbf9ff}\n  theme_colour:{tags: border, value: 0xe7e7e7ff}\n  theme_colour:{tags: text, value: 0xff}\n  theme_colour:{tags: \"weak text\", value: 0x353535ff}\n  theme_colour:{tags: \"good text\", value: 0x32a852ff}\n  theme_colour:{tags: \"neutral text\", value: 0x3a90bbff}\n  theme_colour:{tags: \"bad text\", value: 0xcf5242ff}\n  theme_colour:{tags: hover, value: 0xa7ffff}\n  theme_colour:{tags: \"focus overlay\", value: 0x8eff3f}\n  theme_colour:{tags: \"focus border\", value: 0x8effff}\n  theme_colour:{tags: cursor, value: 0xff}\n  theme_colour:{tags: selection, value: 0x56aaff77}\n  theme_colour:{tags: \"inactive background\", value: 0x17}\n  theme_colour:{tags: drop_shadow, value: 0xe7b27}\n  theme_colour:{tags: \"good_pop background\", value: 0x21a43dff}\n  theme_colour:{tags: \"good_pop border\", value: 0x21a43dff}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0xcb3f23ff}\n  theme_colour:{tags: \"bad_pop text\", value: 0xffcdc4ff}\n  theme_colour:{tags: \"bad_pop text weak\", value: 0xffcdc4ff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xff825cff}\n  theme_colour:{tags: code_default, value: 0x000000ff}\n  theme_colour:{tags: code_symbol, value: 0x74531fff}\n  theme_colour:{tags: code_type, value: 0x2b91afff}\n  theme_colour:{tags: code_local, value: 0x1f377fff}\n  theme_colour:{tags: code_register, value: 0x8a1bffff}\n  theme_colour:{tags: code_keyword, value: 0x0000ffff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x767676ff}\n  theme_colour:{tags: code_numeric, value: 0xff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x1d1d1dff}\n  theme_colour:{tags: code_string, value: 0xa61515ff}\n  theme_colour:{tags: code_meta, value: 0x808080ff}\n  theme_colour:{tags: code_comment, value: 0x008000ff}\n  theme_colour:{tags: line_info_0, value: 0xb5d9c8ff}\n  theme_colour:{tags: line_info_1, value: 0xa9c1d0ff}\n  theme_colour:{tags: line_info_2, value: 0x99abc5ff}\n  theme_colour:{tags: line_info_3, value: 0xc6bcd5ff}\n  theme_colour:{tags: thread_0, value: 0xffb141ff}\n  theme_colour:{tags: thread_1, value: 0x66c407ff}\n  theme_colour:{tags: thread_unwound, value: 0x67b3d7ff}\n  theme_colour:{tags: thread_error, value: 0xff2900ff}\n  theme_colour:{tags: breakpoint, value: 0xff2800ff}\n  theme_colour:{tags: \"floating background\", value: 0xffffffff}\n  theme_colour:{tags: \"floating background alt\", value: 0x11}\n  theme_colour:{tags: \"floating background fresh\", value: 0xa0c2d318}\n  theme_colour:{tags: \"floating border\", value: 0x50}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0xccd5f0ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0xbabdc3ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0xe4e4e4ff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0xf5cc84ff}\n  theme_colour:{tags: \"tab border\", value: 0xae7718ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x3b4f81ff}\n  theme_colour:{tags: \"tab inactive text\", value: 0xffffffff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x3b4f81ff}\n  theme_colour:{tags: \"tab auto background\", value: 0xe99595ff}\n  theme_colour:{tags: \"tab auto border\", value: 0xff6262ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0xac6060ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0xff6262ff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xa7ffff}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x002a35ff}\n  theme_colour:{tags: \"alt background\", value: 0x053542ff}\n  theme_colour:{tags: \"pop background\", value: 0x355b6eff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour:{tags: \"match background\", value: 0x31393dff}\n  theme_colour:{tags: border, value: 0x65166ff}\n  theme_colour:{tags: text, value: 0xeee8d5ff}\n  theme_colour:{tags: \"weak text\", value: 0x93a1a1ff}\n  theme_colour:{tags: \"good text\", value: 0x32a852ff}\n  theme_colour:{tags: \"neutral text\", value: 0x3a90bbff}\n  theme_colour:{tags: \"bad text\", value: 0xcf5242ff}\n  theme_colour:{tags: hover, value: 0xca4b16ff}\n  theme_colour:{tags: \"focus overlay\", value: 0xca4b151f}\n  theme_colour:{tags: \"focus border\", value: 0xca4b16ff}\n  theme_colour:{tags: cursor, value: 0xca4b16ff}\n  theme_colour:{tags: selection, value: 0x99ccff0f}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x5f8700ff}\n  theme_colour:{tags: \"good_pop border\", value: 0x5f8700ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x810000ff}\n  theme_colour:{tags: code_default, value: 0x839496ff}\n  theme_colour:{tags: code_symbol, value: 0xb3880eff}\n  theme_colour:{tags: code_type, value: 0xb3880eff}\n  theme_colour:{tags: code_local, value: 0xeee8d5ff}\n  theme_colour:{tags: code_register, value: 0xeee8d5ff}\n  theme_colour:{tags: code_keyword, value: 0x849804ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x839496ff}\n  theme_colour:{tags: code_numeric, value: 0x2aa198ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x19766bff}\n  theme_colour:{tags: code_string, value: 0x2aa198ff}\n  theme_colour:{tags: code_meta, value: 0xca4b16ff}\n  theme_colour:{tags: code_comment, value: 0x586e75ff}\n  theme_colour:{tags: line_info_0, value: 0x4f3022ff}\n  theme_colour:{tags: line_info_1, value: 0x4f3e15ff}\n  theme_colour:{tags: line_info_2, value: 0x434e2aff}\n  theme_colour:{tags: line_info_3, value: 0x36241fff}\n  theme_colour:{tags: thread_0, value: 0xffcb7fff}\n  theme_colour:{tags: thread_1, value: 0xb2ff65ff}\n  theme_colour:{tags: thread_unwound, value: 0xb2ccd8ff}\n  theme_colour:{tags: thread_error, value: 0xb23219ff}\n  theme_colour:{tags: breakpoint, value: 0xa72911ff}\n  theme_colour:{tags: \"floating background\", value: 0x2a3574}\n  theme_colour:{tags: \"floating background alt\", value: 0x4f}\n  theme_colour:{tags: \"floating background fresh\", value: 0x31393d5f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x53542ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x53542ff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x586e75ff}\n  theme_colour:{tags: \"tab border\", value: 0x90abb3ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x0}\n  theme_colour:{tags: \"tab inactive border\", value: 0x33494fff}\n  theme_colour:{tags: \"tab auto background\", value: 0x565ed2ff}\n  theme_colour:{tags: \"tab auto border\", value: 0xa2a6dfff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x595fbcff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0xfdf6e3ff}\n  theme_colour:{tags: \"alt background\", value: 0xeee8d5ff}\n  theme_colour:{tags: \"pop background\", value: 0x29a19890}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0xf7d38dff}\n  theme_colour:{tags: \"match background\", value: 0xdcddddff}\n  theme_colour:{tags: border, value: 0xd1ccbdff}\n  theme_colour:{tags: \"weak text\", value: 0x93a1a1ff}\n  theme_colour:{tags: text, value: 0x657b323ff}\n  theme_colour:{tags: \"good text\", value: 0x32a852ff}\n  theme_colour:{tags: \"neutral text\", value: 0x3a90bbff}\n  theme_colour:{tags: \"bad text\", value: 0xcf5242ff}\n  theme_colour:{tags: hover, value: 0xca4b16ff}\n  theme_colour:{tags: \"focus overlay\", value: 0xca4b1454}\n  theme_colour:{tags: \"focus border\", value: 0xca4b16ff}\n  theme_colour:{tags: cursor, value: 0xca4b16ff}\n  theme_colour:{tags: selection, value: 0x8594a264}\n  theme_colour:{tags: \"inactive background\", value: 0x14}\n  theme_colour:{tags: drop_shadow, value: 0x3a}\n  theme_colour:{tags: \"good_pop background\", value: 0xd1e99aff}\n  theme_colour:{tags: \"good_pop border\", value: 0xd1e99aff}\n  theme_colour:{tags: \"bad_pop background\", value: 0xd26c6cff}\n  theme_colour:{tags: \"bad_pop border\", value: 0xd26c6cff}\n  theme_colour:{tags: \"bad_pop weak text\", value: 0xffffffff}\n  theme_colour:{tags: \"bad_pop text\", value: 0xffffffff}\n  theme_colour:{tags: code_default, value: 0x839496ff}\n  theme_colour:{tags: code_symbol, value: 0xb3880eff}\n  theme_colour:{tags: code_type, value: 0xb3880eff}\n  theme_colour:{tags: code_local, value: 0x657b323ff}\n  theme_colour:{tags: code_register, value: 0x947298ff}\n  theme_colour:{tags: code_keyword, value: 0x849804ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x839496ff}\n  theme_colour:{tags: code_numeric, value: 0x2aa198ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x19766bff}\n  theme_colour:{tags: code_string, value: 0x2aa198ff}\n  theme_colour:{tags: code_meta, value: 0xca4b16ff}\n  theme_colour:{tags: code_comment, value: 0x586e75ff}\n  theme_colour:{tags: line_info_0, value: 0xeee1dbff}\n  theme_colour:{tags: line_info_1, value: 0xe5d5b1ff}\n  theme_colour:{tags: line_info_2, value: 0xd9e8b6ff}\n  theme_colour:{tags: line_info_3, value: 0xf5d5ccff}\n  theme_colour:{tags: thread_0, value: 0xffa100ff}\n  theme_colour:{tags: thread_1, value: 0x6edd00ff}\n  theme_colour:{tags: thread_unwound, value: 0x708f9eff}\n  theme_colour:{tags: thread_error, value: 0xff5231ff}\n  theme_colour:{tags: breakpoint, value: 0xff411dff}\n  theme_colour:{tags: \"floating background\", value: 0xfdf6e3b1}\n  theme_colour:{tags: \"floating background alt\", value: 0x22}\n  theme_colour:{tags: \"floating background fresh\", value: 0xf7d38dff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0xeee6d0ff}\n  theme_colour:{tags: \"scroll_bar floating background\", value: 0xd5ccb5ff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0xfdf6e3ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0xe0d6bbff}\n  theme_colour:{tags: \"tab auto background\", value: 0xf5cfe1ff}\n  theme_colour:{tags: \"tab auto border\", value: 0xa2a6dfff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0xc8a5b5ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x595fbcff}\n  theme_colour:{tags: \"drop_site background\", value: 0x29a19890}\n  theme_colour:{tags: \"drop_site border\", value: 0x81ddd690}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x0c0c0cff}\n  theme_colour:{tags: \"alt background\", value: 0x161616ff}\n  theme_colour:{tags: \"pop background\", value: 0x355b6eff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour:{tags: \"match background\", value: 0x31393dff}\n  theme_colour:{tags: border, value: 0x404040ff}\n  theme_colour:{tags: text, value: 0xcac1b6ff}\n  theme_colour:{tags: \"weak text\", value: 0xa08563ff}\n  theme_colour:{tags: \"good text\", value: 0x32a852ff}\n  theme_colour:{tags: \"neutral text\", value: 0x3a90bbff}\n  theme_colour:{tags: \"bad text\", value: 0xcf5242ff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0x7485971e}\n  theme_colour:{tags: \"focus border\", value: 0x5e6b79ff}\n  theme_colour:{tags: cursor, value: 0x00ee00ff}\n  theme_colour:{tags: selection, value: 0x99ccff0f}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x2c5b36ff}\n  theme_colour:{tags: \"good_pop border\", value: 0x568761ff}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x803425ff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xff825cff}\n  theme_colour:{tags: code_default, value: 0xa08563ff}\n  theme_colour:{tags: code_symbol, value: 0xcc5735ff}\n  theme_colour:{tags: code_type, value: 0xd8a51cff}\n  theme_colour:{tags: code_local, value: 0xd6b995ff}\n  theme_colour:{tags: code_register, value: 0xc04047ff}\n  theme_colour:{tags: code_keyword, value: 0xac7b0aff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x907553ff}\n  theme_colour:{tags: code_numeric, value: 0x6b32e23f}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x4f681cff}\n  theme_colour:{tags: code_string, value: 0x6b32e23f}\n  theme_colour:{tags: code_meta, value: 0xdab98fff}\n  theme_colour:{tags: code_comment, value: 0x686868ff}\n  theme_colour:{tags: line_info_0, value: 0x4f3022ff}\n  theme_colour:{tags: line_info_1, value: 0x4f3e15ff}\n  theme_colour:{tags: line_info_2, value: 0x434e2aff}\n  theme_colour:{tags: line_info_3, value: 0x36241fff}\n  theme_colour:{tags: thread_0, value: 0xffcb7fff}\n  theme_colour:{tags: thread_1, value: 0xb2ff65ff}\n  theme_colour:{tags: thread_unwound, value: 0xb2ccd8ff}\n  theme_colour:{tags: thread_error, value: 0xb23219ff}\n  theme_colour:{tags: breakpoint, value: 0xa72911ff}\n  theme_colour:{tags: \"floating background\", value: 0x18181980}\n  theme_colour:{tags: \"floating background alt\", value: 0x0000005f}\n  theme_colour:{tags: \"floating background fresh\", value: 0x31393d5f}\n  theme_colour:{tags: \"floating border\", value: 0xbfbfbf1f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x1f1f27ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x3d3d47ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x2b2b2bff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x3f3f3fff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x1f1f27ff}\n  theme_colour:{tags: \"tab border\", value: 0x3d3d47ff}\n  theme_colour:{tags: \"tab text\", value: 0xca9301ff}\n  theme_colour:{tags: \"tab text weak\", value: 0x8c690eff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x171718ff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x1f1f27ff}\n  theme_colour:{tags: \"tab auto background\", value: 0x243b38ff}\n  theme_colour:{tags: \"tab auto border\", value: 0x478980ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x102623ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x1e5850ff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x042327ff}\n  theme_colour:{tags: \"alt background\", value: 0x011b1fff}\n  theme_colour:{tags: \"pop background\", value: 0x355b6eff}\n  theme_colour:{tags: \"pop text\", value: 0xbad7e6ff}\n                 theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour:{tags: \"match background\", value: 0x31393dff}\n  theme_colour:{tags: border, value: 0x334d50ff}\n  theme_colour:{tags: text, value: 0xdad3beff}\n  theme_colour:{tags: \"weak text\", value: 0xb0a688ff}\n  theme_colour:{tags: \"good text\", value: 0x32a852ff}\n  theme_colour:{tags: \"neutral text\", value: 0x3a90bbff}\n  theme_colour:{tags: \"bad text\", value: 0xcf5242ff}\n  theme_colour:{tags: \"menu_bar good text\", value: 0x2a8242ff}\n  theme_colour:{tags: \"menu_bar neutral text\", value: 0x5681ff}\n  theme_colour:{tags: \"menu_bar bad text\", value: 0xa21200ff}\n  theme_colour:{tags: \"menu_bar weak text\", value: 0x313131ff}\n  theme_colour:{tags: \"menu_bar bad_pop text weak\", value: 0xffffffff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0x86e08e20}\n  theme_colour:{tags: \"focus border\", value: 0x86e08fff}\n  theme_colour:{tags: cursor, value: 0x86e08fff}\n  theme_colour:{tags: selection, value: 0x99ccff0f}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x2c5b36ff}\n  theme_colour:{tags: \"good_pop border\", value: 0x568761ff}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x803425ff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xff825cff}\n  theme_colour:{tags: code_default, value: 0xbdb395ff}\n  theme_colour:{tags: code_symbol, value: 0xcbe0f5ff}\n  theme_colour:{tags: code_type, value: 0xcbe0f5ff}\n  theme_colour:{tags: code_local, value: 0xd9cfb3ff}\n  theme_colour:{tags: code_register, value: 0xb7afd5ff}\n  theme_colour:{tags: code_keyword, value: 0x9de3c0ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x767676ff}\n  theme_colour:{tags: code_numeric, value: 0x2ca198ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x217770ff}\n  theme_colour:{tags: code_string, value: 0x2ca198ff}\n  theme_colour:{tags: code_meta, value: 0xB0FFB0ff}\n  theme_colour:{tags: code_comment, value: 0x31b72cff}\n  theme_colour:{tags: line_info_0, value: 0x4f3022ff}\n  theme_colour:{tags: line_info_1, value: 0x4f3e15ff}\n  theme_colour:{tags: line_info_2, value: 0x434e2aff}\n  theme_colour:{tags: line_info_3, value: 0x36241fff}\n  theme_colour:{tags: thread_0, value: 0xffcb7fff}\n  theme_colour:{tags: thread_1, value: 0xb2ff65ff}\n  theme_colour:{tags: thread_unwound, value: 0xb2ccd8ff}\n  theme_colour:{tags: thread_error, value: 0xb23219ff}\n  theme_colour:{tags: breakpoint, value: 0xa72911ff}\n  theme_colour:{tags: \"floating background\", value: 0x3232792}\n  theme_colour:{tags: \"floating background alt\", value: 0x0000005f}\n  theme_colour:{tags: \"floating background fresh\", value: 0x31393d5f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0xe363bff}\n  theme_colour:{tags: \"menu_bar background\", value: 0xb59e7aff}\n  theme_colour:{tags: \"menu_bar text\", value: 0xff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x947d5aff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0xe363bff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x13533aff}\n  theme_colour:{tags: \"tab border\", value: 0x6c9182ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x0}\n  theme_colour:{tags: \"tab inactive border\", value: 0x947d5a6c}\n  theme_colour:{tags: \"tab auto background\", value: 0x5b3939ff}\n  theme_colour:{tags: \"tab auto border\", value: 0x875c5cff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x251b1bff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x563d3dff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x0c0c0cff}\n  theme_colour:{tags: \"alt background\", value: 0x131313ff}\n  theme_colour:{tags: \"pop background\", value: 0x004c00ff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x004c00ff}\n  theme_colour:{tags: \"match background\", value: 0x4c00ff}\n  theme_colour:{tags: border, value: 0x272727ff}\n  theme_colour:{tags: text, value: 0x90b080ff}\n  theme_colour:{tags: \"weak text\", value: 0x6b3245fff}\n  theme_colour:{tags: \"menu_bar background\", value: 0x888888ff}\n  theme_colour:{tags: \"menu_bar text\", value: 0x20202ff}\n  theme_colour:{tags: \"menu_bar text weak\", value: 0x525252ff}\n  theme_colour:{tags: \"good text\", value: 0x32a852ff}\n  theme_colour:{tags: \"neutral text\", value: 0x3a90bbff}\n  theme_colour:{tags: \"bad text\", value: 0xcf5242ff}\n  theme_colour:{tags: hover, value: 0xee00ff}\n  theme_colour:{tags: \"focus overlay\", value: 0xee0012}\n  theme_colour:{tags: \"focus border\", value: 0x00ee00ff}\n  theme_colour:{tags: cursor, value: 0x00ee00ff}\n  theme_colour:{tags: selection, value: 0x99ccff0f}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x4900ff}\n  theme_colour:{tags: \"good_pop border\", value: 0x4900ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x430b00ff}\n  theme_colour:{tags: code_default, value: 0x90b080ff}\n  theme_colour:{tags: code_symbol, value: 0xbfd9b2ff}\n  theme_colour:{tags: code_type, value: 0xbfd9b2ff}\n  theme_colour:{tags: code_local, value: 0xbfd9b2ff}\n  theme_colour:{tags: code_register, value: 0xb324cffff}\n  theme_colour:{tags: code_keyword, value: 0xd08f1fff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x90b080ff}\n  theme_colour:{tags: code_numeric, value: 0x50ff2fff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x30af18ff}\n  theme_colour:{tags: code_string, value: 0x50ff2fff}\n  theme_colour:{tags: code_meta, value: 0x90b080ff}\n  theme_colour:{tags: code_comment, value: 0x1f90f0ff}\n  theme_colour:{tags: line_info_0, value: 0xa253dff}\n  theme_colour:{tags: line_info_1, value: 0x9103dff}\n  theme_colour:{tags: line_info_2, value: 0x1e083dff}\n  theme_colour:{tags: line_info_3, value: 0x1e083dff}\n  theme_colour:{tags: thread_0, value: 0xd08f1fff}\n  theme_colour:{tags: thread_1, value: 0x1ea5d0ff}\n  theme_colour:{tags: thread_unwound, value: 0xb2ccd8ff}\n  theme_colour:{tags: thread_error, value: 0xb23219ff}\n  theme_colour:{tags: breakpoint, value: 0xa72911ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x1d1d1dff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x15490cff}\n  theme_colour:{tags: \"tab border\", value: 0x15490cff}\n  theme_colour:{tags: \"tab text\", value: 0x90b080ff}\n  theme_colour:{tags: \"tab text weak\", value: 0x90b080ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x21321eff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x21321eff}\n  theme_colour:{tags: \"tab auto background\", value: 0x674f3eff}\n  theme_colour:{tags: \"tab auto border\", value: 0x674f3eff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x47382eff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x47382eff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x1b1f22ff}\n  theme_colour:{tags: \"alt background\", value: 0x232929ff}\n  theme_colour:{tags: \"pop background\", value: 0x2f4838ff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour:{  tags: \"match background\"  , value: 0x31393dff }\n  theme_colour:{  tags: border  , value: 0x485347ff }\n  theme_colour:{  tags: text  , value: 0xffffffff }\n  theme_colour:{  tags: \"weak text\"  , value: 0xa2a2a2ff }\n  theme_colour:{  tags: \"good text\"  , value: 0x32a852ff }\n  theme_colour:{  tags: \"neutral text\"  , value: 0x3a90bbff }\n  theme_colour:{  tags: \"bad text\"  , value: 0xcf5242ff }\n  theme_colour:{  tags: hover  , value: 0xffffffff }\n  theme_colour:{  tags: \"focus overlay\"  , value: 0xfda20012 }\n  theme_colour:{  tags: \"focus border\"  , value: 0xfda200ff }\n  theme_colour:{  tags: cursor  , value: 0x8aff00ff }\n  theme_colour:{  tags: selection  , value: 0x99ccff0f }\n  theme_colour:{  tags: \"inactive background\"  , value: 0x0 }\n  theme_colour:{  tags: drop_shadow  , value: 0x0000007f }\n  theme_colour:{  tags: \"good_pop background\"  , value: 0x2c5b36ff }\n  theme_colour:{  tags: \"good_pop border\"  , value: 0x568761ff }\n  theme_colour:{  tags: \"good_pop hover\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"good_pop weak text\"  , value: 0xe3f5d3ff }\n  theme_colour:{  tags: \"bad_pop background\"  , value: 0x803425ff }\n  theme_colour:{  tags: \"bad_pop hover\"  , value: 0xff825cff }\n  theme_colour:{  tags: code_default  , value: 0xad8b69ff }\n  theme_colour:{  tags: code_symbol  , value: 0x87ad6aff }\n  theme_colour:{  tags: code_type  , value: 0xb67474ff }\n  theme_colour:{  tags: code_local  , value: 0xe9bf95ff }\n  theme_colour:{  tags: code_register  , value: 0xa688b2ff }\n  theme_colour:{  tags: code_keyword  , value: 0xe49e17ff }\n  theme_colour:{  tags: code_delimiter_or_operator  , value: 0x795e43ff }\n  theme_colour:{  tags: code_numeric  , value: 0x98b19eff }\n  theme_colour:{  tags: code_numeric_alt_digit_group  , value: 0x688b71ff }\n  theme_colour:{  tags: code_string  , value: 0x98b19eff }\n  theme_colour:{  tags: code_meta  , value: 0xad5979ff }\n  theme_colour:{  tags: code_comment  , value: 0x52675dff }\n  theme_colour:{  tags: line_info_0  , value: 0x4f3022ff }\n  theme_colour:{  tags: line_info_1  , value: 0x4f3e15ff }\n  theme_colour:{  tags: line_info_2  , value: 0x434e2aff }\n  theme_colour:{  tags: line_info_3  , value: 0x36241fff }\n  theme_colour:{  tags: line_info_4  , value: 0x4f3022ff }\n  theme_colour:{  tags: line_info_5  , value: 0x4f3e15ff }\n  theme_colour:{  tags: line_info_6  , value: 0x434e2aff }\n  theme_colour:{  tags: line_info_7  , value: 0x36241fff }\n  theme_colour:{  tags: thread_0  , value: 0xffc258ff }\n  theme_colour:{  tags: thread_1  , value: 0x82d331ff }\n  theme_colour:{  tags: thread_2  , value: 0xff99e5ff }\n  theme_colour:{  tags: thread_3  , value: 0x6598ffff }\n  theme_colour:{  tags: thread_4  , value: 0x65ffcbff }\n  theme_colour:{  tags: thread_5  , value: 0xff9819ff }\n  theme_colour:{  tags: thread_6  , value: 0x9932ffff }\n  theme_colour:{  tags: thread_7  , value: 0x65ff4cff }\n  theme_colour:{  tags: thread_unwound  , value: 0xb2ccd8ff }\n  theme_colour:{  tags: thread_error  , value: 0xb23219ff }\n  theme_colour:{  tags: breakpoint  , value: 0xa72911ff }\n  theme_colour:{  tags: \"floating background\"  , value: 0x1b1f2276 }\n  theme_colour:{  tags: \"floating background alt\"  , value: 0x0000005f }\n  theme_colour:{  tags: \"floating background fresh\"  , value: 0x31393d5f }\n  theme_colour:{  tags: \"floating border\"  , value: 0xbfbfbf1f }\n  theme_colour:{  tags: \"floating scroll_bar background\"  , value: 0x3b3b3b5f }\n  theme_colour:{  tags: \"floating scroll_bar border\"  , value: 0x5f5f5f5f }\n  theme_colour:{  tags: \"menu_bar background\"  , value: 0x243d32ff }\n  theme_colour:{  tags: \"menu_bar border\"  , value: 0x597b63ff }\n  theme_colour:{  tags: \"scroll_bar background\"  , value: 0x232929ff }\n  theme_colour:{  tags: \"scroll_bar border\"  , value: 0x3c4a3fff }\n  theme_colour:{  tags: \"implicit background\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"implicit border\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"hollow background\"  , value: 0x00000000 }\n  theme_colour:{  tags: \"hollow border\"  , value: 0xffffff1f }\n  theme_colour:{  tags: \"tab background\"  , value: 0x243d32ff }\n  theme_colour:{  tags: \"tab border\"  , value: 0x597b63ff }\n  theme_colour:{  tags: \"tab inactive background\"  , value: 0x30383eff }\n  theme_colour:{  tags: \"tab inactive border\"  , value: 0x6b7680ff }\n  theme_colour:{  tags: \"tab auto background\"  , value: 0x30636dff }\n  theme_colour:{  tags: \"tab auto border\"  , value: 0x768f94ff }\n  theme_colour:{  tags: \"tab auto inactive background\"  , value: 0x2f2633ff }\n  theme_colour:{  tags: \"tab auto inactive border\"  , value: 0x685073ff }\n  theme_colour:{  tags: \"drop_site background\"  , value: 0xffffff05 }\n  theme_colour:{  tags: \"drop_site border\"  , value: 0xffffff0f }\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x000080ff}\n                                                                theme_colour:{tags: \"pop background\", value: 0x008080ff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x31393dff}\n  theme_colour:{tags: \"match background\", value: 0x31393dff}\n  theme_colour:{tags: border, value: 0x8080ff}\n  theme_colour:{tags: text, value: 0xffffffff}\n  theme_colour:{tags: \"weak text\", value: 0xffffffff}\n  theme_colour:{tags: \"good text\", value: 0x00ff00ff}\n  theme_colour:{tags: \"neutral text\", value: 0x00ffffff}\n  theme_colour:{tags: \"bad text\", value: 0xff0000ff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0xffff0012}\n  theme_colour:{tags: \"focus border\", value: 0xffff00ff}\n  theme_colour:{tags: cursor, value: 0xffff00ff}\n  theme_colour:{tags: selection, value: 0xffff0018}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x6c17ff}\n  theme_colour:{tags: \"good_pop border\", value: 0x6c17ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0xff0000ff}\n  theme_colour:{tags: code_default, value: 0xffffffff}\n  theme_colour:{tags: code_symbol, value: 0xffffff}\n  theme_colour:{tags: code_type, value: 0x00ff00ff}\n  theme_colour:{tags: code_local, value: 0x00ffffff}\n  theme_colour:{tags: code_register, value: 0xff00ffff}\n  theme_colour:{tags: code_keyword, value: 0xffffffff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0xffffffff}\n  theme_colour:{tags: code_numeric, value: 0xffff00ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0xffff00ff}\n  theme_colour:{tags: code_string, value: 0xffff00ff}\n  theme_colour:{tags: code_meta, value: 0xff0000ff}\n  theme_colour:{tags: code_comment, value: 0x008080ff}\n  theme_colour:{tags: line_info_0, value: 0x8080ff}\n  theme_colour:{tags: line_info_1, value: 0x800080ff}\n  theme_colour:{tags: line_info_2, value: 0x800000ff}\n  theme_colour:{tags: line_info_3, value: 0x08000ff}\n  theme_colour:{tags: thread_0, value: 0xffff00ff}\n  theme_colour:{tags: thread_1, value: 0x00ff00ff}\n  theme_colour:{tags: thread_unwound, value: 0x00ffffff}\n  theme_colour:{tags: thread_error, value: 0xff0000ff}\n  theme_colour:{tags: breakpoint, value: 0xff0000ff}\n  theme_colour:{tags: \"menu_bar background\", value: 0x008080ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x8080ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x008080ff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x8080ff}\n  theme_colour:{tags: \"tab border\", value: 0x8080ff}\n  theme_colour:{tags: \"tab inactive background\", value:0}\n  theme_colour:{tags: \"tab auto background\", value: 0x800000ff}\n  theme_colour:{tags: \"tab auto border\", value: 0x8080ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x300000ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x8080ff}\n  theme_colour:{tags: \"drop_site background\", value: 0x80ff}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x0a0e14ff}\n  theme_colour:{tags: \"alt background\", value: 0x131721ff}\n  theme_colour:{tags: \"pop background\", value: 0x27374eff}\n                                                                         theme_colour:{tags: \"fresh background\", value: 0x1f2430ff}\n  theme_colour: {tags:\"match background\", value: 0x1f2430ff }\n  theme_colour: {tags:\"border\", value: 0x393F4Cff }\n  theme_colour: {tags:\"text\", value: 0xb3b1adff }\n  theme_colour: {tags:\"weak text\", value: 0x5c6773ff }\n  theme_colour: {tags:\"good text\", value: 0xc2d94cff }\n  theme_colour: {tags:\"neutral text\", value: 0x59c2ffff }\n  theme_colour: {tags:\"bad text\", value: 0xf07178ff }\n  theme_colour: {tags:\"hover\", value: 0xe6e4e1ff }\n  theme_colour: {tags:\"focus overlay\", value: 0xffb45418 }\n  theme_colour: {tags:\"focus border\", value: 0xffb454ff }\n  theme_colour: {tags:\"cursor\", value: 0xffb454ff }\n  theme_colour: {tags:\"selection\", value: 0x27374e40 }\n  theme_colour: {tags:\"inactive background\", value: 0x0000002f }\n  theme_colour: {tags:\"drop_shadow\", value: 0x0000007f }\n  theme_colour: {tags:\"good_pop background\", value: 0x3e572dff }\n  theme_colour: {tags:\"good_pop border\", value: 0x67894eff }\n  theme_colour: {tags:\"good_pop hover\", value: 0xe6e4e1ff }\n  theme_colour: {tags:\"good_pop weak text\", value: 0xb3b1adff }\n  theme_colour: {tags:\"bad_pop background\", value: 0x783a3fff }\n  theme_colour: {tags:\"bad_pop hover\", value: 0xe6e4e1ff }\n  theme_colour: {tags:\"code_default\", value: 0xb3b1adff }\n  theme_colour: {tags:\"code_symbol\", value: 0x59c2ffff }\n  theme_colour: {tags:\"code_type\", value: 0xe6b450ff }\n  theme_colour: {tags:\"code_local\", value: 0x95e6cbff }\n  theme_colour: {tags:\"code_register\", value: 0xd2a6ffff }\n  theme_colour: {tags:\"code_keyword\", value: 0xff7733ff }\n  theme_colour: {tags:\"code_delimiter_or_operator\", value: 0xe6e4e1ff }\n  theme_colour: {tags:\"code_numeric\", value: 0xd2a6ffff }\n  theme_colour: {tags:\"code_numeric_alt_digit_group\", value: 0xa885ccff }\n  theme_colour: {tags:\"code_string\", value: 0xc2d94cff }\n  theme_colour: {tags:\"code_meta\", value: 0xf07178ff }\n  theme_colour: {tags:\"code_comment\", value: 0x5c6773ff }\n  theme_colour: {tags:\"line_info_0\", value: 0x252b36ff }\n  theme_colour: {tags:\"line_info_1\", value: 0x2b2536ff }\n  theme_colour: {tags:\"line_info_2\", value: 0x25362bff }\n  theme_colour: {tags:\"line_info_3\", value: 0x362525ff }\n  theme_colour: {tags:\"line_info_4\", value: 0x252b36ff }\n  theme_colour: {tags:\"line_info_5\", value: 0x2b2536ff }\n  theme_colour: {tags:\"line_info_6\", value: 0x25362bff }\n  theme_colour: {tags:\"line_info_7\", value: 0x362525ff }\n  theme_colour: {tags:\"thread_0\", value: 0xff7733ff }\n  theme_colour: {tags:\"thread_1\", value: 0xc2d94Cff }\n  theme_colour: {tags:\"thread_2\", value: 0xd2a6ffff }\n  theme_colour: {tags:\"thread_3\", value: 0x59c2ffff }\n  theme_colour: {tags:\"thread_4\", value: 0x95e6cbff }\n  theme_colour: {tags:\"thread_5\", value: 0xe6b450ff }\n  theme_colour: {tags:\"thread_6\", value: 0xf07178ff }\n  theme_colour: {tags:\"thread_7\", value: 0xffb454ff }\n  theme_colour: {tags:\"thread_unwound\", value: 0x5c6773ff }\n  theme_colour: {tags:\"thread_error\", value: 0xf07178ff }\n  theme_colour: {tags:\"breakpoint\", value: 0xf07178ff }\n  theme_colour: {tags:\"floating background\", value: 0x0a0e14cc }\n  theme_colour: {tags:\"floating background alt\", value: 0x131721cc }\n  theme_colour: {tags:\"floating background fresh\", value: 0x1f2430cc }\n  theme_colour: {tags:\"floating border\", value: 0x393f4c7f }\n  theme_colour: {tags:\"floating scroll_bar background\", value: 0x1f24307f }\n  theme_colour: {tags:\"floating scroll_bar border\", value: 0x393f4c7f }\n  theme_colour: {tags:\"menu_bar background\", value: 0x131721ff }\n  theme_colour: {tags:\"menu_bar border\", value: 0x393f4cff }\n  theme_colour: {tags:\"scroll_bar background\", value: 0x1f2430ff }\n  theme_colour: {tags:\"scroll_bar border\", value: 0x393f4cff }\n  theme_colour: {tags:\"implicit background\", value: 0x00000000 }\n  theme_colour: {tags:\"implicit border\", value: 0x00000000 }\n  theme_colour: {tags:\"hollow background\", value: 0x00000000 }\n  theme_colour: {tags:\"hollow border\", value: 0xb3b1ad1f }\n  theme_colour: {tags:\"tab background\", value: 0xffb454ff }\n  theme_colour: {tags:\"tab border\", value: 0xd99a48ff }\n  theme_colour: {tags:\"tab text\", value: 0x0a0e14ff }\n  theme_colour: {tags:\"tab inactive background\", value: 0x131721ff }\n  theme_colour: {tags:\"tab inactive border\", value: 0x393f4cff }\n  theme_colour: {tags:\"tab auto background\", value: 0x59c2ffff }\n  theme_colour: {tags:\"tab auto border\", value: 0x4a9fd9ff }\n  theme_colour: {tags:\"tab auto inactive background\", value: 0x1a2c3bff }\n  theme_colour: {tags:\"tab auto inactive border\", value: 0x2d4a63ff }\n  theme_colour: {tags:\"drop_site background\", value: 0xffb4540f }\n  theme_colour: {tags:\"drop_site border\", value: 0xffb4542f }\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x282828ff}\n  theme_colour:{tags: \"alt background\", value: 0x32302fff}\n  theme_colour:{tags: \"pop background\", value: 0x504945ff}\n  theme_colour:{tags: \"fresh background\", value: 0x3c3836ff}\n  theme_colour:{tags: \"match background\", value: 0x3c3836ff}\n  theme_colour:{tags: border, value: 0x504945ff}\n  theme_colour:{tags: text, value: 0xebdbb2ff}\n  theme_colour:{tags: \"weak text\", value: 0xa89984ff}\n  theme_colour:{tags: \"good text\", value: 0xb8bb26ff}\n  theme_colour:{tags: \"neutral text\", value: 0x83a598ff}\n  theme_colour:{tags: \"bad text\", value: 0xfb4934ff}\n  theme_colour:{tags: hover, value: 0xfbf1c7ff}\n  theme_colour:{tags: \"focus overlay\", value: 0xfabd2f1c}\n  theme_colour:{tags: \"focus border\", value: 0xfabd2fff}\n  theme_colour:{tags: cursor, value: 0xfe8019ff}\n  theme_colour:{tags: selection, value: 0xd3869b30}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x3a4a2eff}\n  theme_colour:{tags: \"good_pop border\", value: 0x79740eff}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x5a1f1aff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xfb4934ff}\n  theme_colour:{tags: code_default, value: 0xebdbb2ff}\n  theme_colour:{tags: code_symbol, value: 0x83a598ff}\n  theme_colour:{tags: code_type, value: 0xfabd2fff}\n  theme_colour:{tags: code_local, value: 0x8ec07cff}\n  theme_colour:{tags: code_register, value: 0xd3869bff}\n  theme_colour:{tags: code_keyword, value: 0xfb4934ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0xa89984ff}\n  theme_colour:{tags: code_numeric, value: 0xd3869bff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0xb16286ff}\n  theme_colour:{tags: code_string, value: 0xb8bb26ff}\n  theme_colour:{tags: code_meta, value: 0xfe8019ff}\n  theme_colour:{tags: code_comment, value: 0x928374ff}\n  theme_colour:{tags: line_info_0, value: 0x4a3622ff}\n  theme_colour:{tags: line_info_1, value: 0x4a3e15ff}\n  theme_colour:{tags: line_info_2, value: 0x3e4a2aff}\n  theme_colour:{tags: line_info_3, value: 0x3a2820ff}\n  theme_colour:{tags: line_info_4, value: 0x4a3622ff}\n  theme_colour:{tags: line_info_5, value: 0x4a3e15ff}\n  theme_colour:{tags: line_info_6, value: 0x3e4a2aff}\n  theme_colour:{tags: line_info_7, value: 0x3a2820ff}\n  theme_colour:{tags: thread_0, value: 0xfabd2fff}\n  theme_colour:{tags: thread_1, value: 0xb8bb26ff}\n  theme_colour:{tags: thread_2, value: 0xd3869bff}\n  theme_colour:{tags: thread_3, value: 0x83a598ff}\n  theme_colour:{tags: thread_4, value: 0x8ec07cff}\n  theme_colour:{tags: thread_5, value: 0xfe8019ff}\n  theme_colour:{tags: thread_6, value: 0xb16286ff}\n  theme_colour:{tags: thread_7, value: 0x689d6aff}\n  theme_colour:{tags: thread_unwound, value: 0xa89984ff}\n  theme_colour:{tags: thread_error, value: 0xcc241dff}\n  theme_colour:{tags: breakpoint, value: 0xcc241dff}\n  theme_colour:{tags: \"floating background\", value: 0x282828c8}\n  theme_colour:{tags: \"floating background alt\", value: 0x1d20218c}\n  theme_colour:{tags: \"floating background fresh\", value: 0x3c38365f}\n  theme_colour:{tags: \"floating border\", value: 0xa899841f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x3c3836ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x504945ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x32302fff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x504945ff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x504945ff}\n  theme_colour:{tags: \"tab border\", value: 0x7c6f64ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x32302fff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x504945ff}\n  theme_colour:{tags: \"tab auto background\", value: 0x5a3c3cff}\n  theme_colour:{tags: \"tab auto border\", value: 0xcc241dff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x2c2222ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x7c2828ff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x1a1b26ff}\n  theme_colour:{tags: \"alt background\", value: 0x16161eff}\n  theme_colour:{tags: \"pop background\", value: 0x292e42ff}\n  theme_colour:{tags: \"fresh background\", value: 0x24283bff}\n  theme_colour:{tags: \"match background\", value: 0x292e42ff}\n  theme_colour:{tags: border, value: 0x2c3147ff}\n  theme_colour:{tags: text, value: 0xc0caf5ff}\n  theme_colour:{tags: \"weak text\", value: 0xa9b1d6ff}\n  theme_colour:{tags: \"good text\", value: 0x9ece6aff}\n  theme_colour:{tags: \"neutral text\", value: 0x7aa2f7ff}\n  theme_colour:{tags: \"bad text\", value: 0xf7768eff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0x7aa2f71c}\n  theme_colour:{tags: \"focus border\", value: 0x7aa2f7ff}\n  theme_colour:{tags: cursor, value: 0xc0caf5ff}\n  theme_colour:{tags: selection, value: 0x7aa2f733}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x1f3d3aff}\n  theme_colour:{tags: \"good_pop border\", value: 0x73daca80}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x4a1d2bff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xf7768eff}\n  theme_colour:{tags: code_default, value: 0xc0caf5ff}\n  theme_colour:{tags: code_symbol, value: 0x7dcfffff}\n  theme_colour:{tags: code_type, value: 0x2ac3deff}\n  theme_colour:{tags: code_local, value: 0xa9b1d6ff}\n  theme_colour:{tags: code_register, value: 0xbb9af7ff}\n  theme_colour:{tags: code_keyword, value: 0xbb9af7ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x89ddffff}\n  theme_colour:{tags: code_numeric, value: 0xff9e64ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0xb8723bff}\n  theme_colour:{tags: code_string, value: 0x9ece6aff}\n  theme_colour:{tags: code_meta, value: 0xf7768eff}\n  theme_colour:{tags: code_comment, value: 0x565f89ff}\n  theme_colour:{tags: line_info_0, value: 0x2a2632ff}\n  theme_colour:{tags: line_info_1, value: 0x332b1eff}\n  theme_colour:{tags: line_info_2, value: 0x1f2e22ff}\n  theme_colour:{tags: line_info_3, value: 0x2e1f23ff}\n  theme_colour:{tags: line_info_4, value: 0x2a2632ff}\n  theme_colour:{tags: line_info_5, value: 0x332b1eff}\n  theme_colour:{tags: line_info_6, value: 0x1f2e22ff}\n  theme_colour:{tags: line_info_7, value: 0x2e1f23ff}\n  theme_colour:{tags: thread_0, value: 0xff9e64ff}\n  theme_colour:{tags: thread_1, value: 0x9ece6aff}\n  theme_colour:{tags: thread_2, value: 0xbb9af7ff}\n  theme_colour:{tags: thread_3, value: 0x7aa2f7ff}\n  theme_colour:{tags: thread_4, value: 0x7dcfffff}\n  theme_colour:{tags: thread_5, value: 0xe0af68ff}\n  theme_colour:{tags: thread_6, value: 0xf7768eff}\n  theme_colour:{tags: thread_7, value: 0x73daca0ff}\n  theme_colour:{tags: thread_unwound, value: 0x565f89ff}\n  theme_colour:{tags: thread_error, value: 0xf7768eff}\n  theme_colour:{tags: breakpoint, value: 0xdb4b4bff}\n  theme_colour:{tags: \"floating background\", value: 0x1a1b26c8}\n  theme_colour:{tags: \"floating background alt\", value: 0x13131a8c}\n  theme_colour:{tags: \"floating background fresh\", value: 0x24283b5f}\n  theme_colour:{tags: \"floating border\", value: 0x7aa2f72f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x16161eff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x2c3147ff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x1a1b26ff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x2c3147ff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x7aa2f7ff}\n  theme_colour:{tags: \"tab border\", value: 0x5a82c7ff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x16161eff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x2c3147ff}\n  theme_colour:{tags: \"tab auto background\", value: 0x2d2541ff}\n  theme_colour:{tags: \"tab auto border\", value: 0xbb9af7ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x1a1727ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x5a4682ff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x282a36ff}\n  theme_colour:{tags: \"alt background\", value: 0x21222cff}\n  theme_colour:{tags: \"pop background\", value: 0x44475aff}\n  theme_colour:{tags: \"fresh background\", value: 0x343746ff}\n  theme_colour:{tags: \"match background\", value: 0x44475aff}\n  theme_colour:{tags: border, value: 0x44475aff}\n  theme_colour:{tags: text, value: 0xf8f8f2ff}\n  theme_colour:{tags: \"weak text\", value: 0xb8b9c2ff}\n  theme_colour:{tags: \"good text\", value: 0x50fa7bff}\n  theme_colour:{tags: \"neutral text\", value: 0x8be9fdff}\n  theme_colour:{tags: \"bad text\", value: 0xff5555ff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0xbd93f91c}\n  theme_colour:{tags: \"focus border\", value: 0xbd93f9ff}\n  theme_colour:{tags: cursor, value: 0xf8f8f2ff}\n  theme_colour:{tags: selection, value: 0xbd93f933}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x234d2eff}\n  theme_colour:{tags: \"good_pop border\", value: 0x50fa7b80}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x5a1d24ff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xff5555ff}\n  theme_colour:{tags: code_default, value: 0xf8f8f2ff}\n  theme_colour:{tags: code_symbol, value: 0x8be9fdff}\n  theme_colour:{tags: code_type, value: 0x8be9fdff}\n  theme_colour:{tags: code_local, value: 0xf8f8f2ff}\n  theme_colour:{tags: code_register, value: 0xbd93f9ff}\n  theme_colour:{tags: code_keyword, value: 0xff79c6ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0xff79c6ff}\n  theme_colour:{tags: code_numeric, value: 0xbd93f9ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x8a6dc1ff}\n  theme_colour:{tags: code_string, value: 0xf1fa8cff}\n  theme_colour:{tags: code_meta, value: 0xffb86cff}\n  theme_colour:{tags: code_comment, value: 0x6272a4ff}\n  theme_colour:{tags: line_info_0, value: 0x3a2c2cff}\n  theme_colour:{tags: line_info_1, value: 0x3a3624ff}\n  theme_colour:{tags: line_info_2, value: 0x253a2eff}\n  theme_colour:{tags: line_info_3, value: 0x2e253aff}\n  theme_colour:{tags: line_info_4, value: 0x3a2c2cff}\n  theme_colour:{tags: line_info_5, value: 0x3a3624ff}\n  theme_colour:{tags: line_info_6, value: 0x253a2eff}\n  theme_colour:{tags: line_info_7, value: 0x2e253aff}\n  theme_colour:{tags: thread_0, value: 0xffb86cff}\n  theme_colour:{tags: thread_1, value: 0x50fa7bff}\n  theme_colour:{tags: thread_2, value: 0xff79c6ff}\n  theme_colour:{tags: thread_3, value: 0x8be9fdff}\n  theme_colour:{tags: thread_4, value: 0xbd93f9ff}\n  theme_colour:{tags: thread_5, value: 0xf1fa8cff}\n  theme_colour:{tags: thread_6, value: 0xff5555ff}\n  theme_colour:{tags: thread_7, value: 0x6272a4ff}\n  theme_colour:{tags: thread_unwound, value: 0x6272a4ff}\n  theme_colour:{tags: thread_error, value: 0xff5555ff}\n  theme_colour:{tags: breakpoint, value: 0xff5555ff}\n  theme_colour:{tags: \"floating background\", value: 0x282a36c8}\n  theme_colour:{tags: \"floating background alt\", value: 0x1c1d258c}\n  theme_colour:{tags: \"floating background fresh\", value: 0x343746af}\n  theme_colour:{tags: \"floating border\", value: 0xbd93f92f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x21222cff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x44475aff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x21222cff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x44475aff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0xbd93f9ff}\n  theme_colour:{tags: \"tab border\", value: 0x9779cdff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x21222cff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x44475aff}\n  theme_colour:{tags: \"tab auto background\", value: 0x4a3a52ff}\n  theme_colour:{tags: \"tab auto border\", value: 0xff79c6ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x2a212eff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x8b3f6dff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x2e3440ff}\n  theme_colour:{tags: \"alt background\", value: 0x3b4252ff}\n  theme_colour:{tags: \"pop background\", value: 0x434c5eff}\n  theme_colour:{tags: \"fresh background\", value: 0x3b4252ff}\n  theme_colour:{tags: \"match background\", value: 0x434c5eff}\n  theme_colour:{tags: border, value: 0x4c566aff}\n  theme_colour:{tags: text, value: 0xeceff4ff}\n  theme_colour:{tags: \"weak text\", value: 0xd8dee9ff}\n  theme_colour:{tags: \"good text\", value: 0xa3be8cff}\n  theme_colour:{tags: \"neutral text\", value: 0x88c0d0ff}\n  theme_colour:{tags: \"bad text\", value: 0xbf616aff}\n  theme_colour:{tags: hover, value: 0xeceff4ff}\n  theme_colour:{tags: \"focus overlay\", value: 0x88c0d01c}\n  theme_colour:{tags: \"focus border\", value: 0x88c0d0ff}\n  theme_colour:{tags: cursor, value: 0xd8dee9ff}\n  theme_colour:{tags: selection, value: 0x434c5e80}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x3a4a3aff}\n  theme_colour:{tags: \"good_pop border\", value: 0xa3be8c80}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x4a2c30ff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xbf616aff}\n  theme_colour:{tags: code_default, value: 0xd8dee9ff}\n  theme_colour:{tags: code_symbol, value: 0x8fbcbbff}\n  theme_colour:{tags: code_type, value: 0x8fbcbbff}\n  theme_colour:{tags: code_local, value: 0xd8dee9ff}\n  theme_colour:{tags: code_register, value: 0xb48eadff}\n  theme_colour:{tags: code_keyword, value: 0x81a1c1ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x81a1c1ff}\n  theme_colour:{tags: code_numeric, value: 0xb48eadff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x8a6f87ff}\n  theme_colour:{tags: code_string, value: 0xa3be8cff}\n  theme_colour:{tags: code_meta, value: 0xd08770ff}\n  theme_colour:{tags: code_comment, value: 0x616e88ff}\n  theme_colour:{tags: line_info_0, value: 0x3e342aff}\n  theme_colour:{tags: line_info_1, value: 0x3e3624ff}\n  theme_colour:{tags: line_info_2, value: 0x34402aff}\n  theme_colour:{tags: line_info_3, value: 0x2e2a3aff}\n  theme_colour:{tags: line_info_4, value: 0x3e342aff}\n  theme_colour:{tags: line_info_5, value: 0x3e3624ff}\n  theme_colour:{tags: line_info_6, value: 0x34402aff}\n  theme_colour:{tags: line_info_7, value: 0x2e2a3aff}\n  theme_colour:{tags: thread_0, value: 0xd08770ff}\n  theme_colour:{tags: thread_1, value: 0xa3be8cff}\n  theme_colour:{tags: thread_2, value: 0xb48eadff}\n  theme_colour:{tags: thread_3, value: 0x81a1c1ff}\n  theme_colour:{tags: thread_4, value: 0x88c0d0ff}\n  theme_colour:{tags: thread_5, value: 0xebcb8bff}\n  theme_colour:{tags: thread_6, value: 0xbf616aff}\n  theme_colour:{tags: thread_7, value: 0x8fbcbbff}\n  theme_colour:{tags: thread_unwound, value: 0x4c566aff}\n  theme_colour:{tags: thread_error, value: 0xbf616aff}\n  theme_colour:{tags: breakpoint, value: 0xbf616aff}\n  theme_colour:{tags: \"floating background\", value: 0x2e3440c8}\n  theme_colour:{tags: \"floating background alt\", value: 0x242832af}\n  theme_colour:{tags: \"floating background fresh\", value: 0x3b42525f}\n  theme_colour:{tags: \"floating border\", value: 0x88c0d02f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x3b4252ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x4c566aff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x3b4252ff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x4c566aff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0x88c0d0ff}\n  theme_colour:{tags: \"tab border\", value: 0x5e81acff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x3b4252ff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x4c566aff}\n  theme_colour:{tags: \"tab auto background\", value: 0x4a3a4eff}\n  theme_colour:{tags: \"tab auto border\", value: 0xb48eadff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x2c252eff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x6a4d68ff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x1e1e2eff}\n  theme_colour:{tags: \"alt background\", value: 0x181825ff}\n  theme_colour:{tags: \"pop background\", value: 0x313244ff}\n  theme_colour:{tags: \"fresh background\", value: 0x313244ff}\n  theme_colour:{tags: \"match background\", value: 0x45475aff}\n  theme_colour:{tags: border, value: 0x45475aff}\n  theme_colour:{tags: text, value: 0xcdd6f4ff}\n  theme_colour:{tags: \"weak text\", value: 0xa6adc8ff}\n  theme_colour:{tags: \"good text\", value: 0xa6e3a1ff}\n  theme_colour:{tags: \"neutral text\", value: 0x89b4faff}\n  theme_colour:{tags: \"bad text\", value: 0xf38ba8ff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0xcba6f71c}\n  theme_colour:{tags: \"focus border\", value: 0xcba6f7ff}\n  theme_colour:{tags: cursor, value: 0xf5e0dcff}\n  theme_colour:{tags: selection, value: 0x585b7033}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x2d3a2cff}\n  theme_colour:{tags: \"good_pop border\", value: 0xa6e3a180}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x4a1d2eff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xf38ba8ff}\n  theme_colour:{tags: code_default, value: 0xcdd6f4ff}\n  theme_colour:{tags: code_symbol, value: 0x89dceeff}\n  theme_colour:{tags: code_type, value: 0xf9e2afff}\n  theme_colour:{tags: code_local, value: 0xcdd6f4ff}\n  theme_colour:{tags: code_register, value: 0xcba6f7ff}\n  theme_colour:{tags: code_keyword, value: 0xcba6f7ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0x94e2d5ff}\n  theme_colour:{tags: code_numeric, value: 0xfab387ff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0xb07f4dff}\n  theme_colour:{tags: code_string, value: 0xa6e3a1ff}\n  theme_colour:{tags: code_meta, value: 0xf5c2e7ff}\n  theme_colour:{tags: code_comment, value: 0x6c7086ff}\n  theme_colour:{tags: line_info_0, value: 0x3a2e2aff}\n  theme_colour:{tags: line_info_1, value: 0x3a3624ff}\n  theme_colour:{tags: line_info_2, value: 0x2e3a2cff}\n  theme_colour:{tags: line_info_3, value: 0x2e253aff}\n  theme_colour:{tags: line_info_4, value: 0x3a2e2aff}\n  theme_colour:{tags: line_info_5, value: 0x3a3624ff}\n  theme_colour:{tags: line_info_6, value: 0x2e3a2cff}\n  theme_colour:{tags: line_info_7, value: 0x2e253aff}\n  theme_colour:{tags: thread_0, value: 0xfab387ff}\n  theme_colour:{tags: thread_1, value: 0xa6e3a1ff}\n  theme_colour:{tags: thread_2, value: 0xf5c2e7ff}\n  theme_colour:{tags: thread_3, value: 0x89b4faff}\n  theme_colour:{tags: thread_4, value: 0x94e2d5ff}\n  theme_colour:{tags: thread_5, value: 0xf9e2afff}\n  theme_colour:{tags: thread_6, value: 0xf38ba8ff}\n  theme_colour:{tags: thread_7, value: 0xcba6f7ff}\n  theme_colour:{tags: thread_unwound, value: 0x6c7086ff}\n  theme_colour:{tags: thread_error, value: 0xf38ba8ff}\n  theme_colour:{tags: breakpoint, value: 0xf38ba8ff}\n  theme_colour:{tags: \"floating background\", value: 0x1e1e2ec8}\n  theme_colour:{tags: \"floating background alt\", value: 0x11111baf}\n  theme_colour:{tags: \"floating background fresh\", value: 0x313244af}\n  theme_colour:{tags: \"floating border\", value: 0xcba6f72f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x181825ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x45475aff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x181825ff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x45475aff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0xcba6f7ff}\n  theme_colour:{tags: \"tab border\", value: 0xa685cdff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x181825ff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x313244ff}\n  theme_colour:{tags: \"tab auto background\", value: 0x4a2e3aff}\n  theme_colour:{tags: \"tab auto border\", value: 0xf5c2e7ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x2a1d24ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x8b5670ff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n"),
    Str8Lit("theme:\n{\n  theme_colour:{tags: background, value: 0x272822ff}\n  theme_colour:{tags: \"alt background\", value: 0x2d2e27ff}\n  theme_colour:{tags: \"pop background\", value: 0x3e3d32ff}\n  theme_colour:{tags: \"fresh background\", value: 0x3e3d32ff}\n  theme_colour:{tags: \"match background\", value: 0x49483eff}\n  theme_colour:{tags: border, value: 0x49483eff}\n  theme_colour:{tags: text, value: 0xf8f8f2ff}\n  theme_colour:{tags: \"weak text\", value: 0xcfcfc2ff}\n  theme_colour:{tags: \"good text\", value: 0xa6e22eff}\n  theme_colour:{tags: \"neutral text\", value: 0x66d9efff}\n  theme_colour:{tags: \"bad text\", value: 0xf92672ff}\n  theme_colour:{tags: hover, value: 0xffffffff}\n  theme_colour:{tags: \"focus overlay\", value: 0xfd971f1c}\n  theme_colour:{tags: \"focus border\", value: 0xfd971fff}\n  theme_colour:{tags: cursor, value: 0xf8f8f0ff}\n  theme_colour:{tags: selection, value: 0x49483eff}\n  theme_colour:{tags: \"inactive background\", value: 0x0000002f}\n  theme_colour:{tags: drop_shadow, value: 0x0000007f}\n  theme_colour:{tags: \"good_pop background\", value: 0x3a4a23ff}\n  theme_colour:{tags: \"good_pop border\", value: 0xa6e22e80}\n  theme_colour:{tags: \"good_pop hover\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"good_pop weak text\", value: 0xe3f5d3ff}\n  theme_colour:{tags: \"bad_pop background\", value: 0x5a1d2cff}\n  theme_colour:{tags: \"bad_pop hover\", value: 0xf92672ff}\n  theme_colour:{tags: code_default, value: 0xf8f8f2ff}\n  theme_colour:{tags: code_symbol, value: 0xa6e22eff}\n  theme_colour:{tags: code_type, value: 0x66d9efff}\n  theme_colour:{tags: code_local, value: 0xf8f8f2ff}\n  theme_colour:{tags: code_register, value: 0xae81ffff}\n  theme_colour:{tags: code_keyword, value: 0xf92672ff}\n  theme_colour:{tags: code_delimiter_or_operator, value: 0xf92672ff}\n  theme_colour:{tags: code_numeric, value: 0xae81ffff}\n  theme_colour:{tags: code_numeric_alt_digit_group, value: 0x7e5fc1ff}\n  theme_colour:{tags: code_string, value: 0xe6db74ff}\n  theme_colour:{tags: code_meta, value: 0xfd971fff}\n  theme_colour:{tags: code_comment, value: 0x75715eff}\n  theme_colour:{tags: line_info_0, value: 0x3e3022ff}\n  theme_colour:{tags: line_info_1, value: 0x3e3a15ff}\n  theme_colour:{tags: line_info_2, value: 0x334a2aff}\n  theme_colour:{tags: line_info_3, value: 0x2e221fff}\n  theme_colour:{tags: line_info_4, value: 0x3e3022ff}\n  theme_colour:{tags: line_info_5, value: 0x3e3a15ff}\n  theme_colour:{tags: line_info_6, value: 0x334a2aff}\n  theme_colour:{tags: line_info_7, value: 0x2e221fff}\n  theme_colour:{tags: thread_0, value: 0xfd971fff}\n  theme_colour:{tags: thread_1, value: 0xa6e22eff}\n  theme_colour:{tags: thread_2, value: 0xf92672ff}\n  theme_colour:{tags: thread_3, value: 0x66d9efff}\n  theme_colour:{tags: thread_4, value: 0xae81ffff}\n  theme_colour:{tags: thread_5, value: 0xe6db74ff}\n  theme_colour:{tags: thread_6, value: 0xfd5ff8ff}\n  theme_colour:{tags: thread_7, value: 0xa6e22eff}\n  theme_colour:{tags: thread_unwound, value: 0x75715eff}\n  theme_colour:{tags: thread_error, value: 0xf92672ff}\n  theme_colour:{tags: breakpoint, value: 0xf92672ff}\n  theme_colour:{tags: \"floating background\", value: 0x272822c8}\n  theme_colour:{tags: \"floating background alt\", value: 0x1c1d188c}\n  theme_colour:{tags: \"floating background fresh\", value: 0x3e3d325f}\n  theme_colour:{tags: \"floating border\", value: 0xfd971f2f}\n  theme_colour:{tags: \"floating scroll_bar background\", value: 0x3b3b3b5f}\n  theme_colour:{tags: \"floating scroll_bar border\", value: 0x5f5f5f5f}\n  theme_colour:{tags: \"menu_bar background\", value: 0x2d2e27ff}\n  theme_colour:{tags: \"menu_bar border\", value: 0x49483eff}\n  theme_colour:{tags: \"scroll_bar background\", value: 0x2d2e27ff}\n  theme_colour:{tags: \"scroll_bar border\", value: 0x49483eff}\n  theme_colour:{tags: \"implicit background\", value: 0x00000000}\n  theme_colour:{tags: \"implicit border\", value: 0x00000000}\n  theme_colour:{tags: \"hollow background\", value: 0x00000000}\n  theme_colour:{tags: \"hollow border\", value: 0xffffff1f}\n  theme_colour:{tags: \"tab background\", value: 0xfd971fff}\n  theme_colour:{tags: \"tab border\", value: 0xcd771fff}\n  theme_colour:{tags: \"tab inactive background\", value: 0x2d2e27ff}\n  theme_colour:{tags: \"tab inactive border\", value: 0x49483eff}\n  theme_colour:{tags: \"tab auto background\", value: 0x4a2c44ff}\n  theme_colour:{tags: \"tab auto border\", value: 0xf92672ff}\n  theme_colour:{tags: \"tab auto inactive background\", value: 0x2a1d27ff}\n  theme_colour:{tags: \"tab auto inactive border\", value: 0x8b3f5cff}\n  theme_colour:{tags: \"drop_site background\", value: 0xffffff05}\n  theme_colour:{tags: \"drop_site border\", value: 0xffffff0f}\n}\n")
};

AppNameSchemaInfo APP_NAME_SCHEMA_INFO_TABLE[27] = {
    {
        Str8Lit("session"),
        FALSE,
        Str8Lit("@expand_commands(edit_session_theme) x:\n{\n @display_name('Animations') @description(\"Enables animations.\")\n\n@default(1) 'animations': bool,\n@display_name('Scrolling Animations') @description(\"Enables scrolling animations.\")\n@expand_if(\"$.animations\") @default(1) 'scrolling_animations': bool,\n@display_name('Tooltip Animations') @description(\"Enables tooltip animations.\")\n@expand_if(\"$.animations\") @default(1) 'tooltip_animations': bool,\n@display_name('Menu Animations') @description(\"Enables menu animations.\")\n@expand_if(\"$.animations\") @default(1) 'menu_animations': bool,\n\n@display_name('UI Font') @description(\"The name of, or path to, the font used when displaying non-code UI elements.\")\n@default('') 'main_font': string,\n@display_name('Code Font') @description(\"The name of, or path to, the font used when displaying code.\")\n@default('') 'code_font': string,\n\n@no_expand @default('query:commands') 'cmd': string,\n\n@default(\"Ayu Dark\") @display_name('Theme')\n@description(\"The session theme, which describes all colours used throughout the UI.\")\n'theme': string,\n@no_expand @display_name('Theme Colours')\n'theme_colours': query,\n\n@display_name('Autocompletion Lister') @description(\"Enables the autocompletion lister while typing expressions.\") @default(1)\n'autocompletion_lister': bool,\n\n@default(1) @display_name('Cursor Trail') @description(\"Shows a trail after the cursor as it moves\")\n'cursor_trail': bool,\n\n@default(0) @display_name('Opaque Backgrounds') @description(\"Controls whether or not all floating background colours are forced to be fully opaque.\")\n'opaque_backgrounds': bool,\n@default(1) @display_name('Background Blur') @description(\"Controls whether or not occluded regions behind floating elements are blurred.\")\n'background_blur': bool,\n\n@default(1) @display_name('Drop Shadows') @description(\"Controls whether or not drop shadows are drawn.\")\n'drop_shadows': bool,\n@default(1.0f) @display_name('Rounded Corner Amount') @description(\"Controls the degree to which UI corners are rounded.\")\n'rounded_corner_amount': @range[0, 1] f32,\n\n@default(2) @display_name('Tab Width') 'tab_width': @range[1, 32] u64,\n\n@default(1) @display_name('Focus Menu Bar With Alt') @description(\"Mimics standard Windows behavior of focusing the menu bar using the Alt key.\")\n'focus_menu_bar_with_alt': bool,\n\n@default(0) @display_name('Use Native File System Dialog') @description(\"Uses the operating system's file system dialog box, rather than the application's built-in UI.\")\n'use_native_file_system_dialog': bool,\n}\n")
    },
    {
        Str8Lit("theme_colour"),
        FALSE,
        Str8Lit("@collection_commands(add_theme_colour, fork_theme, save_theme, save_and_set_theme)\n@row_commands(duplicate_cfg, remove_cfg)\nx:\n{\n  @display_name('Tags') tags: string,\n  @display_name('Value') value: @colour @hex u32,\n}\n")
    },
    {
        Str8Lit("window"),
        FALSE,
        Str8Lit("x:\n{\n  @default(1) @display_name('Smooth UI Text') @description(\"Controls whether or not UI text is fully anti-aliased, for a smoother appearance.\")\n  'smooth_ui_text': bool,\n  @default(1) @display_name('Hint UI Text') @description(\"Controls whether or not UI text is hinted, for better text readability at small sizes.\")\n  'hint_ui_text': bool,\n  @default(0) @display_name('Smooth Code Text') @description(\"Controls whether or not code text is fully anti-aliased, for a smoother appearance.\")\n  'smooth_code_text': bool,\n  @default(1) @display_name('Hint Code Text') @description(\"Controls whether or not code text is hinted, for better text readability at small sizes.\")\n  'hint_code_text': bool,\n  @default(12.0f) @display_name('Window Font Size') @description(\"Controls the window's default font size. Does not apply to tabs with their own font size set.\")\n  'font_size': @range[6.0f, 72.0f] f32,\n\n  @default(3.0f) @display_name('Window Row Height') @description(\"Controls the window's default row height, in multiples of the font size. Does not apply to tabs with their own row height set.\")\n  'row_height': @range[1.75f, 5.0f] f32,\n  @default(3.0f) @description(\"Controls the height of tabs, in multiples of the font size.\")\n  'tab_height': @range[1.75f, 5.0f] f32,\n\n  @default(1) @display_name('Use Session Theme') @description(\"Prefer using the session theme for this window, if any. If off, only the session theme settings will be used.\")\n  'use_session_theme': bool,\n}\n")
    },
    {
        Str8Lit("tab"),
        FALSE,
        Str8Lit("@row_commands(@file copy_tab_full_path, @file show_file_in_explorer, duplicate_tab, close_tab)\nx:\n{\n  @override @display_name('Tab Font Size') @description(\"Controls the tab's font size.\") @no_callee_helper\n  'font_size': @range[6.0f, 72.0f] f32,\n}\n")
    },
    {
        Str8Lit("bin"), 
        TRUE, 
        Str8Lit("x:{ @description('An expression to which the 2-radix display should apply.') 'expression': expr_string }")
    },
    {
        Str8Lit("oct"), 
        TRUE, 
        Str8Lit("x:{ @description('An expression to which the 8-radix display should apply.') 'expression': expr_string }")
    },
    {
        Str8Lit("hex"), 
        TRUE, 
        Str8Lit("x:{ @description('An expression to which the 16-radix display should apply.') 'expression': expr_string }")
    },
    {
        Str8Lit("digits"), 
        TRUE, 
        Str8Lit("x:{ @description('An expression to which the digit display should apply.') 'expression': expr_string, @description('The minimum number of digits to display.') 'digit_count': u64 }")
    },
    {
        Str8Lit("no_string"), 
        TRUE, 
        Str8Lit("x:{ @description('The expression for which string visualization should be disabled.') 'expression': expr_string}")
    },
    {
        Str8Lit("no_char"), 
        TRUE, 
        Str8Lit("x:{ @description('The expression for which character visualization should be disabled.') 'expression': expr_string}")
    },
    {
        Str8Lit("no_addr"), 
        TRUE, 
        Str8Lit("x:{ @description('The expression for which address visualization should be disabled.') 'expression': expr_string}")
    },
    {
        Str8Lit("sequence"), 
        TRUE, 
        Str8Lit("x:{ @description('The number of expansions to generate.') 'count': expr_string}")
    },
    {
        Str8Lit("rows"), 
        TRUE, 
        Str8Lit("x:{ @description('The expression for which rows should be generated.') 'expression': expr_string, @description('A list of expressions which will be used to generate rows.') '...'}")
    },
    {
        Str8Lit("columns"), 
        TRUE, 
        Str8Lit("x:{ @description('The expression for whose expansions columns should be generated.') 'expression': expr_string, @description('A list of expressions which will be used to generate columns for each expansion from `expression`.') '...'}")
    },
    {
        Str8Lit("range1"), 
        TRUE, 
        Str8Lit("x:{ @description('The expression which should be bounded by `min` and `max`.') 'expression': expr_string, @description('The lower bound.') min, @description('The upper bound.') max}")
    },
    {
        Str8Lit("array"), 
        TRUE, 
        Str8Lit("x:{ @description('An expression of the base address of the array.') 'base_address': expr_string, @description('The number of elements in the array.') count}")
    },
    {
        Str8Lit("text"),
        TRUE,
        Str8Lit("@inherit(tab) @expand_commands(@output clear_output) x:\n{\n  @description(\"An expression to describe data which should be viewed as text or code.\")\n  'expression': expr_string,\n  @description(\"The language that the text should be interpreted as being within. Used for syntax highlighting and other parsing features.\")\n  'lang': code_string,\n  @default(1) @description(\"Controls whether or not line numbers are shown.\")\n  'show_line_numbers':bool,\n  @no_callee_helper @default(1) @display_name('Line Wrapping') @description(\"Splits textual lines into multiple visual lines, so that all text is within the visible area.\")\n  'line_wrapping': bool,\n  @no_callee_helper @default(0) @display_name('Scroll To Bottom On Change') @description(\"Scrolls to the bottom if the text is changed.\")\n  'scroll_to_bottom_on_change': bool,\n  @no_callee_helper @no_revert @default(0) @display_name('Transient') @description(\"Controls whether or not this tab will be automatically replaced by the debugger when it snaps to new source code locations.\")\n  'auto': bool,\n@no_callee_helper @default(1) @display_name('Show Cursor Trail') @description(\"Shows cursor trail when cursor position is changed.\")\n  'cursor_trail': bool,\n}\n")
    },
    {
        Str8Lit("bitmap"),
        TRUE,
        Str8Lit("@inherit(tab) x:\n{\n  @description(\"An expression which refers to the base address of data which should be viewed as a bitmap.\")\n  'expression': expr_string,\n  @description(\"An expression describing the width of the bitmap, in pixels.\") @order(0) 'w': u64,\n  @description(\"An expression describing the height of the bitmap, in pixels.\") @order(1) 'h': u64,\n  @display_name(\"Bitmap Format\") @description(\"The pixel format that the bitmap data should be interpreted as being within.\")\n  'fmt': code_string,\n}\n")
    },
    {
        Str8Lit("colour"),
        TRUE,
        Str8Lit("@inherit(tab) x:\n{\n  @display_name(\"Value\") @description(\"An expression to describe the value or location of the colour.\")\n  'expression': expr_string,\n}\n")
    },
    {
        Str8Lit("getting_started"),
        FALSE,
        Str8Lit("@inherit(tab) x:\n{\n}\n")
    },
    {
        Str8Lit("file_path_map"),
        FALSE,
        Str8Lit("@collection_commands(add_file_path_map) @row_commands(remove_cfg) x:{'source': @no_relativise path, 'dest': @no_relativise path}")
    },
    {
        Str8Lit("recent_session"),
        FALSE,
        Str8Lit("x:{'path':path}")
    },
    {
        Str8Lit("recent_file"),
        FALSE,
        Str8Lit("x:{'path':path}")
    },
    {
        Str8Lit("watch"), 
        FALSE, 
        Str8Lit("@inherit(tab) x:\n{\n  @override @display_name('Tab Row Height') @description(\"Controls the tab's row height, in multiples of the font size.\")\n  'row_height': @range[1.75f, 5.f] f32,\n  'label': code_string,\n  @description(\"The root expression which is evaluated to produce the watch window.\")\n  'expression': expr_string,\n  @no_expand 'watches': set,\n}\n")
    },
    {
        Str8Lit("geo3d"), 
        TRUE, 
        Str8Lit("@inherit(tab) x:\n{\n  @display_name(\"Expression\") @description(\"An expression to describe the base address of the index buffer.\")\n  'expression': expr_string,\n  'count': expr_string,\n  'vtx': expr_string,\n  'vtx_size': expr_string,\n  'yaw': @range[0, 1] f32,\n  'pitch': @range[-0.5, 0] f32,\n  'zoom': @range[0, 100] f32,\n}\n")
    }
};

Str8 APP_REGISTERS_SLOT_CODE_NAME_TABLE[APP_REGISTERS_SLOT_COUNT] = {
    {},
    Str8Lit("Window"),
    Str8Lit("Panel"),
    Str8Lit("Tab"),
    Str8Lit("View"),
    Str8Lit("PrevTab"),
    Str8Lit("DstPanel"),
    Str8Lit("Config"),
    Str8Lit("ConfigList"),
    Str8Lit("Space"),
    Str8Lit("FilePath"),
    Str8Lit("Cursor"),
    Str8Lit("Mark"),
    Str8Lit("TextKey"),
    Str8Lit("Expression"),
    Str8Lit("UI"),
    Str8Lit("SrcUI"),
    Str8Lit("OffsetPX"),
    Str8Lit("RegSlot"),
    Str8Lit("ContextExpression"),
    Str8Lit("DoImplicitRoot"),
    Str8Lit("DoLister"),
    Str8Lit("DoSmall"),
    Str8Lit("DoBigRows"),
    Str8Lit("AllWindows"),
    Str8Lit("ActivateWithSingleClick"),
    Str8Lit("ForceConfirm"),
    Str8Lit("String"),
    Str8Lit("CommandName"),
    Str8Lit("Direction"),
    Str8Lit("OSEvent")
};

r1u64 APP_REGISTERS_SLOT_RANGE_TABLE[APP_REGISTERS_SLOT_COUNT] = {
    {},
    {
        OFFSETOF(AppRegisters, Window),
        OFFSETOF(AppRegisters, Window) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, Panel),
        OFFSETOF(AppRegisters, Panel) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, Tab),
        OFFSETOF(AppRegisters, Tab) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, View),
        OFFSETOF(AppRegisters, View) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, PrevTab),
        OFFSETOF(AppRegisters, PrevTab) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, DstPanel),
        OFFSETOF(AppRegisters, DstPanel) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, Config),
        OFFSETOF(AppRegisters, Config) + sizeof(ConfigID)
    },
    {
        OFFSETOF(AppRegisters, ConfigList),
        OFFSETOF(AppRegisters, ConfigList) + sizeof(ConfigIDList)
    },
    {
        OFFSETOF(AppRegisters, Space),
        OFFSETOF(AppRegisters, Space) + sizeof(EvalSpace)
    },
    {
        OFFSETOF(AppRegisters, FilePath),
        OFFSETOF(AppRegisters, FilePath) + sizeof(Str8)
    },
    {
        OFFSETOF(AppRegisters, Cursor),
        OFFSETOF(AppRegisters, Cursor) + sizeof(TextPoint)
    },
    {
        OFFSETOF(AppRegisters, Mark),
        OFFSETOF(AppRegisters, Mark) + sizeof(TextPoint)
    },
    {
        OFFSETOF(AppRegisters, TextKey),
        OFFSETOF(AppRegisters, TextKey) + sizeof(CKey)
    },
    {
        OFFSETOF(AppRegisters, Expression),
        OFFSETOF(AppRegisters, Expression) + sizeof(Str8)
    },
    {
        OFFSETOF(AppRegisters, UI),
        OFFSETOF(AppRegisters, UI) + sizeof(UIKey)
    },
    {
        OFFSETOF(AppRegisters, SrcUI),
        OFFSETOF(AppRegisters, SrcUI) + sizeof(UIKey)
    },
    {
        OFFSETOF(AppRegisters, OffsetPX),
        OFFSETOF(AppRegisters, OffsetPX) + sizeof(v2f32)
    },
    {
        OFFSETOF(AppRegisters, RegSlot),
        OFFSETOF(AppRegisters, RegSlot) + sizeof(AppRegistersSlot)
    },
    {
        OFFSETOF(AppRegisters, ContextExpression),
        OFFSETOF(AppRegisters, ContextExpression) + sizeof(Str8)
    },
    {
        OFFSETOF(AppRegisters, DoImplicitRoot),
        OFFSETOF(AppRegisters, DoImplicitRoot) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, DoLister),
        OFFSETOF(AppRegisters, DoLister) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, DoSmall),
        OFFSETOF(AppRegisters, DoSmall) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, DoBigRows),
        OFFSETOF(AppRegisters, DoBigRows) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, AllWindows),
        OFFSETOF(AppRegisters, AllWindows) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, ActivateWithSingleClick),
        OFFSETOF(AppRegisters, ActivateWithSingleClick) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, ForceConfirm),
        OFFSETOF(AppRegisters, ForceConfirm) + sizeof(b32)
    },
    {
        OFFSETOF(AppRegisters, String),
        OFFSETOF(AppRegisters, String) + sizeof(Str8)
    },
    {
        OFFSETOF(AppRegisters, CommandName),
        OFFSETOF(AppRegisters, CommandName) + sizeof(Str8)
    },
    {
        OFFSETOF(AppRegisters, Direction),
        OFFSETOF(AppRegisters, Direction) + sizeof(Direction2D)
    },
    {
        OFFSETOF(AppRegisters, OSEvent),
        OFFSETOF(AppRegisters, OSEvent) + sizeof(InputEvent*)
    }
};


Str8 APP_TAB_FAST_PATH_VIEW_NAME_TABLE[APP_COMMAND_KIND_COUNT - APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD] = {
    Str8Lit("watch"),
    Str8Lit("watch"),
    Str8Lit("watch"),
    Str8Lit("text"),
    Str8Lit("bitmap"),
    Str8Lit("colour"),
    Str8Lit("geo3d")
};

Str8 APP_TAB_FAST_PATH_QUERY_NAME[APP_COMMAND_KIND_COUNT - APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD] = {
    Str8Lit(""),
    Str8Lit("query:file_path_maps"),
    Str8Lit("query:types"),
    Str8Lit(""),
    Str8Lit(""),
    Str8Lit(""),
    Str8Lit("")
};

AppVocabInfo APP_VOCAB_INFO_TABLE[232] = {
    {
        Str8Lit("file_path_map"),
        Str8Lit("file_path_maps"),
        Str8Lit("File Path Map"),
        Str8Lit("File Path Maps"),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("view"),
        Str8Lit("views"),
        Str8Lit("View"),
        Str8Lit("Views"),
        APP_ICON_KIND_BINOCULARS 
    },
    { 
        Str8Lit("location"),
        Str8Lit("locations"),
        Str8Lit("Location"),
        Str8Lit("Locations"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("colour"),
        Str8Lit("colours"),
        Str8Lit("Colour"),
        Str8Lit("Colours"),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("theme_colour"),
        Str8Lit("theme_colours"),
        Str8Lit("Theme Colour"),
        Str8Lit("Theme Colours"),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("min"),
        Str8Lit("mins"),
        Str8Lit("Minimum"),
        Str8Lit("Minimums"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("max"),
        Str8Lit("maxs"),
        Str8Lit("Maximum"),
        Str8Lit("Maximums"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("window"),
        Str8Lit("windows"),
        Str8Lit("Window"),
        Str8Lit("Windows"),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("panel"),
        Str8Lit("panels"),
        Str8Lit("Panel"),
        Str8Lit("Panels"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("tab"),
        Str8Lit("tabs"),
        Str8Lit("Tab"),
        Str8Lit("Tabs"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("recent_file"),
        Str8Lit("recent_files"),
        Str8Lit("Recent File"),
        Str8Lit("Recent Files"),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("src"),
        Str8Lit("srcs"),
        Str8Lit("Source"),
        Str8Lit("Sources"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("dst"),
        Str8Lit("dsts"),
        Str8Lit("Destination"),
        Str8Lit("Destinations"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("source"),
        Str8Lit("sources"),
        Str8Lit("Source"),
        Str8Lit("Sources"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("dest"),
        Str8Lit("dests"),
        Str8Lit("Destination"),
        Str8Lit("Destinations"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("conversion_task"),
        Str8Lit("conversion_tasks"),
        Str8Lit("Conversion Task"),
        Str8Lit("Conversion Tasks"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("conversion_fail"),
        Str8Lit("conversion_fails"),
        Str8Lit("Conversion Fail"),
        Str8Lit("Conversion Fails"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("expr"),
        Str8Lit("exprs"),
        Str8Lit("Expression"),
        Str8Lit("Expressions"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("expression"),
        Str8Lit("expressions"),
        Str8Lit("Expression"),
        Str8Lit("Expressions"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("size"),
        Str8Lit("sizes"),
        Str8Lit("Size"),
        Str8Lit("Sizes"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("count"),
        Str8Lit("counts"),
        Str8Lit("Count"),
        Str8Lit("Counts"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("bool"),
        Str8Lit("bools"),
        Str8Lit("Boolean"),
        Str8Lit("Booleans"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("w"),
        Str8Lit("ws"),
        Str8Lit("Width"),
        Str8Lit("Widths"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("h"),
        Str8Lit("hs"),
        Str8Lit("Height"),
        Str8Lit("Heights"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("fmt"),
        Str8Lit("fmts"),
        Str8Lit("Format"),
        Str8Lit("Formats"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("vtx"),
        Str8Lit("vtxs"),
        Str8Lit("Vertex Buffer"),
        Str8Lit("Vertex Buffers"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("vtx_size"),
        Str8Lit("vtx_sizes"),
        Str8Lit("Vertex Buffer Size"),
        Str8Lit("Vertex Buffer Sizes"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("label"),
        Str8Lit("labels"),
        Str8Lit("Label"),
        Str8Lit("Labels"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("getting_started"),
        Str8Lit(""),
        Str8Lit("Getting Started"),
        Str8Lit(""),
        APP_ICON_KIND_QUESTION_MARK 
    },
    { 
        Str8Lit("text"),
        Str8Lit(""),
        Str8Lit("Text"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("type"),
        Str8Lit("types"),
        Str8Lit("Type"),
        Str8Lit("Types"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("output"),
        Str8Lit("outputs"),
        Str8Lit("Output"),
        Str8Lit("Outputs"),
        APP_ICON_KIND_LIST 
    },
    { 
        Str8Lit("enabled"),
        Str8Lit(""),
        Str8Lit("Enabled"),
        Str8Lit("Enabled"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("disabled"),
        Str8Lit(""),
        Str8Lit("Disabled"),
        Str8Lit("Disabled"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("id"),
        Str8Lit("ids"),
        Str8Lit("ID"),
        Str8Lit("IDs"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("last_modified_time"),
        Str8Lit("last_modified_times"),
        Str8Lit("Last Modified Time"),
        Str8Lit("Last Modified Times"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("creation_time"),
        Str8Lit("creation_times"),
        Str8Lit("Creation Time"),
        Str8Lit("Creation Times"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("data"),
        Str8Lit("datas"),
        Str8Lit("Data"),
        Str8Lit("Datas"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("session"),
        Str8Lit("sessions"),
        Str8Lit("Session"),
        Str8Lit("Sessions"),
        APP_ICON_KIND_BRIEFCASE
    },
    { 
        Str8Lit("recent_session"),
        Str8Lit("recent_sessions"),
        Str8Lit("Recent Session"),
        Str8Lit("Recent Sessions"),
        APP_ICON_KIND_BRIEFCASE 
    },
    { 
        Str8Lit("recent_file"),
        Str8Lit("recent_files"),
        Str8Lit("Recent File"),
        Str8Lit("Recent Files"),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("syntax"),
        Str8Lit("syntaxes"),
        Str8Lit("Syntax"),
        Str8Lit("Syntaxes"),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("num_columns"),
        Str8Lit(""),
        Str8Lit("Number of Columns"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("bitmap"),
        Str8Lit("bitmaps"),
        Str8Lit("Bitmap"),
        Str8Lit("Bitmaps"),
        APP_ICON_KIND_BITMAP 
    },
    { 
        Str8Lit("geo3d"),
        Str8Lit(""),
        Str8Lit("Geometry (3D)"),
        Str8Lit(""),
        APP_ICON_KIND_CUBE 
    },
    { 
        Str8Lit("yaw"),
        Str8Lit(""),
        Str8Lit("Yaw"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("pitch"),
        Str8Lit(""),
        Str8Lit("Pitch"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("zoom"),
        Str8Lit(""),
        Str8Lit("Zoom"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("font_size"),
        Str8Lit(""),
        Str8Lit("Font Size"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("row_height"),
        Str8Lit(""),
        Str8Lit("Row Height"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("tab_height"),
        Str8Lit(""),
        Str8Lit("Tab Height"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("rgba"),
        Str8Lit(""),
        Str8Lit("RGBA"),
        Str8Lit(""),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("exit"),
        Str8Lit(""),
        Str8Lit("Exit"),
        Str8Lit(""),
        APP_ICON_KIND_X 
    },
    { 
        Str8Lit("open_palette"),
        Str8Lit(""),
        Str8Lit("Open Palette"),
        Str8Lit(""),
        APP_ICON_KIND_LIST 
    },
    { 
        Str8Lit("run_command"),
        Str8Lit(""),
        Str8Lit("Run Command"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("os_event"),
        Str8Lit(""),
        Str8Lit("OS Event"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("select_entity"),
        Str8Lit(""),
        Str8Lit("Select"),
        Str8Lit(""),
        APP_ICON_KIND_RADIO_HOLLOW 
    },
    { 
        Str8Lit("deselect_entity"),
        Str8Lit(""),
        Str8Lit("Deselect"),
        Str8Lit(""),
        APP_ICON_KIND_RADIO_FILLED 
    },
    { 
        Str8Lit("inc_window_font_size"),
        Str8Lit(""),
        Str8Lit("Increase Window Font Size"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("dec_window_font_size"),
        Str8Lit(""),
        Str8Lit("Decrease Window Font Size"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("inc_view_font_size"),
        Str8Lit(""),
        Str8Lit("Increase View Font Size"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("dec_view_font_size"),
        Str8Lit(""),
        Str8Lit("Decrease View Font Size"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("open_window"),
        Str8Lit(""),
        Str8Lit("Open New Window"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("window_settings"),
        Str8Lit(""),
        Str8Lit("Window Settings"),
        Str8Lit(""),
        APP_ICON_KIND_GEAR 
    },
    { 
        Str8Lit("close_window"),
        Str8Lit(""),
        Str8Lit("Close Window"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("toggle_fullscreen"),
        Str8Lit(""),
        Str8Lit("Toggle Fullscreen"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("bring_to_front"),
        Str8Lit(""),
        Str8Lit("Bring To Front"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("popup_accept"),
        Str8Lit(""),
        Str8Lit("Popup Accept"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("popup_cancel"),
        Str8Lit(""),
        Str8Lit("Popup Cancel"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("reset_to_default_bindings"),
        Str8Lit(""),
        Str8Lit("Reset To Default Bindings"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("reset_to_default_panels"),
        Str8Lit(""),
        Str8Lit("Reset To Default Panel Layout"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("reset_to_compact_panels"),
        Str8Lit(""),
        Str8Lit("Reset To Compact Panel Layout"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("reset_to_simple_panels"),
        Str8Lit(""),
        Str8Lit("Reset To Simple Panel Layout"),
        Str8Lit(""),
        APP_ICON_KIND_WINDOW 
    },
    { 
        Str8Lit("new_panel_left"),
        Str8Lit(""),
        Str8Lit("Split Panel Left"),
        Str8Lit(""),
        APP_ICON_KIND_XSPLIT 
    },
    { 
        Str8Lit("new_panel_up"),
        Str8Lit(""),
        Str8Lit("Split Panel Up"),
        Str8Lit(""),
        APP_ICON_KIND_YSPLIT 
    },
    { 
        Str8Lit("new_panel_right"),
        Str8Lit(""),
        Str8Lit("Split Panel Right"),
        Str8Lit(""),
        APP_ICON_KIND_XSPLIT 
    },
    { 
        Str8Lit("new_panel_down"),
        Str8Lit(""),
        Str8Lit("Split Panel Down"),
        Str8Lit(""),
        APP_ICON_KIND_YSPLIT 
    },
    { 
        Str8Lit("split_panel"),
        Str8Lit(""),
        Str8Lit("Split Panel"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("rotate_panel_columns"),
        Str8Lit(""),
        Str8Lit("Rotate Panel Columns"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("next_panel"),
        Str8Lit(""),
        Str8Lit("Focus Next Panel"),
        Str8Lit(""),
        APP_ICON_KIND_RIGHT_ARROW 
    },
    { 
        Str8Lit("prev_panel"),
        Str8Lit(""),
        Str8Lit("Focus Previous Panel"),
        Str8Lit(""),
        APP_ICON_KIND_LEFT_ARROW 
    },
    { 
        Str8Lit("focus_panel"),
        Str8Lit(""),
        Str8Lit("Focus Panel"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("focus_panel_right"),
        Str8Lit(""),
        Str8Lit("Focus Panel Right"),
        Str8Lit(""),
        APP_ICON_KIND_RIGHT_ARROW 
    },
    { 
        Str8Lit("focus_panel_left"),
        Str8Lit(""),
        Str8Lit("Focus Panel Left"),
        Str8Lit(""),
        APP_ICON_KIND_LEFT_ARROW 
    },
    { 
        Str8Lit("focus_panel_up"),
        Str8Lit(""),
        Str8Lit("Focus Panel Up"),
        Str8Lit(""),
        APP_ICON_KIND_UP_ARROW 
    },
    { 
        Str8Lit("focus_panel_down"),
        Str8Lit(""),
        Str8Lit("Focus Panel Down"),
        Str8Lit(""),
        APP_ICON_KIND_DOWN_ARROW 
    },
    { 
        Str8Lit("undo"),
        Str8Lit(""),
        Str8Lit("Undo"),
        Str8Lit(""),
        APP_ICON_KIND_UNDO 
    },
    { 
        Str8Lit("redo"),
        Str8Lit(""),
        Str8Lit("Redo"),
        Str8Lit(""),
        APP_ICON_KIND_REDO 
    },
    { 
        Str8Lit("go_back"),
        Str8Lit(""),
        Str8Lit("Go Back"),
        Str8Lit(""),
        APP_ICON_KIND_LEFT_ARROW 
    },
    { 
        Str8Lit("go_forward"),
        Str8Lit(""),
        Str8Lit("Go Forward"),
        Str8Lit(""),
        APP_ICON_KIND_RIGHT_ARROW 
    },
    { 
        Str8Lit("close_panel"),
        Str8Lit(""),
        Str8Lit("Close Panel"),
        Str8Lit(""),
        APP_ICON_KIND_CLOSE_PANEL 
    },
    { 
        Str8Lit("focus_tab"),
        Str8Lit(""),
        Str8Lit("Focus Tab"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("next_tab"),
        Str8Lit(""),
        Str8Lit("Focus Next Tab"),
        Str8Lit(""),
        APP_ICON_KIND_RIGHT_ARROW 
    },
    { 
        Str8Lit("prev_tab"),
        Str8Lit(""),
        Str8Lit("Focus Previous Tab"),
        Str8Lit(""),
        APP_ICON_KIND_LEFT_ARROW 
    },
    { 
        Str8Lit("move_tab_right"),
        Str8Lit(""),
        Str8Lit("Move Tab Right"),
        Str8Lit(""),
        APP_ICON_KIND_RIGHT_ARROW 
    },
    { 
        Str8Lit("move_tab_left"),
        Str8Lit(""),
        Str8Lit("Move Tab Left"),
        Str8Lit(""),
        APP_ICON_KIND_LEFT_ARROW 
    },
    { 
        Str8Lit("open_tab"),
        Str8Lit(""),
        Str8Lit("Open New Tab"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("build_tab"),
        Str8Lit(""),
        Str8Lit("Build Tab"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("duplicate_tab"),
        Str8Lit(""),
        Str8Lit("Duplicate Tab"),
        Str8Lit(""),
        APP_ICON_KIND_DUPLICATE 
    },
    { 
        Str8Lit("copy_tab_full_path"),
        Str8Lit(""),
        Str8Lit("Copy Full Path"),
        Str8Lit(""),
        APP_ICON_KIND_CLIPBOARD 
    },
    { 
        Str8Lit("close_tab"),
        Str8Lit(""),
        Str8Lit("Close Tab"),
        Str8Lit(""),
        APP_ICON_KIND_X 
    },
    { 
        Str8Lit("move_view"),
        Str8Lit(""),
        Str8Lit("Move View"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("tab_bar_top"),
        Str8Lit(""),
        Str8Lit("Anchor Tab Bar To Top"),
        Str8Lit(""),
        APP_ICON_KIND_UP_ARROW 
    },
    { 
        Str8Lit("tab_bar_bottom"),
        Str8Lit(""),
        Str8Lit("Anchor Tab Bar To Bottom"),
        Str8Lit(""),
        APP_ICON_KIND_DOWN_ARROW 
    },
    { 
        Str8Lit("tab_settings"),
        Str8Lit(""),
        Str8Lit("Tab Settings"),
        Str8Lit(""),
        APP_ICON_KIND_GEAR 
    },
    { 
        Str8Lit("set_current_path"),
        Str8Lit(""),
        Str8Lit("Set Current Path"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("open"),
        Str8Lit(""),
        Str8Lit("Open"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("switch"),
        Str8Lit(""),
        Str8Lit("Switch"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("switch_to_partner_file"),
        Str8Lit(""),
        Str8Lit("Switch To Partner File"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("record_file_in_session"),
        Str8Lit(""),
        Str8Lit("Record File In Session"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("show_file_in_explorer"),
        Str8Lit(""),
        Str8Lit("Show File In Explorer"),
        Str8Lit(""),
        APP_ICON_KIND_FOLDER_CLOSED_FILLED 
    },
    { 
        Str8Lit("go_to_disassembly"),
        Str8Lit(""),
        Str8Lit("Go To Disassembly"),
        Str8Lit(""),
        APP_ICON_KIND_GLASSES 
    },
    { 
        Str8Lit("go_to_source"),
        Str8Lit(""),
        Str8Lit("Go To Source"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("set_file_replacement_path"),
        Str8Lit(""),
        Str8Lit("Set File Replacement Path"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("new_session"),
        Str8Lit(""),
        Str8Lit("New Session"),
        Str8Lit(""),
        APP_ICON_KIND_ADD 
    },
    { 
        Str8Lit("open_session"),
        Str8Lit(""),
        Str8Lit("Open Session"),
        Str8Lit(""),
        APP_ICON_KIND_BRIEFCASE 
    },
    { 
        Str8Lit("open_recent_session"),
        Str8Lit(""),
        Str8Lit("Open Recent Session"),
        Str8Lit(""),
        APP_ICON_KIND_BRIEFCASE 
    },
    { 
        Str8Lit("save_session"),
        Str8Lit(""),
        Str8Lit("Save Session"),
        Str8Lit(""),
        APP_ICON_KIND_SAVE 
    },
    { 
        Str8Lit("record_session_as_last_opened"),
        Str8Lit(""),
        Str8Lit("Record Session As Last Opened"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("record_recent_session"),
        Str8Lit(""),
        Str8Lit("Records Session In Recents"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("write_session_data"),
        Str8Lit(""),
        Str8Lit("Write Session Data"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("session_settings"),
        Str8Lit(""),
        Str8Lit("Session Settings"),
        Str8Lit(""),
        APP_ICON_KIND_GEAR 
    },
    { 
        Str8Lit("edit"),
        Str8Lit(""),
        Str8Lit("Edit"),
        Str8Lit(""),
        APP_ICON_KIND_PENCIL 
    },
    { 
        Str8Lit("accept"),
        Str8Lit(""),
        Str8Lit("Accept"),
        Str8Lit(""),
        APP_ICON_KIND_CHECK_FILLED 
    },
    { 
        Str8Lit("cancel"),
        Str8Lit(""),
        Str8Lit("Cancel"),
        Str8Lit(""),
        APP_ICON_KIND_X 
    },
    { 
        Str8Lit("move_left"),
        Str8Lit(""),
        Str8Lit("Move Left"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_right"),
        Str8Lit(""),
        Str8Lit("Move Right"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up"),
        Str8Lit(""),
        Str8Lit("Move Up"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down"),
        Str8Lit(""),
        Str8Lit("Move Down"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_left_select"),
        Str8Lit(""),
        Str8Lit("Move Left Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_right_select"),
        Str8Lit(""),
        Str8Lit("Move Right Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_select"),
        Str8Lit(""),
        Str8Lit("Move Up Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_select"),
        Str8Lit(""),
        Str8Lit("Move Down Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_left_chunk"),
        Str8Lit(""),
        Str8Lit("Move Left Chunk"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_right_chunk"),
        Str8Lit(""),
        Str8Lit("Move Right Chunk"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_chunk"),
        Str8Lit(""),
        Str8Lit("Move Up Chunk"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_chunk"),
        Str8Lit(""),
        Str8Lit("Move Down Chunk"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_page"),
        Str8Lit(""),
        Str8Lit("Move Up Page"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_page"),
        Str8Lit(""),
        Str8Lit("Move Down Page"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_whole"),
        Str8Lit(""),
        Str8Lit("Move Up Whole"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_whole"),
        Str8Lit(""),
        Str8Lit("Move Down Whole"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_left_chunk_select"),
        Str8Lit(""),
        Str8Lit("Move Left Chunk Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_right_chunk_select"),
        Str8Lit(""),
        Str8Lit("Move Right Chunk Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_chunk_select"),
        Str8Lit(""),
        Str8Lit("Move Up Chunk Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_chunk_select"),
        Str8Lit(""),
        Str8Lit("Move Down Chunk Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_page_select"),
        Str8Lit(""),
        Str8Lit("Move Up Page Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_page_select"),
        Str8Lit(""),
        Str8Lit("Move Down Page Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_whole_select"),
        Str8Lit(""),
        Str8Lit("Move Up Whole Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_whole_select"),
        Str8Lit(""),
        Str8Lit("Move Down Whole Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_up_reorder"),
        Str8Lit(""),
        Str8Lit("Move Up Reorder"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_down_reorder"),
        Str8Lit(""),
        Str8Lit("Move Down Reorder"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_home"),
        Str8Lit(""),
        Str8Lit("Move Home"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_end"),
        Str8Lit(""),
        Str8Lit("Move End"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_home_select"),
        Str8Lit(""),
        Str8Lit("Move Home Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_end_select"),
        Str8Lit(""),
        Str8Lit("Move End Select"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("select_all"),
        Str8Lit(""),
        Str8Lit("Select All"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("delete_single"),
        Str8Lit(""),
        Str8Lit("Delete Single"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("delete_chunk"),
        Str8Lit(""),
        Str8Lit("Delete Chunk"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("backspace_single"),
        Str8Lit(""),
        Str8Lit("Backspace Single"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("backspace_chunk"),
        Str8Lit(""),
        Str8Lit("Backspace Chunk"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("copy"),
        Str8Lit(""),
        Str8Lit("Copy"),
        Str8Lit(""),
        APP_ICON_KIND_CLIPBOARD 
    },
    { 
        Str8Lit("cut"),
        Str8Lit(""),
        Str8Lit("Cut"),
        Str8Lit(""),
        APP_ICON_KIND_CLIPBOARD 
    },
    { 
        Str8Lit("paste"),
        Str8Lit(""),
        Str8Lit("Paste"),
        Str8Lit(""),
        APP_ICON_KIND_CLIPBOARD 
    },
    { 
        Str8Lit("insert_text"),
        Str8Lit(""),
        Str8Lit("Insert Text"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_next"),
        Str8Lit(""),
        Str8Lit("Move Next"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("move_prev"),
        Str8Lit(""),
        Str8Lit("Move Previous"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("goto_line"),
        Str8Lit(""),
        Str8Lit("Go To Line"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("goto_address"),
        Str8Lit(""),
        Str8Lit("Go To Address"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("centre_cursor"),
        Str8Lit(""),
        Str8Lit("Centre Cursor"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("contain_cursor"),
        Str8Lit(""),
        Str8Lit("Contain Cursor"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("find_next"),
        Str8Lit(""),
        Str8Lit("Find Next"),
        Str8Lit(""),
        APP_ICON_KIND_FIND 
    },
    { 
        Str8Lit("find_prev"),
        Str8Lit(""),
        Str8Lit("Find Previous"),
        Str8Lit(""),
        APP_ICON_KIND_FIND 
    },
    { 
        Str8Lit("goto_name"),
        Str8Lit(""),
        Str8Lit("Go To Name"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("goto_name_at_cursor"),
        Str8Lit(""),
        Str8Lit("Go To Name At Cursor"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("enable_cfg"),
        Str8Lit(""),
        Str8Lit("Enable"),
        Str8Lit(""),
        APP_ICON_KIND_CHECK_HOLLOW 
    },
    { 
        Str8Lit("disable_cfg"),
        Str8Lit(""),
        Str8Lit("Disable"),
        Str8Lit(""),
        APP_ICON_KIND_CHECK_FILLED 
    },
    { 
        Str8Lit("select_cfg"),
        Str8Lit(""),
        Str8Lit("Select"),
        Str8Lit(""),
        APP_ICON_KIND_RADIO_HOLLOW 
    },
    { 
        Str8Lit("deselect_cfg"),
        Str8Lit(""),
        Str8Lit("Deselect"),
        Str8Lit(""),
        APP_ICON_KIND_RADIO_FILLED 
    },
    { 
        Str8Lit("remove_cfg"),
        Str8Lit(""),
        Str8Lit("Remove"),
        Str8Lit(""),
        APP_ICON_KIND_TRASH 
    },
    { 
        Str8Lit("name_cfg"),
        Str8Lit(""),
        Str8Lit("Name"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("condition_cfg"),
        Str8Lit(""),
        Str8Lit("Condition"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("duplicate_cfg"),
        Str8Lit(""),
        Str8Lit("Duplicate"),
        Str8Lit(""),
        APP_ICON_KIND_DUPLICATE 
    },
    { 
        Str8Lit("relocate_cfg"),
        Str8Lit(""),
        Str8Lit("Relocate"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("save_cfg_to_session"),
        Str8Lit(""),
        Str8Lit("Save To Session"),
        Str8Lit(""),
        APP_ICON_KIND_BRIEFCASE 
    },
    { 
        Str8Lit("clear_output"),
        Str8Lit(""),
        Str8Lit("Clear Output"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("add_type_view"),
        Str8Lit(""),
        Str8Lit("Add Type View"),
        Str8Lit(""),
        APP_ICON_KIND_BINOCULARS 
    },
    { 
        Str8Lit("add_file_path_map"),
        Str8Lit(""),
        Str8Lit("Add File Path Map"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("edit_session_theme"),
        Str8Lit(""),
        Str8Lit("Edit Session Theme"),
        Str8Lit(""),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("add_theme_colour"),
        Str8Lit(""),
        Str8Lit("Add Theme Colour"),
        Str8Lit(""),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("fork_theme"),
        Str8Lit(""),
        Str8Lit("Fork Theme"),
        Str8Lit(""),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("save_theme"),
        Str8Lit(""),
        Str8Lit("Save Theme"),
        Str8Lit(""),
        APP_ICON_KIND_SAVE 
    },
    { 
        Str8Lit("save_and_set_theme"),
        Str8Lit(""),
        Str8Lit("Save And Set Theme"),
        Str8Lit(""),
        APP_ICON_KIND_SAVE 
    },
    { 
        Str8Lit("search"),
        Str8Lit(""),
        Str8Lit("Search"),
        Str8Lit(""),
        APP_ICON_KIND_FIND 
    },
    { 
        Str8Lit("search_backwards"),
        Str8Lit(""),
        Str8Lit("Search Backwards"),
        Str8Lit(""),
        APP_ICON_KIND_FIND 
    },
    { 
        Str8Lit("pick_file"),
        Str8Lit(""),
        Str8Lit("Pick File"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("pick_folder"),
        Str8Lit(""),
        Str8Lit("Pick Folder"),
        Str8Lit(""),
        APP_ICON_KIND_FOLDER_OPEN_FILLED 
    },
    { 
        Str8Lit("pick_file_or_folder"),
        Str8Lit(""),
        Str8Lit("Pick File/Folder"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("push_query"),
        Str8Lit(""),
        Str8Lit("Push Query"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("complete_query"),
        Str8Lit(""),
        Str8Lit("Complete Query"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("cancel_query"),
        Str8Lit(""),
        Str8Lit("Cancel Query"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("update_query"),
        Str8Lit(""),
        Str8Lit("Update Query"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("open_event_buffer"),
        Str8Lit(""),
        Str8Lit("Open Event Buffer"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("close_event_buffer"),
        Str8Lit(""),
        Str8Lit("Close Event Buffer"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    { 
        Str8Lit("toggle_dev_menu"),
        Str8Lit(""),
        Str8Lit("Toggle Developer Menu"),
        Str8Lit(""),
        APP_ICON_KIND_NULL 
    },
    {
        Str8Lit("watches"), 
        Str8Lit(""), 
        Str8Lit("Watch"), 
        Str8Lit(""), 
        APP_ICON_KIND_BINOCULARS
    },
    { 
        Str8Lit("types"),
        Str8Lit(""),
        Str8Lit("Types"),
        Str8Lit(""),
        APP_ICON_KIND_BINOCULARS 
    },
    { 
        Str8Lit("file_path_maps"),
        Str8Lit(""),
        Str8Lit("File Path Map"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("output"),
        Str8Lit(""),
        Str8Lit("Output"),
        Str8Lit(""),
        APP_ICON_KIND_LIST 
    },
    { 
        Str8Lit("text"),
        Str8Lit(""),
        Str8Lit("Text"),
        Str8Lit(""),
        APP_ICON_KIND_FILE_OUTLINE 
    },
    { 
        Str8Lit("bitmap"),
        Str8Lit(""),
        Str8Lit("Bitmap"),
        Str8Lit(""),
        APP_ICON_KIND_BITMAP 
    },
    { 
        Str8Lit("colour"),
        Str8Lit(""),
        Str8Lit("Colour"),
        Str8Lit(""),
        APP_ICON_KIND_PALETTE 
    },
    { 
        Str8Lit("geo3d"),
        Str8Lit(""),
        Str8Lit("Geometry (3D"),
        Str8Lit(""),
        APP_ICON_KIND_CUBE 
    },
    {
        Str8Lit("x"),
        Str8Lit("x values"),
        Str8Lit("X"),
        Str8Lit("X Values")
    },
    {
        Str8Lit("y"),
        Str8Lit("y values"),
        Str8Lit("Y"),
        Str8Lit("Y Values")
    },
    {
        Str8Lit("ruler_size"),
        Str8Lit(""),
        Str8Lit("Ruler Size"),
        Str8Lit("")
    },
    {
        Str8Lit("status_size"),
        Str8Lit(""),
        Str8Lit("Status Size"),
        Str8Lit("")
    },
    {
        Str8Lit("orientation_widget_size"),
        Str8Lit(""),
        Str8Lit("Orientation Widget Size"),
        Str8Lit("")
    }
};
EXTERN_C_LINK_END

struct QueryTypeMapping {
    Str8                   Name;
    AppQueryExpressionKind Kind;
};

// TODO(nc): add components, connections, annotations, diagrams, and pids
global QueryTypeMapping Mappings[] = {
    {
        Str8Lit("commands"),
        APP_QUERY_EXPR_KIND_COMMANDS
    },
    {
        Str8Lit("tab_commands"),
        APP_QUERY_EXPR_KIND_TAB_COMMANDS
    },
    {
        Str8Lit("recent_files"),
        APP_QUERY_EXPR_KIND_RECENT_FILES
    },
    {
        Str8Lit("source_files"),
        APP_QUERY_EXPR_KIND_SOURCE_FILES
    },
    {
        Str8Lit("recent_sessions"),
        APP_QUERY_EXPR_KIND_RECENT_SESSIONS
    },
    {
        Str8Lit("themes"),
        APP_QUERY_EXPR_KIND_THEMES
    },
    {
        Str8Lit("types"),
        APP_QUERY_EXPR_KIND_TYPES
    },
    {
        Str8Lit("file_path_maps"),
        APP_QUERY_EXPR_KIND_FILE_PATH_MAPS
    },
    {
        Str8Lit("output"),
        APP_QUERY_EXPR_KIND_OUTPUT
    },
    {
        Str8Lit("session_settings"),
        APP_QUERY_EXPR_KIND_SESSION_SETTINGS
    },
};

APP_VIEW_UI_FUNC_DEF(Null)
{}

EV_EXPAND_RULE_INFO_FUNC_DEF(Text)
{
    EVExpandInfo Result = {};

    Result.RowCount = 8;
    Result.SingleItem = TRUE;

    return Result;
}

APP_VIEW_UI_FUNC_DEF(Text)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Access* Acc = AccessOpen();
    f32 MainFontSize = UITailFontSize();
    r2f32 TextAreaRect = Rect;

    PROFILE(ProcessTextFileCommandsTimer) {
        for (AppCommand* Cmd = NULL; AppNextViewCommand(&Cmd); ) {
            AppCommandKind Kind = AppCommandKindFromStr(Cmd->Name);

            switch (Kind) {
                default: {} break;

                case APP_COMMAND_KIND_PICK_FILE: {
                    Str8 Src = Registers()->FilePath;
                    Str8 Dst = Cmd->Regs->FilePath;

                    if (Src.Size && Dst.Size) {
                        AppStoreViewExprStr(
                            AppEvalStrFromFilePath(Scratch.MemPool, Dst)
                        );
                    }
                } break;
            }
        }

        Registers()->FilePath = AppFilePathFromEval(
            AppFrameMemPool(), 
            Eval
        );
        Registers()->Cursor.Line = AppViewSettingValueFromName("cursor_line"_s8).I64;
        Registers()->Cursor.Column = AppViewSettingValueFromName("cursor_column"_s8).I64;
        Registers()->Mark.Line = AppViewSettingValueFromName("mark_line"_s8).I64;
        Registers()->Mark.Column = AppViewSettingValueFromName("mark_column"_s8).I64;

        if (!Registers()->Cursor.Line)
            Registers()->Cursor.Line = 1;

        if (!Registers()->Cursor.Column)
            Registers()->Cursor.Column = 1;

        if (!Registers()->Mark.Line)
            Registers()->Mark.Line = 1;

        if (!Registers()->Mark.Column)
            Registers()->Mark.Column = 1;

        Str8List Overrides = AppPossibleOverridesFromFilePath(
            Scratch.MemPool,
            Registers()->FilePath
        );
        r1u64 Range = AppSpaceRangeFromEval(Eval);

        Registers()->TextKey = AppKeyFromEvalSpaceRange(
            Eval.Space, 
            Range, 
            TRUE
        );

        u128 TextHash = BigHash(Str8Struct(&Registers()->TextKey));
        Str8 Data = CDataFromHash(Acc, TextHash);
        b32 FileIsMissing = (
            Registers()->FilePath.Size &&
            !SystemGetFileProperties(Registers()->FilePath).Modified
        );
        b32 TextHashHasData = TextHash != u128{};

        if (FileIsMissing) {
            UIWidthFill() {
                UIHeightFill() {
                    UIColumn() {
                        UIPadding(UI_PERCENT(1, 0)) {
                            TempArena Scratch = GetScratch(NULL, 0);

                            UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                UIPreferredHeight(UI_EM(3, 1)) {
                                    UIRow() {
                                        UIPadding(UI_PERCENT(1, 0)) {
                                            UIPreferredWidth(UI_TEXT_DIM(1, 1)) {
                                                UITag("weak"_s8) {
                                                    AppFont(APP_FONT_SLOT_ICONS) {
                                                        UILabel(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_WARNING_BIG]);
                                                        UILabel(
                                                            "Could not find \"%S\".",
                                                            PRINT_STR(Registers()->FilePath)
                                                        );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            UIPreferredHeight(UI_EM(3, 1)) {
                                UIRow() {
                                    UIPadding(UI_PERCENT(1, 0)) {
                                        UIPreferredWidth(UI_TEXT_DIM(10, 1)) {
                                            UICornerRadius(UIHeadFontSize() / 3) {
                                                UIPreferredWidth(UI_TEXT_DIM(10, 1)) {
                                                    UIFocus(UI_FOCUS_KIND_ON) {
                                                        UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                            UITag("pop"_s8) {
                                                                if (UI_CLICKED(UIButton("Find alternative..."_s8))) {
                                                                    AppCmd(
                                                                        APP_COMMAND_KIND_RUN_COMMAND,
                                                                        __Registers.CommandName = APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_PICK_FILE].String
                                                                    );
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            ReleaseScratch(Scratch);
                        }
                    }

                    if (
                        !FileIsMissing && 
                        Eval.Msgs.MaxKind == EVAL_MSG_KIND_NULL
                    ) {
                        AppStoreViewLoadingInfo(
                            1,
                            0,
                            Data.Size
                        );
                    }
                }
            }
        }
    }

    AppStoreViewParameterI64("cursor_line"_s8, Registers()->Cursor.Line);
    AppStoreViewParameterI64("cursor_column"_s8, Registers()->Cursor.Column);
    AppStoreViewParameterI64("mark_line"_s8, Registers()->Mark.Line);
    AppStoreViewParameterI64("mark_column"_s8, Registers()->Mark.Column);
    AccessClose(Acc);
    ReleaseScratch(Scratch);
}

EVAL_TYPE_ACCESS_FUNC_DEF(Commands)
{
    EvalIRTreeAndType Result = {
        EMPTY_EVAL_IR_NODE_VALUE
    };

    if (Expr->Kind == EVAL_EXPR_KIND_MEMBER_ACCESS) {
        Str8 CommandName = Expr->Head->Next->String;
        AppCommandKindInfo* CommandInfo = AppCommandKindInfoFromStr(CommandName);
        EvalTypeKey CommandType = EvalTypeKeyConstruct(
            __Params.Kind = EVAL_TYPE_KIND_U64,
            __Params.Name = "command"_s8
        );

        CommandType = EvalTypeKeyConstructMetaDescription(
            CommandType,
            CommandInfo->Description
        );
        Result.TypeKey = CommandType;
        Result.Mode = EVAL_MODE_VALUE;
        Result.Root = EvalIRTreeSetSpace(
            MemPool,
            EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_CMD),
            EvalIRTreeConstU(MemPool, EvalIDFromStr(CommandName))
        );
    }

    return Result;
}

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Commands)
{
    EvalTypeExpandInfo Result = {};

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        Str8List CommandNames = {};
        EvalType* __Type = EvalTypeFromKey(Eval.IRTree.TypeKey);

        for (
            AppCommandKind K = (AppCommandKind) 1;
            K < APP_COMMAND_KIND_COUNT;
            K = (AppCommandKind) (K + 1)
        ) {
            AppCommandKindInfo* Info = &APP_COMMAND_KIND_INFO_TABLE[K];

            if (
                Info->Flags & APP_COMMAND_KIND_FLAG_LIST_IN_UI &&
                (
                    !StrMatch(__Type->Name, "text_pt_commands"_s8, 0) ||
                    Info->Flags & APP_COMMAND_KIND_FLAG_LIST_IN_TEXT_POINT
                ) &&
                (
                    !StrMatch(__Type->Name, "text_range_commands"_s8, 0) ||
                    Info->Flags & APP_COMMAND_KIND_FLAG_LIST_IN_TEXT_RANGE
                ) &&
                (
                    !StrMatch(__Type->Name, "tab_commands"_s8, 0) ||
                    Info->Flags & APP_COMMAND_KIND_FLAG_LIST_IN_TAB
                )
            ) {
                Str8 CodeName = Info->String;
                Str8 Description =Info->Description;
                Str8 SearchTags = Info->SearchTags;
                Str8 DisplayName = AppDisplayFromCodeName(CodeName);
                FMRangeList DescMatches = FuzzyFind(
                    Scratch.MemPool,
                    Filter,
                    Description
                );
                FMRangeList NameMatches = FuzzyFind(
                    Scratch.MemPool,
                    Filter,
                    DisplayName
                );
                FMRangeList TagMatches = FuzzyFind(
                    Scratch.MemPool,
                    Filter,
                    SearchTags
                );
                b32 BindingMatchesOkay = FALSE;
                ConfigInputMapNodePtrList Bindings = ConfigInputMapNodePtrListFromName(
                    Scratch.MemPool,
                    APP_STATE->KeyMap,
                    CodeName
                );

                for (
                    ConfigInputMapNodePtr* N = Bindings.Head;
                    N;
                    N = N->Next
                ) {
                    Str8 BindingText = StrFromInputModifierInput(
                        Scratch.MemPool,
                        N->V->Binding.ModKind,
                        N->V->Binding.Input
                    );
                    FMRangeList Matches = FuzzyFind(
                        Scratch.MemPool,
                        Filter,
                        BindingText
                    );

                    if (Matches.Count == Matches.StrPartCount) {
                        BindingMatchesOkay = TRUE;
                        break;
                    }
                }

                if (
                    NameMatches.Count == NameMatches.StrPartCount ||
                    DescMatches.Count == DescMatches.StrPartCount ||
                    TagMatches.Count == TagMatches.StrPartCount ||
                    BindingMatchesOkay
                ) {
                    ListPush(Scratch.MemPool, &CommandNames, CodeName);
                }
            }
        }

        Str8Array* Accel = ArenaPushArrayZero(
            MemPool,
            Str8Array,
            1
        );

        *Accel = StrArrayFromList(MemPool, &CommandNames);
        Result.UserData = Accel;
        Result.ExprCount = Accel->Count;
        ReleaseScratch(Scratch);
    }

    return Result;
}

EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Commands)
{
    OUT u64 I = 0;
    Str8Array* Accel = (Str8Array*) UserData;

    for (
        u64 Index = IndexRange.Min;
        Index < IndexRange.Max;
        ++Index, ++I
    ) {
        Str8 CommandName = Accel->Data[Index];
        EvalEval CommandEval = EvalEvalFromStrFmt(
            "query:commands.%S",
            PRINT_STR(CommandName)
        );

        Evals[I] = CommandEval;
    }
}

EVAL_TYPE_ACCESS_FUNC_DEF(Themes)
{
    EvalIRTreeAndType Result = {
        EMPTY_EVAL_IR_NODE_VALUE
    };

    if (
        Expr->Kind == EVAL_EXPR_KIND_ARRAY_INDEX &&
        Expr->Head->Next->Kind == EVAL_EXPR_KIND_LEAF_STR_LIT
    ) {
        Str8 ThemeName = Expr->Head->Next->String;
        EvalTypeKey ThemeType = EvalTypeKeyConstruct(
            __Params.Kind = EVAL_TYPE_KIND_U64,
            __Params.Name = "theme"_s8
        );

        Result.TypeKey = ThemeType;
        Result.Mode = EVAL_MODE_VALUE;
        Result.Root = EvalIRTreeSetSpace(
            MemPool,
            EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_THEME),
            EvalIRTreeConstU(MemPool, EvalIDFromStr(ThemeName))
        );
    }

    return Result;
}

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Themes)
{
    EvalTypeExpandInfo Result = {};

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        Str8List Names = {};

        for (
            AppThemePreset P = 0;
            P < APP_THEME_PRESET_COUNT;
            ++P
        ) {
            Str8 Name = APP_THEME_PRESET_DISPLAY_STR_TABLE[P];
            FMRangeList NameMatches = FuzzyFind(Scratch.MemPool, Filter, Name);

            if (NameMatches.Count == NameMatches.StrPartCount)
                ListPush(Scratch.MemPool, &Names, Name);
        }

        {
            Str8 ThemeFolder = ArenaPushStrFmt(
                Scratch.MemPool,
                "%S/%s/themes",
                PRINT_STR(GetProcessProperties()->UserProgramDataPath),
                NC_DATA_DIR
            );
            FileIter* Iter = SystemFileIterBegin(
                Scratch.MemPool,
                ThemeFolder,
                SYS_FILE_ITER_SKIP_DIRS
            );

            for (
                FileInfo Info = {};
                SystemFileIterNext(Scratch.MemPool, Iter, &Info);
            ) {
                Str8 Name = Info.Name;
                FMRangeList NameMatches = FuzzyFind(Scratch.MemPool, Filter, Name);

                if (NameMatches.Count == NameMatches.StrPartCount)
                    ListPush(Scratch.MemPool, &Names, ArenaPushStrCpy(MemPool, Name));
            }

            SystemFileIterEnd(Iter);
        }

        Str8Array* Accel = ArenaPushArrayZero(
            MemPool,
            Str8Array,
            1
        );

        *Accel = StrArrayFromList(MemPool, &Names);
        Result.UserData = Accel;
        Result.ExprCount = Accel->Count;
        ReleaseScratch(Scratch);
    }

    return Result;
}

EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Themes)
{
    u64 I = 0;
    Str8Array* Accel = (Str8Array*) UserData;

    for (
        u64 Index = IndexRange.Min;
        Index < IndexRange.Max;
        ++Index, ++I
    ) {
        Str8 Name = Accel->Data[Index];

        Evals[I] = EvalEvalWrapFmt(
            Eval,
            "$[\"%S\"]",
            PRINT_STR(Name)
        );
    }
}

EVAL_TYPE_ACCESS_FUNC_DEF(Views)
{
    EvalIRTreeAndType Result = {
        EMPTY_EVAL_IR_NODE_VALUE
    };

    if (
        Expr->Kind == EVAL_EXPR_KIND_ARRAY_INDEX &&
        Expr->Head->Next->Kind == EVAL_EXPR_KIND_LEAF_STR_LIT
    ) {
        TempArena Scratch = GetScratch(&MemPool, 1);
        Str8 ViewName = Expr->Head->Next->String;
        EvalTypeKey ViewType = EvalTypeKeyConstruct(
            __Params.Kind = EVAL_TYPE_KIND_U64,
            __Params.Name = "view"_s8
        );

        Result.TypeKey = ViewType;
        Result.Mode = EVAL_MODE_NULL;
        Result.Root = EvalIRTreeSetSpace(
            MemPool,
            EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_VIEW),
            EvalIRTreeConstU(MemPool, EvalIDFromStr(ViewName))
        );
        ReleaseScratch(Scratch);
    }

    return Result;
}

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Views)
{
    EvalTypeExpandInfo Result = {};

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        Str8List Names = {};

        for (
            u64 Index = 0;
            Index < ARRAY_COUNT(APP_NAME_SCHEMA_INFO_TABLE);
            ++Index
        ) {
            if (APP_NAME_SCHEMA_INFO_TABLE[Index].IsView) {
                Str8 Name = APP_NAME_SCHEMA_INFO_TABLE[Index].Name;
                FMRangeList NameMatches = FuzzyFind(Scratch.MemPool, Filter, Name);

                if (NameMatches.Count == NameMatches.StrPartCount)
                    ListPush(Scratch.MemPool, &Names, Name);
            }
        }

        Str8Array* Accel = ArenaPushArrayZero(MemPool, Str8Array, 1);

        *Accel = StrArrayFromList(MemPool, &Names);
        Result.UserData = Accel;
        Result.ExprCount = Accel->Count;
        ReleaseScratch(Scratch);
    }

    return Result;
}

EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Views)
{
    u64 I = 0;
    Str8Array* Accel = (Str8Array*) UserData;

    for (
        u64 Index = IndexRange.Min;
        Index < IndexRange.Max;
        ++Index, ++I
    ) {
        Str8 Name = Accel->Data[Index];

        Evals[I] = EvalEvalFromStr(Name);
    }
}

EVAL_TYPE_IR_EXT_FUNC_DEF(Schema)
{
    AppSchemaIRExt* Ext = ArenaPushArrayZero(
        MemPool,
        AppSchemaIRExt,
        1
    );

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        EvalOpList OpList = EvalOpListFromIRTree(Scratch.MemPool, IRTree->Root);
        Str8 ByteCode = EvalByteCodeFromOpList(Scratch.MemPool, &OpList);
        EvalInterp Interp = EvalInterpret(ByteCode);
        EvalTypeKey TypeKey = IRTree->TypeKey;
        EvalType* __Type = EvalTypeFromKey(TypeKey);

        Ext->Config = AppConfigFromEvalSpace(Interp.Space);
        Ext->Schemas = ConfigSchemasFromName(MemPool, APP_STATE->CfgSchemaTable, __Type->Name);
        ReleaseScratch(Scratch);
    }

    EvalIRExt Result = {
        Ext
    };

    return Result;
}

EVAL_TYPE_ACCESS_FUNC_DEF(Schema)
{
    AppSchemaIRExt* Ext = (AppSchemaIRExt*) LHSIRTree->UserData;
    EvalIRTreeAndType IRTree = {
        EMPTY_EVAL_IR_NODE_VALUE
    };

    if (Expr->Kind == EVAL_EXPR_KIND_MEMBER_ACCESS) {
        MDNode* ChildSchema = EMPTY_MD_NODE_VALUE;

        for (
            MDNodePtrNode* N = Ext->Schemas.Head;
            N;
            N = N->Next
        ) {
            for EACH_MDNODE(Child, N->V->Head) {
                if (StrMatch(Child->String, Expr->Head->Next->String, 0)) {
                    ChildSchema = Child;
                    break;
                }
            }
        }

        if (ChildSchema != EMPTY_MD_NODE_VALUE) {
            ConfigNode* Cfg = Ext->Config;
            ConfigNode* Child = ConfigNodeChildFromStr(Cfg, ChildSchema->String);
            EvalTypeKey ChildTypeKey = {};
            b32 WrapChildWithMetaExpr = FALSE;
            b32 IsQueryChild = MDNodeHasTag(ChildSchema, "query"_s8, 0);
            EvalTypeFlag TypeFlags = (!!IsQueryChild * EVAL_TYPE_FLAG_IS_NOT_EDITABLE);
            b32 IsMetaEval = TRUE;
            EvalMode ChildMode = EVAL_MODE_OFFSET;
            Str8 TextValue = Child->Head->String;

            if (!TextValue.Size)
                TextValue = MDTagFromStr(ChildSchema, "default"_s8, 0)->Head->String;

            if (
                StrMatch(ChildSchema->Head->String, "code_string"_s8, 0) ||
                StrMatch(ChildSchema->Head->String, "expr_string"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyConstructArray(
                    EvalTypeKeyBasic(EVAL_TYPE_KIND_U8),
                    TextValue.Size,
                    TypeFlags
                );
            } else if (
                StrMatch(ChildSchema->Head->String, "path"_s8, 0) ||
                StrMatch(ChildSchema->Head->String, "path_pt"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyConstructArray(
                    EvalTypeKeyBasic(EVAL_TYPE_KIND_U8),
                    TextValue.Size,
                    TypeFlags | EVAL_TYPE_FLAG_IS_PATH_TEXT
                );
            } else if (
                StrMatch(ChildSchema->Head->String, "string"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyConstructArray(
                    EvalTypeKeyBasic(EVAL_TYPE_KIND_U8),
                    TextValue.Size,
                    TypeFlags | EVAL_TYPE_FLAG_IS_PLAIN_TEXT
                );
            } else if (
                StrMatch(ChildSchema->Head->String, "u64"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyBasic(EVAL_TYPE_KIND_U64);
                WrapChildWithMetaExpr = TRUE;
            } else if (
                StrMatch(ChildSchema->Head->String, "u32"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyBasic(EVAL_TYPE_KIND_U32);
                WrapChildWithMetaExpr = TRUE;
            } else if (
                StrMatch(ChildSchema->Head->String, "f32"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyBasic(EVAL_TYPE_KIND_F32);
                WrapChildWithMetaExpr = TRUE;
            } else if (
                StrMatch(ChildSchema->Head->String, "bool"_s8, 0)
            ) {
                ChildTypeKey = EvalTypeKeyBasic(EVAL_TYPE_KIND_BOOL);
                WrapChildWithMetaExpr = TRUE;
            } else if (
                StrMatch(ChildSchema->Head->String, "set"_s8, 0)
            ) {
                ChildTypeKey = EvalStr2TypeKeyMapLookup(
                    APP_STATE->MetaName2TypeMap,
                    ChildSchema->String
                );
            }

            if (WrapChildWithMetaExpr) {
                TempArena Scratch = GetScratch(&MemPool, 1);
                EvalExpr* ChildExpr = EvalParseFromStr(Child->Head->String).Expr;
                b32 ExprIsSimple = FALSE;

                if (
                    ChildExpr->Kind == EVAL_EXPR_KIND_LEAF_U64 ||
                    ChildExpr->Kind == EVAL_EXPR_KIND_LEAF_F64 ||
                    ChildExpr->Kind == EVAL_EXPR_KIND_LEAF_F32
                ) {
                    ExprIsSimple = TRUE;
                }

                if (
                    (ChildExpr->Kind == EVAL_EXPR_KIND_POS || ChildExpr->Kind == EVAL_EXPR_KIND_NEG) &&
                    ChildExpr->Head == ChildExpr->Tail &&
                    (
                        ChildExpr->Head->Kind == EVAL_EXPR_KIND_LEAF_U64 ||
                        ChildExpr->Head->Kind == EVAL_EXPR_KIND_LEAF_F64 ||
                        ChildExpr->Head->Kind == EVAL_EXPR_KIND_LEAF_F32
                    )
                ) {
                    ExprIsSimple = TRUE;
                }

                if (
                    ChildExpr->Kind == EVAL_EXPR_KIND_LEAF_IDENT &&
                    (
                        StrMatch(ChildExpr->String, "true"_s8, 0) ||
                        StrMatch(ChildExpr->String, "false"_s8, 0)
                    )
                ) {
                    ExprIsSimple = TRUE;
                }

                if (!ExprIsSimple && ChildExpr != EMPTY_EVAL_EXPR_VALUE) {
                    ChildTypeKey = EvalTypeKeyConstructMetaExpr(
                        ChildTypeKey,
                        Child->Head->String
                    );
                }

                ReleaseScratch(Scratch);
            }

            {
                MDNode* DisplayName = MDTagFromStr(ChildSchema, "display_name"_s8, 0);
                MDNode* Description = MDTagFromStr(ChildSchema, "description"_s8, 0);

                if (!MDNodeIsEmpty(DisplayName)) {
                    ChildTypeKey = EvalTypeKeyConstructMetaDisplayName(
                        ChildTypeKey,
                        DisplayName->Head->String
                    );
                }

                if (!MDNodeIsEmpty(Description)) {
                    ChildTypeKey = EvalTypeKeyConstructMetaDescription(
                        ChildTypeKey,
                        Description->Head->String
                    );
                }
            }

            {
                MDNode* Hex = MDTagFromStr(ChildSchema->Head, "hex"_s8, 0);

                if (!MDNodeIsEmpty(Hex)) {
                    ChildTypeKey = EvalTypeKeyConstruct(
                        __Params.Kind = EVAL_TYPE_KIND_LENS,
                        __Params.Name = "hex"_s8,
                        __Params.DirectKey = ChildTypeKey
                    );
                }
            }

            {
                MDNode* Colour = MDTagFromStr(ChildSchema->Head, "colour"_s8, 0);

                if (!MDNodeIsEmpty(Colour)) {
                    ChildTypeKey = EvalTypeKeyConstruct(
                        __Params.Kind = EVAL_TYPE_KIND_LENS,
                        __Params.Name = "colour"_s8,
                        __Params.DirectKey = ChildTypeKey
                    );
                }
            }

            {
                MDNode* Range = MDTagFromStr(ChildSchema->Head, "range"_s8, 0);

                if (!MDNodeIsEmpty(Range)) {
                    EvalExpr* MinBound = EvalParseFromStr(Range->Head->String).Expr;
                    EvalExpr* MaxBound = EvalParseFromStr(Range->Head->Next->String).Expr;
                    EvalExpr* Args[] = {
                        MinBound,
                        MaxBound
                    };

                    ChildTypeKey = EvalTypeKeyConstruct(
                        __Params.Kind = EVAL_TYPE_KIND_LENS,
                        __Params.Name = "range1"_s8,
                        __Params.DirectKey = ChildTypeKey,
                        __Params.Count = 2,
                        __Params.Args = Args
                    );
                }
            }

            EvalSpace ChildEvalSpace = {};

            if (Cfg != EMPTY_CFG_NODE_VALUE) {
                ChildEvalSpace = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_CFG);
                ChildEvalSpace.U64s[0] = Cfg->ID;
                ChildEvalSpace.U64s[1]  = EvalIDFromStr(ChildSchema->String);
            } 
            // else if (
            //     IsMetaEval && 
            //     EntityInfo != EMPTY_APP_PID_LIVE_ENTITY_INFO_VALUE
            // ) {
            //     ChildEvalSpace = AppEvalSpaceFromPIDKey(EntityInfo.Key);
            //     ChildEvalSpace.U64s[2] = EvalIDFromStr(ChildSchema->String);
            // } else if (Entity != EMPTY_PIDE_ENTITY_VALUE) {
            //     ChildEvalSpace = PIDEEvalSpaceFromEntity(
            //         Entity, 
            //         PIDE_EVAL_SPACE_KIND_ENTITY
            //     );
            // }

            IRTree.Root = EvalIRTreeSetSpace(
                MemPool, 
                ChildEvalSpace, 
                EvalPushIRNode(
                    MemPool, 
                    EVAL_EVAL_OP_CONST_U64
                )
            );
            IRTree.TypeKey = ChildTypeKey;
            IRTree.Mode = ChildMode;
        }
    }

    return IRTree;
}

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Schema)
{
    EvalTypeExpandInfo Result = {};

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        AppSchemaIRExt* Ext = (AppSchemaIRExt*) Eval.IRTree.UserData;
        Str8Array Commands = {};

        {
            Str8List CommandsList = {};

            for (
                MDNodePtrNode* N = Ext->Schemas.Head;
                N;
                N = N->Next
            ) {
                MDNode* Schema = N->V;
                MDNode* Tag = MDTagFromStr(Schema, "expand_commands"_s8, 0);

                for EACH_MDNODE(Arg, Tag->Head) {
                    b32 Filtered = FALSE;

                    if (MDNodeHasTag(Arg, "output"_s8, 0)) {
                        Str8 Expr = AppExpressionFromConfig(Ext->Config);

                        Filtered = !StrMatch(Expr, "query:output"_s8, 0);
                    }

                    if (!Filtered) {
                        AppCommandKindInfo* CommandKindInfo = AppCommandKindInfoFromStr(Arg->String);
                        FMRangeList NameMatches = FuzzyFind(
                            Scratch.MemPool, 
                            Filter, 
                            AppDisplayFromCodeName(CommandKindInfo->String)
                        );
                        FMRangeList DescMatches = FuzzyFind(
                            Scratch.MemPool, 
                            Filter, 
                            CommandKindInfo->Description
                        );
                        FMRangeList TagsMatches = FuzzyFind(
                            Scratch.MemPool, 
                            Filter, 
                            CommandKindInfo->SearchTags
                        );

                        if (
                            NameMatches.Count == NameMatches.StrPartCount ||
                            DescMatches.Count == DescMatches.StrPartCount ||
                            TagsMatches.Count == TagsMatches.StrPartCount
                        ) {
                            ListPush(Scratch.MemPool, &CommandsList, Arg->String);
                        }
                    }
                }
            }

            Commands = StrArrayFromList(MemPool, &CommandsList);
        }

        struct ExpandChildNode {
            ExpandChildNode* Next;
            MDNode*          N;
        };

        ExpandChildNode* HeadChildNode = NULL;
        ExpandChildNode* TailChildNode = NULL;
        u64 ChildCount = 0;

        for (
            MDNodePtrNode* N = Ext->Schemas.Head;
            N;
            N = N->Next
        ) {
            MDNode* Schema = N->V;

            for EACH_MDNODE(Child, Schema->Head) {
                if (!MDNodeHasTag(Child, "no_expand"_s8, 0)) {
                    MDNode* ExpandCheck = MDTagFromStr(Child, "expand_if"_s8, 0);
                    b32 ExpandThisChild = TRUE;

                    if (!MDNodeIsEmpty(ExpandCheck)) {
                        EvalParentKey(Eval.Key) {
                            ExpandThisChild = !!EvalValueFromStr(ExpandCheck->Head->String).U64;
                        }
                    }

                    if (ExpandThisChild) {
                        Str8 DisplayName = MDTagFromStr(Child, "display_name"_s8, 0)->Head->String;

                        if (!DisplayName.Size) {
                            DisplayName = AppDisplayFromCodeName(Child->String);
                        }

                        Str8 Desc = MDTagFromStr(Child, "description"_s8, 0)->Head->String;
                        FMRangeList NameMatches = FuzzyFind(Scratch.MemPool, Filter, Child->String);
                        FMRangeList DisplayNameMatches = FuzzyFind(Scratch.MemPool, Filter, DisplayName);
                        FMRangeList DescMatches = FuzzyFind(Scratch.MemPool, Filter, Desc);

                        if (
                            NameMatches.Count == NameMatches.StrPartCount ||
                            DisplayNameMatches.Count == DisplayNameMatches.StrPartCount ||
                            DescMatches.Count == DescMatches.StrPartCount
                        ) {
                            ExpandChildNode* N = ArenaPushArrayZero(
                                Scratch.MemPool,
                                ExpandChildNode,
                                1
                            );

                            N->N = Child;
                            SLL_QUEUE_PUSH(HeadChildNode, TailChildNode, N);
                            ++ChildCount;
                        }
                    }
                }
            }
        }

        MDNode** Children = ArenaPushArray(MemPool, MDNode*, ChildCount);

        {
            u64 Index = 0;

            for (
                ExpandChildNode* N = HeadChildNode;
                N;
                N = N->Next, ++Index
            ) {
                Children[Index] = N->N;
            }
        }

        AppSchemaExpandAccel* Accel = ArenaPushArrayZero(
            MemPool,
            AppSchemaExpandAccel,
            1
        );

        Accel->Commands = Commands;
        Accel->Children = Children;
        Accel->ChildrenCount = ChildCount;
        Result.UserData = Accel;
        Result.ExprCount = ChildCount + Commands.Count;
        ReleaseScratch(Scratch);
    }

    return Result;
}

EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Schema)
{
    AppSchemaExpandAccel* Accel = (AppSchemaExpandAccel*) UserData;
    r1u64 CommandsIndexRange = Rng(0ULL, Accel->Commands.Count);
    r1u64 ChildIndexRange = Rng(
        CommandsIndexRange.Max,
        CommandsIndexRange.Max + Accel->ChildrenCount
    );
    u64 I = 0;

    {
        r1u64 ReadRange = Intersect(
            IndexRange,
            CommandsIndexRange
        );

        for (
            u64 Index = ReadRange.Min;
            Index < ReadRange.Max;
            ++Index, ++I
        ) {
            Evals[I] = EvalEvalFromStrFmt(
                "query:commands.%S",
                PRINT_STR(Accel->Commands.Data[Index - CommandsIndexRange.Min])
            );
        }
    }

    {
        r1u64 ReadRange = Intersect(
            IndexRange,
            ChildIndexRange
        );

        for (
            u64 Index = ReadRange.Min;
            Index < ReadRange.Max;
            ++Index, ++I
        ) {
            MDNode* ChildSchema = Accel->Children[Index - ChildIndexRange.Min];
            Evals[I] = EvalEvalWrapFmt(
                Eval,
                "$.%S",
                PRINT_STR(ChildSchema->String)
            );
        }
    }
}

EVAL_TYPE_ACCESS_FUNC_DEF(Configs)
{
    EvalIRTreeAndType Result = {
        EMPTY_EVAL_IR_NODE_VALUE
    };
    EvalExpr* RHS = Expr->Head->Next;

    if (
        RHS->Kind == EVAL_EXPR_KIND_LEAF_IDENT &&
        StrMatch(StrPrefix(RHS->String, 1), "$"_s8, 0)
    ) {
        Str8 NumericPart = StrSkip(RHS->String, 1);
        ConfigID ID = U64FromStr(NumericPart, 16);
        ConfigNode* Cfg = ConfigNodeFromID(ID);
        EvalSpace Space = AppEvalSpaceFromConfig(Cfg);

        Result.Root = EvalIRTreeSetSpace(MemPool, Space, EvalIRTreeConstU(MemPool, 0));
        Result.TypeKey = EvalStr2TypeKeyMapLookup(APP_STATE->MetaName2TypeMap, Cfg->String);
        Result.Mode = EVAL_MODE_OFFSET;
    }

    return Result;
}

// EVAL_TYPE_ACCESS_FUNC_DEF(Control)
// {
//     EvalIRTreeAndType Result = {
//         EMPTY_EVAL_IR_NODE_VALUE
//     };
//     EvalExpr* RHS = Expr->Head->Next;

//     if (
//         RHS->Kind == EVAL_EXPR_KIND_LEAF_IDENT &&
//         StrMatch(StrPrefix(RHS->String, 1), "$"_s8, 0)
//     ) {
//         PIDEHandle Handle = PIDEHandleFromStr(RHS->String);
//         PIDEEntity* Entity = PIDEEntityFromHandle(Handle);
//         EvalSpace Space = PIDEEvalSpaceFromEntity(
//             Entity, 
//             APP_EVAL_SPACE_KIND_META_CTRL_ENTITY
//         );

//         Result.Root = EvalIRTreeSetSpace(
//             MemPool,
//             Space,
//             EvalIRTreeConstU(MemPool, 0)
//         );
//         Result.TypeKey = EvalStr2TypeKeyMapLookup(
//             APP_STATE->MetaName2TypeMap,
//             PIDE_ENTITY_KIND_CODE_NAME_TABLE[Entity->Kind]
//         );
//         Result.Mode = EVAL_MODE_OFFSET;
//     }

//     return Result;
// }

EVAL_TYPE_IR_EXT_FUNC_DEF(ConfigsSlice)
{
    AppConfigsIRExt* Ext = ArenaPushArrayZero(
        MemPool,
        AppConfigsIRExt,
        1
    );

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        EvalTypeKey TypeKey = IRTree->TypeKey;
        EvalType* __Type = EvalTypeFromKey(TypeKey);
        Str8 ConfigName = AppSingularFromCodeNamePlural(__Type->Name);
        ConfigNodePtrList ConfigsList = {};

        {
            ConfigNodePtrList ConfigsListAll = ConfigNodeTopLevelListFromStr(
                Scratch.MemPool,
                ConfigName
            );

            for (
                ConfigNodePtrNode* N = ConfigsListAll.Head;
                N;
                N = N->Next
            ) {
                if (AppIsProjectFiltered(N->V))
                    continue;

                ListPush(
                    Scratch.MemPool,
                    &ConfigsList,
                    N->V
                );
            }
        }

        Str8List CommandsList = {};

        {
            MDNodePtrList Schemas = ConfigSchemasFromName(
                Scratch.MemPool,
                APP_STATE->CfgSchemaTable,
                ConfigName
            );

            for (
                MDNodePtrNode* N = Schemas.Head;
                N;
                N = N->Next
            ) {
                MDNode* Schema = N->V;
                MDNode* CollectionCommandsRoot = MDTagFromStr(Schema, "collection_commands"_s8, 0);

                for EACH_MDNODE(Command, CollectionCommandsRoot->Head) {
                    ListPush(
                        MemPool,
                        &CommandsList,
                        Command->String
                    );
                }
            }
        }

        Ext->ConfigName = ConfigName;
        Ext->Configs = ConfigNodePtrArrayFromList(MemPool, &ConfigsList);
        Ext->Commands = StrArrayFromList(MemPool, &CommandsList);
        ReleaseScratch(Scratch);
    }

    EvalIRExt Result = {
        Ext
    };

    return Result;
}

struct AppConfigsExpandAccel {
    Str8Array          Commands;
    ConfigNodePtrArray Configs;
    r1u64              CommandsIndexRange;
    r1u64              ConfigsIndexRange;
};

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(ConfigsQuery)
{
    AppConfigsExpandAccel* Accel = ArenaPushArrayZero(
        MemPool,
        AppConfigsExpandAccel,
        1
    );

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        ConfigNode* RootConfig = AppConfigFromEvalSpace(Eval.Space);
        Str8 ChildKey = EvalStrFromID(Eval.Space.U64s[1]);
        Str8 ChildKeySingular = AppSingularFromCodeNamePlural(ChildKey);

        if (ChildKeySingular.Size)
            ChildKey = ChildKeySingular;

        Str8List Commands = {};
        MDNodePtrList Schemas = ConfigSchemasFromName(
            Scratch.MemPool,
            APP_STATE->CfgSchemaTable,
            ChildKey
        );

        for (
            MDNodePtrNode* N = Schemas.Head;
            N;
            N = N->Next
        ) {
            MDNode* Schema = N->V;
            MDNode* CollectionCommandsRoot = MDTagFromStr(Schema, "collection_commands"_s8, 0);

            for EACH_MDNODE(Command, CollectionCommandsRoot->Head) {
                ListPush(
                    Scratch.MemPool,
                    &Commands,
                    Command->String
                );
            }
        }

        ConfigNodePtrList Children = ConfigNodeChildListFromStr(
            Scratch.MemPool,
            RootConfig,
            ChildKey
        );
        ConfigNodePtrList ChildrenFiltered = Children;

        if (Filter.Size) {
            MemZero(&ChildrenFiltered, sizeof(ChildrenFiltered));

            for (
                ConfigNodePtrNode* N = Children.Head;
                N;
                N = N->Next
            ) {
                FancyStrList ConfigFancyStrings = AppTitleFStrFromConfig(
                    Scratch.MemPool,
                    N->V,
                    TRUE
                );
                Str8 ConfigString = Str8FromFancyStrList(
                    Scratch.MemPool,
                    &ConfigFancyStrings
                );
                FMRangeList Ranges = FuzzyFind(
                    Scratch.MemPool,
                    Filter,
                    ConfigString
                );

                if (Ranges.Count == Ranges.StrPartCount) {
                    ListPush(
                        Scratch.MemPool,
                        &ChildrenFiltered,
                        N->V
                    );
                }
            }
        }

        Accel->Commands = StrArrayFromList(MemPool, &Commands);
        Accel->CommandsIndexRange = Rng(0ULL, Accel->Commands.Count);
        Accel->Configs = ConfigNodePtrArrayFromList(
            MemPool,
            &ChildrenFiltered
        );
        Accel->ConfigsIndexRange = Rng(
            Accel->Commands.Count,
            Accel->Commands.Count + Accel->Configs.Count
        );
        ReleaseScratch(Scratch);
    }

    EvalTypeExpandInfo Result = {
        Accel,
        Accel->Configs.Count + Accel->Commands.Count
    };

    return Result;
}

EVAL_TYPE_ACCESS_FUNC_DEF(ConfigsSlice)
{
    EvalIRTreeAndType Result = {
        EMPTY_EVAL_IR_NODE_VALUE
    };
    ConfigNode* Config = EMPTY_CFG_NODE_VALUE;
    AppConfigsIRExt* Ext = (AppConfigsIRExt*) LHSIRTree->UserData;

    switch (Expr->Kind) {
        default: {} break;

        case EVAL_EXPR_KIND_ARRAY_INDEX: {
            EvalValue RHSValue = EvalValueFromExpr(Expr->Head->Next);
            u64 RHSIndex = RHSValue.U64;

            if (RHSIndex >= 0 && RHSIndex < Ext->Configs.Count)
                Config = Ext->Configs.V[RHSIndex];
        } break;

        case EVAL_EXPR_KIND_MEMBER_ACCESS: {
            Str8 RHSName = Expr->Head->Next->String;
            ConfigID ID = 0;

            if (
                StrMatch(
                    StrPrefix(
                        RHSName, 
                        1
                    ), 
                    "$"_s8, 
                    0
                )
            ) {
                ID = U64FromStr(StrSkip(RHSName, 1), 16);
                Config = ConfigNodeFromID(ID);
            }
        } break;
    }

    if (Config != EMPTY_CFG_NODE_VALUE) {
        Result.Root = EvalIRTreeSetSpace(
            MemPool,
            AppEvalSpaceFromConfig(Config),
            EvalIRTreeConstU(MemPool, 0)
        );
        Result.Mode = EVAL_MODE_OFFSET;
        Result.TypeKey = EvalStr2TypeKeyMapLookup(
            APP_STATE->MetaName2TypeMap,
            Ext->ConfigName
        );
    }

    return Result;
}

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(ConfigsSlice)
{
    AppConfigsExpandAccel* Accel = ArenaPushArrayZero(
        MemPool,
        AppConfigsExpandAccel,
        1
    );
    EvalTypeExpandInfo Result = {
        Accel
    };
    TempArena Scratch = GetScratch(&MemPool, 1);

    {
        AppConfigsIRExt* Ext = (AppConfigsIRExt*) Eval.IRTree.UserData;
        ConfigNodePtrArray ConfigsFiltered = Ext->Configs;

        if (Filter.Size) {
            ConfigNodePtrList ConfigsListFiltered = {};

            for (
                u64 Index = 0;
                Index < Ext->Configs.Count;
                ++Index
            ) {
                ConfigNode* Config = Ext->Configs.V[Index];

                if (AppIsProjectFiltered(Config))
                    continue;

                FancyStrList FancyStrings = AppTitleFStrFromConfig(
                    Scratch.MemPool,
                    Config,
                    TRUE
                );
                Str8 String = Str8FromFancyStrList(
                    Scratch.MemPool, 
                    &FancyStrings
                );
                FMRangeList FuzzyMatches = FuzzyFind(
                    Scratch.MemPool,
                    Filter,
                    String
                );

                if (FuzzyMatches.Count == FuzzyMatches.StrPartCount) {
                    ListPush(
                        Scratch.MemPool,
                        &ConfigsListFiltered,
                        Config
                    );
                }
            }

            ConfigsFiltered = ConfigNodePtrArrayFromList(
                MemPool,
                &ConfigsListFiltered
            );
        }

        if (
            ConfigNodeChildFromStr(
                ConfigNodeFromID(Registers()->View),
                "lister"_s8
            ) == EMPTY_CFG_NODE_VALUE
        ) {
            Accel->Commands = Ext->Commands;
            Accel->CommandsIndexRange = Rng(0ULL, Accel->Commands.Count);
        }

        Accel->Configs = ConfigsFiltered;
        Accel->ConfigsIndexRange = Rng(
            Accel->CommandsIndexRange.Max,
            Accel->CommandsIndexRange.Max + Accel->Configs.Count
        );
        Result.ExprCount = (Accel->Commands.Count + Accel->Configs.Count);
    }

    ReleaseScratch(Scratch);

    return Result;
}

EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(ConfigsSlice)
{
    AppConfigsExpandAccel* Accel = (AppConfigsExpandAccel*) UserData;
    r1u64 CommandsIndexRange = Accel->CommandsIndexRange;
    r1u64 ConfigsIndexRange = Accel->ConfigsIndexRange;
    u64 DstIndex = 0;

    {
        r1u64 ReadRange = Intersect(CommandsIndexRange, IndexRange);
        u64 ReadCount = Length(ReadRange);
        EvalEval CommandsEval = EvalEvalFromStr("query:commands"_s8);

        for (
            u64 Index = 0; 
            Index < ReadCount; 
            ++Index, ++DstIndex
        ) {
            Str8 CommandName = Accel->Commands.Data[Index + ReadRange.Min - CommandsIndexRange.Min];
            EvalEval CommandEval = EvalEvalWrapFmt(
                CommandsEval,
                "$.%S",
                PRINT_STR(CommandName)
            );
            Evals[DstIndex] = CommandEval;
        }
    }

    {
        r1u64 ReadRange = Intersect(ConfigsIndexRange, IndexRange);
        u64 ReadCount = Length(ReadRange);
        EvalEval CommandsEval = EvalEvalFromStr("query:commands"_s8);

        for (
            u64 Index = 0; 
            Index < ReadCount; 
            ++Index, ++DstIndex
        ) {
            ConfigNode* Config = Accel->Configs.V[Index + ReadRange.Min - ConfigsIndexRange.Min];

            Evals[DstIndex] = EvalEvalFromStrFmt(
                "query:config.$%llx",
                Config->ID
            );
        }
    }
}

EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_DEF(ConfigsSlice)
{
    u64 Result = 0;
    AppConfigsExpandAccel* Accel = (AppConfigsExpandAccel*) UserData;

    if (Num) {
        u64 Index = Num - 1;

        if (InRange(Accel->ConfigsIndexRange, Index)) {
            ConfigNode* Config = Accel->Configs.V[Index - Accel->ConfigsIndexRange.Min];

            Result = Config->ID;
        } else if (InRange(Accel->CommandsIndexRange, Index)) {
            Result = Num;
            Result |= (1ULL << 63);
        }
    }

    return Result;
}

EVAL_TYPE_EXPAND_NUM_FROM_ID_FUNC_DEF(ConfigsSlice)
{
    u64 Num = 0;
    AppConfigsExpandAccel* Accel = (AppConfigsExpandAccel*) UserData;

    if (ID) {
        if (ID & (1ULL << 63)) {
            Num = ID;
            Num &= ~(1ULL << 63);
        } else {
            for (u64 Index = 0; Index < Accel->Configs.Count; ++Index) {
                if (Accel->Configs.V[Index]->ID == ID) {
                    Num = Index + Accel->ConfigsIndexRange.Min + 1;
                    break;
                }
            }
        }
    }

    return Num;
}

EV_EXPAND_RULE_INFO_FUNC_DEF(Graph)
{
    EVExpandInfo Result = {};

    Result.RowCount = 8;
    Result.SingleItem = TRUE;

    return Result;
}

OCObject
AppBitmapObjectCreate(Str8 Key, b32* Cancel, OUT b32* Retry, OUT u64* Generation)
{
    Access* Acc = AccessOpen();
    u128 BitmapHash = {};
    AppBitmapTopology Top = {};

    {
        u64 KeyReadOffset = 0;

        KeyReadOffset += SerialPopStruct(Key, KeyReadOffset, &BitmapHash);
        KeyReadOffset += SerialPopStruct(Key, KeyReadOffset, &Top);
    }

    Str8 Data = CDataFromHash(Acc, BitmapHash);
    RendHandle Texture = {};

    if (
        Top.Dimensions.X > 0 && 
        Top.Dimensions.Y > 0 && 
        Data.Size >= (
            (u64) Top.Dimensions.X * 
            (u64) Top.Dimensions.Y *
            TEX2D_FMT_BYTES_PER_PIXEL[Top.Fmt]
        )
    ) {
        Texture = Tex2DAlloc(
            RESOURCE_KIND_STATIC,
            Vec((i32) Top.Dimensions.X, (i32) Top.Dimensions.Y),
            Top.Fmt,
            Data.Str
        );
    }

    OCObject Result = {};

    STATIC_ASSERT(sizeof(Result) >= sizeof(Texture), TexObjectSizeCheck);

    MemCpy(
        &Result,
        &Texture,
        MIN(sizeof(Texture), sizeof(Result))
    );
    AccessClose(Acc);

    return Result;
}

void
AppBitmapObjectDestroy(OCObject Object)
{
    RendHandle Texture = {};

    MemCpy(
        &Texture,
        &Object,
        MIN(sizeof(Texture), sizeof(Object))
    );
    Tex2DRelease(Texture);
}

v2f32
AppBitmapScreenPosFromCanvasPos(
    v2f32 ViewCentrePos, 
    f32 Zoom, 
    r2f32 Rect,
    v2f32 CanvasPos
) {
    v2f32 Result = {
        (Rect.X0 + Rect.X1) / 2 + (CanvasPos.X - ViewCentrePos.X) * Zoom,
        (Rect.Y0 + Rect.Y1) / 2 + (CanvasPos.Y - ViewCentrePos.Y) * Zoom
    };

    return Result;
}

r2f32
AppBitmapScreenRectFromCanvasRect(
    v2f32 ViewCentrePos,
    f32 Zoom,
    r2f32 Rect,
    r2f32 CanvasRect
) {
    r2f32 Result = Rng(
        AppBitmapScreenPosFromCanvasPos(
            ViewCentrePos, 
            Zoom, 
            Rect, 
            CanvasRect.Point0
        ),
        AppBitmapScreenPosFromCanvasPos(
            ViewCentrePos, 
            Zoom, 
            Rect, 
            CanvasRect.Point1
        )
    );

    return Result;
}

v2f32
AppBitmapCanvasPosFromScreenPos(
    v2f32 ViewCentrePos,
    f32 Zoom,
    r2f32 Rect,
    v2f32 ScreenPos
) {
    v2f32 Result = {
        (ScreenPos.X - (Rect.X0 + Rect.X1) / 2) / Zoom + ViewCentrePos.X,
        (ScreenPos.Y - (Rect.Y0 + Rect.Y1) / 2) / Zoom + ViewCentrePos.Y
    };

    return Result;
}

r2f32
AppBitmapCanvasRectFromScreenRect(
    v2f32 ViewCentrePos,
    f32 Zoom,
    r2f32 Rect,
    r2f32 ScreenRect
) {
    r2f32 Result = Rng(
        AppBitmapCanvasPosFromScreenPos(
            ViewCentrePos,
            Zoom,
            Rect,
            ScreenRect.Point0
        ),
        AppBitmapCanvasPosFromScreenPos(
            ViewCentrePos,
            Zoom,
            Rect,
            ScreenRect.Point1
        )
    );

    return Result;
}

UI_BOX_CUSTOM_DRAW(AppBitmapViewCanvasBoxDraw)
{
    AppBitmapCanvasBoxDrawData* DrawData = (AppBitmapCanvasBoxDrawData*) UserData;
    r2f32 ScreenRect = Box->Rect;
    r2f32 CanvasRect = AppBitmapCanvasRectFromScreenRect(
        DrawData->ViewCentrePosition,
        DrawData->Zoom,
        ScreenRect,
        ScreenRect
    );
    f32 GridCellSizeCanvas = Box->FontSize * 10.0f;
    f32 GridLineThicknessPX = MAX(2.0f, Box->FontSize * 0.1f);
    v4f32 GridLineColour = {};

    UITag("weak"_s8) {
        GridLineColour = UIColourFromName("text"_s8);
    }

    for (Axis2D Axis = 0; Axis < AXIS_2D_COUNT; ++Axis) {
        for (
            f32 Val = (
                CanvasRect.Point0.V[Axis] - 
                FMod(CanvasRect.Point0.V[Axis], GridCellSizeCanvas)
            );
            Val < CanvasRect.Point1.V[Axis];
            Val += GridCellSizeCanvas
        ) {
            v2f32 CanvasP = {};

            CanvasP.V[Axis] = Val;

            v2f32 ScreenP = AppBitmapScreenPosFromCanvasPos(
                DrawData->ViewCentrePosition, 
                DrawData->Zoom, 
                ScreenRect,
                CanvasP
            );
            r2f32 Rect = {};

            Rect.Point0.V[Axis] = ScreenP.V[Axis] - GridLineThicknessPX / 2;
            Rect.Point1.V[Axis] = ScreenP.V[Axis] + GridLineThicknessPX / 2;
            Rect.Point0.V[FLIP_AXIS(Axis)] = Box->Rect.Point0.V[FLIP_AXIS(Axis)];
            Rect.Point1.V[FLIP_AXIS(Axis)] = Box->Rect.Point1.V[FLIP_AXIS(Axis)];
            DrawRect(
                Rect,
                GridLineColour,
                0.0f,
                0.0f,
                1.0f
            );
        }
    }
}

EV_EXPAND_RULE_INFO_FUNC_DEF(Bitmap)
{
    EVExpandInfo Result = {};

    Result.RowCount = 8;
    Result.SingleItem = TRUE;

    return Result;
}

EV_EXPAND_RULE_INFO_FUNC_DEF(Colour)
{
  EVExpandInfo Result = {};

  Result.RowCount = 12;
  Result.SingleItem = TRUE;

  return Result;
}

EV_EXPAND_RULE_INFO_FUNC_DEF(Geo3D)
{
  EVExpandInfo Result = {};

  Result.RowCount = 16;
  Result.SingleItem = TRUE;

  return Result;
}

EV_EXPAND_RULE_INFO_FUNC_DEF(CAD)
{
    EVExpandInfo Result = {};

    Result.RowCount = 16;
    Result.SingleItem = TRUE;

    return Result;
}

APP_VIEW_UI_FUNC_DEF(Bitmap)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Access* Acc = AccessOpen();
    v2i32 Dimensions = Vec(
        (i32) AppViewSettingFromNameU64("w"_s8),
        (i32) AppViewSettingFromNameU64("h"_s8)
    );
    Str8 FmtString = AppViewSettingFromNameStr8("fmt"_s8);
    RendTex2DFmt Fmt = TEX2D_FMT_RGBA8;

    for (
        RendTex2DFmt F = (RendTex2DFmt) 0; 
        F < TEX2D_FMT_COUNT; 
        F = (RendTex2DFmt) (F + 1)
    ) {
        if (StrMatch(FmtString, TEX2D_FMT_DISP_STRINGS[F], STR_MATCH_ALL_CASES)) {
            Fmt = F;
            break;
        }
    }

    r1u64 EvalRange = EvalRangeFromEval(Eval);
    u64 BaseOffset = EvalRange.Min;
    u64 ExpectedSize = Dimensions.X * Dimensions.Y * TEX2D_FMT_BYTES_PER_PIXEL[Fmt];
    r1u64 OffsetRange = Rng(
        BaseOffset,
        BaseOffset + ExpectedSize
    );
    f32 Zoom = AppViewSettingValueFromName("zoom"_s8).F32;
    v2f32 ViewCentrePos = {
        AppViewSettingValueFromName("x"_s8).F32,
        AppViewSettingValueFromName("y"_s8).F32
    };

    if (Zoom == 0.0f) {
        f32 AvailableDimensionsY = Length(Rect).Y;
        f32 ImageDimensionsY = (f32) Dimensions.Y;

        if (ImageDimensionsY != 0.0f)
            Zoom = (AvailableDimensionsY / ImageDimensionsY) * 0.8f;
        else
            Zoom = 1.0f;
    }

    CKey TextureKey = AppKeyFromEvalSpaceRange(
        Eval.Space,
        OffsetRange,
        FALSE
    );
    AppBitmapTopology Top = {
        Vec((i16) Dimensions.X, (i16) Dimensions.Y),
        Fmt
    };
    u128 DataHash = {};
    RendHandle Texture = {};

    for (u64 RewindIndex = 0; RewindIndex < C_KEY_HASH_HISTORY_COUNT; ++RewindIndex) {
        u128 KeyHash = CHashFromKey(TextureKey, RewindIndex);

        PACKED_STRUCT_BEGIN
        struct {
            u128              Hash;
            AppBitmapTopology Top;
        } PACKED KeyData = {
            KeyHash,
            Top
        };
        PACKED_STRUCT_END

        Str8 Key = Str8Struct(&KeyData);
        OCObject Object = OCObjectFromKey(
            Acc,
            Key,
            AppBitmapObjectCreate,
            AppBitmapObjectDestroy,
            0
        );
        RendHandle TextureCandidate = {};

        MemCpy(
            &TextureCandidate,
            &Object,
            MIN(sizeof(TextureCandidate), sizeof(Object))
        );

        if (TextureCandidate != EMPTY_REND_HANDLE_VALUE) {
            DataHash = KeyHash;
            Texture = TextureCandidate;
            break;
        }
    }

    Str8 Data = CDataFromHash(Acc, DataHash);

    if (
        OffsetRange.Max != OffsetRange.Min &&
        Eval.String.Size &&
        Eval.Msgs.MaxKind == EVAL_MSG_KIND_NULL &&
        (
            DataHash == u128{} ||
            Texture == EMPTY_REND_HANDLE_VALUE ||
            !Data.Size
        )
    ) {
        AppStoreViewLoadingInfo(TRUE, 0, 0);
    }

    UIBox* CanvasBox = EMPTY_UI_BOX_VALUE;
    v2f32 CanvasDimensions = Length(Rect);
    r2f32 CanvasRect = Rng(0.0f, 0.0f, CanvasDimensions.X, CanvasDimensions.Y);
    UIRect(CanvasRect) {
        CanvasBox = UIBuildBoxFromStr(
            UI_BOX_KIND_CLIP | UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_SCROLL, 
            "bmp_canvas"_s8
        );
    }

    UISignal CanvasSig = UISignalFromBox(CanvasBox);

    if (UI_DRAGGING(CanvasSig)) {
        if (UI_PRESSED(CanvasSig)) {
            AppCmd(APP_COMMAND_KIND_FOCUS_PANEL);
            UIStoreDragStruct(&ViewCentrePos);
        }

        v2f32 StartViewCentrePos = *UIGetDragStruct(v2f32);
        v2f32 ScreenDragDelta = UIDragDelta();
        v2f32 CanvasDragDelta = ScreenDragDelta * (1.0f / Zoom);
        v2f32 NewViewCentrePos = StartViewCentrePos - CanvasDragDelta;

        ViewCentrePos = NewViewCentrePos;
    }

    if (CanvasSig.Scroll.Y) {
        f32 NewZoom = Zoom - Zoom * CanvasSig.Scroll.Y / 10.0f;

        NewZoom = CLAMP(1.0f / 256.0f, NewZoom, 256.0f);

        v2f32 ScreenMousePre = UIMouse() - Rect.Point0;
        v2f32 CanvasMouse = AppBitmapCanvasPosFromScreenPos(
            ViewCentrePos,
            Zoom,
            CanvasRect,
            ScreenMousePre
        );
        
        Zoom = NewZoom;

        v2f32 ScreenMousePost = AppBitmapCanvasPosFromScreenPos(
            ViewCentrePos,
            Zoom,
            CanvasRect,
            CanvasMouse
        );
        v2f32 ScreenDrift = ScreenMousePost - ScreenMousePre;

        ViewCentrePos += (ScreenDrift * (1.0f / NewZoom));
    }

    if (UI_DOUBLE_CLICKED(CanvasSig)) {
        UIKillAction();
        MemZero(&ViewCentrePos, sizeof(ViewCentrePos));
        Zoom = 1.0f;
    }

    {
        AppBitmapCanvasBoxDrawData* DrawData = ArenaPushArrayZero(
            UIBuildMemPool(),
            AppBitmapCanvasBoxDrawData,
            1
        );

        DrawData->ViewCentrePosition = ViewCentrePos;
        DrawData->Zoom = Zoom;
        UIBoxEquipCustomDraw(
            CanvasBox,
            AppBitmapViewCanvasBoxDraw,
            DrawData
        );
    }

    r2f32 CanvasImageRect = Rng(
        (f32) -Top.Dimensions.X / 2,
        (f32) -Top.Dimensions.Y / 2,
        (f32) Top.Dimensions.X / 2,
        (f32) Top.Dimensions.Y / 2
    );
    r2f32 ScreenImageRect = AppBitmapScreenRectFromCanvasRect(
        ViewCentrePos,
        Zoom,
        CanvasRect,
        CanvasImageRect
    );
    v2i32 BitmapMouse = {
        -1, 
        -1
    };

    if (UI_HOVERING(CanvasSig)) {
        v2f32 ScreenMouse = UIMouse() - Rect.Point0;
        v2f32 CanvasMouse = AppBitmapCanvasPosFromScreenPos(
            ViewCentrePos,
            Zoom,
            CanvasRect,
            ScreenMouse
        );

        if (InRange(CanvasImageRect, CanvasMouse)) {
            BitmapMouse = Vec(
                (i32) (CanvasMouse.X - CanvasImageRect.X0),
                (i32) (CanvasMouse.Y - CanvasImageRect.Y0)
            );
            i64 OffsetPX = BitmapMouse.Y * Top.Dimensions.X + BitmapMouse.X;
            i64 OffsetBytes = OffsetPX * TEX2D_FMT_BYTES_PER_PIXEL[Top.Fmt];

            if (
                OffsetBytes >= 0 && 
                OffsetBytes + TEX2D_FMT_BYTES_PER_PIXEL[Top.Fmt] <= Data.Size &&
                TEX2D_FMT_BYTES_PER_PIXEL[Top.Fmt]
            ) {
                b32 ColourIsOkay = TRUE;
                v4f32 Colour = {};

                switch (Top.Fmt) {
                    default: {
                        ColourIsOkay = FALSE;
                    } break;

                    case TEX2D_FMT_R8: {
                        Colour = Vec(
                            ((u8*) (Data.Str + OffsetBytes))[0] / 255.0f, 
                            0.0f, 
                            0.0f, 
                            1.0f
                        );
                    } break;

                    case TEX2D_FMT_RG8: {
                        Colour = Vec(
                            ((u8*) (Data.Str + OffsetBytes))[0] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[1] / 255.0f, 
                            0.0f, 
                            1.0f
                        );
                    } break;

                    case TEX2D_FMT_RGBA8: {
                        Colour = Vec(
                            ((u8*) (Data.Str + OffsetBytes))[0] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[1] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[2] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[3] / 255.0f
                        );
                    } break;

                    case TEX2D_FMT_BGRA8: {
                        Colour = Vec(
                            ((u8*) (Data.Str + OffsetBytes))[2] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[1] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[0] / 255.0f, 
                            ((u8*) (Data.Str + OffsetBytes))[3] / 255.0f
                        );
                    } break;

                    case TEX2D_FMT_R16: {
                        Colour = Vec(
                            ((u16*) (Data.Str + OffsetBytes))[0] / (f32) U16_MAX, 
                            0.0f,
                            0.0f, 
                            1.0f
                        );
                    } break;

                    case TEX2D_FMT_RGBA16: {
                        Colour = Vec(
                            ((u16*) (Data.Str + OffsetBytes))[0] / (f32) U16_MAX, 
                            ((u16*) (Data.Str + OffsetBytes))[1] / (f32) U16_MAX, 
                            ((u16*) (Data.Str + OffsetBytes))[2] / (f32) U16_MAX, 
                            ((u16*) (Data.Str + OffsetBytes))[3] / (f32) U16_MAX
                        );
                    } break;

                    case TEX2D_FMT_R32: {
                        Colour = Vec(
                            ((f32*) (Data.Str + OffsetBytes))[0], 
                            0.0f,
                            0.0f, 
                            1.0f
                        );
                    } break;

                    case TEX2D_FMT_RG32: {
                        Colour = Vec(
                            ((f32*) (Data.Str + OffsetBytes))[0],
                            ((f32*) (Data.Str + OffsetBytes))[1],
                            0.0f, 
                            1.0f
                        );
                    } break;

                    case TEX2D_FMT_RGBA32: {
                        Colour = Vec(
                            ((f32*) (Data.Str + OffsetBytes))[0],
                            ((f32*) (Data.Str + OffsetBytes))[1],
                            ((f32*) (Data.Str + OffsetBytes))[2],
                            ((f32*) (Data.Str + OffsetBytes))[3]
                        );
                    } break;
                }

                if (ColourIsOkay) {
                    v4f32 HSVA = HSVAFromRGBA(Colour);

                    UIDoColourTooltip(HSVA);
                }
            }
        }
    }

    UIParent(CanvasBox) {
        if (
            BitmapMouse.X >= 0 &&
            BitmapMouse.X < Dimensions.X &&
            BitmapMouse.Y >= 0 &&
            BitmapMouse.Y < Dimensions.Y
        ) {
            f32 ScreenSizePX = 1.0f * Zoom;
            r2f32 ScreenIndicatorRect = Rng(
                ScreenImageRect.X0 + BitmapMouse.X * ScreenSizePX,
                ScreenImageRect.Y0 + BitmapMouse.Y * ScreenSizePX,
                ScreenImageRect.X0 + (BitmapMouse.X + 1) * ScreenSizePX,
                ScreenImageRect.Y0 + (BitmapMouse.Y + 1) * ScreenSizePX
            );

            UIRect(ScreenIndicatorRect) {
                UIBuildBoxFromKey(
                    UI_BOX_KIND_DRAW_BORDER | UI_BOX_KIND_FLOATING,
                    EMPTY_UI_KEY_VALUE
                );
            }
        }

        UIRect(ScreenImageRect) {
            UIFlags(
                UI_BOX_KIND_DRAW_BORDER | 
                UI_BOX_KIND_DRAW_DROP_SHADOW | 
                UI_BOX_KIND_FLOATING
            ) {
                UIImage(
                    Texture,
                    TEX2D_SAMPLE_NEAREST,
                    Rng(0.0f, 0.0f, (f32) Dimensions.X, (f32) Dimensions.Y),
                    Vec(1.0f, 1.0f, 1.0f, 1.0f),
                    0.0f,
                    "bmp_image"_s8
                );
            }
        }
    }

    AppStoreViewParameterF32("zoom"_s8, Zoom);
    AppStoreViewParameterF32("x"_s8, ViewCentrePos.X);
    AppStoreViewParameterF32("y"_s8, ViewCentrePos.Y);
    AccessClose(Acc);
    ReleaseScratch(Scratch);
}

AppEvalColour
AppEvalColourFromEval(EvalEval Eval)
{
    TempArena Scratch = GetScratch(NULL, 0);
    EvalEval ComponentEvals[4] = {};

    {
        struct LeafTask {
            LeafTask* Next;
            EvalEval  Eval;
        };

        u64 RemainingComponents = 4;
        LeafTask StartTask = {
            NULL,
            Eval
        };
        LeafTask* HeadTask = &StartTask;
        LeafTask* TailTask = HeadTask;

        for (
            LeafTask* T = HeadTask;
            T && RemainingComponents;
            T = T->Next
        ) {
            EvalType* __Type = EvalTypeFromKey(
                EvalTypeKeyUnwrap(
                    T->Eval.IRTree.TypeKey,
                    EVAL_TYPE_UNWRAP_FLAG_ALL_DECORATIVE
                )
            );

            switch (__Type->Kind) {
                default: {} break;

                case EVAL_TYPE_KIND_U32:
                case EVAL_TYPE_KIND_I32:
                case EVAL_TYPE_KIND_U64:
                case EVAL_TYPE_KIND_I64: {
                    ComponentEvals[0] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)(($ & 0xFF000000) >> 24) / 255.0f"_s8
                        )
                    );
                    ComponentEvals[1] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)(($ & 0x00FF0000) >> 16) / 255.0f"_s8
                        )
                    );
                    ComponentEvals[2] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)(($ & 0x0000FF00) >> 8) / 255.0f"_s8
                        )
                    );
                    ComponentEvals[3] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)(($ & 0x000000FF) >> 0) / 255.0f"_s8
                        )
                    );
                    RemainingComponents -= 4;
                } break;

                case EVAL_TYPE_KIND_ARRAY: {
                    ComponentEvals[0] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)($[0])"_s8
                        )
                    );
                    ComponentEvals[1] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)($[1])"_s8
                        )
                    );
                    ComponentEvals[2] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)($[2])"_s8
                        )
                    );
                    ComponentEvals[3] = EvalValueEvalFromEval(
                        EvalEvalWrap(
                            T->Eval,
                            "(float32)($[3])"
                        )
                    );
                    RemainingComponents -= 4;
                } break;
            }
        }
    }

    AppEvalColour Result = {};

    {
        EvalType* LensType = EvalTypeFromKey(Eval.IRTree.TypeKey);

        for (
            EvalType* T = LensType;
            T->Kind == EVAL_TYPE_KIND_LENS;
            T = EvalTypeFromKey(T->DirectTypeKey)
        ) {
            if (StrMatch(T->Name, "colour"_s8, 0)) {
                LensType = T;
                break;
            }
        }

        Str8 FormatString = "rgba"_s8;

        if (LensType->Kind == EVAL_TYPE_KIND_LENS && LensType->Count)
            FormatString = LensType->Args[0]->String;

        if (StrMatch(FormatString, "rgba"_s8, 0)) {
            Result.ColourEvals[0] = ComponentEvals[0];
            Result.ColourEvals[1] = ComponentEvals[1];
            Result.ColourEvals[2] = ComponentEvals[2];
            Result.ColourEvals[3] = ComponentEvals[3];
        } else if (StrMatch(FormatString, "argb"_s8, 0)) {
            Result.ColourEvals[0] = ComponentEvals[1];
            Result.ColourEvals[1] = ComponentEvals[2];
            Result.ColourEvals[2] = ComponentEvals[3];
            Result.ColourEvals[3] = ComponentEvals[0];
        } else if (StrMatch(FormatString, "bgra"_s8, 0)) {
            Result.ColourEvals[0] = ComponentEvals[2];
            Result.ColourEvals[1] = ComponentEvals[1];
            Result.ColourEvals[2] = ComponentEvals[0];
            Result.ColourEvals[3] = ComponentEvals[3];
        } else if (StrMatch(FormatString, "abgr"_s8, 0)) {
            Result.ColourEvals[0] = ComponentEvals[3];
            Result.ColourEvals[1] = ComponentEvals[2];
            Result.ColourEvals[2] = ComponentEvals[1];
            Result.ColourEvals[3] = ComponentEvals[0];
        }

        for (
            u32 Index = 0;
            Index < 4;
            ++Index
        ) {
            Result.Colour.V[Index] = EvalValueEvalFromEval(
                Result.ColourEvals[Index]
            ).Value.F32;
        }
    }

    ReleaseScratch(Scratch);

    return Result;
}

APP_VIEW_UI_FUNC_DEF(Colour)
{
    TempArena Scratch = GetScratch(NULL, 0);

    struct AppColourViewState {
        b32   IsInitialised;
        u32   StartColourRGBA;
        v4f32 HSVA;
    };

    AppColourViewState* State = GetAppViewState(AppColourViewState);
    AppEvalColour EvalColour = AppEvalColourFromEval(Eval);
    u32 ColourU32 = U32FromRGBA(EvalColour.Colour);

    if (!State->IsInitialised || ColourU32 != State->StartColourRGBA) {
        v4f32 RGBA = EvalColour.Colour;
        v4f32 HSVA = HSVAFromRGBA(RGBA);

        State->IsInitialised = TRUE;
        State->StartColourRGBA = ColourU32;
        State->HSVA = HSVA;
    }

    v4f32 HSVA = State->HSVA;
    v4f32 RGBA = RGBAFromHSVA(HSVA);
    v2f32 RectLength = Length(Rect);
    f32 SVLengthPX = MIN(RectLength.X, RectLength.Y);
    f32 Padding = SVLengthPX * 0.2f;

    SVLengthPX -= Padding * 2.0f;
    SVLengthPX = MIN(SVLengthPX, UIHeadFontSize() * 70.0f);

    {
        UIWidthFill() {
            UIHeightFill() {
                UIPreferredHeight(UI_SUM_OF_CHILDREN(1)) {
                    UIColumn() {
                        UIPadding(UI_PERCENT(1.0f, 0.0f)) {
                            UIPreferredHeight(UI_SUM_OF_CHILDREN(1)) {
                                UIRow() {
                                    UIPadding(UI_PERCENT(1.0f, 0.0f)) {
                                        UIPreferredWidth(UI_PX(SVLengthPX, 1.0f)) {
                                            UIPreferredHeight(UI_PX(SVLengthPX, 1.0f)) {
                                                AppFont(APP_FONT_SLOT_CODE) {
                                                    UISignal SVSig = UISaturationValPicker(
                                                        HSVA.X,
                                                        &HSVA.Y,
                                                        &HSVA.Z,
                                                        "sat_val_picker"_s8
                                                    );
                                                    UISignal HSig = {};
                                                    UISignal ASig = {};

                                                    UISpacer(UI_EM(1.0f, 1.0f));

                                                    UIPreferredWidth(UI_EM(3.0f, 1.0f)) {
                                                        ASig = UIAlphaPicker(
                                                            &HSVA.W,
                                                            "alpha_picker"_s8
                                                        );
                                                    }

                                                    UISpacer(UI_EM(1.0f, 1.0f));

                                                    UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                                        UIColumn() {
                                                            UIPreferredWidth(UI_EM(6.0f, 0.0f)) {
                                                                UIPreferredHeight(UI_EM(6.0f, 0.0f)) {
                                                                    UIBackgroundColour(
                                                                        LinearFromSRGBA(
                                                                            Vec(
                                                                                RGBA.R, 
                                                                                RGBA.G, 
                                                                                RGBA.B, 
                                                                                1.0f
                                                                            )
                                                                        )
                                                                    ) {
                                                                        UICornerRadius(4.0f) {
                                                                            UIPreferredWidth(UI_EM(6.0f, 1.0f)) {
                                                                                UIPreferredHeight(UI_EM(6.0f, 1.0f)) {
                                                                                    UIBuildBoxFromStr(
                                                                                        UI_BOX_KIND_DRAW_BORDER | UI_BOX_KIND_DRAW_BACKGROUND,
                                                                                        ""_s8
                                                                                    );
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }

                                                            UISpacer(UI_EM(2.0f, 1.0f));

                                                            UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                                                UIPreferredHeight(UI_SUM_OF_CHILDREN(1)) {
                                                                    UIRow() {
                                                                        UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                                                            UIColumn() {
                                                                                UIPreferredWidth(UI_TEXT_DIM(10, 1)) {
                                                                                    UIPreferredHeight(UI_EM(2.0f, 0.0f)) {
                                                                                        AppFont(APP_FONT_SLOT_CODE) {
                                                                                            UITag("weak"_s8) {
                                                                                                UILabel("Hex"_s8);
                                                                                                UILabel("R"_s8);
                                                                                                UILabel("R"_s8);
                                                                                                UILabel("G"_s8);
                                                                                                UILabel("B"_s8);
                                                                                                UILabel("H"_s8);
                                                                                                UILabel("S"_s8);
                                                                                                UILabel("V"_s8);
                                                                                                UILabel("A"_s8);
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }

                                                                        UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                                                            UIColumn() {
                                                                                UIPreferredWidth(UI_TEXT_DIM(10, 1)) {
                                                                                    UIPreferredHeight(UI_EM(2.0f, 0.0f)) {
                                                                                        AppFont(APP_FONT_SLOT_CODE) {
                                                                                            Str8 HexString = ArenaPushStrFmt(
                                                                                                Scratch.MemPool,
                                                                                                "%02x%02x%02x%02x",
                                                                                                (u8) (RGBA.R * 255.0f),
                                                                                                (u8) (RGBA.G * 255.0f),
                                                                                                (u8) (RGBA.B * 255.0f),
                                                                                                (u8) (RGBA.A * 255.0f)
                                                                                            );

                                                                                            UILabel(HexString);
                                                                                            UILabel("%.2f", RGBA.R);
                                                                                            UILabel("%.2f", RGBA.G);
                                                                                            UILabel("%.2f", RGBA.B);
                                                                                            UILabel("%.2f", HSVA.X);
                                                                                            UILabel("%.2f", HSVA.Y);
                                                                                            UILabel("%.2f", HSVA.Z);
                                                                                            UILabel("%.2f", HSVA.W);
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }

                                                    if (
                                                        UI_DRAGGING(HSig) || 
                                                        UI_DRAGGING(SVSig) || 
                                                        UI_DRAGGING(ASig)
                                                    ) {
                                                        EvalType* __Type = EvalTypeFromKey(
                                                            EvalTypeKeyUnwrap(
                                                                Eval.IRTree.TypeKey, 
                                                                EVAL_TYPE_UNWRAP_FLAG_ALL_DECORATIVE
                                                            )
                                                        );

                                                        if (
                                                            __Type->Kind == EVAL_TYPE_KIND_U32 ||
                                                            __Type->Kind == EVAL_TYPE_KIND_I32 ||
                                                            __Type->Kind == EVAL_TYPE_KIND_U64 ||
                                                            __Type->Kind == EVAL_TYPE_KIND_I64
                                                        ) {
                                                            v4f32 NewRGBA = RGBAFromHSVA(HSVA);
                                                            u32 Colour = U32FromRGBA(NewRGBA);
                                                            Str8 String = ArenaPushStrFmt(
                                                                Scratch.MemPool,
                                                                "0x%x",
                                                                Colour
                                                            );

                                                            if (AppCommitEvalValueStr(Eval, String)) {
                                                                State->StartColourRGBA = Colour;
                                                                State->HSVA = HSVA;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ReleaseScratch(Scratch);
}

OCObject
AppGeo3DObjectCreate(
    Str8 Key,
    b32* CancelSignal,
    OUT b32* Retry,
    OUT u64* Generation
) {
    Access* Acc = AccessOpen();
    u128 DataHash = {};

    SerialPopStruct(Key, 0, &DataHash);

    Str8 Data = CDataFromHash(Acc, DataHash);
    RendHandle Buffer = {};

    if (Data.Size)
        Buffer = RendBufferAlloc(RESOURCE_KIND_STATIC, Data.Size, Data.Str);

    OCObject Result = {};

    MemCpy(
        &Result,
        &Buffer,
        MIN(sizeof(Result), sizeof(Buffer))
    );
    AccessClose(Acc);

    return Result;
}

void
AppGeo3DObjectRelease(OCObject Object)
{
    RendHandle Buffer = {};

    MemCpy(
        &Buffer,
        &Object,
        MIN(sizeof(Buffer), sizeof(Object))
    );
    RendBufferRelease(Buffer);
}

RendHandle
AppGeo3DBufferFromKey(Access* Acc, CKey Key)
{
    RendHandle Result = {};

    for (
        u64 RewindIndex = 0;
        RewindIndex < C_KEY_HASH_HISTORY_COUNT;
        ++RewindIndex
    ) {
        u128 DataHash = CHashFromKey(Key, RewindIndex);
        OCObject Object = OCObjectFromKey(
            Acc,
            Str8Struct(&DataHash),
            AppGeo3DObjectCreate,
            AppGeo3DObjectRelease,
            0
        );
        RendHandle Buffer = {};

        MemCpy(
            &Buffer,
            &Object,
            MIN(sizeof(Buffer), sizeof(Object))
        );

        if (Buffer != EMPTY_REND_HANDLE_VALUE) {
            Result = Buffer;
            break;
        }
    }

    return Result;
}

UI_BOX_CUSTOM_DRAW(AppGeo3DBoxDraw)
{
    AppGeo3DBoxDrawData* DrawData = (AppGeo3DBoxDrawData*) UserData;
    r2f32 Clip = Box->Rect;

    for (
        UIBox* B = Box->Parent;
        !IsUIBoxEmpty(B);
        B = B->Parent
    ) {
        if (B->Kind & UI_BOX_KIND_CLIP)
            Clip = Intersect(B->Rect, Clip);
    }

    v3f32 Target = {};
    v3f32 Camera = Vec(
        DrawData->Zoom * CosF32(DrawData->Yaw) * SinF32(DrawData->Pitch),
        DrawData->Zoom * SinF32(DrawData->Yaw) * SinF32(DrawData->Pitch),
        DrawData->Zoom * CosF32(DrawData->Pitch)
    );
    v2f32 BoxLength = Length(Box->Rect);
    RendPassGeo3DParams* Pass = DrawGeo3DBegin(
        Box->Rect,
        LookAt(Camera, Target, Vec(0.0f, 0.0f, 1.0f)),
        Perspective(0.25f, BoxLength.X / BoxLength.Y, 0.1f, 500.0f)
    );

    Pass->Clip = Clip;
    DrawMesh(
        DrawData->VertBuffer,
        DrawData->IndexBuffer,
        GEO_TOP_TRIANGLES,
        (
            GEO_VERT_FLAG_TEX_COORD |
            GEO_VERT_FLAG_NORMALS |
            GEO_VERT_FLAG_RGB
        ),
        EMPTY_REND_HANDLE_VALUE,
        Mat4(1.0f)
    );
}

APP_VIEW_UI_FUNC_DEF(Geo3D)
{
    TempArena Scratch = GetScratch(NULL, 0);
    Access* Acc = AccessOpen();
    AppGeo3DViewState* State = GetAppViewState(AppGeo3DViewState);
    u64 Count = AppViewSettingFromNameU64("count"_s8);
    u64 VertexBaseOffset = AppViewSettingFromNameU64("vtx"_s8);
    u64 VertexSize = AppViewSettingFromNameU64("vtx_size"_s8);
    f32 YawTarget = AppSettingFromNameF32("yaw"_s8);
    f32 PitchTarget = AppSettingFromNameF32("pitch"_s8);
    f32 ZoomTarget = AppSettingFromNameF32("zoom"_s8);
    r1u64 EvalRange = EvalRangeFromEval(Eval);
    u64 BaseOffset = EvalRange.Min;
    r1u64 IndexRange = Rng(
        BaseOffset,
        BaseOffset + Count * sizeof(u32)
    );
    r1u64 VertexRange = Rng(
        VertexBaseOffset,
        VertexBaseOffset + VertexSize
    );
    CKey IndexKey = AppKeyFromEvalSpaceRange(
        Eval.Space,
        IndexRange,
        FALSE
    );
    CKey VertexKey = AppKeyFromEvalSpaceRange(
        Eval.Space,
        VertexRange,
        FALSE
    );
    RendHandle IndexBuffer = AppGeo3DBufferFromKey(Acc, IndexKey);
    RendHandle VertexBuffer = AppGeo3DBufferFromKey(Acc, VertexKey);

    if (
        Eval.String.Size &&
        Eval.Msgs.MaxKind == EVAL_MSG_KIND_NULL &&
        (
            IndexBuffer == EMPTY_REND_HANDLE_VALUE ||
            VertexBuffer == EMPTY_REND_HANDLE_VALUE
        )
    ) {
        AppStoreViewLoadingInfo(
            TRUE,
            0,
            0
        );
    }

    if (ZoomTarget == 0.0f) {
        YawTarget = -0.125f;
        PitchTarget = -0.125f;
        ZoomTarget = 3.5f;
    }

    {
        f32 FastRate = 1.0f - PowF32(2.0f, (-60.0f * APP_STATE->FrameDeltaTime));
        f32 SlowRate = 1.0f - PowF32(2.0f, (-30.0f * APP_STATE->FrameDeltaTime));

        State->Zoom += (ZoomTarget - State->Zoom) * SlowRate;
        State->Yaw += (YawTarget - State->Yaw) * FastRate;
        State->Pitch += (PitchTarget - State->Pitch) * FastRate;

        if (
            AbsF32(State->Zoom - ZoomTarget) > 0.001f ||
            AbsF32(State->Yaw - YawTarget) > 0.001f ||
            AbsF32(State->Pitch - PitchTarget) > 0.001f
        ) {
            AppRequestFrame();
        }
    }

    if (
        Count &&
        IndexBuffer != EMPTY_REND_HANDLE_VALUE &&
        VertexBuffer != EMPTY_REND_HANDLE_VALUE
    ) {
        v2f32 RectLength = Length(Rect);
        UIBox* Box = EMPTY_UI_BOX_VALUE;

        UIFixedSize(RectLength) {
            Box = UIBuildBoxFromStr(
                (
                    UI_BOX_KIND_DRAW_BORDER |
                    UI_BOX_KIND_DRAW_BACKGROUND |
                    UI_BOX_KIND_CLICKABLE |
                    UI_BOX_KIND_SCROLL
                ),
                "geo_box"_s8
            );
        }

        UISignal Sig = UISignalFromBox(Box);

        if (UI_DRAGGING(Sig)) {
            if (UI_PRESSED(Sig)) {
                AppCmd(APP_COMMAND_KIND_FOCUS_PANEL);

                v2f32 Data = Vec(YawTarget, PitchTarget);

                UIStoreDragStruct(&Data);
            }

            v2f32 DragDelta = UIDragDelta();
            v2f32 DragStartData = *UIGetDragStruct(v2f32);

            YawTarget = DragStartData.X + DragDelta.X / RectLength.X;
            PitchTarget = DragStartData.Y + DragDelta.Y / RectLength.Y;
        }

        ZoomTarget += Sig.Scroll.Y;
        ZoomTarget = CLAMP(0.1f, ZoomTarget, 100.0f);
        PitchTarget = CLAMP(-0.49f, PitchTarget, -0.01f);

        AppGeo3DBoxDrawData* DrawData = ArenaPushArrayZero(
            UIBuildMemPool(),
            AppGeo3DBoxDrawData,
            1
        );

        DrawData->Yaw = State->Yaw;
        DrawData->Pitch = State->Pitch;
        DrawData->Zoom = State->Zoom;
        DrawData->VertBuffer = VertexBuffer;
        DrawData->IndexBuffer = IndexBuffer;
        UIBoxEquipCustomDraw(
            Box,
            AppGeo3DBoxDraw,
            DrawData
        );
    }

    AppStoreViewParameterF32("yaw"_s8, YawTarget);
    AppStoreViewParameterF32("pitch"_s8, PitchTarget);
    AppStoreViewParameterF32("zoom"_s8, ZoomTarget);
    AccessClose(Acc);
    ReleaseScratch(Scratch);
}

EVAL_TYPE_IR_EXT_FUNC_DEF(Watches)
{
    AppWatchesAccel* Accel = ArenaPushArrayZero(MemPool, AppWatchesAccel, 1);

    {
        TempArena Scratch = GetScratch(&MemPool, 1);
        EvalOpList OpList = EvalOpListFromIRTree(Scratch.MemPool, IRTree->Root);
        Str8 ByteCode = EvalByteCodeFromOpList(Scratch.MemPool, &OpList);
        EvalInterp Interpret = EvalInterpret(ByteCode);
        EvalSpace Space = Interpret.Space;
        ConfigNode* Target = AppConfigFromEvalSpace(Space);
        ConfigNodePtrList Configs = {};

        for (
            ConfigNode* Child = Target->Head;
            Child != EMPTY_CFG_NODE_VALUE;
            Child = Child->Next
        ) {
            if (AppIsProjectFiltered(Child))
                continue;

            if (StrMatch(Child->String, "watch"_s8, 0)) {
                ListPush(Scratch.MemPool, &Configs, Child);
            }
        }

        Accel->Configs = ConfigNodePtrArrayFromList(MemPool, &Configs);
        ReleaseScratch(Scratch);
    }

    EvalIRExt Result = {
        Accel
    };

    return Result;
}

EVAL_TYPE_ACCESS_FUNC_DEF(Watches)
{
    EvalIRTreeAndType Result = {
        EMPTY_EVAL_IR_NODE_VALUE
    };

    if (Expr->Kind == EVAL_EXPR_KIND_ARRAY_INDEX) {
        AppWatchesAccel* Accel = (AppWatchesAccel*) LHSIRTree->UserData;
        ConfigNodePtrArray* Configs = &Accel->Configs;
        EvalValue RHSValue = EvalValueFromExpr(Expr->Head->Next);

        if (RHSValue.U64 && RHSValue.U64 < Configs->Count) {
            ConfigNode* Config = Configs->V[RHSValue.U64];

            Result.Root = EvalIRTreeSetSpace(
                MemPool, 
                AppEvalSpaceFromConfig(Config), 
                EvalIRTreeConstU(MemPool, 0)
            );
            Result.TypeKey = EvalTypeKeyConstructArray(
                EvalTypeKeyBasic(EVAL_TYPE_KIND_U8),
                Config->Head->String.Size,
                EVAL_TYPE_FLAG_IS_PLAIN_TEXT
            );
            Result.Mode = EVAL_MODE_OFFSET;
        }
    }

    return Result;
}

EVAL_TYPE_EXPAND_INFO_FUNC_DEF(Watches)
{
    AppWatchesAccel* Ext = (AppWatchesAccel*) Eval.IRTree.UserData;
    AppWatchesAccel* Accel = ArenaPushArrayZero(
        MemPool,
        AppWatchesAccel,
        1
    );

    {
        ConfigNodePtrArray ConfigsFiltered = Ext->Configs;

        if (Filter.Size) {
            TempArena Scratch = GetScratch(&MemPool, 1);
            ConfigNodePtrList ConfigsListFiltered = {};

            for (u64 Index = 0; Index < Ext->Configs.Count; ++Index) {
                ConfigNode* Watch = Ext->Configs.V[Index];
                Str8 String = Watch->Head->String;
                FMRangeList Matches = FuzzyFind(Scratch.MemPool, Filter, String);

                if (Matches.Count == Matches.StrPartCount) {
                    ListPush(
                        Scratch.MemPool,
                        &ConfigsListFiltered,
                        Watch
                    );
                }
            }

            ConfigsFiltered = ConfigNodePtrArrayFromList(MemPool, &ConfigsListFiltered);
            ReleaseScratch(Scratch);
        }

        Accel->Configs = ConfigsFiltered;
    }

    EvalTypeExpandInfo Result = {
        Accel,
        Accel->Configs.Count + 1
    };

    return Result;
}

EVAL_TYPE_EXPAND_RANGE_FUNC_DEF(Watches)
{
    AppWatchesAccel* Accel = (AppWatchesAccel*) UserData;
    r1u64 LegalIndexRange = Rng(0ULL, Accel->Configs.Count);
    r1u64 ReadRange = Intersect(IndexRange, LegalIndexRange);
    u64 ReadRangeCount = Length(ReadRange);

    for (u64 Index = 0; Index < ReadRangeCount; ++Index) {
        u64 ConfigIndex = ReadRange.Min + Index;

        if (ConfigIndex < Accel->Configs.Count) {
            ConfigNode* Config = Accel->Configs.V[ConfigIndex];

            Evals[Index] = EvalEvalFromStr(Config->Head->String);
        }
    }
}

EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_DEF(Watches)
{
    u64 ID = 0;
    AppWatchesAccel* Accel = (AppWatchesAccel*) UserData;

    if (Num >= 1 && Num <= Accel->Configs.Count) {
        u64 Index = (Num - 1);

        ID = Accel->Configs.V[Index]->ID;
    } else if (Num == Accel->Configs.Count + 1) {
        ID = U64_MAX;
    }

    return ID;
}

EVAL_TYPE_EXPAND_NUM_FROM_ID_FUNC_DEF(Watches)
{
    u64 Num = 0;
    AppWatchesAccel* Accel = (AppWatchesAccel*) UserData;

    if (ID && ID != U64_MAX) {
        for (u64 Index = 0; Index < Accel->Configs.Count; ++Index) {
            if (Accel->Configs.V[Index]->ID == ID) {
                Num = Index + 1;
                break;
            }
        }
    } else if (ID == U64_MAX) {
        Num = Accel->Configs.Count + 1;
    }

    return Num;
}

AppRegisters* 
AppGetHoverRegisters(void)
{
    return APP_STATE->HoverRegisters;
}

ConfigNode* 
AppViewFromEval(ConfigNode* Parent, EvalEval Eval)
{
    TempArena Scratch = GetScratch(NULL, 0);
    EvalTypeKey TypeKey = Eval.IRTree.TypeKey;
    EvalType* __Type = EvalTypeFromKey(TypeKey);
    Str8 SchemaName = "watch"_s8;
    b32 TypeIsVisualiser = FALSE;

    if (__Type->Kind == EVAL_TYPE_KIND_LENS) {
        AppViewUIRule* ViewUIRule = AppViewUIRuleFomStr(__Type->Name);

        if (ViewUIRule != EMPTY_APP_VIEW_UI_RULE_VALUE) {
            SchemaName = __Type->Name;
            TypeIsVisualiser = TRUE;
        }
    }

    ConfigNode* Result = ConfigNodeChildFromStrOrAlloc(
        APP_STATE->Config,
        Parent,
        SchemaName
    );

    ConfigNodeChildFromStrOrAlloc(APP_STATE->Config, Result, "selected"_s8);

    {
        EvalEval ExprEval = Eval;

        if (
            Eval.Expr->Kind == EVAL_EXPR_KIND_CALL && 
            TypeIsVisualiser
        ) {
            ExprEval = EvalEvalFromExpr(Eval.Expr->Head->Next);
        }

        EvalExpr** Args = NULL;
        u64 ArgsCount = 0;

        if (__Type->Args) {
            Args = __Type->Args;
            ArgsCount = __Type->Count;
        }

        ConfigNode* ExprRoot = ConfigNodeChildFromStrOrAlloc(
            APP_STATE->Config, 
            Result, 
            "expression"_s8
        );

        ConfigNodeNewReplace(
            APP_STATE->Config, 
            ExprRoot, 
            EvalFullExprFromKey(Scratch.MemPool, ExprEval.Key)
        );

        {
            MDNodePtrList Schemas = ConfigSchemasFromName(
                Scratch.MemPool,
                APP_STATE->CfgSchemaTable,
                SchemaName
            );
            u64 UnNamedOrderIndex = 0;

            for (u64 ArgIndex = 0; ArgIndex < ArgsCount; ++ArgIndex) {
                EvalExpr* Arg = Args[ArgIndex];
                Str8 ParamName = {};
                EvalExpr* ArgExpr = Arg;

                if (Schemas.Tail) {
                    for EACH_MDNODE(SchemaChild, Schemas.Tail->V->Head) {
                        MDNode* OrderTag = MDTagFromStr(SchemaChild, "order"_s8, 0);

                        if (OrderTag != EMPTY_MD_NODE_VALUE) {
                            u64 SchemaChildOrderIndex = U64FromStr(
                                OrderTag->Head->String
                            );

                            if (SchemaChildOrderIndex == UnNamedOrderIndex) {
                                ParamName = SchemaChild->String;
                                ArgExpr = Arg;
                                break;
                            }
                        }
                    }

                    ++UnNamedOrderIndex;
                }

                ConfigNode* ArgRoot = ConfigNodeChildFromStrOrAlloc(
                    APP_STATE->Config,
                    Result,
                    ParamName
                );

                ConfigNodeNewReplace(
                    APP_STATE->Config,
                    ArgRoot,
                    EvalStrFromExpr(Scratch.MemPool, ArgExpr, ""_s8)
                );
            }
        }
    }

    ReleaseScratch(Scratch);

    return Result;
}

Str8 
AppCommandNameFromEval(EvalEval Eval)
{
    Str8 Result = {};

    if (Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CMD)
        Result = EvalStrFromID(Eval.Value.U64);

    return Result;
}

u64 
AppEvalSpaceGen(EvalSpace Space)
{
    u64 Result = 0;

    switch (Space.Kind) {
        default: {
            Result = ConfigChangeGeneration();
        } break;
    }

    return Result;
}

b32 
AppEvalSpaceRead(
    EvalSpace Space, 
    OUT void* Data, 
    OUT EvalSpaceRangeInfo* RangeInfo, 
    r1u64 Range
) {
    TempArena Scratch = GetScratch(NULL, 0);
    b32 Result = FALSE;

    switch (Space.Kind) {
        default: {} break;

        case APP_EVAL_SPACE_KIND_META_CFG: {
            ConfigNode* RootCfg = AppConfigFromEvalSpace(Space);
            Str8 ChildKey = EvalStrFromID(Space.U64s[1]);
            ConfigNode* Cfg = RootCfg;

            if (ChildKey.Size)
                Cfg = ConfigNodeChildFromStr(RootCfg, ChildKey);

            Str8 ReadData = {};

            if (ChildKey.Size) {
                MDNode* ChildSchema = EMPTY_MD_NODE_VALUE;
                MDNode* ExprChildSchema = EMPTY_MD_NODE_VALUE;

                {
                    MDNodePtrList Schemas = ConfigSchemasFromName(
                        Scratch.MemPool,
                        APP_STATE->CfgSchemaTable,
                        RootCfg->String
                    );

                    for (
                        MDNodePtrNode* N = Schemas.Head;
                        N && ChildSchema == EMPTY_MD_NODE_VALUE;
                        N = N->Next
                    ) {
                        ChildSchema = MDChildFromStr(N->V, ChildKey, 0);

                        if (ChildSchema != EMPTY_MD_NODE_VALUE) {
                            ExprChildSchema = MDChildFromStr(
                                N->V,
                                "expression"_s8,
                                0
                            );
                        }
                    }
                }

                Str8 ChildTypeName = ChildSchema->Head->String;
                Str8 ValueString = Cfg->Head->String;

                if (!ValueString.Size) {
                    ValueString = MDTagFromStr(
                        ChildSchema,
                        "default"_s8,
                        0
                    )->Head->String;
                }

                if (
                    !ValueString.Size &&
                    !MDNodeIsEmpty(MDTagFromStr(ChildSchema, "override"_s8, 0))
                ) {
                    for (
                        ConfigNode* Parent = RootCfg->Parent; 
                        Parent != EMPTY_CFG_NODE_VALUE; 
                        Parent = Parent->Parent
                    ) {
                        ConfigNode* ParentChildWithKey = ConfigNodeChildFromStr(
                            Parent,
                            ChildKey
                        );

                        if (ParentChildWithKey != EMPTY_CFG_NODE_VALUE) {
                            ValueString = ParentChildWithKey->Head->String;
                            break;
                        }

                        ValueString = AppDefaultSettingFromNames(
                            Parent->String,
                            ChildKey
                        );

                        if (ValueString.Size)
                            break;
                    }
                }

                // if (MDNodeHasTag(ChildSchema, "query"_s8, 0)) {
                //     if (StrMatch(ChildSchema->String, "guid"_s8, 0)) {
                //         Access* Acc = AccessOpen();
                //         Str8 Path = PathFromConfig(RootCfg);
                //         u64 Timestamp = U64FromStr(
                //             ConfigNodeChildFromStr(
                //                 RootCfg, 
                //                 "timestamp"_s8
                //             )->Head->String
                //         );
                //         AccessClose(Acc);
                //     }
                // }

                if (
                    StrMatch(ChildTypeName, "path"_s8, 0) ||
                    StrMatch(ChildTypeName, "path_pt"_s8, 0) ||
                    StrMatch(ChildTypeName, "code_string"_s8, 0) ||
                    StrMatch(ChildTypeName, "expr_string"_s8, 0) ||
                    StrMatch(ChildTypeName, "string"_s8, 0)
                ) {
                    ReadData = ValueString;
                } else {
                    EvalKey ParentKey = {};

                    if (
                        ExprChildSchema != EMPTY_MD_NODE_VALUE && 
                        ChildSchema != ExprChildSchema
                    ) {
                        ParentKey = EvalKeyFromStr(
                            ConfigNodeChildFromStr(
                                RootCfg,
                                ExprChildSchema->String
                            )->Head->String
                        );
                    }

                    EvalParentKey(ParentKey) {
                        if (StrMatch(ChildTypeName, "bool"_s8, 0)) {
                            b32 Value = !!EvalValueFromStrFmt(
                                "(bool)(%S)",
                                PRINT_STR(ValueString)
                            ).U64;

                            ReadData = ArenaPushStrCpy(
                                Scratch.MemPool,
                                Str8Struct(&Value)
                            );
                        } else if (StrMatch(ChildTypeName, "u64"_s8, 0)) {
                            u64 Value = EvalValueFromStrFmt(
                                "(uint64)(%S)",
                                PRINT_STR(ValueString)
                            ).U64;

                            ReadData = ArenaPushStrCpy(
                                Scratch.MemPool,
                                Str8Struct(&Value)
                            );
                        } else if (StrMatch(ChildTypeName, "u32"_s8, 0)) {
                            u64 Value = EvalValueFromStrFmt(
                                "(uint32)(%S)",
                                PRINT_STR(ValueString)
                            ).U64;

                            ReadData = ArenaPushStrCpy(
                                Scratch.MemPool,
                                Str8Struct(&Value)
                            );
                        } else if (StrMatch(ChildTypeName, "f32"_s8, 0)) {
                            f32 Value = EvalValueFromStrFmt(
                                "(float32)(%S)",
                                PRINT_STR(ValueString)
                            ).F32;

                            ReadData = ArenaPushStrCpy(
                                Scratch.MemPool,
                                Str8Struct(&Value)
                            );
                        }
                    }
                }
            }

            if (!ChildKey.Size)
                ReadData = Cfg->Head->String;

            r1u64 LegalRange = Rng(0ULL, ReadData.Size);
            r1u64 ReadRange = Intersect(Range, LegalRange);

            if (ReadRange.Min < ReadRange.Max) {
                Result = TRUE;
                MemCpy(
                    Data, 
                    ReadData.Str + ReadRange.Min,
                    Length(ReadRange)
                );
            }
        } break;
    }

    ReleaseScratch(Scratch);

    return Result;
}

b32 
AppEvalSpaceWrite(EvalSpace Space, IN void* Data, r1u64 Range)
{
    b32 Result = FALSE;

    switch (Space.Kind) {
        default: {} break;

        case APP_EVAL_SPACE_KIND_META_CFG: {
            Result = TRUE;

            Str8 WriteString = StrRange(
                (u8*) Data,
                (u8*) Data + Length(Range)
            );
            ConfigNode* RootCfg = AppConfigFromEvalSpace(Space);
            Str8 ChildKey = EvalStrFromID(Space.U64s[1]);

            if (!ChildKey.Size) {
                ConfigNodeNewReplace(
                    APP_STATE->Config,
                    RootCfg,
                    WriteString
                );
            } else {
                if (StrMatch(ChildKey, "label"_s8, 0)) {
                    Str8 PreEditLabel = AppLabelFromConfig(RootCfg);

                    if (!StrMatch(PreEditLabel, WriteString, 0)) {
                        EvalExpr* Expr = EvalStr2ExprMapLookup(
                            GetTLS()->EVAL_IR_CTX->MacroMap,
                            PreEditLabel
                        );

                        if (Expr != EMPTY_EVAL_EXPR_VALUE) {
                            EvalStr2ExprMapPoisonIncrement(
                                GetTLS()->EVAL_IR_CTX->MacroMap,
                                PreEditLabel
                            );
                            MapInsert(
                                GetTLS()->EVAL_CACHE->MemPool,
                                GetTLS()->EVAL_IR_CTX->MacroMap,
                                WriteString,
                                Expr
                            );
                        }
                    }
                }

                if (Range.Min == Range.Max) {
                    ConfigNodeRelease(
                        APP_STATE->Config,
                        ConfigNodeChildFromStr(RootCfg, ChildKey)
                    );
                } else {
                    ConfigNode* ChildCfg = ConfigNodeChildFromStrOrAlloc(
                        APP_STATE->Config,
                        RootCfg,
                        ChildKey
                    );

                    ConfigNodeNewReplace(
                        APP_STATE->Config,
                        ChildCfg,
                        WriteString
                    );
                }
            }
        } break;
    }

    return Result;
}

CKey 
AppKeyFromEvalSpaceRange(EvalSpace Space, r1u64 Range, b32 IsZeroTerminated)
{
    CKey Result = {};

    switch (Space.Kind) {
        case EVAL_SPACE_KIND_HASH_STORE_KEY: {
            CRoot Root = {
                Space.U64
            };
            CID ID = {
                Space.U128
            };

            Result = CKeyAlloc(Root, ID);
        } break;

        case EVAL_SPACE_KIND_FILE: {
            u64 FilePathStringID = Space.U64;
            Str8 FilePath = EvalStrFromID(FilePathStringID);

            Result = FSKeyFromPathRange(FilePath, Range, 0);
        } break;
    }

    return Result;
}

r1u64 
AppWholeRangeFromEvalSpace(EvalSpace Space)
{
    r1u64 Result = {};

    switch (Space.Kind) {
        case EVAL_SPACE_KIND_HASH_STORE_KEY: {
            CRoot Root = {
                Space.U64
            };
            CID ID = {
                Space.U128
            };
            CKey Key = CKeyAlloc(Root, ID);
            u128 KeyHash = CHashFromKey(Key, 0);
            Access* Acc = AccessOpen();
            Str8 Data = CDataFromHash(Acc, KeyHash);

            Result = Rng(0ULL, Data.Size);
            AccessClose(Acc);
        } break;

        case EVAL_SPACE_KIND_FILE: {
            u64 FilePathStringID = Space.U64;
            Str8 FilePath = EvalStrFromID(FilePathStringID);
            FileProperties Props = SystemGetFileProperties(FilePath);

            Result = Rng(0ULL, Props.Size);
        } break;
    }

    return Result;
}

b32 
AppCommitEvalValueStr(EvalEval Eval, Str8 String)
{
    b32 Result = FALSE;

    if (Eval.IRTree.Mode == EVAL_MODE_OFFSET) {
        TempArena Scratch = GetScratch(NULL, 0);
        EvalTypeKey TypeKey = EvalTypeKeyUnwrap(
            Eval.IRTree.TypeKey,
            EVAL_TYPE_UNWRAP_FLAG_ALL_DECORATIVE
        );
        EvalTypeKind TypeKind = EvalTypeKindFromKey(TypeKey);
        EvalTypeKey DirectTypeKey = EvalTypeKeyUnwrap(
            Eval.IRTree.TypeKey,
            EVAL_TYPE_UNWRAP_FLAG_ALL
        );
        EvalTypeKind DirectTypeKind = EvalTypeKindFromKey(DirectTypeKey);
        b32 GotCommitData = FALSE;
        Str8 CommitData = {};
        b32 CommitAtPtrDst = FALSE;

        if (TypeKey != EMPTY_EVAL_TYPE_KEY_VALUE) {
            if (!GotCommitData && Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG) {
                GotCommitData = TRUE;
                CommitData = String;
            }

            if (
                !GotCommitData &&
                (
                    (
                        TypeKind >= EVAL_TYPE_KIND_FIRST_BASIC &&
                        TypeKind <= EVAL_TYPE_KIND_LAST_BASIC
                    ) ||
                    TypeKind == EVAL_TYPE_KIND_ENUM
                )
            ) {
                GotCommitData = TRUE;

                EvalEval SrcEval = EvalEvalFromStrFmt(
                    "(%S)(%S)",
                    PRINT_STR(EvalTypeStrFromKey(Scratch.MemPool, TypeKey)),
                    PRINT_STR(String)
                );

                CommitData = ArenaPushStrCpy(
                    Scratch.MemPool,
                    Str8Struct(&SrcEval.Value)
                );
                CommitData.Size = MIN(CommitData.Size, EvalByteSizeFromKey(TypeKey));
            }

            b32 IsQuoted = FALSE;

            if (String.Size && String.Str[0] == '"') {
                String = StrSkip(String, 1);
                IsQuoted = TRUE;
            }

            if (
                !GotCommitData &&
                (
                    (
                        (IsQuoted && TypeKind == EVAL_TYPE_KIND_PTR) ||
                        TypeKind == EVAL_TYPE_KIND_ARRAY
                    ) &&
                    (
                        DirectTypeKind == EVAL_TYPE_KIND_CHAR8 ||
                        DirectTypeKind == EVAL_TYPE_KIND_CHAR16 ||
                        DirectTypeKind == EVAL_TYPE_KIND_CHAR32 ||
                        DirectTypeKind == EVAL_TYPE_KIND_UCHAR8 ||
                        DirectTypeKind == EVAL_TYPE_KIND_UCHAR16 ||
                        DirectTypeKind == EVAL_TYPE_KIND_UCHAR32 ||
                        EvalTypeKindIsInteger(DirectTypeKind)
                    )
                )
            ) {
                GotCommitData = TRUE;

                if (String.Size && String.Str[String.Size - 1] == '"') {
                    String = StrChop(String, 1);
                }

                if (IsQuoted) {
                    CommitData = RawFromEscapedStr8(
                        Scratch.MemPool,
                        String
                    );
                } else {
                    CommitData = ArenaPushStrCpy(Scratch.MemPool, String);
                }

                ++CommitData.Size;

                if (TypeKind == EVAL_TYPE_KIND_PTR)
                    CommitAtPtrDst = TRUE;

                switch (DirectTypeKind) {
                    default: {}  break;

                    case EVAL_TYPE_KIND_I16:
                    case EVAL_TYPE_KIND_U16:
                    case EVAL_TYPE_KIND_CHAR16:
                    case EVAL_TYPE_KIND_UCHAR16: {
                        Str16 Data16 = Str16FromStr8(
                            Scratch.MemPool,
                            CommitData
                        );

                        CommitData = Str(
                            (u8*) Data16.Str,
                            Data16.Size * sizeof(u16)
                        );
                    } break;

                    case EVAL_TYPE_KIND_I32:
                    case EVAL_TYPE_KIND_U32:
                    case EVAL_TYPE_KIND_CHAR32:
                    case EVAL_TYPE_KIND_UCHAR32: {
                        Str32 Data32 = Str32FromStr8(
                            Scratch.MemPool,
                            CommitData
                        );

                        CommitData = Str(
                            (u8*) Data32.Str,
                            Data32.Size * sizeof(u32)
                        );
                    } break;
                }
            }

            if (!GotCommitData && TypeKind == EVAL_TYPE_KIND_PTR) {
                EvalEval SrcEval = EvalEvalFromStr(String);
                EvalEval SrcEvalValue = EvalValueEvalFromEval(SrcEval);
                EvalTypeKind SrcEvalValueTypeKind = EvalTypeKindFromKey(
                    SrcEvalValue.IRTree.TypeKey
                );

                if (
                    (
                        EvalTypeKindIsPtrOrRef(SrcEvalValueTypeKind) || 
                        EvalTypeKindIsInteger(SrcEvalValueTypeKind)
                    ) &&
                    SrcEvalValue.IRTree.Mode == EVAL_MODE_VALUE
                ) {
                    GotCommitData = TRUE;
                    CommitData = ArenaPushStrCpy(
                        Scratch.MemPool,
                        Str8Struct(&SrcEval.Value)
                    );
                    CommitData.Size = MAX(
                        CommitData.Size, 
                        EvalByteSizeFromKey(SrcEval.IRTree.TypeKey)
                    );
                    CommitData.Size = MAX(CommitData.Size, EvalByteSizeFromKey(TypeKey));
                }
            }
        }

        u64 DstOffset = Eval.Value.U64;

        if (GotCommitData && CommitAtPtrDst) {
            EvalEval DstValueEval = EvalValueEvalFromEval(Eval);

            DstOffset = DstValueEval.Value.U64;
        }

        if (GotCommitData) {
            Result = EvalSpaceWrite(
                Eval.Space,
                CommitData.Str,
                Rng(DstOffset, DstOffset + CommitData.Size)
            );
        }

        ReleaseScratch(Scratch);
    }

    return Result;
}

Str8 
AppFilePathFromEval(Arena* MemPool, EvalEval Eval)
{
    Str8 Result = {};

    switch (Eval.Space.Kind) {
        default: {} break;

        case EVAL_SPACE_KIND_FILE: {
            Result = ArenaPushStrCpy(
                MemPool,
                EvalStrFromID(Eval.Space.U64)
            );
        } break;

        case EVAL_SPACE_KIND_FILESYSTEM: {
            Result = ArenaPushStrCpy(
                MemPool,
                EvalStrFromID(Eval.Value.U64)
            );
        } break;
    }

    return Result;
}

Str8 
AppFilePathFromEvalStr(Arena* MemPool, Str8 String)
{
    Str8 Result = {};
    TempArena Scratch = GetScratch(&MemPool, 1);
    EvalEval Eval = EvalEvalFromStr(String);

    Result = AppFilePathFromEval(MemPool, Eval);
    ReleaseScratch(Scratch);

    return Result;
}

r1u64 
AppSpaceRangeFromEval(EvalEval Eval)
{
    r1u64 Result = EvalRangeFromEval(Eval);
    u64 SizeSetting = AppViewSettingValueFromName("size"_s8).U64;

    if (SizeSetting)
        Result.Max = Result.Min + SizeSetting;

    return Result;
}

AppViewUIRule* 
AppViewUIRuleFomStr(Str8 String)
{
    AppViewUIRule* Result = EMPTY_APP_VIEW_UI_RULE_VALUE;
    AppViewUIRuleMap* Map = APP_STATE->ViewUIRuleMap;
    u64 StringHash = Hash(String);
    u64 SlotIndex = StringHash % Map->SlotsCount;

    for (
        AppViewUIRuleNode* Node = Map->Slots[SlotIndex].Head;
        Node;
        Node = Node->Next
    ) {
        if (StrMatch(Node->V.Name, String, 0)) {
            Result = &Node->V;
            break;
        }
    }

    return Result;
}

FancyStrList
AppTitleFStrFromCodeName(Arena* MemPool, Str8 CodeName)
{
    FancyStrList Result = {};
    AppVocabInfo* Info = AppVocabInfoFromCodeName(CodeName);
    FancyStrParams Params = {
        AppFontFromSlot(APP_FONT_SLOT_MAIN),
        AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
        UIColourFromName("text"_s8),
        UIHeadFontSize()
    };
    FancyStrParams Overrides = {};

    Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
    Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
    Overrides.Colour = UIColourFromName("text"_s8);

    if (Info->IconKind != APP_ICON_KIND_NULL) {
        UITag("weak"_s8) {
            ListPush(
                MemPool,
                &Result,
                &Params,
                &Overrides,
                APP_ICON_KIND_TEXT_TABLE[Info->IconKind]
            );
            ListPush(
                MemPool,
                &Result,
                &Params,
                NULL,
                "  "_s8
            );
        }

        if (Info->DisplayName.Size) {
            ListPush(MemPool, &Result, &Params, NULL, Info->DisplayName);
        } else {
            Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_CODE);
            Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_CODE);
            Overrides.Colour = Vec(0.0f, 0.0f, 0.0f, 0.0f);
            ListPush(
                MemPool,
                &Result,
                &Params,
                &Overrides,
                CodeName
            );
        }
    }

    return Result;
}

FancyStrList
AppTitleFStrFromFilePath(Arena* MemPool, Str8 FilePath, b32 IncludeFolder)
{
    FancyStrList Result = {};
    Str8 FileName = StrSkipLastSlash(FilePath);
    FileProperties Props = SystemGetFileProperties(FilePath);
    AppIconKind IconKind = APP_ICON_KIND_FILE_OUTLINE;

    if (Props.Kind & SYS_FILE_IS_DIR)
        IconKind = APP_ICON_KIND_FOLDER_CLOSED_FILLED;

    if (
        !FilePath.Size ||
        StrMatch(FilePath, "/"_s8, STR_MATCH_SKIP_SLASH)
    ) {
        IconKind = APP_ICON_KIND_MACHINE;
        FileName = "File System"_s8;
    }

    FancyStrParams Params = {
        AppFontFromSlot(APP_FONT_SLOT_MAIN),
        AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
        UIColourFromName("text"_s8),
        UIHeadFontSize()
    };
    FancyStrParams Overrides = {};

    Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
    Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
    Overrides.Colour = UIColourFromName("text"_s8);

    UITag("weak"_s8) {
        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            APP_ICON_KIND_TEXT_TABLE[IconKind]
        );
    }

    ListPush(
        MemPool,
        &Result,
        &Params,
        NULL,
        "  "_s8
    );
    ListPush(
        MemPool,
        &Result,
        &Params,
        NULL,
        FileName
    );

    if (IncludeFolder) {
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );

        UITag("weak"_s8) {
            Overrides = {};
            Overrides.Size = Params.Size * 0.9f;
            Overrides.Colour = UIColourFromName("text"_s8);
            ListPush(
                MemPool,
                &Result,
                &Params,
                &Overrides,
                StrChopLastSlash(FilePath)
            );
        }
    }

    return Result;
}

b32 
AppIsProjectFiltered(ConfigNode* Config)
{
    ConfigNode* Session = ConfigNodeChildFromStr(Config, "session"_s8);
    b32 Result = (
        Session != EMPTY_CFG_NODE_VALUE &&
        Session->Head->String.Size &&
        !PathMatchNormalised(
            APP_STATE->SessionPath,
            Session->Head->String
        )
    );

    return Result;
}

AppWatchCell*
__WatchCellPushNew(Arena* MemPool, AppWatchCellList* List, AppWatchCell* Params)
{
    AppWatchCell* Result = ArenaPushArrayZero(
        MemPool,
        AppWatchCell,
        1
    );

    Result->Index = List->Count;
    SLL_QUEUE_PUSH(List->Head, List->Tail, Result);
    ++List->Count;

    u64 Index = Result->Index;

    MemCpy(Result, Params, sizeof(*Result));
    Result->Index = Index;

    if (Result->Percent == 0.0f)
        Result->Percent = Result->DefaultPercent;

    List->TotalPercent += Result->Percent;
    Result->Next = NULL;

    return Result;
}


Str8
AppValueStrFromEval(
    Arena* MemPool,
    Str8 Filter,
    EVStrParams* Params,
    FontTag Font,
    f32 FontSize,
    f32 MaxSize,
    EvalEval Eval
) {
    TempArena Scratch = GetScratch(&MemPool, 1);
    Str8List Strings = {};
    EVStrIter* Iter = EVStrIterBegin(Scratch.MemPool, Eval, Params);
    f32 SpaceTakenPX = 0.0f;

    for (
        Str8 String = {};
        EVStrIterNext(Scratch.MemPool, Iter, &String);
    ) {
        if (SpaceTakenPX > MaxSize) {
            ListPush(Scratch.MemPool, &Strings, "..."_s8);
            break;
        } else {
            ListPush(Scratch.MemPool, &Strings, String);
            SpaceTakenPX += FontDimensionsFromTagSizeString(
                Font,
                FontSize,
                0,
                0,
                String
            ).X;
        }
    }

    Str8 Result = StrListJoin(MemPool, &Strings, NULL);

    ReleaseScratch(Scratch);

    return Result;
}

v4f32
AppRGBAFromCodeColourSlot(AppCodeColourSlotKind SlotKind)
{
    AppWindowState* WS = AppWindowStateFromConfig(
        ConfigNodeFromID(Registers()->Window)
    );
    v4f32 Result = WS->ThemeCodeColours[SlotKind];

    return Result;
}

FancyStrList
AppFStrFromRichString(Arena* MemPool, Str8 String)
{
    TempArena Scratch = GetScratch(&MemPool, 1);

    typedef u32 StrPartFlag;
    enum : u32 {
        STR_PART_FLAG_CODE      = (1 << 0),
        STR_PART_FLAG_UNDERLINE = (1 << 1),
        STR_PART_FLAG_BRIGHT    = (1 << 2)
    };

    struct StrPart {
        StrPart*    Next;
        StrPartFlag Flags;
        Str8        String;
    };

    StrPart* HeadPart = NULL;
    StrPart* TailPart = NULL;
    u64 ActivePartStartIndex = 0;
    StrPartFlag ActivePartFlags = 0;

    for (u64 Index = 0; Index <= String.Size; ++Index) {
        if (Index == String.Size) {
            StrPart* Part = ArenaPushArrayZero(Scratch.MemPool, StrPart, 1);

            Part->Flags = ActivePartFlags;
            Part->String = StrSub(String, Rng(ActivePartStartIndex, Index));
            SLL_QUEUE_PUSH(HeadPart, TailPart, Part);
        } else if (String.Str[Index] == '`') {
            StrPart* Part = ArenaPushArrayZero(
                Scratch.MemPool,
                StrPart,
                1
            );

            Part->Flags = ActivePartFlags;
            Part->String = StrSub(String, Rng(ActivePartStartIndex, Index));
            SLL_QUEUE_PUSH(HeadPart, TailPart, Part);
            ActivePartStartIndex = Index + 1;
            ActivePartFlags ^= STR_PART_FLAG_CODE;
        }
    }

    FancyStrList FStrings = {};

    for (StrPart* Part = HeadPart; Part; Part = Part->Next) {
        FancyStr FString = {};

        {
            FString.String = Part->String;
            FString.Params.Font = UIHeadFont();
            FString.Params.Colour = UIColourFromName("text"_s8);
            FString.Params.Size = UIHeadFontSize();
            FString.Params.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_MAIN);

            if (Part->Flags & STR_PART_FLAG_CODE) {
                FString.Params.Font = AppFontFromSlot(APP_FONT_SLOT_CODE);
                FString.Params.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_CODE);
                FString.Params.Colour = AppRGBAFromCodeColourSlot(APP_CODE_COLOUR_SLOT_CODE_DEFAULT);
            }
        }

        ListPush(MemPool, &FStrings, &FString);
    }

    ReleaseScratch(Scratch);

    return FStrings;
}

AppWatchRowCellInfo 
AppInfoFromWatchRowCell(
    Arena* MemPool, 
    EVRow* Row, 
    EVStringFlag Flags, 
    AppWatchRowInfo* RowInfo, 
    AppWatchCell* Cell, 
    FontTag Font, 
    f32 FontSize, 
    f32 MaxSizePX
) {
    TempArena Scratch = GetScratch(&MemPool, 1);
    AppWatchRowCellInfo Result = {};

    Result.Flags = Cell->Flags;
    Result.ViewUIRule = EMPTY_APP_VIEW_UI_RULE_VALUE;
    Result.Config = EMPTY_CFG_NODE_VALUE;

    EvalType* BlockType = EvalTypeFromKey(Row->Block->Eval.IRTree.TypeKey);
    EvalType* CellType = EvalTypeFromKey(Cell->Eval.IRTree.TypeKey);
    MDNodePtrList CellSchemas = ConfigSchemasFromName(
        Scratch.MemPool,
        APP_STATE->CfgSchemaTable,
        CellType->Name
    );

    if (!Cell->Eval.Space.U64s[1] && CellSchemas.Count)
        Result.Config = AppConfigFromEvalSpace(Cell->Eval.Space);

    Result.CommandName = AppCommandNameFromEval(Cell->Eval);
    Result.FilePath = AppFilePathFromEval(MemPool, Cell->Eval);

    for (
        EvalType* __Type = CellType;
        __Type->Kind == EVAL_TYPE_KIND_LENS;
        __Type = EvalTypeFromKey(__Type->DirectTypeKey)
    ) {
        AppViewUIRule* ViewUIRule = AppViewUIRuleFomStr(__Type->Name);

        if (ViewUIRule != EMPTY_APP_VIEW_UI_RULE_VALUE) {
            Result.ViewUIRule = ViewUIRule;
            break;
        }
    }

    for (
        EvalType* __Type = CellType;
        __Type->Kind != EVAL_TYPE_KIND_NULL;
        __Type = EvalTypeFromKey(__Type->DirectTypeKey)
    ) {
        if (__Type->Kind == EVAL_TYPE_KIND_META_DESCRIPTION) {
            Result.Description = __Type->Name;
            break;
        }
    }

    if (Cell->Kind == APP_WATCH_CELL_KIND_EVAL) {
        if (
            Cell->Flags & APP_WATCH_CELL_FLAG_EXPR &&
            Cell->Flags & APP_WATCH_CELL_FLAG_NO_EVAL
        ) {
            if (RowInfo->ExprIsEditable)
                Result.Flags |= APP_WATCH_CELL_FLAG_CAN_EDIT;
        } else {
            if (
                EVTypeKeyIsEditable(Cell->Eval.IRTree.TypeKey) &&
                Cell->Eval.IRTree.Mode == EVAL_MODE_OFFSET
            ) {
                Result.Flags |= APP_WATCH_CELL_FLAG_CAN_EDIT;
            }
        }
    }

    switch (Cell->Eval.Space.Kind) {
        default: {
            if (
                Cell->Eval.Msgs.MaxKind > EVAL_MSG_KIND_NULL &&
                !(Cell->Flags & APP_WATCH_CELL_FLAG_NO_EVAL)
            ) {
                AppFont(APP_FONT_SLOT_MAIN) {
                    FancyStrParams Params = {
                        AppFontFromSlot(APP_FONT_SLOT_MAIN),
                        AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
                        UIColourFromName("text"_s8),
                        UIHeadFontSize()
                    };
                    FancyStrList FStrings = {};

                    UITag("weak"_s8) {
                        FancyStrParams Overrides = {};

                        Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
                        Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
                        Overrides.Colour = UIColourFromName("text"_s8);
                        ListPush(
                            MemPool,
                            &FStrings,
                            &Params,
                            &Overrides,
                            APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_WARNING_BIG]
                        );
                        ListPush(
                            MemPool,
                            &FStrings,
                            &Params,
                            NULL,
                            "  "_s8
                        );

                        for (
                            EvalMsg* Msg = Cell->Eval.Msgs.Head;
                            Msg;
                            Msg = Msg->Next
                        ) {
                            FancyStrList MsgFStrings = AppFStrFromRichString(
                                MemPool,
                                Msg->Text
                            );

                            if (Msg->Next) {
                                ListPush(
                                    MemPool,
                                    &FStrings,
                                    &Params,
                                    NULL,
                                    " "_s8
                                );
                            }
                        }
                    }

                    Result.ExprFStrings = FStrings;
                }
            } else if (Result.Config != EMPTY_CFG_NODE_VALUE) {
                Result.ExprFStrings = AppTitleFStrFromConfig(
                    MemPool, 
                    Result.Config, 
                    FALSE
                );
                Result.Flags |= APP_WATCH_CELL_FLAG_BTN;
            } else if (Result.CommandName.Size) {
                if (Cell->PX != 0.0f) {
                    FancyStrParams Params = {
                        AppFontFromSlot(APP_FONT_SLOT_MAIN),
                        AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
                        UIColourFromName("text"_s8),
                        UIHeadFontSize()
                    };
                    FancyStrParams Overrides = {};

                    Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
                    Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
                    Overrides.Colour = UIColourFromName("text"_s8);

                    FancyStrList FancyStrings = {};

                    UITag("weak"_s8) {
                        ListPush(
                            MemPool,
                            &FancyStrings,
                            &Params,
                            &Overrides,
                            APP_ICON_KIND_TEXT_TABLE[AppIconKindFromCodeName(Result.CommandName)]
                        );
                    }

                    Result.ExprFStrings = FancyStrings;
                } else {
                    Result.ExprFStrings = AppTitleFStrFromCodeName(
                        MemPool,
                        Result.CommandName
                    );
                }

                Result.Flags |= APP_WATCH_CELL_FLAG_BTN;
            } else if (Result.FilePath.Size) {
                b32 NeedFolder = !StrMatch(RowInfo->GroupCfgName, "folder"_s8, 0);

                Result.ExprFStrings = AppTitleFStrFromFilePath(
                    MemPool, 
                    Result.FilePath, 
                    NeedFolder
                );
                Result.Flags |= APP_WATCH_CELL_FLAG_BTN;
            } else {
                FancyStrList ExprFStrings = {};

                if (Cell->Flags & APP_WATCH_CELL_FLAG_EXPR) {
                    b32 IsNonCode = FALSE;
                    Str8 ExprString = {};

                    if (!ExprString.Size) {
                        for (
                            EvalType* T = EvalTypeFromKey(Cell->Eval.IRTree.TypeKey);
                            T != &EMPTY_EVAL_TYPE_VALUE;
                            T = EvalTypeFromKey(T->DirectTypeKey)
                        ) {
                            if (T->Kind == EVAL_TYPE_KIND_META_DISPLAY_NAME) {
                                IsNonCode = TRUE;
                                ExprString = T->Name;
                                break;
                            }
                        }
                    }

                    if (!ExprString.Size) {
                        ExprString = Cell->Eval.String;

                        if (
                            Cell->Eval.ParentKey != EMPTY_EVAL_KEY_VALUE &&
                            !(BlockType->Flags & EVAL_TYPE_FLAG_EDITABLE_CHILDREN) &&
                            Cell->Eval.Msgs.MaxKind == EVAL_MSG_KIND_NULL
                        ) {
                            EvalExpr* NotableExpr = Cell->Eval.Expr;

                            for (
                                b32 Okay = FALSE;
                                !Okay;
                            ) {
                                switch (NotableExpr->Kind) {
                                    default: {
                                        Okay = TRUE;
                                    } break;

                                    case EVAL_EXPR_KIND_CAST: {
                                        NotableExpr = NotableExpr->Tail;
                                    } break;

                                    case EVAL_EXPR_KIND_REF: {
                                        NotableExpr = NotableExpr->Ref;
                                    } break;
                                }
                            }

                            switch (NotableExpr->Kind) {
                                default: {} break;

                                case EVAL_EXPR_KIND_ARRAY_INDEX: {
                                    ExprString = ArenaPushStrFmt(
                                        MemPool,
                                        "[%S]",
                                        PRINT_STR(EvalStrFromExpr(MemPool, NotableExpr->Tail, ""_s8))
                                    );
                                } break;

                                case EVAL_EXPR_KIND_MEMBER_ACCESS: {
                                    TempArena Scratch = GetScratch(&MemPool, 1);
                                    Str8 MemberName = NotableExpr->Head->Next->String;
                                    Str8 FancyName = {};

                                    if (
                                        !(Flags & EV_STR_FLAG_PRETTY_NAMES) &&
                                        (
                                            Cell->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG ||
                                            Cell->Eval.Space.Kind == EVAL_SPACE_KIND_FILE ||
                                            Cell->Eval.Space.Kind == EVAL_SPACE_KIND_FILESYSTEM
                                        )
                                    ) {
                                        FancyName = AppDisplayFromCodeName(MemberName);
                                    }

                                    if (FancyName.Size) {
                                        IsNonCode = TRUE;
                                        ExprString = FancyName;
                                    } else if (MemberName.Size) {
                                        ExprString = ArenaPushStrFmt(
                                            MemPool,
                                            ".%S",
                                            PRINT_STR(MemberName)
                                        );
                                    }

                                    ReleaseScratch(Scratch);
                                } break;
                            }
                        }
                    }

                    if (IsNonCode) {
                        FancyStrParams Params = {
                            AppFontFromSlot(APP_FONT_SLOT_MAIN),
                            AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
                            UIColourFromName("text"_s8),
                            FontSize,
                            0,
                            0
                        };

                        ListPush(
                            MemPool,
                            &ExprFStrings,
                            &Params,
                            NULL,
                            ExprString
                        );
                    }
                }

                FancyStrList EvalFStrings = {};

                if (!(Cell->Flags & APP_WATCH_CELL_FLAG_NO_EVAL)) {
                    EVStrParams StringParams = {
                        Flags,
                        10
                    };

                    if (Cell->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG)
                        StringParams.Flags |= EV_STR_FLAG_DISABLE_STR_QUOTES;

                    b32 IsCode = TRUE;

                    {
                        EvalType* __Type = EvalTypeFromKey(
                            EvalTypeKeyUnwrap(
                                Cell->Eval.IRTree.TypeKey,
                                EVAL_TYPE_UNWRAP_FLAG_META
                            )
                        );

                        if (__Type->Flags & (EVAL_TYPE_FLAG_IS_PLAIN_TEXT | EVAL_TYPE_FLAG_IS_PATH_TEXT))
                            IsCode = FALSE;
                    }

                    Str8 String = AppValueStrFromEval(
                        MemPool,
                        AppViewQueryInput(),
                        &StringParams,
                        Font,
                        FontSize,
                        MaxSizePX,
                        Cell->Eval
                    );

                    if (!IsCode) {
                        UITag("weak"_s8) {
                            FancyStrParams Params = {
                                AppFontFromSlot(APP_FONT_SLOT_MAIN),
                                AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
                                UIColourFromName("text"_s8),
                                FontSize,
                                0,
                                0
                            };

                            ListPush(
                                MemPool,
                                &EvalFStrings,
                                &Params,
                                NULL,
                                String
                            );
                            Result.Flags |= APP_WATCH_CELL_FLAG_IS_NONE_CODE;
                        }
                    }
                }

                if (Cell->Flags & APP_WATCH_CELL_FLAG_NO_EVAL) {
                    Result.EvalFStrings = ExprFStrings;
                } else {
                    Result.ExprFStrings = ExprFStrings;
                    Result.EvalFStrings = EvalFStrings;
                }
            }
        } break;

        case APP_EVAL_SPACE_KIND_META_THEME: {
            Str8 Name = EvalStrFromID(Cell->Eval.Value.U64);
            FancyStrParams Params = {
                AppFontFromSlot(APP_FONT_SLOT_MAIN),
                AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
                UIColourFromName("text"_s8),
                UIHeadFontSize()
            };
            FancyStrParams Overrides = {};
            FancyStrList FancyStrings = {};

            UITag("weak"_s8) {
                Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
                Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
                Overrides.Colour = UIColourFromName("text"_s8);
                ListPush(
                    MemPool,
                    &FancyStrings,
                    &Params,
                    &Overrides,
                    APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_PALETTE]
                );
            }

            ListPush(
                MemPool,
                &FancyStrings,
                &Params,
                NULL,
                "  "_s8
            );
            ListPush(
                MemPool,
                &FancyStrings,
                &Params,
                NULL,
                Name
            );

            {
                Access* Acc = AccessOpen();
                MDNode* ThemeTree = AppThemeTreeFromName(Scratch.MemPool, Acc, Name);
                u64 ColourIndex = 0;

                for (
                    MDNode* N = ThemeTree;
                    ColourIndex < 4 && !MDNodeIsEmpty(N);
                    N = MDNodeRecordDepthFirstPreOrder(N, ThemeTree).Next
                ) {
                    if (StrMatch(N->String, "theme_colour"_s8, 0)) {
                        Str8 Tags = MDChildFromStr(
                            N,
                            "tags"_s8,
                            0
                        )->Head->String;

                        if (
                            StrMatch(Tags, "background"_s8, 0) ||
                            StrMatch(Tags, "text"_s8, 0) ||
                            StrMatch(Tags, "focus border"_s8, 0) ||
                            StrMatch(Tags, "menu_bar background"_s8, 0) ||
                            StrMatch(Tags, "tab background"_s8, 0) ||
                            StrMatch(Tags, "code_default"_s8, 0)
                        ) {
                            u64 Colour = U64FromStr(
                                MDChildFromStr(N, "value"_s8, 0)->Head->String
                            );

                            if (Colour) {
                                ListPush(
                                    MemPool,
                                    &FancyStrings,
                                    &Params,
                                    NULL,
                                    "  "_s8
                                );
                                Overrides = {};
                                Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
                                Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
                                Overrides.Colour = LinearFromSRGBA(RGBAFromU32((u32) Colour));
                                ListPush(
                                    MemPool,
                                    &FancyStrings,
                                    &Params,
                                    &Overrides,
                                    APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_CIRCLE_FILLED]
                                );
                                ++ColourIndex;
                            }
                        }
                    }
                }

                AccessClose(Acc);
            }

            Result.EvalFStrings = FancyStrings;
        } break;

        // case APP_EVAL_SPACE_KIND_PID_LIVE_TAG: {
        //     if (Result.Entity != EMPTY_PID_KEY_VALUE) {}
        // } break;

        // case APP_EVAL_SPACE_KIND_PID_LIVE_PROPERTY: {
        //     if (Result.Entity != EMPTY_PID_KEY_VALUE) {}
        // } break;
    }

    ReleaseScratch(Scratch);
    return Result;
}

AppWatchRowInfo 
AppWatchRowInfoFromRow(Arena* MemPool, EVRow* Row)
{
    AppWatchRowInfo Result = {};
    
    Result.CanExpand = EVRowIsExpandable(Row);
    Result.GroupCfgParent = EMPTY_CFG_NODE_VALUE;
    Result.GroupCfgChild = EMPTY_CFG_NODE_VALUE;
    Result.ViewUIRule = EMPTY_APP_VIEW_UI_RULE_VALUE;

    TempArena Scratch = GetScratch(&MemPool, 1);
    EvalType* RowType = EvalTypeFromKey(Row->Eval.IRTree.TypeKey);
    EVKey Key = Row->Key;
    EVBlock* Block = Row->Block;
    b32 BlockIsRoot = (Block->Parent == EMPTY_EV_BLOCK_VALUE);
    EvalEval BlockEval = EvalEvalFromKey(Row->Block->Eval.Key);
    EvalTypeKey BlockTypeKey = EvalTypeKeyUnwrap(
        BlockEval.IRTree.TypeKey, 
        EVAL_TYPE_UNWRAP_FLAG_META
    );
    EvalTypeKind BlockTypeKind = EvalTypeKindFromKey(
        BlockTypeKey
    );
    EvalType* BlockType = EvalTypeFromKey(BlockTypeKey);
    ConfigNode* EvalledCfg = AppConfigFromEvalSpace(Row->Eval.Space);
    b32 IsTopLevel = FALSE;

    if (EvalledCfg != EMPTY_CFG_NODE_VALUE) {
        EvalTypeKey TopLevelTypeKey = EvalStr2TypeKeyMapLookup(
            APP_STATE->MetaName2TypeMap,
            EvalledCfg->String
        );

        IsTopLevel = (
            !Row->Eval.Value.U64 &&
            TopLevelTypeKey == Row->Eval.IRTree.TypeKey
        );
    }

    if (
        BlockType->Flags & EVAL_TYPE_FLAG_EDITABLE_CHILDREN ||
        (
            Row->Eval.Key == EMPTY_EVAL_KEY_VALUE && 
            Row->Eval.Expr == EMPTY_EVAL_EXPR_VALUE
        )
    ) {
        Result.ExprIsEditable = TRUE;
    }

    if (!BlockIsRoot) {
        if (
            BlockTypeKind == EVAL_TYPE_KIND_SET &&
            (
                BlockEval.Space.Kind == APP_EVAL_SPACE_KIND_META_QUERY ||
                BlockEval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG
            )
        ) {
            Result.GroupCfgParent = AppConfigFromEvalSpace(BlockEval.Space);
        }
    }

    if (!BlockIsRoot) {
        if (BlockTypeKind == EVAL_TYPE_KIND_SET) {
            Str8 SingularName = AppSingularFromCodeNamePlural(BlockType->Name);

            if (SingularName.Size) {
                Result.GroupCfgName = SingularName;
            } else {
                Result.GroupCfgName = BlockType->Name;
            }
        }
    }

    if (!BlockIsRoot) {
        if (
            Result.GroupCfgName.Size && 
            BlockType->ExpandRule.ExpandIDFromNumFunc == EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_NAME(ConfigsSlice)
        ) {
            ConfigID ID = Row->Key.ChildID;

            Result.GroupCfgChild = ConfigNodeFromID(ID);
        }
    }

    Result.ViewUIRule = AppViewUIRuleFomStr(Row->Block->VisualExpandRule->String);

    EvalType* MaybeTableType = BlockType;

    for (;;) {
        if (
            MaybeTableType->Kind == EVAL_TYPE_KIND_LENS &&
            StrMatch(MaybeTableType->Name, "columns"_s8, 0)
        ) {
            break;
        } else if (MaybeTableType->Kind == EVAL_TYPE_KIND_LENS) {
            MaybeTableType = EvalTypeFromKey(MaybeTableType->DirectTypeKey);
            continue;
        } else {
            break;
        }
    }

    if (
        Block->Parent != EMPTY_EV_BLOCK_VALUE &&
        MaybeTableType->Kind == EVAL_TYPE_KIND_LENS &&
        StrMatch(MaybeTableType->Name, "columns"_s8, 0) &&
        MaybeTableType->Count >= 1
    ) {
        u64 ColumnCount = MaybeTableType->Count;

        Result.CellStyleKey = ArenaPushStrFmt(
            MemPool,
            "table_%llu_cols",
            ColumnCount
        );

        ConfigNode* View = ConfigNodeFromID(Registers()->View);
        ConfigNode* Style = ConfigNodeChildFromStr(View, Result.CellStyleKey);
        ConfigNode* WCfg = Style->Head;
        f32 NextPercent = 0.0f;

#define TAKE_PERCENT() (NextPercent = (f32) F64FromStr(WCfg->String), WCfg = WCfg->Next, NextPercent)

        EvalParentKey(Row->Eval.Key) {
            for (
                u64 Index = 0;
                Index < MaybeTableType->Count;
                ++Index
            ) {
                EvalEval CellEval = EvalEvalFromExpr(MaybeTableType->Args[Index]);

                WatchCellPushNew(
                    MemPool, 
                    &Result.Cells, 
                    APP_WATCH_CELL_KIND_EVAL, 
                    CellEval, 
                    __Cell.DefaultPercent = 1.0f / MaybeTableType->Count,
                    __Cell.Percent = TAKE_PERCENT()
                );
            }
        }

        Result.CanExpand = FALSE;

#undef TAKE_PERCENT
    } else if (
        Row->Eval.Space.Kind == EVAL_SPACE_KIND_FILESYSTEM &&
        EvalTypeKindFromKey(Row->Eval.IRTree.TypeKey) == EVAL_TYPE_KIND_SET
    ) {
        EvalType* __Type = EvalTypeFromKey(Row->Eval.IRTree.TypeKey);
        Str8 FilePath = EvalStrFromID(Row->Eval.Value.U64);

        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                APP_WATCH_CELL_FLAG_EXPR |
                APP_WATCH_CELL_FLAG_NO_EVAL |
                APP_WATCH_CELL_FLAG_INDENTED |
                APP_WATCH_CELL_FLAG_BTN |
                APP_WATCH_CELL_FLAG_IS_NONE_CODE
            ),
            __Cell.Percent = 1.0f
        );

        if (StrMatch(__Type->Name, "file"_s8, 0))
            Result.CanExpand = FALSE;
    } else if (
        ConfigNodeChildFromStr(
            ConfigNodeFromID(Registers()->View), 
            "autocomplete"_s8
        ) != EMPTY_CFG_NODE_VALUE
    ) {
        Result.CanExpand = FALSE;
        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                APP_WATCH_CELL_FLAG_EXPR |
                APP_WATCH_CELL_FLAG_NO_EVAL |
                APP_WATCH_CELL_FLAG_BTN |
                APP_WATCH_CELL_FLAG_INDENTED
            ),
            __Cell.Percent = 1.0f
        );
    } else if (
        ConfigNodeChildFromStr(
            ConfigNodeFromID(Registers()->View),
            "lister"_s8
        ) != EMPTY_CFG_NODE_VALUE
    ) {
        Result.CanExpand = FALSE;

        AppWatchCellFlag ExtraFlags = APP_WATCH_CELL_FLAG_EXPR;

        if (Row->Eval.Msgs.MaxKind != EVAL_MSG_KIND_NULL)
            ExtraFlags = APP_WATCH_CELL_FLAG_EXPR | APP_WATCH_CELL_FLAG_NO_EVAL;

        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                ExtraFlags | 
                APP_WATCH_CELL_FLAG_BTN |
                APP_WATCH_CELL_FLAG_INDENTED
            ),
            __Cell.Percent = 1.0f
        );
    } else if (IsTopLevel && EvalledCfg != EMPTY_CFG_NODE_VALUE) {
        ConfigNode* Cfg = EvalledCfg;

        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                APP_WATCH_CELL_FLAG_EXPR |
                APP_WATCH_CELL_FLAG_NO_EVAL |
                APP_WATCH_CELL_FLAG_BTN |
                APP_WATCH_CELL_FLAG_INDENTED
            ),
            __Cell.Percent = 1.0f
        );

        MDNodePtrList Schemas = ConfigSchemasFromName(
            Scratch.MemPool,
            APP_STATE->CfgSchemaTable,
            Cfg->String
        );

        for (MDNodePtrNode* N = Schemas.Head; N; N = N->Next) {
            MDNode* Schema = N->V;
            MDNode* CommandsRoot = MDTagFromStr(Schema, "row_commands"_s8, 0);

            for EACH_MDNODE(Cmd, CommandsRoot->Head) {
                b32 IsFileOnly = MDNodeHasTag(Cmd, "file"_s8, 0);
                b32 IsCmdLineOnly = MDNodeHasTag(Cmd, "cmd_line"_s8, 0);
                
                if (
                    IsFileOnly &&
                    EvalEvalFromStr(AppExpressionFromConfig(EvalledCfg)).Space.Kind != EVAL_SPACE_KIND_FILE
                ) {
                    continue;
                }

                if (IsCmdLineOnly) {
                    b32 IsCmdLine = FALSE;
                    ConfigNode* CmdLine = ConfigNodeChildFromStr(
                        ConfigNodeRoot(),
                        "command_line"_s8
                    );

                    for (
                        ConfigNode* P = EvalledCfg->Parent; 
                        P != EMPTY_CFG_NODE_VALUE; 
                        P = P->Parent
                    ) {
                        if (P == CmdLine) {
                            IsCmdLine = TRUE;
                            break;
                        }
                    }

                    if (!IsCmdLine)
                        continue;
                }

                Str8 CommandName = Cmd->String;
                AppCommandKind CmdKind = AppCommandKindFromStr(CommandName);

                switch (CmdKind) {
                    default: {} break;

                    case APP_COMMAND_KIND_ENABLE_CONFIG: {
                        b32 IsDisabled = AppIsDisabled(Cfg);

                        if (!IsDisabled)
                            CmdKind = APP_COMMAND_KIND_DISABLE_CONFIG;
                    } break;

                    case APP_COMMAND_KIND_DISABLE_CONFIG: {
                        b32 IsDisabled = AppIsDisabled(Cfg);

                        if (IsDisabled)
                            CmdKind = APP_COMMAND_KIND_ENABLE_CONFIG;
                    } break;

                    case APP_COMMAND_KIND_SELECT_CONFIG: {
                        b32 IsDisabled = AppIsDisabled(Cfg);

                        if (!IsDisabled)
                            CmdKind = APP_COMMAND_KIND_DESELECT_CONFIG;
                    } break;

                    case APP_COMMAND_KIND_DESELECT_CONFIG: {
                        b32 IsDisabled = AppIsDisabled(Cfg);

                        if (IsDisabled)
                            CmdKind = APP_COMMAND_KIND_SELECT_CONFIG;
                    } break;
                }

                if (
                    CmdKind == APP_COMMAND_KIND_ENABLE_CONFIG || 
                    CmdKind == APP_COMMAND_KIND_DISABLE_CONFIG
                ) {
                    WatchCellPushNew(
                        MemPool,
                        &Result.Cells,
                        APP_WATCH_CELL_KIND_EVAL,
                        EvalEvalWrap(Row->Eval, "enabled"_s8),
                        __Cell.Flags = APP_WATCH_CELL_FLAG_BACKGROUND,
                        __Cell.PX = FloorF32(UIHeadFontSize() * 5.5f)
                    );
                } else if (CmdKind != APP_COMMAND_KIND_NULL) {
                    Str8 CmdName = APP_COMMAND_KIND_INFO_TABLE[CmdKind].String;

                    WatchCellPushNew(
                        MemPool,
                        &Result.Cells,
                        APP_WATCH_CELL_KIND_EVAL,
                        EvalEvalFromStrFmt("query:commands.%S", PRINT_STR(CmdName)),
                        __Cell.Flags = APP_WATCH_CELL_FLAG_ACTIVATE_WITH_SINGLE_CLICK | APP_WATCH_CELL_FLAG_BTN,
                        __Cell.PX = FloorF32(UIHeadFontSize() * 3.0f)
                    );
                }
            }
        }
    } else if (
        Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_QUERY ||
        (
            Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG &&
            EvalTypeKindFromKey(
                EvalTypeKeyUnwrap(
                    Row->Eval.IRTree.TypeKey,
                    EVAL_TYPE_UNWRAP_FLAG_META
                )
            ) == EVAL_TYPE_KIND_SET
        )
    ) {
        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                APP_WATCH_CELL_FLAG_EXPR |
                APP_WATCH_CELL_FLAG_NO_EVAL |
                APP_WATCH_CELL_FLAG_INDENTED
            ),
            __Cell.Percent = 1.0f
        );
    } else if (Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CMD) {
        EvalType* __Type = EvalTypeFromKey(Row->Eval.IRTree.TypeKey);

        if (__Type->Kind == EVAL_TYPE_KIND_SET) {
            WatchCellPushNew(
                MemPool,
                &Result.Cells,
                APP_WATCH_CELL_KIND_EVAL,
                Row->Eval,
                __Cell.Flags = (
                    APP_WATCH_CELL_FLAG_EXPR |
                    APP_WATCH_CELL_FLAG_NO_EVAL |
                    APP_WATCH_CELL_FLAG_INDENTED
                ),
                __Cell.Percent = 1.0f
            );
        } else {
            WatchCellPushNew(
                MemPool,
                &Result.Cells,
                APP_WATCH_CELL_KIND_EVAL,
                Row->Eval,
                __Cell.Flags = (
                    APP_WATCH_CELL_FLAG_EXPR |
                    APP_WATCH_CELL_FLAG_NO_EVAL |
                    APP_WATCH_CELL_FLAG_INDENTED |
                    APP_WATCH_CELL_FLAG_BTN |
                    APP_WATCH_CELL_FLAG_ACTIVATE_WITH_SINGLE_CLICK
                ),
                __Cell.Percent = 1.0f
            );
        }
    } else if (Result.ViewUIRule != EMPTY_APP_VIEW_UI_RULE_VALUE) {
        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_VIEW_UI,
            Row->Eval,
            __Cell.Percent = 1.0f
        );
    } else if (
        Row->Eval.Expr == EMPTY_EVAL_EXPR_VALUE &&
        Result.GroupCfgName.Size &&
        Result.GroupCfgChild == EMPTY_CFG_NODE_VALUE
    ) {
        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                APP_WATCH_CELL_FLAG_EXPR |
                APP_WATCH_CELL_FLAG_NO_EVAL |
                APP_WATCH_CELL_FLAG_INDENTED
            ),
            __Cell.Percent = 1.0f
        );
    } else if (
        Result.GroupCfgChild == EMPTY_CFG_NODE_VALUE &&
        EvalTypeKindFromKey(
            EvalTypeKeyUnwrap(Row->Eval.IRTree.TypeKey, EVAL_TYPE_UNWRAP_FLAG_ALL_DECORATIVE)
        ) == EVAL_TYPE_KIND_BOOL &&
        (
            Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG ||
            Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CMD
        )
    ) {
        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = APP_WATCH_CELL_FLAG_EXPR | APP_WATCH_CELL_FLAG_INDENTED,
            __Cell.Percent = 1.0f
        );
    } else {
        if (
            Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG ||
            Row->Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CMD ||
            Row->Eval.Space.Kind == EVAL_SPACE_KIND_FILE
        ) {
            EvalTypeKey SubstantiveRowEvalType = EvalTypeKeyUnwrap(
                Row->Eval.IRTree.TypeKey,
                EVAL_TYPE_UNWRAP_FLAG_META
            );

            if (
                EvalTypeKindFromKey(SubstantiveRowEvalType) == EVAL_TYPE_KIND_ARRAY &&
                EvalTypeKindFromKey(EvalTypeKeyDirect(SubstantiveRowEvalType)) == EVAL_TYPE_KIND_U8
            ) {
                Result.CanExpand = FALSE;
            }
        }

        Result.CellStyleKey = "normal"_s8;

        ConfigNode* View = ConfigNodeFromID(Registers()->View);
        ConfigNode* Style = ConfigNodeChildFromStr(View, Result.CellStyleKey);
        ConfigNode* WCfg = Style->Head;
        f32 NextPercent = 0.0f;

#define TAKE_PERCENT() (NextPercent = (f32) F64FromStr(WCfg->String), WCfg = WCfg->Next, NextPercent)

        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.Flags = (
                APP_WATCH_CELL_FLAG_EXPR | 
                APP_WATCH_CELL_FLAG_NO_EVAL | 
                APP_WATCH_CELL_FLAG_INDENTED
            ),
            __Cell.DefaultPercent = 0.35f,
            __Cell.Percent = TAKE_PERCENT()
        );
        WatchCellPushNew(
            MemPool,
            &Result.Cells,
            APP_WATCH_CELL_KIND_EVAL,
            Row->Eval,
            __Cell.DefaultPercent = 0.65f,
            __Cell.Percent = TAKE_PERCENT()
        );

#undef TAKE_PERCENT
    }

    if (AbsF32(Result.Cells.TotalPercent - 1.0f) > 0.01f) {
        f32 Sum = Result.Cells.TotalPercent;

        if (Sum <= 0.0f)
            Sum = 1.0f;

        for (AppWatchCell* C = Result.Cells.Head; C; C = C->Next) {
            C->Percent /= Sum;
        }
    }

    ReleaseScratch(Scratch);

    return Result;
}

v2i64 
AppTableFromWatchPoint(EVBlockRangeList* BlockRanges, AppWatchPoint Point)
{
    v2i64 Table = {};
    TempArena Scratch = GetScratch(NULL, 0);
    u64 Num = EVNumFromKey(BlockRanges, Point.Key);
    EVRow* Row = EVRowFromNum(
        Scratch.MemPool, 
        AppViewEvalView(), 
        BlockRanges,
        Num
    );
    AppWatchRowInfo RowInfo = AppWatchRowInfoFromRow(Scratch.MemPool, Row);

    Table.X = 0;

    {
        i64 X = 0;

        for (
            AppWatchCell* Cell = RowInfo.Cells.Head;
            Cell;
            Cell = Cell->Next, ++X
        ) {
            u64 CellID = AppIDFromWatchCell(Cell);

            if (CellID == Point.CellID) {
                Table.X = X;
                break;
            }
        }
    }

    Table.Y = (i64) Num;
    ReleaseScratch(Scratch);

    return Table;
}

AppWatchPoint 
AppWatchPointFromTable(EVBlockRangeList* BlockRanges, v2i64 Table)
{
    AppWatchPoint Point = {};
    TempArena Scratch = GetScratch(NULL, 0);
    EVRow* Row = EVRowFromNum(
        Scratch.MemPool, 
        AppViewEvalView(), 
        BlockRanges, 
        (u64) Table.Y
    );
    AppWatchRowInfo RowInfo = AppWatchRowInfoFromRow(Scratch.MemPool, Row);

    {
        i64 X = 0;

        for (
            AppWatchCell* Cell = RowInfo.Cells.Head;
            Cell;
            Cell = Cell->Next, ++X
        ) {
            if (X == Table.X) {
                Point.CellID = AppIDFromWatchCell(Cell);
                break;
            }
        }
    }

    Point.Key = Row->Key;
    Point.ParentKey = Row->Block->Key;
    ReleaseScratch(Scratch);

    return Point;
}

u64 
AppIDFromWatchCell(AppWatchCell* Cell)
{
    u64 Result = 5381;

    Result = Hash(Result, Str8Struct(&Cell->Kind));
    Result = Hash(Result, Str8Struct(&Cell->Index));

    if (Cell->Index)
        Result = Hash(Result, Str8Struct(&Cell->DefaultPercent));

    return Result;
}

AppWatchViewTextEditState* 
AppWatchViewTextEditStateFromPoint(AppWatchViewState* WV, AppWatchPoint Point)
{
    AppWatchViewTextEditState* Result = &WV->DummyTextEditState;

    if (WV->TextEditStateSlotsCount && WV->IsTextEditing) {
        u64 PointHash = Hash(Point.Key);
        u64 SlotIndex = PointHash % WV->TextEditStateSlotsCount;

        for (
            AppWatchViewTextEditState* S = WV->TextEditStateSlots[SlotIndex];
            S;
            S = S->PointHashNext
        ) {
            if (Point == S->Point) {
                Result = S;
                break;
            }
        }
    }

    return Result;
}

AppViewUIRuleMap*
AppViewUIRuleMapAlloc(Arena* MemPool, u64 SlotsCount)
{
    AppViewUIRuleMap* Result = ArenaPushArrayZero(
        MemPool,
        AppViewUIRuleMap,
        1
    );

    Result->SlotsCount = SlotsCount;
    Result->Slots = ArenaPushArrayZero(
        MemPool,
        AppViewUIRuleSlot,
        Result->SlotsCount
    );

    return Result;
}

void
MapInsert(
    Arena* MemPool, 
    AppViewUIRuleMap* Map, 
    Str8 String, 
    AppViewUIFuncType* ViewUIFunc
) {
    u64 StringHash = DHash(String);
    u64 SlotIndex = StringHash % Map->SlotsCount;
    AppViewUIRuleNode* Node = ArenaPushArrayZero(
        MemPool,
        AppViewUIRuleNode,
        1
    );

    Node->V.Name = ArenaPushStrCpy(MemPool, String);
    Node->V.ViewUIFunc = ViewUIFunc;
    SLL_QUEUE_PUSH(
        Map->Slots[SlotIndex].Head,
        Map->Slots[SlotIndex].Tail,
        Node
    );
}

AppViewUIRule*
AppViewUIRuleFromStr(Str8 String)
{
    AppViewUIRule* Result = EMPTY_APP_VIEW_UI_RULE_VALUE;
    AppViewUIRuleMap* Map = APP_STATE->ViewUIRuleMap;
    u64 StringHash = DHash(String);
    u64 SlotIndex = StringHash % Map->SlotsCount;

    for (
        AppViewUIRuleNode* Node = Map->Slots[SlotIndex].Head;
        Node;
        Node = Node->Next
    ) {
        if (StrMatch(Node->V.Name, String, 0)) {
            Result = &Node->V;
            break;
        }
    }

    return Result;
}

Str8 
AppExpressionFromConfig(ConfigNode* Cfg)
{
    ConfigNode* ExpressionRoot = ConfigNodeChildFromStr(Cfg, "expression"_s8);
    Str8 Result = ExpressionRoot->Head->String;

    return Result;
}

b32 
AppViewWatchRowIsExpanded(AppViewWatchState* WatchState, u64 Key)
{
    if (!WatchState->ExpandedKeysCapacity)
        return FALSE;

    u64 Slot = Key % WatchState->ExpandedKeysCapacity;

    for (u64 Index = 0; Index < WatchState->ExpandedKeysCapacity; ++Index) {
        u64 KeyIndex = (Slot + Index) % WatchState->ExpandedKeysCapacity;

        if (WatchState->ExpandedKeys[KeyIndex] == Key)
            return TRUE;

        if (!WatchState->ExpandedKeys[KeyIndex])
            return FALSE;
    }

    return FALSE;
}

void 
AppViewWatchRowSetExpanded(
    AppViewWatchState* WatchState, 
    u64 Key, 
    b32 IsExpanded
) {
    if (!WatchState->ExpandedKeysCapacity) {
        WatchState->ExpandedKeysCapacity = 4096;
        WatchState->ExpandedKeys = ArenaPushArrayZero(
            WatchState->MemPool, 
            u64, 
            WatchState->ExpandedKeysCapacity
        );
    }

    u64 Slot = Key % WatchState->ExpandedKeysCapacity;
    u64 Target = U64_MAX;

    for (u64 Index = 0; Index < WatchState->ExpandedKeysCapacity; ++Index) {
        u64 KeyIndex = (Slot + Index) % WatchState->ExpandedKeysCapacity;

        if (WatchState->ExpandedKeys[KeyIndex] == Key) {
            Target = KeyIndex;
            break;
        }

        if (!WatchState->ExpandedKeys[KeyIndex]) {
            if (Target == U64_MAX) {
                Target = KeyIndex;
                break;
            }
        }
    }

    if (Target != U64_MAX) {
        WatchState->ExpandedKeys[Target] = IsExpanded ? Key : 0;
    }
}

UISignal 
AppCell(AppCellParams* Params, Str8 String)
{
    TempArena Scratch = GetScratch(NULL, 0);
    b32 DoCursorTrail = AppSettingFromNameB32("cursor_trail"_s8);
    f32 ExpanderSizePX = FloorF32(UIHeadFontSize() * 2.0f);
    UIKey Key = UIKeyFromStr(UIActiveSeedKey(), String);
    b32 IsAutoFocusHot = UIIsKeyAutoFocusHot(Key);
    b32 IsAutoFocusActive = UIIsKeyAutoFocusActive(Key);
    
    if (IsAutoFocusHot)
        UIPushFocusHot(UI_FOCUS_KIND_ON);

    if (IsAutoFocusActive)
        UIPushFocusActive(UI_FOCUS_KIND_ON);

    b32 IsFocusHot = UIIsFocusHot();
    b32 IsFocusActive = UIIsFocusActive();
    b32 IsFocusHotDisabled = (!IsFocusHot && UIHeadFocusHot() == UI_FOCUS_KIND_ON);
    b32 IsFocusActiveDisabled = (!IsFocusActive && UIHeadFocusActive() == UI_FOCUS_KIND_ON);
    b32 BuildToggleSwitch = !!(Params->Kind & APP_CELL_KIND_TOGGLE_SWITCH) && !IsFocusActive;
    b32 BuildSlider = !!(Params->Kind & APP_CELL_KIND_SLIDER) && !IsFocusActive;
    b32 BuildBindings = !!(Params->Kind & APP_CELL_KIND_BINDINGS) && !IsFocusActive;
    b32 BuildLHSNameDesc = (Params->MetaFancyStrings.NodeCount || Params->Description.Size);
    b32 BuildLineEdit = (Params->PreEditValue.Size || Params->ValueFancyStrings.NodeCount);
    b32 BuildNote = (Params->NoteFancyStrings.NodeCount && !IsFocusActive);
    FancyStrList LHSNameFancyStrings = Params->MetaFancyStrings;
    FancyStrList ValueNameFancyStrings = Params->ValueFancyStrings;
    FancyStrList NoteFancyStrings = Params->NoteFancyStrings;
    Str8 AutoCompleteHintString = {};

    if (IsFocusActive)
        AutoCompleteHintString = UIAutoComplete();

    if (IsFocusActive || IsFocusActiveDisabled) {
        UISetNextHoverCursor(CURSOR_KIND_I_BAR);
    }

    UIBox* Box = UIBuildBoxFromKey(
        (
            UI_BOX_KIND_CLICKABLE |
            (!!BuildLHSNameDesc * UI_BOX_KIND_DISABLE_FOCUS_BORDER) |
            (!!(Params->Kind & APP_CELL_KIND_KEYBOARD_CLICKABLE) * UI_BOX_KIND_KEYBOARD_CLICKABLE) |
            UI_BOX_KIND_CLICK_TO_FOCUS |
            (!!(Params->Kind & APP_CELL_KIND_BTN) * UI_BOX_KIND_DRAW_HOT_EFFECTS) |
            (!!(Params->Kind & APP_CELL_KIND_SINGLE_CLICK_ACTIVATE) * UI_BOX_KIND_DRAW_ACTIVE_EFFECTS) |
            (!(Params->Kind & APP_CELL_KIND_NO_BACKGROUND) * UI_BOX_KIND_DRAW_BACKGROUND) |
            (!!(Params->Kind & APP_CELL_KIND_BORDER) * UI_BOX_KIND_DRAW_BORDER) |
            ((IsAutoFocusHot || IsAutoFocusActive) * UI_BOX_KIND_KEYBOARD_CLICKABLE) |
            (IsFocusActive || IsFocusActiveDisabled) * UI_BOX_KIND_CLIP
        ),
        Key
    );

    UIParent(Box) {
        for (i32 Index = 0; Index < Params->Depth; ++Index) {
            UISetNextFlags(UI_BOX_KIND_DRAW_SIDE_LEFT);
            UISpacer(UI_EM(1.0f, 1.0f));
        }
    }

    if (Params->Kind & APP_CELL_KIND_EXPANDER) {
        UIPreferredWidth(UI_PX(ExpanderSizePX, 1.0f)) {
            UIParent(Box) {
                UIFlags(UI_BOX_KIND_DRAW_SIDE_LEFT) {
                    UIFocus(UI_FOCUS_KIND_OFF) {
                        UISignal ExpanderSig = UIExpander(*Params->Expanded, "expander"_s8);

                        if (UI_PRESSED(ExpanderSig))
                            *Params->Expanded ^= TRUE;
                    }
                }
            }
        }
    } else if (Params->Kind & APP_CELL_KIND_EXPANDER_PLACEHOLDER) {
        UIParent(Box) {
            UIPreferredWidth(UI_PX(ExpanderSizePX, 1.0f)) {
                UIFocus(UI_FOCUS_KIND_OFF) {
                    UITag("weak"_s8) {
                        UIFlags(UI_BOX_KIND_DRAW_SIDE_LEFT) {
                            AppFont(APP_FONT_SLOT_ICONS) {
                                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                    UILabel(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_DOT]);
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (Params->Kind & APP_CELL_KIND_EXPANDER_SPACE) {
        UIParent(Box) {
            UIFocus(UI_FOCUS_KIND_OFF) {
                UIFlags(UI_BOX_KIND_DRAW_SIDE_LEFT) {
                    UISpacer(UI_PX(ExpanderSizePX, 1.0f));
                }
            }
        }
    }

    UIBox* LHSBox = EMPTY_UI_BOX_VALUE;

    if (BuildLHSNameDesc) {
        UIParent(Box) {
            UIWidthFill() {
                UIChildLayoutAxis(AXIS_2D_Y) {
                    if(
                        UIHeadTextAlignment() == UI_TEXT_ALIGN_LEFT &&
                        (
                            Params->Kind & (
                                APP_CELL_KIND_EXPANDER | 
                                APP_CELL_KIND_EXPANDER_SPACE |
                                APP_CELL_KIND_EXPANDER_PLACEHOLDER
                            )
                        ) == 0
                    ) {
                        UISpacer(UI_EM(1.0f, 1.0f));
                    }

                    LHSBox = UIBuildBoxFromStr(0, "lhs_box"_s8);
                }
            }
        }
    }

    if (BuildLHSNameDesc) {
        UIParent(LHSBox) {
            UIPadding(UI_EM(3.0f, 0.0f)) {
                UIWidthFill() {
                    UIHeightFill() {
                        FMRangeList FuzzyMatches = {};

                        if (Params->SearchNeedle.Size) {
                            FuzzyMatches = FuzzyFindFStrs(
                                Scratch.MemPool, 
                                &LHSNameFancyStrings, 
                                Params->SearchNeedle
                            );
                        }

                        UIRow() {
                            UIBox* NameBox = UIBuildBoxFromKey(
                                UI_BOX_KIND_DRAW_TEXT,
                                {}
                            );

                            UIBoxEquipDisplayFancyStrs(NameBox, &LHSNameFancyStrings);
                            UIBoxEquipFuzzyMatchRanges(NameBox, &FuzzyMatches);
                        }

                        if (Params->Description.Size) {
                            AppFont(APP_FONT_SLOT_MAIN) {
                                UIFontSize(UIHeadFontSize() * 0.85f) {
                                    UIRow() {
                                        UIBox* DescBox = UILabel(Params->Description).Box;
                                        FMRangeList DescFuzzyMatches = FuzzyFind(
                                            Scratch.MemPool, 
                                            Params->SearchNeedle, 
                                            Params->Description
                                        );

                                        UIBoxEquipFuzzyMatchRanges(DescBox, &DescFuzzyMatches);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    UIBox* EditBox = EMPTY_UI_BOX_VALUE;

    if ((IsFocusActive || IsFocusActiveDisabled) || BuildLineEdit) {
        UIParent(Box) {
            b32 IsEditing = (IsFocusActive || IsFocusActiveDisabled);
            UISize EditBoxSize = UI_PERCENT(1, 0);

            if (BuildLHSNameDesc) {
                if (IsEditing) {
                    EditBoxSize = UI_PX(FloorF32(Length(Box->Rect).X * 0.5f), 1.0f);
                } else {
                    EditBoxSize = UI_SUM_OF_CHILDREN(1);
                }
            }

            UIPreferredWidth(EditBoxSize) {
                if (UIHeadPXHeight() > UIHeadFontSize() * 3.0f) {
                    UISetNextPreferredWidth(UI_SUM_OF_CHILDREN(1));

                    UIColumn() {
                        UIPadding(UI_EM(1, 0)) {
                            UIFocus(UI_FOCUS_KIND_ON) {
                                UIPreferredHeight(UI_EM(3.0f, 1.0f)) {
                                    UICornerRadius(UIHeadFontSize() * 0.5f) {
                                        EditBox = UIBuildBoxFromStr(
                                            (
                                                (!!IsEditing * UI_BOX_KIND_DRAW_BORDER) |
                                                UI_BOX_KIND_CLICKABLE |
                                                UI_BOX_KIND_DISABLE_FOCUS_OVERLAY
                                            ),
                                            "edit_box"_s8
                                        );
                                    }
                                }

                                if (Params->LineEditKey)
                                    Params->LineEditKey[0] = EditBox->Key;
                            }
                        }
                    }

                    if (UIHeadTextAlignment() == UI_TEXT_ALIGN_LEFT)
                        UISpacer(UI_EM(1.0f, 1.0f));
                } else {
                    EditBox = UIBuildBoxFromStr(0, "edit_box"_s8);

                    if (Params->LineEditKey)
                        Params->LineEditKey[0] = EditBox->Key;
                }
            }
        }
    }

    b32 EditStarted = FALSE;

    if (
        Params->Kind & APP_CELL_KIND_EMPTY_EDIT_BTN &&
        !IsFocusActive &&
        !IsFocusActiveDisabled &&
        BuildLHSNameDesc &&
        BuildLineEdit &&
        !ValueNameFancyStrings.TotalSize
    ) {
        UITag("."_s8) {
            UITag("weak"_s8) {
                UITag("implicit"_s8) {
                    UIParent(Box) {
                        UIPreferredWidth(UI_EM(2.0f, 1.0f)) {
                            UIColumn() {
                                UIPadding(UI_PERCENT(1, 0)) {
                                    UIPreferredHeight(UI_EM(2.0f, 1.0f)) {
                                        UICornerRadius(UIHeadFontSize() * 0.5f) {
                                            AppFont(APP_FONT_SLOT_ICONS) {
                                                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                    UIBox* EditStartBox = UIBuildBoxFromStrFmt(
                                                        (
                                                            UI_BOX_KIND_DRAW_TEXT |
                                                            UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                            UI_BOX_KIND_DRAW_BORDER |
                                                            UI_BOX_KIND_DRAW_BACKGROUND |
                                                            UI_BOX_KIND_DISABLE_FOCUS_OVERLAY |
                                                            UI_BOX_KIND_DISABLE_FOCUS_BORDER |
                                                            UI_BOX_KIND_CLICKABLE
                                                        ),
                                                        "%S##edit",
                                                        PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_PENCIL])
                                                    );
                                                    UISignal Sig = UISignalFromBox(EditStartBox);

                                                    if (UI_PRESSED(Sig))
                                                        EditStarted = TRUE;
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            UISpacer(UI_EM(1.0f, 1.0f));
                        }
                    }
                }
            }
        }
    }

    UIBox* ScrollableBox = EMPTY_UI_BOX_VALUE;

    if (EditBox != EMPTY_UI_BOX_VALUE) {
        UIParent(EditBox) {
            UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                ScrollableBox = UIBuildBoxFromStrFmt(
                    IsFocusActive * UI_BOX_KIND_ALLOW_OVERFLOW_X,
                    "scroll_box_%p",
                    Params->EditBuffer
                );
            }
        }
    }

    if (Params->Kind & APP_CELL_KIND_REVERT_BTN && !IsFocusActive && !IsFocusActiveDisabled) {
        UIParent(EditBox) {
            UIPreferredWidth(UI_EM(2.0f, 1.0f)) {
                UITag("."_s8) {
                    UITag("weak"_s8) {
                        UITag("implicit"_s8) {
                            UIColumn() {
                                UIPadding(UI_PERCENT(1, 0)) {
                                    UIPreferredHeight(UI_EM(2.0f, 1.0f)) {
                                        UICornerRadius(UIHeadFontSize() * 0.5f) {
                                            AppFont(APP_FONT_SLOT_ICONS) {
                                                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                    UIBox* RevertBox = UIBuildBoxFromStrFmt(
                                                        (
                                                            UI_BOX_KIND_DRAW_TEXT |
                                                            UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                            UI_BOX_KIND_DRAW_BORDER |
                                                            UI_BOX_KIND_DRAW_BACKGROUND |
                                                            UI_BOX_KIND_DISABLE_FOCUS_OVERLAY |
                                                            UI_BOX_KIND_DISABLE_FOCUS_BORDER |
                                                            UI_BOX_KIND_CLICKABLE
                                                        ),
                                                        "%S##revert",
                                                        PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_UNDO])
                                                    );
                                                    UISignal Sig = UISignalFromBox(RevertBox);

                                                    if (UI_HOVERING(Sig)) {
                                                        UITooltip() {
                                                            AppFont(APP_FONT_SLOT_MAIN) {
                                                                GetTLS()->UI_STATE->TooltipAnchorKey = RevertBox->Key;
                                                                UILabel("Revert to Default"_s8);
                                                            }
                                                        }
                                                    }

                                                    if (UI_PRESSED(Sig) && Params->Revert)
                                                        *Params->Revert = TRUE;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (BuildToggleSwitch || BuildSlider)
                    UISpacer(UI_EM(1.0f, 1.0f));
            }
        }
    }

    if (BuildToggleSwitch) {
        UIParent(Box) {
            b32 IsToggled = !!*Params->Toggled;
            f32 ToggleT = UIAnimate(
                UIKeyFromStr(Key, "toggled"_s8),
                (f32) IsToggled,
                __Params.Initial = (f32) IsToggled,
                __Params.Rate = APP_STATE->MenuAnimationRate
            );
            f32 HeightPX = CeilF32(UIHeadFontSize() * 1.75f);
            f32 PaddingPX = CeilF32((UIHeadPXHeight() - HeightPX) / 2.0f);

            UIPreferredWidth(UI_SUM_OF_CHILDREN(1.0f)) {
                UIHeightFill() {
                    UIColumn() {
                        UIPadding(UI_PX(PaddingPX, 1.0f)) {
                            UIRow() {
                                if (UIHeadTextAlignment() == UI_TEXT_ALIGN_CENTRE) {
                                    UISpacer(UI_EM(1.0f, 0.0f));
                                }

                                UIPreferredWidth(UI_EM(3.5f, 1.0f)) {
                                    UIPreferredHeight(UI_PX(HeightPX, 1.0f)) {
                                        UICornerRadius(FloorF32(HeightPX / 2.0f - 1.0f)) {
                                            UITag(IsToggled ? "good_pop"_s8 : ""_s8) {
                                                UIGroupKey(UIKeyFromStr({}, "toggle_switch_group_key"_s8)) {
                                                    UIBox* SwitchBox = UIBuildBoxFromStr(
                                                        (
                                                            UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                            UI_BOX_KIND_DRAW_BORDER |
                                                            UI_BOX_KIND_DRAW_BACKGROUND |
                                                            UI_BOX_KIND_CLICKABLE
                                                        ),
                                                        "toggle_switch"_s8
                                                    );

                                                    UIParent(SwitchBox) {
                                                        AppFont(APP_FONT_SLOT_ICONS) {
                                                            UIPreferredWidth(UI_PERCENT(ToggleT, 0)) {
                                                                UITransparency(1.0f - ToggleT) {
                                                                    UIBuildBoxFromStrFmt(
                                                                        (
                                                                            UI_BOX_KIND_DISABLE_TEXT_TRUNC |
                                                                            (ToggleT > 0.001f ? UI_BOX_KIND_DRAW_TEXT : 0)
                                                                        ),
                                                                        "%S",
                                                                        PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_CHECK])
                                                                    );
                                                                }
                                                            }
                                                        }

                                                        UIBackgroundColour(UIColourFromName("text"_s8)) {
                                                            UIPreferredWidth(UI_PX(HeightPX, 1.0f)) {
                                                                f32 ExtraTogglerPaddingPX = FloorF32(UIHeadFontSize() * 0.35f);
                                                                f32 TogglerSizePX = CeilF32(HeightPX - ExtraTogglerPaddingPX * 2.0f) - 1.0f;

                                                                UIColumn() {
                                                                    UIPadding(UI_PX(ExtraTogglerPaddingPX, 1.0f)) {
                                                                        UIRow() {
                                                                            UIPadding(UI_PX(ExtraTogglerPaddingPX, 1.0f)) {
                                                                                UIPreferredWidth(UI_PX(TogglerSizePX, 1.0f)) {
                                                                                    UIPreferredHeight(UI_PX(TogglerSizePX, 1.0f)) {
                                                                                        UICornerRadius(FloorF32(TogglerSizePX / 2.0f - 1.0f)) {
                                                                                            UIBuildBoxFromKey(
                                                                                                UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_DROP_SHADOW,
                                                                                                EMPTY_UI_KEY_VALUE
                                                                                            );
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }

                                                        UISpacer(UI_PERCENT(1.0f - ToggleT, 0));
                                                    }

                                                    UISignal SwitchSig = UISignalFromBox(SwitchBox);

                                                    if (UI_PRESSED(SwitchSig)) {
                                                        if (UI_DRAGGING(SwitchSig)) {
                                                            UIStoreDragStruct(&SwitchBox->Key);
                                                        }

                                                        *Params->Toggled ^= TRUE;
                                                    }

                                                    if (UI_DRAGGING(SwitchSig)) {
                                                        Str8 AllKeysData = UIGetDragData(sizeof(UIKey));
                                                        UIKey* Keys = (UIKey*) AllKeysData.Str;
                                                        u64 KeysCount = AllKeysData.Size / sizeof(UIKey);
                                                        b32 KeyIsTouched = FALSE;

                                                        for (u64 Index = 0; Index < KeysCount; ++Index) {
                                                            if (Keys[Index] == SwitchBox->Key) {
                                                                KeyIsTouched = TRUE;
                                                                break;
                                                            }
                                                        }

                                                        if (!KeyIsTouched) {
                                                            *Params->Toggled ^= TRUE;

                                                            UIKey* NewKeys = ArenaPushArrayZero(Scratch.MemPool, UIKey, KeysCount + 1);

                                                            MemCpy(NewKeys, Keys, sizeof(UIKey) * KeysCount);
                                                            NewKeys[KeysCount] = SwitchBox->Key;
                                                            UIStoreDragData(Str((u8*) NewKeys, sizeof(UIKey) * (KeysCount + 1)));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                if (UIHeadTextAlignment() == UI_TEXT_ALIGN_CENTRE)
                                    UISpacer(UI_EM(1.0f, 0.0f));
                            }
                        }
                    }
                }
            }

            if (UIHeadTextAlignment() == UI_TEXT_ALIGN_LEFT)
                UISpacer(UI_EM(1.0f, 1.0f));
        }
    }

    if (BuildSlider) {
        UIParent(Box) {
            f32 HeightPX = CeilF32(UIHeadFontSize() * 1.75f);
            f32 PaddingPX = CeilF32((UIHeadPXHeight() - HeightPX) / 2.0f);

            UIPreferredWidth(UI_SUM_OF_CHILDREN(1.0f)) {
                UIHeightFill() {
                    UIColumn() {
                        UIPadding(UI_PX(PaddingPX, 1.0f)) {
                            UIRow() {
                                UIPreferredWidth(UI_PERCENT(0.5f - 0.2f * (!!BuildLHSNameDesc), 0.0f)) {
                                    UIPreferredHeight(UI_PX(HeightPX, 1.0f)) {
                                        UICornerRadius(FloorF32(HeightPX / 2.0f - 1.0f)) {
                                            f32 ExtraTogglerPaddingPX = FloorF32(UIHeadFontSize() * 0.35f);
                                            f32 TogglerSizePX = CeilF32(HeightPX - ExtraTogglerPaddingPX * 2.0f) - 1.0f;

                                            UISetNextHoverCursor(CURSOR_KIND_LEFT_RIGHT);

                                            UIBox* SliderBox = UIBuildBoxFromStr(
                                                (
                                                    UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                    UI_BOX_KIND_DRAW_BORDER |
                                                    UI_BOX_KIND_DRAW_BACKGROUND |
                                                    UI_BOX_KIND_CLICKABLE
                                                ),
                                                "slider"_s8
                                            );

                                            UIParent(SliderBox) {
                                                UITag("pop"_s8) {
                                                    UISignal Sig = UISignalFromBox(SliderBox);

                                                    if (UI_DRAGGING(Sig)) {
                                                        if (UI_PRESSED(Sig)) {
                                                            UIStoreDragStruct(Params->SliderValue);
                                                        }

                                                        f32 DraggableRegionSizePX = Length(SliderBox->Rect).X - (ExtraTogglerPaddingPX * 2.0f + TogglerSizePX);
                                                        f32 InitialPercent = *UIGetDragStruct(f32);
                                                        f32 CurrentPercent = InitialPercent + (UIDragDelta().X / DraggableRegionSizePX);

                                                        *Params->SliderValue = CurrentPercent;
                                                    }

                                                    UIBox* FillBox = EMPTY_UI_BOX_VALUE;

                                                    UIPreferredWidth(UI_SUM_OF_CHILDREN(0)) {
                                                        UIMinWidth(TogglerSizePX + ExtraTogglerPaddingPX * 2.0f) {
                                                            FillBox = UIBuildBoxFromKey(
                                                                UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_BORDER,
                                                                {}
                                                            );
                                                        }
                                                    }

                                                    UIParent(FillBox) {
                                                        UISpacer(UI_PERCENT(CLAMP(0.0f, *Params->SliderValue, 1.0f), 0.0f));

                                                        UIBackgroundColour(UIColourFromName("text"_s8)) {
                                                            UIPreferredWidth(UI_PX(HeightPX, 1.0f)) {
                                                                UIColumn() {
                                                                    UIPadding(UI_PX(ExtraTogglerPaddingPX, 1.0f)) {
                                                                        UIRow() {
                                                                            UIPadding(UI_PX(ExtraTogglerPaddingPX, 1.0f)) {
                                                                                UIPreferredWidth(UI_PX(TogglerSizePX, 1.0f)) {
                                                                                    UIPreferredHeight(UI_PX(TogglerSizePX, 1.0f)) {
                                                                                        UICornerRadius(FloorF32(TogglerSizePX / 2.0f - 1.0f)) {
                                                                                            UIBuildBoxFromKey(
                                                                                                UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_DROP_SHADOW,
                                                                                                {}
                                                                                            );
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }

                                                    UISpacer(UI_PERCENT(1.0f - CLAMP(0.0f, *Params->SliderValue, 1.0f), 0.0f));
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            UISpacer(UI_EM(1.0f, 1.0f));
        }
    }

    if (BuildBindings) {
        UIParent(Box) {
            AppFont(APP_FONT_SLOT_MAIN) {
                UIPermissionFlags(UI_PERMISSION_KIND_CLICKS) {
                    UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                        UIColumn() {
                            UIPadding(UI_PX(UIHeadPXHeight() * 0.2f, 1.0f)) {
                                UIHeightFill() {
                                    UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                        UIRow() {
                                            UIPadding(UI_EM(1.0f, 1.0f)) {
                                                AppCommandBindingButtons(
                                                    Params->BindingsName, 
                                                    Params->SearchNeedle, 
                                                    U64_MAX,
                                                    APP_COMMAND_BINDING_BTN_FLAG_ADD_NEW
                                                );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (BuildNote) {
        UIParent(Box) {
            UIPreferredWidth(Params->NoteWidth) {
                UIBox* NoteBox = UIBuildBoxFromKey(UI_BOX_KIND_DRAW_TEXT, {});

                UIBoxEquipDisplayFancyStrs(NoteBox, &NoteFancyStrings);
            }
        }
    }

    b32 Commit = FALSE;

    if (!IsFocusActive && IsFocusHot) {
        for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
            if (Evt->Flags & UI_EVENT_FLAG_COPY) {
                SetClipboardText(Params->PreEditValue);
            }

            if (Evt->Flags & UI_EVENT_FLAG_DELETE) {
                Commit = TRUE;
                *Params->EditStringSize = 0;
            }
        }
    }

    UISignal Sig = UISignalFromBox(Box);

    if (Commit)
        Sig.Kind |= UI_SIGNAL_KIND_COMMIT;

    b32 FocusStarted = FALSE;

    if (!IsFocusActive) {
        b32 StartEditingViaSig = (UI_DOUBLE_CLICKED(Sig) || Sig.Kind & UI_SIGNAL_KIND_KEYBOARD_PRESSED);
        b32 StartEditingViaTyping = FALSE;

        if (IsFocusHot) {
            for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
                if (Evt->String.Size || Evt->Flags & UI_EVENT_FLAG_PASTE) {
                    StartEditingViaTyping = TRUE;
                    break;
                }
            }
        }

        if (IsFocusHot && UISlotPress(UI_EVENT_ACTION_SLOT_EDIT)) {
            StartEditingViaTyping = TRUE;
        }

        if (StartEditingViaSig || StartEditingViaTyping) {
            Str8 EditString = Params->PreEditValue;

            EditString.Size = MIN(Params->EditBufferSize, Params->PreEditValue.Size);
            MemCpy(Params->EditBuffer, EditString.Str, EditString.Size);
            *Params->EditStringSize = EditString.Size;
            UISetAutoFocusActiveKey(Key);

            if (!(Params->Kind & APP_CELL_KIND_BTN))
                UIKillAction();

            *Params->Cursor = TxtPt(1, EditString.Size + 1);
            *Params->Mark = TxtPt(1, 1);
            FocusStarted = TRUE;
        }
    } else if (IsFocusActive && Sig.Kind & UI_SIGNAL_KIND_KEYBOARD_PRESSED) {
        UISetAutoFocusActiveKey({});
        Sig.Kind |= UI_SIGNAL_KIND_COMMIT;
    }

    b32 ChangesMade = FALSE;

    if (!(Params->Kind & APP_CELL_KIND_DISABLE_EDIT) && (IsFocusActive || FocusStarted)) {
        TempArena Scratch = GetScratch(NULL, 0);

        APP_STATE->TextEditMode = TRUE;

        for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
            Str8 EditString = Str(Params->EditBuffer, *Params->EditStringSize);
            b32 IsAutocompletionCompletion = (
                AutoCompleteHintString.Size &&
                Evt->Kind == UI_EVENT_KIND_PRESS &&
                Evt->Slot == UI_EVENT_ACTION_SLOT_ACCEPT
            );

            if (
                !IsAutocompletionCompletion &&
                (
                    (
                        Evt->Kind != UI_EVENT_KIND_EDIT &&
                        Evt->Kind != UI_EVENT_KIND_NAVIGATE &&
                        Evt->Kind != UI_EVENT_KIND_TEXT
                    ) ||
                    Evt->DeltaI32.Y != 0
                )
            ) {
                continue;
            }

            UITextOp Op = UISingleLineTextOpFromEvent(
                Scratch.MemPool, 
                Evt, 
                EditString, 
                *Params->Cursor, 
                *Params->Mark
            );

            if (
                !(Evt->Flags & UI_EVENT_FLAG_DELETE) &&
                AutoCompleteHintString.Size
            ) {
                ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                AppWindowState* WS = AppWindowStateFromConfig(Window);
                AppAutoCompleteCursorInfo* AutoCompleteCursorInfo = &WS->AutoCompleteCursorInfo;
                Str8 NewString = UIPushStrReplaceRange(
                    Scratch.MemPool,
                    EditString,
                    Rng(
                        (i64) (AutoCompleteCursorInfo->ReplacedRange.Min + 1),
                        (i64) (AutoCompleteCursorInfo->ReplacedRange.Max + 1)
                    ),
                    AutoCompleteHintString
                );

                NewString.Size = MIN(NewString.Size, Params->EditBufferSize);
                MemCpy(
                    Params->EditBuffer,
                    NewString.Str,
                    NewString.Size
                );
                Params->EditStringSize[0] = NewString.Size;
                Params->Cursor[0] = TxtPt(
                    1, 
                    1 + AutoCompleteCursorInfo->ReplacedRange.Min + AutoCompleteHintString.Size
                );
                Params->Mark[0] = Params->Cursor[0];
                EditString = Str(
                    Params->EditBuffer,
                    Params->EditStringSize[0]
                );
                Op = UISingleLineTextOpFromEvent(
                    Scratch.MemPool,
                    Evt,
                    EditString,
                    Params->Cursor[0],
                    Params->Mark[0]
                );
                MemZero(&AutoCompleteHintString, sizeof(AutoCompleteHintString));
            }

            if (Op.Range.Min != Op.Range.Max || Op.Replace.Size) {
                Str8 NewString = UIPushStrReplaceRange(
                    Scratch.MemPool,
                    EditString,
                    Rng(
                        (i64) Op.Range.Min.Column,
                        (i64) Op.Range.Max.Column
                    ),
                    Op.Replace
                );

                NewString.Size = MIN(Params->EditBufferSize, NewString.Size);
                MemCpy(
                    Params->EditBuffer,
                    NewString.Str,
                    NewString.Size
                );
                Params->EditStringSize[0] = NewString.Size;
            }

            if (Op.Kind & UI_TEXT_OP_KIND_COPY)
                SetClipboardText(Op.Copy);

            *Params->Cursor = Op.Cursor;
            *Params->Mark = Op.Mark;

            if (!IsAutocompletionCompletion)
                UIEatEvent(Evt);

            ChangesMade = TRUE;
        }

        ReleaseScratch(Scratch);
    }

    if (EditStarted)
        Sig.Kind |= UI_SIGNAL_KIND_DOUBLE_CLICKED;

    FancyStrList FancyStrings = {};

    if (!IsFocusActive && !IsFocusActiveDisabled && ValueNameFancyStrings.TotalSize) {
        FancyStrings = ValueNameFancyStrings;
    } else if (!IsFocusActive && !IsFocusActiveDisabled) {
        Str8 DisplayString = Params->PreEditValue;

        if (!Params->PreEditValue.Size)
            DisplayString = UIDisplayPartFromKeyStr(String);

        UITag("weak"_s8) {
            FancyStrParams FSParams = {
                UIHeadFont(),
                UIHeadTextRasterKind(),
                UIColourFromName("text"_s8),
                UIHeadFontSize()
            };

            ListPush(Scratch.MemPool, &FancyStrings, &FSParams, NULL, DisplayString);
        }
    } else if (IsFocusActive || IsFocusActiveDisabled) {
        Str8 EditString = Str(Params->EditBuffer, *Params->EditStringSize);
        FancyStrList EditStringFancyStrings = {};
        FancyStrParams FSParams = {
            UIHeadFont(),
            UIHeadTextRasterKind(),
            UIColourFromName("text"_s8),
            UIHeadFontSize()
        };

        ListPush(Scratch.MemPool, &EditStringFancyStrings, &FSParams, NULL, EditString);
        FancyStrings = EditStringFancyStrings;

        if (AutoCompleteHintString.Size) {
            ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
            AppWindowState* WS = AppWindowStateFromConfig(Window);
            AppAutoCompleteCursorInfo* CursorInfo = &WS->AutoCompleteCursorInfo;
            Str8 AppendString = StrSkip(
                AutoCompleteHintString,
                Params->Cursor->Column - 1 - CursorInfo->ReplacedRange.Min
            );
            u64 Offset = 0;
            u64 CursorOffset = Params->Cursor->Column - 1;
            FancyStrNode* PrevNode = NULL;

            for (
                FancyStrNode* Node = EditStringFancyStrings.Head;
                Node;
                Node = Node->Next
            ) {
                if (
                    Offset <= CursorOffset && 
                    CursorOffset <= Offset + Node->V.String.Size
                ) {
                    PrevNode = Node;
                    break;
                }

                Offset += Node->V.String.Size;
            }

            {
                FancyStrNode* AutoCompleteFancyStrNode = ArenaPushArrayZero(
                    Scratch.MemPool,
                    FancyStrNode,
                    1
                );
                FancyStr* FString = &AutoCompleteFancyStrNode->V;

                FString->String = AppendString;
                FString->Params.Font = UIHeadFont();
                FString->Params.RasterKind = UIHeadTextRasterKind();
                FString->Params.Colour = UIColourFromName("text"_s8);
                FString->Params.Colour.W *= 0.5f;
                FString->Params.Size = UIHeadFontSize();
                AutoCompleteFancyStrNode->Next = PrevNode ? PrevNode->Next : NULL;

                if (PrevNode)
                    PrevNode->Next = AutoCompleteFancyStrNode;

                if (!PrevNode) {
                    EditStringFancyStrings.Head = AutoCompleteFancyStrNode;
                    EditStringFancyStrings.Tail = EditStringFancyStrings.Head;
                }

                if (PrevNode && !PrevNode->Next)
                    EditStringFancyStrings.Tail = AutoCompleteFancyStrNode;

                ++EditStringFancyStrings.NodeCount;
                EditStringFancyStrings.TotalSize += AutoCompleteHintString.Size;

                if (
                    PrevNode &&
                    CursorOffset - Offset < PrevNode->V.String.Size
                ) {
                    Str8 FullString = PrevNode->V.String;
                    u64 ChopAmount = FullString.Size - (CursorOffset - Offset);

                    PrevNode->V.String = StrChop(FullString, ChopAmount);
                    EditStringFancyStrings.TotalSize -= ChopAmount;

                    if (ChopAmount) {
                        Str8 PostCursor = StrSkip(
                            FullString, 
                            CursorOffset - Offset
                        );
                        FancyStrNode* PostFancyStringNode = ArenaPushArrayZero(
                            Scratch.MemPool,
                            FancyStrNode,
                            1
                        );
                        FancyStr* PostFString = &PostFancyStringNode->V;

                        MemCpy(
                            PostFString,
                            &PrevNode->V,
                            sizeof(PrevNode->V)
                        );
                        PostFString->String = PostCursor;

                        if (!AutoCompleteFancyStrNode->Next)
                            EditStringFancyStrings.Tail = PostFancyStringNode;

                        PostFancyStringNode->Next = AutoCompleteFancyStrNode->Next;
                        AutoCompleteFancyStrNode->Next = PostFancyStringNode;
                        ++EditStringFancyStrings.NodeCount;
                        EditStringFancyStrings.TotalSize += PostCursor.Size;
                    }
                }
            }
        }

        FancyStrings = EditStringFancyStrings;
    }

    TextPoint MousePt = {};
    f32 CursorOffset = 0.0f;

    if (ScrollableBox != EMPTY_UI_BOX_VALUE) {
        UIParent(ScrollableBox) {
            FMRangeList FuzzyMatches = {};

            if (Params->SearchNeedle.Size) {
                FuzzyMatches = FuzzyFindFStrs(Scratch.MemPool, &FancyStrings, Params->SearchNeedle);
            }

            if (
                UIHeadTextAlignment() == UI_TEXT_ALIGN_LEFT && 
                !(
                    Params->Kind & (
                        APP_CELL_KIND_EXPANDER_SPACE | 
                        APP_CELL_KIND_EXPANDER_PLACEHOLDER | 
                        APP_CELL_KIND_EXPANDER
                    )
                )
            ) {
                UISpacer(UI_EM(0.5f, 1.0f));
            }

            if (IsFocusActive) {
                UISetNextFlags(UI_BOX_KIND_DISABLE_TEXT_TRUNC);
            }

            UISetNextPreferredWidth(UI_TEXT_DIM(UIHeadFontSize() * 0.5f, 0.0f));
            
            UIBox* TextBox = UIBuildBoxFromStr(UI_BOX_KIND_DRAW_TEXT, "###text_box"_s8);

            UIBoxEquipDisplayFancyStrs(TextBox, &FancyStrings);
            UIBoxEquipFuzzyMatchRanges(TextBox, &FuzzyMatches);

            if(IsFocusActive || IsFocusActiveDisabled) {
                Str8 EditString = Str(Params->EditBuffer, *Params->EditStringSize);
                UILineEditDrawData* DrawData = ArenaPushArrayZero(
                    UIBuildMemPool(), 
                    UILineEditDrawData, 
                    1
                );

                DrawData->EditedString = ArenaPushStrCpy(UIBuildMemPool(), EditString);
                DrawData->Cursor = *Params->Cursor;
                DrawData->Mark = *Params->Mark;
                DrawData->Trail = DoCursorTrail && !UI_DRAGGING(Sig);
                UIBoxEquipCustomDraw(TextBox, UILineEditDraw, DrawData);

                v2f32 Text2Mouse = UIMouse() - UIBoxTextPosition(TextBox);
                FontTag Font = UIHeadFont();
                f32 FontSize = UIHeadFontSize();

                if (Params->Kind & APP_CELL_KIND_CODE_CONTENTS)
                    Font = AppFontFromSlot(APP_FONT_SLOT_CODE);

                u64 MousePtOffset = FontCharPositionFromTagSizeString(
                    Font,
                    FontSize,
                    0,
                    UIHeadTabSize(),
                    EditString,
                    Text2Mouse.X
                );

                MousePt = TxtPt(1, 1 + MousePtOffset);
                CursorOffset = FontDimensionsFromTagSizeString(
                    UIHeadFont(),
                    UIHeadFontSize(),
                    0,
                    UIHeadTabSize(),
                    StrPrefix(EditString, Params->Cursor->Column - 1)
                ).X;
            }
        }
    }

    if (IsFocusActive && UI_DRAGGING(Sig)) {
        if (UI_PRESSED(Sig)) {
            *Params->Mark = MousePt;
        }

        *Params->Cursor = MousePt;
    }

    if (!IsFocusActive && IsFocusActiveDisabled && UI_PRESSED(Sig)) {
        *Params->Cursor = MousePt;
        *Params->Mark = MousePt;
    }

    if (ScrollableBox != EMPTY_UI_BOX_VALUE) {
        f32 VisibleDimPX = Length(Box->Rect).X - ExpanderSizePX - UIHeadFontSize() * Params->Depth;

        if (VisibleDimPX > 0.0f) {
            r1f32 CursorRangePX = Rng(
                CursorOffset - UIHeadFontSize() * 2.0f,
                CursorOffset + UIHeadFontSize() * 1.0f
            );
            r1f32 VisibleRangePX = Rng(
                ScrollableBox->ViewOffsetTarget.X,
                ScrollableBox->ViewOffsetTarget.X + VisibleDimPX
            );

            CursorRangePX.Min = CLAMP_BOT(0.0f, CursorRangePX.Min);
            CursorRangePX.Max = CLAMP_BOT(0.0f, CursorRangePX.Max);

            f32 MinDelta = CursorRangePX.Min - VisibleRangePX.Min;
            f32 MaxDelta = CursorRangePX.Max - VisibleRangePX.Max;

            MinDelta = MIN(MinDelta, 0.0f);
            MaxDelta = MAX(MaxDelta, 0.0f);
            ScrollableBox->ViewOffsetTarget.X += MinDelta;
            ScrollableBox->ViewOffsetTarget.X += MaxDelta;
        }

        if (!IsFocusActive && !IsFocusActiveDisabled) {
            ScrollableBox->ViewOffsetTarget.X = 0.0f;
            ScrollableBox->ViewOffset.X = 0.0f;
        }
    }

    if (IsAutoFocusHot)
        UIPopFocusHot();

    if (IsAutoFocusActive)
        UIPopFocusActive();

    ReleaseScratch(Scratch);

    return Sig;
}

UISignal 
AppCell(AppCellParams* Params, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Sig = AppCell(Params, String);

    ReleaseScratch(Scratch);

    return Sig;
}

void
AppStoreViewLoadingInfo(
    b32 IsLoading,
    u64 ProgressU64,
    u64 ProgressTargetU64
) {
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* VS = AppViewStateFromConfig(View);
    b32 LoadingStateIsNew = (
        IsLoading &&
        VS->LoadingTTarget != (f32) !!IsLoading
    );

    VS->LoadingTTarget = (f32) !!IsLoading;
    VS->LoadingProgressV = ProgressU64;
    VS->LoadingProgressVTarget = ProgressTargetU64;

    if (
        LoadingStateIsNew || 
        VS->LastFrameIndexBuilt + 1 < APP_STATE->FrameIndex
    ) {
        VS->LoadingT = VS->LoadingTTarget;
    }
}

void 
AppStoreViewScrollPosition(UIScrollPoint2D Position)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* VS = AppViewStateFromConfig(View);

    VS->ScrollPosition = Position;
}

void 
AppStoreViewParameter(Str8 Key, Str8 Value)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    ConfigNode* Child = ConfigNodeChildFromStrOrAlloc(
        APP_STATE->Config,
        View,
        Key
    );

    ConfigNodeNewReplace(APP_STATE->Config, Child, Value);
}

void 
AppStoreViewParameter(Str8 Key, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(
        Scratch.MemPool,
        Fmt,
        Args
    );

    AppStoreViewParameter(Key, String);
    va_end(Args);
    ReleaseScratch(Scratch);
}

void 
ListPush(Arena* MemPool, AppCommandList* List, Str8 Name, AppRegisters* Regs) 
{
    AppCommandNode* Node = ArenaPushArrayZero(MemPool, AppCommandNode, 1);

    Node->Command.Name = ArenaPushStrCpy(MemPool, Name);
    Node->Command.Regs = CopyRegisters(MemPool, Regs);
    DLL_PUSH_BACK(List->Head, List->Tail, Node);
    ++List->Count;
}

void
AppSetHoverEval(v2f32 Position, Str8 String)
{
    ConfigNode* WindowCfg = ConfigNodeFromID(Registers()->Window);
    AppWindowState* WS = AppWindowStateFromConfig(WindowCfg);

    if (
        WS->HoverEvalLastTimeUSecs < APP_STATE->TimeUSecs &&
        UIActiveKey(UI_MOUSE_BTN_KIND_LEFT) == EMPTY_UI_KEY_VALUE &&
        UIActiveKey(UI_MOUSE_BTN_KIND_MIDDLE) == EMPTY_UI_KEY_VALUE &&
        UIActiveKey(UI_MOUSE_BTN_KIND_RIGHT) == EMPTY_UI_KEY_VALUE
    ) {
        b32 IsNewString = !StrMatch(WS->HoverEvalString, String, 0);

        if (IsNewString) {
            WS->HoverEvalFirstTimeUSecs = APP_STATE->TimeUSecs;
            WS->HoverEvalLastTimeUSecs = APP_STATE->TimeUSecs;
            ArenaClear(WS->HoverEvalMemPool);
            WS->HoverEvalString = ArenaPushStrCpy(WS->HoverEvalMemPool, String);
            WS->HoverEvalFocused = FALSE;
        }

        WS->HoverEvalSpawnPosition = Position;
        WS->HoverEvalLastTimeUSecs = APP_STATE->TimeUSecs;
    }
}

void 
UILoadingOverlay(r2f32 Rect, f32 LoadingT, u64 ProgressV, u64 ProgressVTarget)
{
    if (LoadingT >= 0.001f) {
        UIFocus(UI_FOCUS_KIND_OFF) {
            f32 EdgePadding = 30.0f;
            f32 Width = UIHeadFontSize() * 10.0f;
            f32 Height = UIHeadFontSize() * 1.0f;
            f32 MinThickness = UIHeadFontSize() / 2.0f;
            f32 Trail = UIHeadFontSize() * 4.0f;
            f32 T = Pow(Sin((f32) APP_STATE->TimeSecs / 1.8f), 2.0f);
            f64 V = 1.0f - AbsF32(0.5f - T);

            UICornerRadius(Height / 3.0f) {
                UITransparency(1.0f - LoadingT) {
                    r2f32 IndicatorRgnRect = Rng(
                        (Rect.X0 + Rect.X1) / 2.0f - Width / 2.0f - Rect.X0,
                        (Rect.Y0 + Rect.Y1) / 2.0f - Height / 2.0f - Rect.Y0,
                        (Rect.X0 + Rect.X1) / 2.0f + Width / 2.0f - Rect.X0,
                        (Rect.Y0 + Rect.Y1) / 2.0f + Height / 2.0f - Rect.Y0
                    );
                    r2f32 IndicatorRect = Rng(
                        (f32) (IndicatorRgnRect.X0 + Width * T - MinThickness / 2.0f - Trail * V),
                        (f32) (IndicatorRgnRect.Y0),
                        (f32) (IndicatorRgnRect.X0 + Width * T + MinThickness / 2.0f + Trail * V),
                        (f32) (IndicatorRgnRect.Y1)
                    );

                    IndicatorRect.X0 = CLAMP(IndicatorRgnRect.X0, IndicatorRect.X0, IndicatorRgnRect.X1);
                    IndicatorRect.X1 = CLAMP(IndicatorRgnRect.X0, IndicatorRect.X1, IndicatorRgnRect.X1);
                    IndicatorRect = Widen(IndicatorRect, -1.0f);

                    if (ProgressV != ProgressVTarget) {
                        UITag("drop_site"_s8) {
                            f64 PercentDone64 = ((f64) ProgressV / (f64) ProgressVTarget);
                            f32 PercentDone = (f32) PercentDone64;
                            r2f32 PercentRect = Rng(
                                IndicatorRgnRect.X0,
                                IndicatorRgnRect.Y0,
                                IndicatorRgnRect.X0 + (IndicatorRgnRect.X1 - IndicatorRgnRect.X0) * PercentDone,
                                IndicatorRgnRect.Y1
                            );

                            UIRect(PercentRect) {
                                UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_FLOATING, {});
                            }
                        }
                    }

                    UITag("pop"_s8) {
                        UIRect(IndicatorRect) {
                            UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_FLOATING, {});
                        }
                    }

                    UIRect(IndicatorRgnRect) {
                        UIBox* Box = UIBuildBoxFromStr(
                            (
                                UI_BOX_KIND_DRAW_BACKGROUND |
                                UI_BOX_KIND_DRAW_BORDER |
                                UI_BOX_KIND_FLOATING |
                                UI_BOX_KIND_CLICKABLE
                            ),
                            "bg_system_status"_s8
                        );
                        UISignal Sig = UISignalFromBox(Box);
                    }
                }
            }

            UIWidthFill() {
                UIHeightFill() {
                    UITransparency(1.0f - LoadingT) {
                        UIBlurSize(10.0f * LoadingT) {
                            UISetNextBlurSize(10.0f * LoadingT);
                            UIBuildBoxFromKey(
                                (
                                    UI_BOX_KIND_DRAW_BACKGROUND | 
                                    UI_BOX_KIND_DRAW_BACKGROUND_BLUR | 
                                    UI_BOX_KIND_FLOATING
                                ),
                                {}
                            );
                        }
                    }
                }
            }
        }
    }
}

void 
CopyRegisters(Arena* MemPool, AppRegisters* Dst, AppRegisters* Src) 
{
    MemCpy(Dst, Src, sizeof(AppRegisters));
    Dst->ConfigList = ListCpy(MemPool, &Src->ConfigList);
    Dst->FilePath = ArenaPushStrCpy(MemPool, Src->FilePath);
    Dst->Expression = ArenaPushStrCpy(MemPool, Src->Expression);
    Dst->String = ArenaPushStrCpy(MemPool, Src->String);
    Dst->CommandName = ArenaPushStrCpy(MemPool, Src->CommandName);

    if (!Dst->ConfigList.Count && Dst->Config)
        ListPush(MemPool, &Dst->ConfigList, Dst->Config);
}

AppRegisters* 
CopyRegisters(Arena* MemPool, AppRegisters* Regs) 
{
    AppRegisters* Result = ArenaPushArrayZero(MemPool, AppRegisters, 1);
    CopyRegisters(MemPool, Result, Regs);

    return Result;
}

b32 
AppDragIsActive(void)
{
    return (
        APP_STATE->DragDropState == APP_DRAG_DROP_STATE_DRAGGING ||
        APP_STATE->DragDropState == APP_DRAG_DROP_STATE_DROPPING
    );
}

void 
AppDragBegin(AppRegistersSlot Slot)
{
    if (!AppDragIsActive()) {
        ArenaClear(APP_STATE->DragDropMemPool);
        APP_STATE->DragDropRegisters = CopyRegisters(APP_STATE->DragDropMemPool, Registers());
        APP_STATE->DragDropRegistersSlot = Slot;
        APP_STATE->DragDropState = APP_DRAG_DROP_STATE_DRAGGING;
    }
}

b32 
AppDragDrop(void)
{
    b32 Result = FALSE;

    if (APP_STATE->DragDropState == APP_DRAG_DROP_STATE_DROPPING) {
        Result = TRUE;
        APP_STATE->DragDropState = APP_DRAG_DROP_STATE_NULL;
    }

    return Result;
}

void 
AppDragKill(void)
{
    APP_STATE->DragDropState = APP_DRAG_DROP_STATE_NULL;
}

void 
AppSetHoverRegisters(AppRegistersSlot Slot)
{
    APP_STATE->NextHoverRegisters = CopyRegisters(AppFrameMemPool(), Registers());
    APP_STATE->NextHoverRegistersSlot = Slot;
}

v4f32 
AppHSVAFromConfig(ConfigNode* Config)
{
    v4f32 Result = {};
    ConfigNode* HSVARoot = ConfigNodeChildFromStr(Config, "hsva"_s8);
    ConfigNode* H = HSVARoot->Head;
    ConfigNode* S = H->Next;
    ConfigNode* V = S->Next;
    ConfigNode* A = V->Next;

    Result.X = (f32) F64FromStr(H->String);
    Result.Y = (f32) F64FromStr(S->String);
    Result.Z = (f32) F64FromStr(V->String);
    Result.W = (f32) F64FromStr(A->String);

    return Result;
}

v4f32 
AppColourFromConfig(ConfigNode* Config)
{
    v4f32 HSVA = AppHSVAFromConfig(Config);
    v4f32 Result = LinearFromSRGBA(RGBAFromHSVA(HSVA));

    return Result;
}

b32 
AppIsDisabled(ConfigNode* Config)
{
    TempArena Scratch = GetScratch(NULL, 0);
    MDNode* ChildSchema = EMPTY_MD_NODE_VALUE;
    MDNodePtrList Schemas = ConfigSchemasFromName(
        Scratch.MemPool,
        APP_STATE->CfgSchemaTable,
        Config->String
    );

    for (
        MDNodePtrNode* N = Schemas.Head;
        N && ChildSchema == EMPTY_MD_NODE_VALUE;
        N = N->Next
    ) {
        ChildSchema = MDChildFromStr(N->V, "enabled"_s8, 0);
    }

    MDNode* DefaultTag = MDTagFromStr(ChildSchema, "default"_s8, 0);
    Str8 ValueString = ConfigNodeChildFromStr(Config, "enabled"_s8)->Head->String;

    if (!ValueString.Size)
        ValueString = DefaultTag->Head->String;

    b32 IsEnabled = !!U64FromStr(ValueString);
    b32 IsDisabled = !IsEnabled;

    if (!ValueString.Size)
        IsDisabled = FALSE;

    ReleaseScratch(Scratch);

    return IsDisabled;
}

AppLocation 
AppLocationFromConfig(ConfigNode* Config)
{
    AppLocation Result = {};
    ConfigNode* SrcLocation = ConfigNodeChildFromStr(
        Config, 
        "source_location"_s8
    );
    ConfigNode* AddrLocation = ConfigNodeChildFromStr(
        Config, 
        "address_location"_s8
    );

    if (SrcLocation != EMPTY_CFG_NODE_VALUE) {
        Str8TextPointPair LocationDescription = Str8TextPointPairFromStr(
            SrcLocation->Head->String
        );

        Result.FilePath = LocationDescription.String;
        Result.Point = LocationDescription.Point;
    } else if (AddrLocation != EMPTY_CFG_NODE_VALUE) {
        Result.Expression = AddrLocation->Head->String;
    }

    return Result;
}

Str8 
LabelFromConfig(ConfigNode* Config)
{
    ConfigNode* LabelRoot = ConfigNodeChildFromStr(Config, "label"_s8);
    Str8 Result = LabelRoot->Head->String;

    return Result;
}

Str8 
ExpressionFromConfig(ConfigNode* Config)
{
    ConfigNode* ExpressionRoot = ConfigNodeChildFromStr(Config, "expression"_s8);
    Str8 Result = ExpressionRoot->Head->String;

    return Result;
}

AppVocabInfo*
AppVocabInfoFromCodeName(Str8 CodeName)
{
    AppVocabInfo* Result = EMPTY_APP_VOCAB_INFO_VALUE;

    if (CodeName.Size) {
        u64 Hash = DHash(CodeName);
        u64 SlotIndex = Hash % APP_STATE->VocabInfoMap.SingleSlotsCount;

        for (
            AppVocabInfoMapNode* N = APP_STATE->VocabInfoMap.SingleSlots[SlotIndex].Head;
            N;
            N = N->SingleNext
        ) {
            if (StrMatch(N->V.CodeName, CodeName, 0)) {
                Result = &N->V;
                break;
            }
        }
    }

    return Result;
}

AppVocabInfo*
AppVocabInfoFromCodeNamePlural(Str8 CodeName)
{
    AppVocabInfo* Result = EMPTY_APP_VOCAB_INFO_VALUE;

    if (CodeName.Size) {
        u64 Hash = DHash(CodeName);
        u64 SlotIndex = Hash % APP_STATE->VocabInfoMap.PluralSlotsCount;

        for (
            AppVocabInfoMapNode* N = APP_STATE->VocabInfoMap.PluralSlots[SlotIndex].Head;
            N;
            N = N->SingleNext
        ) {
            if (StrMatch(N->V.CodeName, CodeName, 0)) {
                Result = &N->V;
                break;
            }
        }
    }

    return Result;
}

Str8 
PathFromConfig(ConfigNode* Config)
{
    ConfigNode* PathRoot = ConfigNodeChildFromStr(Config, "path"_s8);
    Str8 Result = PathRoot->Head->String;

    return Result;
}

ConfigNode*
AppConfigFromEvalSpace(EvalSpace Space)
{
    ConfigNode* Result = EMPTY_CFG_NODE_VALUE;

    if (Space.Kind == APP_EVAL_SPACE_KIND_META_CFG) {
        ConfigID ID = Space.U64s[0];

        Result = ConfigNodeFromID(ID);
    }

    return Result;
}

EvalSpace
AppEvalSpaceFromConfig(ConfigNode* Cfg)
{
    EvalSpace Result = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_CFG);

    Result.U64s[0] = Cfg->ID;

    return Result;
}

MDNodePtrList 
AppSchemasFromName(Str8 Name)
{
    MDNodePtrList Result = {};

    for (u64 Index = 0; Index < ARRAY_COUNT(APP_NAME_SCHEMA_INFO_TABLE); ++Index) {
        if (StrMatch(Name, APP_NAME_SCHEMA_INFO_TABLE[Index].Name, 0)) {
            Result = APP_STATE->Schemas[Index];
            break;
        }
    }

    return Result;
}

Str8 
AppDefaultSettingFromNames(Str8 SchemaName, Str8 SettingName)
{
    Str8 Result = {};
    TempArena Scratch = GetScratch(NULL, 0);
    MDNode* SettingSchema = EMPTY_MD_NODE_VALUE;
    MDNodePtrList Schemas = ConfigSchemasFromName(
        Scratch.MemPool,
        APP_STATE->CfgSchemaTable,
        SchemaName
    );

    for (
        MDNodePtrNode* N = Schemas.Head;
        N && SettingSchema == EMPTY_MD_NODE_VALUE;
        N = N->Next
    ) {
        SettingSchema = MDChildFromStr(N->V, SettingName, 0);
    }

    if (SettingSchema != EMPTY_MD_NODE_VALUE) {
        MDNode* DefaultTag = MDTagFromStr(SettingSchema, "default"_s8, 0);

        if (DefaultTag != EMPTY_MD_NODE_VALUE)
            Result = DefaultTag->Head->String;
    }

    ReleaseScratch(Scratch);

    return Result;
}

Str8 
AppSettingFromNameStr8(Str8 Name)
{
    Str8 Result = {};

    if (Name.Size) {
        ConfigNode* Cfg = EMPTY_CFG_NODE_VALUE;

        Cfg = ConfigNodeFromID(Registers()->View);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            for (ConfigNode* C = Cfg; C != EMPTY_CFG_NODE_VALUE; C = C->Parent) {
                ConfigNode* Setting = ConfigNodeChildFromStr(C, Name);

                if (Setting != EMPTY_CFG_NODE_VALUE) {
                    Result = Setting->Head->String;
                    goto END;
                }
            }
        }

        Cfg = ConfigNodeFromID(Registers()->Tab);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            for (ConfigNode* C = Cfg; C != EMPTY_CFG_NODE_VALUE; C = C->Parent) {
                ConfigNode* Setting = ConfigNodeChildFromStr(C, Name);

                if (Setting != EMPTY_CFG_NODE_VALUE) {
                    Result = Setting->Head->String;
                    goto END;
                }
            }
        }

        Cfg = ConfigNodeFromID(Registers()->Panel);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            for (ConfigNode* C = Cfg; C != EMPTY_CFG_NODE_VALUE; C = C->Parent) {
                ConfigNode* Setting = ConfigNodeChildFromStr(C, Name);

                if (Setting != EMPTY_CFG_NODE_VALUE) {
                    Result = Setting->Head->String;
                    goto END;
                }
            }
        }

        Cfg = ConfigNodeFromID(Registers()->Window);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            for (ConfigNode* C = Cfg; C != EMPTY_CFG_NODE_VALUE; C = C->Parent) {
                ConfigNode* Setting = ConfigNodeChildFromStr(C, Name);

                if (Setting != EMPTY_CFG_NODE_VALUE) {
                    Result = Setting->Head->String;
                    goto END;
                }
            }
        }

        {
            ConfigNode* Session = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);
            ConfigNode* Settings = ConfigNodeChildFromStr(Session, "settings"_s8);
            ConfigNode* Setting = ConfigNodeChildFromStr(Settings, Name);

            if (Setting != EMPTY_CFG_NODE_VALUE) {
                Result = Setting->Head->String;
                goto END;
            }
        }

        Cfg = ConfigNodeFromID(Registers()->View);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            Result = AppDefaultSettingFromNames(Cfg->String, Name);

            if (Result.Size) 
                goto END;
        }

        Cfg = ConfigNodeFromID(Registers()->Tab);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            Result = AppDefaultSettingFromNames(Cfg->String, Name);

            if (Result.Size)
                goto END;
        }

        Cfg = ConfigNodeFromID(Registers()->Panel);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            Result = AppDefaultSettingFromNames(Cfg->String, Name);

            if (Result.Size)
                goto END;
        }

        Cfg = ConfigNodeFromID(Registers()->Window);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            Result = AppDefaultSettingFromNames(Cfg->String, Name);

            if (Result.Size) 
                goto END;
        }

        Result = AppDefaultSettingFromNames("session"_s8, Name);
    }

END:
    return Result;
}

b32 
AppSettingFromNameB32(Str8 Name)
{
    b32 Result = FALSE;
    Str8 Value = AppSettingFromNameStr8(Name);

    if (Value.Size) {
        TempArena Scratch = GetScratch(NULL, 0);
        Str8 Expr = ArenaPushStrFmt(
            Scratch.MemPool,
            "raw((bool)(%S))",
            PRINT_STR(Value)
        );
        EvalEval Eval = EvalEvalFromStr(Expr);

        Result = !!EvalValueEvalFromEval(Eval).Value.U64;
        ReleaseScratch(Scratch);
    }

    return Result;
}

u64 
AppSettingFromNameU64(Str8 Name)
{
    u64 Result = 0;
    Str8 Value = AppSettingFromNameStr8(Name);

    if (Value.Size) {
        TempArena Scratch = GetScratch(NULL, 0);
        Str8 Expr = ArenaPushStrFmt(
            Scratch.MemPool,
            "raw((uint64)(%S))",
            PRINT_STR(Value)
        );
        EvalEval Eval = EvalEvalFromStr(Expr);

        Result = EvalValueEvalFromEval(Eval).Value.U64;
        ReleaseScratch(Scratch);
    }

    return Result;
}

f32 
AppSettingFromNameF32(Str8 Name)
{
    f32 Result = 0.0f;
    Str8 Value = AppSettingFromNameStr8(Name);

    if (Value.Size) {
        TempArena Scratch = GetScratch(NULL, 0);
        Str8 Expr = ArenaPushStrFmt(
            Scratch.MemPool,
            "raw((float32)(%S))",
            PRINT_STR(Value)
        );
        EvalEval Eval = EvalEvalFromStr(Expr);

        Result = EvalValueEvalFromEval(Eval).Value.F32;
        ReleaseScratch(Scratch);
    }

    return Result;
}

Arena* 
AppViewMemPool(void)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* VState = AppViewStateFromConfig(View);

    return VState->MemPool;
}

UIScrollPoint2D 
AppViewScrollPosition(void)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* VState = AppViewStateFromConfig(View);

    return VState->ScrollPosition;
}

EVView*
AppViewEvalView(void)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* ViewState = AppViewStateFromConfig(View);

    return ViewState->View;
}

Str8 
AppViewQueryCommand(void)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    ConfigNode* Query = ConfigNodeChildFromStr(View, "query"_s8);
    ConfigNode* Cmd = ConfigNodeChildFromStr(Query, "cmd"_s8);
    Str8 String = Cmd->Head->String;

    return String;
}

Str8 
AppViewQueryInput(void)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    ConfigNode* Query = ConfigNodeChildFromStr(View, "query"_s8);
    ConfigNode* Input = ConfigNodeChildFromStr(Query, "input"_s8);
    Str8 String = Input->Head->String;

    return String;
}


Str8 
AppViewSettingFromNameStr8(Str8 Name)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    Str8 Result = ConfigNodeChildFromStr(View, Name)->Head->String;

    if (!Result.Size)
        Result = AppDefaultSettingFromNames(View->String, Name);

    return Result;
}

EvalValue
AppViewSettingValueFromName(Str8 Name)
{
    Str8 Expr = AppViewSettingFromNameStr8(Name);
    EvalEval Eval = EvalEvalFromStr(Expr);
    EvalValue Result = EvalValueEvalFromEval(Eval).Value;

    return Result;
}

b32 
AppViewSettingFromNameb32(Str8 Name)
{
    Str8 Value = AppViewSettingFromNameStr8(Name);
    b32 Result = !!EvalValueFromStrFmt("raw((bool)(%S))", PRINT_STR(Value)).U64;

    return Result;
}

u64 
AppViewSettingFromNameU64(Str8 Name)
{
    Str8 Value = AppViewSettingFromNameStr8(Name);
    u64 Result = EvalValueFromStrFmt("raw((uint64)(%S))", PRINT_STR(Value)).U64;

    return Result;
}

f32 
AppViewSettingFromNameF32(Str8 Name)
{
    Str8 Value = AppViewSettingFromNameStr8(Name);
    f32 Result = EvalValueFromStrFmt("raw((float32)(%S))", PRINT_STR(Value)).F32;

    return Result;
}

f32 
AppFontSize(void)
{
    f32 Result = AppSettingFromNameF32("font_size"_s8);

    Result = CLAMP(6.0f, Result, 72.0f);

    return Result;
}

FontTag 
AppFontFromSlot(AppFontSlot Slot)
{
    FontTag Result = APP_STATE->FontSlotTable[Slot];

    return Result;
}

FontRasterKind 
AppRasterKindFromSlot(AppFontSlot Slot)
{
    ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
    AppWindowState* WS = AppWindowStateFromConfig(Window);
    FontRasterKind Result = WS->FontSlotRasterKind[Slot];

    return Result;
}

void 
AppRequestFrame(void)
{
    APP_STATE->NumFramesRequested = 4;
}

Arena* 
AppFrameMemPool(void)
{
    return APP_STATE->FrameMemPools[APP_STATE->FrameIndex % ARRAY_COUNT(APP_STATE->FrameMemPools)];
}

void 
AppRegistersFillSlotFromStr(AppRegistersSlot Slot, Str8 QueryExpr, Str8 String)
{
    switch (Slot) {
        default:
        case APP_REGISTERS_SLOT_STRING:
        case APP_REGISTERS_SLOT_FILE_PATH: {
            Str8TextPointPair Pair = Str8TextPointPairFromStr(String);

            Registers()->String = ArenaPushStrCpy(AppFrameMemPool(), String);

            if (Pair.Point.Line != 0) {
                Registers()->FilePath = ArenaPushStrCpy(AppFrameMemPool(), Pair.String);
                Registers()->Cursor = Pair.Point;
            }
        } break;

        case APP_REGISTERS_SLOT_COMMAND_NAME: {
            Registers()->CommandName = ArenaPushStrCpy(AppFrameMemPool(), String);
        } break;

        case APP_REGISTERS_SLOT_CONFIG:
        case APP_REGISTERS_SLOT_WINDOW:
        case APP_REGISTERS_SLOT_PANEL:
        case APP_REGISTERS_SLOT_TAB:
        case APP_REGISTERS_SLOT_VIEW:
        case APP_REGISTERS_SLOT_PREV_TAB:
        case APP_REGISTERS_SLOT_DST_PANEL: {
            b32 IsOkay = FALSE;

            if (!IsOkay && StrMatch(StrPrefix(String, 1), "$"_s8, 0)) {
                Str8 NumericPart = StrSkip(String, 1);
                ConfigID ID = U64FromStr(NumericPart, 16);

                Registers()->Config = ID;
                IsOkay = TRUE;
            }
        } break;

        case APP_REGISTERS_SLOT_CURSOR: {
            u64 Line = 0;
            u64 Column = 0;
            u64 ColonIndex = 0;

            if (StrFindSubStr(String, ":"_s8, 0, 0) != String.Size) {
                Line = U64FromStr(StrPrefix(String, ColonIndex));
                Column = U64FromStr(StrSkip(String, ColonIndex + 1));
            } else {
                Line = U64FromStr(String);
                Column = 1;
            }

            if (Line) {
                Registers()->Cursor = TxtPt(Line, MAX(1, Column));
                Registers()->Mark = Registers()->Cursor;
            }
        } break;
    }
}

AppCommandKind 
AppCommandKindFromStr(Str8 String)
{
    AppCommandKind Result = APP_COMMAND_KIND_NULL;

    for (u64 I = 0; I < ARRAY_COUNT(APP_COMMAND_KIND_INFO_TABLE); ++I) {
        if (StrMatch(String, APP_COMMAND_KIND_INFO_TABLE[I].String, 0)) {
            Result = (AppCommandKind) I;
            break;
        }
    }

    if (Result == APP_COMMAND_KIND_NULL) {
        LogError("'%S' [%llu] is not a valid command", PRINT_STR(String), String.Size);
    }

    return Result;
}

AppCommandKindInfo* 
AppCommandKindInfoFromStr(Str8 String)
{
    AppCommandKindInfo* Result = EMPTY_APP_COMMAND_KIND_INFO_VALUE;

    if (String.Size && String.Str) {
        AppCommandKind Kind = AppCommandKindFromStr(String);

        if (Kind != APP_COMMAND_KIND_NULL)
            Result = &APP_COMMAND_KIND_INFO_TABLE[Kind];
    }

    return Result;
}

void 
AppPushCmd(Str8 Name, AppRegisters* Regs)
{
    ListPush(APP_STATE->CommandMemPools[0], &APP_STATE->Commands[0], Name, Regs);
}

b32 
AppNextCommand(AppCommand** Command)
{
    u64 Slot = APP_STATE->CommandsGeneration % ARRAY_COUNT(APP_STATE->Commands);
    AppCommandNode* StartNode = APP_STATE->Commands[Slot].Head;

    if (Command[0]) {
        StartNode = CAST_FROM_MEMBER(AppCommandNode, Command, Command[0]);
        StartNode = StartNode->Next;
    }

    Command[0] = NULL;

    if (StartNode)
        Command[0] = &StartNode->Command;

    return !!Command[0];
}

b32
AppNextViewCommand(AppCommand **Command)
{
    for (; AppNextCommand(Command); ) {
        if (Registers()->View == Command[0]->Regs->View)
            break;
    }

    b32 Result = !!Command[0];

    return Result;
}

AppRegisters* 
__AppPushRegisters(AppRegisters* Regs)
{
    AppRegistersNode* Node = ArenaPushArrayZero(AppFrameMemPool(), AppRegistersNode, 1);

    CopyRegisters(AppFrameMemPool(), &Node->V, Regs);
    SLL_STACK_PUSH(APP_STATE->HeadRegisters, Node);

    return &Node->V;
}

AppRegisters* 
AppPopRegisters(void)
{
    AppRegisters* Regs = &APP_STATE->HeadRegisters->V;

    SLL_STACK_POP(APP_STATE->HeadRegisters);

    if (!APP_STATE->HeadRegisters)
        APP_STATE->HeadRegisters = &APP_STATE->BaseRegisters;

    return Regs;
}

MDNode*
AppThemeTreeFromName(Arena* MemPool, Access* Acc, Str8 Name)
{
    MDNode* Result = EMPTY_MD_NODE_VALUE;

    for (u32 Index = 0; Index < APP_THEME_PRESET_COUNT; ++Index) {
        if (StrMatch(Name, APP_THEME_PRESET_DISPLAY_STR_TABLE[Index], 0)) {
            Result = APP_STATE->ThemePresetTrees[Index];
            break;
        }
    }

    if (Result == EMPTY_MD_NODE_VALUE) {
        Str8 ThemesFolder = ArenaPushStrFmt(
            MemPool,
            "%S/%s/themes",
            PRINT_STR(GetProcessProperties()->UserProgramDataPath),
            NC_DATA_DIR
        );
        Str8 Path = ArenaPushStrFmt(
            MemPool, 
            "%S/%S",
            PRINT_STR(ThemesFolder),
            PRINT_STR(Name)
        );
        CKey Key = FSKeyFromPathRange(Path, Rng((u64) 0ULL, U64_MAX), 0);
        u128 FileHash = CHashFromKey(Key, 0);
        Str8 Text = CDataFromHash(Acc, FileHash);

        if (Text.Size)
            Result = MDTreeFromStr(MemPool, Text);
        else
            LogWarn("No custom themes exist, using default (Ayu Dark)"_s8);
    }

    if (Result == EMPTY_MD_NODE_VALUE)
        Result = APP_STATE->ThemePresetTrees[APP_THEME_PRESET_VISUAL_STUDIO_DARK];

    return Result;
}

UISignal
AppIconButton(AppIconKind Kind, FMRangeList* Matches, Str8 String)
{
    Str8 DisplayString = UIDisplayPartFromKeyStr(String);

    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(
        (
            UI_BOX_KIND_CLICKABLE |
            UI_BOX_KIND_DRAW_BACKGROUND |
            UI_BOX_KIND_DRAW_HOT_EFFECTS |
            UI_BOX_KIND_DRAW_ACTIVE_EFFECTS
        ),
        String
    );

    UIParent(Box) {
        if (!DisplayString.Size)
            UISpacer(UI_PERCENT(1, 0));
        else
            UISpacer(UI_EM(1.0f, 1.0f));

        UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
            AppFont(APP_FONT_SLOT_ICONS) {
                UIPreferredWidth(UI_EM(2.0f, 1.0f)) {
                    UIPreferredHeight(UI_PERCENT(1, 0)) {
                        UIAddFlags(UI_BOX_KIND_DISABLE_TEXT_TRUNC) {
                            UITag("weak"_s8) {
                                UILabel(APP_ICON_KIND_TEXT_TABLE[Kind]);
                            }
                        }
                    }
                }
            }
        }

        if (DisplayString.Size) {
            UIPreferredWidth(UI_PERCENT(1.0f, 0.0f)) {
                UIBox* Box = UILabel(DisplayString).Box;

                if (Matches)
                    UIBoxEquipFuzzyMatchRanges(Box, Matches);
            }
        }

        if (!DisplayString.Size)
            UISpacer(UI_PERCENT(1, 0));
        else
            UISpacer(UI_EM(1.0f, 1.0f));
    }

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal
AppIconButton(AppIconKind Kind, FMRangeList* Matches, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(
        Scratch.MemPool,
        Fmt,
        Args
    );
    UISignal Result = AppIconButton(Kind, Matches, String);

    va_end(Args);
    ReleaseScratch(Scratch);

    return Result;
}

void 
AppInit(CommandLine* CLI)
{
    EMPTY_APP_VIEW_STATE_VALUE->HashNext = EMPTY_APP_VIEW_STATE_VALUE;
    EMPTY_APP_VIEW_STATE_VALUE->HashPrev = EMPTY_APP_VIEW_STATE_VALUE;
    EMPTY_APP_WINDOW_STATE_VALUE->OrderNext = EMPTY_APP_WINDOW_STATE_VALUE;
    EMPTY_APP_WINDOW_STATE_VALUE->OrderPrev = EMPTY_APP_WINDOW_STATE_VALUE;
    EMPTY_APP_WINDOW_STATE_VALUE->HashNext = EMPTY_APP_WINDOW_STATE_VALUE;
    EMPTY_APP_WINDOW_STATE_VALUE->HashPrev = EMPTY_APP_WINDOW_STATE_VALUE;
    __EMPTY_APP_VOCAB_INFO_VALUE = {};

    PROFILE(AppInitTimer) {
        Arena* MemPool = ArenaAlloc();

        APP_STATE = ArenaPushArrayZero(MemPool, AppState, 1);
        APP_STATE->MemPool = MemPool;
        APP_STATE->SessionPathMemPool = ArenaAlloc();
        APP_STATE->ThemePathMemPool = ArenaAlloc();
        APP_STATE->SessionConfigStringKey = CKeyAlloc(CRootAlloc(), CIDAlloc(0, 0));
        APP_STATE->TransientConfigStringKey = CKeyAlloc(CRootAlloc(), CIDAlloc(0, 0));

        for (u64 Index = 0; Index < ARRAY_COUNT(APP_STATE->FrameMemPools); ++Index)
            APP_STATE->FrameMemPools[Index] = ArenaAlloc();

        APP_STATE->NumFramesRequested = 2;
        APP_STATE->ECache = EvalCacheAlloc();

        for (u64 Index = 0; Index < ARRAY_COUNT(APP_STATE->CommandMemPools); ++Index)
            APP_STATE->CommandMemPools[Index] = ArenaAlloc();

        APP_STATE->CommandOutputMemPool = ArenaAlloc();
        APP_STATE->PopUpMemPool = ArenaAlloc();
        APP_STATE->ContextMenuKey = UIKeyFromStr({}, "top_level_ctx_menu"_s8);
        APP_STATE->DropCompletionKey = UIKeyFromStr({}, "drop_completion_ctx_menu"_s8);
        APP_STATE->BindChangeMemPool = ArenaAlloc();
        APP_STATE->DragDropMemPool = ArenaAlloc();
        APP_STATE->DragDropRegisters = ArenaPushArrayZero(APP_STATE->DragDropMemPool, AppRegisters, 1);
        APP_STATE->HeadRegisters = &APP_STATE->BaseRegisters;

        {
            APP_STATE->CfgSchemaTable = ArenaPushArrayZero(
                APP_STATE->MemPool,
                ConfigSchemaTable,
                1
            );
            APP_STATE->CfgSchemaTable->SlotsCount = 4096;
            APP_STATE->CfgSchemaTable->Slots = ArenaPushArrayZero(
                APP_STATE->MemPool,
                ConfigSchemaNode*,
                APP_STATE->CfgSchemaTable->SlotsCount
            );

            for (
                u64 Index = 0; 
                Index < ARRAY_COUNT(APP_NAME_SCHEMA_INFO_TABLE); 
                ++Index
            ) {
                MDNode* Schema = MDTreeFromStr(
                    APP_STATE->MemPool,
                    APP_NAME_SCHEMA_INFO_TABLE[Index].Schema
                )->Head;

                ConfigSchemaTableInsert(
                    APP_STATE->MemPool,
                    APP_STATE->CfgSchemaTable,
                    APP_NAME_SCHEMA_INFO_TABLE[Index].Name,
                    Schema
                );
            }
        }

        {
            for (
                AppThemePreset ThemePreset = 0; 
                ThemePreset < APP_THEME_PRESET_COUNT; 
                ++ThemePreset
            ) {
                APP_STATE->ThemePresetTrees[ThemePreset] = MDTreeFromStr(
                    APP_STATE->MemPool,
                    APP_THEME_PRESET_CONFIG_STR_TABLE[ThemePreset]
                )->Head;
            }
        }

        APP_STATE->VocabInfoMap.SingleSlotsCount = 1024;
        APP_STATE->VocabInfoMap.SingleSlots = ArenaPushArrayZero(
            APP_STATE->MemPool, 
            AppVocabInfoMapSlot, 
            APP_STATE->VocabInfoMap.SingleSlotsCount
        );
        APP_STATE->VocabInfoMap.PluralSlotsCount = 1024;
        APP_STATE->VocabInfoMap.PluralSlots = ArenaPushArrayZero(
            APP_STATE->MemPool, 
            AppVocabInfoMapSlot, 
            APP_STATE->VocabInfoMap.PluralSlotsCount
        );

        for (u64 Index = 0; Index < ARRAY_COUNT(APP_VOCAB_INFO_TABLE); ++Index) {
            AppVocabInfoMapNode* N = ArenaPushArrayZero(
                APP_STATE->MemPool, 
                AppVocabInfoMapNode, 
                1
            );

            MemCpy(&N->V, &APP_VOCAB_INFO_TABLE[Index], sizeof(N->V));

            u64 SingleHash = DHash(N->V.CodeName);
            u64 PluralHash = DHash(N->V.CodeNamePlural);
            
            if (N->V.CodeName.Size) {
                SLL_QUEUE_PUSH_EX(
                    APP_STATE->VocabInfoMap.SingleSlots[SingleHash % APP_STATE->VocabInfoMap.SingleSlotsCount].Head, 
                    APP_STATE->VocabInfoMap.SingleSlots[SingleHash % APP_STATE->VocabInfoMap.SingleSlotsCount].Tail, 
                    N, 
                    SingleNext
                );
            }

            if (N->V.CodeNamePlural.Size) {
                SLL_QUEUE_PUSH_EX(
                    APP_STATE->VocabInfoMap.PluralSlots[PluralHash % APP_STATE->VocabInfoMap.PluralSlotsCount].Head, 
                    APP_STATE->VocabInfoMap.PluralSlots[PluralHash % APP_STATE->VocabInfoMap.PluralSlotsCount].Tail, 
                    N, 
                    PluralNext
                );
            }
        }

        APP_STATE->ConfigIDSlotsCount = 1024;
        APP_STATE->ConfigIDSlots = ArenaPushArrayZero(
            MemPool, 
            ConfigNodePtrSlot, 
            APP_STATE->ConfigIDSlotsCount
        );
        APP_STATE->Config = ConfigStateAlloc();
        ConfigContextSelect(ConfigContextFromState(APP_STATE->Config));
        ConfigNodeNew(APP_STATE->Config, ConfigNodeRoot(), "session"_s8);
        ConfigNodeNew(APP_STATE->Config, ConfigNodeRoot(), "command_line"_s8);
        ConfigNodeNew(APP_STATE->Config, ConfigNodeRoot(), "transient"_s8);

        APP_STATE->WindowStateSlotsCount = 64;
        APP_STATE->WindowStateSlots = ArenaPushArrayZero(
            MemPool,
            AppWindowStateSlot, 
            APP_STATE->WindowStateSlotsCount
        );
        APP_STATE->HeadWindowState = EMPTY_APP_WINDOW_STATE_VALUE;
        APP_STATE->TailWindowState = EMPTY_APP_WINDOW_STATE_VALUE;
        APP_STATE->ViewStateSlotsCount = 4096;
        APP_STATE->ViewStateSlots = ArenaPushArrayZero(
            MemPool, 
            AppViewStateSlot, 
            APP_STATE->ViewStateSlotsCount
        );

        {
            TempArena Scratch = GetScratch(NULL, 0);
            Str8 SessionPath = CommandLineString(CLI, "session"_s8);

            if (SessionPath.Size && SessionPath.Str) {
                SessionPath = PathAbsoluteDstFromRelativeDstSrc(
                    Scratch.MemPool,
                    SessionPath,
                    GetProcessProperties()->InitialPath
                );
            }

            Str8 SessionProgramDataPath = GetProcessProperties()->UserProgramDataPath;
            Str8 SessionDataPath = ArenaPushStrFmt(
                Scratch.MemPool, 
                "%S/%s", 
                PRINT_STR(SessionProgramDataPath),
                NC_DATA_DIR
            );

            SystemCreateDirectory(SessionDataPath);

            if (!SessionPath.Size || !SessionPath.Str) {
                    SessionPath = ArenaPushStrFmt(
                    Scratch.MemPool, 
                    "%S/default.%s_session", 
                    PRINT_STR(SessionDataPath),
                    NC_DATA_DIR
                );
            }

            if (SessionPath.Size && SessionPath.Str) {
                ArenaClear(APP_STATE->SessionPathMemPool);
                APP_STATE->SessionPath = ArenaPushStrCpy(APP_STATE->SessionPathMemPool, SessionPath);
            }

            AppCmd(
                APP_COMMAND_KIND_OPEN_SESSION,
                __Registers.FilePath = SessionPath
            );

            ReleaseScratch(Scratch);
        }

        // {
        //     TempArena Scratch = GetScratch(NULL, 0);
        //     Str8 Data = APP_ICON_FILE_BYTES;
        //     u8* DataPtr = Data.Str;
        //     u8* DataEnd = DataPtr + Data.Size;

        //     PACKED_STRUCT_BEGIN
        //     struct ICOHeader {
        //         u16 __Reserved;
        //         u16 ImageKind;
        //         u16 ImageCount;
        //     } PACKED;

        //     struct ICOEntry {
        //         u8 ImageWidthPX;
        //         u8 ImageHeightPX;
        //         u8 ColoursCount;
        //         u8 __Reserved;

        //         union {
        //             u16 ICOColourPlanes;
        //             u16 CurHotspotXPX;
        //         };

        //         union {
        //             u16 ICOBitsPerPixel;
        //             u16 CurHotspotYPX;
        //         };

        //         u32 ImageDataSize;
        //         u32 ImageDataOffset;
        //     } PACKED;
        //     PACKED_STRUCT_END

        //     ICOHeader Header = {};

        //     if (DataPtr + sizeof(Header) < DataEnd) {
        //         MemCpy(&Header, DataPtr, sizeof(Header));
        //         DataPtr += sizeof(Header);
        //     }

        //     u64 EntriesCount = Header.ImageCount;
        //     ICOEntry* Entries = ArenaPushArrayZero(
        //         Scratch.MemPool,
        //         ICOEntry,
        //         Header.ImageCount
        //     );

        //     {
        //         u64 BytesToRead = sizeof(ICOEntry) * EntriesCount;

        //         BytesToRead = MIN(BytesToRead, DataEnd - DataPtr);
        //         MemCpy(Entries, DataPtr, BytesToRead);
        //         DataPtr += BytesToRead;
        //     }

        //     ICOEntry* BestEntry = NULL;
        //     u64 BestentryArea = 0;

        //     for (u64 Index = 0; Index < EntriesCount; ++Index) {
        //         ICOEntry* Entry = &Entries[Index];
        //         u64 Width = Entry->ImageWidthPX;

        //         if (!Width)
        //             Width = 256;

        //         u64 Height = Entry->ImageHeightPX;

        //         if (!Height)
        //             Height = 256;

        //         u64 EntryArea = Width * Height;

        //         if (EntryArea > BestentryArea) {
        //             BestEntry = Entry;
        //             BestentryArea = EntryArea;
        //         }
        //     }

        //     u8* ImageData = NULL;
        //     v2i32 ImageLength = {};

        //     // FIXME: this is such a fucking waste of time.
        //     // FIXME: need to actually get an ICO file where each layer is compressed PNG and not BMP/DIB
        //     if (BestEntry) {
        //         u8* FileDataPtr = Data.Str + BestEntry->ImageDataOffset;
        //         u64 FileDataSize = BestEntry->ImageDataSize;
        //         int Width = 0;
        //         int Height = 0;
        //         int Components = 0;
        //         b32 IsPNG = (
        //             FileDataSize > 4 && 
        //             FileDataPtr[0] == 0x89 && FileDataPtr[1] == 0x50 && 
        //             FileDataPtr[2] == 0x4E && FileDataPtr[3] == 0x47
        //         );

        //         if (IsPNG) {
        //             ImageData = stbi_load_from_memory(
        //                 FileDataPtr,
        //                 FileDataSize,
        //                 &Width,
        //                 &Height,
        //                 &Components,
        //                 4
        //             );
        //         } else {
        //             PACKED_STRUCT_BEGIN
        //             struct BMPFileHeader {
        //                 u8  Signature[2];
        //                 u32 FileSize;
        //                 u32 Reserved;
        //                 u32 DataOffset;
        //             } PACKED;
        //             PACKED_STRUCT_END

        //             u32 BmpSize = (u32) (sizeof(BMPFileHeader) + FileDataSize);
        //             u8* BmpData = ArenaPushArrayZero(Scratch.MemPool, u8, BmpSize);
        //             BMPFileHeader* BmpHeader = (BMPFileHeader*)BmpData;

        //             BmpHeader->Signature[0] = 'B';
        //             BmpHeader->Signature[1] = 'M';
        //             BmpHeader->FileSize = BmpSize;
        //             BmpHeader->Reserved = 0;

        //             u32 DIBHeaderSize = 0;
        //             u16 BitCount = 0;
        //             u32 Compression = 0;
        //             u32 ClrUsed = 0;
        //             
        //             MemCpy(&DIBHeaderSize, FileDataPtr, 4);
        //             MemCpy(&BitCount, FileDataPtr + 14, 2);
        //             MemCpy(&Compression, FileDataPtr + 16, 4);
        //             MemCpy(&ClrUsed, FileDataPtr + 32, 4);

        //             u32 PaletteSize = 0;

        //             if (BitCount < 16) {
        //                 PaletteSize = (ClrUsed ? ClrUsed : (1 << BitCount)) * 4;
        //             } else if (Compression == 3 && DIBHeaderSize == 40) {
        //                 PaletteSize = 12;                                       // NOTE: BI_BITFIELDS masks
        //             }

        //             BmpHeader->DataOffset = sizeof(BMPFileHeader) + DIBHeaderSize + PaletteSize;

        //             u8* DIBDest = BmpData + sizeof(BMPFileHeader);

        //             MemCpy(DIBDest, FileDataPtr, FileDataSize);

        //             i32 DIBHeight = 0;

        //             MemCpy(&DIBHeight, DIBDest + 8, 4);
        //             DIBHeight = -Abs32(DIBHeight / 2);
        //             MemCpy(DIBDest + 8, &DIBHeight, 4);
        //             ImageData = stbi_load_from_memory(
        //                 BmpData,
        //                 BmpSize,
        //                 &Width,
        //                 &Height,
        //                 &Components,
        //                 4
        //             );
        //         }

        //         ImageLength.X = Width;
        //         ImageLength.Y = Height;
        //     }

        //     if (ImageData) {
        //         APP_STATE->IconTexture = Tex2DAlloc(
        //             RESOURCE_KIND_STATIC,
        //             ImageLength,
        //             TEX2D_FMT_RGBA8,
        //             ImageData
        //         );
        //         stbi_image_free(ImageData);
        //     } else {
        //         char const* Error = stbi_failure_reason();

        //         LogError("failed to load icon file (%s)", Error);
        //     }

        //     ReleaseScratch(Scratch);
        // }
    }
}


ConfigNode* 
AppImmediateConfigFromKey(Str8 String)
{
    ConfigNode* Transient = ConfigNodeChildFromStr(
        ConfigNodeRoot(),
        "transient"_s8
    );
    ConfigNode* Immediate = EMPTY_CFG_NODE_VALUE;
    ConfigNode* Result = EMPTY_CFG_NODE_VALUE;

    for (
        ConfigNode* Child = Transient->Head;
        Child != EMPTY_CFG_NODE_VALUE;
        Child = Child->Next
    ) {
        if (StrMatch(Child->String, "immediate"_s8, 0)) {
            Result = ConfigNodeChildFromStr(Child, String);

            if (Result != EMPTY_CFG_NODE_VALUE) {
                Immediate = Child;
                break;
            }
        }
    }

    if (Result == EMPTY_CFG_NODE_VALUE) {
        Immediate = ConfigNodeNew(
            APP_STATE->Config,
            Transient,
            "immediate"_s8
        );
        Result = ConfigNodeNew(APP_STATE->Config, Immediate, String);
    }

    ConfigNodeChildFromStrOrAlloc(APP_STATE->Config, Immediate, "hot"_s8);

    return Result;
}

ConfigNode* 
AppImmediateConfigFromKey(char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 Key = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);
    ConfigNode* Result = AppImmediateConfigFromKey(Key);

    va_end(Args);
    ReleaseScratch(Scratch);

    return Result;
}

Str8List 
AppPossibleOverridesFromFilePath(Arena* MemPool, Str8 FilePath)
{
    Str8List Result = {};

    return Result;
}

AppViewState* 
AppViewStateFromConfig(ConfigNode* Config)
{
    AppViewState* Result = EMPTY_APP_VIEW_STATE_VALUE;
    ConfigID ID = Config->ID;

    if (
        ID && 
        ID == APP_STATE->ViewStateLastAccessedID && 
        ID == APP_STATE->ViewStateLastAccessed->CfgID
    ) {
        Result = APP_STATE->ViewStateLastAccessed;
    } else {
        u64 IDHash = DHash(Str8Struct(&ID));
        u64 SlotIndex = IDHash % APP_STATE->ViewStateSlotsCount;
        AppViewStateSlot* Slot = &APP_STATE->ViewStateSlots[SlotIndex];

        for (AppViewState* V = Slot->Head; V; V = V->HashNext) {
            if (V->CfgID == ID) {
                Result = V;
                break;
            }
        }
    }

    if (Result == EMPTY_APP_VIEW_STATE_VALUE) {
        Result = APP_STATE->FreeViewState;

        if (Result)
            SLL_STACK_POP_EX(APP_STATE->FreeViewState, HashNext);
        else
            Result = ArenaPushArrayZero(APP_STATE->MemPool, AppViewState, 1);

        MemCpy(Result, EMPTY_APP_VIEW_STATE_VALUE, sizeof(*Result));

        u64 IDHash = DHash(Str8Struct(&ID));
        u64 SlotIndex = IDHash % APP_STATE->ViewStateSlotsCount;
        AppViewStateSlot* Slot = &APP_STATE->ViewStateSlots[SlotIndex];

        DLL_PUSH_BACK_EX(NULL, Slot->Head, Slot->Tail, Result, HashNext, HashPrev);
        Result->CfgID = ID;
        Result->MemPool = ArenaAlloc();
        Result->MemPoolResetPosition = ArenaGetPosition(Result->MemPool);
        Result->View = EVViewAlloc();
    }

    if (Result != EMPTY_APP_VIEW_STATE_VALUE)
        Result->LastFrameIndexTouched = APP_STATE->FrameIndex;

    APP_STATE->ViewStateLastAccessed = Result;
    APP_STATE->ViewStateLastAccessedID = ID;

    return Result;
}

void 
AppFrame(void)
{
    TempArena Scratch = GetScratch(NULL, 0);

    ++APP_STATE->FrameDepth;
    ArenaClear(AppFrameMemPool());
    APP_STATE->HeadRegisters = &APP_STATE->BaseRegisters;
    CopyRegisters(
        AppFrameMemPool(), 
        &APP_STATE->HeadRegisters->V, 
        &APP_STATE->HeadRegisters->V
    );

    if (APP_STATE->NextHoverRegisters) {
        APP_STATE->HoverRegisters = CopyRegisters(
            AppFrameMemPool(), 
            APP_STATE->NextHoverRegisters
        );
        APP_STATE->HoverRegistersSlot = APP_STATE->NextHoverRegistersSlot;
        APP_STATE->NextHoverRegisters = NULL;
    } else {
        APP_STATE->HoverRegisters = ArenaPushArrayZero(
            AppFrameMemPool(), 
            AppRegisters, 
            1
        );
        APP_STATE->HoverRegistersSlot = APP_REGISTERS_SLOT_NULL;
    }

    b32 AllowTextHotkeys = !APP_STATE->TextEditMode;

    APP_STATE->TextEditMode = FALSE;

    if (APP_STATE->FrameDepth == 1) {
        ArenaClear(APP_STATE->CommandOutputMemPool);
        MemSet(&APP_STATE->CommandOutputs, 0, sizeof(APP_STATE->CommandOutputs));
    }

    if (APP_STATE->FrameDepth == 1) {
        TempArena Scratch = GetScratch(NULL, 0);
        ConfigNodePtrList Windows = ConfigNodeTopLevelListFromStr(
            Scratch.MemPool,
            "window"_s8
        );

        for (
            ConfigNodePtrNode* N = Windows.Head;
            N;
            N = N->Next
        ) {
            ConfigNode* Window = N->V;
            ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                Scratch.MemPool,
                Window
            );

            for (
                ConfigPanelNode* P = PanelTree.Root;
                P != EMPTY_CFG_PANEL_NODE_VALUE;
                P = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, P).Next
            ) {
                ConfigNode* HeadUnfilteredTab = EMPTY_CFG_NODE_VALUE;

                for (
                    ConfigNodePtrNode* Node = P->Tabs.Head;
                    Node;
                    Node = Node->Next
                ) {
                    ConfigNode* Tab = Node->V;

                    if (AppIsProjectFiltered(Tab))
                        continue;

                    if (HeadUnfilteredTab == EMPTY_CFG_NODE_VALUE)
                        HeadUnfilteredTab = Tab;

                    AppViewStateFromConfig(Tab);
                }

                if (
                    P->SelectedTab == EMPTY_CFG_NODE_VALUE &&
                    HeadUnfilteredTab != EMPTY_CFG_NODE_VALUE
                ) {
                    AppCmd(
                        APP_COMMAND_KIND_FOCUS_TAB,
                        __Registers.Panel = P->Config->ID,
                        __Registers.Tab = HeadUnfilteredTab->ID
                    );
                }
            }
        }

        ReleaseScratch(Scratch);
    }

    if (APP_STATE->FrameDepth == 1) {
        ConfigNode* Transient = ConfigNodeChildFromStr(
            ConfigNodeRoot(), 
            "transient"_s8
        );

        for (
            ConfigNode* TLN = Transient->Head, *Next = EMPTY_CFG_NODE_VALUE;
            TLN != EMPTY_CFG_NODE_VALUE;
            TLN = Next
        ) {
            Next = TLN->Next;

            if (StrMatch(TLN->String, "immediate"_s8, 0)) {
                if (ConfigNodeChildFromStr(TLN, "hot"_s8) == EMPTY_CFG_NODE_VALUE)
                    ConfigNodeRelease(APP_STATE->Config, TLN);
            }
        }

        for (
            ConfigNode* TLN = Transient->Head; 
            TLN != EMPTY_CFG_NODE_VALUE; 
            TLN = TLN->Next
        ) {
            if (StrMatch(TLN->String, "immediate"_s8, 0)) {
                for (
                    ConfigNode* Child = TLN->Head, *Next = EMPTY_CFG_NODE_VALUE; 
                    Child != EMPTY_CFG_NODE_VALUE; 
                    Child = Next
                ) {
                    Next = Child->Next;

                    if (StrMatch(Child->String, "hot"_s8, 0))
                        ConfigNodeRelease(APP_STATE->Config, Child);
                }
            }
        }
    }

    if (APP_STATE->FrameDepth == 1) {
        for (u64 SlotIndex = 0; SlotIndex < APP_STATE->ViewStateSlotsCount; ++SlotIndex) {
            for (
                AppViewState* VS = APP_STATE->ViewStateSlots[SlotIndex].Head, *Next; 
                VS; 
                VS = Next
            ) {
                Next = VS->HashNext;

                u64 ExtTotal = 0;

                for (AppArenaExt* Ext = VS->HeadArenaExt; Ext; Ext = Ext->Next) {
                    ExtTotal += ArenaGetPosition(Ext->MemPool);
                }

                if (VS->LastFrameIndexTouched + 2 < APP_STATE->FrameIndex) {
                    EVViewRelease(VS->View);

                    for (
                        AppArenaExt* Ext = VS->HeadArenaExt;
                        Ext;
                        Ext = Ext->Next
                    ) {
                        ArenaRelease(Ext->MemPool);
                    }

                    ArenaRelease(VS->MemPool);
                    DLL_REMOVE_EX(
                        NULL,
                        APP_STATE->ViewStateSlots[SlotIndex].Head,
                        APP_STATE->ViewStateSlots[SlotIndex].Tail,
                        VS,
                        HashNext,
                        HashPrev
                    );
                    SLL_STACK_PUSH_EX(
                        APP_STATE->FreeViewState,
                        VS,
                        HashNext
                    );
                }
            }
        }
    }

    if (APP_STATE->FrameDepth == 1) {
        b32 AnyWindowIsFocused = FALSE;

        for (
            AppWindowState* W = APP_STATE->HeadWindowState; 
            W != EMPTY_APP_WINDOW_STATE_VALUE; 
            W = W->OrderNext
        ) {
            if (WindowIsFocused(W->WinHandle)) {
                AnyWindowIsFocused = TRUE;
                break;
            }
        }

        f32 SlowRate = 1.0f - Pow(2.0f, (-10.0f * APP_STATE->FrameDeltaTime));
        f32 FastRate = 1.0f - Pow(2.0f, (-40.0f * APP_STATE->FrameDeltaTime));

        for (
            u64 SlotIndex = 0; 
            SlotIndex < APP_STATE->ViewStateSlotsCount; 
            ++SlotIndex
        ) {
            for (
                AppViewState* VS = APP_STATE->ViewStateSlots[SlotIndex].Head;
                VS;
                VS = VS->HashNext
            ) {
                f32 ScrollXDiff = (-VS->ScrollPosition.X.Offset);
                f32 ScrollYDiff = (-VS->ScrollPosition.Y.Offset);
                f32 LoadingTDiff = (VS->LoadingTTarget - VS->LoadingT);

                VS->ScrollPosition.X.Offset += ScrollXDiff * APP_STATE->ScrollingAnimationRate;
                VS->ScrollPosition.Y.Offset += ScrollYDiff * APP_STATE->ScrollingAnimationRate;
                VS->LoadingT += LoadingTDiff * SlowRate;

                if (
                    (
                        AnyWindowIsFocused &&
                        AbsF32(LoadingTDiff) > 0.01f
                    ) ||
                    AbsF32(ScrollXDiff) > 0.01f ||
                    AbsF32(ScrollYDiff) > 0.01f
                ) {
                    AppRequestFrame();
                }

                if (AbsF32(ScrollXDiff) <= 0.01f)
                    VS->ScrollPosition.X.Offset = 0.0f;

                if (AbsF32(ScrollYDiff) <= 0.01f)
                    VS->ScrollPosition.Y.Offset = 0.0f;

                ConfigNode* VConfig = ConfigNodeFromID(VS->CfgID);

                if (
                    ConfigNodeChildFromStr(
                        VConfig, 
                        "selected"_s8
                    ) != EMPTY_CFG_NODE_VALUE
                ) {
                    if (VS->LoadingTTarget > 0.5f && AnyWindowIsFocused)
                        AppRequestFrame();

                    VS->LoadingTTarget = 0.0f;
                }
            }
        }
    }

    InputEventList Events = {};

    if (APP_STATE->FrameDepth == 1)
        Events = GetEvents(Scratch.MemPool, !APP_STATE->NumFramesRequested);

    Access* FrameAccessRestore = APP_STATE->FrameAccess;

    APP_STATE->FrameAccess = AccessOpen();

    u64 FrameTimeHistoryAvgUSecs = 0;

    {
        u64 NumFramesInHistory = MIN(
            ARRAY_COUNT(APP_STATE->FrameTimeUSecsHistory), 
            APP_STATE->FrameIndex
        );
        u64 FrameTimeHistorySumUSecs = 0;

        if (NumFramesInHistory) {
            for (u64 Index = 0; Index < NumFramesInHistory; ++Index)
                FrameTimeHistorySumUSecs += APP_STATE->FrameTimeUSecsHistory[Index];

            FrameTimeHistoryAvgUSecs = FrameTimeHistorySumUSecs / NumFramesInHistory;
        }
    }

    f32 TargetHz = GetGraphicsProperties()->DefaultRefreshRate;

    if (APP_STATE->FrameIndex > 32) {
        f32 PossibleHzTargets[] = {
            TargetHz,
            60.0f,
            75.0f,
            120.0f,
            144.0f,
            165.0f,
            240.0f,
            360.0f
        };
        f32 BestTargetHz = TargetHz;
        i64 BestTargetHzFrameTimeUSecsDiff = I64_MAX;

        for (u64 Index = 0; Index < ARRAY_COUNT(PossibleHzTargets); ++Index) {
            f32 Candidate = PossibleHzTargets[Index];

            if (Candidate <= TargetHz) {
                u64 CandidateFrameTimeUSecs = MILLION(1) / (u64) Candidate;
                i64 FrameTimeUSecsDiff = (i64) FrameTimeHistoryAvgUSecs - (i64) CandidateFrameTimeUSecs;

                if (
                    Abs64(FrameTimeUSecsDiff) < BestTargetHzFrameTimeUSecsDiff &&
                    FrameTimeHistoryAvgUSecs < CandidateFrameTimeUSecs + CandidateFrameTimeUSecs / 4
                ) {
                    BestTargetHz = Candidate;
                    BestTargetHzFrameTimeUSecsDiff = FrameTimeUSecsDiff;
                }
            }
        }

        TargetHz = BestTargetHz;
    }

    {
        APP_STATE->FrameEvalMemReadEndTimeUSecs = 0;

        u64 FrameTimeTargetCapacityUSecs = (u64) (MILLION(1) / TargetHz);

        if (FrameTimeHistoryAvgUSecs < FrameTimeTargetCapacityUSecs) {
            u64 SpareTime = (FrameTimeTargetCapacityUSecs - FrameTimeHistoryAvgUSecs) + MILLISECONDS(4);

            APP_STATE->FrameEvalMemReadEndTimeUSecs = TimeGetTimestamp() + SpareTime;
        }
    }

    APP_STATE->FrameDeltaTime = 1.0f / TargetHz;

    u64 StartTimeUSecs = TimeGetTimestamp();

    if (!APP_STATE->PopUpActive && APP_STATE->BindChangeActive) {
        if (InputPress(&Events, EMPTY_HANDLE_VALUE, 0, INPUT_KIND_ESC)) {
            AppRequestFrame();
            APP_STATE->BindChangeActive = FALSE;
        }

        if (InputPress(&Events, EMPTY_HANDLE_VALUE, 0, INPUT_KIND_DELETE)) {
            AppRequestFrame();
            ConfigNodeRelease(
                APP_STATE->Config, 
                ConfigNodeFromID(APP_STATE->BindChangeBindingID)
            );
            APP_STATE->BindChangeActive = FALSE;
        }

        for (
            InputEvent* Event = Events.Head, *Next = NULL; 
            Event; 
            Event = Next
        ) {
            if (
                Event->Kind == EVENT_KIND_PRESS &&
                Event->Input != INPUT_KIND_ESC &&
                Event->Input != INPUT_KIND_RETURN &&
                Event->Input != INPUT_KIND_BACKSPACE &&
                Event->Input != INPUT_KIND_DELETE &&
                Event->Input != INPUT_KIND_LEFT_MOUSE_BTN &&
                Event->Input != INPUT_KIND_RIGHT_MOUSE_BTN &&
                Event->Input != INPUT_KIND_MIDDLE_MOUSE_BTN &&
                Event->Input != INPUT_KIND_CTRL &&
                Event->Input != INPUT_KIND_ALT &&
                Event->Input != INPUT_KIND_SHIFT
            ) {
                APP_STATE->BindChangeActive = FALSE;

                ConfigNode* Binding = ConfigNodeFromID(APP_STATE->BindChangeBindingID);

                if (Binding == EMPTY_CFG_NODE_VALUE) {
                    ConfigNode* Session = ConfigNodeChildFromStr(
                        ConfigNodeRoot(), 
                        "session"_s8
                    );
                    ConfigNode* InputBindings = ConfigNodeChildFromStrOrAlloc(
                        APP_STATE->Config,
                        Session,
                        "keybindings"_s8
                    );

                    Binding = ConfigNodeNew(APP_STATE->Config, InputBindings, ""_s8);
                }

                ConfigNodeReleaseAllChildren(APP_STATE->Config, Binding);
                ConfigNodeNew(
                    APP_STATE->Config, 
                    Binding, 
                    APP_STATE->BindChangeCommandName
                );
                ConfigNodeNew(
                    APP_STATE->Config, 
                    Binding, 
                    INPUT_DISPLAY_STR_TABLE[Event->Input]
                );

                if (Event->Modifier & INPUT_MOD_KIND_CTRL)
                    ConfigNodeNew(APP_STATE->Config, Binding, "ctrl"_s8);

                if (Event->Modifier & INPUT_MOD_KIND_SHIFT)
                    ConfigNodeNew(APP_STATE->Config, Binding, "shift"_s8);

                if (Event->Modifier & INPUT_MOD_KIND_ALT)
                    ConfigNodeNew(APP_STATE->Config, Binding, "alt"_s8);

                u32 CodePoint = CodePointFromInput(Event->Modifier, Event->Input);

                Text(&Events, Event->WindowHandle, CodePoint);
                ConsumeEvent(&Events, Event);
                AppRequestFrame();
                break;
            }
        }
    }

    PROFILE(AppStateKeyMapBuildTimer) {
        APP_STATE->KeyMap = ConfigInputMapFromConfig(AppFrameMemPool());
    }

    Str8 MainFontName = AppSettingFromNameStr8("main_font"_s8);
    Str8 CodeFontName = AppSettingFromNameStr8("code_font"_s8);

    APP_STATE->FontSlotTable[APP_FONT_SLOT_MAIN] = FontTagFromPath(MainFontName);
    APP_STATE->FontSlotTable[APP_FONT_SLOT_CODE] = FontTagFromPath(CodeFontName);

    if (APP_STATE->FontSlotTable[APP_FONT_SLOT_MAIN] == EMPTY_FONT_TAG_VALUE) {
        APP_STATE->FontSlotTable[APP_FONT_SLOT_MAIN] = FontTagFromData(
            (Str8*) &APP_DEFAULT_MAIN_FONT_BYTES
        );
    }

    if (APP_STATE->FontSlotTable[APP_FONT_SLOT_CODE] == EMPTY_FONT_TAG_VALUE) {
        APP_STATE->FontSlotTable[APP_FONT_SLOT_CODE] = FontTagFromData(
            (Str8*) &APP_DEFAULT_DIAGRAM_FONT_BYTES
        );
    }

    APP_STATE->FontSlotTable[APP_FONT_SLOT_ICONS] = FontTagFromData(
        (Str8*) &APP_ICON_FONT_BYTES
    );

    for (
        InputEvent* Event = Events.Head, *Next = NULL;
        Event;
        Event = Next
    ) {
        AppRegistersScope() {
            Next = Event->Next;

            AppWindowState* WS = AppWindowStateFromHandle(Event->WindowHandle);

            if (WS && WS != AppWindowStateFromConfig(ConfigNodeFromID(Registers()->Window))) {
                TempArena Scratch = GetScratch(NULL, 0);
                ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                    Scratch.MemPool, 
                    ConfigNodeFromID(WS->CfgID)
                );

                Registers()->Window = WS->CfgID;
                Registers()->Panel = PanelTree.Focused->Config->ID;
                Registers()->Tab = PanelTree.Focused->SelectedTab->ID;
                Registers()->View = PanelTree.Focused->SelectedTab->ID;
                ReleaseScratch(Scratch);
            }

            b32 Take = FALSE;

            // NOTE(nathan): Start drag+drop
            if (
                AppDragIsActive() && 
                Event->Kind == EVENT_KIND_RELEASE && 
                Event->Input == INPUT_KIND_LEFT_MOUSE_BTN
            ) {
                APP_STATE->DragDropState = APP_DRAG_DROP_STATE_DROPPING;
            }

            // NOTE(nathan): Window close
            if (!Take && Event->Kind == EVENT_KIND_WINDOW_CLOSE && WS) {
                Take = TRUE;
                AppCmd(APP_COMMAND_KIND_EXIT);
            }

            // NOTE(nathan): Menubar operations
            if (APP_STATE->AltMenuBarEnabled && WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                if (
                    !Take && 
                    Event->Kind == EVENT_KIND_PRESS && 
                    Event->Input == INPUT_KIND_ALT && 
                    !Event->Modifier && 
                    !Event->IsRepeat
                ) {
                    Take = TRUE;
                    AppRequestFrame();
                    WS->MenuBarFocusedOnPress = WS->MenuBarFocused;
                    WS->MenuBarKeyHeld = TRUE;
                    WS->MenuBarFocusPressStarted = TRUE;
                }

                if (
                    WS->MenuBarFocused && 
                    Event->Kind == EVENT_KIND_PRESS && 
                    Event->Input == INPUT_KIND_ALT && 
                    !Event->Modifier && 
                    !Event->IsRepeat
                ) {
                    Take = TRUE;
                    AppRequestFrame();
                    WS->MenuBarFocused = FALSE;
                } else if (
                    WS->MenuBarFocusPressStarted && 
                    !WS->MenuBarFocused && 
                    Event->Kind == EVENT_KIND_RELEASE && 
                    !Event->Modifier && 
                    Event->Input == INPUT_KIND_ALT && 
                    !Event->IsRepeat
                ) {
                    Take = TRUE;
                    AppRequestFrame();
                    WS->MenuBarFocused = !WS->MenuBarFocusedOnPress;
                    WS->MenuBarFocusPressStarted = FALSE;
                } else if (
                    Event->Kind == EVENT_KIND_PRESS && 
                    Event->Input == INPUT_KIND_ESC && 
                    WS->MenuBarFocused && 
                    !UIAnyContextMenuIsOpen()
                ) {
                    Take = FALSE;
                    AppRequestFrame();
                    WS->MenuBarFocused = FALSE;
                }
            }

            // NOTE(nathan): Hotkey presses
            if (!Take && Event->Kind == EVENT_KIND_PRESS) {
                ConfigBinding Binding = {};

                Binding.Input = Event->Input;
                Binding.ModKind = Event->Modifier;

                ConfigInputMapNodePtrList KeyMapNodes = ConfigInputMapNodePtrListFromBinding(
                    Scratch.MemPool, 
                    APP_STATE->KeyMap, 
                    Binding
                );

                if (KeyMapNodes.Head) {
                    u32 HitCharacter = CodePointFromInput(Event->Modifier, Event->Input);
                    b32 IsPrintable = (HitCharacter >= 32 && HitCharacter < 127);

                    if (!HitCharacter || AllowTextHotkeys) {
                        AppCmd(
                            APP_COMMAND_KIND_RUN_COMMAND, 
                            __Registers.CommandName = KeyMapNodes.Head->V->Name
                        );

                        if (AllowTextHotkeys && IsPrintable) {
                            Text(&Events, Event->WindowHandle, HitCharacter);
                            Next = Event->Next;
                        }

                        Take = TRUE;

                        if (
                            (Event->Modifier & INPUT_MOD_KIND_ALT) && 
                            WS != EMPTY_APP_WINDOW_STATE_VALUE
                        ) {
                            WS->MenuBarFocusPressStarted = FALSE;
                        }
                    }
                } else if (Event->Input >= INPUT_KIND_FUNC_1 && Event->Input <= INPUT_KIND_FUNC_9) {
                    if (WS != EMPTY_APP_WINDOW_STATE_VALUE)
                        WS->MenuBarFocusPressStarted = FALSE;
                }

                AppRequestFrame();
            }

            // NOTE(nathan): Text events
            if (!Take && Event->Kind == EVENT_KIND_TEXT) {
                Str32 InsertionWide = Str(&Event->Character, 1);
                Str8 Insertion = Str8FromStr32(Scratch.MemPool, InsertionWide);

                AppCmd(APP_COMMAND_KIND_INSERT_TEXT, __Registers.String = Insertion);
                AppRequestFrame();
                Take = TRUE;

                if (
                    (Event->Modifier & INPUT_MOD_KIND_ALT) && 
                    WS != EMPTY_APP_WINDOW_STATE_VALUE
                ) {
                    WS->MenuBarFocusPressStarted = FALSE;
                }
            }

            // NOTE(nathan): Default handler fall-through
            if (!Take) {
                Take = TRUE;
                AppCmd(APP_COMMAND_KIND_OS_EVENT, __Registers.OSEvent = Event);
            }

            if (Take)
                ConsumeEvent(&Events, Event);
        }
    }

    EvalCacheSelect(APP_STATE->ECache);

    EvalBaseContext* EvalBaseCtx = ArenaPushArrayZero(
        Scratch.MemPool,
        EvalBaseContext,
        1
    );

    {
        EvalBaseContext* Ctx = EvalBaseCtx;

        Ctx->SpaceGenFunc = AppEvalSpaceGen;
        Ctx->SpaceReadFunc = AppEvalSpaceRead;
        Ctx->SpaceWriteFunc = AppEvalSpaceWrite;
    }

    EvalBaseContextSelect(EvalBaseCtx);
    EvalStr2ExprMap* MacroMap = ArenaPushArrayZero(
        Scratch.MemPool,
        EvalStr2ExprMap,
        1
    );

    MacroMap[0] = EvalStr2ExprMapAlloc(Scratch.MemPool, 512);

    EvalAutoHookMap* AutoHookMap = ArenaPushArrayZero(
        Scratch.MemPool,
        EvalAutoHookMap,
        1
    );

    AutoHookMap[0] = EvalAutoHookMapAlloc(Scratch.MemPool, 512);
    APP_STATE->MetaName2TypeMap = ArenaPushArrayZero(
        AppFrameMemPool(),
        EvalStr2TypeKeyMap,
        1
    );
    APP_STATE->MetaName2TypeMap[0] = EvalStr2TypeKeyMapAlloc(
        AppFrameMemPool(), 
        256
    );

    EVExpandRuleTable* ExpandRuleTable = ArenaPushArrayZero(
        Scratch.MemPool,
        EVExpandRuleTable,
        1
    );

    APP_STATE->ViewUIRuleMap = AppViewUIRuleMapAlloc(
        Scratch.MemPool,
        512
    );

    PROFILE(BuildExtraTypesAndMapsTimer) {
        {
            Str8 Names[] = {
                "commands"_s8,
                "tab_commands"_s8,
                "text_pt_commands"_s8,
                "text_range_commands"_s8
            };

            for (u64 Index = 0; Index < ARRAY_COUNT(Names); ++Index) {
                Str8 Name = Names[Index];
                EvalTypeKey TypeKey = EvalTypeKeyConstruct(
                    __Params.Kind = EVAL_TYPE_KIND_SET,
                    __Params.Flags = EVAL_TYPE_FLAG_STUB_SINGLE_LINE_EXPANSION,
                    __Params.Name = Name,
                    __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Commands),
                    __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Commands),
                    __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Commands)
                );
                EvalExpr* Expr = EvalPushExpr(
                    Scratch.MemPool,
                    EVAL_EXPR_KIND_LEAF_OFFSET,
                    Rng((u64) 0, (u64) 0)
                );

                Expr->TypeKey = TypeKey;
                Expr->Space = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_QUERY);
                MapInsert(Scratch.MemPool, MacroMap, Name, Expr);
            }
        }

        {
            Str8 Names[] = {
                "themes"_s8
            };

            for (u64 Index = 0; Index < ARRAY_COUNT(Names); ++Index) {
                Str8 Name = Names[Index];
                EvalTypeKey TypeKey = EvalTypeKeyConstruct(
                    __Params.Kind = EVAL_TYPE_KIND_SET,
                    __Params.Flags = EVAL_TYPE_FLAG_STUB_SINGLE_LINE_EXPANSION,
                    __Params.Name = Name,
                    __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Themes),
                    __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Themes),
                    __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Themes)
                );
                EvalExpr* Expr = EvalPushExpr(
                    Scratch.MemPool,
                    EVAL_EXPR_KIND_LEAF_OFFSET,
                    Rng((u64) 0, (u64) 0)
                );

                Expr->TypeKey = TypeKey;
                Expr->Space = EvalSpaceAlloc(
                    APP_EVAL_SPACE_KIND_META_QUERY
                );
                MapInsert(Scratch.MemPool, MacroMap, Name, Expr);
            }
        }

        {
            Str8 Names[] = {
                "views"_s8
            };

            for (u64 Index = 0; Index < ARRAY_COUNT(Names); ++Index) {
                Str8 Name = Names[Index];
                EvalTypeKey TypeKey = EvalTypeKeyConstruct(
                    __Params.Kind = EVAL_TYPE_KIND_SET,
                    __Params.Flags = EVAL_TYPE_FLAG_STUB_SINGLE_LINE_EXPANSION,
                    __Params.Name = Name,
                    __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Views),
                    __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Views),
                    __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Views)
                );
                EvalExpr* Expr = EvalPushExpr(
                    Scratch.MemPool,
                    EVAL_EXPR_KIND_LEAF_OFFSET,
                    Rng((u64) 0, (u64) 0)
                );

                Expr->TypeKey = TypeKey;
                Expr->Space = EvalSpaceAlloc(
                    APP_EVAL_SPACE_KIND_META_QUERY
                );
                MapInsert(Scratch.MemPool, MacroMap, Name, Expr);
            }
        }

        for (
            u64 Index = 0; 
            Index < ARRAY_COUNT(APP_NAME_SCHEMA_INFO_TABLE); 
            ++Index
        ) {
            Str8 Name = APP_NAME_SCHEMA_INFO_TABLE[Index].Name;
            EvalTypeKey TypeKey = EvalTypeKeyConstruct(
                __Params.Name = Name,
                __Params.Kind = EVAL_TYPE_KIND_SET,
                __Params.IRExtFunc = EVAL_TYPE_IR_EXT_FUNC_NAME(Schema),
                __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Schema),
                __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Schema),
                __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Schema)
            );

            MapInsert(
                AppFrameMemPool(),
                APP_STATE->MetaName2TypeMap,
                Name,
                TypeKey
            );
        }

        {
            Str8 Name = "config"_s8;
            EvalTypeKey TypeKey = EvalTypeKeyConstruct(
                __Params.Name = Name,
                __Params.Kind = EVAL_TYPE_KIND_SET,
                __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Configs)
            );
            EvalExpr* Expr = EvalPushExpr(
                Scratch.MemPool,
                EVAL_EXPR_KIND_LEAF_OFFSET,
                Rng((u64) 0, (u64) 0)
            );

            Expr->TypeKey = TypeKey;
            Expr->Space = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_QUERY);
            MapInsert(Scratch.MemPool, MacroMap, Name, Expr);
            MapInsert(
                AppFrameMemPool(), 
                APP_STATE->MetaName2TypeMap, 
                Name, 
                TypeKey
            );
        }

        // {
        //     Str8 Name = "control"_s8;
        //     EvalTypeKey TypeKey = EvalTypeKeyConstruct(
        //         __Params.Name = Name,
        //         __Params.Kind = EVAL_TYPE_KIND_SET,
        //         __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Control)
        //     );
        //     EvalExpr* Expr = EvalPushExpr(
        //         Scratch.MemPool,
        //         EVAL_EXPR_KIND_LEAF_OFFSET,
        //         Rng(0ULL, 0ULL)
        //     );

        //     Expr->TypeKey = TypeKey;
        //     Expr->Space = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_QUERY);
        //     MapInsert(
        //         Scratch.MemPool,
        //         MacroMap,
        //         Name,
        //         Expr
        //     );
        //     MapInsert(
        //         AppFrameMemPool(),
        //         APP_STATE->MetaName2TypeMap,
        //         Name,
        //         TypeKey
        //     );
        // }

        Str8 EvalCfgNames[] = {
            "breakpoint"_s8,
            "watch_pin"_s8,
            "target"_s8,
            "debug_info"_s8,
            "file_path_map"_s8,
            "type_view"_s8,
            "recent_session"_s8
        };

        for (u64 Index = 0; Index < ARRAY_COUNT(EvalCfgNames); ++Index) {
            Str8 CfgName = EvalCfgNames[Index];
            Str8 CollectionName = AppPluralFromCodeName(CfgName);
            EvalTypeKey CollectionTypeKey = EvalTypeKeyConstruct(
                __Params.Kind = EVAL_TYPE_KIND_SET,
                __Params.IRExtFunc = EVAL_TYPE_IR_EXT_FUNC_NAME(ConfigsSlice),
                __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(ConfigsSlice),
                __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(ConfigsSlice),
                __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(ConfigsSlice),
                __Params.ExpandRule.ExpandIDFromNumFunc = EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_NAME(ConfigsSlice),
                __Params.ExpandRule.ExpandNumFromIDFunc = EVAL_TYPE_EXPAND_NUM_FROM_ID_FUNC_NAME(ConfigsSlice)
            );
            EvalExpr* Expr = EvalPushExpr(Scratch.MemPool, EVAL_EXPR_KIND_LEAF_OFFSET, Rng((u64) 0, (u64) 0));

            Expr->TypeKey = CollectionTypeKey;
            Expr->Space = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_QUERY);
            MapInsert(Scratch.MemPool, MacroMap, CollectionName, Expr);
            MapInsert(AppFrameMemPool(), APP_STATE->MetaName2TypeMap, CollectionName, CollectionTypeKey);
        }

        for (
            u64 Index = 0; 
            Index < ARRAY_COUNT(APP_NAME_SCHEMA_INFO_TABLE); 
            ++Index
        ) {
            Str8 Name = APP_NAME_SCHEMA_INFO_TABLE[Index].Name;
            MDNodePtrList Schemas = ConfigSchemasFromName(
                Scratch.MemPool,
                APP_STATE->CfgSchemaTable,
                Name
            );
            b32 IsIndividuallyEvallable = FALSE;

            for (
                MDNodePtrNode* N = Schemas.Head;
                N;
                N = N->Next
            ) {
                if (
                    MDNodeHasChild(N->V, "label"_s8, 0) ||
                    MDNodeHasChild(N->V, "executable"_s8, 0)
                ) {
                    IsIndividuallyEvallable = TRUE;
                    break;
                }
            }

            if (IsIndividuallyEvallable) {
                EvalTypeKey TypeKey = EvalStr2TypeKeyMapLookup(
                    APP_STATE->MetaName2TypeMap, 
                    Name
                );
                ConfigNodePtrList Cfgs = ConfigNodeTopLevelListFromStr(
                    Scratch.MemPool,
                    Name
                );

                for (
                    ConfigNodePtrNode* N = Cfgs.Head;
                    N;
                    N = N->Next
                ) {
                    ConfigNode* Cfg = N->V;
                    Str8 Label = AppLabelFromConfig(Cfg);

                    if (Label.Size) {
                        EvalSpace Space = AppEvalSpaceFromConfig(Cfg);
                        EvalExpr* Expr = EvalPushExpr(
                            Scratch.MemPool,
                            EVAL_EXPR_KIND_LEAF_OFFSET,
                            Rng((u64) 0, (u64) 0)
                        );

                        Expr->Space = Space;
                        Expr->Mode = EVAL_MODE_OFFSET;
                        Expr->TypeKey = TypeKey;
                        MapInsert(
                            Scratch.MemPool,
                            MacroMap,
                            Label,
                            Expr
                        );
                    }
                }
            }
        }

        ConfigNodePtrList WatchTabs = {};

        {
            ConfigNodePtrList Windows = ConfigNodeTopLevelListFromStr(
                Scratch.MemPool,
                "window"_s8
            );

            for (
                ConfigNodePtrNode* N = Windows.Head;
                N;
                N = N->Next
            ) {
                ConfigNode* Window = N->V;

                {
                    EvalTypeKey TypeKey = EvalStr2TypeKeyMapLookup(
                        APP_STATE->MetaName2TypeMap,
                        Window->String
                    );
                    EvalSpace Space = AppEvalSpaceFromConfig(Window);
                    EvalExpr* Expr = EvalPushExpr(
                        Scratch.MemPool,
                        EVAL_EXPR_KIND_LEAF_OFFSET,
                        Rng((u64) 0, (u64) 0)
                    );

                    Expr->Space = Space;
                    Expr->Mode = EVAL_MODE_OFFSET;
                    Expr->TypeKey = TypeKey;
                    MapInsert(
                        Scratch.MemPool,
                        MacroMap,
                        ArenaPushStrFmt(
                            Scratch.MemPool,
                            "query:config.$%llx",
                            Window->ID
                        ),
                        Expr
                    );
                }

                ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                    Scratch.MemPool,
                    Window
                );

                for (
                    ConfigPanelNode* P = PanelTree.Root;
                    P != EMPTY_CFG_PANEL_NODE_VALUE;
                    P = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, P).Next
                ) {
                    for (
                        ConfigNodePtrNode* TabNode = P->Tabs.Head;
                        TabNode;
                        TabNode = TabNode->Next
                    ) {
                        ConfigNode* Tab = TabNode->V;
                        EvalTypeKey TypeKey = EvalStr2TypeKeyMapLookup(
                            APP_STATE->MetaName2TypeMap,
                            Tab->String
                        );
                        EvalSpace Space = AppEvalSpaceFromConfig(Tab);
                        EvalExpr* Expr = EvalPushExpr(
                            Scratch.MemPool,
                            EVAL_EXPR_KIND_LEAF_OFFSET,
                            Rng((u64) 0, (u64) 0)
                        );

                        Expr->Space = Space;
                        Expr->Mode = EVAL_MODE_OFFSET;
                        Expr->TypeKey = TypeKey;
                        MapInsert(
                            Scratch.MemPool,
                            MacroMap,
                            ArenaPushStrFmt(
                                Scratch.MemPool,
                                "query:config.$%llx",
                                Tab->ID
                            ),
                            Expr
                        );

                        if (StrMatch(Tab->String, "watch"_s8, 0))
                            ListPush(Scratch.MemPool, &WatchTabs, Tab);
                    }
                }
            }
        }

        for (
            ConfigNodePtrNode* N = WatchTabs.Head;
            N;
            N = N->Next
        ) {
            ConfigNode* WatchTab = N->V;

            for (
                ConfigNode* Child = WatchTab->Head;
                Child != EMPTY_CFG_NODE_VALUE;
                Child = Child->Next
            ) {
                if (AppIsProjectFiltered(Child))
                    continue;

                if (StrMatch(Child->String, "watch"_s8, 0)) {
                    ConfigNode* Watch = Child;
                    Str8 Expr = Watch->Head->String;
                    EvalParse Parse = EvalParseFromStr(Expr);

                    if (Parse.Msgs.MaxKind == EVAL_MSG_KIND_NULL) {
                        for (
                            EvalExpr* Expr = Parse.Expr;
                            Expr != EMPTY_EVAL_EXPR_VALUE;
                            Expr = Expr->Next
                        ) {
                            struct ExprWalkTask {
                                ExprWalkTask* Next;
                                EvalExpr*     Expr;
                            };

                            ExprWalkTask StartTask = {
                                NULL,
                                Expr
                            };
                            ExprWalkTask* HeadTask = &StartTask;
                            ExprWalkTask* TailTask = HeadTask;

                            for (
                                ExprWalkTask* T = HeadTask;
                                T;
                                T = T->Next
                            ) {
                                switch (T->Expr->Kind) {
                                    default: {
                                        for (
                                            EvalExpr* Child = T->Expr->Head;
                                            Child != EMPTY_EVAL_EXPR_VALUE;
                                            Child = Child->Next
                                        ) {
                                            ExprWalkTask* Task = ArenaPushArrayZero(
                                                Scratch.MemPool,
                                                ExprWalkTask,
                                                1
                                            );

                                            SLL_QUEUE_PUSH(
                                                HeadTask,
                                                TailTask,
                                                Task
                                            );
                                            Task->Expr = Child;
                                        }
                                    } break;

                                    // case EVAL_EXPR_KIND_CALL: {
                                    //     EvalExpr* LHS = T->Expr->Head;
                                    //     EvalExpr* RHS = LHS->Next;

                                    //     if (LHS->Kind == EVAL_EXPR_KIND_LEAF_IDENT) {
                                    //         MapInsert(
                                    //             Scratch.MemPool,
                                    //             MacroMap,
                                    //             LHS->String,
                                    //             RHS
                                    //         );
                                    //     }
                                    // } break;
                                }
                            }
                        }
                    }
                }
            }
        }

        {
            EvalTypeKey TypeKey = EvalStr2TypeKeyMapLookup(
                APP_STATE->MetaName2TypeMap,
                "session"_s8
            );
            EvalSpace Space = AppEvalSpaceFromConfig(
                ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8)
            );
            EvalExpr* Expr = EvalPushExpr(
                Scratch.MemPool,
                EVAL_EXPR_KIND_LEAF_OFFSET,
                Rng((u64) 0, (u64) 0)
            );

            Expr->Space = Space;
            Expr->Mode = EVAL_MODE_OFFSET;
            Expr->TypeKey = TypeKey;
            MapInsert(
                Scratch.MemPool,
                MacroMap,
                "session_settings"_s8,
                Expr
            );
        }

        // Str8 EvallableControlNames[] = {
        //     "project"_s8,
        //     "diagram"_s8,
        //     "symbol_library"_s8
        // };

        // for (
        //     u64 Index = 0; 
        //     Index < ARRAY_COUNT(EvallableControlNames); 
        //     ++Index
        // ) {
        //     Str8 Name = EvallableControlNames[Index];
        //     PIDEEntityKind Kind = PIDEEntityKindFromStr(Name);
        //     PIDEEntityArray Array = PIDEEntityArrayFromKind(Kind);
        //     EvalTypeKey TypeKey = EvalStr2TypeKeyMapLookup(
        //         APP_STATE->MetaName2TypeMap,
        //         Name
        //     );

        //     for (u64 I = 0; I < Array.Count; ++I) {
        //         PIDEEntity* Entity = Array.V[I];
        //         EvalSpace Space = PIDEEvalSpaceFromEntity(
        //             Entity, 
        //             APP_EVAL_SPACE_KIND_META_CTRL_ENTITY
        //         );
        //         EvalExpr* Expr = EvalPushExpr(
        //             Scratch.MemPool,
        //             EVAL_EXPR_KIND_LEAF_OFFSET,
        //             Rng(0ULL, 0ULL)
        //         );

        //         Expr->Space = Space;
        //         Expr->Mode = EVAL_MODE_OFFSET;
        //         Expr->TypeKey = TypeKey;

        //         if (Entity->String.Size) {
        //             MapInsert(
        //                 Scratch.MemPool,
        //                 MacroMap,
        //                 Entity->String,
        //                 Expr
        //             );
        //         }
        //     }
        // }

        // for (u64 Index = 0; Index < ARRAY_COUNT(EvallableControlNames); ++Index) {
        //     Str8 KindName = EvallableControlNames[Index];
        //     Str8 CollectionName = AppPluralFromCodeName(KindName);
        //     EvalTypeKey CollectionTypeKey = EvalTypeKeyConstruct(
        //         __Params.Kind = EVAL_TYPE_KIND_SET,
        //         __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(ControlEntities),
        //         __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(ControlEntities),
        //         __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(ControlEntities)
        //     );
        //     EvalExpr* Expr = EvalPushExpr(
        //         Scratch.MemPool,
        //         EVAL_EXPR_KIND_LEAF_OFFSET,
        //         Rng(0ULL, 0ULL)
        //     );

        //     Expr->TypeKey = CollectionTypeKey;
        //     Expr->Space = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_QUERY);
        //     MapInsert(
        //         Scratch.MemPool,
        //         MacroMap,
        //         CollectionName,
        //         Expr
        //     );
        //     MapInsert(
        //         AppFrameMemPool(),
        //         APP_STATE->MetaName2TypeMap,
        //         CollectionName,
        //         CollectionTypeKey
        //     );
        // }

        {
            MapInsert(
                AppFrameMemPool(),
                APP_STATE->MetaName2TypeMap,
                "watches"_s8,
                EvalTypeKeyConstruct(
                    __Params.Kind = EVAL_TYPE_KIND_SET,
                    __Params.Flags = EVAL_TYPE_FLAG_EDITABLE_CHILDREN | EVAL_TYPE_FLAG_STUB_SINGLE_LINE_EXPANSION,
                    __Params.Name = "watches"_s8,
                    __Params.IRExtFunc = EVAL_TYPE_IR_EXT_FUNC_NAME(Watches),
                    __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(Watches),
                    __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Watches),
                    __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Watches),
                    __Params.ExpandRule.ExpandIDFromNumFunc = EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_NAME(Watches),
                    __Params.ExpandRule.ExpandNumFromIDFunc = EVAL_TYPE_EXPAND_NUM_FROM_ID_FUNC_NAME(Watches)
                )
            );

            MapInsert(
                AppFrameMemPool(),
                APP_STATE->MetaName2TypeMap,
                "theme_colours"_s8,
                EvalTypeKeyConstruct(
                    __Params.Kind = EVAL_TYPE_KIND_SET,
                    __Params.Flags = EVAL_TYPE_FLAG_STUB_SINGLE_LINE_EXPANSION,
                    __Params.Name = "theme_colours"_s8,
                    __Params.IRExtFunc = EVAL_TYPE_IR_EXT_FUNC_NAME(ConfigsSlice),
                    __Params.AccessFunc = EVAL_TYPE_ACCESS_FUNC_NAME(ConfigsSlice),
                    __Params.ExpandRule.ExpandInfoFunc = EVAL_TYPE_EXPAND_INFO_FUNC_NAME(ConfigsQuery),
                    __Params.ExpandRule.ExpandRangeFunc = EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(ConfigsSlice),
                    __Params.ExpandRule.ExpandIDFromNumFunc = EVAL_TYPE_EXPAND_ID_FROM_NUM_FUNC_NAME(ConfigsSlice),
                    __Params.ExpandRule.ExpandNumFromIDFunc = EVAL_TYPE_EXPAND_NUM_FROM_ID_FUNC_NAME(ConfigsSlice)
                )
            );
        }

        struct {
            Str8                          Name;
            b32                           InheritedByMembers;
            b32                           InheritedByElements;
            b32                           ArrayLike;
            EvalTypeIRExtFuncType*        IRExtFunc;
            EvalTypeAccessFuncType*       AccessFunc;
            EvalTypeExpandRule            ExpandRule;
            AppViewUIFuncType*            ViewUIFunc;
            EVExpandRuleInfoHookFuncType* ExpandInfoHookFunc;
        } LensTable[] = {
            {
                "raw"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "bin"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "oct"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "dec"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "hex"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "digits"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "no_string"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "no_char"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "no_addr"_s8,
                TRUE,
                TRUE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "sequence"_s8,
                FALSE,
                FALSE,
                TRUE,
                NULL,
                NULL,
                {
                    EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Sequence),
                    EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Sequence)
                }
            },
            {
                "rows"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {
                    EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Rows),
                    EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Rows)
                }
            },
            {
                "columns"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "flatten"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            // {
            //     "omit"_s8,
            //     FALSE,
            //     FALSE,
            //     FALSE,
            //     NULL,
            //     NULL,
            //     {
            //         EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Omit),
            //         EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Omit)
            //     }
            // },
            {
                "range1"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {}
            },
            {
                "array"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {
                    EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Array),
                    EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Array)
                }
            },
            // {
            //     "slice"_s8,
            //     FALSE,
            //     FALSE,
            //     FALSE,
            //     EVAL_TYPE_IR_EXT_FUNC_NAME(Slice),
            //     EVAL_TYPE_ACCESS_FUNC_NAME(Slice),
            //     {
            //         EVAL_TYPE_EXPAND_INFO_FUNC_NAME(Slice),
            //         EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(Slice)
            //     }
            // },
            // {
            //     "list"_s8,
            //     FALSE,
            //     FALSE,
            //     FALSE,
            //     EVAL_TYPE_IR_EXT_FUNC_NAME(List),
            //     EVAL_TYPE_ACCESS_FUNC_NAME(List),
            //     {
            //         EVAL_TYPE_EXPAND_INFO_FUNC_NAME(List),
            //         EVAL_TYPE_EXPAND_RANGE_FUNC_NAME(List)
            //     }
            // },
            {
                "text"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {},
                APP_VIEW_UI_FUNC_NAME(Text),
                EV_EXPAND_RULE_INFO_FUNC_NAME(Text)
            },
            {
                "bitmap"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {},
                APP_VIEW_UI_FUNC_NAME(Bitmap),
                EV_EXPAND_RULE_INFO_FUNC_NAME(Bitmap)
            },
            {
                "colour"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {},
                APP_VIEW_UI_FUNC_NAME(Colour),
                EV_EXPAND_RULE_INFO_FUNC_NAME(Colour)
            },
            {
                "geo3d"_s8,
                FALSE,
                FALSE,
                FALSE,
                NULL,
                NULL,
                {},
                APP_VIEW_UI_FUNC_NAME(Geo3D),
                EV_EXPAND_RULE_INFO_FUNC_NAME(Geo3D)
            }
        };

        {
            for (u64 Index = 0; Index < ARRAY_COUNT(LensTable); ++Index) {
                if (LensTable[Index].ViewUIFunc) {
                    MapInsert(
                        Scratch.MemPool,
                        APP_STATE->ViewUIRuleMap,
                        LensTable[Index].Name,
                        LensTable[Index].ViewUIFunc
                    );
                }

                if (LensTable[Index].ExpandInfoHookFunc) {
                    TablePushNew(
                        Scratch.MemPool,
                        ExpandRuleTable,
                        __Rule.String = LensTable[Index].Name,
                        __Rule.InfoHookFunc = LensTable[Index].ExpandInfoHookFunc
                    );
                }
            }
        }

        for (u64 Index = 0; Index < ARRAY_COUNT(LensTable); ++Index) {
            EvalTypeFlag TypeFlags = 0;

            if (LensTable[Index].InheritedByMembers)
                TypeFlags |= EVAL_TYPE_FLAG_INHERITED_BY_MEMBERS;

            if (LensTable[Index].InheritedByElements)
                TypeFlags |= EVAL_TYPE_FLAG_INHERITED_BY_ELEMENTS;

            if (LensTable[Index].ArrayLike)
                TypeFlags |= EVAL_TYPE_FLAG_ARRAY_LIKE_EXPANSION;

            EvalExpr* Expr = EvalPushExpr(
                Scratch.MemPool,
                EVAL_EXPR_KIND_LEAF_OFFSET,
                Rng((u64) 0, (u64) 0)
            );

            Expr->TypeKey = EvalTypeKeyConstruct(
                __Params.Kind = EVAL_TYPE_KIND_LENS_SPEC,
                __Params.Flags = TypeFlags,
                __Params.Name = LensTable[Index].Name,
                __Params.IRExtFunc = LensTable[Index].IRExtFunc,
                __Params.AccessFunc = LensTable[Index].AccessFunc,
                __Params.ExpandRule = LensTable[Index].ExpandRule
            );
            MapInsert(
                Scratch.MemPool,
                MacroMap,
                LensTable[Index].Name,
                Expr
            );
        }
    }

    EVSelectExpandRuleTable(ExpandRuleTable);

    {
        ConfigNodePtrList TypeViews = ConfigNodeTopLevelListFromStr(
            Scratch.MemPool, 
            "type_view"_s8
        );
        ConfigNodePtrList RulesList[] = {
            TypeViews
        };

        for (u64 ListIndex = 0; ListIndex < ARRAY_COUNT(RulesList); ++ListIndex) {
            ConfigNodePtrList List = RulesList[ListIndex];

            for (
                ConfigNodePtrNode* N = List.Head;
                N;
                N = N->Next
            ) {
                ConfigNode* Rule = N->V;
                Str8 TypeString = ConfigNodeChildFromStr(
                    Rule,
                    "type"_s8
                )->Head->String;
                Str8 ExprString = ConfigNodeChildFromStr(
                    Rule,
                    "expr"_s8
                )->Head->String;

                AutoHookMapInsert(
                    Scratch.MemPool,
                    AutoHookMap,
                    __Params.TypePattern = TypeString,
                    __Params.TagExprString = ExprString
                );
            }
        }
    }

    EvalIRContext* IRCtx = ArenaPushArrayZero(
        Scratch.MemPool,
        EvalIRContext,
        1
    );

    IRCtx->MacroMap = MacroMap;
    IRCtx->AutoHookMap = AutoHookMap;
    EvalIRContextSelect(IRCtx);

    EvalInterpretContext* InterpCtx = ArenaPushArrayZero(
        Scratch.MemPool,
        EvalInterpretContext,
        1
    );

    InterpCtx->PrimarySpace = EvalSpaceAlloc(APP_EVAL_SPACE_KIND_META_VIEW);
    EvalInterpContextSelect(InterpCtx);
    APP_STATE->AltMenuBarEnabled = AppSettingFromNameB32("focus_menu_bar_with_alt"_s8);
    APP_STATE->EvalVisualBaseStringFlags = 0;

    AppCommand* Command = NULL;

    if (APP_STATE->FrameDepth == 1) {
        for (; AppNextCommand(&Command);) {
            AppRegistersScope() {
                AppCommandKind Kind = AppCommandKindFromStr(Command->Name);

                CopyRegisters(AppFrameMemPool(), Registers(), Command->Regs);
                AppRequestFrame();

                ConfigNode* Config = EMPTY_CFG_NODE_VALUE;
                Str8 DstPath = {};
                Str8 BucketName = {};
                Direction2D SplitDir = DIR_2D_INVALID;
                ConfigNode* SplitPanel = EMPTY_CFG_NODE_VALUE;
                u64 PanelSiblingOffset = 0;
                u64 PanelChildOffset = 0;
                v2i32 PanelChangeDirection = {};

                switch (Kind) {
                    default: {
                        if (Kind >= APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD) {
                            u64 FastPathIndex = (Kind - APP_COMMAND_KIND_FIRST_TAB_FAST_PATH_CMD);
                            Str8 ViewName = APP_TAB_FAST_PATH_VIEW_NAME_TABLE[FastPathIndex];
                            Str8 QueryName = APP_TAB_FAST_PATH_QUERY_NAME[FastPathIndex];

                            AppCmd(
                                APP_COMMAND_KIND_BUILD_TAB,
                                __Registers.String = ViewName,
                                __Registers.Expression = QueryName
                            );
                        }
                    } break;

                    // TODO(nathan): this is annoying, only confirm if unsaved or 
                    //             : something has not completed, etc.
                    case APP_COMMAND_KIND_EXIT: {
                        UIKey Key = UIKeyFromStr({}, "lossy_exit_confirmation"_s8);

                        if (!Registers()->ForceConfirm && APP_STATE->PopUpKey != Key) {
                            APP_STATE->PopUpKey = Key;
                            APP_STATE->PopUpActive = TRUE;
                            ArenaClear(APP_STATE->PopUpMemPool);
                            MemSet(&APP_STATE->PopUpCommands, 0, sizeof(APP_STATE->PopUpCommands));
                            APP_STATE->PopUpTitle = ArenaPushStrCpy(
                                APP_STATE->PopUpMemPool, 
                                "Are you sure you want to exit?"_s8
                            );
                            APP_STATE->PopUpDescription = ArenaPushStrCpy(
                                APP_STATE->PopUpMemPool, 
                                "Select 'OK' to immediately exit program"_s8
                            );

                            AppRegisters* Regs = CopyRegisters(AppFrameMemPool(), Registers());

                            Regs->ForceConfirm = TRUE;
                            ListPush(
                                APP_STATE->PopUpMemPool, 
                                &APP_STATE->PopUpCommands, 
                                APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_EXIT].String, 
                                Regs
                            );
                        } else {
                            AppCmd(APP_COMMAND_KIND_WRITE_SESSION_DATA);
                            APP_STATE->Quit = TRUE;
                        }
                    } break;

                    case APP_COMMAND_KIND_OPEN_PALETTE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Window);
                        ConfigNode* Tab = PanelTree.Focused->SelectedTab;
                        Str8List Exprs = {};

                        {
                            ListPush(
                                Scratch.MemPool,
                                &Exprs,
                                "query:commands"_s8
                            );

                            if (Tab != EMPTY_CFG_NODE_VALUE) {
                                ListPushFmt(
                                    Scratch.MemPool,
                                    &Exprs,
                                    "query:config.$%llx",
                                    Tab->ID
                                );
                            }

                            if (Window != EMPTY_CFG_NODE_VALUE) {
                                ListPushFmt(
                                    Scratch.MemPool,
                                    &Exprs,
                                    "query:config.$%llx",
                                    Window->ID
                                );
                            }

                            ListPush(
                                Scratch.MemPool,
                                &Exprs,
                                "query:recent_sessions"_s8
                            );
                            ListPush(
                                Scratch.MemPool,
                                &Exprs,
                                "query:source_files"_s8
                            );
                            ListPush(
                                Scratch.MemPool,
                                &Exprs,
                                "query:session_settings"_s8
                            );
                            ListPush(
                                Scratch.MemPool,
                                &Exprs,
                                "query:source_files"_s8
                            );
                        }

                        Str8JoinPart Join = {};

                        Join.Sep = ", "_s8;

                        Str8 Expr = StrListJoin(
                            Scratch.MemPool,
                            &Exprs,
                            &Join
                        );

                        AppCmd(
                            APP_COMMAND_KIND_PUSH_QUERY,
                            __Registers.Expression = Expr,
                            __Registers.DoImplicitRoot = TRUE,
                            __Registers.DoLister = TRUE,
                            __Registers.DoBigRows = TRUE,
                            __Registers.View = Tab->ID,
                            __Registers.Tab = Tab->ID
                        );
                    } break;

                    case APP_COMMAND_KIND_RUN_COMMAND:
                    case APP_COMMAND_KIND_OPEN_TAB: {
                        AppCommandKindInfo* Info = AppCommandKindInfoFromStr(Command->Regs->CommandName);

                        if (!(Info->Query.Kind & APP_QUERY_KIND_REQUIRED)) {
                            AppRegistersScope(__Registers.CommandName = ""_s8) {
                                AppPushCmd(Command->Regs->CommandName, Registers());
                            }
                        } else if (
                            Info->Query.Slot == APP_REGISTERS_SLOT_FILE_PATH && 
                            AppSettingFromNameB32("use_native_file_system_dialog"_s8)
                        ) {
                            ConfigNode* Session = ConfigNodeChildFromStr(
                                ConfigNodeRoot(),
                                "session"_s8
                            );
                            ConfigNode* CurrentPath = ConfigNodeChildFromStr(
                                Session, 
                                "current_path"_s8
                            );
                            Str8 CurrentPathString = CurrentPath->Head->String;

                            if (!CurrentPathString.Size)
                                CurrentPathString = PathNormalisedFromStr(Scratch.MemPool, GetCurrentPath(Scratch.MemPool));

                            Str8 FilePath = GraphicalFilePicker(Scratch.MemPool, CurrentPathString);

                            FilePath = PathNormalisedFromStr(Scratch.MemPool, FilePath);

                            if (FilePath.Size) {
                                AppRegistersScope(__Registers.FilePath = FilePath, __Registers.CommandName = ""_s8) {
                                    AppPushCmd(Command->Regs->CommandName, Registers());
                                }

                                AppCmd(
                                    APP_COMMAND_KIND_SET_CURRENT_PATH, 
                                    __Registers.FilePath = StrChopLastSlash(FilePath)
                                );
                            }
                        } else {
                            AppCmd(
                                APP_COMMAND_KIND_PUSH_QUERY,
                                __Registers.DoImplicitRoot = TRUE,
                                __Registers.DoLister = !!Info->Query.Expression.Size,
                                __Registers.Expression = Info->Query.Expression
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_STATE: {} break;

                    case APP_COMMAND_KIND_EVAL: {} break;

                    case APP_COMMAND_KIND_OS_EVENT: {
                        InputEvent* OSEvent = Registers()->OSEvent;

                        if (!OSEvent)
                            break;

                        AppWindowState* WS = AppWindowStateFromHandle(OSEvent->WindowHandle);

                        if (OSEvent && WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                            UIEvent Event = {};
                            UIEventKind EventKind = UI_EVENT_KIND_NULL;

                            switch (OSEvent->Kind) {
                                default: {} break;

                                case EVENT_KIND_PRESS: {
                                    EventKind = UI_EVENT_KIND_PRESS;
                                } break;

                                case EVENT_KIND_RELEASE: {
                                    EventKind = UI_EVENT_KIND_RELEASE;
                                } break;

                                case EVENT_KIND_MOUSE_MOVE: {
                                    EventKind = UI_EVENT_KIND_MOUSE_MOVE;
                                } break;

                                case EVENT_KIND_TEXT: {
                                    EventKind = UI_EVENT_KIND_TEXT;
                                } break;

                                case EVENT_KIND_SCROLL: {
                                    EventKind = UI_EVENT_KIND_SCROLL;
                                } break;

                                case EVENT_KIND_FILE_DROP: {
                                    EventKind = UI_EVENT_KIND_FILE_DROP;
                                } break;
                            }

                            Event.Kind = EventKind;
                            Event.Input = OSEvent->Input;
                            Event.Modifiers = OSEvent->Modifier;

                            if (OSEvent->Kind == EVENT_KIND_TEXT)
                                Event.String = Str8FromStr32(UIBuildMemPool(), Str(&OSEvent->Character, 1));
                            else
                                Event.String = ""_s8;

                            Event.Paths = StrListCopy(Scratch.MemPool, &OSEvent->Strings);
                            Event.Position = OSEvent->Position;
                            Event.DeltaF32 = OSEvent->PositionDelta;
                            Event.TimestampUSecs = OSEvent->TimeStampUSecs;
                            ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                        }
                    } break;

                    case APP_COMMAND_KIND_INC_VIEW_FONT_SIZE:
                    case APP_COMMAND_KIND_INC_WINDOW_FONT_SIZE: {
                        if (Kind == APP_COMMAND_KIND_INC_WINDOW_FONT_SIZE) {
                            Config = ConfigNodeFromID(Registers()->Window);
                            Registers()->View = 0;
                            Registers()->Tab = 0;
                        } else {
                            Config = ConfigNodeFromID(Registers()->View);
                        }

                        if (Config != EMPTY_CFG_NODE_VALUE) {
                            FontReset();

                            f32 CurrentFontSize = AppFontSize();
                            f32 NewFontSize = CurrentFontSize + 1.0f;

                            NewFontSize = CLAMP(6.0f, NewFontSize, 72.0f);

                            ConfigNode* FontSizeConfig = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                Config, 
                                "font_size"_s8
                            );

                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                FontSizeConfig, 
                                (char*) "%f", NewFontSize
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_DEC_VIEW_FONT_SIZE:
                    case APP_COMMAND_KIND_DEC_WINDOW_FONT_SIZE: {
                        if (Kind == APP_COMMAND_KIND_INC_WINDOW_FONT_SIZE) {
                            Config = ConfigNodeFromID(Registers()->Window);
                            Registers()->View = 0;
                            Registers()->Tab = 0;
                        } else {
                            Config = ConfigNodeFromID(Registers()->View);
                        }

                        if (Config != EMPTY_CFG_NODE_VALUE) {
                            FontReset();

                            f32 CurrentFontSize = AppFontSize();
                            f32 NewFontSize = CurrentFontSize - 1.0f;

                            NewFontSize = CLAMP(6.0f, NewFontSize, 72.0f);

                            ConfigNode* FontSizeConfig = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                Config, 
                                "font_size"_s8
                            );

                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                FontSizeConfig, 
                                "%f", 
                                NewFontSize
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_OPEN_WINDOW: {
                        ConfigNode* OldWindow = ConfigNodeFromID(Registers()->Window);
                        ConfigNode* Bucket = OldWindow->Parent;

                        if (Bucket == EMPTY_CFG_NODE_VALUE) {
                            Bucket = ConfigNodeChildFromStr(
                                ConfigNodeRoot(),
                                "session"_s8
                            );
                        }

                        ConfigNode* NewWindow = ConfigNodeNew(
                            APP_STATE->Config,
                            Bucket, 
                            "window"_s8
                        );
                        ConfigNode* Size = ConfigNodeNew(
                            APP_STATE->Config,
                            NewWindow, 
                            "size"_s8
                        );

                        ConfigNodeNew(APP_STATE->Config, Size, "1280");
                        ConfigNodeNew(APP_STATE->Config, Size, "720");

                        for (
                            ConfigNode* OldChild = OldWindow->Head;
                            OldChild != EMPTY_CFG_NODE_VALUE;
                            OldChild = OldChild->Next
                        ) {
                            if (
                                !StrMatch(OldChild->String, "panels"_s8, 0) &&
                                !StrMatch(OldChild->String, "size"_s8, 0) &&
                                !StrMatch(OldChild->String, "pos"_s8, 0) &&
                                !StrMatch(OldChild->String, "monitor"_s8, 0) &&
                                !StrMatch(OldChild->String, "fullscreen"_s8, 0) &&
                                !StrMatch(OldChild->String, "maximised"_s8, 0)
                            ) {
                                ConfigNode* NewChild = ConfigNodeCpy(
                                    APP_STATE->Config,
                                    OldChild
                                );

                                ConfigNodeInsertChild(
                                    APP_STATE->Config,
                                    NewWindow, 
                                    NewWindow->Tail, 
                                    NewChild
                                );
                            }
                        }

                        ConfigNode* Panels = ConfigNodeNew(
                            APP_STATE->Config,
                            NewWindow, 
                            "panels"_s8
                        );

                        ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            Panels, 
                            "selected"_s8
                        );
                    } break;

                    case APP_COMMAND_KIND_WINDOW_SETTINGS: {
                        Str8 Expression = ArenaPushStrFmt(
                            Scratch.MemPool, 
                            "query:config.$%llx", 
                            Registers()->Window
                        );

                        AppCmd(
                            APP_COMMAND_KIND_PUSH_QUERY,
                            __Registers.Expression = Expression,
                            __Registers.DoImplicitRoot = TRUE,
                            __Registers.DoBigRows = TRUE,
                            __Registers.DoLister = TRUE
                        );
                    } break;

                    case APP_COMMAND_KIND_CLOSE_WINDOW: {
                        ConfigNodePtrList AllWindows = ConfigNodeTopLevelListFromStr(
                            Scratch.MemPool,
                            "window"_s8
                        );
                        ConfigNode* WConfig = ConfigNodeFromID(Registers()->Window);

                        if (AllWindows.Count == 1 && AllWindows.Head->V == WConfig)
                            AppCmd(APP_COMMAND_KIND_EXIT);
                        else
                            ConfigNodeRelease(APP_STATE->Config, WConfig);
                    } break;

                    case APP_COMMAND_KIND_TOGGLE_FULLSCREEN: {
                        ConfigNode* WConfig = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(WConfig);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE)
                            WindowSetFullScreen(WS->WinHandle, !WindowIsFullScreen(WS->WinHandle));
                    } break;

                    case APP_COMMAND_KIND_BRING_TO_FRONT: {
                        ConfigNode* LastFocusedWConfig = ConfigNodeFromID(
                            APP_STATE->LastFocusedWindow
                        );
                        AppWindowState* LastFocusedWS = AppWindowStateFromConfig(
                            LastFocusedWConfig
                        );

                        if (LastFocusedWS == EMPTY_APP_WINDOW_STATE_VALUE)
                            LastFocusedWS = APP_STATE->HeadWindowState;

                        if (LastFocusedWS != EMPTY_APP_WINDOW_STATE_VALUE) {
                            WindowSetMinimised(LastFocusedWS->WinHandle, FALSE);
                            WindowFocus(LastFocusedWS->WinHandle);
                        }
                    } break;

                    case APP_COMMAND_KIND_POP_UP_ACCEPT: {
                        APP_STATE->PopUpActive = FALSE;
                        APP_STATE->PopUpKey = {};

                        for (
                            AppCommandNode* N = APP_STATE->PopUpCommands.Head;
                            N;
                            N = N->Next
                        ) {
                            AppPushCmd(N->Command.Name, N->Command.Regs);
                        }
                    } break;

                    case APP_COMMAND_KIND_POP_UP_CANCEL: {
                        APP_STATE->PopUpActive = FALSE;
                        APP_STATE->PopUpKey = {};
                    } break;

                    case APP_COMMAND_KIND_RESET_TO_DEFAULT_BINDINGS: {
                        ConfigNode* Session = ConfigNodeChildFromStr(
                            ConfigNodeRoot(),
                            "session"_s8
                        );
                        ConfigNodePtrList AllInputBindings = ConfigNodeChildListFromStr(
                            Scratch.MemPool,
                            Session,
                            "keybindings"_s8
                        );

                        for (
                            ConfigNodePtrNode* N = AllInputBindings.Head;
                            N;
                            N = N->Next
                        ) {
                            ConfigNodeRelease(APP_STATE->Config, N->V);
                        }

                        ConfigNode* InputBindings = ConfigNodeNew(
                            APP_STATE->Config,
                            Session,
                            "keybindings"_s8
                        );

                        for (
                            u32 Index = 0;
                            Index < ARRAY_COUNT(APP_DEFAULT_BINDING_TABLE);
                            ++Index
                        ) {
                            Str8 Name = APP_DEFAULT_BINDING_TABLE[Index].String;
                            ConfigBinding Binding = APP_DEFAULT_BINDING_TABLE[Index].Binding;
                            ConfigNode* BindingRoot = ConfigNodeNew(
                                APP_STATE->Config,
                                InputBindings,
                                ""_s8
                            );

                            ConfigNodeNew(
                                APP_STATE->Config,
                                BindingRoot,
                                Name
                            );
                            ConfigNodeNew(
                                APP_STATE->Config,
                                BindingRoot,
                                INPUT_DISPLAY_STR_TABLE[Binding.Input]
                            );

                            if (Binding.ModKind & INPUT_MOD_KIND_CTRL) {
                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    BindingRoot,
                                    "ctrl"_s8
                                );
                            }

                            if (Binding.ModKind & INPUT_MOD_KIND_SHIFT) {
                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    BindingRoot,
                                    "shift"_s8
                                );
                            }

                            if (Binding.ModKind & INPUT_MOD_KIND_ALT) {
                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    BindingRoot,
                                    "alt"_s8
                                );
                            }
                        }
                    } break;

                    case APP_COMMAND_KIND_RESET_TO_DEFAULT_PANELS:
                    case APP_COMMAND_KIND_RESET_TO_COMPACT_PANELS:
                    case APP_COMMAND_KIND_RESET_TO_SIMPLE_PANELS: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        ConfigNode* Panels = ConfigNodeChildFromStr(Window, "panels"_s8);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                            Scratch.MemPool, 
                            Window
                        );
                        ConfigNode* GettingStarted = EMPTY_CFG_NODE_VALUE;
                        b32 AnyFixedTabsFound = FALSE;
                        ConfigNodePtrList Texts = {};

                        for (
                            ConfigPanelNode* Panel = PanelTree.Root;
                            Panel != EMPTY_CFG_PANEL_NODE_VALUE;
                            Panel = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, Panel).Next
                        ) {
                            for (
                                ConfigNodePtrNode* N = Panel->Tabs.Head; 
                                N; 
                                N = N->Next
                            ) {
                                ConfigNode* Tab = N->V;
                                b32 NeedUnhook = TRUE;

                                if (StrMatch(Tab->String, "getting_started"_s8, 0)) {
                                    GettingStarted = Tab;
                                } else if (StrMatch(Tab->String, "text"_s8, 0)) {
                                    ListPush(Scratch.MemPool, &Texts, Tab);
                                } else {
                                    NeedUnhook = FALSE;
                                }

                                if (NeedUnhook) {
                                    ConfigNodeUnhook(
                                        APP_STATE->Config,
                                        Panel->Config, 
                                        Tab
                                    );
                                    AnyFixedTabsFound = TRUE;
                                }
                            }
                        }

                        ConfigNodeRelease(APP_STATE->Config, Panels);

                        if (
                            GettingStarted == EMPTY_CFG_NODE_VALUE && 
                            !AnyFixedTabsFound
                        ) {
                            GettingStarted = ConfigNodeAlloc(APP_STATE->Config);
                            ConfigNodeEquipStr(
                                APP_STATE->Config,
                                GettingStarted, 
                                "getting_started"_s8
                            );
                        }

                        if (GettingStarted != EMPTY_CFG_NODE_VALUE) {
                            ConfigNodeRelease(
                                APP_STATE->Config,
                                ConfigNodeChildFromStr(
                                    GettingStarted, 
                                    "selected"_s8
                                )
                            );
                        }

                        for (
                            ConfigNodePtrNode* N = Texts.Head; 
                            N; 
                            N = N->Next
                        ) {
                            ConfigNodeRelease(
                                APP_STATE->Config,
                                ConfigNodeChildFromStr(
                                    N->V, 
                                    "selected"_s8
                                )
                            );
                        }

                        Panels = ConfigNodeNew(
                            APP_STATE->Config, 
                            Window, 
                            "panels"_s8
                        );

                        switch (Kind) {
                            default: {} break;

                            case APP_COMMAND_KIND_RESET_TO_DEFAULT_PANELS: {
                                ConfigNodeChildFromStrOrAlloc(
                                    APP_STATE->Config,
                                    Window, 
                                    "split_x"_s8
                                );

                                ConfigNode* Root0 = ConfigNodeNew(
                                    APP_STATE->Config, 
                                    Panels, 
                                    "100.0"_s8
                                );
                                ConfigNode* MainPanel = Root0;

                                if (GettingStarted != EMPTY_CFG_NODE_VALUE) {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        MainPanel, 
                                        MainPanel->Tail, 
                                        GettingStarted
                                    );
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        GettingStarted, 
                                        "selected"_s8
                                    );
                                } else if (Texts.Head) {
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        Texts.Head->V, 
                                        "selected"_s8
                                    );
                                }

                                for (
                                    ConfigNodePtrNode* N = Texts.Head; 
                                    N; 
                                    N = N->Next
                                ) {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        MainPanel, 
                                        MainPanel->Tail, 
                                        N->V
                                    );
                                }

                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    MainPanel, 
                                    "selected"_s8
                                );
                            } break;

                            case APP_COMMAND_KIND_RESET_TO_COMPACT_PANELS: {
                                ConfigNodeChildFromStrOrAlloc(
                                    APP_STATE->Config,
                                    Window, 
                                    "split_x"_s8
                                );

                                ConfigNode* Root0 = ConfigNodeNew(
                                    APP_STATE->Config,
                                    Panels, 
                                    "100.0"_s8
                                );
                                ConfigNode* MainPanel = Root0;

                                if (GettingStarted != EMPTY_CFG_NODE_VALUE) {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        MainPanel, 
                                        MainPanel->Tail, 
                                        GettingStarted
                                    );
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        GettingStarted, 
                                        "selected"_s8
                                    );
                                } else if (Texts.Head) {
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        Texts.Head->V, 
                                        "selected"_s8
                                    );
                                }

                                for (
                                    ConfigNodePtrNode* N = Texts.Head; 
                                    N; 
                                    N = N->Next
                                ) {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        MainPanel, 
                                        MainPanel->Tail, 
                                        N->V
                                    );
                                }

                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    MainPanel, 
                                    "selected"_s8
                                );
                            } break;

                            case APP_COMMAND_KIND_RESET_TO_SIMPLE_PANELS: {
                                ConfigNodeChildFromStrOrAlloc(
                                    APP_STATE->Config,
                                    Window, 
                                    "split_x"_s8
                                );

                                ConfigNode* Root0 = ConfigNodeNew(
                                    APP_STATE->Config,
                                    Panels, 
                                    "100.0"_s8
                                );
                                ConfigNode* MainPanel = Root0;

                                if (GettingStarted != EMPTY_CFG_NODE_VALUE) {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        MainPanel, 
                                        MainPanel->Tail, 
                                        GettingStarted
                                    );
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        GettingStarted, 
                                        "selected"_s8
                                    );
                                } else if (Texts.Head) {
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        Texts.Head->V, 
                                        "selected"_s8
                                    );
                                }

                                for (
                                    ConfigNodePtrNode* N = Texts.Head; 
                                    N; 
                                    N = N->Next
                                ) {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        MainPanel, 
                                        MainPanel->Tail, 
                                        N->V
                                    );
                                }

                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    MainPanel, 
                                    "selected"_s8
                                );
                            } break;
                        }

                        if (GettingStarted->Parent == EMPTY_CFG_NODE_VALUE) {
                            ConfigNodeRelease(APP_STATE->Config, GettingStarted);
                        }

                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE)
                            WS->WindowLayoutReset = TRUE;
                    } break;

                    case APP_COMMAND_KIND_NEW_PANEL_LEFT:
                    case APP_COMMAND_KIND_NEW_PANEL_UP:
                    case APP_COMMAND_KIND_NEW_PANEL_RIGHT:
                    case APP_COMMAND_KIND_NEW_PANEL_DOWN:
                    case APP_COMMAND_KIND_SPLIT_PANEL: {
                        switch (Kind) {
                            default: {
                                SplitDir = DIR_2D_INVALID;
                            } break;

                            case APP_COMMAND_KIND_NEW_PANEL_LEFT: {
                                SplitDir = DIR_2D_LEFT;
                            } break;

                            case APP_COMMAND_KIND_NEW_PANEL_UP: {
                                SplitDir = DIR_2D_UP;
                            } break;

                            case APP_COMMAND_KIND_NEW_PANEL_RIGHT: {
                                SplitDir = DIR_2D_RIGHT;
                            } break;

                            case APP_COMMAND_KIND_NEW_PANEL_DOWN: {
                                SplitDir = DIR_2D_DOWN;
                            } break;

                            case APP_COMMAND_KIND_SPLIT_PANEL: {
                                SplitDir = Registers()->Direction;
                                SplitPanel = ConfigNodeFromID(Registers()->DstPanel);
                            } break;
                        }

                        if (SplitDir != DIR_2D_INVALID) {
                            Axis2D SplitAxis = Axis2DFromDir2D(SplitDir);
                            SideKind SplitSide = SideFromDir2D(SplitDir);

                            if (SplitPanel == EMPTY_CFG_NODE_VALUE)
                                SplitPanel = ConfigNodeFromID(Registers()->Panel);

                            ConfigNode* NewPanelConfig = EMPTY_CFG_NODE_VALUE;
                            ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                                Scratch.MemPool, 
                                SplitPanel
                            );
                            ConfigPanelNode* PanelRoot = PanelTree.Root;
                            ConfigPanelNode* Panel = ConfigPanelNodeFromConfigTree(
                                PanelRoot, 
                                SplitPanel
                            );
                            ConfigPanelNode* Parent = Panel->Parent;

                            if (
                                Parent != EMPTY_CFG_PANEL_NODE_VALUE && 
                                Parent->SplitAxis == SplitAxis
                            ) {
                                ConfigNode* ParentConfig = Parent->Config;
                                ConfigNode* PanelConfig = Panel->Config;
                                ConfigNode* NewConfig = ConfigNodeAlloc(APP_STATE->Config);

                                ConfigNodeInsertChild(
                                    APP_STATE->Config,
                                    ParentConfig, 
                                    (SplitSide == SIDE_MAX) ? PanelConfig : PanelConfig->Prev, 
                                    NewConfig
                                );
                                ConfigNodeEquipStr(
                                    APP_STATE->Config,
                                    NewConfig, 
                                    "%f",
                                    1.0f / (Parent->ChildCount + 1)
                                );

                                for (
                                    ConfigPanelNode* Child = Parent->Head;
                                    Child != EMPTY_CFG_PANEL_NODE_VALUE;
                                    Child = Child->Next
                                ) {
                                    f32 OldPercent = Child->PercentOfParent;
                                    f32 NewPercent = OldPercent * ((f32) (Parent->ChildCount) / (Parent->ChildCount + 1));

                                    ConfigNodeEquipStr(
                                        APP_STATE->Config,
                                        Child->Config, 
                                        "%f", 
                                        NewPercent
                                    );
                                }

                                NewPanelConfig = NewConfig;
                            } else {
                                ConfigNode* SplitPanelPrev = Panel->Prev->Config;
                                ConfigNode* NewParent = ConfigNodeAlloc(APP_STATE->Config);
                                ConfigNode* NewSibling = ConfigNodeAlloc(APP_STATE->Config);

                                ConfigNodeEquipStr(
                                    APP_STATE->Config, 
                                    NewParent, 
                                    SplitPanel->String
                                );
                                ConfigNodeEquipStr(
                                    APP_STATE->Config, 
                                    SplitPanel, 
                                    "0.5"_s8
                                );
                                ConfigNodeEquipStr(
                                    APP_STATE->Config, 
                                    NewSibling, 
                                    "0.5"_s8
                                );

                                if (Parent->Config != EMPTY_CFG_NODE_VALUE) {
                                    ConfigNodeUnhook(
                                        APP_STATE->Config, 
                                        Parent->Config, 
                                        SplitPanel
                                    );
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        Parent->Config, 
                                        SplitPanelPrev, 
                                        NewParent
                                    );
                                } else {
                                    ConfigNodeEquipStr(
                                        APP_STATE->Config,
                                        NewParent, 
                                        "panels"_s8
                                    );

                                    ConfigNode* WindowConfig = AppWindowFromConfig(
                                        SplitPanel
                                    );

                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        WindowConfig, 
                                        WindowConfig->Tail, 
                                        NewParent
                                    );

                                    if (SplitAxis == AXIS_2D_X) {
                                        ConfigNodeChildFromStrOrAlloc(
                                            APP_STATE->Config,
                                            WindowConfig, 
                                            "split_x"_s8
                                        );
                                    } else {
                                        ConfigNodeRelease(
                                            APP_STATE->Config,
                                            ConfigNodeChildFromStr(
                                                WindowConfig, 
                                                "split_x"_s8
                                            )
                                        );
                                    }
                                }

                                ConfigNode* Min = SplitPanel;
                                ConfigNode* Max = NewSibling;

                                if (SplitSide == SIDE_MIN)
                                    SWAP(ConfigNode*, Min, Max);

                                ConfigNodeInsertChild(
                                    APP_STATE->Config,
                                    NewParent, 
                                    NewParent->Tail, 
                                    Min
                                );
                                ConfigNodeInsertChild(
                                    APP_STATE->Config,
                                    NewParent, 
                                    NewParent->Tail, 
                                    Max
                                );
                                NewPanelConfig = NewSibling;
                            }

                            AppWindowState* WS = AppWindowStateFromConfig(NewPanelConfig);

                            if (WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                                UISelectState(WS->UI);

                                ConfigPanelTree NewPanelTree = ConfigPanelTreeFromConfig(
                                    Scratch.MemPool, 
                                    NewPanelConfig
                                );
                                ConfigPanelNode* NewPanel = ConfigPanelNodeFromConfigTree(
                                    NewPanelTree.Root, 
                                    NewPanelConfig
                                );
                                r2f32 StubContentRect = Rng(
                                    0.0f, 
                                    0.0f, 
                                    1000.0f, 
                                    1000.0f
                                );
                                v2f32 StubContentRectDim = Length(StubContentRect);
                                r2f32 NewRectPX = ConfigTargetRectFromPanelNode(
                                    StubContentRect, 
                                    NewPanelTree.Root, 
                                    NewPanel
                                );
                                r2f32 NewRectPercent = Rng(
                                    NewRectPX.X0 / StubContentRectDim.X,
                                    NewRectPX.Y0 / StubContentRectDim.Y,
                                    NewRectPX.X1 / StubContentRectDim.X,
                                    NewRectPX.Y1 / StubContentRectDim.Y
                                );

                                if (NewPanel->Prev != EMPTY_CFG_PANEL_NODE_VALUE) {
                                    r2f32 TargetPrevRectPX = ConfigTargetRectFromPanelNode(
                                        StubContentRect, 
                                        PanelTree.Root, 
                                        ConfigPanelNodeFromConfigTree(
                                            PanelTree.Root, 
                                            NewPanel->Prev->Config
                                        )
                                    );
                                    r2f32 TargetPrevRectPercent = Rng(
                                        TargetPrevRectPX.X0 / StubContentRectDim.X,
                                        TargetPrevRectPX.Y0 / StubContentRectDim.Y,
                                        TargetPrevRectPX.X1 / StubContentRectDim.X,
                                        TargetPrevRectPX.Y1 / StubContentRectDim.Y
                                    );
                                    r2f32 PrevRectPercent = Rng(
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {}, 
                                                "panel_%p_x0", 
                                                NewPanel->Prev->Config
                                            ), 
                                            TargetPrevRectPercent.X0, 
                                            __Params.Initial = TargetPrevRectPercent.X0,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        ),
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {}, 
                                                "panel_%p_y0", 
                                                NewPanel->Prev->Config
                                            ), 
                                            TargetPrevRectPercent.Y0, 
                                            __Params.Initial = TargetPrevRectPercent.Y0,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        ),
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {}, 
                                                "panel_%p_x1", 
                                                NewPanel->Prev->Config
                                            ), 
                                            TargetPrevRectPercent.X1, 
                                            __Params.Initial = TargetPrevRectPercent.X1,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        ),
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {}, 
                                                "panel_%p_y1", 
                                                NewPanel->Prev->Config
                                            ), 
                                            TargetPrevRectPercent.Y1, 
                                            __Params.Initial = TargetPrevRectPercent.Y1,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        )
                                    );

                                    NewRectPercent = PrevRectPercent;
                                    NewRectPercent.Point0.V[SplitAxis] = NewRectPercent.Point1.V[SplitAxis];
                                }

                                if (NewPanel->Next != EMPTY_CFG_PANEL_NODE_VALUE) {
                                    r2f32 TargetNextRectPX = ConfigTargetRectFromPanelNode(
                                        StubContentRect, 
                                        PanelTree.Root,
                                        ConfigPanelNodeFromConfigTree(
                                            PanelTree.Root,
                                            NewPanel->Next->Config
                                        )
                                    );
                                    r2f32 TargetNextRectPercent = Rng(
                                        TargetNextRectPX.X0 / StubContentRectDim.X,
                                        TargetNextRectPX.Y0 / StubContentRectDim.Y,
                                        TargetNextRectPX.X1 / StubContentRectDim.X,
                                        TargetNextRectPX.Y1 / StubContentRectDim.Y
                                    );
                                    r2f32 NextRectPercent = Rng(
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {},
                                                "panel_%p_x0",
                                                NewPanel->Next->Config
                                            ),
                                            TargetNextRectPercent.X0,
                                            __Params.Initial = TargetNextRectPercent.X0,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        ),
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {},
                                                "panel_%p_y0",
                                                NewPanel->Next->Config
                                            ),
                                            TargetNextRectPercent.Y0,
                                            __Params.Initial = TargetNextRectPercent.Y0,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        ),
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {},
                                                "panel_%p_x1",
                                                NewPanel->Next->Config
                                            ),
                                            TargetNextRectPercent.X1,
                                            __Params.Initial = TargetNextRectPercent.X1,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        ),
                                        UIAnimate(
                                            UIKeyFromStrFmt(
                                                {},
                                                "panel_%p_y1",
                                                NewPanel->Next->Config
                                            ),
                                            TargetNextRectPercent.Y1,
                                            __Params.Initial = TargetNextRectPercent.Y1,
                                            __Params.Rate = APP_STATE->MenuAnimationRate
                                        )
                                    );

                                    NewRectPercent = NextRectPercent;
                                    NewRectPercent.Point1.V[SplitAxis] = NewRectPercent.Point0.V[SplitAxis];
                                }

                                UIAnimate(
                                    UIKeyFromStrFmt(
                                        {},
                                        "panel_%p_x0",
                                        NewPanel->Config
                                    ),
                                    NewRectPercent.X0,
                                    __Params.Initial = NewRectPercent.X0,
                                    __Params.Reset = TRUE,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                );
                                UIAnimate(
                                    UIKeyFromStrFmt(
                                        {},
                                        "panel_%p_x1",
                                        NewPanel->Config
                                    ),
                                    NewRectPercent.X1,
                                    __Params.Initial = NewRectPercent.X1,
                                    __Params.Reset = TRUE,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                );
                                UIAnimate(
                                    UIKeyFromStrFmt(
                                        {},
                                        "panel_%p_y0",
                                        NewPanel->Config
                                    ),
                                    NewRectPercent.Y0,
                                    __Params.Initial = NewRectPercent.Y0,
                                    __Params.Reset = TRUE,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                );
                                UIAnimate(
                                    UIKeyFromStrFmt(
                                        {},
                                        "panel_%p_y1",
                                        NewPanel->Config
                                    ),
                                    NewRectPercent.Y1,
                                    __Params.Initial = NewRectPercent.Y1,
                                    __Params.Reset = TRUE,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                );
                            }

                            ConfigNode* DragDropOriginPanelConfig = ConfigNodeFromID(
                                Registers()->Panel
                            );
                            ConfigNode* DragDropTab = ConfigNodeFromID(
                                Registers()->View
                            );

                            if (
                                Kind == APP_COMMAND_KIND_SPLIT_PANEL &&
                                NewPanelConfig != EMPTY_CFG_NODE_VALUE &&
                                DragDropTab != EMPTY_CFG_NODE_VALUE &&
                                DragDropOriginPanelConfig != EMPTY_CFG_NODE_VALUE
                            ) {
                                ConfigNodeUnhook(
                                    APP_STATE->Config,
                                    DragDropOriginPanelConfig, 
                                    DragDropTab
                                );
                                ConfigNodeInsertChild(
                                    APP_STATE->Config,
                                    NewPanelConfig, 
                                    NewPanelConfig->Tail, 
                                    DragDropTab
                                );

                                ConfigPanelTree OriginPanelTree = ConfigPanelTreeFromConfig(
                                    Scratch.MemPool, 
                                    DragDropOriginPanelConfig
                                );
                                ConfigPanelNode* OriginPanel = ConfigPanelNodeFromConfigTree(
                                    OriginPanelTree.Root, 
                                    DragDropOriginPanelConfig
                                );

                                if (OriginPanel->SelectedTab == EMPTY_CFG_NODE_VALUE) {
                                    for (
                                        ConfigNodePtrNode* N = OriginPanel->Tabs.Head;
                                        N;
                                        N = N->Next
                                    ) {
                                        if (!AppIsProjectFiltered(N->V)) {
                                            AppCmd(
                                                APP_COMMAND_KIND_FOCUS_TAB,
                                                __Registers.Panel = OriginPanel->Config->ID,
                                                __Registers.Tab = N->V->ID
                                            );
                                            break;
                                        }
                                    }
                                }

                                if (OriginPanel->Config != SplitPanel && !OriginPanel->Tabs.Count)
                                    AppCmd(APP_COMMAND_KIND_CLOSE_PANEL);

                                AppCmd(
                                    APP_COMMAND_KIND_FOCUS_TAB,
                                    __Registers.Panel = NewPanelConfig->ID,
                                    __Registers.Tab = DragDropTab->ID
                                );
                            }

                            if (NewPanelConfig != EMPTY_CFG_NODE_VALUE) {
                                AppCmd(
                                    APP_COMMAND_KIND_FOCUS_PANEL, 
                                    __Registers.Panel = NewPanelConfig->ID
                                );
                            }

                            if (
                                Panel->TabSide == SIDE_MAX &&
                                SplitAxis == AXIS_2D_X
                            ) {
                                AppCmd(
                                    APP_COMMAND_KIND_TAB_BAR_BOTTOM,
                                    __Registers.Panel = NewPanelConfig->ID
                                );
                            }
                        }

                        ConfigNode* Window = ConfigNodeFromID(
                            Registers()->Window
                        );
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE)
                            WS->WindowLayoutReset = TRUE;
                    } break;

                    case APP_COMMAND_KIND_ROTATE_PANEL_COLUMNS: {
                        ConfigNode* PanelConfig = ConfigNodeFromID(
                            Registers()->Panel
                        );
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                            Scratch.MemPool, 
                            PanelConfig
                        );
                        ConfigPanelNode* Panel = ConfigPanelNodeFromConfigTree(
                            PanelTree.Root, 
                            PanelConfig
                        );
                        ConfigPanelNode* Parent = EMPTY_CFG_PANEL_NODE_VALUE;

                        for (
                            ConfigPanelNode* P = Panel->Parent;
                            P != EMPTY_CFG_PANEL_NODE_VALUE;
                            P = P->Parent
                        ) {
                            if (P->SplitAxis == AXIS_2D_X) {
                                Parent = P;
                                break;
                            }
                        }

                        if (
                            Parent != EMPTY_CFG_PANEL_NODE_VALUE &&
                            Parent->ChildCount > 1
                        ) {
                            ConfigNode* Rotated = Parent->Head->Config;

                            ConfigNodeUnhook(
                                APP_STATE->Config,
                                Parent->Config, 
                                Parent->Head->Config
                            );
                            ConfigNodeInsertChild(
                                APP_STATE->Config,
                                Parent->Config, 
                                Parent->Tail->Config, 
                                Rotated
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_NEXT_PANEL:
                    case APP_COMMAND_KIND_PREV_PANEL: {
                        if (Kind == APP_COMMAND_KIND_NEXT_PANEL) {
                            PanelSiblingOffset = OFFSETOF(ConfigPanelNode, Next);
                            PanelChildOffset = OFFSETOF(ConfigPanelNode, Head);
                        } else {
                            PanelSiblingOffset = OFFSETOF(ConfigPanelNode, Prev);
                            PanelChildOffset = OFFSETOF(ConfigPanelNode, Tail);
                        }

                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                            Scratch.MemPool, 
                            ConfigNodeFromID(Registers()->Window)
                        );
                        ConfigPanelNode* NextFocused = EMPTY_CFG_PANEL_NODE_VALUE;

                        for (
                            ConfigPanelNode* P = PanelTree.Focused;
                            P != EMPTY_CFG_PANEL_NODE_VALUE;
                            P = ConfigPanelNodeRecDepthFirst(
                                PanelTree.Root, 
                                P, 
                                PanelSiblingOffset, 
                                PanelChildOffset
                            ).Next
                        ) {
                            if (
                                P != PanelTree.Focused && 
                                P->Head == EMPTY_CFG_PANEL_NODE_VALUE
                            ) {
                                NextFocused = P;
                                break;
                            }
                        }

                        if (NextFocused == EMPTY_CFG_PANEL_NODE_VALUE) {
                            for (
                                ConfigPanelNode* P = PanelTree.Root;
                                P != EMPTY_CFG_PANEL_NODE_VALUE;
                                P = ConfigPanelNodeRecDepthFirst(
                                    PanelTree.Root, 
                                    P, 
                                    PanelSiblingOffset, 
                                    PanelChildOffset
                                ).Next
                            ) {
                                if (
                                    P != PanelTree.Focused && 
                                    P->Head == EMPTY_CFG_PANEL_NODE_VALUE
                                ) {
                                    NextFocused = P;
                                    break;
                                }
                            }
                        }

                        AppCmd(
                            APP_COMMAND_KIND_FOCUS_PANEL, 
                            __Registers.Panel = NextFocused->Config->ID
                        );
                    } break;

                    case APP_COMMAND_KIND_FOCUS_PANEL: {
                        ConfigNode* Panel = ConfigNodeFromID(Registers()->Panel);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                            Scratch.MemPool, 
                            Panel
                        );
                        ConfigNode* SelectionConfig = EMPTY_CFG_NODE_VALUE;

                        for (
                            ConfigPanelNode* P = PanelTree.Root;
                            P != EMPTY_CFG_PANEL_NODE_VALUE;
                            P = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, P).Next
                        ) {
                            ConfigNode* PConfig = P->Config;
                            ConfigNode* PSelection = ConfigNodeChildFromStr(
                                PConfig, 
                                "selected"_s8
                            );

                            if (SelectionConfig == EMPTY_CFG_NODE_VALUE) {
                                SelectionConfig = PSelection;
                            } else {
                                for (
                                    ConfigNode* S = PSelection; 
                                    S != EMPTY_CFG_NODE_VALUE; 
                                    S = ConfigNodeChildFromStr(
                                        PConfig, 
                                        "selected"_s8
                                    )
                                ) {
                                    ConfigNodeRelease(APP_STATE->Config, S);
                                }
                            }
                        }

                        if (SelectionConfig == EMPTY_CFG_NODE_VALUE) {
                            SelectionConfig = ConfigNodeAlloc(APP_STATE->Config);
                            ConfigNodeEquipStr(
                                APP_STATE->Config,
                                SelectionConfig, 
                                "selected"_s8
                            );
                        }

                        if (Panel != EMPTY_CFG_NODE_VALUE) {
                            ConfigNodeInsertChild(
                                APP_STATE->Config,
                                Panel, 
                                EMPTY_CFG_NODE_VALUE, 
                                SelectionConfig
                            );

                            ConfigNode* Window = AppWindowFromConfig(Panel);
                            AppWindowState* WS = AppWindowStateFromConfig(Window);

                            WS->MenuBarFocused = FALSE;
                        }
                    } break;

                    case APP_COMMAND_KIND_FOCUS_PANEL_RIGHT:
                    case APP_COMMAND_KIND_FOCUS_PANEL_LEFT:
                    case APP_COMMAND_KIND_FOCUS_PANEL_UP:
                    case APP_COMMAND_KIND_FOCUS_PANEL_DOWN: {
                        if (Kind == APP_COMMAND_KIND_FOCUS_PANEL_RIGHT)
                            PanelChangeDirection = Vec(1, 0);
                        else if (Kind == APP_COMMAND_KIND_FOCUS_PANEL_LEFT)
                            PanelChangeDirection = Vec(-1, 0);
                        else if (Kind == APP_COMMAND_KIND_FOCUS_PANEL_UP)
                            PanelChangeDirection = Vec(0, -1);
                        else
                            PanelChangeDirection = Vec(0, 1);

                        ConfigNode* Window = ConfigNodeFromID(
                            Registers()->Window
                        );
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Window);
                        ConfigPanelNode* SourcePanel = PanelTree.Focused;
                        r2f32 SourcePanelRect = ConfigTargetRectFromPanelNode(
                            Rng(
                                Vec(0.0f, 0.0f), 
                                Vec(1000.0f, 1000.0f)
                            ),
                            PanelTree.Root, 
                            SourcePanel
                        );
                        v2f32 SourcePanelCentre = Centre(SourcePanelRect);
                        v2f32 SourcePanelHalfDim = Length(SourcePanelRect) * 0.5f;
                        v2f32 TravelDim = SourcePanelHalfDim + Vec(10.0f, 10.0f);
                        v2f32 TravelDistance = (
                            SourcePanelCentre + 
                            (
                                TravelDim * Vec(
                                    (f32) PanelChangeDirection.X, 
                                    (f32) PanelChangeDirection.Y
                                )
                            )
                        );
                        ConfigPanelNode* DestinationRoot = EMPTY_CFG_PANEL_NODE_VALUE;

                        for (
                            ConfigPanelNode* P = PanelTree.Root;
                            P != EMPTY_CFG_PANEL_NODE_VALUE;
                            P = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, P).Next
                        ) {
                            if (P == SourcePanel || P->Head != EMPTY_CFG_PANEL_NODE_VALUE)
                                continue;

                            r2f32 PRect = ConfigTargetRectFromPanelNode(
                                Rng(
                                    Vec(0.0f, 0.0f), 
                                    Vec(1000.0f, 1000.0f)
                                ),
                                PanelTree.Root, 
                                P
                            );

                            if (InRange(PRect, TravelDistance)) {
                                DestinationRoot = P;
                                break;
                            }
                        }

                        if (DestinationRoot != EMPTY_CFG_PANEL_NODE_VALUE) {
                            ConfigPanelNode* DestinationPanel = EMPTY_CFG_PANEL_NODE_VALUE;

                            for (
                                ConfigPanelNode* P = DestinationRoot;
                                P != EMPTY_CFG_PANEL_NODE_VALUE;
                                P = ConfigPanelNodeRecDepthFirstPre(DestinationRoot, P).Next
                            ) {
                                if (P->Head == EMPTY_CFG_PANEL_NODE_VALUE && P != SourcePanel) {
                                    DestinationPanel = P;
                                    break;
                                }
                            }

                            AppCmd(
                                APP_COMMAND_KIND_FOCUS_PANEL,
                                __Registers.Panel = DestinationPanel->Config->ID
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_UNDO: {} break;
                    case APP_COMMAND_KIND_REDO: {} break;

                    case APP_COMMAND_KIND_GO_BACK: {} break;
                    case APP_COMMAND_KIND_GO_FORWARD: {} break;

                    case APP_COMMAND_KIND_CLOSE_PANEL: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Window);
                        ConfigPanelNode* Panel = ConfigPanelNodeFromConfigTree(
                            PanelTree.Root, 
                            ConfigNodeFromID(Registers()->Panel)
                        );
                        ConfigPanelNode* Parent = Panel->Parent;

                        if (Parent != EMPTY_CFG_PANEL_NODE_VALUE) {
                            Axis2D SplitAxis = Parent->SplitAxis;

                            if (Parent->ChildCount == 2) {
                                ConfigPanelNode* DiscardChild = Panel;
                                ConfigPanelNode* KeepChild = (Panel == Parent->Head) ? Parent->Tail : Parent->Head;
                                ConfigPanelNode* Grandparent = Parent->Parent;
                                ConfigPanelNode* ParentPrev = Parent->Prev;
                                f32 PercentOfParent = Parent->PercentOfParent;

                                ConfigNodeUnhook(
                                    APP_STATE->Config,
                                    Parent->Config, 
                                    KeepChild->Config
                                );

                                if (Grandparent != EMPTY_CFG_PANEL_NODE_VALUE) {
                                    ConfigNodeUnhook(
                                        APP_STATE->Config,
                                        Grandparent->Config, 
                                        Parent->Config
                                    );
                                } else {
                                    ConfigNodeUnhook(
                                        APP_STATE->Config,
                                        Window, 
                                        Parent->Config
                                    );
                                }

                                ConfigNodeRelease(
                                    APP_STATE->Config, 
                                    Parent->Config
                                );

                                if (Grandparent == EMPTY_CFG_PANEL_NODE_VALUE) {
                                    if (KeepChild->SplitAxis == AXIS_2D_X) {
                                        ConfigNodeChildFromStrOrAlloc(
                                            APP_STATE->Config,
                                            Window, 
                                            "split_x"_s8
                                        );
                                    } else {
                                        ConfigNodeRelease(
                                            APP_STATE->Config,
                                            ConfigNodeChildFromStr(
                                                Window, 
                                                "split_x"_s8
                                            )
                                        );
                                    }

                                    ConfigNodeEquipStr(
                                        APP_STATE->Config,
                                        KeepChild->Config, 
                                        "panels"_s8
                                    );
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        Window, 
                                        Window->Tail, 
                                        KeepChild->Config
                                    );
                                } else {
                                    ConfigNodeInsertChild(
                                        APP_STATE->Config,
                                        Grandparent->Config, 
                                        ParentPrev->Config, 
                                        KeepChild->Config
                                    );
                                    ConfigNodeEquipStr(
                                        APP_STATE->Config,
                                        KeepChild->Config, 
                                        "%f", 
                                        PercentOfParent
                                    );
                                }

                                if (
                                    Grandparent != EMPTY_CFG_PANEL_NODE_VALUE &&
                                    Grandparent->SplitAxis == KeepChild->SplitAxis &&
                                    KeepChild->Head != EMPTY_CFG_PANEL_NODE_VALUE
                                ) {
                                    ConfigNodeUnhook(
                                        APP_STATE->Config,
                                        Grandparent->Config, 
                                        KeepChild->Config
                                    );

                                    ConfigNode* Prev = ParentPrev->Config;

                                    for (
                                        ConfigPanelNode* Child = KeepChild->Head, *Next = EMPTY_CFG_PANEL_NODE_VALUE;
                                        Child != EMPTY_CFG_PANEL_NODE_VALUE;
                                        Child = Next
                                    ) {
                                        Next = Child->Next;
                                        ConfigNodeUnhook(
                                            APP_STATE->Config,
                                            KeepChild->Config, 
                                            Child->Config
                                        );
                                        ConfigNodeInsertChild(
                                            APP_STATE->Config,
                                            Grandparent->Config, 
                                            Prev, 
                                            Child->Config
                                        );
                                        Prev = Child->Config;

                                        f32 OldPercent = Child->PercentOfParent;
                                        f32 NewPercent = OldPercent * PercentOfParent;

                                        ConfigNodeEquipStr(
                                            APP_STATE->Config,
                                            Child->Config, 
                                            "%f", 
                                            NewPercent
                                        );
                                    }

                                    ConfigNodeRelease(
                                        APP_STATE->Config,
                                        KeepChild->Config
                                    );
                                }

                                if (PanelTree.Focused == DiscardChild) {
                                    ConfigPanelTree NewPanelTree = ConfigPanelTreeFromConfig(
                                        Scratch.MemPool, 
                                        Window
                                    );
                                    ConfigPanelNode* NewFocused = ConfigPanelNodeFromConfigTree(
                                        PanelTree.Root, 
                                        KeepChild->Config
                                    );

                                    for (
                                        ConfigPanelNode* GrandChild = NewFocused;
                                        GrandChild != EMPTY_CFG_PANEL_NODE_VALUE;
                                        GrandChild = GrandChild->Head
                                    ) {
                                        NewFocused = GrandChild;
                                    }

                                    AppCmd(
                                        APP_COMMAND_KIND_FOCUS_PANEL,
                                        __Registers.Panel = NewFocused->Config->ID
                                    );
                                }
                            } else {
                                ConfigPanelNode* Next = EMPTY_CFG_PANEL_NODE_VALUE;
                                f32 RemovedSizePercent = Panel->PercentOfParent;

                                if (Next == EMPTY_CFG_PANEL_NODE_VALUE)
                                    Next = Panel->Prev;

                                if (Next == EMPTY_CFG_PANEL_NODE_VALUE)
                                    Next = Panel->Next;

                                ConfigNodeUnhook(
                                    APP_STATE->Config,
                                    Parent->Config, 
                                    Panel->Config
                                );
                                ConfigNodeRelease(
                                    APP_STATE->Config,
                                    Panel->Config
                                );

                                ConfigPanelTree NewPanelTree = ConfigPanelTreeFromConfig(
                                    Scratch.MemPool, 
                                    Window
                                );
                                ConfigPanelNode* NewParent = ConfigPanelNodeFromConfigTree(
                                    NewPanelTree.Root, 
                                    Parent->Config
                                );

                                for (
                                    ConfigPanelNode* Child = NewParent->Head;
                                    Child != EMPTY_CFG_PANEL_NODE_VALUE;
                                    Child = Child->Next
                                ) {
                                    ConfigNode* C = Child->Config;
                                    f32 OldPercent = Child->PercentOfParent;
                                    f32 NewPercent = OldPercent / (1.0f - RemovedSizePercent);

                                    ConfigNodeEquipStr(
                                        APP_STATE->Config,
                                        C, 
                                        "%f", 
                                        NewPercent
                                    );
                                }

                                if (PanelTree.Focused == Panel) {
                                    ConfigPanelTree NewPanelTree = ConfigPanelTreeFromConfig(
                                        Scratch.MemPool, 
                                        Window
                                    );
                                    ConfigPanelNode* NewFocused = ConfigPanelNodeFromConfigTree(
                                        NewPanelTree.Root, 
                                        Next->Config
                                    );

                                    for (
                                        ConfigPanelNode* Grandchild = NewFocused;
                                        Grandchild != EMPTY_CFG_PANEL_NODE_VALUE;
                                        Grandchild = Grandchild->Head
                                    ) {
                                        NewFocused = Grandchild;
                                    }

                                    AppCmd(
                                        APP_COMMAND_KIND_FOCUS_PANEL,
                                        __Registers.Panel = NewFocused->Config->ID
                                    );
                                }
                            }
                        }

                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE)
                            WS->WindowLayoutReset = TRUE;

                    } break;

                    case APP_COMMAND_KIND_FOCUS_TAB: {
                        ConfigNode* Tab = ConfigNodeFromID(Registers()->Tab);
                        ConfigNode* Panel = Tab->Parent;
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                            Scratch.MemPool, 
                            Panel
                        );
                        ConfigPanelNode* PanelNode = ConfigPanelNodeFromConfigTree(
                            PanelTree.Root, 
                            Panel
                        );
                        ConfigNode* SelectionConfig = EMPTY_CFG_NODE_VALUE;

                        for (
                            ConfigNodePtrNode* N = PanelNode->Tabs.Head;
                            N;
                            N = N->Next
                        ) {
                            ConfigNode* TabSelectionConfig = ConfigNodeChildFromStr(
                                N->V, 
                                "selected"_s8
                            );

                            if (SelectionConfig == EMPTY_CFG_NODE_VALUE) {
                                SelectionConfig = TabSelectionConfig;
                            } else {
                                for (
                                    ConfigNode* S = TabSelectionConfig;
                                    S != EMPTY_CFG_NODE_VALUE;
                                    S = ConfigNodeChildFromStr(N->V, "selected"_s8)
                                ) {
                                    ConfigNodeRelease(APP_STATE->Config, S);
                                }
                            }
                        }

                        if (SelectionConfig == EMPTY_CFG_NODE_VALUE) {
                            SelectionConfig = ConfigNodeAlloc(APP_STATE->Config);
                            ConfigNodeEquipStr(
                                APP_STATE->Config, 
                                SelectionConfig, 
                                "selected"_s8
                            );
                        }

                        ConfigNodeInsertChild(
                            APP_STATE->Config,
                            Tab, 
                            EMPTY_CFG_NODE_VALUE, 
                            SelectionConfig
                        );
                    } break;

                    case APP_COMMAND_KIND_NEXT_TAB: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(
                            Scratch.MemPool, 
                            Window
                        );
                        ConfigPanelNode* Focused = PanelTree.Focused;
                        ConfigNodePtrNode* SelectedTabNode = NULL;

                        for (
                            ConfigNodePtrNode* N = Focused->Tabs.Head; 
                            N; 
                            N = N->Next
                        ) {
                            if (N->V == Focused->SelectedTab) {
                                SelectedTabNode = N;
                                break;
                            }
                        }

                        ConfigNode* NextSelectedTab = EMPTY_CFG_NODE_VALUE;
                        u64 Index = 0;

                        for (
                            ConfigNodePtrNode* TabNode = SelectedTabNode;
                            TabNode && (TabNode != SelectedTabNode || !Index);
                            (
                                (!TabNode->Next)
                                ? TabNode = Focused->Tabs.Head
                                : TabNode = TabNode->Next
                            ), ++Index
                        ) {
                            if (
                                !AppIsProjectFiltered(TabNode->V) && 
                                TabNode != SelectedTabNode
                            ) {
                                NextSelectedTab = TabNode->V;
                                break;
                            }
                        }

                        if (NextSelectedTab != EMPTY_CFG_NODE_VALUE)
                            AppCmd(APP_COMMAND_KIND_FOCUS_TAB, __Registers.Tab = NextSelectedTab->ID);
                    } break;

                    case APP_COMMAND_KIND_PREV_TAB: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Window);
                        ConfigPanelNode* Focused = PanelTree.Focused;
                        ConfigNodePtrNode* SelectedTabNode = NULL;

                        for (ConfigNodePtrNode* N = Focused->Tabs.Tail; N; N = N->Prev) {
                            if (N->V == Focused->SelectedTab) {
                                SelectedTabNode = N;
                                break;
                            }
                        }

                        ConfigNode* NextSelectedTab = EMPTY_CFG_NODE_VALUE;
                        u64 Index = 0;

                        for (
                            ConfigNodePtrNode* TabNode = SelectedTabNode;
                            TabNode && (TabNode != SelectedTabNode || !Index);
                            (
                                (!TabNode->Prev)
                                ? TabNode = Focused->Tabs.Tail
                                : TabNode = TabNode->Prev
                            ), ++Index
                        ) {
                            if (
                                !AppIsProjectFiltered(TabNode->V) &&
                                TabNode != SelectedTabNode
                            ) {
                                NextSelectedTab = TabNode->V;
                                break;
                            }
                        }

                        if (NextSelectedTab != EMPTY_CFG_NODE_VALUE)
                            AppCmd(APP_COMMAND_KIND_FOCUS_TAB, __Registers.Tab = NextSelectedTab->ID);
                    } break;

                    case APP_COMMAND_KIND_MOVE_TAB_RIGHT:
                    case APP_COMMAND_KIND_MOVE_TAB_LEFT: {
                        ConfigNode* Tab = ConfigNodeFromID(Registers()->Tab);
                        ConfigNode* Window = AppWindowFromConfig(Tab);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Window);
                        ConfigPanelNode* Panel = ConfigPanelNodeFromConfigTree(PanelTree.Root, Tab->Parent);
                        ConfigNodePtrList FilteredTabs = {};

                        for (
                            ConfigNodePtrNode* N = Panel->Tabs.Head;
                            N;
                            N = N->Next
                        ) {
                            if (AppIsProjectFiltered(N->V))
                                continue;

                            ListPush(Scratch.MemPool, &FilteredTabs, N->V);
                        }

                        ConfigNode* TabPrev2 = EMPTY_CFG_NODE_VALUE;
                        ConfigNode* TabPrev = EMPTY_CFG_NODE_VALUE;
                        ConfigNode* TabNext = EMPTY_CFG_NODE_VALUE;
                        ConfigNode* Prev2 = EMPTY_CFG_NODE_VALUE;
                        ConfigNode* Prev = EMPTY_CFG_NODE_VALUE;
                        ConfigNode* Next = EMPTY_CFG_NODE_VALUE;

                        for (
                            ConfigNodePtrNode* N = FilteredTabs.Head;
                            N;
                            (
                                Prev2 = Prev,
                                Prev = N->V,
                                N = N->Next
                            )
                        ) {
                            Next = (N->Next) ? N->Next->V : EMPTY_CFG_NODE_VALUE;

                            if (N->V == Tab) {
                                TabPrev2 = Prev2;
                                TabPrev = Prev;
                                TabNext = Next;
                                break;
                            }
                        }

                        ConfigNode* NewPrev = (Kind == APP_COMMAND_KIND_MOVE_TAB_RIGHT)
                            ? TabNext
                            : TabPrev2;

                        if (NewPrev == TabPrev && FilteredTabs.Tail)
                            NewPrev = FilteredTabs.Tail->V;

                        AppCmd(
                            APP_COMMAND_KIND_MOVE_VIEW,
                            __Registers.DstPanel = Panel->Config->ID,
                            __Registers.View = Tab->ID,
                            __Registers.PrevTab = NewPrev->ID
                        );
                    } break;

                    case APP_COMMAND_KIND_BUILD_TAB: {
                        Str8 ExpressionFilePath = Registers()->Expression;
                        ConfigNode* Panel = ConfigNodeFromID(Registers()->Panel);
                        ConfigNode* Tab = ConfigNodeNew(
                            APP_STATE->Config, 
                            Panel, 
                            Registers()->String
                        );
                        ConfigNode* Expression = ConfigNodeNew(
                            APP_STATE->Config,
                            Tab, 
                            "expression"_s8
                        );

                        ConfigNodeNew(
                            APP_STATE->Config,
                            Expression, 
                            Registers()->Expression
                        );

                        if (ExpressionFilePath.Size) {
                            ConfigNode* Session = ConfigNodeNew(
                                APP_STATE->Config,
                                Tab, 
                                "session"_s8
                            );

                            ConfigNodeNew(
                                APP_STATE->Config,
                                Session, 
                                APP_STATE->SessionPath
                            );
                        }

                        AppCmd(APP_COMMAND_KIND_FOCUS_TAB, __Registers.Tab = Tab->ID);
                    } break;

                    case APP_COMMAND_KIND_DUPLICATE_TAB: {
                        ConfigNode* Source = ConfigNodeFromID(Registers()->Tab);
                        ConfigNode* Destination = ConfigNodeCpy(
                            APP_STATE->Config, 
                            Source
                        );

                        ConfigNodeInsertChild(
                            APP_STATE->Config,
                            Source->Parent, 
                            Source, 
                            Destination
                        );
                        AppCmd(APP_COMMAND_KIND_COPY_TAB_FULL_PATH, __Registers.Tab = Destination->ID);
                    } break;

                    case APP_COMMAND_KIND_COPY_TAB_FULL_PATH: {
                        ConfigNode* Tab = ConfigNodeFromID(Registers()->Tab);
                        Str8 Expression = ExpressionFromConfig(Tab);

                        SetClipboardText(Expression);
                    } break;

                    case APP_COMMAND_KIND_CLOSE_TAB: {
                        ConfigNode* Tab = ConfigNodeFromID(Registers()->Tab);
                        ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Tab);
                        ConfigPanelNode* Panel = ConfigPanelNodeFromConfigTree(PanelTree.Root, Tab->Parent);

                        if (Panel->SelectedTab == Tab) {
                            b32 FoundSelected = FALSE;
                            ConfigNode* NextSelectedTab = EMPTY_CFG_NODE_VALUE;

                            for (
                                ConfigNodePtrNode* N = Panel->Tabs.Head;
                                N;
                                N = N->Next
                            ) {
                                if (N->V == Panel->SelectedTab) {
                                    FoundSelected = TRUE;
                                } else if (!AppIsProjectFiltered(N->V)) {
                                    NextSelectedTab = N->V;

                                    if (FoundSelected)
                                        break;
                                }
                            }

                            AppCmd(APP_COMMAND_KIND_FOCUS_TAB, __Registers.Tab = NextSelectedTab->ID);
                        }

                        ConfigNodeRelease(APP_STATE->Config, Tab);

                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE)
                            WS->WindowLayoutReset = TRUE;
                    } break;

                    case APP_COMMAND_KIND_MOVE_VIEW: {
                        ConfigNode* View = ConfigNodeFromID(Registers()->View);
                        ConfigNode* PrevTab = ConfigNodeFromID(Registers()->PrevTab);
                        ConfigNode* SrcPanel = View->Parent;
                        ConfigNode* DstPanel = ConfigNodeFromID(Registers()->DstPanel);

                        if (DstPanel != EMPTY_CFG_NODE_VALUE && PrevTab != View) {
                            ConfigNodeUnhook(APP_STATE->Config, SrcPanel, View);
                            ConfigNodeInsertChild(
                                APP_STATE->Config, 
                                DstPanel, 
                                PrevTab, 
                                View
                            );
                            AppCmd(
                                APP_COMMAND_KIND_FOCUS_TAB,
                                __Registers.Panel = DstPanel->ID,
                                __Registers.Tab = View->ID
                            );
                            AppCmd(
                                APP_COMMAND_KIND_FOCUS_PANEL,
                                __Registers.Panel = DstPanel->ID
                            );

                            ConfigPanelTree SrcPanelTree = ConfigPanelTreeFromConfig(
                                Scratch.MemPool,
                                SrcPanel
                            );
                            ConfigPanelNode* SrcPanelNode = ConfigPanelNodeFromConfigTree(
                                SrcPanelTree.Root,
                                SrcPanel
                            );
                            b32 SrcPanelIsEmpty = FALSE;

                            if (SrcPanel != DstPanel) {
                                SrcPanelIsEmpty = TRUE;

                                for (
                                    ConfigNodePtrNode* N = SrcPanelNode->Tabs.Head;
                                    N;
                                    N = N->Next
                                ) {
                                    if (!AppIsProjectFiltered(N->V)) {
                                        AppCmd(
                                            APP_COMMAND_KIND_FOCUS_TAB,
                                            __Registers.Panel = SrcPanel->ID,
                                            __Registers.Tab = N->V->ID
                                        );
                                        SrcPanelIsEmpty = FALSE;
                                        break;
                                    }
                                }
                            }

                            if (SrcPanelIsEmpty) {
                                AppCmd(
                                    APP_COMMAND_KIND_CLOSE_PANEL,
                                    __Registers.Panel = SrcPanel->ID
                                );
                            }
                        }
                    } break;

                    case APP_COMMAND_KIND_TAB_BAR_TOP: {
                        ConfigNode* Panel = ConfigNodeFromID(Registers()->Panel);

                        ConfigNodeRelease(
                            APP_STATE->Config, 
                            ConfigNodeChildFromStr(Panel, "tabs_on_bottom"_s8)
                        );
                    } break;

                    case APP_COMMAND_KIND_TAB_BAR_BOTTOM: {
                        ConfigNode* Panel = ConfigNodeFromID(Registers()->Panel);

                        ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            Panel, 
                            "tabs_on_bottom"_s8
                        );
                    } break;

                    case APP_COMMAND_KIND_TAB_SETTINGS: {
                        Str8 Expression = ArenaPushStrFmt(
                            Scratch.MemPool,
                            "query:config.$%llx",
                            Registers()->Tab
                        );

                        AppCmd(
                            APP_COMMAND_KIND_PUSH_QUERY,
                            __Registers.Expression = Expression,
                            __Registers.DoImplicitRoot = TRUE,
                            __Registers.DoBigRows = TRUE,
                            __Registers.DoLister = TRUE
                        );
                    } break;

                    case APP_COMMAND_KIND_SET_CURRENT_PATH: {
                        ConfigNode* Session = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);
                        ConfigNode* CurrentPath = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            Session, 
                            "current_path"_s8
                        );

                        ConfigNodeNewReplace(
                            APP_STATE->Config,
                            CurrentPath, 
                            Registers()->FilePath
                        );
                    } break;

                    case APP_COMMAND_KIND_OPEN: {
                        Str8 Path = PathAbsoluteDstFromRelativeDstSrc(
                            Scratch.MemPool, 
                            Registers()->FilePath, 
                            GetCurrentPath(Scratch.MemPool)
                        );
                        FileProperties Props = SystemGetFileProperties(Path);

                        if (Props.Created) {
                            AppCmd(APP_COMMAND_KIND_RECORD_FILE_IN_SESSION);
                            AppCmd(
                                APP_COMMAND_KIND_BUILD_TAB,
                                __Registers.String = "pending"_s8,
                                __Registers.Expression = Path
                            );
                        } else {
                            LogError("could not open file '%S'", PRINT_STR(Path));
                        }
                    } break;

                    case APP_COMMAND_KIND_SWITCH: {} break;

                    case APP_COMMAND_KIND_SHOW_FILE_IN_EXPLORER: {
                        if (Registers()->FilePath.Size) {
                            Str8 FullPath = Registers()->FilePath;

                            ShowInFileExplorer(FullPath);
                        }
                    } break;

                    case APP_COMMAND_KIND_NEW_SESSION: {
                        Str8 NewPath = Registers()->FilePath;
                        b32 FileWillBeOverWritten = !!(SystemGetFileProperties(NewPath).Created);
                        UIKey Key = UIKeyFromStr({}, "new_config_overwrite_confirm"_s8);

                        if (
                            FileWillBeOverWritten && 
                            !Registers()->ForceConfirm && 
                            APP_STATE->PopUpKey != Key
                        ) {
                            APP_STATE->PopUpKey = Key;
                            APP_STATE->PopUpActive = TRUE;
                            ArenaClear(APP_STATE->PopUpMemPool);
                            MemSet(&APP_STATE->PopUpCommands, 0, sizeof(APP_STATE->PopUpCommands));
                            APP_STATE->PopUpTitle = ArenaPushStrCpy(
                                APP_STATE->PopUpMemPool, 
                                "Are you sure you want to save to this path?"_s8
                            );
                            APP_STATE->PopUpDescription = ArenaPushStrFmt(
                                APP_STATE->PopUpMemPool, 
                                "The existing file at '%S' will be overwritten.", 
                                PRINT_STR(NewPath)
                            );

                            AppRegisters* Regs = CopyRegisters(AppFrameMemPool(), Registers());

                            Regs->ForceConfirm = TRUE;
                            ListPush(
                                APP_STATE->PopUpMemPool, 
                                &APP_STATE->PopUpCommands, 
                                APP_COMMAND_KIND_INFO_TABLE[Kind].String, 
                                Regs
                            );
                        } else {
                            switch (Kind) {
                                default: {} break;

                                case APP_COMMAND_KIND_NEW_SESSION: {
                                    SystemDeleteFile(NewPath);
                                    AppCmd(APP_COMMAND_KIND_OPEN_SESSION, __Registers.FilePath = NewPath);
                                } break;
                            }
                        }
                    } break;

                    case APP_COMMAND_KIND_OPEN_SESSION: {
                        Str8 FileRootKey = "session"_s8;
                        ConfigNode* FileRoot = ConfigNodeChildFromStr(ConfigNodeRoot(), FileRootKey);
                        Str8 FilePath = Registers()->FilePath;
                        Str8 FileContents = ReadFileContents(Scratch.MemPool, FilePath);
                        FileProperties FileProps = SystemGetFileProperties(FilePath);
                        b32 FileIsOkay = FALSE;
                        Str8 StoredPath = APP_STATE->SessionPath;
                        Str8 FileExpectedHeader = ArenaPushStrFmt(
                            Scratch.MemPool,
                            "// %s",
                            NC_DATA_DIR
                        );

                        FileIsOkay = (
                            (!FileProps.Size && !FileProps.Created) ||
                            StrMatch(FilePath, StoredPath, 0) ||
                            !StoredPath.Size ||
                            StrMatch(
                                StrPrefix(FileContents, FileExpectedHeader.Size), 
                                FileExpectedHeader,
                                0
                            )
                        );

                        // TODO(nathan): Add semantic versioning to files for checking

                        if (!FileIsOkay) {
                            LogError("could not open file '%S'", PRINT_STR(FilePath));
                        }

                        ConfigNodePtrList FileConfigs = {};

                        if (FileIsOkay) {
                            ConfigNodeReleaseAllChildren(
                                APP_STATE->Config, 
                                FileRoot
                            );
                            FileConfigs = ConfigNodePtrListFromStr(
                                Scratch.MemPool, 
                                APP_STATE->Config,
                                APP_STATE->CfgSchemaTable,
                                StrChopLastSlash(FilePath), 
                                FileContents
                            );

                            switch (Kind) {
                                default: {} break;

                                case APP_COMMAND_KIND_OPEN_SESSION: {
                                    ArenaClear(APP_STATE->SessionPathMemPool);
                                    APP_STATE->SessionPath = ArenaPushStrCpy(APP_STATE->SessionPathMemPool, FilePath);
                                } break;
                            }

                            for (
                                ConfigNodePtrNode* N = FileConfigs.Head; 
                                N; 
                                N = N->Next
                            ) {
                                ConfigNodeInsertChild(
                                    APP_STATE->Config,
                                    FileRoot, 
                                    FileRoot->Tail, 
                                    N->V
                                );
                            }

                            if (Kind == APP_COMMAND_KIND_OPEN_SESSION) {
                                ConfigNodePtrList AllUserWindows = ConfigNodeChildListFromStr(
                                    Scratch.MemPool, 
                                    FileRoot, 
                                    "window"_s8
                                );

                                if (!AllUserWindows.Count) {
                                    Handle Monitor = GetPrimaryMonitor();
                                    Str8 MonitorName = NameFromMonitor(Scratch.MemPool, Monitor);
                                    v2f32 MonitorDim = GetMonitorDimensions(Monitor);
                                    f32 MonitorDPI = GetMonitorDPI(Monitor);
                                    v2f32 WindowDim = Vec(
                                        MonitorDim.X * 4 / 5,
                                        MonitorDim.Y * 4 / 5
                                    );

                                    if (WindowDim.X == 0.0f || WindowDim.Y == 0.0f)
                                        WindowDim = Vec(1280.0f, 720.0f);

                                    ConfigNode* NewWindow = ConfigNodeNew(
                                        APP_STATE->Config,
                                        FileRoot, 
                                        "window"_s8
                                    );
                                    ConfigNode* Size = ConfigNodeNew(
                                        APP_STATE->Config,
                                        NewWindow, 
                                        "size"_s8
                                    );

                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        Size, 
                                        "%f", 
                                        WindowDim.X
                                    );
                                    ConfigNodeNew(
                                        APP_STATE->Config,
                                        Size, 
                                        "%f", 
                                        WindowDim.Y
                                    );

                                    f32 LineHeightGuess = 11.0f + (MonitorDPI / 96.0f);
                                    f32 NumLinesInMonitor = MonitorDim.Y / LineHeightGuess;

                                    if (NumLinesInMonitor < 100.0f)
                                        AppCmd(APP_COMMAND_KIND_RESET_TO_COMPACT_PANELS, __Registers.Window = NewWindow->ID);
                                    else
                                        AppCmd(APP_COMMAND_KIND_RESET_TO_DEFAULT_PANELS, __Registers.Window = NewWindow->ID);
                                }

                                ConfigNodePtrList AllKeyBindings = ConfigNodeChildListFromStr(
                                    Scratch.MemPool, 
                                    FileRoot, 
                                    "keybindings"_s8
                                );

                                if (!AllKeyBindings.Count)
                                    AppCmd(APP_COMMAND_KIND_RESET_TO_DEFAULT_BINDINGS);

                                if (FileIsOkay && Kind == APP_COMMAND_KIND_OPEN_SESSION)
                                    AppCmd(APP_COMMAND_KIND_RECORD_FILE_IN_SESSION);
                            }

                            if (FileIsOkay && Kind == APP_COMMAND_KIND_OPEN_SESSION) {
                                AppCmd(APP_COMMAND_KIND_RECORD_SESSION_AS_LAST_OPENED);

                                ConfigNodePtrList Windows = ConfigNodeTopLevelListFromStr(
                                    Scratch.MemPool, 
                                    "window"_s8
                                );

                                for (
                                    ConfigNodePtrNode* N = Windows.Head;
                                    N;
                                    N = N->Next
                                ) {
                                    ConfigPanelTree Panels = ConfigPanelTreeFromConfig(
                                        Scratch.MemPool, 
                                        N->V
                                    );

                                    for (
                                        ConfigPanelNode* Panel = Panels.Root;
                                        Panel != EMPTY_CFG_PANEL_NODE_VALUE;
                                        Panel = ConfigPanelNodeRecDepthFirstPre(Panels.Root, Panel).Next
                                    ) {
                                        if (AppIsProjectFiltered(Panel->SelectedTab)) {
                                            ConfigNode* FallbackTab = EMPTY_CFG_NODE_VALUE;

                                            for (
                                                ConfigNodePtrNode* TabNode = Panel->Tabs.Head;
                                                TabNode;
                                                TabNode = TabNode->Next
                                            ) {
                                                ConfigNode* Tab = TabNode->V;

                                                if (!AppIsProjectFiltered(Tab)) {
                                                    FallbackTab = Tab;
                                                    break;
                                                }
                                            }

                                            AppCmd(
                                                APP_COMMAND_KIND_FOCUS_TAB,
                                                __Registers.Panel = Panel->Config->ID,
                                                __Registers.Tab = FallbackTab->ID
                                            );
                                        }
                                    }
                                }
                            }

                            if (Kind == APP_COMMAND_KIND_OPEN_SESSION && !APP_STATE->SessionPath.Size) {
                                ConfigNode* Session = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);
                                ConfigNode* RecentSession = ConfigNodeChildFromStr(Session, "recent_session"_s8);
                                Str8 SessionPath = PathFromConfig(RecentSession);

                                if (!SessionPath.Size) {
                                    Str8 UserProgramDataPath = GetProcessProperties()->UserProgramDataPath;
                                    Str8 UserDataFolder = ArenaPushStrFmt(
                                        Scratch.MemPool, 
                                        "%S/%s", 
                                        PRINT_STR(UserProgramDataPath), 
                                        NC_DATA_DIR
                                    );

                                    SystemCreateDirectory(UserDataFolder);
                                    SessionPath = ArenaPushStrFmt(
                                        Scratch.MemPool, 
                                        "%S/default.%s_session", 
                                        PRINT_STR(UserDataFolder),
                                        NC_DATA_DIR
                                    );
                                }

                                AppCmd(
                                    APP_COMMAND_KIND_OPEN_SESSION, 
                                    __Registers.FilePath = SessionPath
                                );
                            }

                            Str8 WindowTitle = AppPushWindowTitle(Scratch.MemPool);

                            for (
                                AppWindowState* WState = APP_STATE->HeadWindowState;
                                WState != EMPTY_APP_WINDOW_STATE_VALUE;
                                WState = WState->OrderNext
                            ) {
                                WindowSetTitle(WState->WinHandle, WindowTitle);
                            }
                        }
                    } break;

                    case APP_COMMAND_KIND_OPEN_RECENT_SESSION: {
                        ConfigNode* Config = ConfigNodeFromID(Registers()->Config);
                        ConfigNode* Path = ConfigNodeChildFromStr(Config, "path"_s8);

                        if (
                            StrMatch(Config->String, "recent_session"_s8, 0) &&
                            Path->Head->String.Size
                        ) {
                            AppCmd(
                                APP_COMMAND_KIND_OPEN_SESSION,
                                __Registers.FilePath = Path->Head->String
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_SAVE_SESSION: {
                        Str8 NewPath = Registers()->FilePath;
                        b32 FileWillBeOverWritten = !!(SystemGetFileProperties(NewPath).Created);
                        UIKey Key = UIKeyFromStr({}, "new_config_overwrite_confirm"_s8);

                        if (
                            FileWillBeOverWritten && 
                            !Registers()->ForceConfirm && 
                            APP_STATE->PopUpKey != Key
                        ) {
                            APP_STATE->PopUpKey = Key;
                            APP_STATE->PopUpActive = TRUE;
                            ArenaClear(APP_STATE->PopUpMemPool);
                            MemSet(&APP_STATE->PopUpCommands, 0, sizeof(APP_STATE->PopUpCommands));
                            APP_STATE->PopUpTitle = ArenaPushStrCpy(
                                APP_STATE->PopUpMemPool, 
                                "Are you sure you want to save to this path?"_s8
                            );
                            APP_STATE->PopUpDescription = ArenaPushStrFmt(
                                APP_STATE->PopUpMemPool, 
                                "The existing file at '%S' will be overwritten.", 
                                PRINT_STR(NewPath)
                            );

                            AppRegisters* Regs = CopyRegisters(AppFrameMemPool(), Registers());

                            Regs->ForceConfirm = TRUE;
                            ListPush(APP_STATE->PopUpMemPool, &APP_STATE->PopUpCommands, APP_COMMAND_KIND_INFO_TABLE[Kind].String, Regs);
                        } else {
                            switch (Kind) {
                                default: {} break;

                                case APP_COMMAND_KIND_SAVE_SESSION: {
                                    ArenaClear(APP_STATE->SessionPathMemPool);
                                    APP_STATE->SessionPath = ArenaPushStrCpy(APP_STATE->SessionPathMemPool, NewPath);
                                    AppCmd(APP_COMMAND_KIND_WRITE_SESSION_DATA);
                                    AppCmd(APP_COMMAND_KIND_RECORD_SESSION_AS_LAST_OPENED);
                                } break;
                            }
                        }
                    } break;

                    case APP_COMMAND_KIND_WRITE_SESSION_DATA: {
                        DstPath = APP_STATE->SessionPath;
                        BucketName = "session"_s8;

                        b32 DstExists = !!(SystemGetFileProperties(DstPath).Created);
                        Str8 TempPath = ArenaPushStrFmt(Scratch.MemPool, "%S.temp", PRINT_STR(DstPath));
                        Str8 OverWrittenPath = ArenaPushStrFmt(Scratch.MemPool, "%S.old", PRINT_STR(DstPath));
                        ConfigNode* TreeRoot = ConfigNodeChildFromStr(ConfigNodeRoot(), BucketName);
                        Str8List Strings = {};

                        ListPushFmt(
                            Scratch.MemPool, 
                            &Strings, 
                            "// %s %S file\n\n", 
                            NC_DATA_DIR,
                            PRINT_STR(BucketName)
                        );

                        for (
                            ConfigNode* Child = TreeRoot->Head; 
                            Child != EMPTY_CFG_NODE_VALUE; 
                            Child = Child->Next
                        ) {
                            ListPush(
                                Scratch.MemPool, 
                                &Strings, 
                                ConfigStrFromTree(
                                    Scratch.MemPool,
                                    APP_STATE->CfgSchemaTable,
                                    StrChopLastSlash(DstPath), 
                                    Child
                                )
                            );
                        }

                        Str8 Data = StrListJoin(Scratch.MemPool, &Strings, NULL);
                        b32 TempWriteOkay = WriteFileContents(TempPath, Data);
                        b32 OldDeletionOkay = (TempWriteOkay && SystemDeleteFile(OverWrittenPath));
                        b32 OldMoveOkay = (TempWriteOkay && (!DstExists || SystemMoveFile(OverWrittenPath, DstPath)));
                        b32 NewMoveOkay = (OldMoveOkay && SystemMoveFile(DstPath, TempPath));

                        if (NewMoveOkay && DstExists)
                            SystemDeleteFile(OverWrittenPath);
                        else if (!NewMoveOkay && OldMoveOkay && DstExists)
                            SystemMoveFile(DstPath, OverWrittenPath);
                    } break;

                    case APP_COMMAND_KIND_RECORD_SESSION_AS_LAST_OPENED: {
                        Str8 FilePath = Registers()->FilePath;
                        ConfigNode* Session = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);
                        ConfigNodePtrList RecentSessions = ConfigNodeChildListFromStr(
                            Scratch.MemPool, 
                            Session, 
                            "recent_session"_s8
                        );
                        ConfigNode* RecentSession = EMPTY_CFG_NODE_VALUE;

                        for (
                            ConfigNodePtrNode* N = RecentSessions.Head;
                            N;
                            N = N->Next
                        ) {
                            if (PathMatchNormalised(PathFromConfig(N->V), FilePath)) {
                                RecentSession = N->V;
                                break;
                            }
                        }

                        if (RecentSession == EMPTY_CFG_NODE_VALUE) {
                            RecentSession = ConfigNodeNew(
                                APP_STATE->Config,
                                Session, 
                                "recent_session"_s8
                            );

                            ConfigNode* PathRoot = ConfigNodeNew(
                                APP_STATE->Config, 
                                RecentSession, 
                                "path"_s8
                            );

                            ConfigNodeNew(APP_STATE->Config, PathRoot, FilePath);
                        }

                        ConfigNodeUnhook(APP_STATE->Config, Session, RecentSession);
                        ConfigNodeInsertChild(
                            APP_STATE->Config, 
                            Session, 
                            EMPTY_CFG_NODE_VALUE, 
                            RecentSession
                        );
                        RecentSessions = ConfigNodeChildListFromStr(
                            Scratch.MemPool, 
                            Session, 
                            "recent_session"_s8
                        );

                        if (RecentSessions.Count > 32) {
                            ConfigNodeRelease(
                                APP_STATE->Config,
                                RecentSessions.Tail->V
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_SESSION_SETTINGS: {
                        AppCmd(
                            APP_COMMAND_KIND_PUSH_QUERY,
                            __Registers.Expression = "query:session_settings"_s8,
                            __Registers.DoImplicitRoot = TRUE,
                            __Registers.DoBigRows = TRUE,
                            __Registers.DoLister = TRUE
                        );
                    } break;

                    case APP_COMMAND_KIND_EDIT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_PRESS;
                        Event.Slot = UI_EVENT_ACTION_SLOT_EDIT;
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_ACCEPT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_PRESS;
                        Event.Slot = UI_EVENT_ACTION_SLOT_ACCEPT;
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_CANCEL: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_PRESS;
                        Event.Slot = UI_EVENT_ACTION_SLOT_CANCEL;
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_LEFT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_PICK_SELECT_SIDE | UI_EVENT_FLAG_ZERO_DELTA_ON_SELECT | UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_RIGHT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_PICK_SELECT_SIDE | UI_EVENT_FLAG_ZERO_DELTA_ON_SELECT | UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL | UI_EVENT_FLAG_SECONDARY;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL | UI_EVENT_FLAG_SECONDARY;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_LEFT_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_SECONDARY | UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_RIGHT_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_SECONDARY | UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL | UI_EVENT_FLAG_SECONDARY | UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL | UI_EVENT_FLAG_SECONDARY | UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_LEFT_CHUNK: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_RIGHT_CHUNK: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_CHUNK: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL | UI_EVENT_FLAG_SECONDARY;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_CHUNK: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL | UI_EVENT_FLAG_SECONDARY;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_LEFT_CHUNK_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK | UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_RIGHT_CHUNK_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK | UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_CHUNK_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK | UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_CHUNK_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK | UI_EVENT_FLAG_EXPLICIT_DIRECTIONAL;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_PAGE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_PAGE;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_PAGE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_PAGE;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_WHOLE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_WHOLE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_PAGE_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_PAGE;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_PAGE_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_WHOLE_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_WHOLE_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_UP_REORDER: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_REORDER;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(0, -1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_DOWN_REORDER: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_REORDER;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(0, 1);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_HOME: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_LINE;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_END: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_LINE;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_HOME_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_LINE;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_MOVE_END_SELECT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_NAVIGATE;
                        Event.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_LINE;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_SELECT_ALL: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent MoveEvent = {};
                        UIEvent SelectEvent = {};

                        MoveEvent.Kind = UI_EVENT_KIND_NAVIGATE;
                        MoveEvent.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        MoveEvent.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &MoveEvent);
                        SelectEvent.Kind = UI_EVENT_KIND_NAVIGATE;
                        SelectEvent.Flags = UI_EVENT_FLAG_KEEP_MARK;
                        SelectEvent.DeltaStride = UI_EVENT_DELTA_STRIDE_WHOLE;
                        SelectEvent.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &SelectEvent);
                    } break;

                    case APP_COMMAND_KIND_DELETE_SINGLE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_EDIT;
                        Event.Flags = UI_EVENT_FLAG_DELETE | UI_EVENT_FLAG_ZERO_DELTA_ON_SELECT;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_DELETE_CHUNK: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_EDIT;
                        Event.Flags = UI_EVENT_FLAG_DELETE | UI_EVENT_FLAG_ZERO_DELTA_ON_SELECT;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_BACKSPACE_SINGLE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_EDIT;
                        Event.Flags = UI_EVENT_FLAG_DELETE | UI_EVENT_FLAG_ZERO_DELTA_ON_SELECT;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_BACKSPACE_CHUNK: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_EDIT;
                        Event.Flags = UI_EVENT_FLAG_DELETE | UI_EVENT_FLAG_ZERO_DELTA_ON_SELECT;
                        Event.DeltaStride = UI_EVENT_DELTA_STRIDE_WORD;
                        Event.DeltaI32 = Vec(-1, 0);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_COPY: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_EDIT;
                        Event.Flags = UI_EVENT_FLAG_COPY | UI_EVENT_FLAG_KEEP_MARK;
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_CUT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_EDIT;
                        Event.Flags = UI_EVENT_FLAG_COPY | UI_EVENT_FLAG_DELETE;
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_PASTE: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        UIEvent Event = {};

                        Event.Kind = UI_EVENT_KIND_TEXT;
                        Event.String = GetClipboardText(Scratch.MemPool);
                        ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                    } break;

                    case APP_COMMAND_KIND_INSERT_TEXT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                            UIEvent Event = {};

                            Event.Kind = UI_EVENT_KIND_TEXT;
                            Event.String = Registers()->String;
                            ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                        }
                    } break;

                    case APP_COMMAND_KIND_MOVE_NEXT: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                            UIEvent Event = {};

                            Event.Kind = UI_EVENT_KIND_NAVIGATE;
                            Event.Flags = UI_EVENT_FLAG_SECONDARY;
                            Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                            Event.DeltaI32 = Vec(0, 1);
                            ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                        }
                    } break;

                    case APP_COMMAND_KIND_MOVE_PREV: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                            UIEvent Event = {};

                            Event.Kind = UI_EVENT_KIND_NAVIGATE;
                            Event.Flags = UI_EVENT_FLAG_SECONDARY;
                            Event.DeltaStride = UI_EVENT_DELTA_STRIDE_CHAR;
                            Event.DeltaI32 = Vec(0, -1);
                            ListPush(Scratch.MemPool, &WS->UIEvents, &Event);
                        }
                    } break;

                    case APP_COMMAND_KIND_CENTRE_CURSOR: {} break;

                    case APP_COMMAND_KIND_CONTAIN_CURSOR: {} break;

                    case APP_COMMAND_KIND_FIND_NEXT: {} break;

                    case APP_COMMAND_KIND_FIND_PREV: {} break;

                    case APP_COMMAND_KIND_SELECT_CONFIG: {
                        ConfigNode* Cfg = ConfigNodeFromID(Registers()->Config);
                        ConfigNodePtrList AllOfTheSameKind = ConfigNodeTopLevelListFromStr(
                            Scratch.MemPool, 
                            Cfg->String
                        );
                        b32 IsSelected = !AppIsDisabled(Cfg);

                        for (
                            ConfigNodePtrNode* N = AllOfTheSameKind.Head;
                            N;
                            N = N->Next
                        ) {
                            ConfigNode* C = N->V;

                            ConfigNodeRelease(
                                APP_STATE->Config, 
                                ConfigNodeChildFromStr(C, "enabled"_s8)
                            );
                        }

                        ConfigNode* EnabledRoot = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config, 
                            Cfg, 
                            "enabled"_s8
                        );

                        ConfigNodeNewReplace(APP_STATE->Config, EnabledRoot, "1"_s8);
                    } break;

                    case APP_COMMAND_KIND_ENABLE_CONFIG: {
                        ConfigNode* Cfg = ConfigNodeFromID(Registers()->Config);
                        ConfigNode* EnabledRoot = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config, 
                            Cfg, 
                            "enabled"_s8
                        );

                        ConfigNodeNewReplace(
                            APP_STATE->Config, 
                            EnabledRoot, 
                            "1"_s8
                        );
                    } break;

                    case APP_COMMAND_KIND_DISABLE_CONFIG:
                    case APP_COMMAND_KIND_DESELECT_CONFIG: {
                        ConfigNode* Cfg = ConfigNodeFromID(Registers()->Config);
                        ConfigNode* EnabledRoot = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config, 
                            Cfg, 
                            "enabled"_s8
                        );

                        ConfigNodeNewReplace(
                            APP_STATE->Config, 
                            EnabledRoot, 
                            "0"_s8
                        );
                    } break;

                    case APP_COMMAND_KIND_REMOVE_CONFIG: {
                        ConfigNode* Cfg = ConfigNodeFromID(Registers()->Config);

                        ConfigNodeRelease(APP_STATE->Config, Cfg);
                    } break;

                    case APP_COMMAND_KIND_NAME_CONFIG: {
                        ConfigNode* Cfg = ConfigNodeFromID(Registers()->Config);

                        if (Registers()->String.Size) {
                            ConfigNode* Label = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config, 
                                Cfg, 
                                "label"_s8
                            );

                            ConfigNodeNew(
                                APP_STATE->Config, 
                                Label, 
                                Registers()->String
                            );
                        } else {
                            ConfigNodeRelease(
                                APP_STATE->Config,
                                ConfigNodeChildFromStr(Cfg, "label"_s8)
                            );
                        }
                    } break;

                    case APP_COMMAND_KIND_DUPLICATE_CONFIG: {
                        ConfigNode* Source = ConfigNodeFromID(Registers()->Config);
                        ConfigNode* Destination = ConfigNodeCpy(
                            APP_STATE->Config, 
                            Source
                        );

                        ConfigNodeInsertChild(
                            APP_STATE->Config,
                            Source->Parent, 
                            Source, 
                            Destination
                        );
                    } break;

                    case APP_COMMAND_KIND_SAVE_TO_SESSION: {
                        ConfigNode* Cfg = ConfigNodeFromID(Registers()->Config);

                        ConfigNodeUnhook(
                            APP_STATE->Config,
                            Cfg->Parent, 
                            Cfg
                        );

                        ConfigNode* Session = ConfigNodeChildFromStr(
                            ConfigNodeRoot(), 
                            "session"_s8
                        );

                        ConfigNodeInsertChild(
                            APP_STATE->Config,
                            Session, 
                            Session->Tail, 
                            Cfg
                        );
                    } break;

                    case APP_COMMAND_KIND_EDIT_SESSION_THEME: {
                        ConfigNode* Parent = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);

                        AppCmd(
                            APP_COMMAND_KIND_PUSH_QUERY, 
                            __Registers.Expression = ArenaPushStrFmt(
                                Scratch.MemPool,
                                "query:config.$%llx.theme_colours",
                                Parent->ID
                            )
                        );
                    } break;

                    case APP_COMMAND_KIND_ADD_THEME_COLOUR: {
                        Access* Acc = AccessOpen();
                        ConfigNode* Parent = ConfigNodeFromID(Registers()->Config);
                        ConfigNode* Theme = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            Parent, 
                            "theme"_s8
                        );
                        MDNode* ThemeTree = AppThemeTreeFromName(Scratch.MemPool, Acc, Theme->Head->String);

                        if (ThemeTree == EMPTY_MD_NODE_VALUE) {
                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                Theme, 
                                APP_THEME_PRESET_DISPLAY_STR_TABLE[APP_THEME_PRESET_VISUAL_STUDIO_DARK]
                            );
                        }

                        ConfigNode* Colour = ConfigNodeNew(
                            APP_STATE->Config,
                            Parent, 
                            "theme_colour"_s8
                        );

                        ConfigNodeNew(APP_STATE->Config, Colour, "tags"_s8);

                        ConfigNode* Value = ConfigNodeNew(
                            APP_STATE->Config,
                            Colour, 
                            "value"_s8
                        );

                        ConfigNodeNew(
                            APP_STATE->Config,
                            Value, 
                            "0xFFFFFFFF"_s8
                        );
                        AccessClose(Acc);
                    } break;

                    case APP_COMMAND_KIND_FORK_THEME: {
                        Access* Acc = AccessOpen();
                        ConfigNode* Parent = ConfigNodeFromID(Registers()->Config);
                        ConfigNodePtrList Colours = ConfigNodeChildListFromStr(
                            Scratch.MemPool, 
                            Parent, 
                            "theme_colour"_s8
                        );

                        for (
                            ConfigNodePtrNode* N = Colours.Head;
                            N;
                            N = N->Next
                        ) {
                            ConfigNodeRelease(APP_STATE->Config, N->V);
                        }

                        ConfigNode* ThemeConfig = ConfigNodeChildFromStr(Parent, "theme"_s8);
                        Str8 ThemeName = ThemeConfig->Head->String;
                        MDNode* ThemeTree = AppThemeTreeFromName(Scratch.MemPool, Acc, ThemeName);

                        if (ThemeTree == EMPTY_MD_NODE_VALUE)
                            ThemeTree = APP_STATE->ThemePresetTrees[APP_THEME_PRESET_VISUAL_STUDIO_DARK];

                        for (
                            MDNode* N = ThemeTree; 
                            !MDNodeIsEmpty(N); 
                            N = MDNodeRecordDepthFirstPreOrder(N, ThemeTree).Next
                        ) {
                            if (StrMatch(N->String, "theme_colour"_s8, 0)) {
                                ConfigNode* Colour = ConfigNodeNew(
                                    APP_STATE->Config,
                                    Parent, 
                                    "theme_colour"_s8
                                );
                                ConfigNode* Tags = ConfigNodeNew(
                                    APP_STATE->Config,
                                    Colour, 
                                    "tags"_s8
                                );
                                ConfigNode* Value = ConfigNodeNew(
                                    APP_STATE->Config,
                                    Colour, 
                                    "value"_s8
                                );

                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    Tags, 
                                    MDChildFromStr(N, "tags"_s8, 0)->Head->String
                                );
                                ConfigNodeNew(
                                    APP_STATE->Config,
                                    Value, 
                                    MDChildFromStr(N, "value"_s8, 0)->Head->String
                                );
                            }
                        }

                        ConfigNodeRelease(APP_STATE->Config, ThemeConfig);
                        AccessClose(Acc);
                    } break;

                    case APP_COMMAND_KIND_SAVE_THEME:
                    case APP_COMMAND_KIND_SAVE_AND_SET_THEME: {
                        Str8 Name = Registers()->String;

                        if (Name.Size) {
                            Str8 ThemesFolder = ArenaPushStrFmt(
                                Scratch.MemPool, 
                                "%S/%s/themes",
                                PRINT_STR(GetProcessProperties()->UserProgramDataPath),
                                NC_DATA_DIR
                            );

                            if (SystemCreateDirectory(ThemesFolder)) {
                                Str8 DstPath = ArenaPushStrFmt(
                                    Scratch.MemPool, 
                                    (char*) "%S/%S",
                                    PRINT_STR(ThemesFolder), 
                                    PRINT_STR(Name)
                                );
                                ConfigNode* Parent = ConfigNodeFromID(Registers()->Config);
                                ConfigNodePtrList Colours = ConfigNodeChildListFromStr(
                                    Scratch.MemPool, 
                                    Parent, 
                                    "theme_colour"_s8
                                );
                                Str8List Strings = {};

                                for (
                                    ConfigNodePtrNode* N = Colours.Head;
                                    N;
                                    N = N->Next
                                ) {
                                    ListPush(
                                        Scratch.MemPool, 
                                        &Strings, 
                                        ConfigStrFromTree(
                                            Scratch.MemPool, 
                                            APP_STATE->CfgSchemaTable,
                                            StrChopLastSlash(DstPath), 
                                            N->V
                                        )
                                    );
                                }

                                ConfigNode* Theme = ConfigNodeChildFromStrOrAlloc(
                                    APP_STATE->Config,
                                    Parent, 
                                    "theme"_s8
                                );

                                ConfigNodeNewReplace(
                                    APP_STATE->Config,
                                    Theme, 
                                    Name
                                );
                            }
                        } else {
                            LogError("could not write to theme path"_s8);
                        }
                    } break;

                    case APP_COMMAND_KIND_PICK_FILE: {} break;

                    case APP_COMMAND_KIND_PICK_FOLDER: {} break;

                    case APP_COMMAND_KIND_PICK_FILE_OR_FOLDER: {} break;

                    case APP_COMMAND_KIND_TOGGLE_DEV_MENU: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        WS->DevMenuIsOpen ^= TRUE;
                    } break;

                    case APP_COMMAND_KIND_OPEN_TEXT: {} break;

                    case APP_COMMAND_KIND_QUERY: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        ConfigNode* View = ConfigNodeFromID(Registers()->View);
                        Str8 CommandName = AppViewQueryCommand();
                        b32 IsLister = ((ConfigNodeChildFromStr(View, "lister"_s8)) != EMPTY_CFG_NODE_VALUE);

                        if (CommandName.Size) {
                            AppRegistersScope() {
                                if (IsLister)
                                    Registers()->View = WS->QueryRegs->View;

                                AppPushCmd(CommandName, Registers());
                            }
                        }

                        AppCommandKindInfo* CommandKindInfo = EMPTY_APP_COMMAND_KIND_INFO_VALUE;

                        if (CommandName.Size)
                            CommandKindInfo = AppCommandKindInfoFromStr(CommandName);

                        if (IsLister) {
                            WS->QueryIsActive = FALSE;
                        } else if (!(CommandKindInfo->Query.Kind & APP_QUERY_KIND_KEEP_OLD_INPUT)) {
                            AppViewState* VState = AppViewStateFromConfig(View);

                            VState->QueryIsOpen = FALSE;
                            VState->QueryStringSize = 0;
                        }
                    } break;

                    case APP_COMMAND_KIND_PUSH_QUERY: {
                        Str8 CommandName = Registers()->CommandName;
                        AppCommandKindInfo* Info = AppCommandKindInfoFromStr(CommandName);
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        UIContextMenuClose();
                        WS->MenuBarFocused = FALSE;

                        ConfigNode* View = EMPTY_CFG_NODE_VALUE;
                        b32 IsFloating = (!CommandName.Size || Info->Query.Kind & APP_QUERY_KIND_FLOATING);

                        if (IsFloating) {
                            if (WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                                WS->QueryIsActive = TRUE;
                                ArenaClear(WS->QueryMemPool);
                                WS->QueryRegs = CopyRegisters(WS->QueryMemPool, Registers());
                            }

                            ConfigNode* WindowQuery = AppImmediateConfigFromKey("window_query_%p", Window);

                            ConfigNodeReleaseAllChildren(
                                APP_STATE->Config,
                                WindowQuery
                            );
                            View = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                WindowQuery, 
                                "watch"_s8
                            );

                            ConfigNode* Expression = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                View, 
                                "expression"_s8
                            );

                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                Expression, 
                                Registers()->Expression
                            );

                            ConfigNode* ContextExpression = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                View,
                                "context_expression"_s8
                            );

                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                ContextExpression,
                                Registers()->ContextExpression
                            );
                        } else {
                            View = ConfigNodeFromID(Registers()->View);
                        }

                        b32 ViewIsLister = (ConfigNodeChildFromStr(View, "lister"_s8) != EMPTY_CFG_NODE_VALUE);

                        if (!ViewIsLister) {
                            ConfigNode* Query = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                View, 
                                "query"_s8
                            );
                            ConfigNode* Cmd = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                Query, 
                                "cmd"_s8
                            );
                            ConfigNode* Input = ConfigNodeChildFromStrOrAlloc(
                                APP_STATE->Config,
                                Query, 
                                "input"_s8
                            );

                            if (IsFloating) {
                                if (Registers()->DoImplicitRoot) {
                                    ConfigNodeRelease(
                                        APP_STATE->Config,
                                        ConfigNodeChildFromStr(
                                            View, 
                                            "explicit_root"_s8
                                        )
                                    );
                                } else {
                                    ConfigNodeChildFromStrOrAlloc(
                                        APP_STATE->Config,
                                        View, 
                                        "explicit_root"_s8
                                    );
                                }

                                if (!Registers()->DoLister) {
                                    ConfigNodeRelease(
                                        APP_STATE->Config,
                                        ConfigNodeChildFromStr(
                                            View, 
                                            "lister"_s8
                                        )
                                    );
                                } else {
                                    ConfigNodeChildFromStrOrAlloc(
                                        APP_STATE->Config,
                                        View, 
                                        "lister"_s8
                                    );
                                }

                                if (!Registers()->DoSmall) {
                                    ConfigNodeRelease(
                                        APP_STATE->Config,
                                        ConfigNodeChildFromStr(View, "small"_s8)
                                    );
                                } else {
                                    ConfigNodeChildFromStrOrAlloc(
                                        APP_STATE->Config,
                                        View,
                                        "small"_s8
                                    );
                                }

                                if (!Registers()->ActivateWithSingleClick) {
                                    ConfigNodeRelease(
                                        APP_STATE->Config,
                                        ConfigNodeChildFromStr(
                                            View,
                                            "activate_with_single_click"_s8
                                        )
                                    );
                                } else {
                                    ConfigNodeChildFromStrOrAlloc(
                                        APP_STATE->Config,
                                        View,
                                        "activate_with_single_click"_s8
                                    );
                                }
                            }

                            Str8 InitialInput = ""_s8;

                            if (CommandName.Size) {
                                if (Info->Query.Slot == APP_REGISTERS_SLOT_FILE_PATH) {
                                    ConfigNode* Session = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);
                                    ConfigNode* CurrentPath = ConfigNodeChildFromStr(Session, "current_path"_s8);
                                    Str8 CurrentPathString = CurrentPath->Head->String;

                                    if (!CurrentPathString.Size)
                                        CurrentPathString = PathNormalisedFromStr(Scratch.MemPool, GetCurrentPath(Scratch.MemPool));

                                    InitialInput = CurrentPathString;
                                    InitialInput = ArenaPushStrFmt(Scratch.MemPool, "%S/", PRINT_STR(InitialInput));
                                } else if (Info->Query.Kind & APP_QUERY_KIND_KEEP_OLD_INPUT) {
                                    InitialInput = Input->Head->String;
                                }
                            }

                            Str8 CurrentQueryCommandName = Cmd->Head->String;

                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                Input, 
                                InitialInput
                            );
                            ConfigNodeNewReplace(
                                APP_STATE->Config,
                                Cmd, 
                                CommandName
                            );

                            AppViewState* VS = AppViewStateFromConfig(View);

                            if (CommandName.Size) {
                                if (!VS->QueryIsOpen && Info->Query.Kind & APP_QUERY_KIND_SELECT_OLD_INPUT) {
                                    VS->QueryCursor = TxtPt(1, 1 + Input->Head->String.Size);
                                    VS->QueryMark = TxtPt(1, 1);
                                } else {
                                    VS->QueryCursor = TxtPt(1, 1 + Input->Head->String.Size);
                                    VS->QueryMark = VS->QueryCursor;
                                }

                                if (!StrMatch(CurrentQueryCommandName, CommandName, 0)) {
                                    VS->QueryIsOpen = TRUE;
                                } else {
                                    VS->QueryIsOpen ^= TRUE;
                                }
                            }

                            if (Registers()->DoLister) {
                                VS->QueryIsOpen = TRUE;
                            }

                            VS->ContentsAreFocused = FALSE;
                        }
                    } break;

                    case APP_COMMAND_KIND_COMPLETE_QUERY: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);
                        ConfigNode* View = ConfigNodeFromID(Registers()->View);
                        Str8 CommandName = AppViewQueryCommand();
                        b32 IsLister = (ConfigNodeChildFromStr(View, "lister"_s8) != EMPTY_CFG_NODE_VALUE);

                        if (CommandName.Size) {
                            AppRegistersScope() {
                                if (IsLister) {
                                    Registers()->View = WS->QueryRegs->View;
                                }

                                AppPushCmd(CommandName, Registers());
                            }
                        }

                        AppCommandKindInfo* Info = AppCommandKindInfoFromStr(CommandName);

                        if (IsLister) {
                            WS->QueryIsActive = FALSE;
                        } else if (!(Info->Query.Kind & APP_QUERY_KIND_KEEP_OLD_INPUT)) {
                            AppViewState* VS = AppViewStateFromConfig(View);

                            VS->QueryIsOpen = FALSE;
                            VS->QueryStringSize = 0;
                        }
                    } break;

                    case APP_COMMAND_KIND_CANCEL_QUERY: {
                        ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                        AppWindowState* WS = AppWindowStateFromConfig(Window);

                        if (WS != EMPTY_APP_WINDOW_STATE_VALUE) {
                            WS->QueryIsActive = FALSE;
                            ArenaClear(WS->QueryMemPool);
                            WS->QueryRegs = NULL;
                        } 
                    } break;

                    case APP_COMMAND_KIND_UPDATE_QUERY: {
                        ConfigNode* View = ConfigNodeFromID(Registers()->View);
                        ConfigNode* Query = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            View, 
                            "query"_s8
                        );
                        ConfigNode* Input = ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            Query, 
                            "input"_s8
                        );

                        ConfigNodeNewReplace(
                            APP_STATE->Config,
                            Input, 
                            Registers()->String
                        );

                        AppViewState* VS = AppViewStateFromConfig(View);

                        VS->QueryCursor = TxtPt(1, 1 + Registers()->String.Size);
                        VS->QueryMark = TxtPt(1, 1 + Registers()->String.Size);
                        MemCpy(VS->QueryBuffer, Registers()->String.Str, VS->QueryStringSize);
                    } break;

                    case APP_COMMAND_KIND_RECORD_FILE_IN_SESSION: {} break;
                }
            }
        }
    }

    if (APP_STATE->FrameDepth == 1) {
        Arena* HeadMemPool = APP_STATE->CommandMemPools[0];
        AppCommandList HeadCommands = APP_STATE->Commands[0];

        MemCpy(
            APP_STATE->CommandMemPools,
            APP_STATE->CommandMemPools + 1,
            sizeof(APP_STATE->CommandMemPools[0]) * (ARRAY_COUNT(APP_STATE->CommandMemPools) - 1)
        );
        MemCpy(
            APP_STATE->Commands,
            APP_STATE->Commands + 1,
            sizeof(APP_STATE->Commands[0]) * (ARRAY_COUNT(APP_STATE->Commands) - 1)
        );
        APP_STATE->CommandMemPools[ARRAY_COUNT(APP_STATE->CommandMemPools) - 1] = HeadMemPool;
        APP_STATE->Commands[ARRAY_COUNT(APP_STATE->Commands) - 1] = HeadCommands;
        ArenaClear(APP_STATE->CommandMemPools[0]);
        MemZero(&APP_STATE->Commands[0], sizeof(APP_STATE->Commands[0]));
        ++APP_STATE->CommandsGeneration;
    }

    f32 MasterAnimationsF = (f32) !!AppSettingFromNameB32("animations"_s8);
    f32 MenuAnimationsF = (f32) !!AppSettingFromNameB32("menu_animations"_s8);
    f32 ScrollingAnimationsF = (f32) !!AppSettingFromNameB32("scrolling_animations"_s8);
    f32 TooltipAnimationsF = (f32) !!AppSettingFromNameB32("tooltip_animations"_s8);

    APP_STATE->CatchAllAnimationRate = 1.0f - MasterAnimationsF * Pow(2.0f, (-60.0f * APP_STATE->FrameDeltaTime));
    APP_STATE->MenuAnimationRate = 1.0f - MasterAnimationsF * MenuAnimationsF * Pow(2.0f, (-70.f * APP_STATE->FrameDeltaTime));
    APP_STATE->MenuAnimationRateSlow = 1.0f - MasterAnimationsF * MenuAnimationsF * Pow(2.0f, (-50.f * APP_STATE->FrameDeltaTime));
    APP_STATE->RichHoverAnimationRate = 1.0f - MasterAnimationsF * MenuAnimationsF * Pow(2.0f, (-50.f * APP_STATE->FrameDeltaTime));
    APP_STATE->ScrollingAnimationRate = 1.0f - MasterAnimationsF * ScrollingAnimationsF * Pow(2.0f, (-60.f * APP_STATE->FrameDeltaTime));
    APP_STATE->TooltipAnimationRate = 1.0f - MasterAnimationsF * TooltipAnimationsF * Pow(2.0f, (-60.f * APP_STATE->FrameDeltaTime));

    f32 Rate = (AppSettingFromNameB32("menu_animations"_s8)) ? 1.0f - Pow(2.0f, (-30.0f * APP_STATE->FrameDeltaTime)) : 1.0f;
    b32 PopUpIsOpen = APP_STATE->PopUpActive;

    APP_STATE->PopUpT += Rate * ((f32) !!PopUpIsOpen - APP_STATE->PopUpT);

    if (AbsF32(APP_STATE->PopUpT - (f32) !!PopUpIsOpen) > 0.005f)
        AppRequestFrame();

    DBeginFrame(AppFontFromSlot(APP_FONT_SLOT_ICONS));

    ConfigNodePtrList Windows = ConfigNodeTopLevelListFromStr(
        Scratch.MemPool, 
        "window"_s8
    );

    for (
        ConfigNodePtrNode* N = Windows.Head;
        N;
        N = N->Next
    ) {
        ConfigNode* Window = N->V;
        AppWindowState* W = AppWindowStateFromConfig(Window);
        b32 IsWindowFocused = WindowIsFocused(W->WinHandle);

        if (IsWindowFocused)
            APP_STATE->LastFocusedWindow = W->CfgID;

        AppPushRegisters();
        Registers()->Window = W->CfgID;
        AppWindowFrame();
        MemSet(&W->UIEvents, 0, sizeof(W->UIEvents));

        AppRegisters* WindowRegs = AppPopRegisters();

        if (APP_STATE->LastFocusedWindow == W->CfgID)
            MemCpy(Registers(), WindowRegs, sizeof(*WindowRegs));
    }

    for (
        u64 SlotIndex = 0; 
        SlotIndex < APP_STATE->WindowStateSlotsCount; 
        ++SlotIndex
    ) {
        for (
            AppWindowState* WS = APP_STATE->WindowStateSlots[SlotIndex].Head, *Next;
            WS;
            WS = Next
        ) {
            Next = WS->HashNext;

            ConfigNode* Config = ConfigNodeFromID(WS->CfgID);

            if (
                Config == EMPTY_CFG_NODE_VALUE || 
                WS->LastFrameIndexTouched < APP_STATE->FrameIndex ||
                APP_STATE->Quit
            ) {
                UIStateRelease(WS->UI);
                RendUnequipWindow(WS->WinHandle, WS->RendHandle);
                WindowClose(WS->WinHandle);
                ArenaRelease(WS->DropCompletionMemPool);
                ArenaRelease(WS->QueryMemPool);
                ArenaRelease(WS->HoverEvalMemPool);
                ArenaRelease(WS->MemPool);
                ArenaRelease(WS->AutoCompleteMemPool);
                DLL_REMOVE_EX(
                    EMPTY_APP_WINDOW_STATE_VALUE,
                    APP_STATE->HeadWindowState,
                    APP_STATE->TailWindowState,
                    WS,
                    OrderNext,
                    OrderPrev
                );
                DLL_REMOVE_EX(
                    NULL,
                    APP_STATE->WindowStateSlots[SlotIndex].Head,
                    APP_STATE->WindowStateSlots[SlotIndex].Tail,
                    WS,
                    HashNext,
                    HashPrev
                );
                SLL_STACK_PUSH_EX(APP_STATE->FreeWindowState, WS, OrderNext);
            }
        }
    }

    if (APP_STATE->DragDropState == APP_DRAG_DROP_STATE_DROPPING)
        APP_STATE->DragDropState = APP_DRAG_DROP_STATE_NULL;

    if (APP_STATE->NumFramesRequested)
        --APP_STATE->NumFramesRequested;

    AccessClose(APP_STATE->FrameAccess);
    APP_STATE->FrameAccess = FrameAccessRestore;
    RendBeginFrame();

    for (
        AppWindowState* W = APP_STATE->HeadWindowState; 
        W != EMPTY_APP_WINDOW_STATE_VALUE; 
        W = W->OrderNext
    ) {
        RendBeginWindowFrame(W->WinHandle, W->RendHandle);
        DSubmitBucket(W->WinHandle, W->RendHandle, W->DrawBucket);
        RendEndWindowFrame(W->WinHandle, W->RendHandle);
    }

    RendEndFrame();

    if (APP_STATE->FrameDepth == 1) {
        ConfigIDList WindowsToShow = {};

        for (
            AppWindowState* W = APP_STATE->HeadWindowState; 
            W != EMPTY_APP_WINDOW_STATE_VALUE; 
            W = W->OrderNext
        ) {
            if (W->FramesAlive == 1)
                ListPush(Scratch.MemPool, &WindowsToShow, W->CfgID);
        }

        for (ConfigIDNode* N = WindowsToShow.Head; N; N = N->Next) {
            ConfigNode* Window = ConfigNodeFromID(N->V);
            AppWindowState* WS = AppWindowStateFromConfig(Window);

            WindowFirstPaint(WS->WinHandle);
        }
    }


    u64 EndTimeUSecs = TimeGetTimestamp();
    u64 FrameTimeUSecs = EndTimeUSecs - StartTimeUSecs;

    APP_STATE->FrameTimeUSecsHistory[
        APP_STATE->FrameIndex % ARRAY_COUNT(APP_STATE->FrameTimeUSecsHistory)
    ] = FrameTimeUSecs;
    ++APP_STATE->FrameIndex;
    APP_STATE->TimeSecs += APP_STATE->FrameDeltaTime;
    APP_STATE->TimeUSecs += FrameTimeUSecs;

    if (APP_STATE->FrameDepth == 1)
        ++APP_STATE->CommandsGeneration;

    if (APP_STATE->FrameIndex == 10)
        EnableMaxMemorySpace();

    --APP_STATE->FrameDepth;
    ReleaseScratch(Scratch);
}

Str8 
AppPushWindowTitle(Arena* MemPool)
{
    Str8 Result = {};

    Result = ArenaPushStrFmt(
        MemPool, 
        "%S - %s (v0.1 alpha)", 
        PRINT_STR(StrSkipLastSlash(APP_STATE->SessionPath)),
        NC_DATA_DIR
    );

    return Result;
}

ConfigNode* 
AppWindowFromConfig(ConfigNode* Config)
{
    ConfigNode* Result = EMPTY_CFG_NODE_VALUE;

    for (ConfigNode* C = Config; C != EMPTY_CFG_NODE_VALUE; C = C->Parent) {
        if (
            C->Parent->Parent == ConfigNodeRoot() && 
            StrMatch(C->String, "window"_s8, 0)
        ) {
            Result = C;
            break;
        }
    }

    return Result;
}

AppWindowState* 
AppWindowStateFromConfig(ConfigNode* Config)
{
    ConfigNode* WindowConfig = AppWindowFromConfig(Config);
    ConfigID ID = WindowConfig->ID;
    AppWindowState* WindowState = EMPTY_APP_WINDOW_STATE_VALUE;

    if (
        ID != 0 &&
        ID == APP_STATE->WindowStateLastAccessID &&
        ID == APP_STATE->WindowStateLastAccessed->CfgID
    ) {
        WindowState = APP_STATE->WindowStateLastAccessed;
    } else {
        u64 IDHash = DHash(Str8Struct(&ID));
        u64 SlotIndex = IDHash % APP_STATE->WindowStateSlotsCount;
        AppWindowStateSlot* Slot = &APP_STATE->WindowStateSlots[SlotIndex];

        for (AppWindowState* WS = Slot->Head; WS; WS = WS->HashNext) {
            if (WS->CfgID == ID) {
                WindowState = WS;
                break;
            }
        }
    }

    if (
        WindowConfig != EMPTY_CFG_NODE_VALUE && 
        WindowState == EMPTY_APP_WINDOW_STATE_VALUE
    ) {
        TempArena Scratch = GetScratch(NULL, 0);
        b32 HasPosition = FALSE;
        v2f32 Position = {};
        v2f32 Size = {};
        Handle PreferredMonitor = {};
        ConfigNode* PositionConfig = ConfigNodeChildFromStr(WindowConfig, "pos"_s8);

        HasPosition = (PositionConfig != EMPTY_CFG_NODE_VALUE);

        ConfigNode* SizeConfig = ConfigNodeChildFromStr(WindowConfig, "size"_s8);
        ConfigNode* MonitorConfig = ConfigNodeChildFromStr(WindowConfig, "monitor"_s8);

        Position.X = (f32) F64FromStr(PositionConfig->Head->String);
        Position.Y = (f32) F64FromStr(PositionConfig->Head->Next->String);
        Size.X = (f32) F64FromStr(SizeConfig->Head->String);
        Size.Y = (f32) F64FromStr(SizeConfig->Head->Next->String);

        HandleArray Monitors = ArenaPushMonitorArray(Scratch.MemPool);

        for (u64 Index = 0; Index < Monitors.Count; ++Index) {
            Str8 MonitorName = NameFromMonitor(Scratch.MemPool, Monitors.Data[Index]);

            if (StrMatch(MonitorName, MonitorConfig->Head->String, STR_MATCH_ALL_CASES)) {
                PreferredMonitor = Monitors.Data[Index];
                break;
            }
        }

        WindowState = APP_STATE->FreeWindowState;

        if (WindowState)
            SLL_STACK_POP_EX(APP_STATE->FreeWindowState, OrderNext);
        else
            WindowState = ArenaPushArray(APP_STATE->MemPool, AppWindowState, 1);

        MemSet(WindowState, 0, sizeof(*WindowState));
        WindowState->CfgID = ID;
        WindowState->MemPool = ArenaAlloc();

        Str8 Title = AppPushWindowTitle(Scratch.MemPool);

        WindowState->WinHandle = WindowOpen(
            {
                Position.X, 
                Position.Y, 
                Position.X + Size.X, 
                Position.Y + Size.Y
            },
            (!HasPosition * WINDOW_KIND_USE_DEFAULT_POSITION) | WINDOW_KIND_CUSTOM_BORDER,
            Title
        );
        WindowState->RendHandle = RendEquipWindow(WindowState->WinHandle);
        WindowState->UI = UIStateAlloc();
        WindowState->DropCompletionMemPool = ArenaAlloc();
        WindowState->QueryMemPool = ArenaAlloc();
        WindowState->HoverEvalMemPool = ArenaAlloc();
        WindowState->AutoCompleteMemPool = ArenaAlloc();
        WindowState->LastDPI = GetWindowDPI(WindowState->WinHandle);

        Handle ZeroMonitor = {};

        if (PreferredMonitor != ZeroMonitor)
            WindowSetMonitor(WindowState->WinHandle, PreferredMonitor);

        if (ConfigNodeChildFromStr(WindowConfig, "fullscreen"_s8) != EMPTY_CFG_NODE_VALUE)
            WindowSetFullScreen(WindowState->WinHandle, TRUE);

        if (ConfigNodeChildFromStr(WindowConfig, "maximised"_s8) != EMPTY_CFG_NODE_VALUE)
            WindowSetMaximised(WindowState->WinHandle, TRUE);

        u64 IDHash = DHash(Str8Struct(&ID));
        u64 SlotIndex = IDHash % APP_STATE->WindowStateSlotsCount;
        AppWindowStateSlot* Slot = &APP_STATE->WindowStateSlots[SlotIndex];

        DLL_PUSH_BACK_EX(
            EMPTY_APP_WINDOW_STATE_VALUE, 
            APP_STATE->HeadWindowState, 
            APP_STATE->TailWindowState, 
            WindowState, 
            OrderNext, 
            OrderPrev
        );
        DLL_PUSH_BACK_EX(NULL, Slot->Head, Slot->Tail, WindowState, HashNext, HashPrev);
        ReleaseScratch(Scratch);
    }

    if (WindowState != EMPTY_APP_WINDOW_STATE_VALUE) 
        WindowState->LastFrameIndexTouched = APP_STATE->FrameIndex;

    APP_STATE->WindowStateLastAccessID = WindowState->CfgID;
    APP_STATE->WindowStateLastAccessed = WindowState;

    return WindowState;
}

AppWindowState*
AppWindowStateFromHandle(Handle WinHandle)
{
    AppWindowState* WindowState = EMPTY_APP_WINDOW_STATE_VALUE;

    for (
        AppWindowState* WS = APP_STATE->HeadWindowState;
        WS != EMPTY_APP_WINDOW_STATE_VALUE;
        WS = WS->OrderNext
    ) {
        if (WS->WinHandle == WinHandle) {
            WindowState = WS;
            break;
        }
    }

    return WindowState;
}

void 
__AppSetAutoCompleteRegisters(EvalEval DstEval, AppRegisters* Regs)
{
    ConfigNode* WindowCfg = ConfigNodeFromID(Registers()->Window);
    AppWindowState* WS = AppWindowStateFromConfig(WindowCfg);

    if (WS->AutoCompletePrevFrameIndex < APP_STATE->FrameIndex) {
        ArenaClear(WS->AutoCompleteMemPool);

        b32 IsAllowed = FALSE;
        AppAutoCompleteCursorInfo CursorInfo = {};

        {
            TempArena Scratch = GetScratch(NULL, 0);
            b32 ForceAllow = FALSE;
            b32 ExprBasedReplace = TRUE;
            Str8 ListExpr = "query:views"_s8;

            {
                ConfigNode* Parent = AppConfigFromEvalSpace(DstEval.Space);
                Str8 ChildKey = EvalStrFromID(DstEval.Space.U64s[1]);
                MDNodePtrList Schemas = ConfigSchemasFromName(
                    Scratch.MemPool,
                    APP_STATE->CfgSchemaTable,
                    Parent->String
                );
                MDNode* ChildSchema = EMPTY_MD_NODE_VALUE;

                for (
                    MDNodePtrNode* Node = Schemas.Head;
                    Node && MDNodeIsEmpty(ChildSchema);
                    Node = Node->Next
                ) {
                    ChildSchema = MDChildFromStr(
                        Node->V,
                        ChildKey,
                        0
                    );
                }

                if (StrMatch(ChildKey, "theme"_s8, 0)) {
                    ListExpr = "query:themes"_s8;
                    ExprBasedReplace = FALSE;
                    ForceAllow = TRUE;
                } else if (!StrMatch(ChildSchema->Head->String, "expr_string"_s8, 0)) {
                    MemZero(&ListExpr, sizeof(ListExpr));
                }
            }

            IsAllowed = (ForceAllow || AppSettingFromNameB32("autocompletion_lister"_s8));

            Str8 Filter = Regs->String;
            r1u64 ReplacedRange = Rng(0ULL, Filter.Size);
            Str8 CalleeExpr = {};
            u64 CursorArgIndex = 0;

            if (ExprBasedReplace) {
                u64 CursorOffset = (u64) (Regs->Cursor.Column - 1);
                EvalParse Parse = EvalParseFromStr(Regs->String);
                EvalExpr* CursorExpr = EMPTY_EVAL_EXPR_VALUE;
                EvalExpr* CursorExprParent = EMPTY_EVAL_EXPR_VALUE;

                {
                    struct ExprWalkTask {
                        ExprWalkTask* Next;
                        EvalExpr*     Parent;
                        EvalExpr*     Expr;
                        i32           Depth;
                    };

                    ExprWalkTask StartTask = {
                        NULL,
                        EMPTY_EVAL_EXPR_VALUE,
                        Parse.Expr
                    };
                    ExprWalkTask* HeadTask = &StartTask;
                    ExprWalkTask* TailTask = HeadTask;
                    i32 BestDepth = 0;

                    for (
                        EvalExpr* Chain = Parse.Expr->Next;
                        Chain != EMPTY_EVAL_EXPR_VALUE;
                        Chain = Chain->Next
                    ) {
                        ExprWalkTask* Task = ArenaPushArrayZero(
                            Scratch.MemPool,
                            ExprWalkTask,
                            1
                        );

                        SLL_QUEUE_PUSH(HeadTask, TailTask, Task);
                        Task->Parent = EMPTY_EVAL_EXPR_VALUE;
                        Task->Expr = Chain;
                    }

                    for (ExprWalkTask* T = HeadTask; T; T = T->Next) {
                        EvalExpr* E = T->Expr;

                        if (
                            T->Depth >= BestDepth &&
                            (
                                InRange(E->Range, CursorOffset) || 
                                CursorOffset == E->Range.Max
                            )
                        ) {
                            CursorExprParent = T->Parent;
                            CursorExpr = E;
                            BestDepth = T->Depth;
                        }

                        for (
                            EvalExpr* Child = E->Head;
                            Child != EMPTY_EVAL_EXPR_VALUE;
                            Child = Child->Next
                        ) {
                            ExprWalkTask* Task = ArenaPushArrayZero(
                                Scratch.MemPool,
                                ExprWalkTask,
                                1
                            );

                            SLL_QUEUE_PUSH(HeadTask, TailTask, Task);
                            Task->Parent = E;
                            Task->Expr = Child;
                            Task->Depth = T->Depth + 1;
                        }
                    }
                }

                if (CursorExprParent->Kind == EVAL_EXPR_KIND_CALL) {
                    EvalKey CalleeKey = EvalKeyFromExpr(CursorExprParent->Head);

                    CalleeExpr = EvalFullExprFromKey(Scratch.MemPool, CalleeKey);

                    for (
                        EvalExpr* Arg = CursorExpr->Prev;
                        Arg != CursorExprParent->Head && Arg != EMPTY_EVAL_EXPR_VALUE;
                        Arg = Arg->Prev
                    ) {
                        ++CursorArgIndex;
                    }
                } else if (CursorExpr->Kind == EVAL_EXPR_KIND_CALL) {
                    EvalKey CalleeKey = EvalKeyFromExpr(CursorExpr->Head);

                    CalleeExpr = EvalFullExprFromKey(Scratch.MemPool, CalleeKey);

                    for (
                        EvalExpr* Arg = CursorExpr->Head->Next;
                        Arg != EMPTY_EVAL_EXPR_VALUE;
                        Arg = Arg->Next
                    ) {
                        ++CursorArgIndex;
                    }
                }

                b32 DidSpecialCursorCase = FALSE;

                if (!DidSpecialCursorCase) {
                    EvalExpr* DotExpr = EMPTY_EVAL_EXPR_VALUE;

                    if (
                        CursorExprParent->Kind == EVAL_EXPR_KIND_MEMBER_ACCESS &&
                        CursorExpr == CursorExprParent->Head->Next
                    ) {
                        DotExpr = CursorExpr;
                    } else if (
                        CursorExprParent->Kind == EVAL_EXPR_KIND_MEMBER_ACCESS &&
                        CursorOffset == CursorExpr->Range.Max
                    ) {
                        DotExpr = CursorExprParent;
                    }

                    if (DotExpr != EMPTY_EVAL_EXPR_VALUE) {
                        DidSpecialCursorCase = TRUE;

                        EvalEval LHSEval = EvalEvalFromExpr(DotExpr->Head);
                        EvalEval TypeOfLHSEval = EvalEvalWrap(
                            LHSEval,
                            "typeof($)"_s8
                        );

                        ListExpr = EvalFullExprFromKey(Scratch.MemPool, TypeOfLHSEval.Key);
                        Filter = CursorExpr->String;
                        ReplacedRange = Union(
                            DotExpr->Range,
                            CursorExpr->Range
                        );
                    }
                }

                if (
                    !DidSpecialCursorCase && 
                    CursorExpr->Kind == EVAL_EXPR_KIND_LEAF_IDENT
                ) {
                    DidSpecialCursorCase = TRUE;
                    Filter = StrPrefix(
                        CursorExpr->String,
                        CursorOffset - CursorExpr->Range.Min
                    );
                    ReplacedRange = CursorExpr->Range;
                }
            }

            MDNode* ArgSchema = EMPTY_MD_NODE_VALUE;

            if (CalleeExpr.Size) {
                EvalEval CalleeEval = EvalEvalFromStrFmt(
                    "view:%S",
                    PRINT_STR(CalleeExpr)
                );
                EvalType* CalleeType = EvalTypeFromKey(CalleeEval.IRTree.TypeKey);

                if (CalleeType->Kind == EVAL_TYPE_KIND_LENS_SPEC) {
                    u64 ArgIndex = 0;
                    MDNodePtrList Schemas = ConfigSchemasFromName(
                        Scratch.MemPool,
                        APP_STATE->CfgSchemaTable,
                        CalleeType->Name
                    );

                    for (
                        MDNodePtrNode* Node = Schemas.Head;
                        Node;
                        Node = Node->Next
                    ) {
                        MDNode* Schema = Node->V;

                        for EACH_MDNODE(Child, Schema->Head) {
                            if (!MDNodeHasTag(Child, "no_callee_helper"_s8, 0)) {
                                if (CursorArgIndex == ArgIndex) {
                                    ArgSchema = Child;

                                    goto END_SCHEMA_SEARCH;
                                }

                                ++ArgIndex;
                            }
                        }
                    }

END_SCHEMA_SEARCH:;
                }
            }

            CursorInfo.ListExpr = ArenaPushStrCpy(
                WS->AutoCompleteMemPool,
                ListExpr
            );
            CursorInfo.Filter = ArenaPushStrCpy(
                WS->AutoCompleteMemPool,
                Filter
            );
            CursorInfo.ReplacedRange = ReplacedRange;
            CursorInfo.CalleeExpr = ArenaPushStrFmt(
                WS->AutoCompleteMemPool,
                "view:%S",
                PRINT_STR(CalleeExpr)
            );
            CursorInfo.ArgSchema = ArgSchema;
            ReleaseScratch(Scratch);
        }

        if (IsAllowed) {
            WS->AutoCompletePrevFrameIndex = APP_STATE->FrameIndex;
            WS->AutoCompleteRegs = CopyRegisters(
                WS->AutoCompleteMemPool,
                Regs
            );
            WS->AutoCompleteCursorInfo = CursorInfo;
        }
    }
}

NC_NO_OPTIMISE_BEGIN //________________________________________________________

UISignal
AppLabel(Str8 String)
{
    TempArena Scratch = GetScratch(NULL, 0);
    FancyStrList FStrings = AppFStrFromRichString(Scratch.MemPool, String);
    UIBox* Box = UIBuildBoxFromKey(
        UI_BOX_KIND_DRAW_TEXT,
        EMPTY_UI_KEY_VALUE
    );

    UIBoxEquipDisplayFancyStrs(Box, &FStrings);

    UISignal Result = UISignalFromBox(Box);

    ReleaseScratch(Scratch);

    return Result;
}

Str8
AppLabelFromConfig(ConfigNode* Config)
{
    ConfigNode* LabelRoot = ConfigNodeChildFromStr(Config, "label"_s8);
    Str8 Result = LabelRoot->Head->String;

    return Result;
}

Str8
AppFilePathFromConfig(ConfigNode* Config)
{
    ConfigNode* Root = ConfigNodeChildFromStr(Config, "path"_s8);
    Str8 Result = Root->Head->String;

    return Result;
}

Str8
AppNameFromConfig(ConfigNode* Config)
{
    ConfigNode* NameRoot = ConfigNodeChildFromStr(Config, "name"_s8);
    Str8 Result = NameRoot->Head->String;

    return Result;
}

Str8
AppQueryFromEvalStr(Arena *MemPool, Str8 String) 
{
    Str8 Result = {};
    TempArena Scratch = GetScratch(&MemPool, 1);
    EvalExpr* Expr = EvalParseFromStr(String).Expr;

    if (
        Expr->Kind == EVAL_EXPR_KIND_LEAF_IDENT &&
        StrMatch(Expr->Qualifier, "query"_s8, 0)
    ) {
        Result = Expr->String;
    }

    ReleaseScratch(Scratch);

    return Result;
}

FancyStrList
AppTitleFStrFromConfig(Arena* MemPool, ConfigNode* Config, b32 IncludeExtras)
{
    FancyStrList Result = {};
    TempArena Scratch = GetScratch(&MemPool, 1);
    b32 IsDisabled = AppIsDisabled(Config);
    AppLocation Location = AppLocationFromConfig(Config);
    Str8 NameString = AppNameFromConfig(Config);
    Str8 LabelString = AppLabelFromConfig(Config);
    Str8 ExprString = AppExpressionFromConfig(Config);
    Str8 CollectionName = {};
    Str8 FilePath = AppFilePathFromConfig(Config);
    v4f32 ColourRGBA = AppColourFromConfig(Config);

    if (ColourRGBA.W == 0.0f)
        ColourRGBA = UIColourFromName("text"_s8);

    v4f32 SecondaryColourRGBA = ColourRGBA;

    UITag("weak"_s8) {
        SecondaryColourRGBA = UIColourFromName("text"_s8);
    }

    AppIconKind IconKind = AppIconKindFromCodeName(Config->String);
    b32 IsFromCLI = FALSE;

    {
        ConfigNode* CLIRoot = ConfigNodeChildFromStr(
            ConfigNodeRoot(),
            "command_line"_s8
        );

        for (
            ConfigNode* ParentNode = Config->Parent;
            ParentNode != EMPTY_CFG_NODE_VALUE;
            ParentNode = ParentNode->Parent
        ) {
            if (CLIRoot == ParentNode) {
                IsFromCLI = TRUE;
                break;
            }
        }
    }

    b32 IsWithinWindow = FALSE;

    {
        for (
            ConfigNode* ParentNode = Config->Parent;
            ParentNode != EMPTY_CFG_NODE_VALUE;
            ParentNode = ParentNode->Parent
        ) {
            if (StrMatch(ParentNode->String, "window"_s8, 0)) {
                IsWithinWindow = TRUE;
                break;
            }
        }
    }

    if (ExprString.Size) {
        Str8 QueryName = AppQueryFromEvalStr(MemPool, ExprString);

        if (QueryName.Size) {
            Str8 QueryCodeName = QueryName;
            Str8 QueryDisplayName = AppDisplayFromCodeName(QueryCodeName);

            CollectionName = QueryDisplayName;

            if (!QueryDisplayName.Size) {
                QueryCodeName = AppSingularFromCodeNamePlural(QueryName);
                CollectionName = AppDisplayPluralFromCodeName(QueryCodeName);
            }

            AppIconKind QueryIconKind = AppIconKindFromCodeName(QueryCodeName);

            if (QueryIconKind != APP_ICON_KIND_NULL)
                IconKind = QueryIconKind;
        } else {
            FilePath = AppFilePathFromEvalStr(MemPool, ExprString);

            if (FilePath.Size)
                IconKind = APP_ICON_KIND_FILE_OUTLINE;
        }
    }

    FancyStrParams Params = {
        AppFontFromSlot(APP_FONT_SLOT_MAIN),
        AppRasterKindFromSlot(APP_FONT_SLOT_MAIN),
        ColourRGBA,
        UIHeadFontSize()
    };
    b32 RunningIsSecondary = FALSE;

#define START_SECONDARY() if (!RunningIsSecondary) { RunningIsSecondary = TRUE; Params.Colour = SecondaryColourRGBA; Params.Size = UIHeadFontSize() * 0.95f; }
    
    if (IsDisabled)
        Params.Colour = SecondaryColourRGBA;

    if (IconKind != APP_ICON_KIND_NULL) {
        FancyStrParams Overrides = {};

        Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
        Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
        Overrides.Colour = SecondaryColourRGBA;

        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            APP_ICON_KIND_TEXT_TABLE[IconKind]
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
    }

    if (
        IsWithinWindow &&
        !FilePath.Size &&
        !CollectionName.Size &&
        !LabelString.Size
    ) {
        Str8 ViewDisplayName = AppDisplayFromCodeName(Config->String);

        if (ViewDisplayName.Size) {
            ListPush(
                MemPool,
                &Result,
                &Params,
                NULL,
                ViewDisplayName
            );
            ListPush(
                MemPool,
                &Result,
                &Params,
                NULL,
                "  "_s8
            );
            START_SECONDARY();
        }
    }

    if (Config->Parent == ConfigNodeRoot()) {
        if (StrMatch(Config->String, "session"_s8, 0)) {
            FancyStrParams Overrides = {};

            Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_MAIN);
            Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_MAIN);

            ListPush(
                MemPool,
                &Result,
                &Params,
                &Overrides,
                "session"_s8
            );
            ListPush(
                MemPool,
                &Result,
                &Params,
                NULL,
                "  "_s8
            );
            START_SECONDARY();
        }
    }

    if (NameString.Size) {
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            NameString
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        START_SECONDARY();
    }

    if (LabelString.Size) {
        FancyStrParams Overrides = {};

        Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_CODE);
        Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_CODE);

        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            LabelString
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        START_SECONDARY();
    }

    if (CollectionName.Size) {
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            CollectionName
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        START_SECONDARY();
    } else if (FilePath.Size) {
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            ArenaPushStrCpy(
                MemPool,
                StrSkipLastSlash(FilePath)
            )
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        START_SECONDARY();
    } else if (ExprString.Size) {
        FancyStrParams Overrides = {};

        Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_CODE);
        Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_CODE);

        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            ExprString
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        START_SECONDARY();
    }

    if (Location.FilePath.Size) {
        Str8 Path = Location.FilePath;

        if (!IncludeExtras)
            Path = StrSkipLastSlash(Location.FilePath);

        Str8 LocationString = ArenaPushStrFmt(
            MemPool,
            "%S:%lld:%lld",
            PRINT_STR(Path),
            Location.Point.Line,
            Location.Point.Column
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            LocationString
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        START_SECONDARY();
    }

    // if (Location.Expression.Size) {
    //     AppFont(APP_FONT_SLOT_CODE) {
    //         FancyStrList FancyStrings = AppFStrFromPIDStr(
    //             MemPool,
    //             1.0f,
    //             0,
    //             Params.Colour,
    //             Location.Expression
    //         );

    //         ListCat(&Result, &FancyStrings);
    //     }

    //     ListPush(
    //         MemPool,
    //         &Result,
    //         &Params,
    //         NULL,
    //         "  "_s8
    //     );
    //     START_SECONDARY();
    // }

    // {
    //     Str8 Condition = ConfigNodeChildFromStr(
    //         Config,
    //         "condition"_s8
    //     )->Head->String;

    //     if (Condition.Size) {
    //         FancyStrParams Overrides = {};

    //         Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_CODE);
    //         Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_CODE);

    //         ListPush(
    //             MemPool,
    //             &Result,
    //             &Params,
    //             &Overrides,
    //             "if "_s8
    //         );

    //         AppFont(APP_FONT_SLOT_CODE) {
    //             FancyStrList FancyStrings = AppFStrFromPIDStr(
    //                 MemPool,
    //                 1.0f,
    //                 0,
    //                 Params.Colour,
    //                 Condition
    //             );

    //             ListCat(&Result, &FancyStrings);
    //         }

    //         ListPush(
    //             MemPool,
    //             &Result,
    //             &Params,
    //             NULL,
    //             "  "_s8
    //         );
    //     }
    // }

    if (IsDisabled) {
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "(Disabled)"_s8
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
    }

    if (StrMatch(Config->String, "file_path_map"_s8, 0)) {
        Str8 SrcString = ConfigNodeChildFromStr(
            Config,
            "source"_s8
        )->Head->String;
        Str8 DstString = ConfigNodeChildFromStr(
            Config,
            "dest"_s8
        )->Head->String;
        v4f32 SrcColour = ColourRGBA;
        v4f32 DstColour = ColourRGBA;

        if (!SrcString.Size) {
            SrcString = "(source path)"_s8;
            DstColour = SecondaryColourRGBA;
        }

        FancyStrParams Overrides = {};

        Overrides.Colour = SrcColour;

        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            SrcString
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        Overrides = {};
        Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
        Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
        Overrides.Colour = SecondaryColourRGBA;
        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_RIGHT_ARROW]
        );
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );
        Overrides = {};
        Overrides.Colour = DstColour;
        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            DstString
        );
    }

    if (StrMatch(Config->String, "theme_colour"_s8, 0)) {
        Str8 Tags = ConfigNodeChildFromStr(
            Config,
            "tags"_s8
        )->Head->String;
        Str8 ColourString = ConfigNodeChildFromStr(
            Config,
            "value"_s8
        )->Head->String;
        u32 ColourU32 = EvalValueFromStrFmt(
            "(uint32)(%S)",
            PRINT_STR(ColourString)
        ).U32;
        v4f32 Colour = LinearFromSRGBA(RGBAFromU32(ColourU32));

        if (Tags.Size) {
            ListPush(
                MemPool,
                &Result,
                &Params,
                NULL,
                Tags
            );
        } else {
            FancyStrParams Overrides = {};

            Overrides.Colour = SecondaryColourRGBA;
            ListPush(
                MemPool,
                &Result,
                &Params,
                &Overrides,
                "Colour"_s8
            );
        }

        ListPush(
            MemPool,
            &Result,
            &Params,
            NULL,
            "  "_s8
        );

        FancyStrParams Overrides = {};

        Overrides.Font = AppFontFromSlot(APP_FONT_SLOT_ICONS);
        Overrides.RasterKind = AppRasterKindFromSlot(APP_FONT_SLOT_ICONS);
        Overrides.Colour = Colour;
        ListPush(
            MemPool,
            &Result,
            &Params,
            &Overrides,
            APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_CIRCLE_FILLED]
        );
    }

#undef START_SECONDARY

    ReleaseScratch(Scratch);

    return Result;
}

UISignal
AppErrorLabel(Str8 String)
{
    UIBox* Box = UIBuildBoxFromKey(0, {});
    UISignal Sig = UISignalFromBox(Box);

    UIParent(Box) {
        UISetNextFont(AppFontFromSlot(APP_FONT_SLOT_ICONS));
        UISetNextTextRasterKind(FONT_RASTER_KIND_SMOOTH);
        UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);

        UITagFmt((char*) "weak") {
            UIPreferredWidth(UI_EM(2.25f, 1.0f)) {
                UILabel(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_WARNING_BIG]);
            }
        }

        UIPreferredWidth(UI_TEXT_DIM(10, 0)) {
            UILabel(String);
        }
    }

    return Sig;
}

void
AppCommandBindingButtons(
    Str8 Name, 
    Str8 Filter, 
    u64 Limit, 
    AppCommandBindingButtonFlag Flags
) {
    TempArena Scratch = GetScratch(NULL, 0);
    ConfigInputMapNodePtrList InputMapNodes = ConfigInputMapNodePtrListFromName(
        Scratch.MemPool,
        APP_STATE->KeyMap,
        Name
    );
    u64 InputMapIndex = 0;

    UICornerRadius(UIHeadFontSize() * 0.5f) {
        for (
            ConfigInputMapNodePtr* N = InputMapNodes.Head; 
            N; 
            N = N->Next, ++InputMapIndex
        ) {
            if (InputMapIndex >= Limit)
                break;

            UISpacer(UI_EM(1.0f, 1.0f));

            ConfigBinding Binding = N->V->Binding;
            b32 RebindingActiveForThisBinding = (
                APP_STATE->BindChangeActive &&
                StrMatch(APP_STATE->BindChangeCommandName, Name, 0) &&
                N->V->ID == APP_STATE->BindChangeBindingID
            );
            b32 HasConflicts = FALSE;
            ConfigInputMapNodePtrList NodesWithThisBinding = ConfigInputMapNodePtrListFromBinding(
                Scratch.MemPool,
                APP_STATE->KeyMap,
                Binding
            );

            {
                for (
                    ConfigInputMapNodePtr* N2 = NodesWithThisBinding.Head;
                    N2;
                    N2 = N2->Next
                ) {
                    if (!StrMatch(N->V->Name, N2->V->Name, 0)) {
                        HasConflicts = TRUE;
                        break;
                    }
                }
            }

            Str8 KeyBindingString = {};

            {
                if (Binding.Input != INPUT_KIND_NULL) {
                    KeyBindingString = StrFromInputModifierInput(
                        Scratch.MemPool,
                        Binding.ModKind,
                        Binding.Input
                    );
                } else {
                    KeyBindingString = "[no binding]"_s8;
                }
            }

            FMRangeList Matches = {};

            if (Filter.Size) {
                Matches = FuzzyFind(
                    Scratch.MemPool,
                    Filter,
                    KeyBindingString
                );
            }

            UISetNextTag(HasConflicts ? "bad_pop"_s8 : RebindingActiveForThisBinding ? "pop"_s8 : ""_s8);
            UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);
            UISetNextGroupKey(EMPTY_UI_KEY_VALUE);
            UISetNextPreferredWidth(UI_TEXT_DIM(UIHeadFontSize(), 1.0f));

            UIBox* Box = UIBuildBoxFromStrFmt(
                (
                    UI_BOX_KIND_DRAW_TEXT |
                    (!(Flags & APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT) * UI_BOX_KIND_CLICKABLE) |
                    UI_BOX_KIND_DRAW_ACTIVE_EFFECTS |
                    UI_BOX_KIND_DRAW_HOT_EFFECTS |
                    UI_BOX_KIND_DRAW_BORDER |
                    UI_BOX_KIND_DRAW_BACKGROUND
                ),
                "%S###bind_btn_%S_%x_%x",
                PRINT_STR(KeyBindingString),
                PRINT_STR(Name),
                Binding.Input,
                Binding.ModKind
            );

            UIBoxEquipFuzzyMatchRanges(Box, &Matches);

            UISignal Sig = UISignalFromBox(Box);

            {
                if (!APP_STATE->BindChangeActive && UI_CLICKED(Sig)) {
                    if (
                        (Binding.Input == INPUT_KIND_ESC || Binding.Input == INPUT_KIND_DELETE) &&
                        !Binding.ModKind
                    ) {
                        LogError("Cannot rebind; this command uses a reserved keybinding"_s8);
                    } else {
                        ArenaClear(APP_STATE->BindChangeMemPool);
                        APP_STATE->BindChangeActive = TRUE;
                        APP_STATE->BindChangeCommandName = ArenaPushStrCpy(
                            APP_STATE->BindChangeMemPool,
                            Name
                        );
                        APP_STATE->BindChangeBindingID = N->V->ID;
                    }
                } else if (APP_STATE->BindChangeActive && UI_CLICKED(Sig)) {
                    APP_STATE->BindChangeActive = FALSE;
                }

                if (
                    UI_HOVERING(Sig) &&
                    HasConflicts
                ) {
                    UITooltip() {
                        UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                            AppErrorLabel("This binding conflicts with those for:"_s8);

                            for (
                                ConfigInputMapNodePtr* N2 = NodesWithThisBinding.Head;
                                N2;
                                N2 = N2->Next
                            ) {
                                if (!StrMatch(N2->V->Name, N->V->Name, 0)) {
                                    Str8 DisplayName = AppDisplayFromCodeName(N2->V->Name);

                                    UILabel(
                                        "%S",
                                        PRINT_STR(DisplayName)
                                    );
                                }
                            }
                        }
                    }
                }
            }

            if (RebindingActiveForThisBinding) {
                UIPreferredWidth(UI_EM(2.5f, 1.0f)) {
                    UITag("bad_pop"_s8) {
                        UISetNextGroupKey(EMPTY_UI_KEY_VALUE);

                        UISignal Sig = AppIconButton(
                            APP_ICON_KIND_X,
                            NULL,
                            "###delete_binding"_s8
                        );

                        if (UI_CLICKED(Sig)) {
                            ConfigNodeRelease(
                                APP_STATE->Config,
                                ConfigNodeFromID(APP_STATE->BindChangeBindingID)
                            );
                            APP_STATE->BindChangeActive = FALSE;
                        }
                    }
                }
            }
        }
    }

    if (Flags & APP_COMMAND_BINDING_BTN_FLAG_ADD_NEW) {
        b32 AddingNewBinding = (
            APP_STATE->BindChangeActive &&
            StrMatch(APP_STATE->BindChangeCommandName, Name, 0) &&
            !APP_STATE->BindChangeBindingID
        );

        UISpacer(UI_EM(1.0f, 1.0f));

        AppFont(APP_FONT_SLOT_ICONS) {
            UITag(AddingNewBinding ? "pop"_s8 : ""_s8) {
                UICornerRadius(UIHeadFontSize() * 0.5f) {
                    UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);
                    UISetNextGroupKey(EMPTY_UI_KEY_VALUE);
                    UISetNextPreferredWidth(UI_TEXT_DIM(UIHeadFontSize() * 1.5f, 1.0f));

                    UIBox* Box = UIBuildBoxFromStrFmt(
                        (
                            UI_BOX_KIND_DRAW_TEXT |
                            UI_BOX_KIND_CLICKABLE |
                            UI_BOX_KIND_DRAW_ACTIVE_EFFECTS |
                            UI_BOX_KIND_DRAW_HOT_EFFECTS |
                            UI_BOX_KIND_DRAW_BORDER |
                            UI_BOX_KIND_DRAW_BACKGROUND
                        ),
                        "%S###add_binding",
                        PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_ADD])
                    );
                    UISignal Sig = UISignalFromBox(Box);

                    if (UI_CLICKED(Sig)) {
                        if (!AddingNewBinding && UI_CLICKED(Sig)) {
                            ArenaClear(APP_STATE->BindChangeMemPool);
                            APP_STATE->BindChangeActive = TRUE;
                            APP_STATE->BindChangeCommandName = ArenaPushStrCpy(
                                APP_STATE->BindChangeMemPool,
                                Name
                            );
                            APP_STATE->BindChangeBindingID = 0;
                        } else if (AddingNewBinding && UI_CLICKED(Sig)) {
                            APP_STATE->BindChangeActive = FALSE;
                        }
                    }
                }
            }
        }
    }

    ReleaseScratch(Scratch);
}


UISignal
AppCommandSpecButton(Str8 Name)
{
    AppCommandKindInfo* Info = AppCommandKindInfoFromStr(Name);

    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStrFmt(
        (
            UI_BOX_KIND_DRAW_BORDER |
            UI_BOX_KIND_DRAW_BACKGROUND |
            UI_BOX_KIND_DRAW_HOT_EFFECTS |
            UI_BOX_KIND_DRAW_ACTIVE_EFFECTS |
            UI_BOX_KIND_CLICKABLE
        ),
        "###cmd_%p",
        Info
    );

    UIParent(Box) {
        UIHeightFill() {
            UIPadding(UI_EM(1.0f, 1.0f)) {
                AppIconKind CanonicalIcon = AppIconKindFromCodeName(Name);

                if (CanonicalIcon != APP_ICON_KIND_NULL) {
                    AppFont(APP_FONT_SLOT_ICONS) {
                        UIPreferredWidth(UI_EM(2.0f, 1.0f)) {
                            UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                UITag("weak"_s8) {
                                    UILabel(APP_ICON_KIND_TEXT_TABLE[CanonicalIcon]);
                                }
                            }
                        }
                    }
                }

                UIPreferredWidth(UI_TEXT_DIM(10, 1.0f)) {
                    UIFlags(UI_BOX_KIND_DRAW_TEXT_FASTPATH_CODEPOINT) {
                        UIFastpathCodepoint(Box->FastpathCodepoint) {
                            UILabel(AppDisplayFromCodeName(Name));
                        }
                    }

                    UISpacer(UI_PERCENT(1, 0));
                    UISetNextFlags(UI_BOX_KIND_CLICKABLE);
                    UISetNextGroupKey({});

                    UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                        UIFontSize(UIHeadFontSize() * 0.95f) {
                            UIHeightFill() {
                                UINamedRow("###bindings"_s8) {
                                    UITag("weak"_s8) {
                                        UIFastpathCodepoint(0) {
                                            AppCommandBindingButtons(
                                                Name, 
                                                ""_s8, 
                                                U64_MAX, 
                                                APP_COMMAND_BINDING_BTN_FLAG_ADD_NEW
                                            );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    UISignal Sig = UISignalFromBox(Box);

    return Sig;
}

UISignal
AppMenuBarButton(Str8 String)
{
    UIBox* Box = UIBuildBoxFromStr(
        (
            UI_BOX_KIND_DRAW_TEXT |
            UI_BOX_KIND_DRAW_BACKGROUND |
            UI_BOX_KIND_CLICKABLE |
            UI_BOX_KIND_DRAW_HOT_EFFECTS
        ),
        String
    );

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

void*
AppViewStateFromSize(u64 Size)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* VS = AppViewStateFromConfig(View);

    if (!VS->UserData)
        VS->UserData = ArenaPushArrayZero(VS->MemPool, u8, Size);

    return VS->UserData;
}

Arena*
AppPushViewMemPool(void)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    AppViewState* VS = AppViewStateFromConfig(View);
    AppArenaExt* Ext = ArenaPushArrayZero(VS->MemPool, AppArenaExt, 1);

    Ext->MemPool = ArenaAlloc();
    SLL_QUEUE_PUSH(VS->HeadArenaExt, VS->TailArenaExt, Ext);

    return Ext->MemPool;
}

void
AppStoreViewExprStr(Str8 String)
{
    ConfigNode* View = ConfigNodeFromID(Registers()->View);
    ConfigNode* Expr = ConfigNodeChildFromStrOrAlloc(
        APP_STATE->Config,
        View,
        "expression"_s8
    );

    ConfigNodeNewReplace(APP_STATE->Config, Expr, String);
}

Str8
AppEvalStrFromFilePath(Arena* MemPool, Str8 String)
{
    TempArena Scratch = GetScratch(&MemPool, 1);
    Str8 StringEscaped = EscapedFromRawStr8(Scratch.MemPool, String);
    Str8 Result = ArenaPushStrFmt(
        MemPool,
        "file:\"%S\".data",
        PRINT_STR(StringEscaped)
    );

    ReleaseScratch(Scratch);

    return Result;
}

void 
AppWindowFrame(void)
{
    TempArena Scratch = GetScratch(NULL, 0);
    ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
    AppWindowState* WS = AppWindowStateFromConfig(ConfigNodeFromID(Registers()->Window));
    ConfigPanelTree PanelTree = ConfigPanelTreeFromConfig(Scratch.MemPool, Window);
    b32 IsWindowFocused = WindowIsFocused(WS->WinHandle);
    b32 PopUpIsOpen = APP_STATE->PopUpActive;
    b32 QueryIsOpen = WS->QueryIsActive;
    u64 HoverEvalOpenDelayUSecs = MILLISECONDS(400);
    b32 HoverEvalIsOpen = (
        !PopUpIsOpen &&
        !QueryIsOpen &&
        WS->HoverEvalString.Size &&
        WS->HoverEvalFirstTimeUSecs + HoverEvalOpenDelayUSecs < WS->HoverEvalLastTimeUSecs &&
        APP_STATE->TimeUSecs - WS->HoverEvalLastTimeUSecs < HoverEvalOpenDelayUSecs
    );

    if (!IsWindowFocused || PopUpIsOpen)
        WS->MenuBarKeyHeld = FALSE;

    UISelectState(WS->UI);
    Registers()->Panel = PanelTree.Focused->Config->ID;
    Registers()->Tab = PanelTree.Focused->SelectedTab->ID;
    Registers()->View = PanelTree.Focused->SelectedTab->ID;

    // NOTE(nathan): Compute window theme
    {
        Access* Acc = AccessOpen();
        ConfigNode* Session = ConfigNodeChildFromStr(ConfigNodeRoot(), "session"_s8);
        ConfigNode* Window = ConfigNodeChildFromStr(Session, "window"_s8);
        ConfigNodePtrList ColoursConfigs = {};

        for (
            ConfigNode* Child = Window->Head; 
            Child != EMPTY_CFG_NODE_VALUE; 
            Child = Child->Next
        ) {
            if (StrMatch(Child->String, "theme_colour"_s8, 0))
                ListPushFront(Scratch.MemPool, &ColoursConfigs, Child);
        }

        b32 ShouldUseSessionTheme = AppSettingFromNameB32("use_session_theme"_s8);
        ConfigNode* ThemeConfig = EMPTY_CFG_NODE_VALUE;

        if (ShouldUseSessionTheme)
            ThemeConfig = ConfigNodeChildFromStr(Session, "theme"_s8);
        else
            ThemeConfig = ConfigNodeChildFromStr(Window, "theme"_s8);

        Str8 ThemeName = (ThemeConfig != EMPTY_CFG_NODE_VALUE && ThemeConfig->Head != EMPTY_CFG_NODE_VALUE)
            ? ThemeConfig->Head->String
            : "Ayu Dark"_s8;
        MDNode* ThemeTree = AppThemeTreeFromName(Scratch.MemPool, Acc, ThemeName);

        if (ThemeTree == EMPTY_MD_NODE_VALUE)
            ThemeTree = APP_STATE->ThemePresetTrees[APP_THEME_PRESET_AYU_DARK];

        struct ThemeTask {
            ThemeTask* Next;
            MDNode* Tree;
        };

        ThemeTask StartTask = {
            NULL,
            ThemeTree
        };
        ThemeTask* HeadTask = &StartTask;
        ThemeTask* TailTask = HeadTask;

        for (
            ConfigNodePtrNode* Node = ColoursConfigs.Head; 
            Node; 
            Node = Node->Next
        ) {
            ThemeTask* T = ArenaPushArrayZero(Scratch.MemPool, ThemeTask, 1);

            SLL_QUEUE_PUSH_FRONT(HeadTask, TailTask, T);
            T->Tree = MDTreeFromStr(
                Scratch.MemPool,
                ConfigStrFromTree(
                    Scratch.MemPool, 
                    APP_STATE->CfgSchemaTable, 
                    ""_s8, 
                    Node->V
                )
            );
        }

        struct ThemePatternNode {
            ThemePatternNode* Next;
            UIThemePattern Pattern;
        };

        ThemePatternNode* HeadPattern = NULL;
        ThemePatternNode* TailPattern = NULL;
        u64 PatternCount = 0;

        for (ThemeTask* T = HeadTask; T; T = T->Next) {
            MDNode* TreeRoot = T->Tree;

            for (
                MDNode* N = TreeRoot;
                !MDNodeIsEmpty(N);
                N = MDNodeRecordDepthFirstPreOrder(N, TreeRoot).Next
            ) {
                if (StrMatch(N->String, "theme_colour"_s8, 0)) {
                    MDNode* TagsChild = MDChildFromStr(N, "tags"_s8, 0);
                    MDNode* ValueChild = MDChildFromStr(N, "value"_s8, 0);
                    u8 SplitChar = ' ';
                    Str8List Tags = StrSplit(
                        Scratch.MemPool, 
                        TagsChild->Head->String, 
                        &SplitChar, 
                        1, 
                        FALSE
                    );

                    u32 ColourU32 = U32FromStr(ValueChild->Head->String, 16);
                    v4f32 ColourLinear = LinearFromSRGBA(RGBAFromU32(ColourU32));
                    ThemePatternNode* Node = ArenaPushArrayZero(
                        Scratch.MemPool, 
                        ThemePatternNode, 
                        1
                    );

                    Node->Pattern.Tags = StrArrayFromList(AppFrameMemPool(), &Tags);
                    Node->Pattern.Linear = ColourLinear;
                    SLL_QUEUE_PUSH(HeadPattern, TailPattern, Node);
                    ++PatternCount;
                }
            }
        }

        WS->Theme = ArenaPushArrayZero(AppFrameMemPool(), UITheme, 1);
        WS->Theme->PatternsCount = PatternCount;
        WS->Theme->Patterns = ArenaPushArrayZero(
            AppFrameMemPool(), 
            UIThemePattern, 
            WS->Theme->PatternsCount
        );

        u64 Index = 0;

        for (ThemePatternNode* N = HeadPattern; N; N = N->Next, ++Index)
            WS->Theme->Patterns[Index] = N->Pattern;

        AccessClose(Acc);
    }

    // NOTE(nathan): Compute window font raster kind
    {
        WS->FontSlotRasterKind[APP_FONT_SLOT_ICONS] = FONT_RASTER_KIND_SMOOTH;
        WS->FontSlotRasterKind[APP_FONT_SLOT_MAIN] = (
            AppSettingFromNameB32("smooth_ui_text"_s8) * FONT_RASTER_KIND_SMOOTH) | 
            (AppSettingFromNameB32("hint_ui_text"_s8) * FONT_RASTER_KIND_HINTED
        );
        WS->FontSlotRasterKind[APP_FONT_SLOT_CODE] = (
            AppSettingFromNameB32("smooth_code_text"_s8) * FONT_RASTER_KIND_SMOOTH) | 
            (AppSettingFromNameB32("hint_code_text"_s8) * FONT_RASTER_KIND_HINTED
        );
    }

    // NOTE(nathan): Pre-emptively rasterise common glyphs on the first frame
    if (APP_STATE->HeadWindowState == WS && APP_STATE->TailWindowState == WS && !WS->FramesAlive) {
        f32 FontSize = AppFontSize();
        AppFontSlot EnglishFontSlots[] = {
            APP_FONT_SLOT_MAIN,
            APP_FONT_SLOT_CODE
        };
        AppFontSlot IconFontSlot = APP_FONT_SLOT_ICONS;

        for (u64 Index = 0; Index < ARRAY_COUNT(EnglishFontSlots); ++Index) {
            TempArena Scratch = GetScratch(NULL, 0);
            AppFontSlot Slot = EnglishFontSlots[Index];
            Str8 SampleText = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890~!@#$%^&*()-_+=[{]}\\|;:'\",<.>/?"_s8;

            FontRunFromStr(AppFontFromSlot(Slot), FontSize, 0.0f, 0.0f, 0, SampleText);
            FontRunFromStr(AppFontFromSlot(Slot), FontSize, 0.0f, 0.0f, 0, SampleText);
            ReleaseScratch(Scratch);
        }

        for (
            AppIconKind IconKind = APP_ICON_KIND_NULL; 
            IconKind < APP_ICON_KIND_COUNT; 
            IconKind = (AppIconKind) (IconKind + 1)
        ) {
            TempArena Scratch = GetScratch(NULL, 0);

            FontRunFromStr(
                AppFontFromSlot(IconFontSlot), 
                FontSize, 
                0.0f, 
                0.0f, 
                FONT_RASTER_KIND_SMOOTH, 
                APP_ICON_KIND_TEXT_TABLE[IconKind]
            );
            FontRunFromStr(
                AppFontFromSlot(IconFontSlot), 
                FontSize, 
                0.0f, 
                0.0f, 
                FONT_RASTER_KIND_SMOOTH, 
                APP_ICON_KIND_TEXT_TABLE[IconKind]
            );
            FontRunFromStr(
                AppFontFromSlot(IconFontSlot), 
                FontSize, 
                0.0f, 
                0.0f, 
                FONT_RASTER_KIND_SMOOTH, 
                APP_ICON_KIND_TEXT_TABLE[IconKind]
            );
            ReleaseScratch(Scratch);
        }
    }

    // NOTE(nathan): Commit window position/status to config tree
    {
        TempArena Scratch = GetScratch(NULL, 0);
        b32 IsFullScreen = WindowIsFullScreen(WS->WinHandle);
        b32 IsMaximised = WindowIsMaximised(WS->WinHandle);
        b32 IsMinimised = WindowIsMinimised(WS->WinHandle);

        if (IsFullScreen) {
            ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Window, 
                "fullscreen"_s8
            );
        } else {
            ConfigNodeRelease(
                APP_STATE->Config,
                ConfigNodeChildFromStr(
                    Window,
                    "fullscreen"_s8
                )
            );
        }

        if (IsMaximised) {
            ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Window, 
                "maximised"_s8
            );
        } else {
            ConfigNodeRelease(
                APP_STATE->Config,
                ConfigNodeChildFromStr(
                    Window, 
                    "maximised"_s8
                )
            );
        }

        f32 DPI = GetWindowDPI(WS->WinHandle);

        if (DPI != WS->LastDPI) {
            FontReset();

            f32 CurrentFontSize = AppFontSize();
            f32 NewFontSize = CurrentFontSize * (DPI / WS->LastDPI);

            NewFontSize = CLAMP(6.0f, NewFontSize, 72.0f);

            ConfigNode* FontSizeConfig = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Window,
                "font_size"_s8
            );

            ConfigNodeNewReplace(
                APP_STATE->Config,
                FontSizeConfig, 
                "%f", 
                NewFontSize
            );
            WS->LastDPI = DPI;
        }

        r2f32 WindowRect = GetWindowRect(WS->WinHandle);

        if (!IsFullScreen && !IsMaximised && !IsMinimised) {
            v2f32 Position = WindowRect.Point0;
            ConfigNode* PositionRoot = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Window, 
                "pos"_s8
            );

            if (
                (i32) Position.X != (i32) F64FromStr(PositionRoot->Head->String) ||
                (i32) Position.Y != (i32) F64FromStr(PositionRoot->Tail->String)
            ) {
                ConfigNode* X = PositionRoot->Head;

                if (X == EMPTY_CFG_NODE_VALUE) {
                    X = ConfigNodeAlloc(APP_STATE->Config);
                    ConfigNodeInsertChild(
                        APP_STATE->Config,
                        PositionRoot, 
                        EMPTY_CFG_NODE_VALUE, 
                        X
                    );
                }

                ConfigNode* Y = X->Next;

                if (Y == EMPTY_CFG_NODE_VALUE) {
                    Y = ConfigNodeAlloc(APP_STATE->Config);
                    ConfigNodeInsertChild(APP_STATE->Config, PositionRoot, X, Y);
                }

                ConfigNodeEquipStr(
                    APP_STATE->Config, 
                    X,  
                    "%d", 
                    (i32) Position.X
                );
                ConfigNodeEquipStr(
                    APP_STATE->Config, 
                    Y, 
                    "%d", 
                    (i32) Position.Y
                );
            }
        }

        if (!IsFullScreen && !IsMaximised && !IsMinimised) {
            v2f32 Size = Length(WindowRect);
            ConfigNode* SizeRoot = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Window, 
                "size"_s8
            );

            if (
                (i32) Size.X != (i32) F64FromStr(SizeRoot->Head->String) ||
                (i32) Size.Y != (i32) F64FromStr(SizeRoot->Tail->String)
            ) {
                ConfigNode* Width = SizeRoot->Head;

                if (Width == EMPTY_CFG_NODE_VALUE) {
                    Width = ConfigNodeAlloc(APP_STATE->Config);
                    ConfigNodeInsertChild(
                        APP_STATE->Config,
                        SizeRoot, 
                        EMPTY_CFG_NODE_VALUE, 
                        Width
                    );
                }

                ConfigNode* Height = Width->Next;

                if (Height == EMPTY_CFG_NODE_VALUE) {
                    Height = ConfigNodeAlloc(APP_STATE->Config);
                    ConfigNodeInsertChild(
                        APP_STATE->Config, 
                        SizeRoot, 
                        Width, 
                        Height
                    );
                }

                ConfigNodeEquipStr(
                    APP_STATE->Config,
                    Width, 
                    "%d", 
                    (i32) Size.X
                );
                ConfigNodeEquipStr(
                    APP_STATE->Config,
                    Height, 
                    "%d", 
                    (i32) Size.Y
                );
            }
        }

        if (!IsMinimised) {
            Handle Monitor = MonitorFromWindow(WS->WinHandle);
            Str8 MonitorName = NameFromMonitor(Scratch.MemPool, Monitor);
            ConfigNode* MonitorRoot = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Window, 
                "monitor"_s8
            );

            if (!StrMatch(MonitorRoot->Head->String, MonitorName, 0)) {
                ConfigNodeNewReplace(
                    APP_STATE->Config,
                    MonitorRoot, 
                    MonitorName
                );
            }
        }

        ReleaseScratch(Scratch);
    }

    // NOTE(nathan): Build UI
    UIBox* ListerBox = EMPTY_UI_BOX_VALUE;
    f32 TopLevelFontSize = AppFontSize();

    AppRegistersScope(__Registers.View = 0, __Registers.Panel = 0) {
        TopLevelFontSize = AppFontSize();
    }

    UIIconInfo IconInfo = {};

    IconInfo.IconFont = AppFontFromSlot(APP_FONT_SLOT_ICONS);
    IconInfo.IconKindTextMap[UI_ICON_KIND_RIGHT_ARROW] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_RIGHT_SCROLL];
    IconInfo.IconKindTextMap[UI_ICON_KIND_DOWN_ARROW] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_DOWN_SCROLL];
    IconInfo.IconKindTextMap[UI_ICON_KIND_LEFT_ARROW] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_LEFT_SCROLL];
    IconInfo.IconKindTextMap[UI_ICON_KIND_UP_ARROW] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_UP_SCROLL];
    IconInfo.IconKindTextMap[UI_ICON_KIND_RIGHT_CARET] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_RIGHT_CARET];
    IconInfo.IconKindTextMap[UI_ICON_KIND_DOWN_CARET] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_DOWN_CARET];
    IconInfo.IconKindTextMap[UI_ICON_KIND_LEFT_CARET] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_LEFT_CARET];
    IconInfo.IconKindTextMap[UI_ICON_KIND_UP_CARET] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_UP_CARET];
    IconInfo.IconKindTextMap[UI_ICON_KIND_CHECK_HOLLOW] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_CHECK_HOLLOW];
    IconInfo.IconKindTextMap[UI_ICON_KIND_CHECK_FILLED] = APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_CHECK_FILLED];

    UIAnimationInfo AnimationInfo = {};

    AnimationInfo.HotAnimationRate = APP_STATE->CatchAllAnimationRate;
    AnimationInfo.ActiveAnimationRate = APP_STATE->CatchAllAnimationRate;
    AnimationInfo.FocusAnimationRate = 1.0f;
    AnimationInfo.TooltipAnimationRate = APP_STATE->TooltipAnimationRate;
    AnimationInfo.MenuAnimationRate = APP_STATE->MenuAnimationRate;
    AnimationInfo.ScrollAnimationRate = APP_STATE->ScrollingAnimationRate;
    UIBeginBuild(
        WS->WinHandle,
        &WS->UIEvents,
        &IconInfo,
        WS->Theme,
        &AnimationInfo,
        APP_STATE->FrameDeltaTime,
        APP_STATE->FrameDeltaTime
    );
    UIPushFont(AppFontFromSlot(APP_FONT_SLOT_MAIN));
    UIPushFontSize(TopLevelFontSize);
    UIPushTextPadding(FloorF32(UIHeadFontSize() * 0.3f));
    UIPushPreferredWidth(UI_PX(FloorF32(UIHeadFontSize() * 20.0f), 1.0f));
    UIPushPreferredHeight(UI_PX(FloorF32(UIHeadFontSize() * 3.0f), 1.0f));
    UIPushBlurSize(10.0f);

    FontRasterKind TextRasterKind = 0;

    if (AppSettingFromNameB32("smooth_ui_text"_s8))
        TextRasterKind |= FONT_RASTER_KIND_SMOOTH;

    if (AppSettingFromNameB32("hint_ui_text"_s8))
        TextRasterKind |= FONT_RASTER_KIND_HINTED;

    UIPushTextRasterKind(TextRasterKind);

    // NOTE(nathan): Calculate code colour slot RGBAs
    for (u32 S = 0; S < APP_CODE_COLOUR_SLOT_COUNT; ++S)
        WS->ThemeCodeColours[S] = UIColourFromName(APP_CODE_COLOUR_SLOT_NAME_TABLE[S]);

    // NOTE(nathan): Calculate top-level rectangles/sizes
    r2f32 WindowRect = GetWindowClientRect(WS->WinHandle);
    v2f32 WindowRectDim = Length(WindowRect);
    f32 TopBarDimPX = FloorF32(UIHeadFontSize() * 3.0f);
    r2f32 TopBarRect = Rng(
        WindowRect.X0, 
        WindowRect.Y0, 
        WindowRect.X1,
        WindowRect.Y0 + TopBarDimPX
    );
    r2f32 ContentRect = Rng(
        WindowRect.X0,
        TopBarRect.Y1,
        WindowRect.X1,
        WindowRect.Y1
    );
    f32 WindowEdgePX = GetWindowDPI(WS->WinHandle) * 0.035f;

    // NOTE(nathan): Truncated string hover
    if (UIStringHoverActive()) {
        UITooltip() {
            TempArena Scratch = GetScratch(NULL, 0);
            FancyStrList FancyStrings = UIStringHoverFancyStrings(Scratch.MemPool);
            UIBox* Box = UIBuildBoxFromKey(UI_BOX_KIND_DRAW_TEXT, {});

            UIBoxEquipDisplayFancyStrs(Box, &FancyStrings);
            ReleaseScratch(Scratch);
        }
    }

    // NOTE(nathan): Rich hover / drag+drop tooltips
    if (
        (APP_STATE->HoverRegistersSlot != APP_REGISTERS_SLOT_NULL) ||
        (APP_STATE->DragDropRegistersSlot != APP_REGISTERS_SLOT_NULL && AppDragIsActive())
    ) {
        TempArena Scratch = GetScratch(NULL, 0);
        AppRegistersSlot Slot = ((APP_STATE->DragDropRegistersSlot != APP_REGISTERS_SLOT_NULL && AppDragIsActive()) ? APP_STATE->DragDropRegistersSlot : APP_STATE->HoverRegistersSlot);
        AppRegisters* Regs = (((APP_STATE->DragDropRegistersSlot != APP_REGISTERS_SLOT_NULL && AppDragIsActive()) ? APP_STATE->DragDropRegisters : APP_STATE->HoverRegisters));
        
        GetTLS()->UI_STATE->TooltipAnchorKey = Regs->UI;
        GetTLS()->UI_STATE->TooltipCanOverflowWindow = AppDragIsActive();

        switch (Slot) {
            default: {} break;

            case APP_REGISTERS_SLOT_COMMAND_NAME: {
                UITooltip() {
                    Str8 CommandName = Regs->CommandName;
                    FancyStrList FStrings = AppTitleFStrFromCodeName(
                        Scratch.MemPool,
                        CommandName
                    );

                    UIPreferredWidth(UI_SUM_OF_CHILDREN(1.0f)) {
                        UIRow() {
                            UIPreferredWidth(UI_TEXT_DIM(5.0f, 1.0f)) {
                                UIBox* Box = UIBuildBoxFromKey(UI_BOX_KIND_DRAW_TEXT, {});

                                UIBoxEquipDisplayFancyStrs(Box, &FStrings);
                                AppCommandBindingButtons(
                                    CommandName, 
                                    ""_s8, 
                                    4,
                                    APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT
                                );
                            }
                        }
                    }
                }
            } break;

            case APP_REGISTERS_SLOT_FILE_PATH: {
                UITooltip() {
                    FileProperties Props = SystemGetFileProperties(Regs->FilePath);

                    UISetNextPreferredWidth(UI_SUM_OF_CHILDREN(1.0f));

                    UIRow() {
                        AppFont(APP_FONT_SLOT_ICONS) {
                            UILabel(
                                APP_ICON_KIND_TEXT_TABLE[Props.Kind & SYS_FILE_IS_DIR 
                                    ? APP_ICON_KIND_FOLDER_CLOSED_FILLED 
                                    : APP_ICON_KIND_FILE_OUTLINE
                                ]
                            );
                            UILabel(Regs->FilePath);
                        }
                    }
                }
            } break;

            case APP_REGISTERS_SLOT_CONFIG: {
                UITooltip() {
                    ConfigNode* Config = ConfigNodeFromID(Regs->Config);
                    FancyStrList FStrings = AppTitleFStrFromConfig(
                        Scratch.MemPool,
                        Config,
                        FALSE
                    );

                    UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                        UIRow() {
                            UIPreferredWidth(UI_TEXT_DIM(5, 1)) {
                                UIBox* Box = UIBuildBoxFromKey(
                                    UI_BOX_KIND_DRAW_TEXT,
                                    EMPTY_UI_KEY_VALUE
                                );

                                UIBoxEquipDisplayFancyStrs(Box, &FStrings);
                            }
                        }
                    }
                }
            } break;

            case APP_REGISTERS_SLOT_EXPRESSION: {
                UITooltip() {
                    AppFont(APP_FONT_SLOT_CODE) {
                        UISetNextPreferredWidth(UI_SUM_OF_CHILDREN(1));

                        UIRow() {
                            UILabel(APP_STATE->DragDropRegisters->Expression);

                            EvalEval Eval = EvalEvalFromStr(APP_STATE->DragDropRegisters->Expression);

                            if (Eval.IRTree.Mode != EVAL_MODE_NULL) {
                                EVStrParams Params = {};

                                Params.Flags = EV_STR_FLAG_READ_ONLY_DISPLAY_RULES | APP_STATE->EvalVisualBaseStringFlags;
                                Params.Radix = 10;

                                Str8 ValueString = AppValueStrFromEval(
                                    Scratch.MemPool,
                                    ""_s8,
                                    &Params,
                                    UIHeadFont(),
                                    UIHeadFontSize(),
                                    UIHeadFontSize() * 20.0f,
                                    Eval
                                );

                                if (ValueString.Size) {
                                    UISpacer(UI_EM(2.0f, 1.0f));
                                    UILabel(ValueString);
                                }
                            }
                        }
                    }
                }
            } break;
        }

        ReleaseScratch(Scratch);
    }

    // NOTE(nathan): Drag+drop visualition tooltips
    if (AppDragIsActive() && IsWindowFocused) {
        AppRegistersScope(
            __Registers.Window = APP_STATE->DragDropRegisters->Window,
            __Registers.Panel = APP_STATE->DragDropRegisters->Panel,
            __Registers.Tab = 0,
            __Registers.View = APP_STATE->DragDropRegisters->View
        ) {
            TempArena Scratch = GetScratch(NULL, 0);
            ConfigNode* View = ConfigNodeFromID(APP_STATE->DragDropRegisters->View);

            {
                if (APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW && View != EMPTY_CFG_NODE_VALUE) {
                    ConfigNode* ImmediateParent = EMPTY_CFG_NODE_VALUE;

                    for (
                        ConfigNode* P = View->Parent;
                        P != EMPTY_CFG_NODE_VALUE;
                        P = P->Parent
                    ) {
                        if (StrMatch(P->Parent->String, "immediate"_s8, 0)) {
                            ImmediateParent = P->Parent;
                            break;
                        }
                    }

                    if (ImmediateParent != EMPTY_CFG_NODE_VALUE) {
                        ConfigNodeChildFromStrOrAlloc(
                            APP_STATE->Config,
                            ImmediateParent, 
                            "hot"_s8
                        );
                    }

                    UISize MainWidth = UIHeadPreferredWidth();
                    UISize MainHeight = UIHeadPreferredHeight();
                    UITextAlign MainTextAlignment = UIHeadTextAlignment();

                    UITooltip() {
                        UIPreferredWidth(MainWidth) {
                            UIPreferredHeight(MainHeight) {
                                UITextAlignment(MainTextAlignment) {
                                    GetTLS()->UI_STATE->TooltipCanOverflowWindow = TRUE;
                                    UISetNextPreferredWidth(UI_EM(60.0f, 1.0f));
                                    UISetNextPreferredHeight(UI_EM(40.0f, 1.0f));
                                    UISetNextChildLayoutAxis(AXIS_2D_Y);

                                    UIBox* Container = UIBuildBoxFromKey(0, {});

                                    UIParent(Container) {
                                        UIRow() {
                                            UIPreferredWidth(UI_TEXT_DIM(10, 1)) {
                                                FancyStrList FancyStrings = AppTitleFStrFromConfig(Scratch.MemPool, View, FALSE);
                                                UIBox* NameBox = UIBuildBoxFromKey(UI_BOX_KIND_DRAW_TEXT, {});

                                                UIBoxEquipDisplayFancyStrs(NameBox, &FancyStrings);
                                            }
                                        }

                                        UISetNextPreferredWidth(UI_PERCENT(1, 0));
                                        UISetNextPreferredHeight(UI_PERCENT(1, 0));
                                        UISetNextChildLayoutAxis(AXIS_2D_Y);

                                        UIBox* ViewPreviewContainer = UIBuildBoxFromStrFmt(
                                            UI_BOX_KIND_DRAW_BORDER | UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_CLIP,
                                            (char*) "###view_preview_container"
                                        );

                                        UIParent(ViewPreviewContainer) {
                                            UIFocus(UI_FOCUS_KIND_OFF) {
                                                UIWidthFill() {
                                                    AppViewUI(ViewPreviewContainer->Rect);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            ReleaseScratch(Scratch);
        }
    }

    // NOTE(nathan): Developer menu
    if (WS->DevMenuIsOpen) {
        AppFont(APP_FONT_SLOT_CODE) {
            UISetNextFlags(
                UI_BOX_KIND_VIEW_SCROLL_Y | 
                UI_BOX_KIND_ALLOW_OVERFLOW_Y | 
                UI_BOX_KIND_VIEW_CLAMP
            );

            UIPaneFmt(
                Rng(
                    30.0f, 
                    30.0f, 
                    30.0f + UIHeadFontSize() * 100.0f, 
                    UIHeadFontSize() * 60.0f
                ), 
                "###dev_ctx_menu"
            ) {
                for (AppWindowState* W = APP_STATE->HeadWindowState; W != EMPTY_APP_WINDOW_STATE_VALUE; W = W->OrderNext) {
                    f64 AverageUIHashChainLength = 0.0;

                    {
                        f64 ChainCount = 0.0;
                        f64 ChainLengthSum = 0.0;

                        for (u64 I = 0; I < W->UI->BoxTableSize; ++I) {
                            f64 ChainLength = 0.0;

                            for (UIBox* B = W->UI->BoxTable[I].Head; !IsUIBoxEmpty(B); B = B->HashNext) {
                                ++ChainLength;
                            }

                            if (ChainLength > 0.0) {
                                ChainLengthSum += ChainLength;
                                ++ChainCount;
                            }
                        }

                        AverageUIHashChainLength = ChainLengthSum / ChainCount;
                    }

                    UILabel("Target HZ: %.2f", 1.0f / APP_STATE->FrameDeltaTime);
                    UILabel("Window %p", W);
                    UISetNextPreferredWidth(UI_SUM_OF_CHILDREN(1));
                    UISetNextPreferredHeight(UI_SUM_OF_CHILDREN(1));

                    UIRow() {
                        UISpacer(UI_EM(2.0f, 1.0f));
                        UILabel((char*) "Box Count: %llu", W->UI->LastBuildBoxCount);
                    }

                    UISetNextPreferredWidth(UI_SUM_OF_CHILDREN(1));
                    UISetNextPreferredHeight(UI_SUM_OF_CHILDREN(1));

                    UIRow() {
                        UISpacer(UI_EM(2.0f, 1.0f));
                        UILabel((char*) "Average UI Hash Chain Length: %f", AverageUIHashChainLength);
                    }
                }

                UIDivider(UI_EM(1.0f, 1.0f));
            }
        }
    }

    // NOTE(nathan): Drop-completion context menu
    if (WS->HeadDropCompletionTask) {
        AppDropCompletionTask* Task = WS->HeadDropCompletionTask;
        b32 Done = FALSE;

        UIContextMenu(APP_STATE->DropCompletionKey) {
            UIPreferredWidth(UI_EM(40.0f, 1.0f)) {
                UITag("implicit"_s8) {
                    UITag("weak"_s8) {
                        UIRow() {
                            UIPadding(UI_EM(1.25f, 1.0f)) {
                                Str8List Strings = {};
                                u64 Index = 0;

                                for (
                                    Str8Node* N = Task->Paths.Head;
                                    N && Index < 20;
                                    N = N->Next, ++Index
                                ) {
                                    ListPush(
                                        Scratch.MemPool, 
                                        &Strings, 
                                        StrSkipLastSlash(N->String)
                                    );

                                    if (Index + 1 == 20) {
                                        ListPush(
                                            Scratch.MemPool,
                                            &Strings,
                                            "..."_s8
                                        );
                                    }
                                }

                                Str8JoinPart Join = {};

                                Join.Sep = ", "_s8;

                                Str8 String = StrListJoin(
                                    Scratch.MemPool,
                                    &Strings,
                                    &Join
                                );

                                UIPreferredWidth(UI_PERCENT(1, 0)) {
                                    UILabel(String);
                                }
                            }

                            // TODO(nathan): determine what was being dropped
                            //             : and store appropriately

                            if (
                                UI_CLICKED(
                                    AppIconButton(
                                        APP_ICON_KIND_FILE_OUTLINE,
                                        NULL,
                                        "View file%s contents",
                                        (Task->Paths.Count > 1) ? "s'" : ""
                                    )
                                )
                            ) {
                                for (
                                    Str8Node* N = Task->Paths.Head;
                                    N;
                                    N = N->Next
                                ) {
                                    AppCmd(
                                        APP_COMMAND_KIND_OPEN,
                                        __Registers.FilePath = N->String
                                    );
                                    Done = TRUE;
                                }
                            }

                            if (Done) {
                                SLL_STACK_POP(WS->HeadDropCompletionTask);

                                if (!WS->HeadDropCompletionTask)
                                    UIContextMenuClose();
                            }
                        }
                    }
                }
            }
        }
    }

    // NOTE(nathan): PopUp
    if (APP_STATE->PopUpT > 0.005f) {
        UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
            UIFocus(APP_STATE->PopUpActive ? UI_FOCUS_KIND_ROOT : UI_FOCUS_KIND_OFF) {
                v2f32 WindowLength = Length(WindowRect);
                UIBox* BackgroundBox = EMPTY_UI_BOX_VALUE;

                UIRect(WindowRect) {
                    UIChildLayoutAxis(AXIS_2D_X) {
                        UIFocus(UI_FOCUS_KIND_ON) {
                            UIBlurSize(10.0f * APP_STATE->PopUpT) {
                                UITransparency(1.0f - APP_STATE->PopUpT) {
                                    UITag("floating"_s8) {
                                        BackgroundBox = UIBuildBoxFromStrFmt(
                                            (
                                                UI_BOX_KIND_FIXED_SIZE |
                                                UI_BOX_KIND_FLOATING |
                                                UI_BOX_KIND_CLICKABLE |
                                                UI_BOX_KIND_SCROLL |
                                                UI_BOX_KIND_DEFAULT_FOCUS_NAV |
                                                UI_BOX_KIND_DISABLE_FOCUS_OVERLAY |
                                                UI_BOX_KIND_DRAW_BACKGROUND_BLUR |
                                                UI_BOX_KIND_DRAW_BACKGROUND
                                            ),
                                            "###popup_%p",
                                            WS
                                        );
                                    }
                                }
                            }
                        }
                    }
                }

                if (APP_STATE->PopUpActive) {
                    UIParent(BackgroundBox) {
                        UITransparency(1.0f - APP_STATE->PopUpT) {
                            UIContextMenuClose();

                            UIWidthFill() {
                                UIPreferredHeight(UI_SUM_OF_CHILDREN(1.0f)) {
                                    UIColumn() {
                                        UIPadding(UI_PERCENT(1, 0)) {
                                            UITextRasterFlags(AppRasterKindFromSlot(APP_FONT_SLOT_MAIN)) {
                                                UIFontSize(UIHeadFontSize() * 2.0f) {
                                                    UIPreferredHeight(UI_EM(3.0f, 1.0f)) {
                                                        UILabel(APP_STATE->PopUpTitle);
                                                    }
                                                }
                                            }

                                            UIPreferredHeight(UI_EM(3.0f, 1.0f)) {
                                                UITagFmt((char*) "weak") {
                                                    UILabel(APP_STATE->PopUpDescription);
                                                }
                                            }

                                            UISpacer(UI_EM(1.5f, 1.0f));

                                            UIRow() {
                                                UIPadding(UI_PERCENT(1.0f, 0.0f)) {
                                                    UIPreferredWidth(UI_EM(16.0f, 1.0f)) {
                                                        UIPreferredHeight(UI_EM(3.5f, 1.0f)) {
                                                            UITagFmt((char*) "pop") {
                                                                if (
                                                                    UI_CLICKED(UIButton((char*) "OK")) || 
                                                                    (
                                                                        (BackgroundBox->DefaultNavFocusHotKey == EMPTY_UI_KEY_VALUE) && 
                                                                        UISlotPress(UI_EVENT_ACTION_SLOT_ACCEPT)
                                                                    )
                                                                ) {
                                                                    AppCmd(APP_COMMAND_KIND_POP_UP_ACCEPT);
                                                                }

                                                                UISpacer(UI_EM(1.0f, 1.0f));

                                                                if (UI_CLICKED(UIButton((char*) "Cancel")) || UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)) {
                                                                    AppCmd(APP_COMMAND_KIND_POP_UP_CANCEL);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }

                                            UISpacer(UI_EM(3.0f, 1.0f));
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                UISignalFromBox(BackgroundBox);
            }
        }
    }

    // NOTE(nathan): Gather all tasks to build floating views
    struct FloatingViewTask {
        FloatingViewTask* Next;
        ConfigNode*       View;
        AppRegisters*     Regs;
        r2f32             Rect;
        b32               IsFocused;
        b32               IsAnchored;
        b32               ForceInsideWindowX;
        b32               ForceInsideWindowY;
        b32               OnlySecondaryNavigation;
        b32               ResetOpen;
        UISignal          Signal;
        b32               Pressed;
        b32               PressedOutside;
        u64               ProjectIndex;
    };

    FloatingViewTask* AutoCompleteFloatingViewTask = NULL;
    FloatingViewTask* HoverEvalFloatingViewTask = NULL;
    FloatingViewTask* QueryFloatingViewTask = NULL;
    FloatingViewTask* HeadFloatingViewTask = NULL;
    FloatingViewTask* TailFloatingViewTask = NULL;

    AppFont(APP_FONT_SLOT_CODE) {
        if (
            WS->AutoCompleteRegs &&
            WS->AutoCompletePrevFrameIndex + 1 >= APP_STATE->FrameIndex
        ) {
            ConfigNode* Root = AppImmediateConfigFromKey(
                "autocomp_view_%llx",
                Window->ID
            );
            ConfigNode* View = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Root,
                "watch"_s8
            );

            ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                View,
                "autocomplete"_s8
            );

            ConfigNode* Query = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                View,
                "query"_s8
            );
            ConfigNode* Input = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                Query,
                "input"_s8
            );

            ConfigNodeNewReplace(
                APP_STATE->Config,
                Input,
                WS->AutoCompleteCursorInfo.Filter
            );

            ConfigNode* Expr = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                View,
                "expression"_s8
            );

            ConfigNodeNewReplace(
                APP_STATE->Config,
                Expr,
                WS->AutoCompleteCursorInfo.ListExpr
            );

            EVBlockTree PredictedBlockTree = {};

            AppRegistersScope(
                __Registers.View = View->ID,
                __Registers.Tab = 0
            ) {
                Str8 Expr = AppExpressionFromConfig(View);
                EvalEval ListEval = EvalEvalFromStr(Expr);

                EVKeySetExpansion(
                    AppViewEvalView(),
                    EVKeyRoot(),
                    EVKeyAlloc(Hash(EVKeyRoot()), 1),
                    TRUE
                );
                PredictedBlockTree = EVBlockTreeFromEval(
                    Scratch.MemPool,
                    AppViewEvalView(),
                    AppViewQueryInput(),
                    ListEval
                );
            }

            f32 RowHeightPX = UIHeadPXHeight();
            u64 MaxRowCount = (u64) FloorF32(UIHeadFontSize() * 30.0f / RowHeightPX);
            u64 RequiredRowCount = MIN(MaxRowCount, PredictedBlockTree.TotalRowCount - 1);
            f32 WidthPX = FloorF32(UIHeadFontSize() * 30.0f);
            f32 HeightPX = RequiredRowCount * RowHeightPX;
            r2f32 Rect = Rng(0.0f, 0.0f, 0.0f, 0.0f);

            if (WS->AutoCompleteRegs->UI != EMPTY_UI_KEY_VALUE) {
                UIBox* AnchorBox = UIBoxFromKey(
                    WS->AutoCompleteRegs->UI
                );

                Rect.X0 = AnchorBox->Rect.X0;
                Rect.Y0 = AnchorBox->Rect.Y1;
                Rect.X1 = Rect.X0 + WidthPX;
                Rect.Y1 = Rect.Y0 + HeightPX;
            }

            if (PredictedBlockTree.TotalRowCount > 1) {
                FloatingViewTask* T = ArenaPushArrayZero(
                    Scratch.MemPool, 
                    FloatingViewTask,
                    1
                );

                SLL_QUEUE_PUSH(
                    HeadFloatingViewTask,
                    TailFloatingViewTask,
                    T
                );
                AutoCompleteFloatingViewTask = T;
                T->View = View;
                T->Rect = Rect;
                T->IsFocused = TRUE;
                T->IsAnchored = TRUE;
                T->OnlySecondaryNavigation = TRUE;
            }
        }

        b32 BuildHoverEval = (
            HoverEvalIsOpen && 
            !AppDragIsActive()
        );

        if (HoverEvalIsOpen) {
            for (
                ConfigPanelNode* Panel = PanelTree.Root;
                Panel != EMPTY_CFG_PANEL_NODE_VALUE;
                Panel = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, Panel).Next
            ) {
                if (Panel->Head != EMPTY_CFG_PANEL_NODE_VALUE)
                    continue;

                ConfigNode* Tab = Panel->SelectedTab;

                if (Tab != EMPTY_CFG_NODE_VALUE) {
                    AppViewState* VS = AppViewStateFromConfig(Tab);
                    r2f32 PanelRect = ConfigTargetRectFromPanelNode(
                        ContentRect,
                        PanelTree.Root,
                        Panel
                    );

                    if (
                        InRange(PanelRect, UIMouse()) &&
                        (
                            AbsF32(VS->ScrollPosition.X.Offset) > 0.01f ||
                            AbsF32(VS->ScrollPosition.Y.Offset) > 0.01f
                        )
                    ) {
                        BuildHoverEval = FALSE;
                        WS->HoverEvalFirstTimeUSecs = APP_STATE->TimeUSecs;
                    }
                }
            }
        }

        Str8 HoverEvalExpr = WS->HoverEvalString;
        EvalEval HoverEval = EvalEvalFromStr(HoverEvalExpr);

        {
            if (HoverEval.Msgs.MaxKind > EVAL_MSG_KIND_NULL) {
                for (
                    EvalMsg* Msg = HoverEval.Msgs.Head;
                    Msg;
                    Msg = Msg->Next
                ) {
                    LogError("[MSG] %S", PRINT_STR(Msg->Text));
                }

                BuildHoverEval = FALSE;
            } else if (
                HoverEval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG &&
                AppConfigFromEvalSpace(HoverEval.Space) == EMPTY_CFG_NODE_VALUE
            ) {
                BuildHoverEval = FALSE;
            }
        }


        if (
            WS->HoverEvalString.Size &&
            !HoverEvalIsOpen &&
            WS->HoverEvalLastTimeUSecs < WS->HoverEvalFirstTimeUSecs + HoverEvalOpenDelayUSecs &&
            APP_STATE->TimeUSecs - WS->HoverEvalLastTimeUSecs < HoverEvalOpenDelayUSecs * 2
        ) {
            AppRequestFrame();
        }

        if (BuildHoverEval) {
            EVExpandRule* ExpandRule = EVExpandRuleFromTypeKey(HoverEval.IRTree.TypeKey);
            AppViewUIRule* ViewUIRule = AppViewUIRuleFomStr(ExpandRule->String);
            ConfigNode* Root = AppImmediateConfigFromKey("hover_eval_view_%llx", WS->CfgID);
            ConfigNode* View = AppViewFromEval(Root, HoverEval);

            ConfigNodeChildFromStrOrAlloc(APP_STATE->Config, View, "explicit_root"_s8);

            EVBlockTree PredictedBlockTree = {};
            f32 FontSizePX = 0.0f;
            f32 RowHeightPX = 0.0f;

            AppRegistersScope(
                __Registers.View = View->ID,
                __Registers.Tab = 0
            ) {
                EVKeySetExpansion(
                    AppViewEvalView(),
                    EVKeyRoot(),
                    EVKeyAlloc(Hash(EVKeyRoot()), 1),
                    TRUE
                );
                PredictedBlockTree = EVBlockTreeFromEval(
                    Scratch.MemPool,
                    AppViewEvalView(),
                    ""_s8,
                    HoverEval
                );
                FontSizePX = AppFontSize();
                RowHeightPX = FloorF32(
                    FontSizePX * AppSettingFromNameF32("row_height"_s8)
                );
            }

            u64 MaxRowCount = 12;
            f32 WidthPX = FloorF32(70.0f * UIHeadFontSize());
            u64 RequiredRowCount = MIN(MaxRowCount, PredictedBlockTree.TotalRowCount);
            f32 HeightPX = RequiredRowCount * RowHeightPX;

            if (ViewUIRule != EMPTY_APP_VIEW_UI_RULE_VALUE)
                HeightPX = FloorF32(40.0f * UIHeadFontSize());

            r2f32 Rect = Rng(
                WS->HoverEvalSpawnPosition.X,
                WS->HoverEvalSpawnPosition.Y,
                WS->HoverEvalSpawnPosition.X + WidthPX,
                WS->HoverEvalSpawnPosition.Y + HeightPX
            );

            {
                FloatingViewTask* T = ArenaPushArrayZero(
                    Scratch.MemPool,
                    FloatingViewTask,
                    1
                );

                SLL_QUEUE_PUSH(HeadFloatingViewTask, TailFloatingViewTask, T);
                HoverEvalFloatingViewTask = T;
                T->View = View;
                T->Rect = Rect;
                T->IsFocused = WS->HoverEvalFocused;
                T->IsAnchored = TRUE;
                T->ForceInsideWindowX = TRUE;
            }
        }

        if (!BuildHoverEval || !WS->HoverEvalString.Size || !HoverEvalIsOpen)
            WS->HoverEvalFocused = FALSE;

        if (QueryIsOpen) {
            UIBox* Box = UIBoxFromKey(WS->QueryRegs->UI);

            if (WS->QueryRegs->UI != EMPTY_UI_KEY_VALUE && IsUIBoxEmpty(Box)) {
                QueryIsOpen = FALSE;
                AppCmd(APP_COMMAND_KIND_CANCEL_QUERY);
            }
        }

        if (QueryIsOpen) {
            Str8 Expr = WS->QueryRegs->Expression;
            EvalEval Eval = EvalEvalFromStr(Expr);

            if (Eval.Msgs.MaxKind > EVAL_MSG_KIND_NULL) {
                QueryIsOpen = FALSE;
                AppCmd(APP_COMMAND_KIND_CANCEL_QUERY);
            } else if (
                Eval.Space.Kind == APP_EVAL_SPACE_KIND_META_CFG &&
                AppConfigFromEvalSpace(Eval.Space) == EMPTY_CFG_NODE_VALUE
            ) {
                QueryIsOpen = FALSE;
                AppCmd(APP_COMMAND_KIND_CANCEL_QUERY);
            }
        }

        if (QueryIsOpen) {
            ConfigNode* WindowQuery = AppImmediateConfigFromKey("window_query_%p", Window);
            ConfigNode* View = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                WindowQuery, 
                "watch"_s8
            );
            ConfigNode* Query = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                View, 
                "query"_s8
            );
            b32 IsLister = (ConfigNodeChildFromStr(View, "lister"_s8) != EMPTY_CFG_NODE_VALUE);
            b32 IsSmall = (ConfigNodeChildFromStr(View, "small"_s8) != EMPTY_CFG_NODE_VALUE);
            b32 RootIsExplicit = (ConfigNodeChildFromStr(View, "explicit_root"_s8) != EMPTY_CFG_NODE_VALUE);
            AppViewState* VS = AppViewStateFromConfig(View);
            b32 ResetOpen = FALSE;

            if (View->ID != WS->QueryLastViewID) {
                WS->QueryLastViewID = View->ID;
                ResetOpen = TRUE;
            }

            Str8 CommandName = WS->QueryRegs->CommandName;
            AppCommandKindInfo* Info = AppCommandKindInfoFromStr(CommandName);
            Str8 QueryExpression = WS->QueryRegs->Expression;

            if (!QueryExpression.Size && CommandName.Size)
                QueryExpression = Info->Query.Expression;

            b32 QueryIsAnchored = (!IsUIBoxEmpty(UIBoxFromKey(WS->QueryRegs->UI)));
            b32 SizeQueryByExpressionValue = (QueryIsAnchored || !QueryExpression.Size);

            if (!QueryExpression.Size) {
                QueryExpression = Str(VS->QueryBuffer, VS->QueryStringSize);
            } else {
                u64 InputInsertionPosition = StrFindSubStr(
                    QueryExpression, 
                    "$input"_s8, 
                    0, 
                    0
                );

                if (InputInsertionPosition < QueryExpression.Size) {
                    Str8 PreInsertion = StrPrefix(QueryExpression, InputInsertionPosition);
                    Str8 PostInsertion = StrSkip(QueryExpression, InputInsertionPosition + 6);
                    Str8 InputText = Str(VS->QueryBuffer, VS->QueryStringSize);
                    Str8 InputTextEscaped = EscapedFromRawStr8(Scratch.MemPool, InputText);

                    QueryExpression = ArenaPushStrFmt(
                        Scratch.MemPool, 
                        "%S%S%S", 
                        PRINT_STR(PreInsertion), 
                        PRINT_STR(InputTextEscaped), 
                        PRINT_STR(PostInsertion)
                    );
                }
            }

            ConfigNode* Expression = ConfigNodeChildFromStrOrAlloc(
                APP_STATE->Config,
                View,
                "expression"_s8
            );

            ConfigNodeNewReplace(
                APP_STATE->Config,
                Expression, 
                QueryExpression
            );

            EvalEval QueryEval = EvalEvalFromStr(QueryExpression);

            if (WS->QueryRegs->DoBigRows) {
                f32 RowHeight = 5.0f;
                f32 RowHeightPX = RowHeight * UIHeadFontSize();
                ConfigNode* RowHeightRoot = ConfigNodeChildFromStrOrAlloc(
                    APP_STATE->Config,
                    View,
                    "row_height"_s8
                );

                ConfigNodeNewReplace(
                    APP_STATE->Config,
                    RowHeightRoot,
                    "%f",
                    RowHeight
                );
            }

            r2f32 Rect = {};

            AppRegistersScope(
                __Registers.View = View->ID,
                __Registers.Tab = 0
            ) {
                f32 RowHeightPX = UIHeadFontSize() * AppSettingFromNameF32("row_height"_s8);
                v2f32 ContentRectCentre = Centre(ContentRect);
                v2f32 ContentRectLength = Length(ContentRect);

                EVKeySetExpansion(
                    AppViewEvalView(),
                    EVKeyRoot(),
                    EVKeyAlloc(Hash(EVKeyRoot()), 1),
                    TRUE
                );

                EVBlockTree PredictedBlockTree = EVBlockTreeFromEval(
                    Scratch.MemPool,
                    AppViewEvalView(),
                    AppViewQueryInput(),
                    QueryEval
                );

                f32 QueryWidthPX = FloorF32(ContentRectLength.X * 0.35f);
                f32 MaxQueryHeightPX = ContentRectLength.Y * 0.8f;

                if (IsSmall) {
                    QueryWidthPX = FloorF32(ContentRectLength.X * 0.15f);
                    MaxQueryHeightPX = ContentRectLength.Y * 0.3f;
                }

                if (WS->QueryRegs->UI != EMPTY_UI_KEY_VALUE) {
                    QueryWidthPX = IsSmall
                        ? UIHeadFontSize() * 30.0f
                        : UIHeadFontSize() * 60.0f;
                    MaxQueryHeightPX = IsSmall
                        ? UIHeadFontSize() * 40.0f
                        : UIHeadFontSize() * 80.0f;
                }

                f32 QueryHeightPX = MaxQueryHeightPX;

                if (SizeQueryByExpressionValue) {
                    f32 SearchRowOpenT = UIAnimate(
                        UIKeyFromStrFmt(EMPTY_UI_KEY_VALUE, "search_row_open_%p", View),
                        (f32) !!VS->QueryIsOpen,
                        __Params.Initial = (f32) !!VS->QueryIsOpen,
                        __Params.Epsilon = 0.01f,
                        __Params.Rate = APP_STATE->MenuAnimationRate
                    );

                    QueryHeightPX = RowHeightPX * (
                        PredictedBlockTree.TotalRowCount - !RootIsExplicit
                    ) + UIHeadPXHeight() * SearchRowOpenT;
                    QueryHeightPX = MIN(QueryHeightPX, MaxQueryHeightPX);
                }

                Rect = Rng(
                    ContentRectCentre.X - QueryWidthPX / 2.0f,
                    ContentRectCentre.Y - MaxQueryHeightPX / 2.0f,
                    ContentRectCentre.X + QueryWidthPX / 2.0f,
                    ContentRectCentre.Y - MaxQueryHeightPX / 2.0f + QueryHeightPX
                );

                if (WS->QueryRegs->UI != EMPTY_UI_KEY_VALUE) {
                    UIBox* AnchorBox = UIBoxFromKey(WS->QueryRegs->UI);

                    if (AnchorBox != EMPTY_UI_BOX_VALUE) {
                        Rect.X0 = AnchorBox->Rect.X0 + WS->QueryRegs->OffsetPX.X;
                        Rect.Y0 = AnchorBox->Rect.Y1 + WS->QueryRegs->OffsetPX.Y;
                        Rect.X1 = Rect.X0 + QueryWidthPX;
                        Rect.Y1 = Rect.Y0 + QueryHeightPX;
                    }
                }
            }

            {
                FloatingViewTask* T = ArenaPushArrayZero(
                    Scratch.MemPool,
                    FloatingViewTask,
                    1
                );

                SLL_QUEUE_PUSH(HeadFloatingViewTask, TailFloatingViewTask, T);
                QueryFloatingViewTask = T;
                T->View = View;
                T->Regs = WS->QueryRegs;
                T->Rect = Rect;
                T->IsFocused = TRUE;
                T->IsAnchored = QueryIsAnchored;
                T->ResetOpen = ResetOpen;
                T->ForceInsideWindowX = TRUE;
                T->ForceInsideWindowY = TRUE;
            }
        }
    }

    // NOTE(nathan): Build all floating views
    AppFont(APP_FONT_SLOT_CODE) {
        UITagFmt((char*) "floating") {
            UIFocus(UIAnyContextMenuIsOpen() || WS->MenuBarFocused ? UI_FOCUS_KIND_OFF : UI_FOCUS_KIND_NULL) {
                f32 FastOpenRate = APP_STATE->MenuAnimationRate;
                f32 SlowOpenRate = APP_STATE->MenuAnimationRateSlow;

                for (FloatingViewTask* T = HeadFloatingViewTask; T; T = T->Next) {
                    ConfigNode* View = T->View;
                    r2f32 Rect = T->Rect;
                    b32 IsFocused = T->IsFocused;
                    b32 IsAnchored = T->IsAnchored;
                    b32 IsLister = (ConfigNodeChildFromStr(View, "lister"_s8) != EMPTY_CFG_NODE_VALUE);
                    b32 IsPIDHover = (ConfigNodeChildFromStr(View, "pid_hover"_s8) != EMPTY_CFG_NODE_VALUE);
                    b32 OnlySecondaryNavigation = T->OnlySecondaryNavigation;
                    f32 OpenT = UIAnimate(
                        UIKeyFromStrFmt({}, (char*) "floating_view_open_%p", View),
                        1.0f,
                        __Params.Rate = (IsAnchored) ? FastOpenRate : SlowOpenRate,
                        __Params.Reset = T->ResetOpen,
                        __Params.Initial = 0.0f
                    );

                    if (T->ForceInsideWindowX || T->ForceInsideWindowY) {
                        b32 AxisMask[] = {
                            T->ForceInsideWindowX, 
                            T->ForceInsideWindowY
                        };
                        r2f32 WindowRect = GetWindowClientRect(WS->WinHandle);

                        for (Axis2D Axis = 0; Axis < AXIS_2D_COUNT; ++Axis) {
                            if (!AxisMask[Axis])
                                continue;

                            f32 MaxDelta = Rect.Point1.V[Axis] - WindowRect.Point1.V[Axis];
                            f32 MinDelta = WindowRect.Point0.V[Axis] - Rect.Point0.V[Axis];
                            f32 TotalDelta = MAX(MinDelta, 0.0f) - MAX(MaxDelta, 0.0f);

                            Rect.Point0.V[Axis] += TotalDelta;
                            Rect.Point1.V[Axis] += TotalDelta;
                        }
                    }

                    AppPushRegisters();

                    if (T->Regs)
                        Registers()->Config = T->Regs->Config;

                    Registers()->View = View->ID;

                    Str8 ViewExpression = AppExpressionFromConfig(View);
                    Str8 ViewFilePath = AppFilePathFromEvalStr(
                        AppFrameMemPool(),
                        ViewExpression
                    );

                    if (ViewFilePath.Size)
                        Registers()->FilePath = ViewFilePath;

                    UIFocus(
                        IsFocused ? UI_FOCUS_KIND_ON : UI_FOCUS_KIND_OFF
                    ) {
                        UIPermissionFlags(
                            OnlySecondaryNavigation
                                ? (UI_PERMISSION_KIND_KEYBRD_SECONDARY | UI_PERMISSION_KIND_CLICKS | UI_PERMISSION_KIND_SCROLL_X | UI_PERMISSION_KIND_SCROLL_Y)
                                : UI_PERMISSION_KIND_ALL
                        ) {
                            b32 HasFooter = (IsLister && !IsAnchored);
                            UIBox* Container = EMPTY_UI_BOX_VALUE;
                            f32 ContainerCornerRadiusPX = HasFooter
                                ? UIHeadFontSize()
                                : UIHeadFontSize() * 0.15f;

                            if (IsPIDHover)
                                ContainerCornerRadiusPX = FloorF32(UIHeadFontSize() * 0.75f);

                            UIRect(Rect) {
                                UIChildLayoutAxis(AXIS_2D_Y) {
                                    UISquish(0.1f - 0.1f * OpenT) {
                                        UITransparency(1.0f - OpenT) {
                                            UICornerRadius(ContainerCornerRadiusPX) {
                                                Container = UIBuildBoxFromStrFmt(
                                                    (
                                                        UI_BOX_KIND_CLICKABLE |
                                                        UI_BOX_KIND_DRAW_BORDER |
                                                        UI_BOX_KIND_DRAW_BACKGROUND |
                                                        UI_BOX_KIND_DRAW_BACKGROUND_BLUR |
                                                        UI_BOX_KIND_ROUND_CHILDREN_BY_PARENT |
                                                        UI_BOX_KIND_DISABLE_FOCUS_OVERLAY|
                                                        UI_BOX_KIND_DRAW_DROP_SHADOW |
                                                        (UI_BOX_KIND_SQUISH_ANCHORED * !!IsAnchored)
                                                    ),
                                                    "floating_view_container_%p",
                                                    View
                                                );
                                            }
                                        }
                                    }
                                }
                            }

                            {
                                for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
                                    if (
                                        Evt->Kind == UI_EVENT_KIND_PRESS &&
                                        Evt->Input == INPUT_KIND_LEFT_MOUSE_BTN
                                    ) {
                                        if (InRange(Container->Rect, Evt->Position))
                                            T->Pressed = TRUE;
                                        else
                                            T->PressedOutside = TRUE;
                                    }
                                }
                            }

                            UIBox* LoadingOverlayContainer = EMPTY_UI_BOX_VALUE;

                            UIParent(Container) {
                                UIWidthFill() {
                                    UIHeightFill() {
                                        LoadingOverlayContainer = UIBuildBoxFromKey(
                                            UI_BOX_KIND_FLOATING,
                                            EMPTY_UI_KEY_VALUE
                                        );
                                    }
                                }
                            }

                            f32 TotalHeightPX = Length(Rect).Y;
                            f32 FooterHeightPX = 0.0f;

                            if (HasFooter) {
                                FooterHeightPX = UIHeadFontSize() * 4.0f;
                            }

                            UIParent(Container) {
                                UIFocus(IsFocused ? UI_FOCUS_KIND_NULL : UI_FOCUS_KIND_OFF) {
                                    UISetNextPreferredWidth(UI_PERCENT(1.0f, 0.0f));
                                    UISetNextPreferredHeight(UI_PX(TotalHeightPX - FooterHeightPX, 1.0f));
                                    UISetNextChildLayoutAxis(AXIS_2D_Y);

                                    UIBox* ViewContentsContainer = UIBuildBoxFromStr(
                                        UI_BOX_KIND_CLIP,
                                        "###view_contents_container"_s8
                                    );

                                    UIParent(ViewContentsContainer) {
                                        UIWidthFill() {
                                            AppViewUI(Rect);
                                        }
                                    }
                                }
                            }

                            if (IsLister && !IsAnchored) {
                                // TODO(nc): maybe show something in the footer? it does look clean with nothing
                                // UIParent(Container) {
                                //     UIWidthFill() {
                                //         UIHeightFill() {
                                //             UIFocus(UI_FOCUS_KIND_OFF) {
                                //                 UISetNextFlags(
                                //                     UI_BOX_KIND_DRAW_BACKGROUND |
                                //                     UI_BOX_KIND_DRAW_BORDER
                                //                 );

                                //                 UIColumn() {
                                //                     UIPadding(UI_EM(1.0f, 1.0f)) {
                                //                         UIRow() {
                                //                             UIPadding(UI_PERCENT(1.0f, 0.0f)) {
                                //                                 AppFont(APP_FONT_SLOT_MAIN) {
                                //                                     UIFontSize(UIHeadFontSize() * 0.8f) {
                                //                                         UITag("alt"_s8) {
                                //                                             UIPreferredWidth(UI_TEXT_DIM(1.0f, 1.0f)) {
                                //                                                 UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                //                                                     AppCommandBindingButtons(
                                //                                                         APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_MOVE_UP].String,
                                //                                                         ""_s8,
                                //                                                         1,
                                //                                                         APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT
                                //                                                     );
                                //                                                     UISpacer(UI_EM(1.0f, 1.0f));
                                //                                                     UILabel("/"_s8);
                                //                                                     AppCommandBindingButtons(
                                //                                                         APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_MOVE_DOWN].String,
                                //                                                         ""_s8,
                                //                                                         1,
                                //                                                         APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT
                                //                                                     );
                                //                                                     UILabel("to navigate"_s8);
                                //                                                     UISpacer(UI_EM(1.0f, 1.0f));
                                //                                                     AppCommandBindingButtons(
                                //                                                         APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_ACCEPT].String,
                                //                                                         ""_s8,
                                //                                                         1,
                                //                                                         APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT
                                //                                                     );
                                //                                                     UILabel("to use"_s8);
                                //                                                     UISpacer(UI_EM(1.0f, 1.0f));
                                //                                                     AppCommandBindingButtons(
                                //                                                         APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_CANCEL].String,
                                //                                                         ""_s8,
                                //                                                         1,
                                //                                                         APP_COMMAND_BINDING_BTN_FLAG_NO_EDIT
                                //                                                     );
                                //                                                     UILabel("to cancel"_s8);
                                //                                                 }
                                //                                             }
                                //                                         }
                                //                                     }
                                //                                 }
                                //                             }
                                //                         }
                                //                     }
                                //                 }
                                //             }
                                //         }
                                //     }
                                // }
                            }

                            {
                                AppViewState* VS = AppViewStateFromConfig(View);
                                f32 LoadingT = VS->LoadingT;

                                if (LoadingT > 0.01f) {
                                    UIParent(LoadingOverlayContainer) {
                                        UILoadingOverlay(
                                            Rect,
                                            LoadingT,
                                            VS->LoadingProgressV,
                                            VS->LoadingProgressVTarget
                                        );
                                    }
                                }
                            }

                            UISignal Sig = UISignalFromBox(Container);

                            T->Signal = Sig;
                        }
                    }

                    AppRegisters* ViewRegs = AppPopRegisters();

                    if (IsFocused)
                        MemCpy(Registers(), ViewRegs, sizeof(AppRegisters));

                    if (!IsAnchored) {
                        UITag("inactive"_s8) {
                            UITransparency(1.0f - OpenT) {
                                UIRect(ContentRect) {
                                    UIBuildBoxFromKey(
                                        UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_FLOATING,
                                        EMPTY_UI_KEY_VALUE
                                    );
                                }
                            }
                        }
                    }

                    if (T == AutoCompleteFloatingViewTask) {
                        b32 HasAutoCompleteHint = !!UIGetAutoCompleteStr().Size;
                        b32 HasAcceptOperation = FALSE;

                        for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
                            if (
                                Evt->Kind == UI_EVENT_KIND_PRESS &&
                                Evt->Slot == UI_EVENT_ACTION_SLOT_ACCEPT
                            ) {
                                HasAcceptOperation = TRUE;
                                break;
                            }
                        }

                        if (HasAutoCompleteHint && HasAcceptOperation) {
                            AutoCompleteFloatingViewTask->Signal.Box->FixedPosition = Vec(
                                10000.0f,
                                10000.0f
                            );
                        }
                    }

                    if (T == HoverEvalFloatingViewTask) {
                        UISignal Sig = HoverEvalFloatingViewTask->Signal;

                        if (UI_PRESSED(Sig) || HoverEvalFloatingViewTask->Pressed) {
                            WS->HoverEvalFocused = TRUE;
                        }

                        if (UI_MOUSE_OVER(Sig) || WS->HoverEvalFocused) {
                            WS->HoverEvalLastTimeUSecs = APP_STATE->TimeUSecs;
                        } else if (WS->HoverEvalLastTimeUSecs + 1000000 < APP_STATE->TimeUSecs) {
                            AppRequestFrame();
                        }

                        if (
                            HoverEvalFloatingViewTask->PressedOutside ||
                            UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)
                        ) {
                            WS->HoverEvalFocused = FALSE;
                            MemZero(&WS->HoverEvalString, sizeof(WS->HoverEvalString));
                            ArenaClear(WS->HoverEvalMemPool);
                            AppRequestFrame();
                        }
                    }

                    if (T == QueryFloatingViewTask) {
                        ConfigNode* View = QueryFloatingViewTask->View;
                        AppViewState* VS = AppViewStateFromConfig(
                            QueryFloatingViewTask->View
                        );
                        Str8 CommandName = WS->QueryRegs->CommandName;
                        AppCommandKindInfo* CommandKindInfo = AppCommandKindInfoFromStr(
                            CommandName
                        );

                        if (
                            QueryFloatingViewTask->PressedOutside ||
                            (
                                ConfigNodeChildFromStr(View, "lister"_s8) != EMPTY_CFG_NODE_VALUE &&
                                !VS->QueryIsOpen
                            ) ||
                            (
                                CommandName.Size &&
                                !VS->QueryIsOpen
                            ) ||
                            UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)
                        ) {
                            AppCmd(APP_COMMAND_KIND_CANCEL_QUERY);
                        }

                        if (CommandKindInfo->Query.Slot == APP_REGISTERS_SLOT_FILE_PATH) {
                            ConfigNode* Query = ConfigNodeChildFromStr(
                                View,
                                "query"_s8
                            );
                            ConfigNode* Input = ConfigNodeChildFromStr(
                                View,
                                "input"_s8
                            );

                            if (Input != EMPTY_CFG_NODE_VALUE) {
                                Str8 PathChopped = StrChopLastSlash(Input->Head->String);
                                ConfigNode* Session = ConfigNodeChildFromStr(
                                    ConfigNodeRoot(),
                                    "session"_s8
                                );
                                ConfigNode* CurrentPath = ConfigNodeChildFromStrOrAlloc(
                                    APP_STATE->Config,
                                    Session,
                                    "current_path"_s8
                                );

                                if (!StrMatch(CurrentPath->Head->String, PathChopped, 0)) {
                                    AppCmd(
                                        APP_COMMAND_KIND_SET_CURRENT_PATH,
                                        __Registers.FilePath = PathChopped
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // NOTE(nathan): Top bar
    WindowClearCustomBorderData(WS->WinHandle);
    WindowPushCustomEdges(WS->WinHandle, WindowEdgePX);
    WindowPushCustomTitleBar(WS->WinHandle, Length(TopBarRect).Y);
    UISetNextFlags(UI_BOX_KIND_DEFAULT_FOCUS_NAV | UI_BOX_KIND_DISABLE_FOCUS_OVERLAY);

    r2f32 SearchBarSpaceRect = {};

    UIFocus(
        (WS->MenuBarFocused && IsWindowFocused && !UIAnyContextMenuIsOpen())
        ? UI_FOCUS_KIND_ON
        : UI_FOCUS_KIND_NULL
    ) {
        UITagFmt((char*) "menu_bar") {
            UIPane(TopBarRect, "###top_bar"_s8) {
                UIWidthFill() {
                    UIRow() {
                        UIFocus(UI_FOCUS_KIND_NULL) {
                            UIKey MenuBarGroupKey = UIKeyFromStr({}, "###top_bar_group"_s8);

                            MemSet(
                                UIHeadParent()->Parent->CornerRadii, 
                                0, 
                                ARRAY_COUNT(UIHeadParent()->Parent->CornerRadii) * sizeof(f32)
                            );

                            {
                                UISetNextFlags(
                                    UI_BOX_KIND_CLIP | 
                                    UI_BOX_KIND_DRAW_BACKGROUND_BLUR | 
                                    UI_BOX_KIND_VIEW_CLAMP
                                );

                                UIWidthFill() {
                                    UINamedRow("###menu_bar"_s8) {
                                        UIPadding(UI_EM(0.5f, 1.0f)) {
                                            UIPreferredWidth(UI_PX(Length(TopBarRect).Y - UIHeadFontSize() * 0.8f, 1.0f)) {
                                                UIColumn() {
                                                    UIPadding(UI_EM(0.4f, 1.0f)) {
                                                        UIHeightFill() {
                                                            RendHandle Texture = APP_STATE->IconTexture;
                                                            v2i32 TextureDim = SizeFromTex2D(Texture);

                                                            UIImage(
                                                                Texture,
                                                                TEX2D_SAMPLE_LINEAR,
                                                                Rng(0.0f, 0.0f, (f32) TextureDim.X, (f32) TextureDim.Y),
                                                                Vec(1.0f, 1.0f, 1.0f, 1.0f),
                                                                0.0f,
                                                                ""_s8
                                                            );
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        if (Length(TopBarRect).X > UIHeadFontSize() * 60.0f) {
                                            UISetNextFlags(UI_BOX_KIND_DRAW_BACKGROUND);

                                            UIPreferredWidth(UI_SUM_OF_CHILDREN(1)) {
                                                UIRow() {
                                                    UIPreferredWidth(UI_TEXT_DIM(20, 1)) {
                                                        UIGroupKey(MenuBarGroupKey) {
                                                            // NOTE(nathan): File Menu
                                                            UIKey FileMenuKey = UIKeyFromStr({}, "_file_menu_key_"_s8);

                                                            UIContextMenu(FileMenuKey) {
                                                                UIPreferredWidth(UI_EM(50.0f, 1.0f)) {
                                                                    UITag("implicit"_s8) {
                                                                        Str8 Commands[] = {
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN_PALETTE].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEW_SESSION].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN_SESSION].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN_RECENT_SESSION].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_SAVE_SESSION].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_SESSION_SETTINGS].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_EXIT].String
                                                                        };

                                                                        u32 CodePoints[] = {
                                                                            'o',
                                                                            'n',
                                                                            0,
                                                                            'j',
                                                                            'p',
                                                                            'a',
                                                                            'e',
                                                                            't',
                                                                            0,
                                                                            'x'
                                                                        };

                                                                        STATIC_ASSERT(ARRAY_COUNT(CodePoints) == ARRAY_COUNT(Commands), FileMenuCommandsSizeCheck);
                                                                        AppCommandListMenuButtons(Commands, ARRAY_COUNT(Commands), CodePoints);
                                                                    }
                                                                }
                                                            }

                                                            // NOTE(nathan): Window Menu
                                                            UIKey WindowMenuKey = UIKeyFromStr({}, "_window_menu_key_"_s8);

                                                            UIContextMenu(WindowMenuKey) {
                                                                UIPreferredWidth(UI_EM(50.0f, 1.0f)) {
                                                                    UITag("implicit"_s8) {
                                                                        Str8 Commands[] = {
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN_WINDOW].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_CLOSE_WINDOW].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_TOGGLE_FULLSCREEN].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_WINDOW_SETTINGS].String,
                                                                        };

                                                                        u32 CodePoints[] = {
                                                                            'w',
                                                                            'c',
                                                                            'f',
                                                                            0,
                                                                            's',
                                                                        };

                                                                        STATIC_ASSERT(ARRAY_COUNT(Commands) == ARRAY_COUNT(CodePoints), WindowMenuCommandsSizeCheck);
                                                                        AppCommandListMenuButtons(Commands, ARRAY_COUNT(Commands), CodePoints);
                                                                    }
                                                                }
                                                            }

                                                            // NOTE(nathan): Panel Menu
                                                            UIKey PanelMenuKey = UIKeyFromStr({}, "_panel_menu_key_"_s8);

                                                            UIContextMenu(PanelMenuKey) {
                                                                UIPreferredWidth(UI_EM(50.0f, 1.0f)) {
                                                                    UITag("implicit"_s8) {
                                                                        Str8 Commands[] = {
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEW_PANEL_UP].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEW_PANEL_DOWN].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEW_PANEL_RIGHT].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEW_PANEL_LEFT].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_CLOSE_PANEL].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_ROTATE_PANEL_COLUMNS].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEXT_PANEL].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_PREV_PANEL].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_TAB_BAR_TOP].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_TAB_BAR_BOTTOM].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_RESET_TO_DEFAULT_PANELS].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_RESET_TO_COMPACT_PANELS].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_RESET_TO_SIMPLE_PANELS].String,
                                                                        };

                                                                        u32 CodePoints[] = {
                                                                            'u',
                                                                            'd',
                                                                            'r',
                                                                            'l',
                                                                            'x',
                                                                            'c',
                                                                            0,
                                                                            'n',
                                                                            'p',
                                                                            0,
                                                                            0,
                                                                            0,
                                                                            0,
                                                                            'e',
                                                                            'o',
                                                                            's'
                                                                        };

                                                                        STATIC_ASSERT(ARRAY_COUNT(CodePoints) == ARRAY_COUNT(Commands), PanelMenuCommandsSizeCheck);
                                                                        AppCommandListMenuButtons(Commands, ARRAY_COUNT(Commands), CodePoints);
                                                                    }
                                                                }
                                                            }

                                                            // NOTE(nathan): View Menu
                                                            UIKey TabMenuKey = UIKeyFromStr({}, "_tab_menu_key_"_s8);

                                                            UIContextMenu(TabMenuKey) {
                                                                UIPreferredWidth(UI_EM(50.0f, 1.0f)) {
                                                                    UITag("implicit"_s8) {
                                                                        Str8 Commands[] = {
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN_TAB].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_CLOSE_TAB].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_DUPLICATE_TAB].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_MOVE_TAB_LEFT].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_MOVE_TAB_RIGHT].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_NEXT_TAB].String,
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_PREV_TAB].String,
                                                                            {},
                                                                            APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_TAB_SETTINGS].String
                                                                        };

                                                                        u32 CodePoints[] = {
                                                                            'o',
                                                                            'c',
                                                                            'd',
                                                                            0,
                                                                            'l',
                                                                            'r',
                                                                            0,
                                                                            'n',
                                                                            'p',
                                                                            0,
                                                                            's'
                                                                        };

                                                                        STATIC_ASSERT(ARRAY_COUNT(Commands) == ARRAY_COUNT(CodePoints), ViewMenuCommandsSizeCheck);
                                                                        AppCommandListMenuButtons(Commands, ARRAY_COUNT(Commands), CodePoints);
                                                                    }
                                                                }
                                                            }

                                                            // NOTE(nathan): Help Menu
                                                            UIKey HelpMenuKey = UIKeyFromStr({}, "_help_menu_key_"_s8);

                                                            UIContextMenu(HelpMenuKey) {
                                                                UIPreferredWidth(UI_EM(50.0f, 1.0f)) {
                                                                    UITag("implicit"_s8) {
                                                                        UIRow() {
                                                                            UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                                                UITag("weak"_s8) {
                                                                                    UILabel("Help"_s8);
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }

                                                            // NOTE(nathan): Buttons
                                                            UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                                UIHeightFill() {
                                                                    struct {
                                                                        Str8      Name;
                                                                        u32       CodePoint;
                                                                        InputKind Input;
                                                                        UIKey     MenuKey;
                                                                    } Items[] = {
                                                                        {
                                                                            Str8Lit("File"),
                                                                            'f',
                                                                            INPUT_KIND_F,
                                                                            FileMenuKey
                                                                        },
                                                                        {
                                                                            Str8Lit("Window"),
                                                                            'w',
                                                                            INPUT_KIND_W,
                                                                            WindowMenuKey
                                                                        },
                                                                        {
                                                                            Str8Lit("Panel"),
                                                                            'p',
                                                                            INPUT_KIND_P,
                                                                            PanelMenuKey
                                                                        },
                                                                        {
                                                                            Str8Lit("Tab"),
                                                                            'b',
                                                                            INPUT_KIND_V,
                                                                            TabMenuKey
                                                                        },
                                                                        {
                                                                            Str8Lit("Help"),
                                                                            'h',
                                                                            INPUT_KIND_H,
                                                                            HelpMenuKey
                                                                        }
                                                                    };

                                                                    b32 MenuOpen = FALSE;
                                                                    u64 OpenMenuIndex = 0;

                                                                    for (u64 I = 0; I < ARRAY_COUNT(Items); ++I) {
                                                                        if (UIContextMenuIsOpen(Items[I].MenuKey)) {
                                                                            MenuOpen = TRUE;
                                                                            OpenMenuIndex = I;
                                                                            break;
                                                                        }
                                                                    }

                                                                    u64 OpenMenuIndexPrime = OpenMenuIndex;

                                                                    if (MenuOpen && WS->MenuBarFocused && IsWindowFocused) {
                                                                        for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
                                                                            b32 Taken = FALSE;

                                                                            if (Evt->DeltaI32.X > 0) {
                                                                                Taken = TRUE;
                                                                                ++OpenMenuIndexPrime;
                                                                                OpenMenuIndexPrime = OpenMenuIndexPrime % ARRAY_COUNT(Items);
                                                                            }

                                                                            if (Evt->DeltaI32.X < 0) {
                                                                                Taken = TRUE;
                                                                                OpenMenuIndexPrime = OpenMenuIndexPrime > 0 ? OpenMenuIndexPrime - 1 : (ARRAY_COUNT(Items) - 1);
                                                                            }

                                                                            if (Taken)
                                                                                UIEatEvent(Evt);
                                                                        }
                                                                    }

                                                                    for (u64 I = 0; I < ARRAY_COUNT(Items); ++I) {
                                                                        UISetNextFastpathCodepoint(Items[I].CodePoint);

                                                                        b32 AltFastpathKey = FALSE;

                                                                        if (AppSettingFromNameB32("focus_menu_bar_with_alt"_s8) && UIKeyPress(INPUT_MOD_KIND_ALT, Items[I].Input)) {
                                                                            AltFastpathKey = TRUE;
                                                                        }

                                                                        if ((WS->MenuBarKeyHeld || WS->MenuBarFocused) && !UIAnyContextMenuIsOpen())
                                                                            UISetNextFlags(UI_BOX_KIND_DRAW_TEXT_FASTPATH_CODEPOINT);

                                                                        UISignal Sig = AppMenuBarButton(Items[I].Name);

                                                                        WindowPushCustomTitleBarClientArea(WS->WinHandle, Sig.Box->Rect);

                                                                        if (MenuOpen) {
                                                                            if (
                                                                                (UI_HOVERING(Sig) && !UIContextMenuIsOpen(Items[I].MenuKey)) ||
                                                                                (OpenMenuIndexPrime == I && OpenMenuIndexPrime != OpenMenuIndex)
                                                                            ) {
                                                                                UIContextMenuOpen(Items[I].MenuKey, Sig.Box->Key, Vec(0.0f, Sig.Box->Rect.Y1 - Sig.Box->Rect.Y0));
                                                                            }
                                                                        } else if (UI_PRESSED(Sig) || AltFastpathKey) {
                                                                            if (UIContextMenuIsOpen(Items[I].MenuKey)) {
                                                                                UIContextMenuClose();
                                                                            } else {
                                                                                UIContextMenuOpen(Items[I].MenuKey, Sig.Box->Key, Vec(0.0f, Sig.Box->Rect.Y1 - Sig.Box->Rect.Y0));
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        // NOTE(nathan): middle part
                        if (Length(TopBarRect).X > UIHeadFontSize() * 60.0f) {
                            UISpacer(UI_PERCENT(1, 0));
                        }

                        UIWidthFill() {
                            UIRow() {
                                b32 DoSessionProf = (Length(TopBarRect).X > UIHeadFontSize() * 80);

                                UISpacer(UI_PERCENT(1, 0));

                                if (DoSessionProf) {
                                    UITagFmt((char*) "pop") {
                                        UISetNextPreferredWidth(UI_SUM_OF_CHILDREN(1));
                                        UISetNextChildLayoutAxis(AXIS_2D_X);

                                        UIBox* SessionBox = UIBuildBoxFromStr(
                                            (
                                                UI_BOX_KIND_CLICKABLE |
                                                UI_BOX_KIND_DRAW_BACKGROUND |
                                                UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                UI_BOX_KIND_DRAW_ACTIVE_EFFECTS
                                            ),
                                            "###loaded_session_button"_s8
                                        );

                                        WindowPushCustomTitleBarClientArea(WS->WinHandle, SessionBox->Rect);

                                        UIParent(SessionBox) {
                                            UIPreferredWidth(UI_TEXT_DIM(10, 0)) {
                                                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                    Str8 SessionPath = APP_STATE->SessionPath;

                                                    SessionPath = StrChopLastDot(SessionPath);

                                                    AppFont(APP_FONT_SLOT_ICONS) {
                                                        UITextRasterFlags(AppRasterKindFromSlot(APP_FONT_SLOT_ICONS)) {
                                                            UILabel(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_BRIEFCASE]);
                                                        }
                                                    }

                                                    if (StrSkipLastSlash(SessionPath).Size)
                                                        UILabel(StrSkipLastSlash(SessionPath));
                                                    else
                                                        UILabel("Open Session"_s8);
                                                }
                                            }
                                        }

                                        UISignal SessionSig = UISignalFromBox(SessionBox);

                                        if (UI_CLICKED(SessionSig)) {
                                            AppCmd(
                                                APP_COMMAND_KIND_RUN_COMMAND,
                                                __Registers.CommandName = APP_COMMAND_KIND_INFO_TABLE[APP_COMMAND_KIND_OPEN_SESSION].String
                                            );
                                        }
                                    }

                                    if (DoSessionProf)
                                        UISpacer(UI_EM(0.75f, 0));
                                }

                                UIKey CloseContextMenuKey = UIKeyFromStrFmt({}, (char*) "###close_ctx_menu");

                                UIContextMenu(CloseContextMenuKey) {
                                    if (UI_CLICKED(AppIconButton(APP_ICON_KIND_WINDOW, NULL, "Close Window"_s8)))
                                        AppCmd(APP_COMMAND_KIND_CLOSE_WINDOW);

                                    if (UI_CLICKED(AppIconButton(APP_ICON_KIND_X, NULL, "Exit"_s8)))
                                        AppCmd(APP_COMMAND_KIND_EXIT);
                                }

                                // NOTE(nathan): [Minimise|Maximise|Close] Buttons
                                {
                                    UISignal MinimiseSignal = {};
                                    UISignal MaximiseSignal = {};
                                    UISignal CloseSignal = {};
                                    v2f32 BarDim = Length(TopBarRect);
                                    f32 ButtonDim = FloorF32(BarDim.Y);

                                    UIPreferredWidth(UI_PX(ButtonDim, 1.0f)) {
                                        UIFontSize(UIHeadFontSize() * 0.75f) {
                                            MinimiseSignal = AppIconButton(APP_ICON_KIND_WINDOW_MINIMIZE, NULL, "##minimise"_s8);
                                            MaximiseSignal = AppIconButton((WindowIsMaximised(WS->WinHandle)) ? APP_ICON_KIND_WINDOW_RESTORE : APP_ICON_KIND_WINDOW, NULL, "##maximise"_s8);
                                        }
                                    }

                                      
                                    UIPreferredWidth(UI_PX(ButtonDim, 1.0f)) {
                                        UITagFmt((char*) "bad_pop") {
                                            CloseSignal = AppIconButton(APP_ICON_KIND_X, NULL, "##close"_s8);
                                            CloseSignal.Box->Kind = (CloseSignal.Box->Kind & ~UI_BOX_KIND_DRAW_BACKGROUND);
                                        }
                                    }

                                    if (UI_CLICKED(MinimiseSignal)) {
                                        WindowSetMinimised(WS->WinHandle, TRUE);
                                    }
                                      
                                    if (UI_CLICKED(MaximiseSignal)) {
                                        WindowSetMaximised(WS->WinHandle, !WindowIsMaximised(WS->WinHandle));
                                    }

                                    // NOTE(nathan): only show close background on hover
                                    if (UI_HOVERING(CloseSignal)) {
                                        CloseSignal.Box->Kind = (CloseSignal.Box->Kind | UI_BOX_KIND_DRAW_BACKGROUND);
                                    }
                                      
                                    if (UI_CLICKED(CloseSignal)) {
                                        if (WS->OrderNext != EMPTY_APP_WINDOW_STATE_VALUE || WS->OrderPrev != EMPTY_APP_WINDOW_STATE_VALUE) {
                                            UIContextMenuOpen(
                                                CloseContextMenuKey,
                                                CloseSignal.Box->Key,
                                                Vec(0.0f, Length(CloseSignal.Box->Rect).Y)
                                            );
                                        } else {
                                            AppCmd(APP_COMMAND_KIND_EXIT);
                                        }
                                    }
                                      
                                    WindowPushCustomTitleBarClientArea(WS->WinHandle, MinimiseSignal.Box->Rect);
                                    WindowPushCustomTitleBarClientArea(WS->WinHandle, MaximiseSignal.Box->Rect);
                                    WindowPushCustomTitleBarClientArea(WS->WinHandle, Widen(CloseSignal.Box->Rect, 2.0f));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // NOTE(nathan): Panel non-leaf UI (drag boundaries, drag+drop sites)
    b32 IsChangingPanelBoundaries = FALSE;

    for (
        ConfigPanelNode* Panel = PanelTree.Root;
        Panel != EMPTY_CFG_PANEL_NODE_VALUE;
        Panel = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, Panel).Next
    ) {
        if (Panel->Head == EMPTY_CFG_PANEL_NODE_VALUE)
            continue;

        Axis2D SplitAxis = Panel->SplitAxis;
        r2f32 PanelRect = ConfigTargetRectFromPanelNode(ContentRect, PanelTree.Root, Panel);

        {
            ConfigNode* DragView = ConfigNodeFromID(APP_STATE->DragDropRegisters->View);

            if (
                AppDragIsActive() && 
                APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW && 
                DragView != EMPTY_CFG_NODE_VALUE
            ) {
                f32 DropSiteMajorDimPX = CeilF32(UIHeadFontSize() * 7.0f);
                f32 DropSiteMinorDimPX = CeilF32(UIHeadFontSize() * 5.0f);
                f32 CornerRadius = UIHeadFontSize() * 0.5f;
                f32 Padding = CeilF32(UIHeadFontSize() * 0.5f);

                if (Panel == PanelTree.Root) {
                    UICornerRadius(CornerRadius) {
                        v2f32 PanelRectCentre = Centre(PanelRect);
                        Axis2D Axis = FLIP_AXIS(PanelTree.Root->SplitAxis);

                        for (SideKind Side = 0; Side < SIDE_COUNT; ++Side) {
                            UIKey Key = UIKeyFromStrFmt({}, (char*) "root_extra_split_%i", Side);
                            r2f32 SiteRect = PanelRect;

                            SiteRect.Point0.V[FLIP_AXIS(Axis)] = PanelRectCentre.V[FLIP_AXIS(Axis)] - DropSiteMajorDimPX / 2.0f;
                            SiteRect.Point1.V[FLIP_AXIS(Axis)] = PanelRectCentre.V[FLIP_AXIS(Axis)] + DropSiteMajorDimPX / 2.0f;
                            SiteRect.Point0.V[Axis] = PanelRect.V[Side].V[Axis] - DropSiteMinorDimPX / 2.0f;
                            SiteRect.Point1.V[Axis] = PanelRect.V[Side].V[Axis] + DropSiteMinorDimPX / 2.0f;

                            UIBox* SiteBox = EMPTY_UI_BOX_VALUE;

                            {
                                f32 SiteOpenT = UIAnimate(
                                    UIKeyFromStrFmt(Key, (char*) "open_t"), 
                                    1.0f,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                );

                                UIRect(SiteRect) {
                                    UISquish(0.1f - 0.1f * SiteOpenT) {
                                        UITransparency(1.0f - SiteOpenT) {
                                            SiteBox = UIBuildBoxFromKey(
                                                (
                                                    UI_BOX_KIND_DROP_SITE | 
                                                    UI_BOX_KIND_DRAW_HOT_EFFECTS
                                                ),
                                                Key
                                            );
                                            UISignalFromBox(SiteBox);
                                        }
                                    }
                                }

                                UIBox* SiteBoxVisualisation = EMPTY_UI_BOX_VALUE;

                                UIParent(SiteBox) {
                                    UIWidthFill() {
                                        UIHeightFill() {
                                            UIPadding(UI_PX(Padding, 1.0f)) {
                                                UIColumn() {
                                                    UIPadding(UI_PX(Padding, 1.0f)) {
                                                        UIGroupKey(Key) {
                                                            UISetNextChildLayoutAxis(FLIP_AXIS(Axis));
                                                            SiteBoxVisualisation = UIBuildBoxFromKey(
                                                                (
                                                                    UI_BOX_KIND_DRAW_BACKGROUND |
                                                                    UI_BOX_KIND_DRAW_BORDER |
                                                                    UI_BOX_KIND_DRAW_DROP_SHADOW |
                                                                    UI_BOX_KIND_DRAW_BACKGROUND_BLUR |
                                                                    UI_BOX_KIND_DRAW_HOT_EFFECTS
                                                                ),
                                                                EMPTY_UI_KEY_VALUE
                                                            );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                UIParent(SiteBoxVisualisation) {
                                    UIWidthFill() {
                                        UIHeightFill() {
                                            UIPadding(UI_PX(Padding, 1.0f)) {
                                                UISetNextChildLayoutAxis(Axis);

                                                UIBox* RowOrColumn = UIBuildBoxFromKey(0, {});

                                                UIParent(RowOrColumn) {
                                                    UIPadding(UI_PX(Padding, 1.0f)) {
                                                        UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, EMPTY_UI_KEY_VALUE);
                                                        UISpacer(UI_PX(Padding, 1.0f));
                                                        UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, EMPTY_UI_KEY_VALUE);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            if (SiteBox->Key == UIDropHotKey()) {
                                r2f32 FutureSplitRectTarget = SiteRect;

                                FutureSplitRectTarget.Point0.V[Axis] -= DropSiteMajorDimPX;
                                FutureSplitRectTarget.Point1.V[Axis] += DropSiteMajorDimPX;
                                FutureSplitRectTarget.Point0.V[FLIP_AXIS(Axis)] = PanelRect.Point0.V[FLIP_AXIS(Axis)];
                                FutureSplitRectTarget.Point1.V[FLIP_AXIS(Axis)] = PanelRect.Point1.V[FLIP_AXIS(Axis)];
                                FutureSplitRectTarget = Widen(FutureSplitRectTarget, -UIHeadFontSize() * 2.0f);

                                v2f32 FutureSplitRectTargetCentre = Centre(FutureSplitRectTarget);
                                r2f32 FutureSplitRect = {
                                    UIAnimate(
                                        UIKeyFromStr(
                                            {},
                                            "drop_site_v0"_s8
                                        ),
                                        FutureSplitRectTarget.X0,
                                        __Params.Initial = FutureSplitRectTargetCentre.X,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    ),
                                    UIAnimate(
                                        UIKeyFromStr(
                                            {},
                                            "drop_site_v1"_s8
                                        ),
                                        FutureSplitRectTarget.Y0,
                                        __Params.Initial = FutureSplitRectTargetCentre.Y,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    ),
                                    UIAnimate(
                                        UIKeyFromStr(
                                            {},
                                            "drop_site_v2"_s8
                                        ),
                                        FutureSplitRectTarget.X1,
                                        __Params.Initial = FutureSplitRectTargetCentre.X,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    ),
                                    UIAnimate(
                                        UIKeyFromStr(
                                            {},
                                            "drop_site_v3"_s8
                                        ),
                                        FutureSplitRectTarget.Y1,
                                        __Params.Initial = FutureSplitRectTargetCentre.Y,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    )
                                };

                                UIRect(FutureSplitRect) {
                                    UITag("drop_site"_s8) {
                                        UICornerRadius(UIHeadFontSize() * 2.0f) {
                                            UIBuildBoxFromKey(
                                                UI_BOX_KIND_DRAW_BACKGROUND_BLUR | UI_BOX_KIND_DRAW_BORDER, 
                                                EMPTY_UI_KEY_VALUE
                                            );
                                        }
                                    }
                                }
                            }

                            if (SiteBox->Key == UIDropHotKey() && AppDragDrop()) {
                                Direction2D Direction = (
                                    (Axis == AXIS_2D_Y)
                                        ? (Side == SIDE_MIN)
                                            ? DIR_2D_UP
                                            : DIR_2D_DOWN
                                        : (Axis == AXIS_2D_X)
                                            ? (Side == SIDE_MIN)
                                                ? DIR_2D_LEFT
                                                : DIR_2D_RIGHT
                                            : DIR_2D_INVALID
                                );

                                if (Direction != DIR_2D_INVALID) {
                                    ConfigPanelNode* SplitPanel = Panel;

                                    AppCmd(
                                        APP_COMMAND_KIND_SPLIT_PANEL,
                                        __Registers.DstPanel  = SplitPanel->Config->ID,
                                        __Registers.Panel = APP_STATE->DragDropRegisters->Panel,
                                        __Registers.View = APP_STATE->DragDropRegisters->View,
                                        __Registers.Direction = Direction
                                    );
                                }
                            }
                        }
                    }
                }

                Axis2D SplitAxis = Panel->SplitAxis;

                UICornerRadius(CornerRadius) {
                    for (
                        ConfigPanelNode* Child = Panel->Head;
                        ;
                        Child = Child->Next
                    ) {
                        r2f32 ChildRect = ConfigTargetRectFromPanelNodeChild(
                            PanelRect, 
                            Panel, 
                            Child
                        );
                        v2f32 ChildRectCentre = Centre(ChildRect);
                        UIKey Key = UIKeyFromStrFmt({}, (char*) "drop_boundary_%p_%p", Panel->Config, Child->Config);
                        r2f32 SiteRect = Rng(ChildRectCentre, ChildRectCentre);

                        SiteRect.Point0.V[SplitAxis] = ChildRect.Point0.V[SplitAxis] - DropSiteMinorDimPX / 2.0f;
                        SiteRect.Point1.V[SplitAxis] = ChildRect.Point0.V[SplitAxis] + DropSiteMinorDimPX / 2.0f;
                        SiteRect.Point0.V[FLIP_AXIS(SplitAxis)] -= DropSiteMajorDimPX / 2.0f;
                        SiteRect.Point1.V[FLIP_AXIS(SplitAxis)] += DropSiteMajorDimPX / 2.0f;

                        UIBox* SiteBox = EMPTY_UI_BOX_VALUE;

                        {
                            f32 SiteOpenT = UIAnimate(
                                UIKeyFromStr(
                                    Key, 
                                    "open_t"_s8
                                ), 
                                1.0f, 
                                __Params.Rate = APP_STATE->MenuAnimationRate
                            );

                            UIRect(SiteRect) {
                                UISquish(0.1f - 0.1f * SiteOpenT) {
                                    UITransparency(1.0f - SiteOpenT) {
                                        SiteBox = UIBuildBoxFromKey(
                                            UI_BOX_KIND_DROP_SITE | UI_BOX_KIND_DRAW_HOT_EFFECTS, 
                                            Key
                                        );
                                        UISignalFromBox(SiteBox);
                                    }
                                }
                            }

                            UIBox* SiteBoxVisualisation = EMPTY_UI_BOX_VALUE;

                            UIParent(SiteBox) {
                                UIWidthFill() {
                                    UIHeightFill() {
                                        UIPadding(UI_PX(Padding, 1.0f)) {
                                            UIColumn() {
                                                UIPadding(UI_PX(Padding, 1.0f)) {
                                                    UIGroupKey(Key) {
                                                        UISetNextChildLayoutAxis(FLIP_AXIS(SplitAxis));
                                                        SiteBoxVisualisation = UIBuildBoxFromKey(
                                                            (
                                                                UI_BOX_KIND_DRAW_BACKGROUND |
                                                                UI_BOX_KIND_DRAW_BORDER |
                                                                UI_BOX_KIND_DRAW_DROP_SHADOW |
                                                                UI_BOX_KIND_DRAW_BACKGROUND_BLUR |
                                                                UI_BOX_KIND_DRAW_HOT_EFFECTS
                                                            ),
                                                            EMPTY_UI_KEY_VALUE
                                                        );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            UIParent(SiteBoxVisualisation) {
                                UIWidthFill() {
                                    UIHeightFill() {
                                        UIPadding(UI_PX(Padding, 1.0f)) {
                                            UISetNextChildLayoutAxis(SplitAxis);

                                            UIBox* RowOrColumn = UIBuildBoxFromKey(
                                                0,
                                                EMPTY_UI_KEY_VALUE
                                            );

                                            UIParent(RowOrColumn) {
                                                UIPadding(UI_PX(Padding, 1.0f)) {
                                                    UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, EMPTY_UI_KEY_VALUE);
                                                    UISpacer(UI_PX(Padding, 1.0f));
                                                    UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, EMPTY_UI_KEY_VALUE);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (SiteBox->Key == UIDropHotKey()) {
                            r2f32 FutureSplitRectTarget = SiteRect;

                            FutureSplitRectTarget.Point0.V[SplitAxis] -= DropSiteMajorDimPX;
                            FutureSplitRectTarget.Point1.V[SplitAxis] += DropSiteMajorDimPX;
                            FutureSplitRectTarget.Point0.V[FLIP_AXIS(SplitAxis)] = ChildRect.Point0.V[FLIP_AXIS(SplitAxis)];
                            FutureSplitRectTarget.Point1.V[FLIP_AXIS(SplitAxis)] = ChildRect.Point1.V[FLIP_AXIS(SplitAxis)];
                            FutureSplitRectTarget = Widen(FutureSplitRectTarget, -UIHeadFontSize() * 2.0f);

                            v2f32 FutureSplitRectTargetCentre = Centre(FutureSplitRectTarget);
                            r2f32 FutureSplitRect = {
                                UIAnimate(
                                    UIKeyFromStr(
                                        EMPTY_UI_KEY_VALUE,
                                        "drop_site_v0"_s8
                                    ),
                                    FutureSplitRectTarget.X0,
                                    __Params.Initial = FutureSplitRectTargetCentre.X,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                ),
                                UIAnimate(
                                    UIKeyFromStr(
                                        EMPTY_UI_KEY_VALUE,
                                        "drop_site_v1"_s8
                                    ),
                                    FutureSplitRectTarget.Y0,
                                    __Params.Initial = FutureSplitRectTargetCentre.Y,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                ),
                                UIAnimate(
                                    UIKeyFromStr(
                                        EMPTY_UI_KEY_VALUE,
                                        "drop_site_v2"_s8
                                    ),
                                    FutureSplitRectTarget.X1,
                                    __Params.Initial = FutureSplitRectTargetCentre.X,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                ),
                                UIAnimate(
                                    UIKeyFromStr(
                                        EMPTY_UI_KEY_VALUE,
                                        "drop_site_v3"_s8
                                    ),
                                    FutureSplitRectTarget.Y1,
                                    __Params.Initial = FutureSplitRectTargetCentre.Y,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                )
                            };

                            UIRect(FutureSplitRect) {
                                UITag("drop_site"_s8) {
                                    UICornerRadius(UIHeadFontSize() * 2.0f) {
                                        UIBuildBoxFromKey(
                                            UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_BORDER,
                                            EMPTY_UI_KEY_VALUE
                                        );
                                    }
                                }
                            }
                        }

                        if (SiteBox->Key == UIDropHotKey() && AppDragDrop()) {
                            Direction2D Direction = (Panel->SplitAxis == AXIS_2D_X) ? DIR_2D_LEFT : DIR_2D_UP;
                            ConfigPanelNode* SplitPanel = Child;

                            if (SplitPanel == EMPTY_CFG_PANEL_NODE_VALUE) {
                                SplitPanel = Panel->Tail;
                                Direction = (Panel->SplitAxis == AXIS_2D_X) ? DIR_2D_RIGHT : DIR_2D_DOWN;
                            }

                            AppCmd(
                                APP_COMMAND_KIND_SPLIT_PANEL,
                                __Registers.DstPanel = SplitPanel->Config->ID,
                                __Registers.Panel = APP_STATE->DragDropRegisters->Panel,
                                __Registers.View = APP_STATE->DragDropRegisters->View,
                                __Registers.Direction = Direction
                            );
                        }

                        if (Child == EMPTY_CFG_PANEL_NODE_VALUE)
                            break;
                    }
                }
            }
        }

        //
        // NOTE(nathan): Do UI for drag boundaries between all children
        for (
            ConfigPanelNode* Child = Panel->Head;
            Child != EMPTY_CFG_PANEL_NODE_VALUE && Child->Next != EMPTY_CFG_PANEL_NODE_VALUE;
            Child = Child->Next
        ) {
            ConfigPanelNode* MinChild = Child;
            ConfigPanelNode* MaxChild = MinChild->Next;
            r2f32 MinChildRect = ConfigTargetRectFromPanelNodeChild(
                PanelRect, 
                Panel, 
                MinChild
            );
            r2f32 MaxChildRect = ConfigTargetRectFromPanelNodeChild(
                PanelRect, 
                Panel, 
                MaxChild
            );
            r2f32 BoundaryRect = {};

            {
                BoundaryRect.Point0.V[SplitAxis] = MinChildRect.Point1.V[SplitAxis] - UIHeadFontSize() / 3.0f;
                BoundaryRect.Point1.V[SplitAxis] = MaxChildRect.Point0.V[SplitAxis] + UIHeadFontSize() / 3.0f;
                BoundaryRect.Point0.V[FLIP_AXIS(SplitAxis)] = PanelRect.Point0.V[FLIP_AXIS(SplitAxis)];
                BoundaryRect.Point1.V[FLIP_AXIS(SplitAxis)] = PanelRect.Point1.V[FLIP_AXIS(SplitAxis)];
            }

            UIRect(BoundaryRect) {
                UISetNextHoverCursor((SplitAxis == AXIS_2D_X) ? CURSOR_KIND_LEFT_RIGHT : CURSOR_KIND_UP_DOWN);

                UIBox* Box = UIBuildBoxFromStrFmt(
                    UI_BOX_KIND_CLICKABLE,
                    "###%p_%p",
                    MinChild->Config,
                    MaxChild->Config
                );
                UISignal Sig = UISignalFromBox(Box);

                if (UI_DOUBLE_CLICKED(Sig)) {
                    UIKillAction();

                    f32 SumPercent = MinChild->PercentOfParent + MaxChild->PercentOfParent;

                    MinChild->PercentOfParent = 0.5f * SumPercent;
                    MaxChild->PercentOfParent = 0.5f * SumPercent;
                    ConfigNodeEquipStr(
                        APP_STATE->Config, 
                        MinChild->Config, 
                        "%f", 
                        MinChild->PercentOfParent
                    );
                    ConfigNodeEquipStr(
                        APP_STATE->Config, 
                        MaxChild->Config, 
                        "%f", 
                        MaxChild->PercentOfParent
                    );
                } else if (UI_PRESSED(Sig)) {
                    v2f32 V = {
                        MinChild->PercentOfParent,
                        MaxChild->PercentOfParent
                    };

                    UIStoreDragStruct(&V);
                } else if (UI_DRAGGING(Sig)) {
                    v2f32 V = *UIGetDragStruct(v2f32);
                    v2f32 MouseDelta = UIDragDelta();
                    f32 TotalSize = Length(PanelRect).V[SplitAxis];
                    f32 MinPercentBefore = V.V[0];
                    f32 MinPixelsBefore = MinPercentBefore * TotalSize;
                    f32 MinPixelsAfter = MinPixelsBefore + MouseDelta.V[SplitAxis];

                    if (MinPixelsAfter < 50.0f)
                        MinPixelsAfter = 50.0f;

                    f32 MinPercentAfter = MinPixelsAfter / TotalSize;
                    f32 PercentDelta = MinPercentAfter - MinPercentBefore;
                    f32 MaxPercentBefore = V.V[1];
                    f32 MaxPercentAfter = MaxPercentBefore - PercentDelta;
                    f32 MaxPixelsAfter = MaxPercentAfter * TotalSize;

                    if (MaxPixelsAfter < 50.0f) {
                        MaxPixelsAfter = 50.0f;
                        MaxPercentAfter = MaxPixelsAfter / TotalSize;
                        PercentDelta = -(MaxPercentAfter - MaxPercentBefore);
                        MinPercentAfter = MinPercentBefore + PercentDelta;
                    }

                    MinChild->PercentOfParent = MinPercentAfter;
                    MaxChild->PercentOfParent = MaxPercentAfter;
                    ConfigNodeEquipStr(
                        APP_STATE->Config, 
                        MinChild->Config, 
                        "%f", 
                        MinPercentAfter
                    );
                    ConfigNodeEquipStr(
                        APP_STATE->Config, 
                        MaxChild->Config, 
                        "%f", 
                        MaxPercentAfter
                    );
                    IsChangingPanelBoundaries = TRUE;
                }
            }
        }
    }

    // NOTE(nathan): Animate panels
    {
        b32 WindowIsResizing = (
            WS->LastWindowRect.X1 != WindowRect.X1 ||
            WS->LastWindowRect.Y1 != WindowRect.Y1
        );
        v2f32 ContentRectDim = Length(ContentRect);

        if (ContentRectDim.X > 0.0f && ContentRectDim.Y > 0.0f) {
            for (
                ConfigPanelNode* Panel = PanelTree.Root;
                Panel != EMPTY_CFG_PANEL_NODE_VALUE;
                Panel = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, Panel).Next
            ) {
                r2f32 TargetRectPX = ConfigTargetRectFromPanelNode(
                    ContentRect, 
                    PanelTree.Root, 
                    Panel
                );
                r2f32 TargetRectPercent = Rng(
                    TargetRectPX.X0 / ContentRectDim.X,
                    TargetRectPX.Y0 / ContentRectDim.Y,
                    TargetRectPX.X1 / ContentRectDim.X,
                    TargetRectPX.Y1 / ContentRectDim.Y
                );
                b32 Reset = (WindowIsResizing || WS->WindowLayoutReset || WS->FramesAlive < 5 || IsChangingPanelBoundaries);

                UIAnimate(
                    UIKeyFromStrFmt(
                        {},
                        "panel_%p_x0",
                        Panel->Config
                    ),
                    TargetRectPercent.X0,
                    __Params.Initial = TargetRectPercent.X0,
                    __Params.Reset = Reset,
                    __Params.Rate = APP_STATE->MenuAnimationRate
                );
                UIAnimate(
                    UIKeyFromStrFmt(
                        {},
                        "panel_%p_y0",
                        Panel->Config
                    ),
                    TargetRectPercent.Y0,
                    __Params.Initial = TargetRectPercent.Y0,
                    __Params.Reset = Reset,
                    __Params.Rate = APP_STATE->MenuAnimationRate
                );
                UIAnimate(
                    UIKeyFromStrFmt(
                        {},
                        "panel_%p_x1",
                        Panel->Config
                    ),
                    TargetRectPercent.X1,
                    __Params.Initial = TargetRectPercent.X1,
                    __Params.Reset = Reset,
                    __Params.Rate = APP_STATE->MenuAnimationRate
                );
                UIAnimate(
                    UIKeyFromStrFmt(
                        {},
                        "panel_%p_y1",
                        Panel->Config
                    ),
                    TargetRectPercent.Y1,
                    __Params.Initial = TargetRectPercent.Y1,
                    __Params.Reset = Reset,
                    __Params.Rate = APP_STATE->MenuAnimationRate
                );
            }
        }

        WS->WindowLayoutReset = FALSE;
    }

    // NOTE(nathan): Panel leaf UI
    if (ContentRect.X1 > ContentRect.X0 && ContentRect.Y1 > ContentRect.Y0) {
        for (
            ConfigPanelNode* Panel = PanelTree.Root;
            Panel != EMPTY_CFG_PANEL_NODE_VALUE;
            Panel = ConfigPanelNodeRecDepthFirstPre(PanelTree.Root, Panel).Next
        ) {
            if (Panel->Head != EMPTY_CFG_PANEL_NODE_VALUE)
                continue;

            b32 PanelIsFocused = (
                IsWindowFocused &&
                !WS->MenuBarFocused &&
                !QueryIsOpen &&
                !UIAnyContextMenuIsOpen() &&
                PanelTree.Focused == Panel
            );
            ConfigNode* SelectedTab = Panel->SelectedTab;
            AppViewState* SelectedTabViewState = AppViewStateFromConfig(SelectedTab);

            UIFocus((PanelIsFocused) ? UI_FOCUS_KIND_NULL : UI_FOCUS_KIND_OFF) {
                v2f32 ContentRectDim = Length(ContentRect);
                r2f32 TargetRectPX = ConfigTargetRectFromPanelNode(ContentRect, PanelTree.Root, Panel);
                r2f32 TargetRectPercent = Rng(
                    TargetRectPX.X0 / ContentRectDim.X,
                    TargetRectPX.Y0 / ContentRectDim.Y,
                    TargetRectPX.X1 / ContentRectDim.X,
                    TargetRectPX.Y1 / ContentRectDim.Y
                );
                r2f32 PanelRectPercent = Rng(
                    UIAnimate(
                        UIKeyFromStrFmt(
                            {},
                             "panel_%p_x0",
                            Panel->Config
                        ),
                        TargetRectPercent.X0,
                        __Params.Initial = TargetRectPercent.X0,
                        __Params.Rate = APP_STATE->MenuAnimationRate
                    ),
                    UIAnimate(
                        UIKeyFromStrFmt(
                            {},
                            "panel_%p_y0",
                            Panel->Config
                        ),
                        TargetRectPercent.Y0,
                        __Params.Initial = TargetRectPercent.Y0,
                        __Params.Rate = APP_STATE->MenuAnimationRate
                    ),
                    UIAnimate(
                        UIKeyFromStrFmt(
                            {},
                            "panel_%p_x1",
                            Panel->Config
                        ),
                        TargetRectPercent.X1,
                        __Params.Initial = TargetRectPercent.X1,
                        __Params.Rate = APP_STATE->MenuAnimationRate
                    ),
                    UIAnimate(
                        UIKeyFromStrFmt(
                            {},
                            "panel_%p_y1",
                            Panel->Config
                        ),
                        TargetRectPercent.Y1,
                        __Params.Initial = TargetRectPercent.Y1,
                        __Params.Rate = APP_STATE->MenuAnimationRate
                    )
                );
                r2f32 PanelRect = Rng(
                    PanelRectPercent.X0 * ContentRectDim.X,
                    PanelRectPercent.Y0 * ContentRectDim.Y,
                    PanelRectPercent.X1 * ContentRectDim.X,
                    PanelRectPercent.Y1 * ContentRectDim.Y
                );

                PanelRect = Widen(PanelRect, FloorF32(-UIHeadFontSize() * 0.15f));
                PanelRect = Rng(
                    RoundF32(PanelRect.X0),
                    RoundF32(PanelRect.Y0),
                    RoundF32(PanelRect.X1),
                    RoundF32(PanelRect.Y1)
                );

                f32 TabBarRHeight = FloorF32(UIHeadFontSize() * 3.5f);
                f32 TabBarVHeight = FloorF32(UIHeadFontSize() * AppSettingFromNameF32("tab_height"_s8));
                f32 TabBarRVDiff = TabBarRHeight - TabBarVHeight;
                f32 TabSpacing = FloorF32(UIHeadFontSize() * 0.4f);
                r2f32 TabBarRect = Rng(
                    PanelRect.X0, 
                    PanelRect.Y0, 
                    PanelRect.X1, 
                    PanelRect.Y0 + TabBarVHeight
                );
                r2f32 ContentRect = Rng(
                    PanelRect.X0, 
                    PanelRect.Y0 + TabBarVHeight, 
                    PanelRect.X1, 
                    PanelRect.Y1
                );

                if (Panel->TabSide == SIDE_MAX) {
                    TabBarRect.Y0 = PanelRect.Y1 - TabBarVHeight;
                    TabBarRect.Y1 = PanelRect.Y1;
                    ContentRect.Y0 = PanelRect.Y0;
                    ContentRect.Y1 = PanelRect.Y1 - TabBarVHeight;
                }

                TabBarRect = Intersect(TabBarRect, PanelRect);
                ContentRect = Intersect(ContentRect, PanelRect);

                b32 BuildPanel = (
                    ContentRect.X1 > ContentRect.X0 &&
                    ContentRect.Y1 > ContentRect.Y0
                );

                if (BuildPanel) {
                    ConfigNode* View = ConfigNodeFromID(APP_STATE->DragDropRegisters->View);

                    if (
                        AppDragIsActive() &&
                        APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW &&
                        View != EMPTY_CFG_NODE_VALUE &&
                        InRange(PanelRect, UIMouse()) &&
                        UIDropHotKey() == EMPTY_UI_KEY_VALUE
                    ) {
                        f32 DropSiteDimPX = CeilF32(UIHeadFontSize() * 7.0f);

                        DropSiteDimPX = MIN(DropSiteDimPX, Length(PanelRect).V[Panel->SplitAxis] / 4.0f);
                        DropSiteDimPX = MAX(DropSiteDimPX, CeilF32(UIHeadFontSize() * 3.0f));

                        v2f32 DropSiteHalfDim = Vec(DropSiteDimPX / 2.0f, DropSiteDimPX / 2.0f);
                        v2f32 PanelCentre = Centre(PanelRect);
                        f32 CornerRadius = UIHeadFontSize() * 0.5f;
                        f32 Padding = CeilF32(UIHeadFontSize() * 0.5f);

                        struct {
                            UIKey       Key;
                            Direction2D SplitDirection;
                            r2f32       Rect;
                        } Sites[] = {
                            {
                                UIKeyFromStrFmt({}, "drop_split_centre_%p", Panel->Config),
                                DIR_2D_INVALID,
                                Rng(
                                    PanelCentre - DropSiteHalfDim, 
                                    PanelCentre + DropSiteHalfDim
                                )
                            },
                            {
                                UIKeyFromStrFmt({}, "drop_split_up_%p", Panel->Config),
                                DIR_2D_UP,
                                Rng(
                                    Vec(
                                        PanelCentre.X - DropSiteHalfDim.X,
                                        PanelCentre.Y - DropSiteHalfDim.Y - DropSiteHalfDim.Y * 2.0f
                                    ),
                                    Vec(
                                        PanelCentre.X + DropSiteHalfDim.X,
                                        PanelCentre.Y + DropSiteHalfDim.Y - DropSiteHalfDim.Y * 2.0f
                                    )
                                )
                            },
                            {
                                UIKeyFromStrFmt({}, "drop_split_down_%p", Panel->Config),
                                DIR_2D_DOWN,
                                Rng(
                                    Vec(
                                        PanelCentre.X - DropSiteHalfDim.X,
                                        PanelCentre.Y - DropSiteHalfDim.Y + DropSiteHalfDim.Y * 2.0f
                                    ),
                                    Vec(
                                        PanelCentre.X + DropSiteHalfDim.X,
                                        PanelCentre.Y + DropSiteHalfDim.Y + DropSiteHalfDim.Y * 2.0f
                                    )
                                )
                            },
                            {
                                UIKeyFromStrFmt({}, "drop_split_left_%p", Panel->Config),
                                DIR_2D_LEFT,
                                Rng(
                                    Vec(
                                        PanelCentre.X - DropSiteHalfDim.X - DropSiteHalfDim.X * 2.0f,
                                        PanelCentre.Y - DropSiteHalfDim.Y
                                    ),
                                    Vec(
                                        PanelCentre.X + DropSiteHalfDim.X - DropSiteHalfDim.X * 2.0f,
                                        PanelCentre.Y + DropSiteHalfDim.Y
                                    )
                                )
                            },
                            {
                                UIKeyFromStrFmt({}, "drop_split_right_%p", Panel->Config),
                                DIR_2D_RIGHT,
                                Rng(
                                    Vec(
                                        PanelCentre.X - DropSiteHalfDim.X + DropSiteHalfDim.X * 2.0f,
                                        PanelCentre.Y - DropSiteHalfDim.Y
                                    ),
                                    Vec(
                                        PanelCentre.X + DropSiteHalfDim.X + DropSiteHalfDim.X * 2.0f,
                                        PanelCentre.Y + DropSiteHalfDim.Y
                                    )
                                )
                            }
                        };

                        UICornerRadius(CornerRadius) {
                            for (u64 Index = 0; Index < ARRAY_COUNT(Sites); ++Index) {
                                UIKey Key = Sites[Index].Key;
                                Direction2D Direction = Sites[Index].SplitDirection;
                                r2f32 Rect = Sites[Index].Rect;
                                Axis2D SplitAxis = Axis2DFromDir2D(Direction);
                                SideKind SplitSide = SideFromDir2D(Direction);

                                if (Direction != DIR_2D_INVALID && SplitAxis == Panel->Parent->SplitAxis)
                                    continue;

                                UIBox* SiteBox = EMPTY_UI_BOX_VALUE;

                                {
                                    f32 SiteOpenT = UIAnimate(
                                        UIKeyFromStr(Key, "open_t"_s8),
                                        1.0f,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    );

                                    UIRect(Rect) {
                                        UISquish(0.1f - 0.1f * SiteOpenT) {
                                            UITransparency(1.0f - SiteOpenT) {
                                                SiteBox = UIBuildBoxFromKey(UI_BOX_KIND_DROP_SITE | UI_BOX_KIND_DRAW_HOT_EFFECTS, Key);
                                                UISignalFromBox(SiteBox);
                                            }
                                        }
                                    }

                                    UIBox* SiteBoxVisualisation = EMPTY_UI_BOX_VALUE;

                                    UIGroupKey(Key) {
                                        UIParent(SiteBox) {
                                            UIWidthFill() {
                                                UIHeightFill() {
                                                    UIPadding(UI_PX(Padding, 1.0f)) {
                                                        UIColumn() {
                                                            UIPadding(UI_PX(Padding, 1.0f)) {
                                                                UISetNextChildLayoutAxis(SplitAxis);
                                                                SiteBoxVisualisation = UIBuildBoxFromKey(
                                                                    (
                                                                        UI_BOX_KIND_DRAW_BACKGROUND |
                                                                        UI_BOX_KIND_DRAW_BORDER |
                                                                        UI_BOX_KIND_DRAW_DROP_SHADOW |
                                                                        UI_BOX_KIND_DRAW_BACKGROUND_BLUR |
                                                                        UI_BOX_KIND_DRAW_HOT_EFFECTS
                                                                    ),
                                                                    EMPTY_UI_KEY_VALUE
                                                                );
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if (Direction != DIR_2D_INVALID) {
                                        UIParent(SiteBoxVisualisation) {
                                            UIWidthFill() {
                                                UIHeightFill() {
                                                    UIPadding(UI_PX(Padding, 1.0f)) {
                                                        UISetNextChildLayoutAxis(SplitAxis);

                                                        UIBox* RowOrColumn = UIBuildBoxFromKey(0, {});

                                                        UIParent(RowOrColumn) {
                                                            UIPadding(UI_PX(Padding, 1.0f)) {
                                                                UITag("drop_site"_s8) {
                                                                    if (SplitSide == SIDE_MIN)
                                                                        UISetNextFlags(UI_BOX_KIND_DRAW_BACKGROUND);

                                                                    UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, EMPTY_UI_KEY_VALUE);
                                                                    UISpacer(UI_PX(Padding, 1.0f));

                                                                    if (SplitSide == SIDE_MAX)
                                                                        UISetNextFlags(UI_BOX_KIND_DRAW_BACKGROUND);

                                                                    UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, EMPTY_UI_KEY_VALUE);
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        UIParent(SiteBoxVisualisation) {
                                            UIWidthFill() {
                                                UIHeightFill() {
                                                    UIPadding(UI_PX(Padding, 1.0f)) {
                                                        UISetNextChildLayoutAxis(SplitAxis);

                                                        UIBox* RowOrColumn = UIBuildBoxFromKey(0, EMPTY_UI_KEY_VALUE);

                                                        UIParent(RowOrColumn) {
                                                            UIPadding(UI_PX(Padding, 1.0f)) {
                                                                UITag("drop_site"_s8) {
                                                                    UIBuildBoxFromKey(
                                                                        UI_BOX_KIND_DRAW_BORDER | UI_BOX_KIND_DRAW_HOT_EFFECTS,
                                                                        {}
                                                                    );
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                if (SiteBox->Key == UIDropHotKey() && AppDragDrop()) {
                                    if (Direction != DIR_2D_INVALID) {
                                        AppCmd(
                                            APP_COMMAND_KIND_SPLIT_PANEL,
                                            __Registers.DstPanel = Panel->Config->ID,
                                            __Registers.Panel = APP_STATE->DragDropRegisters->Panel,
                                            __Registers.View = APP_STATE->DragDropRegisters->View,
                                            __Registers.Direction = Direction
                                        );
                                    } else {
                                        AppCmd(
                                            APP_COMMAND_KIND_MOVE_VIEW,
                                            __Registers.DstPanel = Panel->Config->ID,
                                            __Registers.Panel = APP_STATE->DragDropRegisters->Panel,
                                            __Registers.View = APP_STATE->DragDropRegisters->View,
                                            __Registers.PrevTab = ConfigTailNode(&Panel->Tabs)->ID
                                        );
                                    }
                                }
                            }
                        }

                        for (u64 Index = 0; Index < ARRAY_COUNT(Sites); ++Index) {
                            b32 IsDropHot = UIDropHotKey() == Sites[Index].Key;

                            if (IsDropHot) {
                                Axis2D SplitAxis = Axis2DFromDir2D(Sites[Index].SplitDirection);
                                SideKind SplitSide = SideFromDir2D(Sites[Index].SplitDirection);
                                r2f32 FutureSplitRectTarget = PanelRect;

                                if (Sites[Index].SplitDirection != DIR_2D_INVALID) {
                                    v2f32 PanelCentre = Centre(PanelRect);

                                    FutureSplitRectTarget.V[FLIP_SIDE(SplitSide)].V[SplitAxis] = PanelCentre.V[SplitAxis];
                                }

                                FutureSplitRectTarget = Widen(FutureSplitRectTarget, -UIHeadFontSize() * 2.0f);

                                v2f32 FutureSplitRectTargetCentre = Centre(FutureSplitRectTarget);
                                r2f32 FutureSplitRect = {
                                    UIAnimate(
                                        UIKeyFromStr({}, "drop_site_v0"_s8),
                                        FutureSplitRectTarget.X0,
                                        __Params.Initial = FutureSplitRectTargetCentre.X,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    ),
                                    UIAnimate(
                                        UIKeyFromStr({}, "drop_site_v1"_s8),
                                        FutureSplitRectTarget.Y0,
                                        __Params.Initial = FutureSplitRectTargetCentre.Y,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    ),
                                    UIAnimate(
                                        UIKeyFromStr({}, "drop_site_v2"_s8),
                                        FutureSplitRectTarget.X1,
                                        __Params.Initial = FutureSplitRectTargetCentre.X,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    ),
                                    UIAnimate(
                                        UIKeyFromStr({}, "drop_site_v3"_s8),
                                        FutureSplitRectTarget.Y1,
                                        __Params.Initial = FutureSplitRectTargetCentre.Y,
                                        __Params.Rate = APP_STATE->MenuAnimationRate
                                    )
                                };

                                UIRect(FutureSplitRect) {
                                    UITag("drop_site"_s8) {
                                        UIBuildBoxFromKey(
                                            UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_BORDER,
                                            {}
                                        );
                                    }
                                }
                            }
                        }
                    }
                }

                UIKey CatchAllDropSiteKey = UIKeyFromStrFmt(
                    {}, 
                    "catchall_drop_site_%p", 
                    Panel->Config
                );

                if (
                    BuildPanel && 
                    AppDragIsActive() && 
                    APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW
                ) {
                    UIRect(PanelRect) {
                        UIBox* CatchAllDropSite = UIBuildBoxFromKey(
                            UI_BOX_KIND_DROP_SITE, 
                            CatchAllDropSiteKey
                        );

                        UISignalFromBox(CatchAllDropSite);
                    }
                }

                if (BuildPanel) {
                    if (Panel != PanelTree.Focused) {
                        UIRect(ContentRect) {
                            UITag("inactive"_s8) {
                                UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BACKGROUND, {});
                            }
                        }
                    }
                }

                UIBox* PanelBox = EMPTY_UI_BOX_VALUE;

                if (BuildPanel) {
                    UIRect(ContentRect) {
                        UIChildLayoutAxis(AXIS_2D_Y) {
                            UICornerRadius(0) {
                                UIFocus(UI_FOCUS_KIND_ON) {
                                    UIKey PanelKey = UIKeyFromStrFmt({}, "panel_box_%p", Panel->Config);

                                    PanelBox = UIBuildBoxFromKey(
                                        (
                                            UI_BOX_KIND_MOUSE_CLICKABLE |
                                            UI_BOX_KIND_CLIP |
                                            UI_BOX_KIND_DRAW_BORDER |
                                            UI_BOX_KIND_DISABLE_FOCUS_OVERLAY |
                                            ((PanelTree.Focused != Panel) * UI_BOX_KIND_DISABLE_FOCUS_BORDER)
                                        ),
                                        PanelKey
                                    );
                                }
                            }
                        }
                    }
                }

                UIBox* LoadingOverlayContainer = EMPTY_UI_BOX_VALUE;

                if (BuildPanel) {
                    UIParent(PanelBox) {
                        UIWidthFill() {
                            UIHeightFill() {
                                LoadingOverlayContainer = UIBuildBoxFromKey(UI_BOX_KIND_FLOATING, {});
                            }
                        }
                    }
                }

                if (BuildPanel) {
                    UIParent(PanelBox) {
                        UIFocus((PanelIsFocused) ? UI_FOCUS_KIND_NULL : UI_FOCUS_KIND_OFF) {
                            UIWidthFill() {
                                AppPushRegisters(
                                    // __Registers.Window = Registers()->Window,
                                    __Registers.Panel = Panel->Config->ID,
                                    __Registers.Tab = SelectedTab->ID,
                                    __Registers.View = SelectedTab->ID
                                );

                                {
                                    Str8 ViewExpr = AppExpressionFromConfig(SelectedTab);
                                    Str8 ViewFilePath = AppFilePathFromEvalStr(
                                        AppFrameMemPool(),
                                        ViewExpr
                                    );

                                    if (ViewFilePath.Size)
                                        Registers()->FilePath = ViewFilePath;
                                }

                                UIBox* ViewDropSite = EMPTY_UI_BOX_VALUE;

                                {
                                    AppViewUIRule* ViewUIRule = AppViewUIRuleFromStr(
                                        SelectedTab->String
                                    );

                                    if (
                                        ViewUIRule != EMPTY_APP_VIEW_UI_RULE_VALUE &&
                                        AppDragIsActive() &&
                                        APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_EXPRESSION &&
                                        !StrMatch(SelectedTab->String, "text"_s8, 0)
                                    ) {
                                        UIFixedSize(Length(ContentRect)) {
                                            ViewDropSite = UIBuildBoxFromStrFmt(
                                                UI_BOX_KIND_DROP_SITE | UI_BOX_KIND_FLOATING,
                                                "drop_site_%llx",
                                                SelectedTab->ID
                                            );
                                        }
                                    }
                                }

                                UIBox* ViewContainerBox = EMPTY_UI_BOX_VALUE;

                                UIFixedWidth(Length(ContentRect).X) {
                                    UIFixedHeight(Length(ContentRect).Y) {
                                        UIChildLayoutAxis(AXIS_2D_Y) {
                                            ViewContainerBox = UIBuildBoxFromKey(0, EMPTY_UI_KEY_VALUE);
                                        }
                                    }
                                }

                                UIParent(ViewContainerBox) {
                                    if (
                                        SelectedTab == EMPTY_CFG_NODE_VALUE &&
                                        Panel->Parent != EMPTY_CFG_PANEL_NODE_VALUE
                                    ) {
                                        UISetNextFlags(UI_BOX_KIND_DEFAULT_FOCUS_NAV);

                                        UIFocus(UI_FOCUS_KIND_ON) {
                                            UIWidthFill() {
                                                UIHeightFill() {
                                                    UINamedColumn("empty_view"_s8) {
                                                        UITag("weak"_s8) {
                                                            UIPadding(UI_PERCENT(1, 0)) {
                                                                UIFocus(UI_FOCUS_KIND_NULL) {
                                                                    UIPreferredHeight(UI_EM(3.0f, 1.0f)) {
                                                                        UIRow() {
                                                                            UIPadding(UI_PERCENT(1, 0)) {
                                                                                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                                                    UIPreferredWidth(UI_EM(15.0f, 1.0f)) {
                                                                                        UITag("bad_pop"_s8) {
                                                                                            if (UI_CLICKED(AppIconButton(APP_ICON_KIND_X, NULL, "Close Panel"_s8)))
                                                                                                AppCmd(APP_COMMAND_KIND_CLOSE_PANEL);
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }

                                UIParent(ViewContainerBox) {
                                    if (SelectedTab != EMPTY_CFG_NODE_VALUE) {
                                        AppViewUI(ContentRect);
                                    }
                                }

                                if (ViewDropSite != EMPTY_UI_BOX_VALUE) {
                                    UISignal Sig = UISignalFromBox(ViewDropSite);

                                    if (ViewDropSite->Key == UIDropHotKey()) {
                                        UIParent(ViewDropSite) {
                                            UIWidthFill() {
                                                UIHeightFill() {
                                                    UITag("drop_site"_s8) {
                                                        UIBuildBoxFromKey(
                                                            (
                                                                UI_BOX_KIND_DRAW_BACKGROUND |
                                                                UI_BOX_KIND_DRAW_BORDER
                                                            ),
                                                            EMPTY_UI_KEY_VALUE
                                                        );
                                                    }
                                                }
                                            }
                                        }

                                        if (AppDragDrop()) {
                                            AppStoreViewExprStr(APP_STATE->DragDropRegisters->Expression);
                                        }
                                    }
                                }

                                AppRegisters* ViewRegs = AppPopRegisters();

                                if (PanelIsFocused)
                                    MemCpy(Registers(), ViewRegs, sizeof(AppRegisters));
                            }
                        }
                    }
                }

                if (BuildPanel) {
                    f32 SelectedTabLoadingT = SelectedTabViewState->LoadingT;

                    if (SelectedTabLoadingT > 0.01f) {
                        UIParent(LoadingOverlayContainer) {
                            UILoadingOverlay(
                                LoadingOverlayContainer->Rect,
                                SelectedTabLoadingT,
                                SelectedTabViewState->LoadingProgressV,
                                SelectedTabViewState->LoadingProgressVTarget
                            );
                        }
                    }
                }

                if (BuildPanel) {
                    UISignal PanelSig = UISignalFromBox(PanelBox);

                    if (UI_PRESSED(PanelSig))
                        AppCmd(APP_COMMAND_KIND_FOCUS_PANEL, __Registers.Panel = Panel->Config->ID);
                }

                struct TabTask {
                    TabTask*     Next;
                    ConfigNode*  Tab;
                    FancyStrList FancyStrings;
                    f32          TabWidth;
                };

                TabTask* HeadTabTask = NULL;
                TabTask* TailTabTask = NULL;
                u64 TabTaskCount = 0;
                f32 TabCloseWidthPX = UIHeadFontSize() * 2.5f;
                f32 MaxTabWidthPX = UIHeadFontSize() * 20.0f;

                if (BuildPanel) {
                    UITag("tab"_s8) {
                        b32 Reset = (
                            WS->WindowLayoutReset ||
                            WS->FramesAlive < 5 ||
                            IsChangingPanelBoundaries
                        );

                        for (
                            ConfigNodePtrNode* N = Panel->Tabs.Head;
                            N;
                            N = N->Next
                        ) {
                            ConfigNode* Tab = N->V;

                            if (AppIsProjectFiltered(Tab))
                                continue;

                            UITag((Tab != Panel->SelectedTab) ? "inactive"_s8 : ""_s8) {
                                TabTask* T = ArenaPushArrayZero(Scratch.MemPool, TabTask, 1);

                                T->Tab = Tab;
                                T->FancyStrings = AppTitleFStrFromConfig(Scratch.MemPool, Tab, FALSE);

                                f32 TabWidthTarget = DDimensionsFromFancyStrList(UIHeadTabSize(), &T->FancyStrings).X + TabCloseWidthPX + UIHeadFontSize() * 1.0f;

                                TabWidthTarget = MIN(MaxTabWidthPX, TabWidthTarget);

                                f32 TabWidth = UIAnimate(
                                    UIKeyFromStrFmt({}, "tab_width_%p", Tab),
                                    TabWidthTarget,
                                    __Params.Initial = (Reset) ? TabWidthTarget : 0,
                                    __Params.Rate = APP_STATE->MenuAnimationRate
                                );

                                T->TabWidth = FloorF32(TabWidth);
                                SLL_QUEUE_PUSH(HeadTabTask, TailTabTask, T);
                                ++TabTaskCount;
                            }
                        }
                    }
                }

                UIBox* TabBarBox = EMPTY_UI_BOX_VALUE;

                if (BuildPanel) {
                    UICornerRadius(0) {
                        UIRect(TabBarRect) {
                            TabBarBox = UIBuildBoxFromStrFmt(
                                (
                                    UI_BOX_KIND_CLIP |
                                    UI_BOX_KIND_ALLOW_OVERFLOW_Y |
                                    UI_BOX_KIND_VIEW_CLAMP_X |
                                    UI_BOX_KIND_VIEW_SCROLL_X |
                                    UI_BOX_KIND_CLICKABLE
                                ),
                                "tab_bar_%p",
                                Panel->Config
                            );

                            if (Panel->TabSide == SIDE_MAX)
                                TabBarBox->ViewOffset.Y = TabBarBox->ViewOffsetTarget.Y = (TabBarRHeight - TabBarVHeight);
                            else
                                TabBarBox->ViewOffset.Y = TabBarBox->ViewOffsetTarget.Y = 0.0f;
                        }
                    }
                }

                b32 TabDropIsActive = AppDragIsActive() && (UIDropHotKey() == CatchAllDropSiteKey);
                ConfigNode* TabDropPrev = EMPTY_CFG_NODE_VALUE;

                if (BuildPanel) {
                    f32 BestPrevDistancePX = (f32) MILLION(1.0f);
                    TabTask StartBoundaryTabTask = {
                        HeadTabTask,
                        EMPTY_CFG_NODE_VALUE
                    };
                    f32 Offset = 0.0f;

                    for (TabTask* Task = &StartBoundaryTabTask; Task; Task = Task->Next) {
                        Offset += Task->TabWidth;

                        v2f32 AnchorPoint = Vec(TabBarBox->Rect.X0 + Offset, TabBarBox->Rect.Y1);
                        f32 Distance = Length(UIMouse() - AnchorPoint);

                        if (Distance < BestPrevDistancePX) {
                            BestPrevDistancePX = Distance;
                            TabDropPrev = Task->Tab;
                        }
                    }
                }

                if (TabDropIsActive && APP_STATE->DragDropRegisters->Panel == Panel->Config->ID) {
                    TabTask StartBoundaryTabTask = {
                        HeadTabTask,
                        EMPTY_CFG_NODE_VALUE
                    };

                    if (TabDropPrev->ID == APP_STATE->DragDropRegisters->View)
                        TabDropIsActive = FALSE;

                    if (TabDropIsActive) {
                        for (TabTask* T = &StartBoundaryTabTask; T; T = T->Next) {
                            if (T->Tab == TabDropPrev && T->Next && T->Next->Tab->ID == APP_STATE->DragDropRegisters->View) {
                                TabDropIsActive = FALSE;
                                break;
                            }
                        }
                    }
                }

                if (BuildPanel) {
                    UIFocus(UI_FOCUS_KIND_OFF) {
                        UIParent(TabBarBox) {
                            UISetNextChildLayoutAxis(AXIS_2D_X);
                            UIPadding(UI_EM(0.5f, 1.0f)) {
                                UIPreferredHeight(UI_PERCENT(1, 0)) {
                                    UITag("tab"_s8) {
                                        f32 CornerRadius = UIHeadFontSize() * 0.6f;
                                        TabTask StartBoundaryTabTask = {
                                            HeadTabTask,
                                            EMPTY_CFG_NODE_VALUE
                                        };

                                        UICornerRadius00(Panel->TabSide == SIDE_MIN ? CornerRadius : 0) {
                                            UICornerRadius01(Panel->TabSide == SIDE_MIN ? 0 : CornerRadius) {
                                                UICornerRadius10(Panel->TabSide == SIDE_MIN ? CornerRadius : 0) {
                                                    UICornerRadius11(Panel->TabSide == SIDE_MIN ? 0 : CornerRadius) {
                                                        for (TabTask* TTask = &StartBoundaryTabTask; TTask; TTask = TTask->Next) {
                                                            ConfigNode* Tab = TTask->Tab;
                                                            FancyStrList String = TTask->FancyStrings;
                                                            f32 TabWidthPX = TTask->TabWidth;

                                                            if (Tab != EMPTY_CFG_NODE_VALUE) {
                                                                AppRegistersScope(
                                                                    __Registers.Panel = Panel->Config->ID,
                                                                    __Registers.View = Tab->ID,
                                                                    __Registers.Tab = Tab->ID
                                                                ) {
                                                                    b32 TabIsSelected = (Tab == Panel->SelectedTab);
                                                                    b32 TabIsAuto = AppViewSettingFromNameb32("auto"_s8);

                                                                    UISetNextChildLayoutAxis(AXIS_2D_Y);
                                                                    UISetNextPreferredWidth(UI_PX(TabWidthPX, 1));

                                                                    UIBox* TabColumnBox = UIBuildBoxFromStrFmt(
                                                                        (
                                                                            !IsChangingPanelBoundaries * UI_BOX_KIND_ANIMATE_POS_X
                                                                        ),
                                                                        "tab_column_%p",
                                                                        Tab
                                                                    );
                                                                    b32 OmitName = FALSE;

                                                                    if (
                                                                        AppDragIsActive() &&
                                                                        APP_STATE->DragDropRegisters->View == Tab->ID &&
                                                                        APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW
                                                                    ) {
                                                                        OmitName = TRUE;
                                                                    }

                                                                    UIParent(TabColumnBox) {
                                                                        UIPreferredHeight(UI_PX(TabBarVHeight, 1)) {
                                                                            UITag((OmitName) ? "hollow"_s8 : ""_s8) {
                                                                                UITag((!OmitName && !TabIsSelected) ? "inactive"_s8 : ""_s8) {
                                                                                    UITag((!OmitName && TabIsAuto) ? "auto"_s8 : ""_s8) {
                                                                                        if (Panel->TabSide == SIDE_MAX)
                                                                                            UISpacer(UI_PX(TabBarRVDiff - 1.0f, 1.0f));
                                                                                        else
                                                                                            UISpacer(UI_PX(1.0f, 1.0f));

                                                                                        UIBox* TabBox = UIBuildBoxFromStrFmt(
                                                                                            (
                                                                                                UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                                                                UI_BOX_KIND_DRAW_BACKGROUND |
                                                                                                UI_BOX_KIND_DRAW_BORDER |
                                                                                                (TabIsSelected * UI_BOX_KIND_DRAW_DROP_SHADOW) |
                                                                                                UI_BOX_KIND_CLICKABLE
                                                                                            ),
                                                                                            "tab_%p",
                                                                                            Tab
                                                                                        );

                                                                                        if (!OmitName) {
                                                                                            UIParent(TabBox) {
                                                                                                UIWidthFill() {
                                                                                                    UIRow() {
                                                                                                        UISpacer(UI_EM(0.5f, 1.0f));

                                                                                                        UIPreferredWidth(UI_TEXT_DIM(10, 0)) {
                                                                                                            UIBox* NameBox = UIBuildBoxFromKey(UI_BOX_KIND_DRAW_TEXT, {});

                                                                                                            UIBoxEquipDisplayFancyStrs(NameBox, &String);
                                                                                                        }
                                                                                                    }
                                                                                                }

                                                                                                if (
                                                                                                    TabIsSelected &&
                                                                                                    PanelTree.Focused == Panel
                                                                                                ) {
                                                                                                    UIPreferredWidth(UI_PX(TabCloseWidthPX, 1.0f)) {
                                                                                                        UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                                                                            AppFont(APP_FONT_SLOT_ICONS) {
                                                                                                                UIFontSize(UIHeadFontSize() * 0.75f) {
                                                                                                                    UITag("."_s8) {
                                                                                                                        UITag("weak"_s8) {
                                                                                                                            UITag("implicit"_s8) {
                                                                                                                                UICornerRadius(0) {
                                                                                                                                    UIBox* EditBox = UIBuildBoxFromStrFmt(
                                                                                                                                        (
                                                                                                                                            UI_BOX_KIND_CLICKABLE |
                                                                                                                                            UI_BOX_KIND_DRAW_BORDER |
                                                                                                                                            UI_BOX_KIND_DRAW_BACKGROUND |
                                                                                                                                            UI_BOX_KIND_DRAW_TEXT |
                                                                                                                                            UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                                                                                                            UI_BOX_KIND_DRAW_ACTIVE_EFFECTS
                                                                                                                                        ),
                                                                                                                                        "%S###edit_view_%p",
                                                                                                                                        PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_GEAR]),
                                                                                                                                        Tab
                                                                                                                                    );

                                                                                                                                    UISignal Sig = UISignalFromBox(EditBox);

                                                                                                                                    if (UI_PRESSED(Sig)) {
                                                                                                                                        if (WS->QueryIsActive && Sig.Box->Key == WS->QueryRegs->UI) {
                                                                                                                                            AppCmd(APP_COMMAND_KIND_CANCEL_QUERY);
                                                                                                                                        } else {
                                                                                                                                            AppCmd(
                                                                                                                                                APP_COMMAND_KIND_PUSH_QUERY,
                                                                                                                                                __Registers.UI = Sig.Box->Key,
                                                                                                                                                __Registers.Expression = ArenaPushStrFmt(
                                                                                                                                                    Scratch.MemPool,
                                                                                                                                                    "query:config.$%llx",
                                                                                                                                                    Tab->ID
                                                                                                                                                )
                                                                                                                                            );
                                                                                                                                        }
                                                                                                                                    }
                                                                                                                                }
                                                                                                                            }
                                                                                                                        }
                                                                                                                    }
                                                                                                                }
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }

                                                                                                UIPreferredWidth(UI_PX(TabCloseWidthPX, 1.0f)) {
                                                                                                    UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                                                                                        AppFont(APP_FONT_SLOT_ICONS) {
                                                                                                            UIFontSize(UIHeadFontSize() * 0.75f) {
                                                                                                                UITag("."_s8) {
                                                                                                                    UITag("weak"_s8) {
                                                                                                                        UITag("implicit"_s8) {
                                                                                                                            UIBox* CloseBox = UIBuildBoxFromStrFmt(
                                                                                                                                (
                                                                                                                                    UI_BOX_KIND_CLICKABLE |
                                                                                                                                    UI_BOX_KIND_DRAW_BORDER |
                                                                                                                                    UI_BOX_KIND_DRAW_BACKGROUND |
                                                                                                                                    UI_BOX_KIND_DRAW_TEXT |
                                                                                                                                    UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                                                                                                    UI_BOX_KIND_DRAW_ACTIVE_EFFECTS
                                                                                                                                ),
                                                                                                                                "%S###close_view_%p",
                                                                                                                                PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_X]),
                                                                                                                                Tab
                                                                                                                            );

                                                                                                                            UISignal Sig = UISignalFromBox(CloseBox);

                                                                                                                            if (UI_CLICKED(Sig) || UI_MIDDLE_CLICKED(Sig))
                                                                                                                            AppCmd(APP_COMMAND_KIND_CLOSE_TAB);
                                                                                                                        }
                                                                                                                    }
                                                                                                                }
                                                                                                            }
                                                                                                        }
                                                                                                    }
                                                                                                }
                                                                                            }
                                                                                        }

                                                                                        {
                                                                                            UISignal Sig = UISignalFromBox(TabBox);

                                                                                            if (UI_PRESSED(Sig)) {
                                                                                                AppCmd(APP_COMMAND_KIND_FOCUS_TAB);
                                                                                                AppCmd(APP_COMMAND_KIND_FOCUS_PANEL);
                                                                                            } else if (UI_DRAGGING(Sig) && !AppDragIsActive() && Length(UIDragDelta()) > 10.0f) {
                                                                                                AppDragBegin(APP_REGISTERS_SLOT_VIEW);
                                                                                            } else if (UI_RIGHT_CLICKED(Sig)) {
                                                                                                AppCmd(
                                                                                                    APP_COMMAND_KIND_PUSH_QUERY,
                                                                                                    __Registers.UI = Sig.Box->Key,
                                                                                                    __Registers.Expression = ArenaPushStrFmt(Scratch.MemPool,  "query:config.$%llx", Tab->ID)
                                                                                                );
                                                                                            } else if (UI_MIDDLE_CLICKED(Sig)) {
                                                                                                AppCmd(APP_COMMAND_KIND_CLOSE_TAB);
                                                                                            }
                                                                                        }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }

                                                                    UISpacer(UI_PX(FloorF32(UIHeadFontSize() * 0.4f), 1.0f));
                                                                }
                                                            }

                                                            if (
                                                                TabDropIsActive && 
                                                                AppDragIsActive() && 
                                                                APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW &&
                                                                Tab == TabDropPrev
                                                            ) {
                                                                UISetNextChildLayoutAxis(AXIS_2D_Y);
                                                                UISetNextPreferredWidth(UI_PX(UIHeadFontSize() * 4.0f, 1));

                                                                UIBox* TabColumnBox = UIBuildBoxFromStrFmt(
                                                                    !IsChangingPanelBoundaries * UI_BOX_KIND_ANIMATE_POS_X,
                                                                    "tab_column_%p",
                                                                    Tab
                                                                );

                                                                UIParent(TabColumnBox) {
                                                                    UIPreferredHeight(UI_PX(TabBarVHeight, 1)) {
                                                                        UITag("hollow"_s8) {
                                                                            if (Panel->TabSide == SIDE_MAX)
                                                                                UISpacer(UI_PX(TabBarRVDiff - 1.0f, 1.0f));
                                                                            else
                                                                                UISpacer(UI_PX(1.0f, 1.0f));

                                                                            UISetNextGroupKey(CatchAllDropSiteKey);

                                                                            UIBox* TabBox = UIBuildBoxFromKey(
                                                                                (
                                                                                    UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                                                    UI_BOX_KIND_DRAW_BACKGROUND |
                                                                                    UI_BOX_KIND_DRAW_BORDER |
                                                                                    UI_BOX_KIND_CLICKABLE
                                                                                ),
                                                                                EMPTY_UI_KEY_VALUE
                                                                            );
                                                                        }
                                                                    }
                                                                }

                                                                {
                                                                    UISpacer(UI_PX(FloorF32(UIHeadFontSize() * 0.4f), 1.0f));
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }

                                        UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                            UIPreferredWidth(UI_PX(TabBarVHeight, 1.0f)) {
                                                UIPreferredHeight(UI_PX(TabBarVHeight, 1.0f)) {
                                                    UITag("."_s8) {
                                                        UISetNextChildLayoutAxis(AXIS_2D_Y);

                                                        UIBox* Container = UIBuildBoxFromStr(
                                                            !IsChangingPanelBoundaries * UI_BOX_KIND_ANIMATE_POS_X,
                                                            "###add_new_tab"_s8
                                                        );

                                                        UIParent(Container) {
                                                            if (Panel->TabSide == SIDE_MAX)
                                                                UISpacer(UI_PX(TabBarRVDiff - 1.0f, 1.0f));
                                                            else
                                                                UISpacer(UI_PX(1.0f, 1.0f));

                                                            AppFont(APP_FONT_SLOT_ICONS) {
                                                                UIFontSize(UIHeadFontSize()) {
                                                                    UITag("implicit"_s8) {
                                                                        UITag("weak"_s8) {
                                                                            UIBox* AddNewBox = UIBuildBoxFromStrFmt(
                                                                                (
                                                                                    UI_BOX_KIND_DRAW_TEXT |
                                                                                    UI_BOX_KIND_DRAW_BORDER |
                                                                                    UI_BOX_KIND_DRAW_BACKGROUND |
                                                                                    UI_BOX_KIND_DRAW_HOT_EFFECTS |
                                                                                    UI_BOX_KIND_DRAW_ACTIVE_EFFECTS |
                                                                                    UI_BOX_KIND_CLICKABLE |
                                                                                    UI_BOX_KIND_DISABLE_TEXT_TRUNC
                                                                                ),
                                                                                "%S##add_new_tab_button_%p",
                                                                                PRINT_STR(APP_ICON_KIND_TEXT_TABLE[APP_ICON_KIND_ADD]),
                                                                                Panel->Config
                                                                            );
                                                                            UISignal Sig = UISignalFromBox(AddNewBox);

                                                                            if (UI_PRESSED(Sig)) {
                                                                                AppCmd(APP_COMMAND_KIND_FOCUS_PANEL, __Registers.Panel = Panel->Config->ID);

                                                                                if (WS->QueryIsActive && (AddNewBox->Key == WS->QueryRegs->UI)) {
                                                                                    AppCmd(APP_COMMAND_KIND_CANCEL_QUERY);
                                                                                } else {
                                                                                    AppCmd(
                                                                                        APP_COMMAND_KIND_PUSH_QUERY,
                                                                                        __Registers.Expression = "query:tab_commands"_s8,
                                                                                        __Registers.Panel = Panel->Config->ID,
                                                                                        __Registers.DoImplicitRoot = TRUE,
                                                                                        __Registers.DoSmall = TRUE,
                                                                                        __Registers.DoLister = TRUE,
                                                                                        __Registers.ActivateWithSingleClick = TRUE,
                                                                                        __Registers.UI = AddNewBox->Key
                                                                                    );
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    UISignalFromBox(TabBarBox);
                }

                if (TabDropIsActive && AppDragDrop() && APP_STATE->DragDropRegistersSlot == APP_REGISTERS_SLOT_VIEW) {
                    AppCmd(
                        APP_COMMAND_KIND_MOVE_VIEW,
                        __Registers.DstPanel = Panel->Config->ID,
                        __Registers.Panel = APP_STATE->DragDropRegisters->Panel,
                        __Registers.View = APP_STATE->DragDropRegisters->View,
                        __Registers.PrevTab = TabDropPrev->ID
                    );
                }

                {
                    for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
                        if (Evt->Kind == UI_EVENT_KIND_FILE_DROP && InRange(ContentRect, Evt->Position)) {
                            b32 NeedDropCompletion = FALSE;

                            ArenaClear(WS->DropCompletionMemPool);
                            WS->HeadDropCompletionTask = NULL;
                            WS->DropCompletionPanel = Panel->Config->ID;

                            for (Str8Node* N = Evt->Paths.Head; N; N = N->Next) {
                                TempArena Scratch = GetScratch(NULL, 0);
                                Str8 Path = N->String;

                                if (FALSE) {
                                    // TODO(nathan): add specific file types that need
                                    //             : specific actions to be taken when dropped
                                } else {
                                    AppCmd(
                                        APP_COMMAND_KIND_OPEN, 
                                        __Registers.FilePath = Path,
                                        __Registers.Panel = Panel->Config->ID
                                    );
                                }

                                ReleaseScratch(Scratch);
                            }

                            if (NeedDropCompletion)
                                UIContextMenuOpen(APP_STATE->DropCompletionKey, EMPTY_UI_KEY_VALUE, Evt->Position);

                            UIEatEvent(Evt);
                        }
                    }
                }
            }
        }
    }

    // NOTE(nathan): Drag+drop cancelling
    if (AppDragIsActive() && UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)) {
        AppDragKill();
        UIKillAction();
    }

    // NOTE(nathan): Font size changing
    for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
        if (Evt->Kind == UI_EVENT_KIND_SCROLL && Evt->Modifiers == INPUT_MOD_KIND_CTRL) {
            UIEatEvent(Evt);

            if (Evt->DeltaF32.Y < 0.0f)
                AppCmd(APP_COMMAND_KIND_INC_WINDOW_FONT_SIZE);
            else if (Evt->DeltaF32.Y > 0.0f)
                AppCmd(APP_COMMAND_KIND_DEC_WINDOW_FONT_SIZE);
        }
    }

    UIEndBuild();

    if (
        WS->HoverEvalString.Size &&
        UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)
    ) {
        MemZero(&WS->HoverEvalString, sizeof(WS->HoverEvalString));
        ArenaClear(WS->HoverEvalMemPool);
        WS->HoverEvalFocused = FALSE;
        AppRequestFrame();
    }

    // NOTE(nathan): Animate
    if (UIIsAnimatingFromState(WS->UI))
        AppRequestFrame();

    // NOTE(nathan): Draw UI
    WS->DrawBucket = DBucketAlloc();

    DBucketScope(WS->DrawBucket) {
        TempArena Scratch = GetScratch(NULL, 0);
        f32 BoxSquishEpsilon = 0.001f;
        r2f32 WindowRect = GetWindowClientRect(WS->WinHandle);
        f32 RoundedCornerAmount = AppSettingFromNameF32("rounded_corner_amount"_s8);
        f32 BorderSoftness = 1.0f;
        b32 DoBackgroundBlur = AppSettingFromNameB32("background_blur"_s8);
        b32 ForceOpaqueFloatingBackgrounds = AppSettingFromNameB32("opaque_backgrounds"_s8);
        b32 DoDropShadows = AppSettingFromNameB32("drop_shadows"_s8);
        v4f32 BaseBackgroundColour = UIColourFromName("background"_s8);
        v4f32 BaseBorderColour = UIColourFromName("border"_s8);
        v4f32 DropShadowColour = UIColourFromName("drop_shadow"_s8);

        DrawRect(
            GetWindowClientRect(WS->WinHandle),
            BaseBackgroundColour,
            0.0f,
            0.0f,
            0.0f
        );
        DrawRect(
            GetWindowClientRect(WS->WinHandle),
            BaseBorderColour,
            0.0f,
            1.0f,
            0.0f
        );

        UIBox* HoverDebugBox = EMPTY_UI_BOX_VALUE;

        for (UIBox* Box = UIRootFromState(WS->UI); !IsUIBoxEmpty(Box); ) {
            f32 BoxCornerRadii[CORNER_COUNT] = {
                Box->CornerRadii[CORNER_00] * RoundedCornerAmount,
                Box->CornerRadii[CORNER_01] * RoundedCornerAmount,
                Box->CornerRadii[CORNER_10] * RoundedCornerAmount,
                Box->CornerRadii[CORNER_11] * RoundedCornerAmount
            };
            UIBoxRecord Rec = UIBoxRecordDFPost(Box, EMPTY_UI_BOX_VALUE);

            if (Box->Kind & UI_BOX_KIND_DEBUG && InRange(Box->Rect, UIMouse()))
                HoverDebugBox = Box;

            if (Box->Transparency != 0.0f)
                DPushTransparency(Box->Transparency);

            if (Box->Squish > BoxSquishEpsilon) {
                v2f32 BoxLength = Length(Box->Rect);
                v2f32 AnchorOffset = {};

                if (Box->Kind & UI_BOX_KIND_SQUISH_ANCHORED)
                    AnchorOffset.X = BoxLength.X / 2.0f;
                else
                    AnchorOffset.Y = -BoxLength.Y / 8.0f;

                m3f32 Box2OriginXForm = Translation(
                    Vec(
                        -Box->Rect.X0 - BoxLength.X / 2.0f + AnchorOffset.X,
                        -Box->Rect.Y0 + AnchorOffset.Y
                    )
                );
                m3f32 ScaleXForm = Scaler(
                    Vec(1.0f - Box->Squish, 1.0f - Box->Squish)
                );
                m3f32 Origin2BoxXForm = Translation(
                    Vec(
                        Box->Rect.X0 + BoxLength.X / 2.0f - AnchorOffset.X,
                        Box->Rect.Y0 - AnchorOffset.Y
                    )
                );
                m3f32 XForm = Origin2BoxXForm * (ScaleXForm * Box2OriginXForm);

                DPushXForm2D(XForm);
                DPushTex2DSampleKind(TEX2D_SAMPLE_LINEAR);
            }

            if (DoDropShadows && Box->Kind & UI_BOX_KIND_DRAW_DROP_SHADOW) {
                r2f32 DropShadowRect = Shift(Widen(Box->Rect, 8.0f), Vec(4.0f, 4.0f));
                Rect2DInstr* Instruction = DrawRect(
                    DropShadowRect,
                    DropShadowColour,
                    0.8f,
                    0.0f,
                    8.0f
                );

                MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);
            }

            if (DoBackgroundBlur && Box->Kind & UI_BOX_KIND_DRAW_BACKGROUND_BLUR) {
                RendPassBlurParams* Params = DrawBlur(
                    Widen(Box->Rect, 1.0f),
                    Box->BlurSize * (1.0f - Box->Transparency),
                    0.0f
                );

                MemCpyArray(Params->CornerRadii, BoxCornerRadii);
            }

            f32 EffectiveActiveT = Box->Active;

            if (!(Box->Kind & UI_BOX_KIND_DRAW_ACTIVE_EFFECTS))
                EffectiveActiveT = 0.0f;

            f32 T = Box->Hot * (1.0f - EffectiveActiveT);
            v4f32 BoxBackgroundColour = Box->BackgroundColour;

            if (
                ForceOpaqueFloatingBackgrounds &&
                Box->Kind & UI_BOX_KIND_FLOATING &&
                Box->Kind & UI_BOX_KIND_DRAW_DROP_SHADOW
            ) {
                BoxBackgroundColour.W = 1.0f;
            }

            if (Box->Kind & UI_BOX_KIND_DRAW_BACKGROUND) {
                if (Box->Kind & UI_BOX_KIND_DRAW_HOT_EFFECTS) {
                    r2f32 DropShadowRect = Shift(Widen(Box->Rect, 8.0f), Vec(4.0f, 4.0f));
                    v4f32 Colour = DropShadowColour;

                    Colour.W *= T * BoxBackgroundColour.W;
                    DrawRect(
                        DropShadowRect,
                        Colour,
                        0.8f,
                        0.0f,
                        8.0f
                    );
                }

                Rect2DInstr* Instruction = DrawRect(
                    Widen(Box->Rect, 1.0f),
                    BoxBackgroundColour,
                    0.0f,
                    0.0f,
                    BorderSoftness * 1.0f
                );

                MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);

                if (Box->Kind & UI_BOX_KIND_DRAW_HOT_EFFECTS) {
                    b32 IsHot = (
                        Box->Key != EMPTY_UI_KEY_VALUE && 
                        Box->Key == UIHotKey()
                    );
                    v4f32 HoverColour = UIColourFromTagsKeyName(
                        Box->TagsKey, 
                        "hover"_s8
                    );

                    {
                        v4f32 Colour = HoverColour;

                        Colour.W *= 0.05f;

                        if (!IsHot)
                            Colour.W *= T;

                        Rect2DInstr* Instruction = DrawRect(
                            Widen(Box->Rect, 1.0f),
                            Vec(0.0f, 0.0f, 0.0f, 0.0f),
                            0.0f,
                            0.0f,
                            BorderSoftness * 1.0f
                        );

                        Instruction->Colours[CORNER_00] = Colour;
                        Instruction->Colours[CORNER_10] = Colour;
                        MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);
                    }

                    if (Box->Hot > 0.01f) {
                        DClipScope(Intersect(Box->Rect, DTopClip())) {
                            v4f32 Colour = HoverColour;

                            Colour.W *= 0.025f;

                            if (!IsHot)
                                Colour.W *= T;

                            v2f32 MouseCentre = UIMouse();
                            v2f32 BoxLength = Length(Box->Rect);
                            f32 MaxLength = MAX(BoxLength.X, BoxLength.Y);
                            f32 Radius = Box->FontSize * 12.0f;

                            // FIXME(nathan): the radius drawn goes over the 
                            //              : corner radius, shown as square
                            Radius = MIN(MaxLength, Radius);
                            DrawRect(
                                Widen(Rng(MouseCentre, MouseCentre), Radius),
                                Colour,
                                Radius,
                                0.0f,
                                Radius / 3.0f
                            );
                        }
                    }

                    if (Box->Kind & UI_BOX_KIND_DRAW_ACTIVE_EFFECTS) {
                        v4f32 ShadowColour = DropShadowColour;

                        ShadowColour.W *= 0.5f * Box->Active;

                        v2f32 ShadowSize = {
                            (Box->Rect.X1 - Box->Rect.X0) * 0.60f * Box->Active,
                            (Box->Rect.Y1 - Box->Rect.Y0) * 0.60f * Box->Active
                        };

                        ShadowSize.X = CLAMP(0.0f, ShadowSize.X, Box->FontSize * 2.0f);
                        ShadowSize.Y = CLAMP(0.0f, ShadowSize.Y, Box->FontSize * 2.0f);

                        {
                            Rect2DInstr* Instruction = DrawRect(
                                {
                                    Box->Rect.X0, 
                                    Box->Rect.Y0, 
                                    Box->Rect.X1, 
                                    Box->Rect.Y0 + ShadowSize.Y
                                },
                                Vec(0.0f, 0.0f, 0.0f, 0.0f),
                                0.0f,
                                0.0f,
                                1.0f
                            );

                            Instruction->Colours[CORNER_00] = ShadowColour;
                            Instruction->Colours[CORNER_10] = ShadowColour;
                            Instruction->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instruction->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);
                        }

                        {
                            Rect2DInstr* Instruction = DrawRect(
                                {
                                    Box->Rect.X0, 
                                    Box->Rect.Y1 - ShadowSize.Y, 
                                    Box->Rect.X1, 
                                    Box->Rect.Y1
                                },
                                Vec(0.0f, 0.0f, 0.0f, 0.0f),
                                0.0f,
                                0.0f,
                                1.0f
                            );

                            Instruction->Colours[CORNER_00] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instruction->Colours[CORNER_10] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instruction->Colours[CORNER_01] = Vec(1.0f, 1.0f, 1.0f, 0.8f * Box->Active);
                            Instruction->Colours[CORNER_11] = Vec(1.0f, 1.0f, 1.0f, 0.8f * Box->Active);
                            MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);
                        }

                        {
                            Rect2DInstr* Instruction = DrawRect(
                                {
                                    Box->Rect.X0, 
                                    Box->Rect.Y0, 
                                    Box->Rect.X0 + ShadowSize.X, 
                                    Box->Rect.Y1
                                },
                                Vec(0.0f, 0.0f, 0.0f, 0.0f),
                                0.0f,
                                0.0f,
                                1.0f
                            );

                            Instruction->Colours[CORNER_00] = ShadowColour;
                            Instruction->Colours[CORNER_10] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instruction->Colours[CORNER_01] = ShadowColour;
                            Instruction->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);
                        }

                        {
                            Rect2DInstr* Instruction = DrawRect(
                                {
                                    Box->Rect.X1 - ShadowSize.X, 
                                    Box->Rect.Y0, 
                                    Box->Rect.X1, 
                                    Box->Rect.Y1
                                },
                                Vec(0.0f, 0.0f, 0.0f, 0.0f),
                                0.0f,
                                0.0f,
                                1.0f
                            );

                            Instruction->Colours[CORNER_00] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instruction->Colours[CORNER_10] = ShadowColour;
                            Instruction->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instruction->Colours[CORNER_11] = ShadowColour;
                            MemCpyArray(Instruction->CornerRadii, BoxCornerRadii);
                        }
                    }
                }
            }

            if (Box->Kind & UI_BOX_KIND_DRAW_TEXT) {
                v2f32 TextPosition = UIBoxTextPosition(Box);
                f32 MaxX = THOUSAND(100.0f);
                FontRun EllipsesRun = {};

                if (!(Box->Kind & UI_BOX_KIND_DISABLE_TEXT_TRUNC)) {
                    FontTag EllipsesFont = Box->Font;
                    f32 EllipsesSize = Box->FontSize;
                    FontRasterKind EllipsesRasterKind = Box->TextRasterKind;

                    if (Box->DisplayFancyStrings.Tail) {
                        EllipsesFont = Box->DisplayFancyStrings.Tail->V.Params.Font;
                        EllipsesSize = Box->DisplayFancyStrings.Tail->V.Params.Size;
                        EllipsesRasterKind = Box->DisplayFancyStrings.Tail->V.Params.RasterKind;
                    }

                    MaxX = (Box->Rect.X1 - TextPosition.X);
                    EllipsesRun = FontRunFromStr(
                        EllipsesFont,
                        EllipsesSize,
                        0.0f,
                        Box->TabSize,
                        EllipsesRasterKind,
                        "..."_s8
                    );
                }

                if (Box->Kind & UI_BOX_KIND_HAS_FUZZYMATCH_RANGES) {
                    UITag("match"_s8) {
                        v4f32 MatchColour = UIColourFromTagsKeyName(
                            UIHeadTagsKey(),
                            "background"_s8
                        );

                        DrawTruncatedFancyRunFuzzyMatches(
                            TextPosition,
                            &Box->DisplayFancyRuns,
                            MaxX,
                            &Box->FuzzyMatchRanges,
                            MatchColour
                        );
                    }
                }

                DrawTruncatedFancyRunList(
                    TextPosition,
                    &Box->DisplayFancyRuns,
                    MaxX,
                    EllipsesRun
                );
            }

            if (Box->Kind & UI_BOX_KIND_CLIP) {
                r2f32 TopClip = DTopClip();
                r2f32 NewClip = Widen(Box->Rect, -1.0f);

                if (TopClip.X1 != 0.0f || TopClip.Y1 != 0.0f)
                    NewClip = Intersect(NewClip, TopClip);

                DPushClip(NewClip);
            }

            if (Box->Kind & UI_BOX_KIND_DRAW_BUCKET) {
                m3f32 XForm = Translation(Box->PositionDelta);

                DXForm2DScope(XForm) {
                    DrawSubBucket(Box->DrawBucket);
                }
            }

            if (Box->CustomDrawFunc)
                Box->CustomDrawFunc(Box, Box->CustomDrawUserData);

            {
                i32 PopIndex = 0;

                for (
                    UIBox* B = Box; 
                    !IsUIBoxEmpty(Box) && PopIndex <= Rec.PopCount;
                    B = B->Parent
                ) {
                    ++PopIndex;

                    if (B == Box && Rec.PushCount)
                        continue;

                    if (B->Kind & UI_BOX_KIND_CLIP)
                        DPopClip();

                    f32 BCornerRadii[CORNER_COUNT] = {
                        B->CornerRadii[CORNER_00] * RoundedCornerAmount,
                        B->CornerRadii[CORNER_01] * RoundedCornerAmount,
                        B->CornerRadii[CORNER_10] * RoundedCornerAmount,
                        B->CornerRadii[CORNER_11] * RoundedCornerAmount
                    };

                    if (B->Kind & UI_BOX_KIND_DRAW_BORDER) {
                        v4f32 BorderColour = B->BorderColour;
                        r2f32 BBorderRect = Widen(B->Rect, 1.0f);
                        Rect2DInstr* Instruction = DrawRect(
                            BBorderRect,
                            BorderColour,
                            0.0f,
                            1.0f,
                            BorderSoftness * 1.0f
                        );

                        MemCpyArray(Instruction->CornerRadii, BCornerRadii);

                        if (B->Kind & UI_BOX_KIND_DRAW_HOT_EFFECTS) {
                            v4f32 Colour = UIColourFromTagsKeyName(
                                Box->TagsKey,
                                "hover"_s8
                            );

                            if (
                                B->Key == EMPTY_UI_KEY_VALUE || 
                                B->Key != UIHotKey()
                            ) {
                                Colour.W *= B->Hot;
                            }

                            Colour.W *= 0.01f;

                            Rect2DInstr* Instruction = DrawRect(
                                BBorderRect,
                                Colour,
                                0.0f,
                                1.0f,
                                1.0f
                            );

                            MemCpyArray(Instruction->CornerRadii, BCornerRadii);
                        }
                    }

                    if (
                        B->Kind & (
                            UI_BOX_KIND_DRAW_FADE_TOP |
                            UI_BOX_KIND_DRAW_FADE_BOTTOM |
                            UI_BOX_KIND_DRAW_FADE_LEFT |
                            UI_BOX_KIND_DRAW_FADE_RIGHT
                        )
                    ) {
                        v2f32 FadeLength = Length(B->Rect) * 0.05f;
                        v4f32 FadeColour = DropShadowColour;

                        if (B->Kind * UI_BOX_KIND_DRAW_FADE_TOP) {
                            f32 T = UIAnimate(
                                UIKeyFromStr(B->Key, "fade_top"_s8),
                                1.0f,
                                __Params.Rate = APP_STATE->CatchAllAnimationRate
                            );
                            r2f32 Rect = Rng(
                                B->Rect.X0,
                                B->Rect.Y0,
                                B->Rect.X1,
                                B->Rect.Y0 + FadeLength.Y * T
                            );
                            Rect2DInstr* Instr = DrawRect(
                                Rect,
                                FadeColour,
                                0.0f,
                                0.0f,
                                0.0f
                            );

                            MemCpyArray(Instr->CornerRadii, BCornerRadii);
                            Instr->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instr->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                        }

                        if (B->Kind * UI_BOX_KIND_DRAW_FADE_BOTTOM) {
                            f32 T = UIAnimate(
                                UIKeyFromStr(B->Key, "fade_bottom"_s8),
                                1.0f,
                                __Params.Rate = APP_STATE->CatchAllAnimationRate
                            );
                            r2f32 Rect = Rng(
                                B->Rect.X0,
                                B->Rect.Y1 - FadeLength.Y * T,
                                B->Rect.X1,
                                B->Rect.Y1
                            );
                            Rect2DInstr* Instr = DrawRect(
                                Rect,
                                FadeColour,
                                0.0f,
                                0.0f,
                                0.0f
                            );

                            MemCpyArray(Instr->CornerRadii, BCornerRadii);
                            Instr->Colours[CORNER_00] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instr->Colours[CORNER_10] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                        }

                        if (B->Kind * UI_BOX_KIND_DRAW_FADE_LEFT) {
                            f32 T = UIAnimate(
                                UIKeyFromStr(B->Key, "fade_left"_s8),
                                1.0f,
                                __Params.Rate = APP_STATE->CatchAllAnimationRate
                            );
                            r2f32 Rect = Rng(
                                B->Rect.X0,
                                B->Rect.Y0,
                                B->Rect.X1 + FadeLength.X * T,
                                B->Rect.Y1
                            );
                            Rect2DInstr* Instr = DrawRect(
                                Rect,
                                FadeColour,
                                0.0f,
                                0.0f,
                                0.0f
                            );

                            MemCpyArray(Instr->CornerRadii, BCornerRadii);
                            Instr->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instr->Colours[CORNER_10] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                        }

                        if (B->Kind * UI_BOX_KIND_DRAW_FADE_RIGHT) {
                            f32 T = UIAnimate(
                                UIKeyFromStr(B->Key, "fade_right"_s8),
                                1.0f,
                                __Params.Rate = APP_STATE->CatchAllAnimationRate
                            );
                            r2f32 Rect = Rng(
                                B->Rect.X1 - FadeLength.X * T,
                                B->Rect.Y0,
                                B->Rect.X1,
                                B->Rect.Y1
                            );
                            Rect2DInstr* Instr = DrawRect(
                                Rect,
                                FadeColour,
                                0.0f,
                                0.0f,
                                0.0f
                            );

                            MemCpyArray(Instr->CornerRadii, BCornerRadii);
                            Instr->Colours[CORNER_00] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                            Instr->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 0.0f);
                        }
                    }

                    if (B->Kind & UI_BOX_KIND_DEBUG) {
                        Rect2DInstr* Instruction = DrawRect(
                            B->Rect,
                            Vec(
                                1.0f * Box->PreferredSize[AXIS_2D_X].Strictness,
                                0.0f,
                                1.0f,
                                0.25f
                            ),
                            0.0f,
                            1.0f,
                            0.0f
                        );

                        MemCpyArray(Instruction->CornerRadii, BCornerRadii);
                    }

                    if (
                        B->Kind & (
                            UI_BOX_KIND_DRAW_SIDE_TOP |
                            UI_BOX_KIND_DRAW_SIDE_BOTTOM |
                            UI_BOX_KIND_DRAW_SIDE_LEFT |
                            UI_BOX_KIND_DRAW_SIDE_RIGHT
                        )
                    ) {
                        v4f32 BorderColour = B->BorderColour;
                        r2f32 R = B->Rect;
                        f32 HalfThickness = 1.0f;
                        f32 Softness = 0.0f;

                        if (B->Kind & UI_BOX_KIND_DRAW_SIDE_TOP) {
                            DrawRect(
                                Rng(
                                    Vec(R.X0, R.Y0), 
                                    Vec(R.X1, R.Y0 + 2 * HalfThickness)
                                ),
                                BorderColour,
                                0.0f,
                                0.0f,
                                Softness
                            );
                        }

                        if (B->Kind & UI_BOX_KIND_DRAW_SIDE_BOTTOM) {
                            DrawRect(
                                Rng(
                                    Vec(R.X0, R.Y1 - 2 * HalfThickness), 
                                    Vec(R.X1, R.Y1)
                                ),
                                BorderColour,
                                0.0f,
                                0.0f,
                                Softness
                            );
                        }

                        if (B->Kind & UI_BOX_KIND_DRAW_SIDE_LEFT) {
                            DrawRect(
                                Rng(
                                    Vec(R.X0, R.Y0), 
                                    Vec(R.X0 + 2 * HalfThickness, R.Y1)
                                ),
                                BorderColour,
                                0.0f,
                                0.0f,
                                Softness
                            );
                        }

                        if (B->Kind & UI_BOX_KIND_DRAW_SIDE_RIGHT) {
                            DrawRect(
                                Rng(
                                    Vec(R.X1 - 2 * HalfThickness, R.Y0), 
                                    Vec(R.X1, R.Y1)
                                ),
                                BorderColour,
                                0.0f,
                                0.0f,
                                Softness
                            );
                        }
                    }

                    if (
                        B->Kind & UI_BOX_KIND_CLICKABLE && 
                        !(B->Kind & UI_BOX_KIND_DISABLE_FOCUS_OVERLAY) &&
                        B->FocusHot > 0.01f
                    ) {
                        Str8 Extras[] = {
                            "focus"_s8,
                            "overlay"_s8
                        };
                        Str8Array ExtrasArray = {
                            Extras,
                            ARRAY_COUNT(Extras)
                        };
                        v4f32 Colour = UIColourFromTagsKeyExtras(
                            B->TagsKey,
                            ExtrasArray
                        );

                        Colour.W *= B->FocusHot;

                        Rect2DInstr* Instruction = DrawRect(
                            B->Rect,
                            Colour,
                            0.0f,
                            0.0f,
                            0.0f
                        );

                        MemCpyArray(Instruction->CornerRadii, BCornerRadii);
                    }

                    if (
                        B->Kind & UI_BOX_KIND_CLICKABLE && 
                        !(B->Kind & UI_BOX_KIND_DISABLE_FOCUS_BORDER) &&
                        B->FocusActive > 0.01f
                    ) {
                        r2f32 Rect = B->Rect;

                        if (B->Kind & UI_BOX_KIND_FLOATING) {
                            Rect = Widen(Rect, 1.0f);
                            Rect = Intersect(WindowRect, Rect);
                        }

                        Str8 Extras[] = {
                            "focus"_s8,
                            "border"_s8
                        };
                        Str8Array ExtrasArray = {
                            Extras,
                            ARRAY_COUNT(Extras)
                        };
                        v4f32 Colour = UIColourFromTagsKeyExtras(
                            B->TagsKey,
                            ExtrasArray
                        );

                        Colour.W *= B->FocusActive;

                        Rect2DInstr* Instruction = DrawRect(
                            B->Rect,
                            Colour,
                            0.0f,
                            1.0f,
                            BorderSoftness * 1.0f
                        );

                        MemCpyArray(Instruction->CornerRadii, BCornerRadii);
                    }

                    if (B->Disabled >= 0.005f) {
                        v4f32 DisabledOverlayColour = Vec(
                            BaseBackgroundColour.R,
                            BaseBackgroundColour.G,
                            BaseBackgroundColour.B,
                            B->Disabled * 0.3f
                        );
                        Rect2DInstr* Instruction = DrawRect(
                            B->Rect,
                            DisabledOverlayColour,
                            0.0f,
                            0.0f,
                            1.0f
                        );

                        MemCpyArray(Instruction->CornerRadii, BCornerRadii);
                    }

                    if (B->Squish > BoxSquishEpsilon) {
                        DPopXForm2D();
                        DPopTex2DSampleKind();
                    }

                    if (B->Transparency != 0.0f)
                        DPopTransparency();
                }
            }

            Box = Rec.Next;
        }

        if (HoverDebugBox != EMPTY_UI_BOX_VALUE) {
            FontTag Font = AppFontFromSlot(APP_FONT_SLOT_CODE);
            v2f32 P = UIMouse();

            DrawRect(
                HoverDebugBox->Rect,
                Vec(1.0f, 1.0f, 1.0f, 0.2f),
                0.0f,
                0.0f,
                0.0f
            );
            NC_DrawText(
                Font,
                12.0f,
                0.0f,
                0.0f,
                FONT_RASTER_KIND_HINTED,
                P,
                Vec(1.0f, 1.0f, 1.0f, 1.0f),
                ArenaPushStrFmt(Scratch.MemPool, "key: 0x%llx", *HoverDebugBox->Key.V)
            );

            P.Y += 20.0f;

            NC_DrawText(
                Font,
                12.0f,
                0.0f,
                0.0f,
                FONT_RASTER_KIND_HINTED,
                P,
                Vec(1.0f, 1.0f, 1.0f, 1.0f),
                ArenaPushStrFmt(Scratch.MemPool, "string: '%S'", PRINT_STR(HoverDebugBox->String))
            );

            P.Y += 20.0f;
        }

        if (WS->ErrorT > 0.01f) {
            UITag("bad"_s8) {
                v4f32 Colour = UIColourFromName("text"_s8);

                Colour.W *= WS->ErrorT;

                r2f32 Rect = GetWindowClientRect(WS->WinHandle);

                DrawRect(
                    Widen(Rect, 24.0f),
                    Colour,
                    0.0f,
                    16.0f,
                    12.0f
                );
                DrawRect(
                    Rect,
                    Vec(Colour.R, Colour.G, Colour.B, Colour.A * 0.025f),
                    0.0f,
                    0.0f,
                    0.0f
                );
            }

            if (APP_STATE->BindChangeActive) {
                UITag("pop"_s8) {
                    v4f32 Colour = UIColourFromName("background"_s8);
                    r2f32 Rect = GetWindowClientRect(WS->WinHandle);

                    DrawRect(
                        Widen(Rect, 24.0f),
                        Colour,
                        0.0f,
                        16.0f,
                        12.0f
                    );
                    DrawRect(
                        Rect,
                        Vec(Colour.R, Colour.G, Colour.B, Colour.A * 0.025f),
                        0.0f,
                        0.0f,
                        0.0f
                    );
                }
            }
        }

        ReleaseScratch(Scratch);
    }

    // NOTE(nathan): Update per-window frame counters/info
    ++WS->FramesAlive;
    WS->LastWindowRect = GetWindowClientRect(WS->WinHandle);
    ReleaseScratch(Scratch);
}

NC_NO_OPTIMISE_END //__________________________________________________________

AppRegistersSlot 
AppRegistersFillSlotFromCtxEval(EvalEval Eval)
{
    AppRegistersSlot Result = APP_REGISTERS_SLOT_NULL;

    {
        ConfigNode* Cfg = AppConfigFromEvalSpace(Eval.Space);

        if (Cfg != EMPTY_CFG_NODE_VALUE) {
            Registers()->Config = Cfg->ID;
            Result = APP_REGISTERS_SLOT_CONFIG;

            TempArena Scratch = GetScratch(NULL, 0);
            ConfigPanelTree Panels = ConfigPanelTreeFromConfig(Scratch.MemPool, Cfg);
            ConfigPanelNode* ParentPanelNode = ConfigPanelNodeFromConfigTree(
                Panels.Root,
                Cfg->Parent
            );

            if (ParentPanelNode != EMPTY_CFG_PANEL_NODE_VALUE) {
                Registers()->Tab = Cfg->ID;
                Registers()->View = Cfg->ID;
            }

            ReleaseScratch(Scratch);
        }
    }

    return Result;
}

AppRegistersSlot 
AppRegistersFillSlotFromCellCtx(Str8 RootCtxExpr, EVRow* Row)
{
    AppRegistersSlot Result = APP_REGISTERS_SLOT_NULL;

    if (Result == APP_REGISTERS_SLOT_NULL) {
        Result = AppRegistersFillSlotFromCtxEval(Row->Eval);
    }

    for (
        EVBlock* Block = Row->Block;
        Block != EMPTY_EV_BLOCK_VALUE && Result == APP_REGISTERS_SLOT_NULL;
        Block = Block->Parent
    ) {
        Result = AppRegistersFillSlotFromCtxEval(Block->Eval);
    }

    if (Result == APP_REGISTERS_SLOT_NULL) {
        EvalEval RootCtxEval = EvalEvalFromStr(RootCtxExpr);

        Result = AppRegistersFillSlotFromCtxEval(RootCtxEval);
    }

    return Result;
}

void 
AppCommandListMenuButtons(
    Str8* CommandNames, 
    u64 CommandNamesCount, 
    u32* FastPointCodePoints
) {
    TempArena Scratch = GetScratch(NULL, 0);

    for (u64 Index = 0; Index < CommandNamesCount; ++Index) {
        if (!CommandNames[Index].Size) {
            UITag("floating"_s8) {
                UIDivider(UI_EM(1.0f, 1.0f));
            }
        } else {
            UISetNextFastpathCodepoint(FastPointCodePoints[Index]);

            UISignal Sig = AppCommandSpecButton(CommandNames[Index]);

            if (UI_CLICKED(Sig)) {
                AppCmd(
                    APP_COMMAND_KIND_RUN_COMMAND,
                    __Registers.CommandName = CommandNames[Index]
                );
                UIContextMenuClose();

                ConfigNode* Window = ConfigNodeFromID(Registers()->Window);
                AppWindowState* WS = AppWindowStateFromConfig(Window);

                WS->MenuBarFocused = FALSE;
            }
        }
    }

    ReleaseScratch(Scratch);
}