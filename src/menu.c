/* This is a game where the player walks through a metro tunnel.
 * Copyright (C) 2019  Jens Pitkanen <jens@neon.moe>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>

#include "menu.h"
#include "font_setting.h"
#include "raymath.h"
#include "render_utils.h"

static Color textColor = { 0xEE, 0xEE, 0xEE, 0xFF };
static Color sliderColor = { 0x55, 0x50, 0x40, 0xFF };
static float optionFontSize = 28.0f;
static float optionEntryHeight = 40.0f;

static
bool IsNextSelected(void) {
    bool shifted = IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT);
    return IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S) ||
        (IsKeyPressed(KEY_TAB) && !shifted);
}

static
bool IsPreviousSelected(void) {
    bool shifted = IsKeyDown(KEY_LEFT_SHIFT) ||
        IsKeyDown(KEY_RIGHT_SHIFT);
    return IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W) ||
        (IsKeyPressed(KEY_TAB) && shifted);
}

static
bool IsSelectionActivated(void) {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

static
void DrawOutline(Rectangle button, int offset, int thickness, Color color) {
    Rectangle selectRect = {
        button.x - offset, button.y - offset,
        button.width + offset * 2, button.height + offset * 2
    };
    DrawRectangleLinesEx(selectRect, thickness, color);
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
    DrawOutline(button, 2, 2, (Color){0, 0, 0, 0x77});

    if (selected) {
        DrawOutline(button, 4, 2, (Color){0x88, 0x88, 0x88, 0xFF});
        if (IsSelectionActivated()) {
            clicked = true;
        }
    }
    DrawTextEx(*fontSetting->currentFont, text,
               textPosition, optionFontSize, 0.0f, textColor);
    return clicked;
}

static const char *pressedSlider = 0;
static float mousePositionRelativeToHandle = 0.0f;

static
void Slider(FontSetting *fontSetting, float deltaTime,
            const char* text, const char* formattingString,
            float *value, float min, float max, float step,
            Vector2 sliderStart, float width,
            Vector2 textPosition, Color color, Color highlightedColor,
            bool selected) {
    Vector2 mousePosition = GetMousePosition();
    bool justPressed = false;

    // Draw slider line
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
    DrawOutline(button, 2, 2, (Color){0, 0, 0, 0x77});

    if (selected) {
        DrawOutline(button, 4, 2, (Color){0x88, 0x88, 0x88, 0xFF});
    }

    // Draw text
    DrawTextEx(*fontSetting->currentFont, text,
               textPosition, optionFontSize, 0.0f, textColor);
    Vector2 size = MeasureTextEx(*fontSetting->currentFont, text, optionFontSize, 0.0f);
    textPosition.x += size.x;
    char valueDisplay[16];
    snprintf(valueDisplay, 16, formattingString, *value);
    DrawTextEx(*fontSetting->currentFont, valueDisplay,
               textPosition, optionFontSize, 0.0f, textColor);

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
        // Sorry about using static here, but I just, really can't be
        // bothered to not do it this way right now.
        static double leftPressedTime = 0.0;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            delta -= step;
            leftPressedTime = GetTime();
        } else if ((IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) &&
                   GetTime() - leftPressedTime > 0.5) {
            delta -= (max - min) * 0.7f * deltaTime;
        }

        static double rightPressedTime = 0.0;
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            delta += step;
            rightPressedTime = GetTime();
        } else if ((IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) &&
                   GetTime() - rightPressedTime > 0.5) {
            delta += (max - min) * 0.7f * deltaTime;
        }
        *value = Clamp(*value + delta, min, max);
    }
}

static
int GetNewSelectionIndex(int selectionIndex, bool optionsOpened) {
    if (selectionIndex == -1) {
        return IsNextSelected() || IsPreviousSelected() ? 0 : -1;
    }
    int indexCount = optionsOpened ? 8 : 3;
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

static
int GetUIScale(void) {
    int width = GetScreenWidth();
    int height = GetScreenHeight();
    int size = width < height ? width : height;
    int scale = size / 480;
    return scale < 1 ? 1 : scale;
}

static bool optionsOpened = false;

bool ShowMainMenu(FontSetting *fontSetting, Texture2D gameRenderTexture,
                  bool gameStarted, float *fov, float *bobIntensity,
                  int *mouseSpeedX, int *mouseSpeedY) {
    bool continueGame = false;
    int selectionIndex = -1;
    float lastTime = (float)GetTime();
    float startTime = (float)GetTime();

    bool mouseInvertedX = *mouseSpeedX < 0;
    bool mouseInvertedY = *mouseSpeedY < 0;

    while (!continueGame) {
        if (WindowShouldClose()) {
            return true;
        }
        if (IsKeyPressed(KEY_P)) {
            continueGame = true;
        }

        int uiScale = GetUIScale();
        optionFontSize = 28.0f * uiScale;
        optionEntryHeight = 40.0f * uiScale;

        float time = (float)GetTime();
        float delta = time - lastTime;
        lastTime = time;

        int screenSizeOffsetX = (GetScreenWidth() - 640 * uiScale) / 2;
        int screenSizeOffsetY = (GetScreenHeight() - 480 * uiScale) / 2;
        int controlOffsetX = 295 * uiScale;

        float fontOffset = fontSetting->clearFontEnabled ? 30.0f : 0.0f;
        Color buttonColor = (Color){ 0x44, 0x44, 0x44, 0xFF };
        Color buttonHighlightColor = (Color){ 0x55, 0x55, 0x55, 0xFF };
        Color checkedColor = (Color){ 0x88, 0x88, 0x88, 0xFF };
        Color redButtonColor = (Color){ 0x55, 0x44, 0x44, 0xFF };
        Color redButtonHighlightColor = (Color){ 0x66, 0x44, 0x44, 0xFF };

        selectionIndex = GetNewSelectionIndex(selectionIndex, optionsOpened);

        BeginDrawing();
        ClearBackground((Color){ 0x33, 0x33, 0x33, 0xFF });

        float controlX = 50.0f + screenSizeOffsetX;
        float controlY = 40.0f + screenSizeOffsetY;

        // Draw game
        DrawGameView(gameRenderTexture);
        // Draw filter to fade it out a bit
        char filterAlpha;
        if (gameStarted) {
            float filterAnimTime = 0.2f;
            filterAlpha = (char)(Clamp((time - startTime) / filterAnimTime,
                                       0, 1) * 0xBB);
        } else {
            filterAlpha = 0xBB;
        }
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      (Color){0x20, 0x24, 0x30, filterAlpha});

        // Draw the header
        DrawTextEx(*fontSetting->currentFont, "A Walk In A Metro Tunnel",
                   (Vector2) { controlX, controlY },
                   optionFontSize * 1.5f, 0.0f, textColor);

        // Start drawing the menu
        controlX -= 10.0f;
        controlY += 90.0f * uiScale;
        Rectangle startButton = { controlX, controlY,
                                  optionEntryHeight * 5.0f - fontOffset,
                                  optionEntryHeight };
        Vector2 startTextPosition = { controlX + 20.0f, controlY + 5.0f * uiScale };
        if (Button(fontSetting, gameStarted ? "Continue" : "Start walking",
                   startButton, startTextPosition,
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 0)) {
            continueGame = true;
        }

        if (optionsOpened) {
            controlX += optionEntryHeight * 5.6f - fontOffset * uiScale;
        } else {
            controlY += (optionEntryHeight + 20.0f) * 2.0f;
        }
        if (Button(fontSetting, "Close Application",
                   (Rectangle){ controlX, controlY,
                           optionEntryHeight * 5.8f - fontOffset,
                           optionEntryHeight },
                   (Vector2){ controlX + 20.0f, controlY + 5.0f * uiScale },
                   redButtonColor, redButtonHighlightColor,
                   selectionIndex == (optionsOpened ? 1 : 2))) {
            return true;
        }
        if (optionsOpened) {
            controlX -= optionEntryHeight * 5.6f - fontOffset * uiScale;
            controlY += optionEntryHeight + 20.0f;
        } else {
            controlY -= optionEntryHeight + 20.0f;
        }

        if (optionsOpened) {
            controlX += 13.0f;

            controlX -= optionEntryHeight + 20.0f;
            controlY -= 2.0f;
            if (Button(fontSetting, "<",
                       (Rectangle){ controlX, controlY,
                               optionFontSize + 5.0f, optionFontSize + 5.0f },
                       (Vector2){ controlX + 12.0f, controlY + 3.0f },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 2)) {
                optionsOpened = false;
                if (selectionIndex != -1) {
                    selectionIndex = 1;
                }
            }
            controlX += optionEntryHeight + 20.0f;
            controlY += 2.0f;

            if (Button(fontSetting, "Use fancy font:",
                       (Rectangle){ controlX + controlOffsetX, controlY,
                               optionFontSize, optionFontSize },
                       (Vector2){ controlX, controlY },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 3)) {
                SwitchFont(fontSetting);
            }
            if (!fontSetting->clearFontEnabled) {
                DrawRectangle((int)controlX + controlOffsetX + 8 * uiScale,
                              (int)controlY + 8 * uiScale,
                              (int)optionFontSize - 16 * uiScale,
                              (int)optionFontSize - 16 * uiScale,
                              checkedColor);
            }

            controlY += optionEntryHeight;
            Slider(fontSetting, delta, "Field of view: ", "%3.0f",
                   fov, 60.0f, 120.0f, 1.0f,
                   (Vector2){ controlX + controlOffsetX,
                           controlY + optionFontSize / 2.0f}, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 4);

            controlY += optionEntryHeight;
            float bobValue = *bobIntensity * 100.0f;
            Slider(fontSetting, delta, "Bob intensity: ", "%3.0f%%",
                   &bobValue, 0.0f, 100.0f, 1.0f,
                   (Vector2){ controlX + controlOffsetX,
                           controlY + optionFontSize / 2.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 5);
            *bobIntensity = bobValue / 100.0f;

            controlY += optionEntryHeight;
            float mouseXVal = fabsf((float)*mouseSpeedX / 100.0f);
            Slider(fontSetting, delta, "Mouse speed X: ", "%1.1f",
                   &mouseXVal, 0.0f, 4.0f, 0.1f,
                   (Vector2){ controlX + controlOffsetX,
                           controlY + optionFontSize / 2.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 6);
            *mouseSpeedX = (int)(mouseXVal * 100.0f
                                 * (mouseInvertedX ? -1.0f : 1.0f));

            controlY += optionEntryHeight;
            if (Button(fontSetting, "Invert:",
                       (Rectangle){ controlX + controlOffsetX, controlY,
                               optionFontSize, optionFontSize },
                       (Vector2){ controlX + 20.0f, controlY },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 3)) {
                mouseInvertedX = !mouseInvertedX;
            }
            if (mouseInvertedX) {
                DrawRectangle((int)controlX + controlOffsetX + 8 * uiScale,
                              (int)controlY + 8 * uiScale,
                              (int)optionFontSize - 16 * uiScale,
                              (int)optionFontSize - 16 * uiScale,
                              checkedColor);
            }

            controlY += optionEntryHeight;
            float mouseYVal = fabsf((float)*mouseSpeedY / 100.0f);
            Slider(fontSetting, delta, "Mouse speed Y: ", "%1.1f",
                   &mouseYVal, 0.0f, 4.0f, 0.1f,
                   (Vector2){ controlX + controlOffsetX, controlY + 19.0f }, 220.0f,
                   (Vector2){ controlX, controlY },
                   buttonColor, buttonHighlightColor,
                   selectionIndex == 7);
            *mouseSpeedY = (int)(mouseYVal * 100.0f
                                 * (mouseInvertedY ? -1.0f : 1.0f));

            controlY += optionEntryHeight;
            if (Button(fontSetting, "Invert:",
                       (Rectangle){ controlX + controlOffsetX, controlY,
                               optionFontSize, optionFontSize },
                       (Vector2){ controlX + 20.0f, controlY },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 3)) {
                mouseInvertedY = !mouseInvertedY;
            }
            if (mouseInvertedY) {
                DrawRectangle((int)controlX + controlOffsetX + 8 * uiScale,
                              (int)controlY + 8 * uiScale,
                              (int)optionFontSize - 16 * uiScale,
                              (int)optionFontSize - 16 * uiScale,
                              checkedColor);
            }
        } else {
            if (Button(fontSetting, "Options",
                       (Rectangle){ controlX, controlY,
                               optionEntryHeight * 3.6f - fontOffset,
                               optionEntryHeight },
                       (Vector2){ controlX + 20.0f, controlY + 5.0f * uiScale },
                       buttonColor, buttonHighlightColor,
                       selectionIndex == 1)) {
                optionsOpened = true;
                if (selectionIndex != -1) {
                    selectionIndex = 2;
                }
            }
        }

        EndDrawing();
    }
    return false;
}
