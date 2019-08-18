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

double Smoothstep(double x) {
    x = x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x);
    return x * x * (3.0f - 2.0f * x);
}

float GetXOffset(float z, float maxDistance) {
    float x = z / maxDistance;
    return -sinf(x * 6.2831853f - 1.2f) * (float)Smoothstep(x * 2.0f - 0.3f)
        * 0.07f * maxDistance;
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


/* Double precision versions of the above functions for more accurate collisions */
typedef struct {
    double x;
    double y;
    double z;
} Vector3d;

Vector3d FromVector3(Vector3 vec) {
    return (Vector3d){ vec.x, vec.y, vec.z };
}

Vector3 ToVector3(Vector3d vec) {
    return (Vector3){ (float)vec.x, (float)vec.y, (float)vec.z };
}

Vector3d Vector3dScale(Vector3d vec, double scale) {
    return (Vector3d){ vec.x * scale, vec.y * scale, vec.z * scale };
}

Vector3d Vector3dAdd(Vector3d a, Vector3d b) {
    return (Vector3d){ a.x + b.x, a.y + b.y, a.z + b.z };
}

Vector3d Vector3dNormalize(Vector3d vec) {
    double length = sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    return (Vector3d){ vec.x / length, vec.y / length, vec.z / length };
}

double GetXOffsetD(double z, double maxDistance) {
    double x = z / maxDistance;
    return -sin(x * 6.2831853 - 1.2) * Smoothstep(x * 2.0 - 0.3)
        * 0.07 * maxDistance;
}

Vector3d GetPathNormalD(Vector3d samplePos, double maxDistance) {
    double currentXOffset = GetXOffsetD(samplePos.z, maxDistance);
    double nextXOffset = GetXOffsetD(samplePos.z + 0.001f, maxDistance);
    return Vector3dNormalize((Vector3d){ -0.001, 0.0,
                nextXOffset - currentXOffset });
}

Vector3d TransformToMetroSpaceD(Vector3d samplePos, double maxDistance) {
    Vector3d normal = GetPathNormalD(samplePos, maxDistance);
    double originalX = samplePos.x;
    samplePos.x = -GetXOffsetD(samplePos.z, maxDistance);
    samplePos = Vector3dAdd(samplePos, Vector3dScale(normal, originalX));
    return samplePos;
}

#endif
