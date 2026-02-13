# Wolf-Timer

App to keep pace when doing questions, and/or cover your answer choices.

<img width="472" alt="image" src="https://github.com/user-attachments/assets/84722979-d19a-4be1-9a47-7a2eb5f0b01c" />
<img width="600" alt="image" src="https://github.com/user-attachments/assets/e152d7b8-e3c7-4bd1-b9a1-0e10c364af4d" />


Current version: `v0.5`

## Project Structure

```text
.
├─ src/                  # Windows (Win32 C++) source/resources
├─ macos/                # Native macOS (AppKit, Swift) source/build script
├─ .github/workflows/    # CI workflows (includes macOS app build)
├─ LICENSE
├─ README.md
└─ CMakeLists.txt
```

Features:
- Always-on-top
- Opaque black draggable/resizable cover overlay (always-on-top)
- `Shift+Space` global shortcut to show/hide the cover square
- Cover square size/position persists across sessions
- Settings supports `Cover only` mode (cover-only workflow)
- Right-click cover menu: `Settings...` and `Close`
- Dark-themed setup/settings dialog
- Slim and narrow so it takes up the least amount of screen space
- Set opacity level
- Two progress bars: per question, and per block
- DPI aware for high-resolution displays

## Building

Requires Visual Studio with Desktop C++ tools (VS 2022 Build Tools or VS 2026).

```bash
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

If you only have VS 2022 Build Tools installed, use generator `"Visual Studio 17 2022"` instead.

## Building (macOS)

The macOS app is native AppKit Swift code and is built in CI on GitHub Actions.

1. Run workflow: `.github/workflows/build-macos.yml`
2. Download artifact: `Wolf-Timer-macOS`

You can also build locally on a Mac:

```bash
chmod +x macos/build_app.sh
macos/build_app.sh
```

The artifact includes:
- `Wolf-Timer.app`
- `Wolf-Timer-macOS.zip`

Note for macOS hotkey:
- Global `Shift+Space` handling may require enabling input monitoring/accessibility permissions for the app, depending on macOS privacy settings.
