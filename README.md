# Session Timer

Helps users know how far along they should be per question block, and per question itself.

<img width="600" height="467" alt="image" src="https://github.com/user-attachments/assets/2d1eb5d8-1fdb-4b11-9e58-58a4cb2b22f8" />
<img width="472" height="481" alt="image" src="https://github.com/user-attachments/assets/7932ad67-926a-4b4c-b86b-df816c7fd68c" />



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
