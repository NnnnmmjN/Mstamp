# Mstamp - Music with Timestamps

Inspired by Tsoding's Musializer (http://github.com/tsoding/musializer), which I have implemented with SDL2 and OpenGL, I also stumbled across a similar idea. Instead of loading multiple files, I thought about loading one big, but divided into smaller songs with timestamps - for example movie or game OST.

## Layout

- `music` folder with music files
- `timestamps` folder with `.time` files (examples provided)
- files in both folders need specifying inside `main.c` as a map - which music file assign to what timestamp file

## Running (console)

Works on Linux and MacOS. I tried building for Windows (with mingw), but somethings's not willing to collaborate (either from `miniaudio.h` side or Windows not-emulation through `wine`).

Done once (bootstrap):
```
gcc nob.c -o nob
```
Recompiling:
```
./nob
```
Running:
```
./main music/<music_file.mp3> [index]
```
`index` is a 0-based, non-negative and optional index of tracks "list" from `.time` file. Default is 0 (which is first track).

## Dependencies

- nob.h - https://github.com/tsoding/nob.h
- Arena allocator - https://github.com/tsoding/arena
- Miniaudio - https://github.com/mackron/miniaudio
