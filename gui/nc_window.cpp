#include "nc_window.h"
#include "nc_memory.h"

Str8 
StrFromInputModifierInput(
    Arena* MemPool, 
    InputModifierKind Modifiers, 
    InputKind Input
) {
    Str8 Result = {};

    if (Input != INPUT_KIND_NULL) {
        TempArena Scratch = GetScratch(&MemPool, 1);
        Str8List Mods = StrListFromInputModKind(Scratch.MemPool, Modifiers);
        Str8 KeyString = INPUT_DISPLAY_STR_TABLE[Input];

        ListPush(Scratch.MemPool, &Mods, KeyString);

        Str8JoinPart Join = {};

        Join.Sep = Str8Lit(" + ");
        Result = StrListJoin(MemPool, &Mods, &Join);
        ReleaseScratch(Scratch);
    }

    return Result;
}

Str8List 
StrListFromInputModKind(Arena* MemPool, InputModifierKind Modifiers)
{
    Str8List Result = {};
    Str8 ModifierStrings[3] = {
        Str8Lit("Ctrl"),
        Str8Lit("Shift"),
        Str8Lit("Alt")
    };

    StrListFromKinds(
        MemPool, 
        &Result, 
        Modifiers, 
        ModifierStrings, 
        3
    );

    return Result;
}

void 
ConsumeEvent(InputEventList* List, InputEvent* Event)
{
    DLL_REMOVE(List->Head, List->Tail, Event);
    Event->Next = NULL;
    Event->Prev = NULL;
    --List->Count;
}

b32 
InputPress(
    InputEventList* List, 
    Handle WindowHandle, 
    InputModifierKind Modifier, 
    InputKind Input
) {
    b32 Result = FALSE;

    for (InputEvent* Event = List->Head; Event; Event = Event->Next) {
        if (
            (
                Event->WindowHandle == WindowHandle || 
                WindowHandle == EMPTY_HANDLE_VALUE
            ) &&
            Event->Kind == EVENT_KIND_PRESS && 
            Event->Input == Input && 
            Event->Modifier == Modifier
        ) {
            Result = TRUE;
            ConsumeEvent(List, Event);
            break;
        }
    }

    return Result;
}

b32 
InputRelease(
    InputEventList* List, 
    Handle WindowHandle, 
    InputModifierKind Modifier, 
    InputKind Input
) {
    b32 Result = FALSE;

    for (InputEvent* Event = List->Head; Event; Event = Event->Next) {
        if (
            (
                Event->WindowHandle == WindowHandle || 
                WindowHandle == EMPTY_HANDLE_VALUE
            ) &&
            Event->Kind == EVENT_KIND_RELEASE &&
            Event->Input == Input &&
            Event->Modifier == Modifier
        ) {
            Result = TRUE;
            ConsumeEvent(List, Event);
            break;
        }
    }

    return Result;
}

b32 
Text(InputEventList* List, Handle WindowHandle, u32 Character)
{
    b32 Result = FALSE;

    for (InputEvent* Event = List->Head; Event; Event = Event->Next) {
        if (
            (
                Event->WindowHandle == WindowHandle || 
                WindowHandle == EMPTY_HANDLE_VALUE
            ) &&
            Event->Kind == EVENT_KIND_TEXT && 
            Event->Character == Character
        ) {
            Result = TRUE;
            ConsumeEvent(List, Event);
            break;
        }
    }

    return Result;
}

InputEventList 
EventListCopy(Arena* MemPool, InputEventList* List)
{
    InputEventList Result = {};

    for (InputEvent* Event = List->Head; Event; Event = Event->Next) {
        InputEvent* NewEvent = ArenaPushArrayZero(MemPool, InputEvent, 1);

        MemCpy(NewEvent, Event, sizeof(*Event));
        NewEvent->Strings = StrListCopy(MemPool, &Event->Strings);
        DLL_PUSH_BACK(Result.Head, Result.Tail, NewEvent);
        ++Result.Count;
    }

    return Result;
}

void 
EventListCat(InputEventList* Dst, InputEventList* Src)
{
    if (Dst->Tail && Src->Head) {
        Dst->Tail->Next = Src->Head;
        Src->Head->Prev = Dst->Tail;
        Dst->Tail = Src->Tail;
        Dst->Count += Src->Count;
    } else if (!Dst->Tail && Src->Head) {
        MemCpy(Dst, Src, sizeof(*Src));
    }

    MemSet(Src, 0, sizeof(*Src));
}

InputEvent*
ListPush(Arena* MemPool, InputEventList* List, InputEventKind Event)
{
    InputEvent* NewEvent = ArenaPushArrayZero(MemPool, InputEvent, 1);

    DLL_PUSH_BACK(List->Head, List->Tail, NewEvent);
    NewEvent->TimeStampUSecs = (u64) TimeNow();
    NewEvent->Kind = Event;
    ++List->Count;

    return NewEvent;
}


u32
CodePointFromInput(InputModifierKind Mods, InputKind Input)
{
    u32 Result = 0;

    local readonly struct {
        u32 Character;
        InputKind Input;
        InputModifierKind Mod;
    } Map[] = {
        {
            '!', 
            INPUT_KIND_1, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '@', 
            INPUT_KIND_2, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '#', 
            INPUT_KIND_3, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '$', 
            INPUT_KIND_4, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '%', 
            INPUT_KIND_5, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '^', 
            INPUT_KIND_6, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '&', 
            INPUT_KIND_7, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '*', 
            INPUT_KIND_8, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '(', 
            INPUT_KIND_9, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            ')', 
            INPUT_KIND_0, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '_', 
            INPUT_KIND_MINUS, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '_', 
            INPUT_KIND_MINUS, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '-', 
            INPUT_KIND_MINUS, 
            0
        },
        {
            '=', 
            INPUT_KIND_EQUAL, 
            0
        },
        {
            '+', 
            INPUT_KIND_EQUAL, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '`', 
            INPUT_KIND_TILDE, 
            0
        },
        {
            '~', 
            INPUT_KIND_TILDE, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '[', 
            INPUT_KIND_LEFTBRACKET, 
            0
        },
        {
            ']', 
            INPUT_KIND_RIGHTBRACKET, 
            0
        },
        {
            '{', 
            INPUT_KIND_LEFTBRACKET, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '}', 
            INPUT_KIND_RIGHTBRACKET, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '\\', 
            INPUT_KIND_BACKSLASH, 
            0
        },
        {
            '|', 
            INPUT_KIND_BACKSLASH, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            ';', 
            INPUT_KIND_SEMICOLON, 
            0
        },
        {
            ':', 
            INPUT_KIND_SEMICOLON, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '\'', 
            INPUT_KIND_QUOTE, 
            0
        },
        {
            '"', 
            INPUT_KIND_QUOTE, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '.', 
            INPUT_KIND_PERIOD, 
            0
        },
        {
            ',', 
            INPUT_KIND_COMMA, 
            0
        },
        {
            '<', 
            INPUT_KIND_PERIOD, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '>', 
            INPUT_KIND_COMMA, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            '/', 
            INPUT_KIND_SLASH, 
            0
        },
        {
            '?', 
            INPUT_KIND_SLASH, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'a', 
            INPUT_KIND_A, 
            0
        },
        {
            'b', 
            INPUT_KIND_B, 
            0
        },
        {
            'c', 
            INPUT_KIND_C, 
            0
        },
        {
            'd', 
            INPUT_KIND_D, 
            0
        },
        {
            'e', 
            INPUT_KIND_E, 
            0
        },
        {
            'f', 
            INPUT_KIND_F, 
            0
        },
        {
            'g', 
            INPUT_KIND_G, 
            0
        },
        {
            'h', 
            INPUT_KIND_H, 
            0
        },
        {
            'i', 
            INPUT_KIND_I, 
            0
        },
        {
            'j', 
            INPUT_KIND_J, 
            0
        },
        {
            'k', 
            INPUT_KIND_K, 
            0
        },
        {
            'l', 
            INPUT_KIND_L, 
            0
        },
        {
            'm', 
            INPUT_KIND_M, 
            0
        },
        {
            'n', 
            INPUT_KIND_N, 
            0
        },
        {
            'o', 
            INPUT_KIND_O, 
            0
        },
        {
            'p', 
            INPUT_KIND_P, 
            0
        },
        {
            'q', 
            INPUT_KIND_Q, 
            0
        },
        {
            'r', 
            INPUT_KIND_R, 
            0
        },
        {
            's', 
            INPUT_KIND_S, 
            0
        },
        {
            't', 
            INPUT_KIND_T, 
            0
        },
        {
            'u', 
            INPUT_KIND_U, 
            0
        },
        {
            'v', 
            INPUT_KIND_V, 
            0
        },
        {
            'w', 
            INPUT_KIND_W, 
            0
        },
        {
            'x', 
            INPUT_KIND_X, 
            0
        },
        {
            'y', 
            INPUT_KIND_Y, 
            0
        },
        {
            'z', 
            INPUT_KIND_Z, 
            0
        },
        {
            'A', 
            INPUT_KIND_A, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'B', 
            INPUT_KIND_B, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'C', 
            INPUT_KIND_C, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'D', 
            INPUT_KIND_D, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'E', 
            INPUT_KIND_E, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'F', 
            INPUT_KIND_F, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'G', 
            INPUT_KIND_G, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'H', 
            INPUT_KIND_H, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'I', 
            INPUT_KIND_I, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'J', 
            INPUT_KIND_J, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'K', 
            INPUT_KIND_K, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'L', 
            INPUT_KIND_L, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'M', 
            INPUT_KIND_M, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'N', 
            INPUT_KIND_N, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'O', 
            INPUT_KIND_O, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'P', 
            INPUT_KIND_P, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'Q', 
            INPUT_KIND_Q, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'R', 
            INPUT_KIND_R, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'S', 
            INPUT_KIND_S, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'T', 
            INPUT_KIND_T, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'U', 
            INPUT_KIND_U, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'V', 
            INPUT_KIND_V, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'W', 
            INPUT_KIND_W, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'X', 
            INPUT_KIND_X, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'Y', 
            INPUT_KIND_Y, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            'Z', 
            INPUT_KIND_Z, 
            INPUT_MOD_KIND_SHIFT
        },
        {
            ' ', 
            INPUT_KIND_SPACE, 
            0
        }
    };

    if (Input >= INPUT_KIND_0 && Input <= INPUT_KIND_9)
        Result = '0' + (Input - INPUT_KIND_0);

    for (u64 Index = 0; Index < ARRAY_COUNT(Map); ++Index) {
        if (Map[Index].Input == Input && Map[Index].Mod == Mods) {
            Result = Map[Index].Character;
            break;
        }
    }

    return Result;
}
