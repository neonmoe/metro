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
    {"This is a metro tunnel under Helsinki,",
     "or more specifically, the Ruoholahti station.",
     "During the experience you'll walk (or run), in real time,",
     "the whole way to the next station to the west, Lauttasaari.",
     "(Run/walk toggle: Left Shift)",
     "(Auto-walk toggle: Q)"},
    {"I think metro lines are one of the more interesting",
     "parts of city infrastructure.",
     "They are similar to trams and trains, but they don't",
     "disrupt other forms of transport above ground.",
     "Metros are like 20th century teleporter networks!",
     "The teleportation is just a bit slow."},
    {"I don't really know much about metros, so I won't recite",
     "Wikipedia to you. But I do appreciate them.",
     "Of course, in other countries, metros are probably really",
     "full all the time, and hence uncomfortable.",
     "But I like the idea of disappearing below the ground at",
     "one point in the city, and coming up from another point."},
    {"You should be seeing some black dots on the walls by now.", "",
     "We'll get to those later.",
     "",
     "But they are, in a way, intentional.",
     "Just so you know."},
    {"Back to my metro appreciation. This metro line you're",
     "walking on, is pretty new.",
     "Opened only a few years ago, this extension to the metro",
     "made it so it extends to the neighbouring city.",
     "I waited for it for all of my high school, but it kept",
     "getting delayed, until I moved to Helsinki myself, heh."},
    {"I'll give you a moment to meditate in the noise.",
     "", "", "", "", ""},
    {"Want to hear a surprising fact?",
     "A big part of the Helsinki metro is not underground.",
     "Shocking, I know.", "",
     "I guess surface metros aren't all that uncommon though.",
     "I think Stockholm has parts on the surface. And Berlin."},
    {"Now we're getting into the really noisy part,",
     "so I'll take a moment to talk about the noise.",
     "When programming computer graphics, we use so-called",
     "\"floating-point numbers\" to represent coordinates in 3D space.",
     "These floating-point numbers are efficient,",
     "but struggle with with accuracy sometimes."},
    {"I find the noise in this metro tunnel quite interesting.", "",
     "", "How the tunnel gets noisier, and then less noisy.",
     "The patterns demonstrate how floating-point numbers have",
     "less accurate \"areas\" of numbers. Or, in this case, actual areas."},
    {"Another interesting part about the noise, is that it",
     "can look quite different on different computers.",
     "This is like taking a peek at your computer's way of",
     "handling floating-point numbers! :)", "", ""},
    {"If you want to know more about floating-point numbers,",
     "Computerphile on YouTube has a few videos on them.",
     "", "", "", ""},
    {"You might've seen the tracks go a bit weird back there.",
     "",
     "I'm not sure if it's caused by the noise, probably not,",
     "since it seems so deterministic.",
     "But it looks interesting, so I won't \"fix\" it.",
     "I hope you appreciate surreal tracks as well."},
    {"...And that's that for our public infrastructure appreciation /",
     "computer science education walk.",
     "We have arrived at Lauttasaari station.",
     "Thanks for walking the whole way, I wish you a good day.",
     "(You can press F4 to exit, or just close the game window.)",
     "(Pressing the Esc key will release the mouse if it's locked.)"}
};

#endif
