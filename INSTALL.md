Installation
============

Distribution Packaging
----------------------

Currently there are no official distribution packages for alien-console. For
Arch Linux, there is an [AUR package][AUR package] which you can build and use.

[AUR package]: https://aur.archlinux.org/packages/alien-console/

Manual Installation
-------------------

alien-console depends on three things (in addition to a standard C compiler and
make utility):

- libconfig to read the configuration file
- ncurses to display the interface
- alsa-utils (optionally) to play sounds. Note that sounds are not packaged with
  this repository, so unless you manually provide sounds, you don't need to
  worry about this dependency.

Additionally, if you want the interface to look like shown in the screenshot,
you'll want to look into installing the program "cool-retro-term", which may not
be packaged for your distribution.

Below are instructions per-distribution.

### Arch Linux

Install dependencies:

    sudo pacman -S ncurses libconfig
    # optional:
    sudo pacman -S alsa-utils

Build:

    make

System install:

    sudo make install

### Ubuntu

Install dependencies:

    sudo apt install build-essential libncurses-dev libconfig-dev
    # optional:
    sudo apt install alsa-utils

Build:

    make

System install:

    sudo make install
