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

const int VIRTUAL_SCREEN_HEIGHT = 240;
const float WALK_SPEED = 1.4;
const float HEAD_BOB_MAGNITUDE = 0.03;
const float HEAD_BOB_FREQUENCY = 1.6;

Rectangle GetRenderSrc(int screenWidth, int screenHeight);
Rectangle GetRenderDest(int screenWidth, int screenHeight);

int main(void) {
    float cameraPosition[] = { 0.0, 1.75, 0.0 };
    float cameraRotation[] = { 0.0, 0.0, 0.0 };
    float walkingTime = 0.0;
    float headBobAmount = 0.0;

    // Runtime configurable options
    float bobbingIntensity = 1.0;

    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(640, 480, "HEL Underground");

    // TODO: For Raspberry Pi compatibility, add OpenGL ES version of the shader
    // (it's probably enough to just load the shaders and switch
    // around the #version to "100 es")

    // TODO: Add an error message state for missing shader file
    // ("you seem to be missing sdf_shader.glsl! It should've come
    // with the download." or something)

    Shader sdfShader = LoadShader(0, "sdf_shader.glsl");
    int resolutionLocation = GetShaderLocation(sdfShader, "resolution");
    int cameraPositionLocation = GetShaderLocation(sdfShader, "cameraPosition");
    int cameraRotationLocation = GetShaderLocation(sdfShader, "cameraRotation");

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
        // TODO: Mouselook
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

        // Walk
        float r = cameraRotation[1] * DEG2RAD;
        bool walking = false;
        if (IsKeyDown(KEY_W)) {
            cameraPosition[0] += delta * WALK_SPEED * sinf(r);
            cameraPosition[2] += delta * WALK_SPEED * cosf(r);
            walking = true;
        }
        if (IsKeyDown(KEY_S)) {
            cameraPosition[0] -= delta * WALK_SPEED * sinf(r);
            cameraPosition[2] -= delta * WALK_SPEED * cosf(r);
            walking = true;
        }
        if (IsKeyDown(KEY_D)) {
            cameraPosition[0] += delta * WALK_SPEED * cosf(r);
            cameraPosition[2] += delta * WALK_SPEED * -sinf(r);
            walking = true;
        }
        if (IsKeyDown(KEY_A)) {
            cameraPosition[0] -= delta * WALK_SPEED * cosf(r);
            cameraPosition[2] -= delta * WALK_SPEED * -sinf(r);
            walking = true;
        }
        if (walking) {
            walkingTime += delta;
        } else {
            walkingTime = 0;
        }

        headBobAmount = Lerp(headBobAmount, sinf(walkingTime * 6.28 * HEAD_BOB_FREQUENCY) *
                             HEAD_BOB_MAGNITUDE * bobbingIntensity, 10.0 * delta);

        // Crouch
        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            cameraPosition[1] = Lerp(cameraPosition[1], 0.9, 10.0 * delta);
        } else {
            cameraPosition[1] = Lerp(cameraPosition[1], 1.75, 10.0 * delta);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        // Upload uniforms
        cameraPosition[1] += headBobAmount;
        SetShaderValue(sdfShader, cameraPositionLocation, cameraPosition, UNIFORM_VEC3);
        cameraPosition[1] -= headBobAmount;
        SetShaderValue(sdfShader, cameraRotationLocation, cameraRotation, UNIFORM_VEC3);

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

        EndDrawing();
    }

    UnloadRenderTexture(targetTex);
    UnloadTexture(targetTex.texture);
    UnloadShader(sdfShader);

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
