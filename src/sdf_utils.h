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

/* This file contains a few functions from sdf_shader.glsl, translated
 * to C for usage in collision code. */

#ifndef SDF_UTILS
#define SDF_UTILS

#include <math.h>

#include "raymath.h"

float Smoothstep(float x) {
    x = Clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

float GetXOffset(float z, float maxDistance) {
    float x = z / maxDistance;
    return -sinf(x * 6.2831853f - 1.2f) * Smoothstep(x * 2.0f - 0.3f) *
        0.07f * maxDistance;
}
Vector3 GetPathNormal(Vector3 samplePos, float maxDistance) {
    float currentXOffset = GetXOffset(samplePos.z, maxDistance);
    float nextXOffset = GetXOffset(samplePos.z + 0.001f, maxDistance);
    return Vector3Normalize((Vector3){ -0.001f, 0.0f,
                nextXOffset - currentXOffset });
}
Vector3 TransformFromMetroSpace(Vector3 samplePos, float maxDistance) {
    Vector3 normal = GetPathNormal(samplePos, maxDistance);
    float originalX = samplePos.x;
    samplePos.x = GetXOffset(samplePos.z, maxDistance);
    samplePos = Vector3Subtract(samplePos, Vector3Scale(normal, originalX));
    return samplePos;
}
Vector3 TransformToMetroSpace(Vector3 samplePos, float maxDistance) {
    Vector3 normal = GetPathNormal(samplePos, maxDistance);
    float originalX = samplePos.x;
    samplePos.x = -GetXOffset(samplePos.z, maxDistance);
    samplePos = Vector3Add(samplePos, Vector3Scale(normal, originalX));
    return samplePos;
}

Vector3 GetPathForward(Vector3 samplePos, float maxDistance) {
    float currentXOffset = GetXOffset(samplePos.z, maxDistance);
    float nextXOffset = GetXOffset(samplePos.z + 0.001f, maxDistance);
    return Vector3Normalize((Vector3){ nextXOffset - currentXOffset,
                0.0f, 0.001f });
}

#endif
