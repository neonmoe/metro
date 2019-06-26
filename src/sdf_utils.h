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

float GetXOffset(Vector3 samplePos, float maxDistance) {
    float funcX = samplePos.z / maxDistance;
    return -sinf(funcX * 3.14159 * 1.6) * 0.1 * pow(funcX, 1.5) * 2.0 * maxDistance;
}
Vector3 GetPathNormal(Vector3 samplePos, float maxDistance) {
    float sampleXOffset = GetXOffset(samplePos, maxDistance);
    Vector3 a = { samplePos.x + sampleXOffset, samplePos.y, samplePos.z };
    Vector3 b = { samplePos.x + GetXOffset((Vector3){ samplePos.x, samplePos.y, samplePos.z + 0.001f }, maxDistance),
                  samplePos.y, samplePos.z + 0.001f};
    Vector3 forward = Vector3Normalize(Vector3Subtract(b, a));
    return Vector3CrossProduct(forward, (Vector3){ 0.0, 1.0, 0.0 });
}
Vector3 TransformFromMetroSpace(Vector3 samplePos, float maxDistance) {
    Vector3 normal = GetPathNormal(samplePos, maxDistance);
    float originalX = samplePos.x;
    samplePos.x = GetXOffset(samplePos, maxDistance);
    samplePos = Vector3Subtract(samplePos, Vector3Scale(normal, originalX));
    return samplePos;
}
Vector3 TransformToMetroSpace(Vector3 samplePos, float maxDistance) {
    Vector3 normal = GetPathNormal(samplePos, maxDistance);
    float originalX = samplePos.x;
    samplePos.x = -GetXOffset(samplePos, maxDistance);
    samplePos = Vector3Add(samplePos, Vector3Scale(normal, originalX));
    return samplePos;
}

#endif
