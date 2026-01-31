# Session Timer

Helps users know how far along they should be per question block, and per question itself.

Features:
- Always-on-top
- Slim and narrow so it takes up the least amount of screen space
- Set opacity level
- Two progress bars: per question, and per block
- DPI aware for high-resolution displays

## Building

Requires Visual Studio 2022+ with C++ build tools.

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
