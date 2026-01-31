// main.cpp - Session Timer Win32 Application Entry Point

#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include "SetupDialog.h"
#include "TimerState.h"
#include "TimerWindow.h"

#pragma comment(lib, "comctl32.lib")

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPWSTR lpCmdLine, int nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // Set DPI awareness for crisp rendering on high-DPI displays
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // Initialize common controls (needed for progress bars and trackbars)
  INITCOMMONCONTROLSEX icc = {};
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_PROGRESS_CLASS | ICC_BAR_CLASSES;
  InitCommonControlsEx(&icc);

  // Register the timer window class
  if (!RegisterTimerWindowClass(hInstance)) {
    MessageBox(NULL, L"Failed to register window class.", L"Error",
               MB_OK | MB_ICONERROR);
    return 1;
  }

  // Main application loop - show setup, then timer, repeat
  bool continueApp = true;
  while (continueApp) {
    TimerConfig config = {};

    // Show setup dialog
    if (!ShowSetupDialog(hInstance, NULL, config)) {
      // User cancelled
      continueApp = false;
      break;
    }

    // Create and show timer window
    HWND hTimerWnd = CreateTimerWindow(hInstance, config);
    if (!hTimerWnd) {
      MessageBox(NULL, L"Failed to create timer window.", L"Error",
                 MB_OK | MB_ICONERROR);
      continue;
    }

    // Run message loop for timer window
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    // Timer window closed, loop back to show setup dialog again
  }

  return 0;
}
