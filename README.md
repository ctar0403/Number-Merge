# Wood Number Merge (C++)

A native Windows puzzle game inspired by the core gameplay and warm wooden
presentation of Wood Number Merge. Number tiles launch upward into five lanes;
groups of at least two matching tiles combine into a larger number.

This is an original implementation. It does not include code, branding, or
assets from the commercial mobile game.

## Features

- Native mouse-driven graphical interface (no console window)
- Procedurally drawn wooden board and colorful number tiles
- Smooth launch, impact, merge-pulse, spark, particle, and combo effects
- Five lanes with eight tile positions each
- Orthogonal pair matching, upward compaction, and cascade combos
- Current and next tile previews
- Persistent local best score
- Responsive window scaling and keyboard shortcuts
- Dependency-free C++17 game logic with automated tests

## Build and play

This graphical version targets Windows and uses the built-in Win32/GDI API.
No external libraries or asset downloads are required.

With MinGW/G++:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude src/main.cpp src/game.cpp `
    -mwindows -lgdi32 -luser32 -o number_merge.exe
.\number_merge.exe
```

With CMake:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\number_merge.exe
```

The executable location can vary with the selected CMake generator.

## Controls

- Click one of the five board columns to launch the current tile.
- Press `1` through `5` as keyboard shortcuts for the columns.
- Click **New Game**, or press `R`/`N`, to restart.
- Press `Esc` to quit.

Two or more equal tiles connected horizontally or vertically merge into a
tile worth twice as much. A merged tile can immediately trigger another merge,
creating a higher-scoring combo. The game ends when all lanes are full.

The best score is stored beside the executable in `number_merge_best.txt`.

## Run the logic tests

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude tests/game_tests.cpp src/game.cpp `
    -o number_merge_tests.exe
.\number_merge_tests.exe
```

## Open-source research

The project was written from scratch after reviewing related projects:

- [plibither8/2048.cpp](https://github.com/plibither8/2048.cpp) — portable
  C++ game structure.
- [modern-cpp-examples/match3](https://github.com/modern-cpp-examples/match3)
  — C++ match-three architecture.
- [PlaytestersKitchen/match-three-js](https://github.com/PlaytestersKitchen/match-three-js)
  — documentation of match detection, compaction, and cascades.

No source code or assets were copied from these repositories.

## License

MIT. See [LICENSE](LICENSE).
