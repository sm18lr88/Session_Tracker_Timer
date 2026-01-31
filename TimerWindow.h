// TimerWindow.h - Main timer bar window

#ifndef TIMERWINDOW_H
#define TIMERWINDOW_H

#include <windows.h>

#include "TimerState.h"

// Window class name
#define TIMER_WINDOW_CLASS L"SessionTimerWindowClass"

// Custom messages
#define WM_UPDATE_TRANSPARENCY (WM_USER + 100)

// Register the timer window class
bool RegisterTimerWindowClass(HINSTANCE hInstance);

// Create and show the timer window
HWND CreateTimerWindow(HINSTANCE hInstance, const TimerConfig& config);

// Window procedure
LRESULT CALLBACK TimerWindowProc(HWND hWnd, UINT message, WPARAM wParam,
                                 LPARAM lParam);

// Get timer state for external access
TimerState* GetTimerState(HWND hWnd);

#endif  // TIMERWINDOW_H
