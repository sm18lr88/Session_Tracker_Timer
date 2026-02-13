#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
SRC_DIR="$ROOT_DIR/macos/Sources/WolfTimerMac"
BUILD_DIR="$ROOT_DIR/build-macos"
APP_DIR="$BUILD_DIR/Wolf-Timer.app"
BIN_DIR="$APP_DIR/Contents/MacOS"
RES_DIR="$APP_DIR/Contents/Resources"
DIST_DIR="$ROOT_DIR/dist"
BIN_NAME="Wolf-Timer"

rm -rf "$BUILD_DIR" "$DIST_DIR"
mkdir -p "$BIN_DIR" "$RES_DIR" "$DIST_DIR"

swiftc \
  -O \
  -framework AppKit \
  "$SRC_DIR"/*.swift \
  -o "$BIN_DIR/$BIN_NAME"

cat > "$APP_DIR/Contents/Info.plist" <<'PLIST'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>
  <string>Wolf-Timer</string>
  <key>CFBundleDisplayName</key>
  <string>Wolf-Timer</string>
  <key>CFBundleExecutable</key>
  <string>Wolf-Timer</string>
  <key>CFBundleIdentifier</key>
  <string>com.wolftimer.app</string>
  <key>CFBundleShortVersionString</key>
  <string>1.0.0</string>
  <key>CFBundleVersion</key>
  <string>1</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>LSMinimumSystemVersion</key>
  <string>13.0</string>
  <key>NSHighResolutionCapable</key>
  <true/>
</dict>
</plist>
PLIST

# Ad-hoc sign for cleaner local launch behavior in CI artifacts.
codesign --force --deep --sign - "$APP_DIR" >/dev/null 2>&1 || true

ditto -c -k --sequesterRsrc --keepParent "$APP_DIR" "$DIST_DIR/Wolf-Timer-macOS.zip"
echo "Built app: $APP_DIR"
echo "Packaged: $DIST_DIR/Wolf-Timer-macOS.zip"
