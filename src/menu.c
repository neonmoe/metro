#include "menu.h"
#include "font_setting.h"
#include "raymath.h"
#include <stdio.h>

static Color textColor = { 0xEE, 0xEE, 0xEE, 0xFF };

static
bool IsNextSelected() {
    bool shifted = IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT);
    return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
        (IsKeyPressed(KEY_TAB) && !shifted);
}

static
bool IsPreviousSelected() {
    bool shifted = IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT);
    return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
        (IsKeyPressed(KEY_TAB) && shifted);
}

static
bool IsSelectionActivated() {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

static
void DrawSelection(Rectangle button) {
    Rectangle selectRect = {
        button.x - 4.0f, button.y - 4.0f,
        button.width + 8.0f, button.height + 9.0f
    };
    DrawRectangleLinesEx(selectRect, 2, (Color){0x88, 0x88, 0x88, 0xFF});
}

static
bool Button(FontSetting *fontSetting, const char* text, Rectangle button,
            Vector2 textPosition, Color color, Color highlightedColor,
            bool selected) {
    Vector2 mousePosition = GetMousePosition();
    bool clicked = false;
    if (CheckCollisionPointRec(mousePosition, button)) {
        DrawRectangleRec(button, highlightedColor);
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            clicked = true;
        }
    } else {
        DrawRectangleRec(button, color);
    }
    if (selected) {
        DrawSelection(button);
        if (IsSelectionActivated()) {
            clicked = true;
        }
    }
    DrawTextEx(*fontSetting->currentFont, text,
               textPosition, 36, 0.0f, textColor);
    return clicked;
}

static const char *pressedSlider = 0;
static float mousePositionRelativeToHandle = 0.0f;

static
void Slider(FontSetting *fontSetting, double deltaTime,
            const char* text, const char* formattingString,
            float *value, float min, float max, float step,
            Vector2 sliderStart, float width,
            Vector2 textPosition, Color color, Color highlightedColor,
            bool selected) {
    Vector2 mousePosition = GetMousePosition();
    bool justPressed = false;

    // Draw slider line
    Color sliderColor = color;
    sliderColor.r -= 0x22;
    sliderColor.g -= 0x22;
    sliderColor.b -= 0x22;
    Rectangle slider = {
        sliderStart.x, sliderStart.y, width, 2.0f
    };
    DrawRectangleRec(slider, sliderColor);
    // Draw handle
    Rectangle button = {
        sliderStart.x - 8.0f, sliderStart.y - 12.0f, 16.0f, 24.0f
    };
    button.x += width * (*value - min) / (max - min);
    if (CheckCollisionPointRec(mousePosition, button)) {
        DrawRectangleRec(button, highlightedColor);
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            justPressed = true;
        }
    } else {
        DrawRectangleRec(button, color);
    }

    if (selected) {
        DrawSelection(button);
    }

    // Draw text
    DrawTextEx(*fontSetting->currentFont, text,
               textPosition, 36, 0.0f, textColor);
    Vector2 size = MeasureTextEx(*fontSetting->currentFont, text, 36, 0.0f);
    textPosition.x += size.x;
    char valueDisplay[16];
    snprintf(valueDisplay, 16, formattingString, *value);
    DrawTextEx(*fontSetting->currentFont, valueDisplay,
               textPosition, 36, 0.0f, textColor);

    if (pressedSlider == text) {
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            float mouse = mousePosition.x - mousePositionRelativeToHandle;
            float newPosition = Clamp(mouse - slider.x, 0.0f, width);
            float newValue = newPosition / width * (max - min) + min;
            newValue = roundf(newValue / step) * step;
            *value = newValue;
        } else {
            pressedSlider = 0;
        }
    } else if (justPressed) {
        pressedSlider = text;
        mousePositionRelativeToHandle = mousePosition.x - button.x - button.width / 2.0f;
    }

    if (selected) {
        float delta = 0.0f;
        // FIXME: smooth, delta based sliding
        if (IsKeyPressed(KEY_LEFT)) {
            delta -= step;
        }
        if (IsKeyPressed(KEY_RIGHT)) {
            delta += step;
        }
        *value = Clamp(*value + delta, min, max);
    }
}

static
int GetNewSelectionIndex(int selectionIndex, bool optionsOpened) {
    if (selectionIndex == -1) {
        return IsNextSelected() ? 0 : -1;
    }
    int indexCount = optionsOpened ? 8 : 3; // fixme
    if (IsNextSelected()) {
        selectionIndex++;
        if (selectionIndex >= indexCount) {
            selectionIndex -= indexCount;
        }
    }
    if (IsPreviousSelected()) {
        selectionIndex--;
        if (selectionIndex < 0) {
            selectionIndex += indexCount;
        }
    }
    return selectionIndex;
}

static bool optionsOpened = false;

bool ShowMainMenu(FontSetting *fontSetting, float *fov, float *bobIntensity,
                  int *mouseSpeedX, int *mouseSpeedY) {
    bool continueGame = false;
    int selectionIndex = -1;
    double lastTime = GetTime();

    while (!continueGame) {
        if (WindowShouldClose()) {
            return true;
        }
        if (IsKeyPressed(KEY_P)) {
            continueGame = true;
        }

        double time = GetTime();
        double delta = time - lastTime;
        lastTime = time;
        int screenSizeOffsetX = (GetScreenWidth() - 640) / 2;
        int screenSizeOffsetY = (GetScreenHeight() - 480) / 2;
        float fontOffset = fontSetting->clearFontEnabled ? 30.0f : 0.0f;
        Color buttonColor = (Color){ 0x44, 0x44, 0x44, 0xFF };
        Color buttonHighlightColor = (Color){ 0x55, 0x55, 0x55, 0xFF };
        Color checkedColor = (Color){ 0x66, 0x66, 0x66, 0xFF };
        Color redButtonColor = (Color){ 0x55, 0x44, 0x44, 0xFF };
        Color redButtonHighlightColor = (Color){ 0x66, 0x44, 0x44, 0xFF };

        selectionIndex = GetNewSelectionIndex(selectionIndex, optionsOpened);

        BeginDrawing();
        ClearBackground((Color){ 0x33, 0x33, 0x33, 0xFF });

        float controlX = 50.0f + screenSizeOffsetX;
        float controlY = 40.0f + screenSizeOffsetY;

        DrawTextEx(*fontSetting->currentFont, "A Walk In A Metro Tunnel",
                   (Vector2) { controlX, controlY }, 48, 0.0f, textColor);

        controlX -= 10.0f;
        controlY += 90.0f;
        Rectangle startButton = { controlX, controlY,
                                  240.0f - fontOffset, 50.0f };
        Vector2 startTextPosition = { controlX + 20.0f, controlY + 5.0f };
        if (Button(fontSetting, "Start walking",
                   startButton, startTextPosition,
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 0)) {
            continueGame = true;
        }

        if (optionsOpened) {
            controlX += 270.0f;
        } else {
            controlY += 140.0f;
        }
        if (Button(fontSetting, "Close Application",
                   (Rectangle){ controlX, controlY,
                           285.0f - fontOffset, 50.0f },
                   (Vector2){ controlX + 20.0f, controlY + 5.0f },
                   redButtonColor, redButtonHighlightColor,
                   selectionIndex == (optionsOpened ? 1 : 2))) {
            return true;
        }
        if (optionsOpened) {
            controlX -= 270.0f;
            controlY += 70.0f;
        } else {
            controlY -= 70.0f;
        }

        if (optionsOpened) {
            controlX += 13.0f;

            controlX -= 70.0f;
            controlY -= 2.0f;
            if (Button(fontSetting, "<",
                       (Rectangle){ controlX, controlY, 40.0f, 40.0f },
                       (Vector2){ controlX + 12.0f, controlY + 3.0f },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 2)) {
                optionsOpened = false;
                selectionIndex = 1;
            }
            controlX += 70.0f;
            controlY += 2.0f;

            int fontToggleOffsetX = 385;
            if (Button(fontSetting, "Use fancy font:",
                       (Rectangle){ controlX + fontToggleOffsetX, controlY,
                               40.0f, 40.0f },
                       (Vector2){ controlX, controlY },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 3)) {
                SwitchFont(fontSetting);
            }
            if (!fontSetting->clearFontEnabled) {
                DrawRectangle((int)controlX + fontToggleOffsetX + 8,
                              (int)controlY + 8, 24, 24, checkedColor);
            }

            controlY += 50.0f;
            Slider(fontSetting, delta, "Field of view: ", "%3.0f",
                   fov, 60.0f, 120.0f, 1.0f,
                   (Vector2){ controlX + 295.0f, controlY + 19.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 4);

            controlY += 50.0f;
            float bobValue = *bobIntensity * 100.0f;
            Slider(fontSetting, delta, "Bob intensity: ", "%3.0f%%",
                   &bobValue, 0.0f, 100.0f, 1.0f,
                   (Vector2){ controlX + 295.0f, controlY + 19.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 5);
            *bobIntensity = bobValue / 100.0f;

            controlY += 50.0f;
            float mouseXVal = (float)*mouseSpeedX / 100.0f;
            Slider(fontSetting, delta, "Mouse speed X: ", "%1.1f",
                   &mouseXVal, -4.0f, 4.0f, 0.1f,
                   (Vector2){ controlX + 295.0f, controlY + 19.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 6);
            *mouseSpeedX = (int)(mouseXVal * 100.0f);

            controlY += 50.0f;
            float mouseYVal = (float)*mouseSpeedY / 100.0f;
            Slider(fontSetting, delta, "Mouse speed Y: ", "%1.1f",
                   &mouseYVal, -4.0f, 4.0f, 0.1f,
                   (Vector2){ controlX + 295.0f, controlY + 19.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 7);
            *mouseSpeedY = (int)(mouseYVal * 100.0f);
        } else {
            if (Button(fontSetting, "Options",
                       (Rectangle){ controlX, controlY,
                               165.0f - fontOffset, 50.0f },
                       (Vector2){ controlX + 20.0f, controlY + 5.0f },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 1)) {
                optionsOpened = true;
                selectionIndex = 2;
            }
        }

        EndDrawing();
    }
    return false;
}
