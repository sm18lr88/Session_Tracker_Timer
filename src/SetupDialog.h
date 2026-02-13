// SetupDialog.h - Setup and settings dialog

#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <windows.h>

#include "TimerState.h"

enum class SetupDialogResult {
  Cancelled = 0,
  Accepted = 1,
  SquareOnly = 2
};

// Show the setup dialog
// Returns the selected dialog action
// config is populated with the user's choices
SetupDialogResult ShowSetupDialog(HINSTANCE hInstance, HWND hParent,
                                  TimerConfig& config);

// Show settings dialog (same as setup but pre-populated)
// Returns the selected dialog action
SetupDialogResult ShowSettingsDialog(HINSTANCE hInstance, HWND hParent,
                                     TimerConfig& config);

// Dialog procedure
INT_PTR CALLBACK SetupDialogProc(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam);

#endif  // SETUPDIALOG_H
