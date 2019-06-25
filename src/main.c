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

#include "raylib.h"
#include "raymath.h"
#include "math.h"
#include "script.h"

const int VIRTUAL_SCREEN_HEIGHT = 256;

const float WALK_SPEED = 1.4;
const float HEAD_BOB_MAGNITUDE = 0.05;
const float HEAD_BOB_FREQUENCY = 1.6;
const float COMMENT_LENGTH = 28.0;

const float SUBTITLE_DURATION = 6.0;

Rectangle GetRenderSrc(int screenWidth, int screenHeight);
Rectangle GetRenderDest(int screenWidth, int screenHeight);
float NoiseifyPosition(float position);

int main(void) {
    // Player values
    float cameraPosition[] = { 0.0, 1.75, 0.0 };
    float cameraRotation[] = { 0.0, 0.0, 0.0 };
    float walkingTime = 0.0;
    float headBobAmount = 0.0;
    bool autoMove = false;

    // Mouselook values
    float mouseX = -1.0;
    float mouseY = -1.0;
    bool mouseLookEnabled = false;

    // Progress values
    int lightsStage = 0;
    int narrationStage = -1;
    float narrationTime = 0.0;

    // Runtime configurable options
    float bobbingIntensity = 1.0;
    Vector2 mouseSensitivity = (Vector2){150.0, 150.0};

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 480, "HEL Underground");
    SetExitKey(KEY_F4);

    // TODO: For Raspberry Pi compatibility, add OpenGL ES version of the shader
    // (it's probably enough to just load the shaders and switch
    // around the #version to "100 es")

    // TODO: Add an error message state for missing shader file
    // ("you seem to be missing sdf_shader.glsl! It should've come
    // with the download." or something)

    Font mainFont = LoadFontEx("vt323.ttf", 72, 0, 0);

    Shader sdfShader = LoadShader(0, "sdf_shader.glsl");
    int resolutionLocation = GetShaderLocation(sdfShader, "resolution");
    int cameraPositionLocation = GetShaderLocation(sdfShader, "cameraPosition");
    int cameraRotationLocation = GetShaderLocation(sdfShader, "cameraRotation");
    int lightsStageLocation = GetShaderLocation(sdfShader, "stage");

    RenderTexture2D targetTex = LoadRenderTexture(VIRTUAL_SCREEN_HEIGHT * 2,
                                                  VIRTUAL_SCREEN_HEIGHT);
    float resolution[] = { VIRTUAL_SCREEN_HEIGHT * 2, VIRTUAL_SCREEN_HEIGHT };
    SetShaderValue(sdfShader, resolutionLocation, resolution, UNIFORM_VEC2);

    float lastTime = (float)GetTime();
    while (!WindowShouldClose()) {
        float currentTime = (float)GetTime();
        float delta = currentTime - lastTime;
        lastTime = currentTime;

        // Configuration keys
        if (IsKeyPressed(KEY_B)) {
            // Toggle bobbing
            bobbingIntensity = bobbingIntensity > 0.5 ? 0.0 : 1.0;
        }

        // Turn around
        if (IsKeyDown(KEY_LEFT)) {
            cameraRotation[1] -= delta * 120.0;
            if (cameraRotation[1] < 0.0) {
                cameraRotation[1] += 360;
            }
        }
        if (IsKeyDown(KEY_RIGHT)) {
            cameraRotation[1] += delta * 120.0;
            if (cameraRotation[1] > 360) {
                cameraRotation[1] -= 360;
            }
        }
        if (IsKeyDown(KEY_UP)) {
            cameraRotation[0] -= delta * 90.0;
            if (cameraRotation[0] < -90.0) {
                cameraRotation[0] = -90.0;
            }
        }
        if (IsKeyDown(KEY_DOWN)) {
            cameraRotation[0] += delta * 90.0;
            if (cameraRotation[0] > 90.0) {
                cameraRotation[0] = 90.0;
            }
        }

        // Mouselook
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            mouseLookEnabled = true;
            DisableCursor();
        }
        if (IsKeyPressed(KEY_ESCAPE)) {
            mouseLookEnabled = false;
            EnableCursor();
        }
        if (mouseLookEnabled && mouseX >= 0 && mouseY >= 0) {
            float deltaX = (GetMouseX() - mouseX) /
                GetScreenHeight() * mouseSensitivity.x;
            float deltaY = (GetMouseY() - mouseY) /
                GetScreenHeight() * mouseSensitivity.y;
            mouseX = GetMouseX();
            mouseY = GetMouseY();
            cameraRotation[0] = Clamp(cameraRotation[0] + deltaY, -90.0, 90.0);
            cameraRotation[1] += deltaX;
        } else {
            mouseX = GetMouseX();
            mouseY = GetMouseY();
        }

        // Walk
        float r = cameraRotation[1] * DEG2RAD;
        bool walking = autoMove;
        if (IsKeyPressed(KEY_Q)) {
            autoMove = !autoMove;
        }
        if (autoMove) {
            cameraPosition[0] += delta * WALK_SPEED * sinf(r);
            cameraPosition[2] += delta * WALK_SPEED * cosf(r);
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_I)) {
            cameraPosition[0] += delta * WALK_SPEED * sinf(r);
            cameraPosition[2] += delta * WALK_SPEED * cosf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_K)) {
            cameraPosition[0] -= delta * WALK_SPEED * sinf(r);
            cameraPosition[2] -= delta * WALK_SPEED * cosf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_L)) {
            cameraPosition[0] += delta * WALK_SPEED * cosf(r);
            cameraPosition[2] += delta * WALK_SPEED * -sinf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_J)) {
            cameraPosition[0] -= delta * WALK_SPEED * cosf(r);
            cameraPosition[2] -= delta * WALK_SPEED * -sinf(r);
            walking = true;
            autoMove = false;
        }
        float maxDistance = COMMENT_LENGTH * COMMENTS_COUNT;
        cameraPosition[0] = Clamp(cameraPosition[0], -1.9, 1.9);
        cameraPosition[2] = Clamp(cameraPosition[2], -10.0, maxDistance + 10.0);
        if (walking) {
            walkingTime += delta;
        } else {
            walkingTime = 0;
        }

        // Crouch and bob
        cameraPosition[1] -= headBobAmount;
        float targetBob = sinf(walkingTime * 6.28 * HEAD_BOB_FREQUENCY) *
            HEAD_BOB_MAGNITUDE * bobbingIntensity;
        headBobAmount = Lerp(headBobAmount, targetBob, 10.0 * delta);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            cameraPosition[1] = Lerp(cameraPosition[1], 0.9, 10.0 * delta);
        } else {
            cameraPosition[1] = Lerp(cameraPosition[1], 1.75, 10.0 * delta);
        }
        cameraPosition[1] += headBobAmount;

        // Activate location-based actions
        // TODO: Add a fence or something at the end.
        float lightMaxDistance = maxDistance - 9;
        float triggerPosition = Clamp(NoiseifyPosition(cameraPosition[2]),
                                      0, lightMaxDistance);
        if (triggerPosition > (lightsStage + 1) * 9) {
            lightsStage++;
        }
        if (cameraPosition[2] > (narrationStage + 1) * COMMENT_LENGTH + 6) {
            narrationStage++;
            narrationTime = 0.0;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // Upload uniforms
        SetShaderValue(sdfShader, cameraPositionLocation,
                       cameraPosition, UNIFORM_VEC3);
        SetShaderValue(sdfShader, cameraRotationLocation,
                       cameraRotation, UNIFORM_VEC3);
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
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        DrawTexturePro(targetTex.texture,
                       GetRenderSrc(screenWidth, screenHeight),
                       GetRenderDest(screenWidth, screenHeight),
                       (Vector2){0.0, 0.0}, 0.0, WHITE);

        // Narration text display
        narrationTime += delta;
        int lineIndex = narrationTime / SUBTITLE_DURATION;
        if (// Leave a break for the last 10% of the subtitle display time
            narrationTime / SUBTITLE_DURATION - lineIndex < 0.9 &&
            // LineIndex is within bounds
            lineIndex >= 0 && lineIndex < COMMENT_LINES &&
            // NarrationStage is within bounds
            narrationStage >= 0 && narrationStage < COMMENTS_COUNT) {
            float fontSize = screenHeight / 240.0 * 12.0;
            const char *line = narratorComments[narrationStage][lineIndex];
            Vector2 size = MeasureTextEx(mainFont, line, fontSize, 0.0);
            Vector2 position = { (screenWidth - (size.x - fontSize)) / 2.0,
                                 screenHeight * 0.9 - fontSize };
            DrawTextEx(mainFont, line, position, fontSize, 0.0, YELLOW);
        }

        if (IsKeyDown(KEY_F3)) {
            DrawFPS(50, 50);
        }

        EndDrawing();
    }

    UnloadRenderTexture(targetTex);
    UnloadTexture(targetTex.texture);
    UnloadShader(sdfShader);
    UnloadFont(mainFont);

    CloseWindow();
    return 0;
}

Rectangle GetRenderSrc(int screenWidth, int screenHeight) {
    float ratio = (float)screenWidth / (float)screenHeight;
    if (ratio < 2.0) {
        int margin = (VIRTUAL_SCREEN_HEIGHT * (2.0 - ratio)) / 2.0;
        return (Rectangle){margin, 0,
                VIRTUAL_SCREEN_HEIGHT * ratio, VIRTUAL_SCREEN_HEIGHT};
    } else {
        return (Rectangle){0, 0,
                VIRTUAL_SCREEN_HEIGHT * 2, VIRTUAL_SCREEN_HEIGHT};
    }
}

Rectangle GetRenderDest(int screenWidth, int screenHeight) {
    float ratio = (float)screenWidth / (float)screenHeight;
    if (ratio < 2.0) {
        return (Rectangle){0, 0, screenWidth, screenHeight};
    } else {
        int margin = (screenWidth - screenHeight * 2) / 2;
        return (Rectangle){margin, 0,
                screenHeight * 2, screenHeight};
    }
}

float NoiseifyPosition(float position) {
    return position + (int)(position * 4.1) % 14 - 7;
}
