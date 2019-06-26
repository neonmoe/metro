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

#version 120

// Just in case, for ES 2.0
precision highp float;

// Ray marching variables (affect performance a lot)
#define RAY_STEPS_MAX 128
#define SDF_SURFACE_THRESHOLD 0.006
#define NORMAL_EPSILON 0.001

// 3D environment defining variables
// TODO: Make a better palette
#define COLOR_LIGHT_OFF vec3(141.0 / 255.0, 189.0 / 255.0, 168.0 / 255.0)
#define COLOR_LIGHT_ON (COLOR_LIGHT_OFF * 5.0)
#define COLOR_WOOD vec3(184.0 / 255.0, 140.0 / 255.0, 90.0 / 255.0)
#define COLOR_RAIL vec3(126.0 / 255.0, 123.0 / 255.0, 134.0 / 255.0)
#define COLOR_TUNNEL vec3(113.0 / 255.0, 113.0 / 255.0, 99.0 / 255.0)

struct Camera {
    vec3 position;
    vec3 direction;
};

struct SDFSample {
    float distance;
    vec3 color;
};

varying vec4 position;

uniform vec2 resolution;
uniform vec3 cameraPosition;
uniform vec3 cameraRotation;

uniform int stage = 0;
uniform float maxDistance = 100.0;

// The Ruoholahti-Lauttasaari line on page 41 of this pdf:
// https://www.hel.fi/hel2/ksv/Aineistot/maanalainen/Maanalaisen_yleiskaavan_selostus.pdf
// Looks kinda like the following curve in the range 0-1: -sin(x*pi*1.6)0.1x^(1.5)*2
// (where 0 is the Ruoholahti station, and 1 is the Lauttasaari one)
float getXOffset(vec3 samplePos) {
    float funcX = samplePos.z / maxDistance;
    return -sin(funcX * 3.14159 * 1.6) * 0.1 * pow(funcX, 1.5) * 2.0 * maxDistance;
}

vec3 getPathNormal(vec3 samplePos) {
    float sampleXOffset = getXOffset(samplePos);
    vec3 a = vec3(samplePos.x + sampleXOffset, samplePos.yz);
    vec3 b = vec3(samplePos.x + getXOffset(vec3(samplePos.xy, samplePos.z + 0.001)), samplePos.y, samplePos.z + 0.001);
    vec3 forward = normalize(b - a);
    return cross(forward, vec3(0.0, 1.0, 0.0));
}

vec3 transformFromMetroSpace(vec3 samplePos) {
    vec3 normal = getPathNormal(samplePos);
    float originalX = samplePos.x;
    samplePos.x = getXOffset(samplePos);
    samplePos -= normal * originalX;
    return samplePos;
}

vec3 transformToMetroSpace(vec3 samplePos) {
    vec3 normal = getPathNormal(samplePos);
    float originalX = samplePos.x;
    samplePos.x = -getXOffset(samplePos);
    samplePos += normal * originalX;
    return samplePos;
}

vec4 rotate_x(vec4 direction, float r) {
    float c = cos(r);
    float s = sin(r);
    mat4 transform = mat4(vec4(1.0, 0.0, 0.0, 0.0),
                          vec4(0.0, c, s, 0.0),
                          vec4(0.0, -s, c, 0.0),
                          vec4(0.0, 0.0, 0.0, 1.0));
    return transform * direction;
}

vec4 rotate_y(vec4 direction, float r) {
    float c = cos(r);
    float s = sin(r);
    mat4 transform = mat4(vec4(c, 0.0, -s, 0.0),
                          vec4(0.0, 1.0, 0.0, 0.0),
                          vec4(s, 0.0, c, 0.0),
                          vec4(0.0, 0.0, 0.0, 1.0));
    return transform * direction;
}

/* SDFs: https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm */

float sdfSphere(vec3 samplePos, vec3 position, float radius) {
    return length(position - samplePos) - radius;
}

float sdfBox(vec3 samplePos, vec3 center, vec3 extents) {
  vec3 d = abs(center - samplePos) - extents;
  return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float sdfRoundedBox(vec3 samplePos, vec3 center, vec3 extents, float radius) {
  vec3 d = abs(center - samplePos) - extents;
  return length(max(d, 0.0)) - radius + min(max(d.x, max(d.y, d.z)), 0.0);
}

SDFSample sdfRails(vec3 samplePos) {
    vec3 period = vec3(0.0, 0.0, 1.0);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    repeatedSample.x = abs(repeatedSample.x);
    float distance = sdfBox(repeatedSample, vec3(0.762, 0.2, 0.0), vec3(0.07, 0.1, 0.5));
    float subDistance = min(sdfBox(repeatedSample, vec3(0.762 - 0.07, 0.2, 0.0), vec3(0.05, 0.06, 0.5)),
                            sdfBox(repeatedSample, vec3(0.762 + 0.07, 0.2, 0.0), vec3(0.05, 0.06, 0.5)));
    return SDFSample(distance + max(0.0, -subDistance), COLOR_RAIL);
}

SDFSample sdfTunnel(vec3 samplePos) {
    vec3 period = vec3(0.0, 0.0, 1.0);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    repeatedSample.x = abs(repeatedSample.x);
    float distance = -sdfRoundedBox(repeatedSample, vec3(0.0, 2.0, 0.0), vec3(2.0, 2.0, 1.0), 0.1);
    return SDFSample(distance, COLOR_TUNNEL);
}

SDFSample sdfRailPlanks(vec3 samplePos) {
    vec3 period = vec3(0.0, 0.0, 1.0);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    repeatedSample.x = abs(repeatedSample.x);
    float distance = sdfBox(repeatedSample, vec3(0.0, 0.0, 0.0), vec3(1.0, 0.1, 0.2));
    return SDFSample(distance, COLOR_WOOD);
}

SDFSample sdfLightMeshes(vec3 samplePos) {
    vec3 period = vec3(0.0, 0.0, 9.0);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    float distance = sdfRoundedBox(repeatedSample, vec3(-1.8, 3.6, -1.0), vec3(0.2, 0.2, 0.3), 0.05);
    if (abs(floor(samplePos.z / 9.0) - stage) <= 1) {
        return SDFSample(distance, COLOR_LIGHT_ON);
    } else {
        return SDFSample(distance, COLOR_LIGHT_OFF);
    }
}

#define OBJECTS_COUNT 4
SDFSample sdf(vec3 samplePos, bool ignoreLightMeshes) {
    if (samplePos.z > 0) {
        samplePos = transformFromMetroSpace(samplePos);
    }

    SDFSample samples[OBJECTS_COUNT] = SDFSample[OBJECTS_COUNT]
        (ignoreLightMeshes ? SDFSample(10000.0, vec3(0.0, 0.0, 0.0)) : sdfLightMeshes(samplePos),
         sdfTunnel(samplePos),
         sdfRails(samplePos),
         sdfRailPlanks(samplePos));

    float lowestDistance = samples[0].distance;
    vec3 color = samples[0].color;
    for (int i = 1; i < OBJECTS_COUNT; i++) {
        if (samples[i].distance < lowestDistance) {
            lowestDistance = samples[i].distance;
            color = samples[i].color;
        }
    }
    return SDFSample(lowestDistance, color);
}

float get_fog(vec3 cam, vec3 position) {
    return min(1.0, pow(15.0 / length(cam - position), 1.5));
}

vec3 get_normal(vec3 samplePos) {
    float x = sdf(samplePos + vec3(NORMAL_EPSILON, 0.0, 0.0), false).distance -
        sdf(samplePos - vec3(NORMAL_EPSILON, 0.0, 0.0), false).distance;
    float y = sdf(samplePos + vec3(0.0, NORMAL_EPSILON, 0.0), false).distance -
        sdf(samplePos - vec3(0.0, NORMAL_EPSILON, 0.0), false).distance;
    float z = sdf(samplePos + vec3(0.0, 0.0, NORMAL_EPSILON), false).distance -
        sdf(samplePos - vec3(0.0, 0.0, NORMAL_EPSILON), false).distance;
    return normalize(vec3(x, y, z));
}

float get_shadow(vec3 samplePos, vec3 lightPos) {
    vec3 direction = normalize(lightPos - samplePos);
    vec3 position = samplePos + direction * 0.2;
    int steps = 1;
    for (; steps < 15; steps++) {
        SDFSample s = sdf(position, true);
        float maxDistance = length(lightPos - position);
        if (s.distance > maxDistance) {
            break;
        }

        if (s.distance < SDF_SURFACE_THRESHOLD) {
            return 1.0 - pow(float(steps) / 15.0, 1.5);
        } else {
            position += direction * s.distance;
        }
    }
    return 0.0;
}

// TODO: Add specularity
// Specular should probably be passed as a paramenter, and sourced from SDFSample
float get_brightness(vec3 samplePos, vec3 normal, float fog) {
    float ambient = 0.1;
    float diffuse = 0.0;
    for (int i = stage - 1; i <= stage + 1; i++) {
        vec3 lightPosition = transformToMetroSpace(vec3(0.0, 3.6, 3.5 + 9.0 * float(i)));
        vec3 lightDir = lightPosition - samplePos;
        float lightDistance = 11.0;
        diffuse += pow(1.0 - max(0.0, min(1.0, length(lightDir) / lightDistance)), 0.5) *
            max(0.0, dot(normal, normalize(lightDir))) *
            (1.0 - get_shadow(samplePos, lightPosition) * 0.75) * 0.35;
    }
    return min(1.0, diffuse) + ambient;
}

// TODO: Improve the AO algorithm, it's a bit dumb currently
float get_ambient_occlusion(vec3 samplePos, vec3 normal) {
    float step = 0.005;
    vec3 position = samplePos + normal * step;
    int steps = 0;
    for (; steps < 10; steps++) {
        SDFSample s = sdf(position, false);
        if (s.distance <= (float(steps) + 1.0) * step) {
            return 1.0 - float(steps) / 10.0;
        }
        position += min(s.distance, step);
    }
    return 0.0;
}

vec4 get_color(vec2 screenPosition, vec3 position, vec3 rotation) {
    // TODO: Configurable FoV
    float nearDistance = 0.5; // This is 90 degrees FoV
    vec3 direction = vec3(screenPosition.x, screenPosition.y, nearDistance);
    direction = normalize(direction);
    direction = rotate_x(vec4(direction, 1.0), radians(rotation.x)).xyz;
    direction = rotate_y(vec4(direction, 1.0), radians(rotation.y)).xyz;

    vec3 originalPosition = position;
    bool hit = false;
    vec3 normal = vec3(0.0, 0.0, 0.0);
    vec3 color = vec3(1.0, 1.0, 1.0);
    int steps = 1;
    for (; steps < RAY_STEPS_MAX; steps++) {
        SDFSample s = sdf(position, false);
        float distance = s.distance;
        if (distance < SDF_SURFACE_THRESHOLD) {
            hit = true;
            normal = get_normal(position);
            color = s.color;
            break;
        } else {
            position += direction * distance;
        }
    }

    if (hit) {
        float fog = get_fog(originalPosition, position);
        float brightness = get_brightness(position, normal, fog);
        float ambientOcclusion = 1.0 - get_ambient_occlusion(position, normal) * 0.5;
        return vec4(color * brightness * fog * ambientOcclusion, 1.0);
    } else {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }
}

void main() {
    // TODO: Ensure HiDPI compatibility

    // NOTE: The y coordinate is flipped because we're rendering to a render texture
    vec2 pixelCoords = vec2((gl_FragCoord.x - resolution.x / 2.0) / resolution.y,
                            (gl_FragCoord.y - resolution.y / 2.0) / resolution.y * -1.0);

    // TODO: Switch to render textures for lowering the resolution

    gl_FragColor = get_color(pixelCoords, cameraPosition, cameraRotation);
}
