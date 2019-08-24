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

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "script.h"
#include "sdf_utils.h"
#include "resources.h"
#include "font_setting.h"
#include "menu.h"
#include "render_utils.h"

#define DEFAULT_SCREEN_WIDTH 800
#define DEFAULT_SCREEN_HEIGHT 500

#define DEFAULT_MAX_DISTANCE 2140.0f
#define WALK_SPEED 1.4f
#define RUN_SPEED 4.2f
#define HEAD_BOB_MAGNITUDE 0.05f
#define HEAD_BOB_FREQUENCY 1.3f
#define COMMENT_LENGTH (DEFAULT_MAX_DISTANCE / COMMENTS_COUNT)
#define BACKTRACKING_WARNING_DISTANCE 10.0f

#define LONGEST_COMMENT_CHARACTER_COUNT_ESTIMATE 400
#define METERS_PER_CHARACTER (DEFAULT_MAX_DISTANCE / (COMMENTS_COUNT * LONGEST_COMMENT_CHARACTER_COUNT_ESTIMATE))

bool FileMissing(const char *path);
void DrawWarningText(const char *text, int fontSize, int y, Color color);
bool EnsureResourcesExist(void);
Shader LoadSDFShader(void);
bool ShowEpilepsyWarning(FontSetting *fontSetting);
Rectangle GetRenderSrc(int screenWidth, int screenHeight);
Rectangle GetRenderDest(int screenWidth, int screenHeight);
Vector3 GetLegalPlayerMovement(Vector3 position, Vector3 movement, float maxDistance);
float NoiseifyPosition(float position);
void DisplaySubtitle(Font font, const char *subtitle, float fontSize, float y);
int GetLine(float narrationTime, int narrationStage, int linesPerScreen);

int main(void) {
    // Player values
    float cameraPosition[] = { 0.0f, 1.75f, 0.0f };
    float cameraRotation[] = { 0.0f, 0.0f, 0.0f };
    float walkingTime = 0.0f;
    float headBobAmount = 0.0f;
    bool autoMove = false;
    bool running = false;
    float metersWalked = 0.0f;

    // Mouselook values
    int mouseX = -1;
    int mouseY = -1;
    bool mouseLookEnabled = false;
    bool narrationEnabled = true;

    // Progress values
    int lightsStage = 0;
    int narrationStage = -1;
    float narrationStartZ = 0.0f;
    float furthestDistanceSoFar = cameraPosition[2];
    float backtrackingTime = 0.0f;

    // Runtime configurable options
    float fieldOfView = 80.0f;
    float bobbingIntensity = 1.0f;
    int mouseSpeedX = 150;
    int mouseSpeedY = 150;
    bool showMetersWalked = false;

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    SetExitKey(KEY_F4);
    InitWindow(DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT,
               "A Walk In A Metro Tunnel");

    Image windowIcon = LoadImage(resourcePaths[RESOURCE_ICON]);
    SetWindowIcon(windowIcon);

    if (EnsureResourcesExist()) {
        return 0;
    }

    Font vt323Font = LoadFontEx(resourcePaths[RESOURCE_VT323], 72, 0, 0);
    Font openSansFont = LoadFontEx(resourcePaths[RESOURCE_OPEN_SANS], 72, 0, 0);
    FontSetting fontSetting = {
        vt323Font, openSansFont, false, NULL /* This is set immediately after */
    };
    /* DO NOT REMOVE THE FOLLOWING LINE IT'LL BE A NULL POINTER OTHERWISE */
    fontSetting.currentFont = &fontSetting.mainFont;
    /* currentFont is initialized as NULL and then set to point to
     * mainFont because MSVC complains if you try to set currentFont
     * to &vt323Font in the initializer */

    Shader sdfShader = LoadSDFShader();
    int resolutionLocation = GetShaderLocation(sdfShader, "resolution");
    int cameraPositionLocation = GetShaderLocation(sdfShader, "cameraPosition");
    int cameraRotationLocation = GetShaderLocation(sdfShader, "cameraRotation");
    int cameraFieldOfViewLocation = GetShaderLocation(sdfShader, "cameraFieldOfView");
    int lightsStageLocation = GetShaderLocation(sdfShader, "stage");
    int maxDistanceLocation = GetShaderLocation(sdfShader, "maxDistance");

    RenderTexture2D targetTex = LoadRenderTexture(VIRTUAL_SCREEN_HEIGHT * 2,
                                                  VIRTUAL_SCREEN_HEIGHT);
    float resolution[] = { (float)VIRTUAL_SCREEN_HEIGHT * 2,
                           (float)VIRTUAL_SCREEN_HEIGHT };
    SetShaderValue(sdfShader, resolutionLocation, resolution, UNIFORM_VEC2);
    float maxDistance = DEFAULT_MAX_DISTANCE;
    SetShaderValue(sdfShader, maxDistanceLocation, &maxDistance, UNIFORM_FLOAT);

    bool windowClosedInMenu = ShowEpilepsyWarning(&fontSetting);
    bool firstMainMenuShown = false;
    bool firstGameRenderDone = false;

    float lastTime = (float)GetTime();
    while (!WindowShouldClose() && !windowClosedInMenu) {
        float currentTime = (float)GetTime();
        float delta = currentTime - lastTime;
        delta = delta > 0.03f ? 0.03f : delta;
        lastTime = currentTime;

        // Configuration keys
        if (IsKeyPressed(KEY_B)) {
            // Toggle bobbing
            bobbingIntensity = bobbingIntensity > 0.5f ? 0.0f : 1.0f;
        }

        if (IsKeyPressed(KEY_T)) {
            SwitchFont(&fontSetting);
        }

        // Menu access
        if (IsKeyPressed(KEY_ESCAPE) || (!firstMainMenuShown && firstGameRenderDone)) {
            mouseLookEnabled = false;
            EnableCursor();
            windowClosedInMenu |=
                ShowMainMenu(&fontSetting, targetTex.texture,
                             firstMainMenuShown, &fieldOfView, &bobbingIntensity,
                             &mouseSpeedX, &mouseSpeedY, &showMetersWalked,
                             &narrationEnabled);
            firstMainMenuShown = true;
        }

        // Turn around
        if (IsKeyDown(KEY_LEFT)) {
            cameraRotation[1] -= delta * 120.0f * (mouseSpeedX / 100.0f);
            if (cameraRotation[1] < 0.0) {
                cameraRotation[1] += 360.0f;
            }
        }
        if (IsKeyDown(KEY_RIGHT)) {
            cameraRotation[1] += delta * 120.0f * (mouseSpeedX / 100.0f);
            if (cameraRotation[1] > 360) {
                cameraRotation[1] -= 360.0f;
            }
        }
        if (IsKeyDown(KEY_UP)) {
            cameraRotation[0] -= delta * 90.0f * (mouseSpeedY / 100.0f);
            if (cameraRotation[0] < -90.0) {
                cameraRotation[0] = -90.0f;
            }
        }
        if (IsKeyDown(KEY_DOWN)) {
            cameraRotation[0] += delta * 90.0f * (mouseSpeedY / 100.0f);
            if (cameraRotation[0] > 90.0) {
                cameraRotation[0] = 90.0f;
            }
        }

        // Mouselook
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mouseLookEnabled = true;
            DisableCursor();
        }
        if (mouseLookEnabled) {
            float dx = (float)(GetMouseX() - mouseX) / GetScreenHeight() * mouseSpeedX;
            float dy = (float)(GetMouseY() - mouseY) / GetScreenHeight() * mouseSpeedY;
            mouseX = GetMouseX();
            mouseY = GetMouseY();
            cameraRotation[0] = Clamp(cameraRotation[0] + dy, -90.0f, 90.0f);
            cameraRotation[1] += dx;
        } else {
            mouseX = GetMouseX();
            mouseY = GetMouseY();
        }

        // Run toggle
        if (IsKeyPressed(KEY_LEFT_SHIFT)) {
            running = !running;
        }

        // Walk
        float r = cameraRotation[1] * DEG2RAD;
        bool walking = autoMove;
        float speed = running ? RUN_SPEED : WALK_SPEED;
        Vector3 movement = { 0.0f, 0.0f, 0.0f };
        if (IsKeyPressed(KEY_Q)) {
            autoMove = !autoMove;
        }
        if (autoMove) {
            movement.x += delta * speed * sinf(r);
            movement.z += delta * speed * cosf(r);
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_I)) {
            movement.x += delta * speed * sinf(r);
            movement.z += delta * speed * cosf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_K)) {
            movement.x -= delta * speed * sinf(r);
            movement.z -= delta * speed * cosf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_L)) {
            movement.x += delta * speed * cosf(r);
            movement.z += delta * speed * -sinf(r);
            walking = true;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_J)) {
            movement.x -= delta * speed * cosf(r);
            movement.z -= delta * speed * -sinf(r);
            walking = true;
        }

        // The actual movement
        Vector3 position = { cameraPosition[0], 0.0f, cameraPosition[2] };
        // ..on the forward axis
        Vector3 forward = GetPathForward(position, maxDistance);
        float forwardDotMovement = Vector3DotProduct(forward, movement);
        forward = Vector3Scale(forward, forwardDotMovement);
        position = GetLegalPlayerMovement(position, forward, maxDistance);
        // ..on the right axis
        Vector3 right = GetPathNormal(position, maxDistance);
        right = Vector3Scale(right, Vector3DotProduct(right, movement));
        position = GetLegalPlayerMovement(position, right, maxDistance);
        // ..and finally applying it to the actual coordinates
        float previousX = cameraPosition[0];
        float previousZ = cameraPosition[2];
        cameraPosition[0] = position.x;
        cameraPosition[2] = Clamp(position.z, -10.0f, maxDistance + 10.0f);
        float deltaX = cameraPosition[0] - previousX;
        float deltaZ = cameraPosition[2] - previousZ;
        metersWalked += sqrtf(deltaX * deltaX + deltaZ * deltaZ);

        if (walking) {
            walkingTime += delta;
        } else {
            walkingTime = 0.0f;
        }

        // Crouch and bob
        cameraPosition[1] -= headBobAmount;
        float bobTime = walkingTime * 6.28f * HEAD_BOB_FREQUENCY *
            (running ? 1.4f : 1.0f);
        float targetBob = sinf(bobTime) * HEAD_BOB_MAGNITUDE * bobbingIntensity;
        headBobAmount = Lerp(headBobAmount, targetBob, 0.2f);
        Vector3 cameraPositionVec = {
            cameraPosition[0], cameraPosition[1], cameraPosition[2]
        };
        float relativeX = TransformToMetroSpace(cameraPositionVec, maxDistance).x;
        bool onPlank = fabs(relativeX) < 1.0;
        bool onRail = fabs(relativeX) > 0.762 - 0.05 &&
            fabs(relativeX) < 0.762 + 0.05;
        float height = onRail ? 0.3f : (onPlank ? 0.1f : 0.0f);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            height += 0.9f;
        } else {
            height += 1.75f;
        }
        cameraPosition[1] = Lerp(cameraPosition[1], height, 10.0f * delta);
        cameraPosition[1] += headBobAmount;

        // Activate location-based actions
        float lightMaxDistance = maxDistance - 9.0f;
        float triggerPosition = Clamp(NoiseifyPosition(cameraPosition[2]),
                                      0.0f, lightMaxDistance);
        if (triggerPosition > (lightsStage + 1) * 9) {
            lightsStage++;
        }
        if (cameraPosition[2] > (narrationStage + 1) * COMMENT_LENGTH + 6) {
            narrationStartZ = (narrationStage + 1) * COMMENT_LENGTH + 6;
            narrationStage++;
        }

        // Backtracking check
        bool backtracking = false;
        if (cameraPosition[2] > furthestDistanceSoFar) {
            furthestDistanceSoFar = cameraPosition[2];
            backtrackingTime = 0.0f;
        } else if (furthestDistanceSoFar - cameraPosition[2] > BACKTRACKING_WARNING_DISTANCE) {
            backtracking = true;
            backtrackingTime += delta;
        }

        BeginDrawing();
        ClearBackground((Color){ 0x20, 0x24, 0x30, 0xFF });

        // Upload uniforms
        SetShaderValue(sdfShader, cameraPositionLocation,
                       cameraPosition, UNIFORM_VEC3);
        SetShaderValue(sdfShader, cameraRotationLocation,
                       cameraRotation, UNIFORM_VEC3);
        SetShaderValue(sdfShader, cameraFieldOfViewLocation,
                       &fieldOfView, UNIFORM_FLOAT);
        SetShaderValue(sdfShader, lightsStageLocation,
                       &lightsStage, UNIFORM_INT);

        // Draw the scene (to the render texture)
        BeginTextureMode(targetTex);
        BeginShaderMode(sdfShader);
        DrawRectangle(0, 0, VIRTUAL_SCREEN_HEIGHT * 2, VIRTUAL_SCREEN_HEIGHT,
                      (Color){0xFF, 0x00, 0xFF, 0xFF});
        EndShaderMode();
        EndTextureMode();

        // Draw the render texture to the screen
        if (!firstGameRenderDone) {
            // Sorry about this hack, the idea is as following:

            // 1. Show warning about noise and flashing graphics
            // 2. Render 1 frame of the game    <- you're here
            // 3. Render main menu, with the frame rendered in step 2 in the bg
            // 4. Continue as normal

            // As you might be able to guess, this branch exists to
            // make step 2 not cause a flicker. Yes, this should be
            // done in a more elegant way, but honestly, this is like,
            // one of the last things I'm going to write in this
            // codebase. You'll live.

            firstGameRenderDone = true;
        } else {
            DrawGameView(targetTex.texture);
        }

        // Narration text display
        int screenHeight = GetScreenHeight();
        float fontSize = screenHeight / 240.0f * 12.0f;
        if (narrationStage >= 0 && narrationStage < COMMENTS_COUNT
            && narrationEnabled) {
            float narrationTime = cameraPosition[2] - narrationStartZ;
            int linesPerScreen = 2;
            int lineIndex = GetLine(narrationTime, narrationStage,
                                    linesPerScreen);
            if (lineIndex != -1) {
                float y = screenHeight * 0.9f - fontSize;
                for (int i = 0; i < linesPerScreen; i++) {
                    int index = lineIndex + i;
                    if (index >= 0 && index < COMMENT_LINES) {
                        const char *line =
                            narratorComments[narrationStage][index];
                        DisplaySubtitle(*fontSetting.currentFont,
                                        line, fontSize, y);
                        y += fontSize;
                    }
                }
            }
        }

        // Warning for the player that they're going backwards
        if (backtracking && forwardDotMovement < 0.0) {
            DisplaySubtitle(*fontSetting.currentFont,
                            "Warning: You're going the wrong way.",
                            fontSize, 50.0f);
        }

        if (showMetersWalked) {
            const char *text = TextFormat("%4.0fm", metersWalked);
            DrawTextEx(*fontSetting.currentFont, text,
                       (Vector2){ 30.0f, 30.0f },
                       fontSize, 0.0f, YELLOW);
        }

        if (IsKeyDown(KEY_F3)) {
            DrawFPS(50, 50);
        }

        EndDrawing();
    }

    UnloadRenderTexture(targetTex);
    UnloadTexture(targetTex.texture);
    UnloadShader(sdfShader);
    UnloadFont(openSansFont);
    UnloadFont(vt323Font);

    CloseWindow();
    return 0;
}

bool FileMissing(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return true;
    }
    fclose(file);
    return false;
}

void DrawWarningText(const char *text, int fontSize, int y, Color color) {
    int height = GetScreenHeight();
    fontSize = fontSize * height / 480;
    y = y * height / 480;
    int width = MeasureText(text, fontSize);
    int x = (GetScreenWidth() - width) / 2;
    DrawText(text, x, y, fontSize, color);
}

bool EnsureResourcesExist(void) {
    bool fileLoaded[RESOURCE_COUNT] = { false };
    bool missingFiles = true;
    double lastFileCheck = -1.0;
    while (missingFiles) {
        if (WindowShouldClose()) {
            return true;
        }

        double time = GetTime();
        if (time > lastFileCheck + 0.1) {
            lastFileCheck = time;
            missingFiles = false;
            for (int i = 0; i < RESOURCE_COUNT; i++) {
                bool missing = FileMissing(resourcePaths[i]);
                fileLoaded[i] = !missing;
                missingFiles |= missing;
            }
        } else {
            BeginDrawing();
            ClearBackground((Color){ 0x44, 0x11, 0x11, 0xFF });
            Color textColor = (Color){ 0xEE, 0xEE, 0x88, 0xFF };
            DrawWarningText("Missing files:", 64, 100, textColor);
            int warningY = 200;
            for (int i = 0; i < RESOURCE_COUNT; i++) {
                if (!fileLoaded[i]) {
                    DrawWarningText(resourcePaths[i], 32, warningY, textColor);
                    warningY += 48;
                }
            }
            EndDrawing();
        }
    }
    return false;
}

static Shader LoadSDFShaderWithVersion(char* versionString) {
    // Should this be freed?
    char* rawShaderCode = LoadText(resourcePaths[RESOURCE_SHADER]);

    // Memory: Version string + \n + shader code + \0
    int len = strlen(versionString) + 1 + strlen(rawShaderCode) + 1;

    char* shaderCode = (char *)malloc(len);
    snprintf(shaderCode, len, "%s\n%s", versionString, rawShaderCode);
    Shader shader = LoadShaderCode(0, shaderCode);

    // The shader code memory can be freed after use, because LoadShaderCode
    // just passes the code to glShaderSource, whose documentation
    // explicitly states that the original memory can be freed,
    // since it is copied.
    free(shaderCode);
    return shader;
}

Shader LoadSDFShader(void) {
    int glVersion = rlGetVersion();
    Shader shader;
    if (glVersion == OPENGL_11) {
        printf("ERROR: OpenGL 1.1 is not supported.\n");
        shader = LoadShader(0, 0);
    } else if (glVersion == OPENGL_33) {
        shader = LoadSDFShaderWithVersion("#version 330");
    } else if (glVersion == OPENGL_ES_20) {
        shader = LoadSDFShaderWithVersion("#version 100 es");
    } else {
        shader = LoadSDFShaderWithVersion("#version 120");
    }
    return shader;
}

bool ShowEpilepsyWarning(FontSetting *fontSetting) {
    float time = (float)GetTime();
    float showPromptTime = time + 1.0f;
    float showPromptFadeDuration = 0.3f;
    float acceptPromptTime = time + 0.5f;

    while (GetTime() < acceptPromptTime || IsKeyUp(KEY_SPACE)) {
        if (WindowShouldClose()) {
            return true;
        }

        time = (float)GetTime();

        BeginDrawing();
        ClearBackground((Color){ 0x20, 0x24, 0x30, 0xFF });
        Color textColor = (Color){ 0xEE, 0xEE, 0xEE, 0xFF };
        float x = (GetScreenWidth() - 640.0f) / 2.0f + 180.0f;
        float y = (GetScreenHeight() - 480.0f) / 2.0f + 400.0f;

        if (time > showPromptTime) {
            float progress =
                Clamp((time - showPromptTime) / showPromptFadeDuration, 0, 1);
            char alpha = (char)(progress * 255);
            DrawTextEx(fontSetting->clearFont, "Press Space to continue",
                       (Vector2){ x, y }, 32, 0.0f,
                       (Color){ textColor.r, textColor.g, textColor.b, alpha });
        }

        x += 20.0f;
        y -= 300.0f;
        DrawTextEx(fontSetting->clearFont, "Warning:",
                   (Vector2){ x, y }, 72, 0.0f, textColor);
        x -= 196.0f;
        y += 100.0f;
        DrawTextEx(fontSetting->clearFont,
                   "The following experience contains noisy and flashing",
                   (Vector2){ x, y }, 36, 0.0f, textColor);
        x -= 1.0f;
        y += 40.0f;
        DrawTextEx(fontSetting->clearFont,
                   "graphics. If this sounds uncomfortable to you, please",
                   (Vector2){ x, y }, 36, 0.0f, textColor);
        x -= 25.0f;
        y += 40.0f;
        DrawTextEx(fontSetting->clearFont,
                   "exit the experience by closing the window or pressing F4.",
                   (Vector2){ x, y }, 36, 0.0f, textColor);
        EndDrawing();
    }
    return false;
}

Vector3 GetLegalPlayerMovement(Vector3 position, Vector3 movement, float maxDistance) {
    Vector3d newPos = Vector3dAdd(FromVector3(position), FromVector3(movement));
    Vector3d transformedPos = TransformToMetroSpaceD(newPos, maxDistance);
    if (transformedPos.x > -1.5f && transformedPos.x < 1.5f) {
        return ToVector3(newPos);
    }
    return position;
}

float NoiseifyPosition(float position) {
    return position + (int)(position * 4.1) % 14 - 7;
}

void DisplaySubtitle(Font font, const char *subtitle, float fontSize, float y) {
    Vector2 size = MeasureTextEx(font, subtitle, fontSize, 0.0f);
    Vector2 position = { (GetScreenWidth() - (size.x - fontSize)) / 2.0f, y };
    DrawTextEx(font, subtitle, position, fontSize, 0.0f, YELLOW);
}

int GetLine(float narrationTime, int narrationStage, int linesPerScreen) {
    float timeCounter = 0.0f;
    const char **lines = narratorComments[narrationStage];
    int lineIndex = 0;
    for (; lineIndex < COMMENT_LINES; lineIndex += linesPerScreen) {
        for (int j = 0; j < linesPerScreen; j++) {
            const char *line = lines[lineIndex + j];
            timeCounter += (float)strlen(line) * METERS_PER_CHARACTER;
        }
        if (narrationTime < timeCounter) {
            if (narrationTime > timeCounter - METERS_PER_CHARACTER * 2.0f) {
                // Leave gaps between lines
                return -1;
            }
            break;
        }
    }
    return lineIndex;
}
