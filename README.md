# A Walk In A Metro Tunnel
A Walk In A Metro Tunnel is a walking simulator. The environment is
the Ruoholahti-Lauttasaari part of the Helsinki metro, with the
interiors being completely improvised, but the route based on [some
plans found on the internet (page 41)][metro-plan].

After writing most of this game, I ran into a problem: the default
floating-point numbers used in GLSL (that is, 32-bit ones) aren't
accurate enough after running them through as many transformations as
the ray marching code does, which results in varying amounts of visual
noise. To preserve compatibility and avoid complexity, I decided to
incorporate the noise into this project. So while one theme of this
game is metros, and the Helsinki Metro specifically, the other part is
about floating-point inaccuracy, and how you can get some
understanding about how floating-point numbers work when their errors
are such a visible part of the world.

The game takes about 25 minutes to walk through completely.

### Controls
- WASD or IJKL to move
- Arrow keys or mouse to look around
- Shift to run
- Q to auto-walk (like pressing W/I continuously, but just one key
  press)

#### Configuration keys
- B to toggle head bobbing animation
- T to toggle the font (between VT323, which fits the game better
  aesthetically, and Open Sans, which is easier to read)

### Building
Just run the script relevant to your operating system. If it doesn't
work, refer to the documentation of
[raylib-template](https://git.neon.moe/neon/raylib-template).

### macOS
Out of the big three OSes, Linux, Windows, and macOS, this game
notably does *not* support macOS, mostly because Apple has deprecated
OpenGL, which most of this game relies upon. In addition to this, the
game runs very badly on macOS's OpenGL drivers, so you can't really
play the game even with an older version of macOS. There is a branch
that has most of the compatibility problems fixed, except for the
performance issue, `macos-compatibility`, if you want to try building
the game yourself.

### License
[This walking simulator](src/) is distributed under the [GNU
GPLv3](LICENSE.md) license. The dependencies (under
[vendor/](vendor/)) are distributed under their respective licenses.

[metro-plan]: https://www.hel.fi/hel2/ksv/Aineistot/maanalainen/Maanalaisen_yleiskaavan_selostus.pdf "A PDF containing the mentioned plans"
