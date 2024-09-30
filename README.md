# Wood Number Merge (C++)

A small, dependency-free terminal puzzle inspired by the core mechanic of
Wood Number Merge: drop numbered tiles, connect at least three equal values,
and build larger numbers through cascading merges.

This is an original implementation. It does not include code, branding, or
assets from the commercial mobile game.

## Gameplay

- The board has 5 columns and 8 rows.
- Enter `1` through `5` to drop the current tile into a column.
- Three or more equal tiles connected horizontally or vertically merge.
- The newest tile becomes twice as valuable; gravity then pulls tiles down.
- Cascading merges earn a combo multiplier.
- The game ends when all five columns reach the top.
- Use `r` to restart, `h` for help, or `q` to quit.

## Build

With a C++17 compiler only:

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude src/main.cpp src/game.cpp -o number_merge.exe
.\number_merge.exe
```

With CMake:

```powershell
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Run the tests without CMake

```powershell
g++ -std=c++17 -Wall -Wextra -pedantic -Iinclude tests/game_tests.cpp src/game.cpp -o number_merge_tests.exe
.\number_merge_tests.exe
```

## Open-source references

The project was written from scratch after reviewing several related projects:

- [plibither8/2048.cpp](https://github.com/plibither8/2048.cpp) — portable
  C++ terminal-game structure (MIT).
- [modern-cpp-examples/match3](https://github.com/modern-cpp-examples/match3)
  — C++ match-three architecture using SDL2.
- [PlaytestersKitchen/match-three-js](https://github.com/PlaytestersKitchen/match-three-js)
  — clear documentation of match detection, gravity, and cascades (MIT).

No source code or assets were copied from these repositories.

## License

MIT. See [LICENSE](LICENSE).

