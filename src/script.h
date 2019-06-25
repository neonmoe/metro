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

/* This file contains the script that is displayed to the player
 * during the game. */

#ifndef NARRATOR_SCRIPT_H
#define NARRATOR_SCRIPT_H

#define COMMENTS_COUNT 13
#define COMMENT_LINES 3

const char *narratorComments[COMMENTS_COUNT][COMMENT_LINES] = {
    {"This is a metro tunnel under Helsinki.", "", ""},
    {"The Helsinki metro only has one fork and 25 stations,",
     "but the original plans were a bit more ambitious.",
     "There was a plan for 108 stations, built a lot closer to the surface."},
    {"That said, I prefer the cosy feeling the current one has.",
     "No maps needed; just go east or west.",
     "And it's a good bit below ground, feels like going into a bunker."},
    {"The Kamppi escalator is definitely a bit spooky at first.", "...",
     "To clarify, the station is very deep. So the escalator is very long."},
    {"", "(reading wikipedia)", ""},
    {"After some reading, turns out that one new station has longer stairs.",
     "Never seen that specific escalator myself though.",
     ""},
    {"That one is under water as well. Neat, right?",
     "Must've been quite the engineering challenge.",
     ""},
    {"", "", ""},
    {"Want to hear a surprising fact?",
     "A big part of the metro line is not underground.",
     "Shocking, I know."},
    {"Based on the Stockholm metro", "", ""},
    {"", "", ""},
    {"Just to clarify, what you're looking at isn't really the Helsinki metro.",
     "I couldn't actually find any blueprints, and only a few pictures.",
     "So this is just my artistic interpretation."},
    {"And that's that for our public infrastructure appreciation walk.",
     "The next station is right up ahead, but that's off-limits for you.",
     "Thanks for walking the whole way, I wish you a good day."}
};

#endif
