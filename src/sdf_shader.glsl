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

// Ray marching variables (affect performance a lot)
#define RAY_STEPS_MAX 100
#define SDF_SURFACE_THRESHOLD 0.005

// 3D environment defining variables
#define LIGHTS_COUNT 2

varying vec4 position;

uniform vec2 resolution;
uniform vec3 cameraPosition;
uniform vec3 cameraRotation;

struct Camera {
    vec3 position;
    vec3 direction;
};

struct Light {
    vec3 position;
    float distance;
};

uniform Light lights[LIGHTS_COUNT] =
    Light[LIGHTS_COUNT](Light(vec3(3.0, 3.0, 4.0), 3.0),
                        Light(vec3(-4.0, 2.5, 6.0), 1.0));

/* SDFs: https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm */

float sdfSphere(vec3 samplePos, vec3 position, float radius) {
    return length(position - samplePos) - radius;
}

float sdfBox(vec3 samplePos, vec3 center, vec3 extents) {
  vec3 d = abs(center - samplePos) - extents;
  return length(max(d, 0.0)) + min(max(d.x, max(d.y, d.z)), 0.0);
}

float sdf(vec3 samplePos) {
    return min(
        sdfSphere(samplePos, vec3(0.0, 1.0, 5.0), 0.5),
        sdfBox(samplePos, vec3(0.0, -0.5, 5.0), vec3(5.0, 1.0, 5.0)));
}

vec4 rotate_x(vec4 direction, float degrees) {
    float r = radians(degrees);
    mat4 transform = mat4(vec4(1.0, 0.0, 0.0, 0.0),
                          vec4(0.0, cos(r), sin(r), 0.0),
                          vec4(0.0, -sin(r), cos(r), 0.0),
                          vec4(0.0, 0.0, 0.0, 1.0));
    return transform * direction;
}

vec4 rotate_y(vec4 direction, float degrees) {
    float r = radians(degrees);
    mat4 transform = mat4(vec4(cos(r), 0.0, -sin(r), 0.0),
                          vec4(0.0, 1.0, 0.0, 0.0),
                          vec4(sin(r), 0.0, cos(r), 0.0),
                          vec4(0.0, 0.0, 0.0, 1.0));
    return transform * direction;
}

float get_fog(vec3 cam, vec3 position) {
    return min(1.0, pow(5.0 / length(cam - position), 1.5));
}

vec3 get_normal(vec3 samplePos) {
    float x = sdf(samplePos + vec3(SDF_SURFACE_THRESHOLD, 0.0, 0.0)) -
        sdf(samplePos - vec3(SDF_SURFACE_THRESHOLD, 0.0, 0.0));
    float y = sdf(samplePos + vec3(0.0, SDF_SURFACE_THRESHOLD, 0.0)) -
        sdf(samplePos - vec3(0.0, SDF_SURFACE_THRESHOLD, 0.0));
    float z = sdf(samplePos + vec3(0.0, 0.0, SDF_SURFACE_THRESHOLD)) -
        sdf(samplePos - vec3(0.0, 0.0, SDF_SURFACE_THRESHOLD));
    return normalize(vec3(x, y, z));
}

float get_brightness(vec3 samplePos, vec3 normal, float fog) {
    float ambient = 0.1;
    float diffuse = 0.0;
    for (int i = 0; i < LIGHTS_COUNT; i++) {
        Light light = lights[i];
        vec3 lightDir = light.position - samplePos;
        diffuse += light.distance / length(lightDir) *
            max(0.0, dot(normal, normalize(lightDir))) * 0.2;
    }
    return min(1.0, diffuse) + ambient;
}

// TODO: Improve the AO algorithm, it's a bit dumb currently
float get_ambient_occlusion(vec3 samplePos, vec3 normal) {
    vec3 step = normal * 0.25;
    float distance = 0.1;
    for (; distance < 1.0; distance += 0.1) {
        if (sdf(samplePos + step * distance) < SDF_SURFACE_THRESHOLD) {
            break;
        }
    }
    return 1.1 - distance;
}

vec4 get_color(vec2 screenPosition, vec3 position, vec3 rotation) {
    // TODO: Configurable FoV
    float nearDistance = 0.5; // This is 90 degrees FoV
    vec3 direction = vec3(screenPosition.x, screenPosition.y, nearDistance);
    direction = normalize(direction);
    direction = rotate_x(vec4(direction, 1.0), rotation.x).xyz;
    direction = rotate_y(vec4(direction, 1.0), rotation.y).xyz;

    vec3 originalPosition = position;
    bool hit = false;
    vec3 normal = vec3(0.0, 0.0, 0.0);
    int steps = 1;
    for (; steps < RAY_STEPS_MAX; steps++) {
        float distance = sdf(position);
        if (distance < SDF_SURFACE_THRESHOLD) {
            hit = true;
            normal = get_normal(position);
            break;
        } else {
            position += direction * distance;
        }
    }

    if (hit) {
        float fog = get_fog(originalPosition, position);
        float brightness = get_brightness(position, normal, fog);
        float ambientOcclusion = pow(get_ambient_occlusion(position, normal), 4.0) * 0.2;
        return vec4(vec3(1.0, 1.0, 1.0) * brightness * fog - ambientOcclusion, 1.0);
    } else {
        return vec4(0.0, 0.0, 0.0, 1.0);
    }
}

void main() {
    // TODO: Ensure HiDPI compatibility

    // NOTE: The y coordinate is flipped because we're rendering to a render texture
    vec2 pixelCoords = vec2((gl_FragCoord.x - resolution.x / 2.0) / resolution.y,
                            (gl_FragCoord.y / resolution.y - 0.5) * -1.0);

    // TODO: Switch to render textures for lowering the resolution

    gl_FragColor = get_color(pixelCoords, cameraPosition, cameraRotation);
}
