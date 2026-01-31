// SetupDialog.h - Setup and settings dialog

#ifndef SETUPDIALOG_H
#define SETUPDIALOG_H

#include <windows.h>

#include "TimerState.h"

// Show the setup dialog
// Returns true if user clicked OK with valid settings
// config is populated with the user's choices
bool ShowSetupDialog(HINSTANCE hInstance, HWND hParent, TimerConfig& config);

// Show settings dialog (same as setup but pre-populated)
// Returns true if user clicked OK
bool ShowSettingsDialog(HINSTANCE hInstance, HWND hParent, TimerConfig& config);

// Dialog procedure
INT_PTR CALLBACK SetupDialogProc(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam);

#endif  // SETUPDIALOG_H
