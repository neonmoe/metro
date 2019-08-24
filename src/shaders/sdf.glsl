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

// If using this shader somewhere else, uncomment the following
// (this shader should work with pretty much any version of glsl)
// (this is commented because the metro game adds the version header in code)
// #version 120

// Ray marching variables (affect performance a lot)
#define RAY_STEPS_MAX 128
#define SDF_SURFACE_THRESHOLD 0.01
#define NORMAL_EPSILON 0.001

// 3D environment defining variables
// TODO: Make a better palette
#define COLOR_LIGHT_OFF vec3(141.0 / 255.0, 189.0 / 255.0, 168.0 / 255.0)
#define COLOR_LIGHT_ON (COLOR_LIGHT_OFF * 5.0)
#define COLOR_WOOD vec3(184.0 / 255.0, 140.0 / 255.0, 90.0 / 255.0)
#define COLOR_RAIL vec3(126.0 / 255.0, 123.0 / 255.0, 134.0 / 255.0)
#define COLOR_TUNNEL vec3(113.0 / 255.0, 113.0 / 255.0, 99.0 / 255.0)
#define COLOR_YELLOW_LINE vec3(184.0 / 255.0, 184.0 / 255.0, 90.0 / 255.0)
#define COLOR_DARKENED_PLATFORM vec3(93.0 / 255.0, 93.0 / 255.0, 79.0 / 255.0)
#define COLOR_DARKENED_ROOF vec3(30.0 / 255.0, 30.0 / 255.0, 30.0 / 255.0)
#define COLOR_STATION_LINING_WHITE vec3(270.0 / 255.0, 270.0 / 255.0, 270.0 / 255.0)
#define COLOR_STATION_LINING_RED vec3(270.0 / 255.0, 80.0 / 255.0, 80.0 / 255.0)
#define COLOR_STATION_LIGHTS vec3(300.0 / 255.0, 300.0 / 255.0, 300.0 / 255.0)
#define COLOR_DISPLAY_BACK vec3(93.0 / 255.0, 93.0 / 255.0, 79.0 / 255.0)
#define COLOR_DISPLAY_LIGHT vec3(120.0 / 255.0, 150.0 / 255.0, 270.0 / 255.0)

#define STATION_START_Z (maxDistance - 120.0)
#define STATION_WIDTH 16.0

struct Camera {
    vec3 position;
    vec3 direction;
};

struct SDFSample {
    float distance;
    vec3 color;
};

uniform vec2 resolution;
uniform vec3 cameraPosition;
uniform vec3 cameraRotation;
uniform float cameraFieldOfView;

uniform int stage = 0;
uniform float maxDistance = 100.0;

// The Ruoholahti-Lauttasaari line on page 41 of this pdf:
// https://www.hel.fi/hel2/ksv/Aineistot/maanalainen/Maanalaisen_yleiskaavan_selostus.pdf
// Looks like the following curve in the range 0-1:
//   -sin(x*pi*2-1.2)*smoothstep(x*2-0.3)*0.07
// (where 0 is the Ruoholahti station, and 1 is the Lauttasaari one)
// Also, based on roughly mapping out this distance on Google Maps,
// the length of the line is about 2.1 kilometers.

// Here starts the bits that need to be updated in sdf_utils.h
float getXOffset(float z) {
    float x = z / maxDistance;
    return -sin(x * 6.2831853 - 1.2) * smoothstep(0.0, 1.0, x * 2 - 0.3) * 0.07 * maxDistance;
}
vec3 getPathNormal(vec3 samplePos) {
    float currentXOffset = getXOffset(samplePos.z);
    float nextXOffset = getXOffset(samplePos.z + 0.001);
    return normalize(vec3(-0.001, 0.0, nextXOffset - currentXOffset));
}
vec3 transformFromMetroSpace(vec3 samplePos) {
    vec3 normal = getPathNormal(samplePos);
    float originalX = samplePos.x;
    samplePos.x = getXOffset(samplePos.z);
    samplePos -= normal * originalX;
    return samplePos;
}
vec3 transformToMetroSpace(vec3 samplePos) {
    vec3 normal = getPathNormal(samplePos);
    float originalX = samplePos.x;
    samplePos.x = -getXOffset(samplePos.z);
    samplePos += normal * originalX;
    return samplePos;
}
// Here ends the bits that need to be updated in sdf_utils.h

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

vec3 random(float x, float y) {
    return vec3(fract((sin(x * 5.362) + sin(y * 5.742)) * 589174.0),
                fract((sin(x * 5.822) + sin(y * 5.532)) * 591267.0),
                fract((sin(x * 5.746) + sin(y * 5.321)) * 586575.0));
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
    vec3 repeatedSample = mod(samplePos, period);
    float distance = sdfRoundedBox(repeatedSample, vec3(-1.8, 3.6, 1.0), vec3(0.2, 0.2, 0.3), 0.05);
    if (samplePos.z > STATION_START_Z && samplePos.z <= STATION_START_Z + 90.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    } else if (abs(floor(samplePos.z / 9.0) - stage) <= 1) {
        return SDFSample(distance, COLOR_LIGHT_ON);
    } else {
        return SDFSample(distance, COLOR_LIGHT_OFF);
    }
}

SDFSample sdfFence(vec3 samplePos) {
    float z = maxDistance - 15.0f;
    float signDistance = sdfBox(samplePos, vec3(0.0, 1.5, z),
                            vec3(1.9, 0.4, 0.2));
    float poleDistance = sdfBox(samplePos, vec3(-1.5, 0.75, z),
                                vec3(0.14, 1.5, 0.15));
    poleDistance = min(poleDistance,
                       sdfBox(samplePos, vec3(1.52, 0.7, z + 0.05),
                              vec3(0.14, 1.5, 0.15)));
    if (signDistance < poleDistance) {
        return SDFSample(signDistance, COLOR_WOOD);
    } else {
        return SDFSample(poleDistance, COLOR_RAIL);
    }
}

SDFSample sdfStation(vec3 samplePos) {
    float startZ = STATION_START_Z;
    float distance = -sdfRoundedBox(samplePos, vec3(-2.0 - STATION_WIDTH / 2.0, 4.5, startZ + 45.0),
                                    vec3(STATION_WIDTH / 2.0, 3.5, 45.0), 0.1);
    return SDFSample(distance, COLOR_TUNNEL);
}

SDFSample sdfStationBoxes(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ + 10.0 || samplePos.z >= startZ + 90.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    vec3 period = vec3(0.0, 0.0, 18.0);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    repeatedSample.x = abs(repeatedSample.x);
    float distance = sdfBox(repeatedSample, vec3(2.0 + STATION_WIDTH / 2.0, 2.5, 0.0), vec3(1.0, 1.6, 1.25));

    period = vec3(0.15, 0.15, 0.15);
    repeatedSample = mod(samplePos, period) - 0.5 * period;
    vec3 repeatedSampleX = repeatedSample;
    repeatedSampleX.z = 0;
    vec3 repeatedSampleZ = repeatedSample;
    repeatedSampleZ.x = 0;
    float holeDistanceX = sdfBox(repeatedSampleX, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0) * 0.03);
    float holeDistanceZ = sdfBox(repeatedSampleZ, vec3(0.0, 0.0, 0.0), vec3(1.0, 1.0, 1.0) * 0.03);

    return SDFSample(max(-holeDistanceZ, max(-holeDistanceX, distance)), COLOR_TUNNEL);
}

SDFSample sdfStationYellowLine(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ || samplePos.z >= startZ + 90.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    float centerPoint = STATION_WIDTH / 2.0 + 2.0;
    vec3 transformedSample = samplePos;
    transformedSample.x = abs(transformedSample.x + centerPoint) - centerPoint;
    float distance = sdfBox(transformedSample,
                            vec3(-4.75, 0.905, STATION_START_Z - 1.0),
                            vec3(0.15, 0.01, 92.0));
    return SDFSample(distance, COLOR_YELLOW_LINE);
}

SDFSample sdfStationDarkenedParts(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ + 4.0 || samplePos.z >= startZ + 86.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    float centerPoint = STATION_WIDTH / 2.0 + 2.0;
    vec3 transformedSample = samplePos;
    transformedSample.x = abs(transformedSample.x + centerPoint) - centerPoint;
    transformedSample.z = mod(transformedSample.z, 7.0);
    float distance = sdfBox(transformedSample,
                            vec3(-3.6, 0.9025, 0),
                            vec3(1.0, 0.005, 3.0));
    return SDFSample(distance, COLOR_DARKENED_PLATFORM);
}

SDFSample sdfStationDarkenedRoof(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ || samplePos.z >= startZ + 90.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    float distance = sdfBox(samplePos, vec3(-2.0 - STATION_WIDTH / 2.0, 8.0, startZ + 45.0),
                            vec3(STATION_WIDTH / 2.0, 0.05, 90.0));
    return SDFSample(distance, COLOR_DARKENED_ROOF);
}

SDFSample sdfStationBorderLights(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ || samplePos.z >= startZ + 90.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    float centerPointX = STATION_WIDTH / 2.0 + 2.0;
    float centerPointZ = startZ + 45.0;
    vec3 mirroredPos = samplePos;
    mirroredPos.x = abs(mirroredPos.x + centerPointX) - centerPointX;
    mirroredPos.z = abs(mirroredPos.z - centerPointZ) + centerPointZ;
    float distanceX = sdfBox(mirroredPos, vec3(-2.0, 4.3, startZ + 45.0), vec3(0.1, 0.2, 90.0));
    float distanceZ = sdfBox(mirroredPos, vec3(-2.0 - STATION_WIDTH / 2.0, 4.3, startZ + 90.0),
                             vec3(STATION_WIDTH / 2.0 + 0.1, 0.2, 0.1));
    vec3 color = samplePos.y > 4.3 ? COLOR_STATION_LINING_RED : COLOR_STATION_LINING_WHITE;
    return SDFSample(min(distanceX, distanceZ), color);
}

SDFSample sdfStationCeilingLights(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ + 5.0 || samplePos.z >= startZ + 85.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    vec3 rand = random(floor(samplePos.x / 1.5), floor(samplePos.z / 1.5));
    vec3 period = vec3(1.5, 0.0, 1.5);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    float distance = sdfBox(repeatedSample, vec3(floor(rand.x * 10.0) / 10.0 * 0.3,
                                                 floor(rand.y * 10.0) / 10.0 + 6.5,
                                                 floor(rand.z * 10.0) / 10.0 * 0.3),
                            vec3(0.3, 0.2, 0.3));
    float boundingBoxDistance = sdfBox(samplePos, vec3(-2.0 - STATION_WIDTH / 2.0, 6.5, startZ + 45.0),
                                       vec3(STATION_WIDTH / 2.0 - 3.0, 2.0, 41.0));
    return SDFSample(max(distance, boundingBoxDistance), COLOR_STATION_LIGHTS);
}

SDFSample sdfStationTrainDisplay(vec3 samplePos) {
    float startZ = STATION_START_Z;
    if (samplePos.z < startZ + 5.0 || samplePos.z >= startZ + 85.0) {
        return SDFSample(100000.0, vec3(0.0, 0.0, 0.0));
    }
    float centerPointX = STATION_WIDTH / 2.0 + 2.0;
    vec3 rand = random(floor(samplePos.x / 1.5), floor(samplePos.z / 1.5));
    vec3 period = vec3(0.0, 0.0, 10.0);
    vec3 repeatedSample = mod(samplePos, period) - 0.5 * period;
    repeatedSample.x = abs(repeatedSample.x + centerPointX) - centerPointX;
    float poleDistance = sdfBox(repeatedSample, vec3(-3.2, 5.8, 2.0),
                                vec3(1.1, 0.15, 0.15));
    float distance = sdfBox(repeatedSample, vec3(-3.3, 5.0, 2.0),
                            vec3(0.8, 0.5, 0.2));
    float displayDistance = sdfBox(repeatedSample, vec3(-3.3, 5.0, 2.0),
                                   vec3(0.65, 0.35, 0.21));
    if (displayDistance < distance) {
        return SDFSample(distance, COLOR_DISPLAY_LIGHT);
    } else {
        return SDFSample(min(distance, poleDistance), COLOR_DISPLAY_BACK);
    }
}

#define ROOM_COUNT 3
#define OBJECTS_COUNT 11
SDFSample sdf(vec3 samplePos, bool ignoreLightMeshes) {
    if (samplePos.z > 0) {
        samplePos = transformFromMetroSpace(samplePos);
    }

    // Currently the scene consists of:
    // - the tunnel
    // - the lights
    // - the rails
    // - fence at the end
    // - the planks below the rails
    // - the final station
    //   - the blocks
    //   - the yellow do not cross? line
    //   - darkened parts for entry to metro
    //   - dark roof
    //   - white/red lining along the roof
    //   - ceiling lights
    //   - train displays
    // Scene additions TODO:
    // - rocks/gravel
    SDFSample roomSamples[ROOM_COUNT] = SDFSample[ROOM_COUNT]
        (sdfTunnel(samplePos),
         sdfTunnel(samplePos + vec3(2.0 + STATION_WIDTH + 2.0, 0.0, 0.0)),
         sdfStation(samplePos));
    SDFSample samples[OBJECTS_COUNT] = SDFSample[OBJECTS_COUNT]
        (ignoreLightMeshes ? SDFSample(10000.0, vec3(0.0, 0.0, 0.0)) : sdfLightMeshes(samplePos),
         sdfRails(samplePos),
         sdfFence(samplePos),
         sdfRailPlanks(samplePos),
         sdfStationBoxes(samplePos),
         sdfStationYellowLine(samplePos),
         sdfStationDarkenedParts(samplePos),
         sdfStationDarkenedRoof(samplePos),
         sdfStationBorderLights(samplePos),
         sdfStationCeilingLights(samplePos),
         sdfStationTrainDisplay(samplePos));

    float lowestDistance = samples[0].distance;
    vec3 color = samples[0].color;
    for (int i = 1; i < OBJECTS_COUNT; i++) {
        if (samples[i].distance < lowestDistance) {
            lowestDistance = samples[i].distance;
            color = samples[i].color;
        }
    }
    float roomDistance = roomSamples[0].distance;
    vec3 roomColor = roomSamples[0].color;
    for (int i = 1; i < ROOM_COUNT; i++) {
        if (roomSamples[i].distance > roomDistance) {
            roomDistance = roomSamples[i].distance;
            roomColor = roomSamples[i].color;
        }
    }

    if (roomDistance < lowestDistance) {
        lowestDistance = roomDistance;
        color = roomColor;
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

float get_light_contribution(vec3 position, vec3 normal,
                             vec3 lightPosition, float lightDistance) {
    vec3 lightDir = lightPosition - position;
    return (1.0 - max(0.0, min(1.0, length(lightDir) / lightDistance))) *
        max(0.0, dot(normal, normalize(lightDir))) *
        (1.0 - get_shadow(position, lightPosition) * 0.75) * 0.35;
}

// Specular should probably be passed as a paramenter, and sourced from SDFSample
float get_brightness(vec3 samplePos, vec3 normal, float fog) {
    float ambient = 0.1;
    float diffuse = 0.0;
    float lightDistance = 11.0;
    // Lights along the tunnel
    for (int i = stage - 1; i <= stage + 1; i++) {
        vec3 lightPosition = transformToMetroSpace(vec3(1.8, 3.6, 1.0 + 9.0 * float(i)));
        if (lightPosition.z > STATION_START_Z && lightPosition.z <= STATION_START_Z + 90.0) {
            continue;
        }
        diffuse += get_light_contribution(samplePos, normal, lightPosition,
                                          lightDistance);
    }
    // Lights in the station
    for (int x = 0; x < 2; x++) {
        for (int z = 0; z < 10; z++) {
            float margin = 2.0;
            float lightGap = STATION_WIDTH - margin * 2.0;
            vec3 lightPosition = vec3(2.0 + margin + x * lightGap, 6.5,
                                      STATION_START_Z + z * 9.0 + 4.5);
            lightPosition = transformToMetroSpace(lightPosition);
            diffuse += get_light_contribution(samplePos, normal, lightPosition,
                                              lightDistance);
        }
    }
    return min(1.0, diffuse) + ambient;
}

// TODO: Improve the AO algorithm/remove it
float get_ambient_occlusion(vec3 samplePos, vec3 normal) {
    // Cheap hack to avoid wall artifacts:
    if (normal.y == 0.0) {
        return 0.0;
    }

    float step = 0.01;
    vec3 position = samplePos + normal * step;
    int steps = 0;
    for (; steps < 4; steps++) {
        SDFSample s = sdf(position, false);
        if (s.distance <= (float(steps) + 1.0) * step) {
            return 1.0 - float(steps) / 4.0;
        }
        position += min(s.distance, step);
    }
    return 0.0;
}

vec4 get_color(vec2 screenPosition, vec3 position, vec3 rotation) {
    // Not sure if this is correct but it seems right /shrug
    float r = (180.0 - cameraFieldOfView) / 720.0 * 3.14159;
    float nearDistance = sin(r) / cos(r);
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
    // NOTE: The y coordinate is flipped because we're rendering to a render texture
    vec2 pixelCoords = vec2((gl_FragCoord.x - resolution.x / 2.0) / resolution.y,
                            (gl_FragCoord.y - resolution.y / 2.0) / resolution.y * -1.0);

    gl_FragColor = get_color(pixelCoords, cameraPosition, cameraRotation);
}
