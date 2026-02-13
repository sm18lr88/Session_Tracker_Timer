#ifndef COVER_SQUARE_WINDOW_H
#define COVER_SQUARE_WINDOW_H

#include <windows.h>

// Messages sent to the timer/controller window from the cover square.
#define WM_COVER_SQUARE_OPEN_SETTINGS (WM_APP + 220)
#define WM_COVER_SQUARE_CLOSE_APP (WM_APP + 221)

// Register the black cover square window class.
bool RegisterCoverSquareWindowClass(HINSTANCE hInstance);

// Create the always-on-top black cover square.
// hController receives right-click menu actions from the cover square.
HWND CreateCoverSquareWindow(HINSTANCE hInstance, HWND hController);

#endif  // COVER_SQUARE_WINDOW_H
