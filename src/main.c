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

#include "raylib.h"
#include "raymath.h"
#include "script.h"
#include "sdf_utils.h"

#define VIRTUAL_SCREEN_HEIGHT 256

#define DEFAULT_MAX_DISTANCE 2150.0f
#define WALK_SPEED 1.4f
#define HEAD_BOB_MAGNITUDE 0.05f
#define HEAD_BOB_FREQUENCY 1.6f
#define COMMENT_LENGTH (DEFAULT_MAX_DISTANCE / COMMENTS_COUNT)

#define SUBTITLE_DURATION 15.0f

Rectangle GetRenderSrc(int screenWidth, int screenHeight);
Rectangle GetRenderDest(int screenWidth, int screenHeight);
float NoiseifyPosition(float position);
void DisplaySubtitle(Font font, const char *subtitle, float fontSize, float y);

int main(void) {
    // Player values
    float cameraPosition[] = { 0.0f, 1.75f, 0.0f };
    float cameraRotation[] = { 0.0f, 0.0f, 0.0f };
    float walkingTime = 0.0f;
    float headBobAmount = 0.0f;
    bool autoMove = false;

    // Mouselook values
    int mouseX = -1;
    int mouseY = -1;
    bool mouseLookEnabled = false;

    // Progress values
    int lightsStage = 0;
    int narrationStage = -1;
    float narrationTime = 0.0f;

    // Runtime configurable options
    float bobbingIntensity = 1.0f;
    int mouseSpeedX = 150;
    int mouseSpeedY = 150;

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
    int maxDistanceLocation = GetShaderLocation(sdfShader, "maxDistance");

    RenderTexture2D targetTex = LoadRenderTexture(VIRTUAL_SCREEN_HEIGHT * 2,
                                                  VIRTUAL_SCREEN_HEIGHT);
    float resolution[] = { (float)VIRTUAL_SCREEN_HEIGHT * 2,
                           (float)VIRTUAL_SCREEN_HEIGHT };
    SetShaderValue(sdfShader, resolutionLocation, resolution, UNIFORM_VEC2);
    float maxDistance = DEFAULT_MAX_DISTANCE;
    SetShaderValue(sdfShader, maxDistanceLocation, &maxDistance, UNIFORM_FLOAT);

    float lastTime = (float)GetTime();
    while (!WindowShouldClose()) {
        float currentTime = (float)GetTime();
        float delta = currentTime - lastTime;
        lastTime = currentTime;

        if (IsKeyDown(KEY_LEFT_SHIFT)) {
            delta *= 10.0f;
        }

        // Configuration keys
        if (IsKeyPressed(KEY_B)) {
            // Toggle bobbing
            bobbingIntensity = bobbingIntensity > 0.5f ? 0.0f : 1.0f;
        }

        // Turn around
        if (IsKeyDown(KEY_LEFT)) {
            cameraRotation[1] -= delta * 120.0f;
            if (cameraRotation[1] < 0.0) {
                cameraRotation[1] += 360.0f;
            }
        }
        if (IsKeyDown(KEY_RIGHT)) {
            cameraRotation[1] += delta * 120.0f;
            if (cameraRotation[1] > 360) {
                cameraRotation[1] -= 360.0f;
            }
        }
        if (IsKeyDown(KEY_UP)) {
            cameraRotation[0] -= delta * 90.0f;
            if (cameraRotation[0] < -90.0) {
                cameraRotation[0] = -90.0f;
            }
        }
        if (IsKeyDown(KEY_DOWN)) {
            cameraRotation[0] += delta * 90.0f;
            if (cameraRotation[0] > 90.0) {
                cameraRotation[0] = 90.0f;
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

        // Walk
        float r = cameraRotation[1] * DEG2RAD;
        bool walking = autoMove;
        Vector3 movement = { cameraPosition[0], 0.0f, cameraPosition[2] };
        if (IsKeyPressed(KEY_Q)) {
            autoMove = !autoMove;
        }
        if (autoMove) {
            movement.x += delta * WALK_SPEED * sinf(r);
            movement.z += delta * WALK_SPEED * cosf(r);
        }
        if (IsKeyDown(KEY_W) || IsKeyDown(KEY_I)) {
            movement.x += delta * WALK_SPEED * sinf(r);
            movement.z += delta * WALK_SPEED * cosf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_S) || IsKeyDown(KEY_K)) {
            movement.x -= delta * WALK_SPEED * sinf(r);
            movement.z -= delta * WALK_SPEED * cosf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_L)) {
            movement.x += delta * WALK_SPEED * cosf(r);
            movement.z += delta * WALK_SPEED * -sinf(r);
            walking = true;
            autoMove = false;
        }
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_J)) {
            movement.x -= delta * WALK_SPEED * cosf(r);
            movement.z -= delta * WALK_SPEED * -sinf(r);
            walking = true;
            autoMove = false;
        }

        Vector3 transformedPos = TransformToMetroSpace(movement, maxDistance);
        if (transformedPos.x > -1.8f && transformedPos.x < 1.8f) {
            cameraPosition[0] = movement.x;
            cameraPosition[2] = Clamp(movement.z, -10.0f, maxDistance + 10.0f);
        }

        if (walking) {
            walkingTime += delta;
        } else {
            walkingTime = 0.0f;
        }

        // Crouch and bob
        // TODO: Make the player a bit higher when on the rails/planks
        cameraPosition[1] -= headBobAmount;
        float targetBob = sinf(walkingTime * 6.28f * HEAD_BOB_FREQUENCY) *
            HEAD_BOB_MAGNITUDE * bobbingIntensity;
        headBobAmount = Lerp(headBobAmount, targetBob, 0.2f);
        bool onPlank = fabs(transformedPos.x) < 1.0;
        bool onRail = fabs(transformedPos.x) > 0.762 - 0.05 &&
            fabs(transformedPos.x) < 0.762 + 0.05;
        float height = onRail ? 0.3f : (onPlank ? 0.1f : 0.0f);
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            height += 0.9f;
        } else {
            height += 1.75f;
        }
        cameraPosition[1] = Lerp(cameraPosition[1], height, 10.0f * delta);
        cameraPosition[1] += headBobAmount;

        // Activate location-based actions
        // TODO: Add a fence or something at the end.
        float lightMaxDistance = maxDistance - 9.0f;
        float triggerPosition = Clamp(NoiseifyPosition(cameraPosition[2]),
                                      0.0f, lightMaxDistance);
        if (triggerPosition > (lightsStage + 1) * 9) {
            lightsStage++;
        }
        if (cameraPosition[2] > (narrationStage + 1) * COMMENT_LENGTH + 6) {
            narrationStage++;
            narrationTime = 0.0f;
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
                       (Vector2){ 0.0f, 0.0f }, 0.0f, WHITE);

        // Narration text display
        narrationTime += delta;
        int lineIndex = (int)(narrationTime / SUBTITLE_DURATION) * 2;
        if (// Leave a break for the last 10% of the subtitle display time
            narrationTime / SUBTITLE_DURATION - lineIndex < 0.9 &&
            // NarrationStage is within bounds
            narrationStage >= 0 && narrationStage < COMMENTS_COUNT) {
            float fontSize = screenHeight / 240.0f * 12.0f;
            float y = screenHeight * 0.9f - fontSize;
            for (int i = 0; i < 2; i++) {
                int index = lineIndex + i;
                if (index >= 0 && index < COMMENT_LINES) {
                    const char *line = narratorComments[narrationStage][index];
                    DisplaySubtitle(mainFont, line, fontSize, y + i * fontSize);
                }
            }
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
    float margin, width, height;
    if (ratio < 2.0) {
        margin = (VIRTUAL_SCREEN_HEIGHT * (2.0f - ratio)) / 2.0f;
        width = VIRTUAL_SCREEN_HEIGHT * ratio;
        height = (float)VIRTUAL_SCREEN_HEIGHT;
    } else {
        margin = 0.0f;
        width = VIRTUAL_SCREEN_HEIGHT * 2.0f;
        height = (float)VIRTUAL_SCREEN_HEIGHT;
    }
    return (Rectangle){margin, 0.0f, width, height};
}

Rectangle GetRenderDest(int screenWidth, int screenHeight) {
    float ratio = (float)screenWidth / (float)screenHeight;
    float margin, width, height;
    if (ratio < 2.0) {
        margin = 0.0f;
        width = (float)screenWidth;
        height = (float)screenHeight;
    } else {
        margin = (screenWidth - screenHeight * 2.0f) / 2.0f;
        width = screenHeight * 2.0f;
        height = (float)screenHeight;
    }
    return (Rectangle){margin, 0.0f, width, height};
}

float NoiseifyPosition(float position) {
    return position + (int)(position * 4.1) % 14 - 7;
}

void DisplaySubtitle(Font font, const char *subtitle, float fontSize, float y) {
    Vector2 size = MeasureTextEx(font, subtitle, fontSize, 0.0f);
    Vector2 position = { (GetScreenWidth() - (size.x - fontSize)) / 2.0f, y };
    DrawTextEx(font, subtitle, position, fontSize, 0.0f, YELLOW);
}
