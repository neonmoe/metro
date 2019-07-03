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
#define COMMENT_LINES 6

const char *narratorComments[COMMENTS_COUNT][COMMENT_LINES] = {
    {"This is a metro tunnel under Helsinki.",
     "This specific part is the Ruoholahti station.",
     "During the experience you'll walk (or run), in real time,",
     "the whole way to the next station, Lauttasaari.",
     "(Run/walk toggle: Left Shift)",
     "(Auto-walk toggle: Q)"},
    {"The Helsinki metro only has one fork and 25 stations,",
     "but the original plans were a bit more ambitious.",
     "There was a plan for 108 stations, built a lot closer to the surface.",
     "", "On the bright side, the current system is a lot simpler to use.", ""},
    {"And I prefer the cosy feeling the current one has.",
     "The metro is quite deep in the ground, feels like going into a bunker.",
     "Though the Kamppi escalator is definitely a bit spooky at first.",
     "", "Why?",
     "The station is very deep. So the escalator is very long."},
    {"You should be seeing some black dots on the walls by now.", "",
     "Those appear because the numbers I use to represent",
     "coordinates in the code are not precise enough.",
     "Seeing past the matrix is part of the experience.",
     "Just so you know."},
    {"Back to the metro. I searched the net a bit, and found something.",
     "Turns out that one new station has longer stairs.",
     "Never seen that specific escalator myself though.",
     "", "That one is under water as well. Neat, right?", ""},
    {"Want to hear a surprising fact?",
     "A big part of the Helsinki metro is not underground.",
     "Shocking, I know.", "", "", ""},
    {"I guess surface metros aren't all that uncommon though.",
     "I think Stockholm has parts on the surface. And Berlin.",
     "But especially in the case of Berlin, the name would be very misleading.",
     "", "", ""},
    {"We're getting into the really noisy part,",
     "so I'll take a moment to talk about the noise.",
     "When programming computer graphics, we use so-called",
     "'floating-point numbers' to represent coordinates in 3D space.",
     "These floating-point numbers are efficient,",
     "but struggle with with accuracy sometimes."},
    {"I find the noise in this metro tunnel quite interesting.", "",
     "", "How the tunnel gets noisier, and then less noisy.",
     "The patterns demonstrate how floating-point numbers have",
     "less accurate 'areas' of numbers. Or, in this case, actual areas."},
    {"Another interesting part about the noise, is that it",
     "looks a bit different on different computers.",
     "This is like taking a peek at your computer's way of",
     "handling floating-point numbers! :)", "", ""},
    {"If you want to know more about floating-point numbers,",
     "Computerphile on YouTube has a few videos on them.",
     "", "", "", ""},
    {"You might've seen the tracks go a bit weird back there.",
     "",
     "I'm not sure if it's caused by the noise, probably not,",
     "since it's so prominently visible.",
     "But it looks interesting, so I won't fix it.",
     "I hope you appreciate surreal tracks as well."},
    {"And that's that for our public infrastructure appreciation walk.",
     "We have arrived at Lauttasaari station.",
     "Thanks for walking the whole way, I wish you a good day.", "", "", ""}
};

#endif
