#include <stdarg.h>

#include "nc_ui_components.h"
#include "nc_tls.h"

void 
UIDivider(UISize Size)
{
    UIBox* Parent = UIHeadParent();

    UISetNextPreferredSize(Parent->ChildLayoutAxis, Size);
    UISetNextChildLayoutAxis(Parent->ChildLayoutAxis);

    UIBox* Box = UIBuildBoxFromKey(0, EMPTY_UI_KEY_VALUE);

    UIParent(Box) {
        UIPadding(UI_PERCENT(0.5f, 0.5f)) {
            UIPreferredSize(Parent->ChildLayoutAxis, UI_EM(0.1f, 1.0f)) {
                UIBackgroundColour(UIColourFromName("border"_s8)) {
                    UISetNextChildLayoutAxis(FLIP_AXIS(Parent->ChildLayoutAxis));

                    UIBox* SpacingParent = UIBuildBoxFromKey(0, EMPTY_UI_KEY_VALUE);

                    UIParent(SpacingParent) {
                        UIPadding(UI_EM(1.0f, 1.0f)) {
                            UIPreferredSize(FLIP_AXIS(Parent->ChildLayoutAxis), UI_PERCENT(1, 0)) {
                                UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BACKGROUND, EMPTY_UI_KEY_VALUE);
                            }
                        }
                    }
                }
            }
        }
    }
}

UISignal 
UILabel(Str8 String)
{
    UIBox* Box = UIBuildBoxFromStr(UI_BOX_KIND_DRAW_TEXT, {});

    UIBoxEquipDisplayStr(Box, String);

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UILabel(char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);
    
    va_end(Args);

    UISignal Result = UILabel(String);

    ReleaseScratch(Scratch);

    return Result;
}

void 
UILabelMultiline(f32 Max, Str8 String)
{
    TempArena Scratch = GetScratch(NULL, 0);

    UISetNextChildLayoutAxis(AXIS_2D_Y);
    UISetNextPreferredHeight(UI_SUM_OF_CHILDREN(1.0f));

    UIBox* Box = UIBuildBoxFromKey(0, {});
    Str8List Lines = FontWrappedStringLinesFromFontSizeStringMax(
        Scratch.MemPool,
        UIHeadFont(),
        UIHeadFontSize(),
        0.0f,
        UIHeadTabSize(),
        String,
        Max
    );

    for (Str8Node* Node = Lines.Head; Node; Node = Node->Next)
        UILabel(Node->String);

    ReleaseScratch(Scratch);
}

void 
UILabelMultiline(f32 Max, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);
    UILabelMultiline(Max, String);
    ReleaseScratch(Scratch);
}

UISignal 
UIButton(Str8 String)
{
    UIBox* Box = UIBuildBoxFromStr(
        (
            UI_BOX_KIND_CLICKABLE |
            UI_BOX_KIND_DRAW_BACKGROUND |
            UI_BOX_KIND_DRAW_BORDER |
            UI_BOX_KIND_DRAW_TEXT |
            UI_BOX_KIND_DRAW_HOT_EFFECTS |
            UI_BOX_KIND_DRAW_ACTIVE_EFFECTS
        ),
        String
    );
    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UIButton(char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIButton(String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UIHoverLabel(Str8 String)
{
    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_TEXT,
        String
    );

    UISignal Result = UISignalFromBox(Box);

    if (UI_HOVERING(Result))
        Box->Kind |= UI_BOX_KIND_DRAW_BORDER;

    return Result;
}

UISignal 
UIHoverLabel(char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIHoverLabel(String);

    ReleaseScratch(Scratch);

    return Result;
}

UI_BOX_CUSTOM_DRAW(UILineEditDraw)
{
    UILineEditDrawData* DrawData = (UILineEditDrawData*) UserData;
    FontTag Font = Box->Font;
    f32 FontSize = Box->FontSize;
    f32 TabSize = Box->TabSize;
    v4f32 CursorColour = UIColourFromTagsKeyName(Box->TagsKey, "cursor"_s8);

    CursorColour.W *= Box->Parent->Parent->FocusActive;

    v4f32 SelectionColour = UIColourFromTagsKeyName(Box->TagsKey, "selection"_s8);

    SelectionColour.W *= Box->Parent->Parent->FocusActive * 0.2f + 0.8f;

    v4f32 TrailColour = CursorColour;

    TrailColour.W *= 0.25f;

    v2f32 TextPosition = UIBoxTextPosition(Box);
    Str8 EditedString = DrawData->EditedString;
    TextPoint Cursor = DrawData->Cursor;
    TextPoint Mark = DrawData->Mark;
    f32 CursorPixelOffset = FontDimensionsFromTagSizeString(
        Font, 
        FontSize, 
        0.0f, 
        TabSize, 
        StrPrefix(EditedString, Cursor.Column - 1)
    ).X;
    f32 CursorPixelOffsetAnimated = UIAnimate(
        UIKeyFromStr(Box->Key, "cursor_off_px"_s8), 
        CursorPixelOffset
    );
    f32 MarkPixelOffset = FontDimensionsFromTagSizeString(
        Font, 
        FontSize, 
        0.0f, 
        TabSize, 
        StrPrefix(EditedString, Mark.Column - 1)
    ).X;
    f32 CursorThickness = CLAMP_BOT(1.0f, FloorF32(FontSize / 10.0f));
    r2f32 CursorRect = {
        TextPosition.X + CursorPixelOffset,
        Box->Parent->Parent->Rect.Y0 + UIHeadFontSize() * 0.5f,
        TextPosition.X + CursorPixelOffset + CursorThickness,
        Box->Parent->Parent->Rect.Y1 - UIHeadFontSize() * 0.5f
    };
    r1f32 TrailOffSpan = Rng(CursorPixelOffset, CursorPixelOffsetAnimated);
    r2f32 TrailRect = {
        TextPosition.X + TrailOffSpan.Min,
        CursorRect.Y0,
        TextPosition.X + TrailOffSpan.Max,
        CursorRect.Y1
    };
    r2f32 MarkRect = {
        TextPosition.X + MarkPixelOffset,
        Box->Parent->Parent->Rect.Y0 + UIHeadFontSize() * 0.5f,
        TextPosition.X + MarkPixelOffset + CursorThickness,
        Box->Parent->Parent->Rect.Y1 - UIHeadFontSize() * 0.5f
    };
    r2f32 SelectionRect = Union(CursorRect, MarkRect);

    DrawRect(SelectionRect, SelectionColour, FontSize / 2.0f, 0.0f, 1.0f);
    DrawRect(CursorRect, CursorColour, 0.0f, 0.0f, 0.0f);

    if (DrawData->Trail) {
        Rect2DInstr* TrailInstr = DrawRect(
            TrailRect, 
            TrailColour, 
            UIHeadFontSize() * 0.2f, 
            0.0f, 
            1.0f
        );

        if (CursorPixelOffset > CursorPixelOffsetAnimated) {
            TrailInstr->Colours[CORNER_00].W *= 0.1f;
            TrailInstr->Colours[CORNER_01].W *= 0.1f;
        } else {
            TrailInstr->Colours[CORNER_10].W *= 0.1f;
            TrailInstr->Colours[CORNER_11].W *= 0.1f;
        }
    }
}

UISignal 
UILineEdit(
    TextPoint* Cursor, 
    TextPoint* Mark, 
    u8* EditBuffer, 
    u64 EditBufferSize, 
    OUT u64* EditStrSize, 
    Str8 PreEditValue, 
    Str8 String
) {
    UIKey Key = UIKeyFromStr(UIActiveSeedKey(), String);
    b32 IsAutoFocusHot = UIIsKeyAutoFocusHot(Key);
    b32 IsAutoFocusActive = UIIsKeyAutoFocusActive(Key);

    UIPushFocusHot(IsAutoFocusHot ? UI_FOCUS_KIND_ON : UI_FOCUS_KIND_NULL);
    UIPushFocusActive(IsAutoFocusActive ? UI_FOCUS_KIND_ON : UI_FOCUS_KIND_NULL);

    b32 IsFocusHot = UIIsFocusHot();
    b32 IsFocusActive = UIIsFocusActive();
    b32 IsFocusHotDisabled = (!IsFocusHot && UIHeadFocusHot() == UI_FOCUS_KIND_ON);
    b32 IsFocusActiveDisabled = (!IsFocusActive && UIHeadFocusActive() == UI_FOCUS_KIND_ON);

    UISetNextHoverCursor(IsFocusActive ? CURSOR_KIND_I_BAR : CURSOR_KIND_POINTER);

    UIBox* Box = UIBuildBoxFromKey(
        (
            UI_BOX_KIND_DRAW_BACKGROUND |
            UI_BOX_KIND_DRAW_BORDER |
            UI_BOX_KIND_MOUSE_CLICKABLE |
            UI_BOX_KIND_CLICK_TO_FOCUS |
            ((IsAutoFocusHot || IsAutoFocusActive) * UI_BOX_KIND_KEYBOARD_CLICKABLE) |
            UI_BOX_KIND_DRAW_HOT_EFFECTS |
            (IsFocusActive || IsFocusActiveDisabled) * (
                UI_BOX_KIND_CLIP |
                UI_BOX_KIND_ALLOW_OVERFLOW_X |
                UI_BOX_KIND_VIEW_CLAMP
            )
        ),
        Key
    );

    b32 ChangesMade = FALSE;

    if (IsFocusActive) {
        TempArena Scratch = GetScratch(NULL, 0);

        for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
            Str8 EditString = Str(EditBuffer, *EditStrSize);

            if (
                (
                    Evt->Kind != UI_EVENT_KIND_EDIT &&
                    Evt->Kind != UI_EVENT_KIND_NAVIGATE &&
                    Evt->Kind != UI_EVENT_KIND_TEXT
                ) ||
                Evt->DeltaI32.Y != 0
            ) {
                continue;
            }

            UITextOp Op = UISingleLineTextOpFromEvent(Scratch.MemPool, Evt, EditString, *Cursor, *Mark);

            if (Op.Range.Min != Op.Range.Max || Op.Replace.Size) {
                Str8 NewString = UIPushStrReplaceRange(
                    Scratch.MemPool,
                    EditString,
                    Rng(Op.Range.Min.Column, Op.Range.Max.Column),
                    Op.Replace
                );

                NewString.Size = MIN(EditBufferSize, NewString.Size);
                MemCpy(EditBuffer, NewString.Str, NewString.Size);
                *EditStrSize = NewString.Size;
            }

            if (Op.Kind & UI_TEXT_OP_KIND_COPY)
                SetClipboardText(Op.Copy);

            *Cursor = Op.Cursor;
            *Mark = Op.Mark;

            UIEatEvent(Evt);
            ChangesMade = TRUE;
        }

        ReleaseScratch(Scratch);
    }

    TextPoint MousePt = {};
    f32 CursorOffset = 0.0f;

    UIParent(Box) {
        Str8 EditString = Str(EditBuffer, *EditStrSize);

        if (!IsFocusActive && !IsFocusActiveDisabled) {
            Str8 DisplayString = UIDisplayPartFromKeyStr(String);

            if (PreEditValue.Size)
                DisplayString = PreEditValue;

            UILabel(DisplayString);
        } else {
            f32 TotalTextWidth = FontDimensionsFromTagSizeString(
                UIHeadFont(), 
                UIHeadFontSize(), 
                0,
                UIHeadTabSize(),
                EditString
            ).X;

            UISetNextPreferredWidth(UI_PX(TotalTextWidth + UIHeadFontSize() * 5, 1.0f));

            UIBox* EditStringBox = UIBuildBoxFromStr(
                UI_BOX_KIND_DRAW_TEXT | UI_BOX_KIND_DISABLE_TEXT_TRUNC,
                "###editstr"_s8
            );
            UILineEditDrawData* DrawData = ArenaPushArrayZero(
                UIBuildMemPool(),
                UILineEditDrawData,
                1
            );

            DrawData->EditedString = ArenaPushStrCpy(UIBuildMemPool(), EditString);
            DrawData->Cursor = *Cursor;
            DrawData->Mark = *Mark;
            DrawData->Trail = TRUE;
            UIBoxEquipDisplayStr(EditStringBox, EditString);
            UIBoxEquipCustomDraw(EditStringBox, UILineEditDraw, DrawData);
            MousePt = TxtPt(1, 1 + UIBoxCharPositionFromXY(EditStringBox, UIMouse()));
            CursorOffset = FontDimensionsFromTagSizeString(
                UIHeadFont(),
                UIHeadFontSize(),
                0,
                UIHeadTabSize(),
                StrPrefix(EditString, Cursor->Column - 1)
            ).X;
        }
    }

    UISignal Sig = UISignalFromBox(Box);

    if (
        !IsFocusActive && 
        Sig.Kind & (
            UI_SIGNAL_KIND_CLICKED | 
            UI_SIGNAL_KIND_KEYBOARD_PRESSED
        )
    ) {
        UISetAutoFocusActiveKey({});
        Sig.Kind |= UI_SIGNAL_KIND_COMMIT;
    }

    if (IsFocusActive && UI_DRAGGING(Sig)) {
        if (UI_PRESSED(Sig))
            *Mark = MousePt;

        *Cursor = MousePt;
    }

    r1f32 CursorRangePX = Rng(
        CursorOffset - UIHeadFontSize() * 2.0f,
        CursorOffset + UIHeadFontSize() * 2.0f
    );
    r1f32 VisibleRangePX = Rng(
        Box->ViewOffsetTarget.X,
        Box->ViewOffsetTarget.X + Length(Box->Rect).X
    );

    CursorRangePX.Min = CLAMP_BOT(0.0f, CursorRangePX.Min);
    CursorRangePX.Max = CLAMP_BOT(0.0f, CursorRangePX.Max);

    f32 MinDelta = CursorRangePX.Min - VisibleRangePX.Min;
    f32 MaxDelta = CursorRangePX.Max - VisibleRangePX.Max;

    MinDelta = MIN(MinDelta, 0.0f);
    MaxDelta = MAX(MaxDelta, 0.0f);
    Box->ViewOffsetTarget.X += MinDelta;
    Box->ViewOffsetTarget.X += MaxDelta;
    UIPopFocusHot();
    UIPopFocusActive();

    return Sig;
}

UISignal 
UILineEdit(
    TextPoint* Cursor, 
    TextPoint* Mark, 
    u8* EditBuffer, 
    u64 EditBufferSize, 
    OUT u64* EditStrSize, 
    Str8 PreEditValue, 
    char* Fmt, 
    ...
) {
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UILineEdit(Cursor, Mark, EditBuffer, EditBufferSize, EditStrSize, PreEditValue, String);

    ReleaseScratch(Scratch);

    return Result;
}

struct UIImageDrawData {
    RendHandle          Texture;
    RendTex2DSampleKind SampleKind;
    r2f32               Region;
    v4f32               Tint;
    f32                 Blur;
};

UI_BOX_CUSTOM_DRAW(UIImageDraw)
{
    UIImageDrawData* DrawData = (UIImageDrawData*) UserData;

    if (DrawData->Texture == EMPTY_REND_HANDLE_VALUE) {
        Rect2DInstr* Instruction = DrawRect(
            Box->Rect, 
            Vec(
                0.0f, 
                0.0f, 
                0.0f, 
                0.0f
            ),
            0.0f,
            0.0f,
            1.0f
        );

        MemCpyArray(Instruction->CornerRadii, Box->CornerRadii);
    } else {
        DEFER(DPushTex2DSampleKind(DrawData->SampleKind), DPopTex2DSampleKind()) {
            Rect2DInstr* Instruction = DrawImage(
                Box->Rect, 
                DrawData->Region, 
                DrawData->Texture, 
                DrawData->Tint, 
                0.0f, 
                0.0f, 
                0.0f
            );

            MemCpyArray(Instruction->CornerRadii, Box->CornerRadii);
        }
    }

    if (DrawData->Blur > 0.01f) {
        r2f32 Clip = Box->Rect;

        for (UIBox* B = Box->Parent; !IsUIBoxEmpty(B); B = B->Parent)
            if (B->Kind & UI_BOX_KIND_CLIP)
                Clip = Intersect(B->Rect, Clip);

        RendPassBlurParams* Blur = DrawBlur(
            Intersect(Clip, Box->Rect), 
            DrawData->Blur, 
            0.0f
        );
        MemCpyArray(Blur->CornerRadii, Box->CornerRadii);
    }
}

UISignal 
UIImage(
    RendHandle Texture, 
    RendTex2DSampleKind SampleKind, 
    r2f32 Region, 
    v4f32 Tint, 
    f32 Blur, 
    Str8 String
) {
    UIBox* Box = UIBuildBoxFromStr(0, String);
    UIImageDrawData* DrawData = ArenaPushArrayZero(UIBuildMemPool(), UIImageDrawData, 1);

    DrawData->Texture = Texture;
    DrawData->SampleKind = SampleKind;
    DrawData->Region = Region;
    DrawData->Tint = Tint;
    DrawData->Blur = Blur;
    UIBoxEquipCustomDraw(Box, UIImageDraw, DrawData);

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UIImage(
    RendHandle Texture, 
    RendTex2DSampleKind SampleKind, 
    r2f32 Region, 
    v4f32 Tint, 
    f32 Blur, 
    char* Fmt, 
    ...
) {
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIImage(Texture, SampleKind, Region, Tint, Blur, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UIExpander(b32 IsExpanded, Str8 String)
{
    UISetNextHoverCursor(CURSOR_KIND_HAND_POINT);
    UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);
    UISetNextFont(UIIconFont());

    UIBox* Box = UIBuildBoxFromStr(UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_TEXT, String);

    UIBoxEquipDisplayStr(Box, IsExpanded ? "v"_s8 : ">"_s8);

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UIExpander(b32 IsExpanded, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIExpander(IsExpanded, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UISortHeader(b32 Sorting, b32 Ascending, Str8 String)
{
    UISetNextChildLayoutAxis(AXIS_2D_X);
    UISetNextHoverCursor(CURSOR_KIND_HAND_POINT);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_ACTIVE_EFFECTS,
        String
    );

    UIPushParent(Box);

    if (Sorting) {
        UISetNextPreferredWidth(UI_EM(1.8f, 1.0f));
        UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);
        UISetNextFont(UIIconFont());

        UIBox* Icon = UIBuildBoxFromStr(UI_BOX_KIND_DRAW_TEXT, ""_s8);

        UIBoxEquipDisplayStr(Icon, Ascending ? "^"_s8 : "v"_s8);
    }

    UILabel(String);
    UIPopParent();

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UISortHeader(b32 Sorting, b32 Ascending, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UISortHeader(Sorting, Ascending, String);

    ReleaseScratch(Scratch);

    return Result;
}

void 
UIDoColourTooltip(v3f32 HSV)
{
    v3f32 ColourRGB = RGBFromHSV(HSV);

    UITooltip() {
        UIPadding(UI_EM(2.0f, 1.0f)) {
            UIPreferredWidth(UI_EM(22.0f, 1.0f)) {
                UIPreferredHeight(UI_EM(6.0f, 1.0f)) {
                    UIRow() {
                        UIPadding(UI_PERCENT(1.0f, 0.0f)) {
                            UIBackgroundColour(
                                LinearFromSRGBA(
                                    Vec(
                                        ColourRGB.X, 
                                        ColourRGB.Y, 
                                        ColourRGB.Z, 
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
                }
            }
        }

        UISpacer(UI_EM(0.3f, 1.0f));

        UIPreferredWidth(UI_EM(22.0f, 1.0f)) {
            UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                UILabel(
                    (char*) "Hex: #%02X%02X%02X", 
                    (u8) (ColourRGB.X * 255.0f), 
                    (u8) (ColourRGB.Y * 255.0f), 
                    (u8) (ColourRGB.Z * 255.0f)
                );
            }
        }

        UISpacer(UI_EM(0.3f, 1.0f));

        UIPreferredWidth(UI_EM(22.0f, 1.0f)) {
            UIPreferredHeight(UI_SUM_OF_CHILDREN(1.0f)) {
                UIRow() {
                    UIWidthFill() {
                        UIColumn() {
                            UIPreferredHeight(UI_EM(1.8f, 1.0f)) {
                                UILabel((char*) "Red   : %.2f", ColourRGB.X);
                                UILabel((char*) "Green : %.2f", ColourRGB.Y);
                                UILabel((char*) "Blue  : %.2f", ColourRGB.Z);
                            }
                        }
                    }
                    UIWidthFill() {
                        UIColumn() {
                            UIPreferredHeight(UI_EM(1.8f, 1.0f)) {
                                UILabel((char*) "Hue : %.2f", HSV.X);
                                UILabel((char*) "Sat : %.2f", HSV.Y);
                                UILabel((char*) "Val : %.2f", HSV.Z);
                            }
                        }
                    }
                }
            }
        }
    }
}

void 
UIDoColourTooltip(v4f32 HSVA)
{
    v3f32 HSV = Vec(HSVA.X, HSVA.Y, HSVA.Z);
    v3f32 RGB = RGBFromHSV(HSV);
    v4f32 RGBA = Vec(RGB.R, RGB.G, RGB.B, HSVA.W);

    UITooltip() {
        UIPadding(UI_EM(2.0f, 1.0f)) {
            UIPreferredWidth(UI_EM(22.0f, 1.0f)) {
                UIPreferredHeight(UI_EM(6.0f, 1.0f)) {
                    UIRow() {
                        UIPadding(UI_PERCENT(1, 0)) {
                            UIBackgroundColour(LinearFromSRGBA(RGBA)) {
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
                }
            }

            UISpacer(UI_EM(0.3f, 1.0f));

            UIPreferredWidth(UI_EM(22.0f, 1.0f)) {
                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                    UILabel(
                        "Hex: #%02X%02X%02X%02X",
                        (u8) (RGBA.R * 255.0f),
                        (u8) (RGBA.G * 255.0f),
                        (u8) (RGBA.B * 255.0f),
                        (u8) (RGBA.A * 255.0f)
                    );
                }
            }

            UISpacer(UI_EM(0.3f, 1.0f));

            UIPreferredWidth(UI_EM(22.0f, 1.0f)) {
                UIPreferredHeight(UI_SUM_OF_CHILDREN(1)) {
                    UIRow() {
                        UIWidthFill() {
                            UIColumn() {
                                UIPreferredHeight(UI_EM(1.8f, 1.0f)) {
                                    UILabel("Red   : %.2f", RGBA.R);
                                    UILabel("Green : %.2f", RGBA.G);
                                    UILabel("Blue  : %.2f", RGBA.B);
                                    UILabel("Alpha : %.2f", RGBA.A);
                                }
                            }
                        }

                        UIWidthFill() {
                            UIColumn() {
                                UIPreferredHeight(UI_EM(1.8f, 1.0f)) {
                                    UILabel("Hue   : %.2f", HSVA.X);
                                    UILabel("Sat   : %.2f", HSVA.Y);
                                    UILabel("Val   : %.2f", HSVA.Z);
                                    UILabel("Alpha : %.2f", HSVA.W);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UISaturationValPickerDraw)
{
    UISaturationValDrawData* Data = (UISaturationValDrawData*) UserData;
    v3f32 HueRGB = RGBFromHSV(Vec(Data->Hue, 1.0f, 1.0f));
    v3f32 HueRGBLinear = LinearFromSRGB(HueRGB);

    DrawRect(
        Widen(Box->Rect, -1.0f),
        Vec(
            HueRGBLinear.X,
            HueRGBLinear.Y,
            HueRGBLinear.Z,
            1.0f
        ),
        4.0f,
        0.0f,
        1.0f
    );

    {
        Rect2DInstr* Instr = DrawRect(
            Widen(Box->Rect, -1.0f),
            Vec(
                HueRGBLinear.X,
                HueRGBLinear.Y,
                HueRGBLinear.Z,
                1.0f
            ),
            4.0f,
            0.0f,
            1.0f
        );

        Instr->Colours[CORNER_00] = Vec(1.0f, 1.0f, 1.0f, 1.0f);
        Instr->Colours[CORNER_01] = Vec(1.0f, 1.0f, 1.0f, 1.0f);
    }

    {
        Rect2DInstr* Instr = DrawRect(
            Widen(Box->Rect, -1.0f),
            Vec(0.0f, 0.0f, 0.0f, 0.0f),
            4.0f,
            0.0f,
            1.0f
        );

        Instr->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 1.0f);
        Instr->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 1.0f);
    }

    {
        Rect2DInstr* Instr = DrawRect(
            Widen(Box->Rect, -1.0f),
            Vec(0.0f, 0.0f, 0.0f, 0.0f),
            4.0f,
            0.0f,
            1.0f
        );

        Instr->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 1.0f);
        Instr->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 1.0f);
    }

    {
        Rect2DInstr* Instr = DrawRect(
            Widen(Box->Rect, -1.0f),
            Vec(0.0f, 0.0f, 0.0f, 0.0f),
            4.0f,
            0.0f,
            1.0f
        );

        Instr->Colours[CORNER_01] = Vec(0.0f, 0.0f, 0.0f, 0.2f);
        Instr->Colours[CORNER_11] = Vec(0.0f, 0.0f, 0.0f, 0.2f);
    }

    {
        v2f32 BoxRectDim = Length(Box->Rect);
        v2f32 BoxCentre = Vec(
            Box->Rect.X0 + Data->Saturation * BoxRectDim.X,
            Box->Rect.Y0 + (1.0f - Data->Value) * BoxRectDim.Y
        );
        f32 HalfSize = Box->FontSize * (0.5f + Box->Active * 0.2f);
        r2f32 Rect = Rng(
            BoxCentre.X - HalfSize,
            BoxCentre.Y - HalfSize,
            BoxCentre.X + HalfSize,
            BoxCentre.Y + HalfSize
        );

        DrawRect(
            Rect,
            Vec(1.0f, 1.0f, 1.0f, 1.0f),
            HalfSize / 2.0f,
            2.0f,
            1.0f
        );
    }
}

UISignal 
UISaturationValPicker(f32 Hue, OUT f32* Saturation, OUT f32* Val, Str8 String)
{
    UISetNextHoverCursor(CURSOR_KIND_HAND_POINT);

    UIBox* Box = UIBuildBoxFromStr(UI_BOX_KIND_CLICKABLE, String);
    UISaturationValDrawData* User = ArenaPushArrayZero(
        UIBuildMemPool(), 
        UISaturationValDrawData, 
        1
    );

    UIBoxEquipCustomDraw(Box, UISaturationValPickerDraw, User);

    UISignal Sig = UISignalFromBox(Box);

    if (UI_DRAGGING(Sig)) {
        v2f32 Dim = Length(Box->Rect);

        *Saturation = (UIMouse().X - Box->Rect.X0) / Dim.X;
        *Val = 1.0f - (UIMouse().Y - Box->Rect.Y0) / Dim.Y;
        *Saturation = CLAMP(0.0f, *Saturation, 1.0f);
        *Val = CLAMP(0.0f, *Val, 1.0f);

        UIDoColourTooltip(Vec(Hue, *Saturation, *Val));

        if (UI_PRESSED(Sig)) {
            v2f32 Data = Vec(*Saturation, *Val);

            UIStoreDragStruct(&Data);
        }

        if (UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)) {
            v2f32 Data = *UIGetDragStruct(v2f32);
            
            *Saturation = Data.X;
            *Val = Data.Y;
            UIKillAction();
        }
    }

    {
        User->Hue = Hue;
        User->Saturation = *Saturation;
        User->Value = *Val;
    }

    return Sig;
}

UISignal 
UISaturationValPicker(f32 Hue, OUT f32* Saturation, OUT f32* Val, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Sig = UISaturationValPicker(Hue, Saturation, Val, String);

    ReleaseScratch(Scratch);

    return Sig;
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIHuePickerDraw)
{
    UIHuePickerDrawData* Data = (UIHuePickerDrawData*) UserData;
    v2f32 BoxLength = Length(Box->Rect);
    f32 SegmentLength = FloorF32(BoxLength.Y / 6.0f);
    r2f32 HueCycleRect = Box->Rect;
    v2f32 HueCycleCentre = Centre(HueCycleRect);

    HueCycleRect.X0 += (HueCycleCentre.X - HueCycleRect.X0) * 0.3f;
    HueCycleRect.X1 += (HueCycleCentre.X - HueCycleRect.X1) * 0.3f;

    r2f32 Rect = Rng(
        HueCycleRect.X0,
        HueCycleRect.Y0,
        HueCycleRect.X1,
        HueCycleRect.Y0 + SegmentLength
    );

    for (u8 Segment = 0; Segment < 6; ++Segment) {
        f32 Hue0 = (f32) (Segment) / 6;
        f32 Hue1 = (f32) (Segment + 1) / 6;

        v3f32 RGB0 = RGBFromHSV(Vec(Hue0, 1.0f, 1.0f));
        v3f32 RGB1 = RGBFromHSV(Vec(Hue1, 1.0f, 1.0f));
        v4f32 RGBA0Linear = LinearFromSRGBA(
            Vec(
                RGB0.R,
                RGB0.G,
                RGB0.B,
                1.0f
            )
        );
        v4f32 RGBA1Linear = LinearFromSRGBA(
            Vec(
                RGB1.R,
                RGB1.G,
                RGB1.B,
                1.0f
            )
        );
        Rect2DInstr* Instruction = DrawRect(
            Rect,
            Vec(0.0f, 0.0f, 0.0f, 0.0f),
            0.0f,
            0.0f,
            0.0f
        );

        Instruction->Colours[CORNER_00] = RGBA0Linear;
        Instruction->Colours[CORNER_01] = RGBA1Linear;
        Instruction->Colours[CORNER_10] = RGBA0Linear;
        Instruction->Colours[CORNER_11] = RGBA1Linear;
        Rect.Y0 += SegmentLength;
        Rect.Y1 += SegmentLength;
    }

    {
        v2f32 BoxRectLength = Length(Box->Rect);
        v2f32 BoxCentre = Vec(
            (Box->Rect.X0 + Box->Rect.X1) / 2.0f,
            Box->Rect.Y0 + (Data->Hue) * BoxRectLength.Y
        );
        f32 HalfSize = BoxRectLength.X * (0.52f + 0.02f * Box->Active);
        r2f32 Rect = Rng(
            BoxCentre.X - HalfSize,
            BoxCentre.Y - Box->FontSize * (0.5f + 0.1f * Box->Active),
            BoxCentre.X + HalfSize,
            BoxCentre.Y + Box->FontSize * (0.5f + 0.1f * Box->Active)
        );

        DrawRect(Rect, Vec(1.0f, 1.0f, 1.0f, 1.0f), 1.0f, 2.0f, 1.0f);
    }
}

UISignal 
UIHuePicker(OUT f32* Hue, f32 Sat, f32 Val, Str8 String)
{
    UISetNextHoverCursor(CURSOR_KIND_HAND_POINT);

    UIBox* Box = UIBuildBoxFromStr(UI_BOX_KIND_CLICKABLE, String);
    UIHuePickerDrawData* Data = ArenaPushArrayZero(
        UIBuildMemPool(),
        UIHuePickerDrawData,
        1
    );

    UIBoxEquipCustomDraw(Box, UIHuePickerDraw, Data);

    UISignal Result = UISignalFromBox(Box);

    if (UI_DRAGGING(Result)) {
        v2f32 BoxLength = Length(Box->Rect);

        *Hue = (UIMouse().Y - Box->Rect.Y0) / BoxLength.Y;
        *Hue = CLAMP(0.0f, *Hue, 1.0f);
        UIDoColourTooltip(Vec(*Hue, Sat, Val));

        if (UI_PRESSED(Result))
            UIStoreDragStruct(Hue);

        if (UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)) {
            *Hue = *UIGetDragStruct(f32);
            UIKillAction();
        }
    }

    {
        Data->Hue = *Hue;
        Data->Saturation = Sat;
        Data->Value = Val;
    }

    return Result;
}

UISignal 
UIHuePicker(OUT f32* Hue, f32 Sat, f32 Val, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIHuePicker(Hue, Sat, Val, String);

    ReleaseScratch(Scratch);

    return Result;
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIAlphaPickerDraw)
{
    UIAlphaPickerDrawData *Data = (UIAlphaPickerDrawData *) UserData;
    v2f32 BoxLength = Length(Box->Rect);
  
    {
        r2f32 Rect = Box->Rect;
        v2f32 BoxRectCentre = Centre(Rect);

        Rect.X0 += (BoxRectCentre.X - Rect.X0) * 0.3f;
        Rect.X1 += (BoxRectCentre.X - Rect.X1) * 0.3f;

        Rect2DInstr *Instruction = DrawRect(
            Rect, 
            Vec(0.0f, 0.0f, 0.0f, 0.0f), 
            0.0f, 
            0.0f, 
            0.0f
        );

        Instruction->Colours[CORNER_00] = Vec(1.0f, 1.0f, 1.0f, 1.0f);
        Instruction->Colours[CORNER_10] = Vec(1.0f, 1.0f, 1.0f, 1.0f);
    }
  
    {
        v2f32 BoxRectLength = Length(Box->Rect);
        v2f32 BoxRectCentre = Vec(
            (Box->Rect.X0 + Box->Rect.X1) / 2.0f, 
            Box->Rect.Y0 + (1.0f - Data->Alpha) * BoxRectLength.Y
        );
        f32 HalfSize = BoxRectLength.X * (0.52f + 0.02f * Box->Active);
        r2f32 Rect = Rng(
            BoxRectCentre.X - HalfSize, 
            BoxRectCentre.Y - Box->FontSize * (0.5f + 0.1f * Box->Active),
            BoxRectCentre.X + HalfSize,
            BoxRectCentre.Y + Box->FontSize * (0.5f + 0.1f * Box->Active)
        );

        DrawRect(
            Rect, 
            Vec(1.0f, 1.0f, 1.0f, 1.0f), 
            1.f, 
            2.f, 
            1.f
        );
    }
}

UISignal 
UIAlphaPicker(OUT f32* Alpha, Str8 String)
{
    UISetNextHoverCursor(CURSOR_KIND_HAND_POINT);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE,
        String
    );
    UIAlphaPickerDrawData* Data = ArenaPushArrayZero(
        UIBuildMemPool(),
        UIAlphaPickerDrawData,
        1
    );

    UIBoxEquipCustomDraw(Box, UIAlphaPickerDraw, Data);

    UISignal Result = UISignalFromBox(Box);

    if (UI_DRAGGING(Result)) {
        v2f32 BoxLength = Length(Box->Rect);
        f32 DragPercent = (UIMouse().Y - Box->Rect.Y0) / BoxLength.Y;

        DragPercent = CLAMP(0.0f, DragPercent, 1.0f);
        *Alpha = 1.0f - DragPercent;

        if (UI_PRESSED(Result))
            UIStoreDragStruct(Alpha);

        if (UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)) {
            *Alpha = *UIGetDragStruct(f32);
            UIKillAction();
        }
    }

    {
        Data->Alpha = *Alpha;
    }

    return Result;
}

UISignal 
UIAlphaPicker(OUT f32* Alpha, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIAlphaPicker(Alpha, String);

    ReleaseScratch(Scratch);

    return Result;
}

UIBox* 
UIRowBegin(void)
{
    return UINamedRowBegin(""_s8);
}

UISignal 
UIRowEnd(void)
{
    return UINamedRowEnd();
}

UIBox* 
UIColumnBegin(void)
{
    return UINamedColumnBegin(""_s8);
}

UISignal 
UIColumnEnd(void)
{
    return UINamedColumnEnd();
}

UIBox* 
UINamedRowBegin(Str8 String)
{
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(0, String);

    UIPushParent(Box);

    return Box;
}

UISignal 
UINamedRowEnd(void)
{
    UIBox* Box = UIPopParent();
    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UIBox* 
UINamedColumnBegin(Str8 String)
{
    UISetNextChildLayoutAxis(AXIS_2D_Y);

    UIBox* Box = UIBuildBoxFromStr(0, String);

    UIPushParent(Box);

    return Box;
}

UISignal 
UINamedColumnEnd(void)
{
    UIBox* Box = UIPopParent();
    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UIBox* 
UIPaneBegin(r2f32 Rect, Str8 String)
{
    UIPushRect(Rect);
    UISetNextChildLayoutAxis(AXIS_2D_Y);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_CLIP | UI_BOX_KIND_DRAW_BORDER | UI_BOX_KIND_DRAW_BACKGROUND,
        String
    );

    UIPopRect();
    UIPushParent(Box);
    UIPushPreferredWidth(UI_PERCENT(1.0f, 0.0f));

    return Box;
}

UIBox* 
UIPaneBegin(r2f32 Rect, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIBox* Result = UIPaneBegin(Rect, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UIPaneEnd(void)
{
    UIPopPreferredWidth();

    UIBox* Box = UIPopParent();
    UISignal Result = UISignalFromBox(Box);

    return Result;
}

void 
UITableBegin(u64 ColumnPercentsCount, f32** ColumnPercents, Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();

    TLS->UITableSizeColumnPercentsCount = ColumnPercentsCount;
    UISetNextPreferredHeight(UI_SUM_OF_CHILDREN(1.0f));
    UISetNextChildLayoutAxis(AXIS_2D_Y);

    UIBox* Table = UIBuildBoxFromStr(0, String);

    UIPushParent(Table);

    f32 XOffset = ((TLS->UITableSizeColumnPercentsCount > 0) ? *ColumnPercents[0] : 0.0f) * Length(Table->Rect).X;

    for (u64 ColumnIndex = 1; ColumnIndex < TLS->UITableSizeColumnPercentsCount; ++ColumnIndex) {
        r2f32 Rect = {};

        Rect.X0 = XOffset - 3.0f;
        Rect.Y0 = 0.0f;
        Rect.X1 = XOffset + 3.0f;
        Rect.Y1 = Length(Table->Rect).Y;
        XOffset += *ColumnPercents[ColumnIndex] * Length(Table->Rect).X;

        UIRect(Rect) {
            UISetNextHoverCursor(CURSOR_KIND_LEFT_RIGHT);

            UIBox* Box = UIBuildBoxFromStrFmt(UI_BOX_KIND_CLICKABLE, (char*) "###%S_boundary_%llu", PRINT_STR(Table->String), ColumnIndex);
            f32* LeftPercentPtr = (ColumnIndex < TLS->UITableSizeColumnPercentsCount) ? ColumnPercents[ColumnIndex - 1] : NULL;
            f32* RightPercentPtr = (ColumnIndex < TLS->UITableSizeColumnPercentsCount) ? ColumnPercents[ColumnIndex] : NULL;
            UISignal Signal = UISignalFromBox(Box);

            if (UI_DRAGGING(Signal)) {
                if (UI_PRESSED(Signal)) {
                    v2f32 V = Vec(*LeftPercentPtr, *RightPercentPtr);

                    UIStoreDragStruct(&V);
                }

                f32 AdjustableTableDimension = 0.0f;

                if (Table->ChildLayoutAxis == AXIS_2D_Y) {
                    AdjustableTableDimension = Length(Table->Rect).X;
                } else {
                    u64 ChildIndex = 0;

                    for (UIBox* V = Table->Head; !IsUIBoxEmpty(V); V = V->Next, ++ChildIndex) {
                        u64 ColumnIndex = (ChildIndex + 1);

                        if (ColumnIndex < TLS->UITableSizeColumnPercentsCount)
                            AdjustableTableDimension += Length(V->Rect).X;
                        else
                            break;
                    }
                }

                f32 MinSize = 30.0f;
                f32 LeftPercentBefore = UIGetDragStruct(v2f32)->X;
                f32 LeftPixelsBefore = LeftPercentBefore * AdjustableTableDimension;
                f32 LeftPixelsAfter = LeftPixelsBefore + UIDragDelta().X;

                if (LeftPixelsAfter < MinSize)
                    LeftPixelsAfter = MinSize;

                f32 LeftPercentAfter = LeftPixelsAfter / AdjustableTableDimension;
                f32 PercentDelta = LeftPercentAfter - LeftPercentBefore;
                f32 RightPercentBefore = UIGetDragStruct(v2f32)->Y;
                f32 RightPercentAfter = RightPercentBefore - PercentDelta;
                f32 RightPixelsAfter = RightPercentAfter * AdjustableTableDimension;

                if (RightPixelsAfter < MinSize) {
                    RightPixelsAfter = MinSize;
                    RightPercentAfter = RightPixelsAfter / AdjustableTableDimension;
                    PercentDelta = -(RightPercentAfter - RightPercentBefore);
                    LeftPercentAfter = LeftPercentBefore + PercentDelta;
                }

                *LeftPercentPtr = LeftPercentAfter;
                *RightPercentPtr = RightPercentAfter;
            }
        }
    }

    TLS->UITableSizeColumnPercentsStable = ArenaPushArrayZero(
        UIBuildMemPool(), 
        f32, 
        TLS->UITableSizeColumnPercentsCount
    );

    for (u64 I = 0; I < ColumnPercentsCount; ++I)
        TLS->UITableSizeColumnPercentsStable[I] = *ColumnPercents[I];

    TLS->UITableSizeVectorIndex = 0;
}

void 
UITableBegin(u64 ColumnPercentsCount, f32** ColumnPercents, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);
    UITableBegin(ColumnPercentsCount, ColumnPercents, String);
    ReleaseScratch(Scratch);
}

void 
UITableEnd(void)
{
    UIPopParent();
}

UIBox* 
UINamedTableVectorBegin(Str8 String)
{
    UISetNextPreferredWidth(UI_PERCENT(1.0f, 0.0f));
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Result = UIBuildBoxFromStr(UI_BOX_KIND_DRAW_SIDE_BOTTOM, String);

    ++GetTLS()->UITableSizeVectorIndex;
    GetTLS()->UITableSizeCellIndex = 0;
    UIPushParent(Result);

    return Result;
}

UIBox* 
UINamedTableVectorBegin(char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIBox* Result = UINamedTableVectorBegin(String);

    ReleaseScratch(Scratch);

    return Result;
}

UIBox* 
UITableVectorBegin(void)
{
    UIBox* Table = UIHeadParent();
    UIBox* Result = UINamedTableVectorBegin(
        (char*) "###tbl_vec_%p_%llu", 
        Table, 
        GetTLS()->UITableSizeVectorIndex
    );

    return Result;
}

UISignal 
UITableVectorEnd(void)
{
    UIBox* Box = UIPopParent();
    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UIBox* 
UITableCellBegin(void)
{
    u64 ColumnIndex = GetTLS()->UITableSizeCellIndex;
    f32 WidthPercent = (ColumnIndex < GetTLS()->UITableSizeColumnPercentsCount)
            ? GetTLS()->UITableSizeColumnPercentsStable[ColumnIndex]
            : 1.0f;
    UIBox* Result = UITableCellSizedBegin(UI_PERCENT(WidthPercent, 0.0f));

    return Result;
}

UISignal 
UITableCellEnd(void)
{
    UIBox* Cell = UIPopParent();
    UISignal Result = UISignalFromBox(Cell);

    return Result;
}

UIBox* 
UITableCellSizedBegin(UISize Size)
{
    UIBox* Vector = UIHeadParent();
    u64 ColumnIndex = GetTLS()->UITableSizeCellIndex;

    ++GetTLS()->UITableSizeCellIndex;
    UISetNextPreferredWidth(Size);
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Result = UIBuildBoxFromStrFmt(
        ((ColumnIndex > 0) ? UI_BOX_KIND_DRAW_SIDE_LEFT : 0), 
        (char*) "###tbl_cell_%p_%llu", 
        Vector,
        GetTLS()->UITableSizeCellIndex
    );

    UIPushParent(Result);

    return Result;
}

void 
ListPush(
    Arena* MemPool, 
    UIScrollListRowBlockChunkList* List, 
    u64 Capacity, 
    UIScrollListRowBlock* Block
) {
    UIScrollListRowBlockChunkNode* N = List->Tail;

    if (!N || N->Count >= N->Capacity) {
        N = ArenaPushArrayZero(MemPool, UIScrollListRowBlockChunkNode, 1);
        N->Capacity = Capacity;
        N->V = ArenaPushArray(MemPool, UIScrollListRowBlock, N->Capacity);
        SLL_QUEUE_PUSH(List->Head, List->Tail, N);
        ++List->ChunkCount;
    }

    MemCpy(&N->V[N->Count], Block, sizeof(N->V[N->Count]));
    ++N->Count;
    ++List->TotalCount;
}

UIScrollListRowBlockArray 
UIScrollListRowBlockArrayFromChunkList(
    Arena* MemPool, 
    UIScrollListRowBlockChunkList* List
) {
    UIScrollListRowBlockArray Result = {};

    Result.Count = List->TotalCount;
    Result.V = ArenaPushArray(MemPool, UIScrollListRowBlock, Result.Count);

    u64 Index = 0;

    for (
        UIScrollListRowBlockChunkNode* N = List->Head;
        N;
        N = N->Next
    ) {
        MemCpy(Result.V + Index, N->V, sizeof(N->V[0]) * N->Count);
        Index += N->Count;
    }

    return Result;
}

u64 
UIScrollListRowFromItem(UIScrollListRowBlockArray* Blocks, u64 Item)
{
    u64 Result = 0;
    u64 RowIndex = 0;
    u64 ItemIndex = 0;

    for (u64 BlockIndex = 0; BlockIndex < Blocks->Count; ++BlockIndex) {
        UIScrollListRowBlock* Block = &Blocks->V[BlockIndex];
        u64 NextRowIndex = RowIndex + Block->RowCount;
        u64 NextItemIndex = ItemIndex + Block->ItemCount;

        if (ItemIndex <= Item && Item < NextItemIndex) {
            u64 ItemOffsetRows = (Item - ItemIndex) * (Block->RowCount / Block->ItemCount);

            Result = RowIndex + ItemOffsetRows;
            break;
        }

        RowIndex = NextRowIndex;
        ItemIndex = NextItemIndex;
    }

    return Result;
}

u64 
UIScrollListItemFromRow(UIScrollListRowBlockArray* Blocks, u64 Row)
{
    u64 Result = 0;
    u64 RowIndex = 0;
    u64 ItemIndex = 0;

    for (u64 BlockIndex = 0; BlockIndex < Blocks->Count; ++BlockIndex) {
        UIScrollListRowBlock* Block = &Blocks->V[BlockIndex];
        u64 NextRowIndex = RowIndex + Block->RowCount;
        u64 NextItemIndex = ItemIndex + Block->ItemCount;

        if (RowIndex <= Row && Row < NextRowIndex) {
            Result = ItemIndex;
            break;
        }

        RowIndex = NextRowIndex;
        ItemIndex = NextItemIndex;
    }

    return Result;
}

UIScrollPoint 
UIScrollBar(
    Axis2D Axis, 
    UISize OffsetAxisSize, 
    UIScrollPoint Point, 
    r1i64 IndexRange, 
    i64 ViewNumIndices
) {
    UIPushTag("scroll_bar"_s8);
    UIPushFontSize(UITailFontSize() * 0.65f);

    i64 IndexRangeDim = MAX(Length(IndexRange), 1);
    UIBoxKind DisabledKind = 0;

    if (IndexRange.Min == IndexRange.Max)
        DisabledKind |= UI_BOX_KIND_DISABLED;

    f32 ScrollerFrac = CLAMP(
        0.05f,
        (f32)((f64) MAX(ViewNumIndices, 1) / (f64) IndexRangeDim),
        1.0f
    );
f32 ScrollFrac = (f32)((f64)(Point.Index - IndexRange.Min) / (f64) IndexRangeDim);
f32 BeforeFrac = (1.0f - ScrollerFrac) * ScrollFrac;
f32 AfterFrac  = (1.0f - ScrollerFrac) - BeforeFrac;

    UISetNextPreferredSize(FLIP_AXIS(Axis), OffsetAxisSize);
    UISetNextChildLayoutAxis(Axis);

    UIBox* ContainerBox = UIBuildBoxFromKey(UI_BOX_KIND_DRAW_BORDER, {});
    UISignal MinScrollSig = {};

    UIParent(ContainerBox) {
        UIPreferredSize(Axis, OffsetAxisSize) {
            UIFlags(UI_BOX_KIND_DRAW_BORDER | DisabledKind) {
                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                    UIFont(UIIconFont()) {
                        Str8 ArrowString = UIIconStringFromIconKind((Axis == AXIS_2D_X) ? UI_ICON_KIND_LEFT_ARROW : UI_ICON_KIND_UP_ARROW);

                        MinScrollSig = UIButton("%S##_min_scroll_%i", PRINT_STR(ArrowString), Axis);
                    }
                }
            }
        }
    }

    UISignal SpaceBeforeSig = {};
    UISignal SpaceAfterSig = {};
    UISignal ScrollerSig = {};
    UIBox* ScrollAreaBox = EMPTY_UI_BOX_VALUE;
    UIBox* ScrollerBox = EMPTY_UI_BOX_VALUE;

    UIParent(ContainerBox) {
        UISetNextPreferredSize(Axis, UI_PERCENT(1, 0));
        UISetNextChildLayoutAxis(Axis);
        ScrollAreaBox = UIBuildBoxFromStrFmt(0, (char*) "##_scroll_area_%i", Axis);

        UIParent(ScrollAreaBox) {
            if (IndexRange.Max != IndexRange.Min) {
                UISetNextPreferredSize(Axis, UI_PERCENT(BeforeFrac, 0));

                UIBox* SpaceBeforeBox = UIBuildBoxFromStrFmt(UI_BOX_KIND_CLICKABLE, (char*) "##scroll_area_before");

                SpaceBeforeSig = UISignalFromBox(SpaceBeforeBox);
            }

            UIFlags(DisabledKind) {
                UIPreferredSize(Axis, UI_PERCENT(ScrollerFrac, 0)) {
                    ScrollerSig = UIButton((char*) "##_scroller_%i", Axis);
                    ScrollerBox = ScrollerSig.Box;
                }
            }

            if (IndexRange.Max != IndexRange.Min) {
                UISetNextPreferredSize(Axis, UI_PERCENT(AfterFrac, 0));

                UIBox* SpaceAfterBox = UIBuildBoxFromStrFmt(UI_BOX_KIND_CLICKABLE, (char*) "##scroll_area_after");

                SpaceAfterSig = UISignalFromBox(SpaceAfterBox);
            }
        }
    }

    UISignal MaxScrollSig = {};

    UIParent(ContainerBox) {
        UIPreferredSize(Axis, OffsetAxisSize) {
            UIFlags(UI_BOX_KIND_DRAW_BORDER | DisabledKind) {
                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                    UIFont(UIIconFont()) {
                        Str8 ArrowString = UIIconStringFromIconKind((Axis == AXIS_2D_X) ? UI_ICON_KIND_RIGHT_ARROW : UI_ICON_KIND_DOWN_ARROW);

                        MaxScrollSig = UIButton("%S##_max_scroll_%i", PRINT_STR(ArrowString), Axis);
                    }
                }
            }
        }
    }

    UIScrollPoint NewPoint = Point;

    struct UIScrollBarDragData {
        UIScrollPoint StartPoint;
        f32 ScrollSpacePX;
    };

    if (UI_DRAGGING(ScrollerSig)) {
        if (UI_PRESSED(ScrollerSig)) {
            UIScrollBarDragData DragData = {
                Point,
                FloorF32(Length(ScrollAreaBox->Rect).V[Axis])
            };

            UIStoreDragStruct(&DragData);
        }

        UIScrollBarDragData* DragData = UIGetDragStruct(UIScrollBarDragData);
        UIScrollPoint OriginalPoint = DragData->StartPoint;
        f32 DragDelta = UIDragDelta().V[Axis];
        f32 DragPercent = DragDelta / DragData->ScrollSpacePX;
        i64 NewIndex = OriginalPoint.Index + DragPercent * IndexRangeDim;

        NewIndex = CLAMP(IndexRange.Min, NewIndex, IndexRange.Max);
        UIScrollPointTargetIndex(&NewPoint, NewIndex);
        NewPoint.Offset = 0.0f;
    }

    if (UI_DRAGGING(MinScrollSig) || UI_DRAGGING(SpaceBeforeSig)) {
        i64 NewIndex = NewPoint.Index - 1;

        NewIndex = CLAMP(IndexRange.Min, NewIndex, IndexRange.Max);
        UIScrollPointTargetIndex(&NewPoint, NewIndex);
    }

    if (UI_DRAGGING(MaxScrollSig) || UI_DRAGGING(SpaceAfterSig)) {
        i64 NewIndex = NewPoint.Index + 1;

        NewIndex = CLAMP(IndexRange.Min, NewIndex, IndexRange.Max);
        UIScrollPointTargetIndex(&NewPoint, NewIndex);
    }

    UIPopFontSize();
    UIPopTag();

    return NewPoint;
}

void 
UIScrollListBegin(
    UIScrollListParameters* Params, 
    OUT UIScrollPoint* ScrollPoint, 
    OUT v2i64* Cursor, 
    OUT v2i64* Mark, 
    OUT r1i64* VisibleRowRange, 
    OUT UIScrollListSignal* Signal
) {
    r1i64 ScrollRowIndexRange = Rng(
        (i64) Params->ItemRange.Min, 
        (i64) CLAMP_BOT(
            Params->ItemRange.Min, 
            Params->ItemRange.Max - 1
        )
    );
    i64 NumPossibleVisibleRows = (i64) (Params->DimensionsPX.Y / Params->RowHeightPX);
    b32 Moved = FALSE;

    if (Params->Kind & UI_SCROLL_LIST_KIND_NAV && Cursor && UIIsFocusActive()) {
        v2i64 CursorPosition = *Cursor;
        v2i64 MarkPosition = (Mark) ? *Mark : CursorPosition;

        for (UIEvent* Evt = NULL; UINextEvent(&Evt); ) {
            if ((Evt->DeltaI32.X == 0 && Evt->DeltaI32.Y == 0) || Evt->Flags & UI_EVENT_FLAG_DELETE)
                continue;

            UIEatEvent(Evt);
            Moved = TRUE;

            switch (Evt->DeltaStride) {
                default: {
                    Moved = FALSE;
                } break;

                case UI_EVENT_DELTA_STRIDE_CHAR: {
                    for (Axis2D Axis = (Axis2D) 0; Axis < AXIS_2D_COUNT; Axis = (Axis2D) (Axis + 1)) {
                        CursorPosition.V[Axis] += Evt->DeltaI32.V[Axis];

                        if (CursorPosition.V[Axis] < Params->CursorRange.Min.V[Axis])
                            CursorPosition.V[Axis] = Params->CursorRange.Max.V[Axis];

                        if (CursorPosition.V[Axis] > Params->CursorRange.Max.V[Axis])
                            CursorPosition.V[Axis] = Params->CursorRange.Min.V[Axis];

                        CursorPosition.V[Axis] = Clamp(
                            Rng(
                                (i64) Params->CursorRange.Min.V[Axis], 
                                (i64) Params->CursorRange.Max.V[Axis]
                            ), 
                            CursorPosition.V[Axis]
                        );
                    }
                } break;

                case UI_EVENT_DELTA_STRIDE_WORD:
                case UI_EVENT_DELTA_STRIDE_LINE:
                case UI_EVENT_DELTA_STRIDE_PAGE: {
                    CursorPosition.X = ((Evt->DeltaI32.X > 0) ? Params->CursorRange.Max.X : (Evt->DeltaI32.X < 0) ? Params->CursorRange.Min.X + !!Params->CursorMinIsEmptySelection[AXIS_2D_X] : CursorPosition.X);
                    CursorPosition.Y = ((Evt->DeltaI32.Y > 0) ? + (NumPossibleVisibleRows - 3) : (Evt->DeltaI32.Y < 0) ? -(NumPossibleVisibleRows - 3) : 0);
                    CursorPosition.Y = Clamp(
                        Rng(
                            (i64) Params->CursorRange.Min.Y + !!Params->CursorMinIsEmptySelection[AXIS_2D_Y], 
                            (i64) Params->CursorRange.Max.Y
                        ), 
                        CursorPosition.Y
                    );
                } break;

                case UI_EVENT_DELTA_STRIDE_WHOLE: {
                    for (Axis2D Axis = (Axis2D) 0; Axis < AXIS_2D_COUNT; Axis = (Axis2D) (Axis + 1))
                        CursorPosition.V[Axis] = ((Evt->DeltaI32.V[Axis] > 0) ? Params->CursorRange.Max.V[Axis] : (Evt->DeltaI32.V[Axis] < 0) ? Params->CursorRange.Min.V[Axis] + !!Params->CursorMinIsEmptySelection[Axis] : CursorPosition.V[Axis]);
                } break;
            }

            if (!(Evt->Flags & UI_EVENT_FLAG_KEEP_MARK))
                MarkPosition = CursorPosition;
        }

        if (Moved) {
            *Cursor = CursorPosition;

            if (Mark)
                *Mark = MarkPosition;
        }
    }

    if (Params->Kind & UI_SCROLL_LIST_KIND_SNAP &&  Moved) {
        i64 CursorItemIndex = Cursor->Y - 1;

        if (Params->ItemRange.Min <= CursorItemIndex && CursorItemIndex <= Params->ItemRange.Max) {
            r1i64 VisibleRowRangePosition = Rng(
                ScrollPoint->Index + 0 - !!(ScrollPoint->Offset < 0),
                ScrollPoint->Index + 0 + NumPossibleVisibleRows + 1
            );
            r1i64 CursorVisibilityRowRange = {};

            if (!Params->RowBlocks.Count) {
                CursorVisibilityRowRange = Rng(CursorItemIndex - 1, CursorItemIndex + 3);
            } else {
                CursorVisibilityRowRange.Min = (i64) UIScrollListRowFromItem(&Params->RowBlocks, (u64) CursorItemIndex);
                CursorVisibilityRowRange.Max = CursorVisibilityRowRange.Min + 4;
            }

            i64 MinDelta = MIN(0, CursorVisibilityRowRange.Min - VisibleRowRangePosition.Min);
            i64 MaxDelta = MAX(0, CursorVisibilityRowRange.Max - VisibleRowRangePosition.Max);
            i64 NewIndex = ScrollPoint->Index + MinDelta + MaxDelta;

            NewIndex = Clamp(ScrollRowIndexRange, NewIndex);
            UIScrollPointTargetIndex(ScrollPoint, NewIndex);
        }
    }

    if (Signal)
        Signal->CursorMoved = Moved;

    r1i64 VisibleRowRangePosition = Rng(
        ScrollPoint->Index + (i64) (ScrollPoint->Offset) + 0 - !!(ScrollPoint->Offset < 0.0f),
        ScrollPoint->Index + (i64) (ScrollPoint->Offset) + 0 + NumPossibleVisibleRows + 1
    );

    VisibleRowRangePosition.Min = Clamp(Params->ItemRange, VisibleRowRangePosition.Min);
    VisibleRowRangePosition.Max = Clamp(Params->ItemRange, VisibleRowRangePosition.Max);
    *VisibleRowRange = VisibleRowRangePosition;
    GetTLS()->UIScrollListScrollBarDimensionsPX = UITailFontSize() * 1.5f;
    GetTLS()->UIScrollListScrollPointPtr = ScrollPoint;
    GetTLS()->UIScrollListDimensionsPX = Params->DimensionsPX;
    GetTLS()->UIScrollListScrollIndexRange = *(r1u64*) &ScrollRowIndexRange;

    UIBox* ContainerBox = EMPTY_UI_BOX_VALUE;

    UIFixedWidth(Params->DimensionsPX.X) {
        UIFixedHeight(Params->DimensionsPX.Y) {
            UIChildLayoutAxis(AXIS_2D_X) {
                ContainerBox = UIBuildBoxFromKey(0, {});
            }
        }
    }

    UIBox* ScrollableContainerBox = EMPTY_UI_BOX_VALUE;

    UIParent(ContainerBox) {
        UIChildLayoutAxis(AXIS_2D_Y) {
            UIFixedWidth(Params->DimensionsPX.X - GetTLS()->UIScrollListScrollBarDimensionsPX) {
                UIFixedHeight(Params->DimensionsPX.Y) {
                    UIBoxKind FadeKind = 0;

                    if (ScrollPoint->Index > Params->ItemRange.Min)
                        FadeKind |= UI_BOX_KIND_DRAW_FADE_TOP;

                    if (ScrollPoint->Index + NumPossibleVisibleRows < Params->ItemRange.Max)
                        FadeKind |= UI_BOX_KIND_DRAW_FADE_BOTTOM;

                    ScrollableContainerBox = UIBuildBoxFromStrFmt(
                        (
                            FadeKind |
                            UI_BOX_KIND_CLIP |
                            UI_BOX_KIND_ALLOW_OVERFLOW_Y |
                            UI_BOX_KIND_SCROLL
                        ),
                        (char*) "###sp"
                    );
                    ScrollableContainerBox->ViewOffset.Y = ScrollableContainerBox->ViewOffsetTarget.Y = Params->RowHeightPX * FMod(ScrollPoint->Offset, 1.0f) + Params->RowHeightPX * (ScrollPoint->Offset < 0.0f) - Params->RowHeightPX * (ScrollPoint->Offset == -1.0f && ScrollPoint->Index == 1);
                }
            }
        }
    }

    UIParent(ContainerBox) {
        UIFocus(UI_FOCUS_KIND_NULL) {
            UISetNextFixedWidth(GetTLS()->UIScrollListScrollBarDimensionsPX);
            UISetNextFixedHeight(GetTLS()->UIScrollListDimensionsPX.Y);

            *GetTLS()->UIScrollListScrollPointPtr = UIScrollBar(
                AXIS_2D_Y,
                UI_PX(GetTLS()->UIScrollListScrollBarDimensionsPX, 1.0f),
                *GetTLS()->UIScrollListScrollPointPtr,
                ScrollRowIndexRange,
                NumPossibleVisibleRows
            );
        }
    }

    UIPushParent(ContainerBox);
    UIPushParent(ScrollableContainerBox);
    UIPushPreferredHeight(UI_PX(Params->RowHeightPX, 1.0f));
}

void 
UIScrollListEnd(void)
{
    UIPopPreferredHeight();

    UIBox* ScrollableContainerBox = UIPopParent();
    UIBox* ContainerBox = UIPopParent();
    UISignal Sig = UISignalFromBox(ScrollableContainerBox);

    if (Sig.Scroll.Y != 0) {
        i64 NewIndex = GetTLS()->UIScrollListScrollPointPtr->Index + Sig.Scroll.Y;

        NewIndex = Clamp(*(r1i64*) &GetTLS()->UIScrollListScrollIndexRange, NewIndex);
        UIScrollPointTargetIndex(GetTLS()->UIScrollListScrollPointPtr, NewIndex);
    }

    UIScrollPointClampIndex(
        GetTLS()->UIScrollListScrollPointPtr, 
        *(r1i64*) &GetTLS()->UIScrollListScrollIndexRange
    );
}

UISignal 
UICheckBox(b32* Checked, Str8 String)
{
    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_HOT_EFFECTS,
        String
    );

    UIParent(Box) {
        UIPreferredWidth(UI_EM(1.4f, 1.0f)) {
            UIPreferredHeight(UI_EM(1.4f, 1.0f)) {
                UICornerRadius(UIHeadFontSize() * 0.2f) {
                    UIBox* CheckBox = UIBuildBoxFromStr(
                        UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_BORDER,
                        "###check_box"_s8
                    );

                    if (*Checked) {
                        UIParent(CheckBox) {
                            UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                UIFont(UIIconFont()) {
                                    UIBox* CheckMark = UIBuildBoxFromStr(
                                        UI_BOX_KIND_DRAW_TEXT,
                                        "###check_mark"_s8
                                    );

                                    UIBoxEquipDisplayStr(
                                        CheckMark,
                                        UIIconStringFromIconKind(UI_ICON_KIND_CHECK_FILLED)
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }

        UISpacer(UI_EM(0.5f, 1.0f));

        Str8 DisplayString = UIDisplayPartFromKeyStr(String);

        if (DisplayString.Size)
            UILabel(DisplayString);
    }

    UISignal Sig = UISignalFromBox(Box);

    if (UI_CLICKED(Sig))
        *Checked = !(*Checked);

    UIPopHoverCursor();

    return Sig;
}

UISignal 
UICheckBox(b32* Checked, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Sig = UICheckBox(Checked, String);

    ReleaseScratch(Scratch);

    return Sig;
}

UISignal 
UIToggle(b32* Enabled, UIToggleKind Kind, Str8 String)
{
    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_HOT_EFFECTS,
        String
    );

    UIParent(Box) {
        if (Kind == UI_TOGGLE_KIND_CHECKBOX) {
            UIPreferredWidth(UI_EM(1.4f, 1.0f)) {
                UIPreferredHeight(UI_EM(1.4f, 1.0f)) {
                    UICornerRadius(UIHeadFontSize() * 0.2f) {
                        UIBox* CheckBox = UIBuildBoxFromStr(
                            UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_BORDER,
                            "###toggle_box"_s8
                        );

                        if (*Enabled) {
                            UIParent(CheckBox) {
                                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                                    UIFont(UIIconFont()) {
                                        UIBox* CheckMark = UIBuildBoxFromStr(
                                            UI_BOX_KIND_DRAW_TEXT,
                                            "###check_mark"_s8
                                        );

                                        UIBoxEquipDisplayStr(
                                            CheckMark,
                                            UIIconStringFromIconKind(
                                                UI_ICON_KIND_CHECK_FILLED
                                            )
                                        );
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if (Kind == UI_TOGGLE_KIND_SWITCH) {
            UIPreferredWidth(UI_EM(2.8f, 1.0f)) {
                UIPreferredHeight(UI_EM(1.4f, 1.0f)) {
                    UIKey AnimKey = UIKeyFromStr(
                        UIActiveSeedKey(),
                        "###toggle_anim"_s8
                    );
                    f32 EnabledT = UIAnimate(AnimKey, *Enabled ? 1.0f : 0.0f);
                    UIBox* SwitchBox = UIBuildBoxFromStr(
                        0,
                        "###toggle_switch"_s8
                    );
                    UIToggleSwitchDrawData* DrawData = ArenaPushArrayZero(
                        UIBuildMemPool(),
                        UIToggleSwitchDrawData,
                        1
                    );

                    DrawData->EnabledT = EnabledT;
                    UIBoxEquipCustomDraw(
                        SwitchBox,
                        UIToggleSwitchDraw,
                        DrawData
                    );
                }
            }
        }

        UISpacer(UI_EM(0.5f, 1.0f));

        Str8 DisplayString = UIDisplayPartFromKeyStr(String);

        if (DisplayString.Size)
            UILabel(DisplayString);
    }

    UISignal Sig = UISignalFromBox(Box);

    if (UI_CLICKED(Sig))
        *Enabled = !(*Enabled);

    UIPopHoverCursor();

    return Sig;
}

UISignal 
UIToggle(b32* Enabled, UIToggleKind Kind, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Sig = UIToggle(Enabled, Kind, String);

    ReleaseScratch(Scratch);

    return Sig;
}

UISliderResult 
UISlider(f32* Value, r1f32 Range, UISliderKind Kind, Str8 String)
{
    UISliderResult Result = {};
    f32 OriginalValue = *Value;
    Axis2D Axis = (Kind == UI_SLIDER_KIND_HORZ) ? AXIS_2D_X : AXIS_2D_Y;

    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);

    UIBox* Box = UIBuildBoxFromStr(UI_BOX_KIND_CLICKABLE, String);

    UISliderDrawData* DrawData = ArenaPushArrayZero(UIBuildMemPool(), UISliderDrawData, 1);

    DrawData->Value = *Value;
    DrawData->Range = Range;
    DrawData->Kind = Kind;
    UIBoxEquipCustomDraw(Box, UISliderDraw, DrawData);

    UISignal Sig = UISignalFromBox(Box);

    if (UI_DRAGGING(Sig)) {
        v2f32 BoxDim = Length(Box->Rect);
        f32 KnobRadius = BoxDim.V[FLIP_AXIS(Axis)] * 0.4f;
        f32 TrackStart = Box->Rect.Point0.V[Axis] + KnobRadius;
        f32 TrackEnd = Box->Rect.Point1.V[Axis] - KnobRadius;
        f32 TrackLen = TrackEnd - TrackStart;

        if (TrackLen > 0.0f) {
            f32 MousePos = UIMouse().V[Axis];
            f32 NormPos = (MousePos - TrackStart) / TrackLen;

            NormPos = CLAMP(0.0f, NormPos, 1.0f);

            if (Axis == AXIS_2D_Y)
                NormPos = 1.0f - NormPos;

            *Value = LerpF32(NormPos, Range.Min, Range.Max);
        }

        if (UI_PRESSED(Sig))
            UIStoreDragStruct(&OriginalValue);

        if (UISlotPress(UI_EVENT_ACTION_SLOT_CANCEL)) {
            f32* StoredValue = UIGetDragStruct(f32);

            *Value = *StoredValue;
            UIKillAction();
        }
    }

    DrawData->Value = *Value;
    Result.Sig = Sig;
    Result.Value = *Value;
    Result.Changed = (*Value != OriginalValue);

    UIPopHoverCursor();

    return Result;
}

UISliderResult 
UISlider(f32* Value, r1f32 Range, UISliderKind Kind, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISliderResult Result = UISlider(Value, Range, Kind, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISliderResult 
UISlider(i32* Value, r1i32 Range, UISliderKind Kind, Str8 String)
{
    f32 FloatValue = (f32) *Value;
    UISliderResult Result = UISlider(
        &FloatValue, 
        Rng((f32) Range.Min, (f32) Range.Max), 
        Kind, 
        String
    );

    *Value = (i32) RoundF32(FloatValue);
    Result.Value = (f32) *Value;

    return Result;
}

UISliderResult 
UISlider(i32* Value, r1i32 Range, UISliderKind Kind, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISliderResult Result = UISlider(Value, Range, Kind, String);

    ReleaseScratch(Scratch);

    return Result;
}

UIDropDownResult 
UIDropDown(i64* SelectedIndex, Str8Array Options, Str8 String)
{
    UIDropDownResult Result = {};
    UIKey Key = UIKeyFromStr(UIActiveSeedKey(), String);
    UIKey MenuKey = UIKeyFromStr(Key, "###dropdown_menu"_s8);
    b32 MenuOpen = UIContextMenuIsOpen(MenuKey);

    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(
        (
            UI_BOX_KIND_CLICKABLE |
            UI_BOX_KIND_DRAW_BACKGROUND |
            UI_BOX_KIND_DRAW_BORDER |
            UI_BOX_KIND_DRAW_HOT_EFFECTS |
            UI_BOX_KIND_DRAW_ACTIVE_EFFECTS
        ),
        String
    );

    UIParent(Box) {
        UIWidthFill() {
            if (*SelectedIndex >= 0 && (u64) *SelectedIndex < Options.Count)
                UILabel(Options.Data[*SelectedIndex]);
            else
                UILabel("Select..."_s8);
        }

        UISpacer(UI_EM(0.5f, 1.0f));

        UIFont(UIIconFont()) {
            UIPreferredWidth(UI_EM(1.2f, 1.0f)) {
                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                    UIBox* Arrow = UIBuildBoxFromStr(UI_BOX_KIND_DRAW_TEXT, "###arrow"_s8);

                    UIBoxEquipDisplayStr(Arrow, MenuOpen ? 
                        UIIconStringFromIconKind(UI_ICON_KIND_UP_CARET) : 
                        UIIconStringFromIconKind(UI_ICON_KIND_DOWN_CARET)
                    );
                }
            }
        }
    }

    UISignal Sig = UISignalFromBox(Box);

    if (UI_CLICKED(Sig)) {
        if (MenuOpen)
            UIContextMenuClose();
        else
            UIContextMenuOpen(MenuKey, Box->Key, Vec(0.0f, Length(Box->Rect).Y));
    }

    UIContextMenu(MenuKey) {
        UIPreferredWidth(UI_PX(Length(Box->Rect).X, 1.0f)) {
            for (u64 I = 0; I < Options.Count; ++I) {
                UISignal ItemSig = UIButton("%S###item_%llu", PRINT_STR(Options.Data[I]), I);

                if (UI_CLICKED(ItemSig)) {
                    if (*SelectedIndex != (i64) I) {
                        Result.Changed = TRUE;
                        *SelectedIndex = (i64) I;
                    }

                    UIContextMenuClose();
                }
            }
        }
    }

    Result.Sig = Sig;
    Result.SelectedIndex = *SelectedIndex;

    UIPopHoverCursor();

    return Result;
}

UIDropDownResult 
UIDropDown(i64* SelectedIndex, Str8Array Options, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIDropDownResult Result = UIDropDown(SelectedIndex, Options, String);

    ReleaseScratch(Scratch);

    return Result;
}

UIDropDownResult 
UIComboBox(i64* SelectedIndex, Str8Array Options, b32* IsOpen, Str8 String)
{
    UIDropDownResult Result = UIDropDown(SelectedIndex, Options, String);

    *IsOpen = UIContextMenuIsOpen(
        UIKeyFromStr(
            UIKeyFromStr(
                UIActiveSeedKey(), 
                String
            ), 
            "###dropdown_menu"_s8
        )
    );

    return Result;
}

UIDropDownResult 
UIComboBox(i64* SelectedIndex, Str8Array Options, b32* IsOpen, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIDropDownResult Result = UIComboBox(
        SelectedIndex, 
        Options, 
        IsOpen, 
        String
    );

    ReleaseScratch(Scratch);

    return Result;
}

UINumericEditResult 
UINumericEdit(f64* Value, r1f64 Range, f64 Step, Str8 String)
{
    UINumericEditResult Result = {};
    f64 OriginalValue = *Value;

    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* ContainerBox = UIBuildBoxFromStr(
        UI_BOX_KIND_DRAW_BACKGROUND | UI_BOX_KIND_DRAW_BORDER,
        String
    );

    TextPoint Cursor = TxtPt(1, 1);
    TextPoint Mark = Cursor;
    u8 EditBuffer[64] = {};
    u64 EditStrSize = 0;

    TempArena Scratch = GetScratch(NULL, 0);
    Str8 ValueStr = ArenaPushStrFmt(Scratch.MemPool, "%.4lf", *Value);

    MemCpy(EditBuffer, ValueStr.Str, MIN(ValueStr.Size, sizeof(EditBuffer) - 1));
    EditStrSize = MIN(ValueStr.Size, sizeof(EditBuffer) - 1);
    Cursor.Column = (i64) EditStrSize + 1;

    UIParent(ContainerBox) {
        UIPreferredWidth(UI_EM(1.2f, 1.0f)) {
            UIPushHoverCursor(CURSOR_KIND_HAND_POINT);
            UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);
            UISetNextFont(UIIconFont());

            UISignal DecrSig = UIButton("%S###decr", PRINT_STR(UIIconStringFromIconKind(UI_ICON_KIND_LEFT_CARET)));

            if (UI_CLICKED(DecrSig)) {
                *Value = CLAMP(Range.Min, *Value - Step, Range.Max);
                Result.Changed = TRUE;
            }

            UIPopHoverCursor();
        }

        UIWidthFill() {
            UISignal EditSig = UILineEdit(
                &Cursor, 
                &Mark, 
                EditBuffer, 
                sizeof(EditBuffer), 
                &EditStrSize, 
                ValueStr, 
                "###edit"_s8
            );

            if (UI_COMMITTED(EditSig)) {
                Str8 EditStr = Str(EditBuffer, EditStrSize);
                f64 ParsedValue = 0.0;

                if (EditStr.Size > 0) {
                    ParsedValue = F64FromStr(StrPrefix(EditStr, 64));
                }

                *Value = CLAMP(Range.Min, ParsedValue, Range.Max);
                Result.Committed = TRUE;
                Result.Changed = (*Value != OriginalValue);
            }

            Result.Sig = EditSig;
        }

        UIPreferredWidth(UI_EM(1.2f, 1.0f)) {
            UIPushHoverCursor(CURSOR_KIND_HAND_POINT);
            UISetNextTextAlignment(UI_TEXT_ALIGN_CENTRE);
            UISetNextFont(UIIconFont());

            UISignal IncrSig = UIButton("%S###incr", PRINT_STR(UIIconStringFromIconKind(UI_ICON_KIND_RIGHT_CARET)));

            if (UI_CLICKED(IncrSig)) {
                *Value = CLAMP(Range.Min, *Value + Step, Range.Max);
                Result.Changed = TRUE;
            }

            UIPopHoverCursor();
        }
    }

    ReleaseScratch(Scratch);

    Result.Value = *Value;

    return Result;
}

UINumericEditResult 
UINumericEdit(f64* Value, r1f64 Range, f64 Step, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UINumericEditResult Result = UINumericEdit(Value, Range, Step, String);

    ReleaseScratch(Scratch);

    return Result;
}

UINumericEditResult 
UINumericEdit(i64* Value, r1i64 Range, i64 Step, Str8 String)
{
    f64 FloatValue = (f64) *Value;
    UINumericEditResult Result = UINumericEdit(
        &FloatValue, 
        Rng(
            (f64) Range.Min, 
            (f64) Range.Max
        ),
        (f64) Step, 
        String
    );

    *Value = (i64) RoundF64(FloatValue);
    Result.Value = (f64) *Value;

    return Result;
}

UINumericEditResult 
UINumericEdit(i64* Value, r1i64 Range, i64 Step, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UINumericEditResult Result = UINumericEdit(Value, Range, Step, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UIProgressBar(f32 Progress, Str8 String)
{
    UIBox* Box = UIBuildBoxFromStr(UI_BOX_KIND_DRAW_BORDER, String);

    UIProgressBarDrawData* DrawData = ArenaPushArrayZero(
        UIBuildMemPool(), 
        UIProgressBarDrawData, 
        1
    );

    DrawData->Progress = Progress;
    UIBoxEquipCustomDraw(Box, UIProgressBarDraw, DrawData);

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UIProgressBar(f32 Progress, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIProgressBar(Progress, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UIProgressSpinner(f32 SpinRate, Str8 String)
{
    UIKey Key = UIKeyFromStr(UIActiveSeedKey(), String);
    UIKey AngleKey = UIKeyFromStr(Key, "###spinner_angle"_s8);
    f32 DeltaTime = UIDeltaTime();
    f32 Angle = UIAnimate(AngleKey, DeltaTime * SpinRate * 2.0f * NC_PI32, __Params.Rate = 1.0f);

    UIBox* Box = UIBuildBoxFromStr(0, String);

    UIProgressSpinnerDrawData* DrawData = ArenaPushArrayZero(UIBuildMemPool(), UIProgressSpinnerDrawData, 1);

    DrawData->Angle = Angle;
    DrawData->SpinRate = SpinRate;
    UIBoxEquipCustomDraw(Box, UIProgressSpinnerDraw, DrawData);

    UISignal Result = UISignalFromBox(Box);

    return Result;
}

UISignal 
UIProgressSpinner(f32 SpinRate, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIProgressSpinner(SpinRate, String);

    ReleaseScratch(Scratch);

    return Result;
}

b32 
UITreeNodeBegin(b32* IsExpanded, b32 HasChildren, Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();

    ++TLS->UITreeNodeDepth;

    f32 IndentSize = UIHeadFontSize() * 1.0f * (f32) (TLS->UITreeNodeDepth - 1);

    UISetNextChildLayoutAxis(AXIS_2D_Y);

    UIBox* ContainerBox = UIBuildBoxFromStr(0, String);

    UIPushParent(ContainerBox);

    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* HeaderBox = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_HOT_EFFECTS,
        "###header"_s8
    );

    UIParent(HeaderBox) {
        UISpacer(UI_PX(IndentSize, 1.0f));

        if (HasChildren) {
            UIPreferredWidth(UI_EM(1.4f, 1.0f)) {
                UIPreferredHeight(UI_EM(1.4f, 1.0f)) {
                    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);

                    UISignal ExpanderSig = UIExpander(*IsExpanded, "###expander"_s8);

                    if (UI_CLICKED(ExpanderSig))
                        *IsExpanded = !(*IsExpanded);

                    UIPopHoverCursor();
                }
            }
        } else {
            UISpacer(UI_EM(1.4f, 1.0f));
        }

        UISpacer(UI_EM(0.3f, 1.0f));

        UILabel(UIDisplayPartFromKeyStr(String));
    }

    UISignal HeaderSig = UISignalFromBox(HeaderBox);

    if (HasChildren && UI_CLICKED(HeaderSig))
        *IsExpanded = !(*IsExpanded);

    if (HasChildren && *IsExpanded) {
        UISetNextChildLayoutAxis(AXIS_2D_Y);

        UIBox* ChildContainer = UIBuildBoxFromStr(0, "###children"_s8);

        UIPushParent(ChildContainer);

        return TRUE;
    }

    return FALSE;
}

b32 
UITreeNodeBegin(b32* IsExpanded, b32 HasChildren, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    b32 Result = UITreeNodeBegin(IsExpanded, HasChildren, String);

    ReleaseScratch(Scratch);

    return Result;
}

void 
UITreeNodeEnd(void)
{
    ThreadLocalStorage* TLS = GetTLS();

    UIPopParent();
    --TLS->UITreeNodeDepth;
}

void 
UITreeNodeLeaf(Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();
    f32 IndentSize = UIHeadFontSize() * 1.0f * (f32) TLS->UITreeNodeDepth;

    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_HOT_EFFECTS, 
        String
    );

    UIParent(Box) {
        UISpacer(UI_PX(IndentSize, 1.0f));
        UISpacer(UI_EM(1.4f, 1.0f));
        UISpacer(UI_EM(0.3f, 1.0f));
        UILabel(UIDisplayPartFromKeyStr(String));
    }
}

void 
UITreeNodeLeaf(char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);
    UITreeNodeLeaf(String);
    ReleaseScratch(Scratch);
}

void 
UIRadioGroupBegin(i64* SelectedIndex, i64 OptionCount, Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();

    TLS->UIRadioGroupSelectedIndex = SelectedIndex;
    TLS->UIRadioGroupOptionCount = OptionCount;

    UISetNextChildLayoutAxis(AXIS_2D_Y);

    UIBox* Box = UIBuildBoxFromStr(0, String);

    UIPushParent(Box);
}

void 
UIRadioGroupBegin(i64* SelectedIndex, i64 OptionCount, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIRadioGroupBegin(SelectedIndex, OptionCount, String);
    ReleaseScratch(Scratch);
}

UISignal 
UIRadioOption(i64 Index, Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();

    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);
    UISetNextChildLayoutAxis(AXIS_2D_X);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_CLICKABLE | UI_BOX_KIND_DRAW_HOT_EFFECTS,
        String
    );

    b32 IsSelected = (
        TLS->UIRadioGroupSelectedIndex && 
        *TLS->UIRadioGroupSelectedIndex == Index
    );

    UIParent(Box) {
        UIPreferredWidth(UI_EM(1.4f, 1.0f)) {
            UIPreferredHeight(UI_EM(1.4f, 1.0f)) {
                UIKey AnimKey = UIKeyFromStr(Box->Key, "###radio_anim"_s8);
                f32 SelectedT = UIAnimate(AnimKey, IsSelected ? 1.0f : 0.0f);

                UIBox* RadioButton = UIBuildBoxFromStr(0, "###radio"_s8);

                UIRadioButtonDrawData* DrawData = ArenaPushArrayZero(
                    UIBuildMemPool(),
                    UIRadioButtonDrawData,
                    1
                );

                DrawData->SelectedT = SelectedT;
                UIBoxEquipCustomDraw(RadioButton, UIRadioButtonDraw, DrawData);
            }
        }

        UISpacer(UI_EM(0.5f, 1.0f));
        UILabel(UIDisplayPartFromKeyStr(String));
    }

    UISignal Sig = UISignalFromBox(Box);

    if (UI_CLICKED(Sig) && TLS->UIRadioGroupSelectedIndex)
        *TLS->UIRadioGroupSelectedIndex = Index;

    UIPopHoverCursor();

    return Sig;
}

UISignal 
UIRadioOption(i64 Index, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIRadioOption(Index, String);

    ReleaseScratch(Scratch);

    return Result;
}

void 
UIRadioGroupEnd(void)
{
    ThreadLocalStorage* TLS = GetTLS();

    UIPopParent();
    TLS->UIRadioGroupSelectedIndex = NULL;
    TLS->UIRadioGroupOptionCount = 0;
}

UIBox* 
UIBreadCrumbBegin(i64 Count, Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();

    TLS->UIBreadCrumbCount = Count;
    TLS->UIBreadCrumbCurrentIndex = 0;

    UISetNextChildLayoutAxis(AXIS_2D_X);

    TLS->UIBreadCrumbContainer = UIBuildBoxFromStr(0, String);

    UIPushParent(TLS->UIBreadCrumbContainer);

    return TLS->UIBreadCrumbContainer;
}

UIBox* 
UIBreadCrumbBegin(i64 Count, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIBox* Result = UIBreadCrumbBegin(Count, String);

    ReleaseScratch(Scratch);

    return Result;
}

UISignal 
UIBreadCrumbItem(i64 Index, Str8 String)
{
    ThreadLocalStorage* TLS = GetTLS();

    UIPushHoverCursor(CURSOR_KIND_HAND_POINT);

    b32 IsLast = (Index == TLS->UIBreadCrumbCount - 1);

    UIBox* Box = UIBuildBoxFromStr(
        (
            UI_BOX_KIND_CLICKABLE |
            UI_BOX_KIND_DRAW_TEXT |
            UI_BOX_KIND_DRAW_HOT_EFFECTS |
            (IsLast ? 0 : UI_BOX_KIND_DRAW_TEXT_WEAK)
        ),
        String
    );

    UIBoxEquipDisplayStr(Box, UIDisplayPartFromKeyStr(String));

    UISignal Sig = UISignalFromBox(Box);

    if (!IsLast) {
        UISpacer(UI_EM(0.4f, 1.0f));

        UIFont(UIIconFont()) {
            UIPreferredWidth(UI_EM(0.8f, 1.0f)) {
                UITextAlignment(UI_TEXT_ALIGN_CENTRE) {
                    UIBox* Separator = UIBuildBoxFromStr(
                        UI_BOX_KIND_DRAW_TEXT, 
                        "###sep"_s8
                    );

                    UIBoxEquipDisplayStr(
                        Separator, 
                        UIIconStringFromIconKind(
                            UI_ICON_KIND_RIGHT_CARET
                        )
                    );
                }
            }
        }

        UISpacer(UI_EM(0.4f, 1.0f));
    }

    ++TLS->UIBreadCrumbCurrentIndex;

    UIPopHoverCursor();

    return Sig;
}

UISignal 
UIBreadCrumbItem(i64 Index, char* Fmt, ...)
{
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UISignal Result = UIBreadCrumbItem(Index, String);

    ReleaseScratch(Scratch);

    return Result;
}

void 
UIBreadCrumbEnd(void)
{
    ThreadLocalStorage* TLS = GetTLS();

    UIPopParent();
    TLS->UIBreadCrumbContainer = NULL;
    TLS->UIBreadCrumbCount = 0;
    TLS->UIBreadCrumbCurrentIndex = 0;
}

UIRadialMenuResult 
UIRadialMenu(
    v2f32 MenuCentre, 
    f32 InnerRadius, 
    f32 OuterRadius, 
    UIRadialMenuItem* Items, 
    u64 ItemCount, 
    Str8 String
) {
    UIRadialMenuResult Result = {};
    
    Result.HoveredIndex = -1;

    if (ItemCount == 0)
        return Result;

    f32 Size = OuterRadius * 2.0f;

    UISetNextFixedX(MenuCentre.X - OuterRadius);
    UISetNextFixedY(MenuCentre.Y - OuterRadius);
    UISetNextFixedWidth(Size);
    UISetNextFixedHeight(Size);

    UIBox* Box = UIBuildBoxFromStr(
        UI_BOX_KIND_FLOATING | UI_BOX_KIND_FIXED_SIZE | UI_BOX_KIND_CLICKABLE,
        String
    );

    UISignal Sig = UISignalFromBox(Box);

    v2f32 Mouse = UIMouse();
    v2f32 Delta = Vec(Mouse.X - MenuCentre.X, Mouse.Y - MenuCentre.Y);
    f32 Distance = SqrtF32(Delta.X * Delta.X + Delta.Y * Delta.Y);

    if (Distance >= InnerRadius && Distance <= OuterRadius) {
        f32 Angle = ArcTan2(Delta.Y, Delta.X);
        f32 AngleStep = 2.0f * NC_PI32 / (f32) ItemCount;
        f32 StartAngle = -NC_PI32 * 0.5f - AngleStep * 0.5f;
        f32 NormAngle = Angle - StartAngle;

        if (NormAngle < 0.0f)
            NormAngle += 2.0f * NC_PI32;

        i64 HoveredIndex = (i64) (NormAngle / AngleStep);

        if (HoveredIndex >= 0 && (u64) HoveredIndex < ItemCount)
            Result.HoveredIndex = HoveredIndex;
    }

    if (UI_CLICKED(Sig) && Result.HoveredIndex >= 0 && Items[Result.HoveredIndex].Enabled) {
        Result.Committed = TRUE;
        Result.SelectedIndex = Result.HoveredIndex;
    }

    UIKey HoverKey = UIKeyFromStr(Box->Key, "###hover"_s8);
    f32 HoverT = UIAnimate(HoverKey, Result.HoveredIndex >= 0 ? 1.0f : 0.0f);

    UIRadialMenuDrawData* DrawData = ArenaPushArrayZero(UIBuildMemPool(), UIRadialMenuDrawData, 1);

    DrawData->MenuCentre = MenuCentre;
    DrawData->InnerRadius = InnerRadius;
    DrawData->OuterRadius = OuterRadius;
    DrawData->Items = Items;
    DrawData->ItemCount = ItemCount;
    DrawData->HoveredIndex = Result.HoveredIndex;
    DrawData->HoverT = HoverT;
    UIBoxEquipCustomDraw(Box, UIRadialMenuDraw, DrawData);

    Result.Sig = Sig;

    return Result;
}

UIRadialMenuResult 
UIRadialMenu(
    v2f32 MenuCentre, 
    f32 InnerRadius, 
    f32 OuterRadius, 
    UIRadialMenuItem* Items, 
    u64 ItemCount, 
    char* Fmt, 
    ...
) {
    TempArena Scratch = GetScratch(NULL, 0);
    va_list Args;

    va_start(Args, Fmt);

    Str8 String = ArenaPushStrFmtV(Scratch.MemPool, Fmt, Args);

    va_end(Args);

    UIRadialMenuResult Result = UIRadialMenu(
        MenuCentre, 
        InnerRadius, 
        OuterRadius, 
        Items, 
        ItemCount, 
        String
    );

    ReleaseScratch(Scratch);

    return Result;
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UISliderDraw)
{
    UISliderDrawData* Data = (UISliderDrawData*) UserData;
    Axis2D Axis = (Data->Kind == UI_SLIDER_KIND_HORZ) ? AXIS_2D_X : AXIS_2D_Y;
    Axis2D CrossAxis = FLIP_AXIS(Axis);
    v2f32 BoxDim = Length(Box->Rect);
    f32 NormValue = (Data->Range.Max > Data->Range.Min)
        ? (Data->Value - Data->Range.Min) / (Data->Range.Max - Data->Range.Min)
        : 0.0f;

    NormValue = CLAMP(0.0f, NormValue, 1.0f);

    if (Axis == AXIS_2D_X)
        NormValue = 1.0f - NormValue;

    f32 TrackThickness = BoxDim.V[CrossAxis] * 0.3f;
    f32 TrackOffset = (BoxDim.V[CrossAxis] - TrackThickness) * 0.5f;
    f32 KnobRadius = BoxDim.V[CrossAxis] * 0.4f;
    f32 TrackPadding = KnobRadius;
    r2f32 TrackRect = {};

    if (Axis == AXIS_2D_X) {
        TrackRect = Rng(
            Box->Rect.X0 + TrackPadding,
            Box->Rect.Y0 + TrackOffset,
            Box->Rect.X1 - TrackPadding,
            Box->Rect.Y0 + TrackOffset + TrackThickness
        );
    } else {
        TrackRect = Rng(
            Box->Rect.X0 + TrackOffset,
            Box->Rect.Y0 + TrackPadding,
            Box->Rect.X0 + TrackOffset + TrackThickness,
            Box->Rect.Y1 - TrackPadding
        );
    }

    v4f32 TrackColour = UIColourFromTagsKeyName(Box->TagsKey, "background"_s8);

    DrawRect(
        TrackRect,
        TrackColour,
        TrackThickness * 0.5f,
        0.0f,
        1.0f
    );
    
    r2f32 FillRect = TrackRect;

    if (Axis == AXIS_2D_X)
        FillRect.X1 = LerpF32(NormValue, TrackRect.X0, TrackRect.X1);
    else
        FillRect.Y0 = LerpF32(1.0f - NormValue, TrackRect.Y1, TrackRect.Y0);

    v4f32 FillColour = UIColourFromTagsKeyName(Box->TagsKey, "accent"_s8);

    DrawRect(
        FillRect,
        FillColour,
        TrackThickness * 0.5f,
        0.0f,
        1.0f
    );

    v2f32 KnobCentre = {};

    if (Axis == AXIS_2D_X) {
        KnobCentre.X = LerpF32(NormValue, Box->Rect.X0 + KnobRadius, Box->Rect.X1 - KnobRadius);
        KnobCentre.Y = Box->Rect.Y0 + BoxDim.Y * 0.5f;
    } else {
        KnobCentre.X = Box->Rect.X0 + BoxDim.X * 0.5f;
        KnobCentre.Y = LerpF32(NormValue, Box->Rect.Y0 + KnobRadius, Box->Rect.Y1 - KnobRadius);
    }

    f32 KnobScale = 1.0f + Box->Active * 0.15f;
    f32 ScaledRadius = KnobRadius * KnobScale;
    r2f32 KnobRect = Rng(
        KnobCentre.X - ScaledRadius,
        KnobCentre.Y - ScaledRadius,
        KnobCentre.X + ScaledRadius,
        KnobCentre.Y + ScaledRadius
    );
    v4f32 KnobColour = UIColourFromTagsKeyName(Box->TagsKey, "text"_s8);

    DrawRect(
        KnobRect,
        KnobColour,
        ScaledRadius,
        0.0f,
        1.0f
    );
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIToggleSwitchDraw)
{
    UIToggleSwitchDrawData* Data = (UIToggleSwitchDrawData*) UserData;
    v2f32 BoxDim = Length(Box->Rect);
    f32 TrackHeight = BoxDim.Y * 0.6f;
    f32 TrackY0 = Box->Rect.Y0 + (BoxDim.Y - TrackHeight) * 0.5f;
    f32 KnobRadius = BoxDim.Y * 0.4f;
    f32 KnobPadding = BoxDim.Y * 0.1f;
    f32 TrackPadding = KnobRadius + KnobPadding;
    r2f32 TrackRect = Rng(
        Box->Rect.X0 + TrackPadding * 0.5f,
        TrackY0,
        Box->Rect.X1 - TrackPadding * 0.5f,
        TrackY0 + TrackHeight
    );
    v4f32 TrackColourOff = UIColourFromTagsKeyName(Box->TagsKey, "background"_s8);
    v4f32 TrackColourOn = UIColourFromTagsKeyName(Box->TagsKey, "accent"_s8);
    v4f32 TrackColour = Vec(
        LerpF32(Data->EnabledT, TrackColourOff.X, TrackColourOn.X),
        LerpF32(Data->EnabledT, TrackColourOff.Y, TrackColourOn.Y),
        LerpF32(Data->EnabledT, TrackColourOff.Z, TrackColourOn.Z),
        LerpF32(Data->EnabledT, TrackColourOff.W, TrackColourOn.W)
    );

    DrawRect(
        TrackRect,
        TrackColour,
        TrackHeight * 0.5f,
        0.0f,
        1.0f
    );

    f32 KnobMinX = Box->Rect.X0 + KnobRadius + KnobPadding;
    f32 KnobMaxX = Box->Rect.X1 - KnobRadius - KnobPadding;
    f32 KnobX = LerpF32(Data->EnabledT, KnobMinX, KnobMaxX);
    f32 KnobY = Box->Rect.Y0 + BoxDim.Y * 0.5f;
    r2f32 KnobRect = Rng(
        KnobX - KnobRadius,
        KnobY - KnobRadius,
        KnobX + KnobRadius,
        KnobY + KnobRadius
    );
    v4f32 KnobColour = UIColourFromTagsKeyName(Box->TagsKey, "text"_s8);

    DrawRect(
        KnobRect,
        KnobColour,
        KnobRadius,
        0.0f,
        1.0f
    );
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIProgressBarDraw)
{
    UIProgressBarDrawData* Data = (UIProgressBarDrawData*) UserData;
    v2f32 BoxDim = Length(Box->Rect);
    f32 Progress = CLAMP(0.0f, Data->Progress, 1.0f);
    f32 CornerRadius = BoxDim.Y * 0.25f;
    
    v4f32 TrackColour = UIColourFromTagsKeyName(Box->TagsKey, "background"_s8);

    DrawRect(
        Widen(Box->Rect, -1.0f),
        TrackColour,
        CornerRadius,
        0.0f,
        1.0f
    );
    
    r2f32 FillRect = Widen(Box->Rect, -2.0f);

    FillRect.X1 = LerpF32(Progress, FillRect.X0, FillRect.X1);

    if (FillRect.X1 > FillRect.X0) {
        v4f32 FillColour = UIColourFromTagsKeyName(Box->TagsKey, "accent"_s8);

        DrawRect(
            FillRect,
            FillColour,
            CornerRadius - 1.0f,
            0.0f,
            1.0f
        );
    }
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIProgressSpinnerDraw)
{
    UIProgressSpinnerDrawData* Data = (UIProgressSpinnerDrawData*) UserData;
    v2f32 BoxDim = Length(Box->Rect);
    v2f32 Centre = Vec(
        Box->Rect.X0 + BoxDim.X * 0.5f,
        Box->Rect.Y0 + BoxDim.Y * 0.5f
    );
    f32 Radius = MIN(BoxDim.X, BoxDim.Y) * 0.4f;
    f32 Thickness = Radius * 0.2f;
    v4f32 Colour = UIColourFromTagsKeyName(Box->TagsKey, "accent"_s8);
    
    u32 SegmentCount = 12;
    f32 ArcLength = 0.7f;

    for (u32 I = 0; I < SegmentCount; ++I) {
        f32 SegmentT = (f32) I / (f32) SegmentCount;
        f32 Angle0 = Data->Angle + SegmentT * 2.0f * NC_PI32 * ArcLength;
        f32 Angle1 = Data->Angle + (SegmentT + 1.0f / (f32) SegmentCount) * 2.0f * NC_PI32 * ArcLength;
        f32 Alpha = 1.0f - SegmentT * 0.7f;
        v4f32 SegmentColour = Vec(Colour.X, Colour.Y, Colour.Z, Colour.W * Alpha);
        v2f32 P0 = Vec(Centre.X + Cos(Angle0) * Radius, Centre.Y + Sin(Angle0) * Radius);
        v2f32 P1 = Vec(Centre.X + Cos(Angle1) * Radius, Centre.Y + Sin(Angle1) * Radius);
        r2f32 SegmentRect = Rng(
            MIN(P0.X, P1.X) - Thickness * 0.5f,
            MIN(P0.Y, P1.Y) - Thickness * 0.5f,
            MAX(P0.X, P1.X) + Thickness * 0.5f,
            MAX(P0.Y, P1.Y) + Thickness * 0.5f
        );

        DrawRect(
            SegmentRect,
            SegmentColour,
            Thickness * 0.5f,
            0.0f,
            1.0f
        );
    }
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIRadialMenuDraw)
{
    UIRadialMenuDrawData* Data = (UIRadialMenuDrawData*) UserData;
    
    if (Data->ItemCount == 0)
        return;

    f32 AngleStep = 2.0f * NC_PI32 / (f32) Data->ItemCount;
    f32 StartAngle = -NC_PI32 * 0.5f - AngleStep * 0.5f;
    
    v4f32 BaseColour = UIColourFromTagsKeyName(Box->TagsKey, "background"_s8);
    v4f32 HoverColour = UIColourFromTagsKeyName(Box->TagsKey, "accent"_s8);
    v4f32 TextColour = UIColourFromTagsKeyName(Box->TagsKey, "text"_s8);
    v4f32 DisabledColour = Vec(TextColour.X, TextColour.Y, TextColour.Z, TextColour.W * 0.3f);

    for (u64 I = 0; I < Data->ItemCount; ++I) {
        f32 Angle0 = StartAngle + AngleStep * (f32) I;
        f32 Angle1 = StartAngle + AngleStep * (f32) (I + 1);
        f32 MidAngle = (Angle0 + Angle1) * 0.5f;
        b32 IsHovered = (Data->HoveredIndex == (i64) I);
        b32 IsEnabled = Data->Items[I].Enabled;
        
        f32 HoverT = IsHovered ? Data->HoverT : 0.0f;
        
        v4f32 SegmentColour = Vec(
            LerpF32(HoverT, BaseColour.X, HoverColour.X),
            LerpF32(HoverT, BaseColour.Y, HoverColour.Y),
            LerpF32(HoverT, BaseColour.Z, HoverColour.Z),
            LerpF32(HoverT, BaseColour.W, HoverColour.W)
        );

        if (!IsEnabled)
            SegmentColour.W *= 0.5f;
        
        u32 SegmentCount = 16;

        for (u32 J = 0; J < SegmentCount; ++J) {
            f32 SegAngle0 = LerpF32((f32) J / (f32) SegmentCount, Angle0, Angle1);
            f32 SegAngle1 = LerpF32((f32) (J + 1) / (f32) SegmentCount, Angle0, Angle1);
            v2f32 Inner0 = Vec(
                Data->MenuCentre.X + Cos(SegAngle0) * Data->InnerRadius, 
                Data->MenuCentre.Y + Sin(SegAngle0) * Data->InnerRadius
            );
            v2f32 Inner1 = Vec(
                Data->MenuCentre.X + Cos(SegAngle1) * Data->InnerRadius, 
                Data->MenuCentre.Y + Sin(SegAngle1) * Data->InnerRadius
            );
            v2f32 Outer0 = Vec(
                Data->MenuCentre.X + Cos(SegAngle0) * Data->OuterRadius, 
                Data->MenuCentre.Y + Sin(SegAngle0) * Data->OuterRadius
            );
            v2f32 Outer1 = Vec(
                Data->MenuCentre.X + Cos(SegAngle1) * Data->OuterRadius, 
                Data->MenuCentre.Y + Sin(SegAngle1) * Data->OuterRadius
            );
            r2f32 SegmentRect = Rng(
                MIN(MIN(Inner0.X, Inner1.X), MIN(Outer0.X, Outer1.X)),
                MIN(MIN(Inner0.Y, Inner1.Y), MIN(Outer0.Y, Outer1.Y)),
                MAX(MAX(Inner0.X, Inner1.X), MAX(Outer0.X, Outer1.X)),
                MAX(MAX(Inner0.Y, Inner1.Y), MAX(Outer0.Y, Outer1.Y))
            );

            DrawRect(
                SegmentRect,
                SegmentColour,
                0.0f,
                0.0f,
                1.0f
            );
        }
        
        f32 LabelRadius = (Data->InnerRadius + Data->OuterRadius) * 0.5f;
        v2f32 LabelPos = Vec(
            Data->MenuCentre.X + Cos(MidAngle) * LabelRadius,
            Data->MenuCentre.Y + Sin(MidAngle) * LabelRadius
        );

        v4f32 LabelColour = IsEnabled ? TextColour : DisabledColour;
        Str8 Label = Data->Items[I].DisplayString;

        if (Label.Size > 0) {
            v2f32 TextDim = FontDimensionsFromTagSizeString(
                Box->Font,
                Box->FontSize,
                0.0f,
                Box->TabSize,
                Label
            );

            NC_DrawText(
                Box->Font,
                Box->FontSize,
                0.0f,
                Box->TabSize,
                Box->TextRasterKind,
                Vec(LabelPos.X - TextDim.X * 0.5f, LabelPos.Y - TextDim.Y * 0.5f),
                LabelColour,
                Label
            );
        }
    }
}

INTERNAL
UI_BOX_CUSTOM_DRAW(UIRadioButtonDraw)
{
    UIRadioButtonDrawData* Data = (UIRadioButtonDrawData*) UserData;
    v2f32 BoxDim = Length(Box->Rect);
    v2f32 Centre = Vec(
        Box->Rect.X0 + BoxDim.X * 0.5f,
        Box->Rect.Y0 + BoxDim.Y * 0.5f
    );
    f32 OuterRadius = MIN(BoxDim.X, BoxDim.Y) * 0.4f;
    f32 InnerRadius = OuterRadius * 0.5f * Data->SelectedT;

    v4f32 BorderColour = UIColourFromTagsKeyName(Box->TagsKey, "border"_s8);

    DrawRect(
        Rng(
            Centre.X - OuterRadius, 
            Centre.Y - OuterRadius, 
            Centre.X + OuterRadius, 
            Centre.Y + OuterRadius
        ),
        BorderColour,
        OuterRadius,
        1.5f,
        1.0f
    );

    if (Data->SelectedT > 0.01f) {
        v4f32 FillColour = UIColourFromTagsKeyName(Box->TagsKey, "accent"_s8);

        DrawRect(
            Rng(Centre.X - InnerRadius, Centre.Y - InnerRadius, Centre.X + InnerRadius, Centre.Y + InnerRadius),
            FillColour,
            InnerRadius,
            0.0f,
            1.0f
        );
    }
}
