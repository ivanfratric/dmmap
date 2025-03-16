# Automapper for Dungeon Master

![screenshot](https://raw.githubusercontent.com/ivanfratric/dmmap/refs/heads/main/screenshot.png)

This is an automapper / automap mod for Dungeon Master and Chaos Strikes Back. Currently this works only with Amiga version of the game. Dungeon Master 3.6 and Chaos Strikes Back 3.5 were tested (more specifically, WHDLoad versions of these games), but other versions might work too.

**Important Disclaimer: This code is provided as is and I don't have the capacity to maintain it. If something is broken, please fix it and submit a patch instead of filing issues.**

To build from source using Visual Studio compiler, run

```
cl dmmap.cpp User32.lib
```

To use, run:
```
dmmap.exe <game> <process>
```

Where `<game>` is either `dm` or `csb` and `process` is the name of emulator process, e.g. `retroarch.exe`

You can use the `x` button on keyboard to mark a location on a map with a different color. The primary usecase for this is to mark points of interest.

The automapper reads the following data from memory, which it uses to draw the map as the player plays:
 - Party coordinates
 - Current level
 - Party orientation

All map data is saved in the current directory.

