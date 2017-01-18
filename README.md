# DLRTS
Something that's starting to resemble an RTS game engine. Might even become a game some day.

Most work so far has been on core engine. This is as far from polished as can be.

It relies on Horde3d for rendering. Everything else implemented in C++ from the ground up.

Currently this project is on the shelf, to be resumed some day maybe. If not it can serve as an example to scare people away from trying to build an entire game engine from scratch by themselves.

Copyright (C) 2016 Erik Boström.

This application has the following dependencies:

  - Eigen3
    http://eigen.tuxfamily.org/index.php?title=Main_Page
  - SDL2
    http://www.libsdl.org/
  - Horde3d
    http://www.horde3d.org/
  - Squirrel3
    http://www.squirrel-lang.org/

These libraries are included as code in the project repository. The reason for this is that the libraries used have some slight modifications. In particular SDL2 has a bug in it's Wayland support which has been reported, but not yet pushed to the release branch. Horde3d and squirrel also has some added features.

## Building
Just run make. Currently works only on Linux. Requires gcc 4.8, make and cmake 2.6. Tested on Manjaro and Ubuntu.

Please excuse the build-system, it's a quickly thrown together mess of makefiles. Has been tested on Manjaro and Ubuntu.

## Instructions
Build and run the application (it's created in bin/release).

The game world should consist of something that resembles water (excuse the procedurally generated graphics). Camera can be controlled with wsad.

Use the GUI to create a game world. The GUI is hastily thrown together and has some bugs if you try to resize the windows. It will be replaced completely in the future. Press the run button to run the application. Press ESC to return to the world editor.

The red cubes act as units. They can be selected with left mouse button and ordered to go places with right. Light sources can be placed (but not removed yet) to illuminate parts of the world, which extends the units sight range to that location. They are currently blaringly bright, but it's just for testing the fog of war.

Known bugs:
  - Resizing the windows of the GUI sometimes makes them blow up. The GUI is slated for removal completely.
  - Generation of navigation meshes sometimes crashes or freezes. It works 99% of the time but there are still some corner cases where it fails.
  - Ordering units to move outside of the navigation mesh crashes occasionally. Works 99% of the time though.
  - The camera tries to correct it's height depending on the height of the map underneath it. When the map is modified underneath the camera it can cause the camera to jump suddenly. This is just hasty implementation of something not very important, but can be annoying.
