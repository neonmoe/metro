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

#ifndef RESOURCES_H
#define RESOURCES_H

enum {
    RESOURCE_OPEN_SANS, RESOURCE_VT323, RESOURCE_SHADER,
    RESOURCE_FOOTSTEP_1, RESOURCE_FOOTSTEP_2, RESOURCE_FOOTSTEP_3,
    RESOURCE_FOOTSTEP_4, RESOURCE_FOOTSTEP_5, RESOURCE_COUNT
};

const char *resourcePaths[RESOURCE_COUNT] = {
    "metro_assets/fonts/open_sans.ttf", "metro_assets/fonts/vt323.ttf", "metro_assets/shaders/sdf.glsl",
    "metro_assets/sfx/footstep01.ogg", "metro_assets/sfx/footstep02.ogg", "metro_assets/sfx/footstep03.ogg",
    "metro_assets/sfx/footstep04.ogg", "metro_assets/sfx/footstep05.ogg"
};

#endif
