# A Walk In A Metro Tunnel
A Walk In A Metro Tunnel is a walking simulator. The environment is the
Ruoholahti-Lauttasaari part of the Helsinki metro, with the interiors
being completely improvised, but the route based on [some plans found
on the internet (page 41)][metro-plan].

The game takes about 25 minutes to walk through completely.

### Controls
- WASD or IJKL to move
- Arrow keys or mouse to look around

#### Configuration keys
- B to toggle head bobbing animation
- T to toggle the font (between VT323, which fits the game better
  aesthetically, and Open Sans, which is easier to read)

### Building
Just run the script relevant to your operating system. If it doesn't
work, refer to the documentation of
[raylib-template](https://git.neon.moe/neon/raylib-template).

### License
[This walking simulator](src/) is distributed under the [GNU
GPLv3](LICENSE.md) license. The dependencies (under
[vendor/](vendor/)) are distributed under their respective licenses.

[metro-plan]: https://www.hel.fi/hel2/ksv/Aineistot/maanalainen/Maanalaisen_yleiskaavan_selostus.pdf "A PDF containing the mentioned plans"
