// SetupDialog.cpp - Setup and settings dialog implementation

#include "SetupDialog.h"

#include <commctrl.h>
#include <windowsx.h>

#include <cstdio>
#include <cstring>

#include "resource.h"

// Pointer to config passed via LPARAM
static TimerConfig* g_pConfig = nullptr;
static bool g_isSettings =
    false;  // true if this is settings dialog (pre-populated)

INT_PTR CALLBACK SetupDialogProc(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam) {
  switch (message) {
    case WM_INITDIALOG: {
      g_pConfig = reinterpret_cast<TimerConfig*>(lParam);

      // Center dialog on screen
      RECT rc;
      GetWindowRect(hDlg, &rc);
      int width = rc.right - rc.left;
      int height = rc.bottom - rc.top;
      int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
      int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
      SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

      // Initialize transparency slider (0-100)
      HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER_TRANSPARENCY);
      SendMessage(hSlider, TBM_SETRANGE, TRUE,
                  MAKELPARAM(20, 100));  // Min 20% to keep visible
      SendMessage(hSlider, TBM_SETTICFREQ, 10, 0);

      if (g_isSettings && g_pConfig) {
        // Pre-populate with current values
        SetDlgItemInt(hDlg, IDC_EDIT_TIME_PER_BLOCK, g_pConfig->timePerBlock,
                      FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_NUM_BLOCKS, g_pConfig->numBlocks, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_NUM_QUESTIONS, g_pConfig->numQuestions,
                      FALSE);
        SendMessage(hSlider, TBM_SETPOS, TRUE, g_pConfig->transparency);

        wchar_t buf[16];
        swprintf_s(buf, L"%d%%", g_pConfig->transparency);
        SetDlgItemText(hDlg, IDC_STATIC_TRANSPARENCY, buf);

        SetWindowText(hDlg, L"Settings");
      } else {
        // Defaults
        SetDlgItemInt(hDlg, IDC_EDIT_TIME_PER_BLOCK, 30, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_NUM_BLOCKS, 3, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_NUM_QUESTIONS, 5, FALSE);
        SendMessage(hSlider, TBM_SETPOS, TRUE, 75);
        SetDlgItemText(hDlg, IDC_STATIC_TRANSPARENCY, L"75%");
      }

      return TRUE;
    }

    case WM_HSCROLL: {
      // Handle slider movement
      HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER_TRANSPARENCY);
      if ((HWND)lParam == hSlider) {
        int pos = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);
        wchar_t buf[16];
        swprintf_s(buf, L"%d%%", pos);
        SetDlgItemText(hDlg, IDC_STATIC_TRANSPARENCY, buf);

        // Live update transparency if this is settings dialog
        if (g_isSettings) {
          HWND hParent = GetParent(hDlg);
          if (hParent) {
            // Send custom message to update transparency
            PostMessage(hParent, WM_USER + 100, pos, 0);
          }
        }
      }
      return TRUE;
    }

    case WM_COMMAND: {
      switch (LOWORD(wParam)) {
        case IDOK: {
          BOOL success = TRUE;
          int timePerBlock =
              GetDlgItemInt(hDlg, IDC_EDIT_TIME_PER_BLOCK, &success, FALSE);
          if (!success || timePerBlock <= 0) {
            MessageBox(hDlg, L"Please enter a valid time per block (> 0).",
                       L"Invalid Input", MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(hDlg, IDC_EDIT_TIME_PER_BLOCK));
            return TRUE;
          }

          int numBlocks =
              GetDlgItemInt(hDlg, IDC_EDIT_NUM_BLOCKS, &success, FALSE);
          if (!success || numBlocks <= 0) {
            MessageBox(hDlg, L"Please enter a valid number of blocks (> 0).",
                       L"Invalid Input", MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(hDlg, IDC_EDIT_NUM_BLOCKS));
            return TRUE;
          }

          int numQuestions =
              GetDlgItemInt(hDlg, IDC_EDIT_NUM_QUESTIONS, &success, FALSE);
          if (!success || numQuestions <= 0) {
            MessageBox(hDlg, L"Please enter a valid number of questions (> 0).",
                       L"Invalid Input", MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(hDlg, IDC_EDIT_NUM_QUESTIONS));
            return TRUE;
          }

          HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER_TRANSPARENCY);
          int transparency = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);

          if (g_pConfig) {
            g_pConfig->timePerBlock = timePerBlock;
            g_pConfig->numBlocks = numBlocks;
            g_pConfig->numQuestions = numQuestions;
            g_pConfig->transparency = transparency;
            g_pConfig->ComputeDerivedValues();
          }

          EndDialog(hDlg, IDOK);
          return TRUE;
        }

        case IDCANCEL:
          EndDialog(hDlg, IDCANCEL);
          return TRUE;
      }
      break;
    }

    case WM_CLOSE:
      EndDialog(hDlg, IDCANCEL);
      return TRUE;
  }

  return FALSE;
}

// Align pointer to DWORD boundary
static LPWORD lpwAlign(LPWORD lpIn) {
  ULONG_PTR ul = (ULONG_PTR)lpIn;
  ul += 3;
  ul >>= 2;
  ul <<= 2;
  return (LPWORD)ul;
}

// Helper to add a control with numeric class ID
static LPWORD AddControlNumeric(LPWORD lpw, DWORD style, DWORD exStyle, short x,
                                short y, short cx, short cy, WORD id,
                                WORD classId, const wchar_t* text) {
  lpw = lpwAlign(lpw);
  LPDLGITEMTEMPLATE lpdit = (LPDLGITEMTEMPLATE)lpw;
  lpdit->style = style;
  lpdit->dwExtendedStyle = exStyle;
  lpdit->x = x;
  lpdit->y = y;
  lpdit->cx = cx;
  lpdit->cy = cy;
  lpdit->id = id;

  lpw = (LPWORD)(lpdit + 1);
  *lpw++ = 0xFFFF;
  *lpw++ = classId;

  // Copy text
  LPWSTR lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 64, text);
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);
  *lpw++ = 0;  // No creation data

  return lpw;
}

// Helper to add a control with string class name
static LPWORD AddControlString(LPWORD lpw, DWORD style, DWORD exStyle, short x,
                               short y, short cx, short cy, WORD id,
                               const wchar_t* className, const wchar_t* text) {
  lpw = lpwAlign(lpw);
  LPDLGITEMTEMPLATE lpdit = (LPDLGITEMTEMPLATE)lpw;
  lpdit->style = style;
  lpdit->dwExtendedStyle = exStyle;
  lpdit->x = x;
  lpdit->y = y;
  lpdit->cx = cx;
  lpdit->cy = cy;
  lpdit->id = id;

  lpw = (LPWORD)(lpdit + 1);

  // Copy class name string
  LPWSTR lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 64, className);
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);

  // Copy text
  lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 64, text);
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);
  *lpw++ = 0;  // No creation data

  return lpw;
}

static HGLOBAL CreateSetupDialogTemplate() {
  // Allocate memory for dialog template
  HGLOBAL hgbl = GlobalAlloc(GMEM_ZEROINIT, 4096);
  if (!hgbl) return NULL;

  LPDLGTEMPLATE lpdt = (LPDLGTEMPLATE)GlobalLock(hgbl);
  if (!lpdt) {
    GlobalFree(hgbl);
    return NULL;
  }

  // Dialog style
  lpdt->style = WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_CENTER |
                DS_SETFONT;
  lpdt->dwExtendedStyle = 0;
  lpdt->cdit = 11;  // Number of controls
  lpdt->x = 0;
  lpdt->y = 0;
  lpdt->cx = 220;
  lpdt->cy = 140;

  LPWORD lpw = (LPWORD)(lpdt + 1);
  *lpw++ = 0;  // No menu
  *lpw++ = 0;  // Default dialog class

  // Title
  LPWSTR lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 32, L"Session Timer Setup");
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);

  // Font
  *lpw++ = 9;  // Font size
  lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 32, L"Segoe UI");
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);

  // Add controls using numeric class IDs (0x0080=Button, 0x0081=Edit,
  // 0x0082=Static)

  // Static: Time per block label
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 12, 100,
                          10, IDC_STATIC_TIME_LABEL, 0x0082,
                          L"Time per block (min):");

  // Edit: Time per block
  lpw = AddControlNumeric(
      lpw, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 0, 120,
      10, 40, 14, IDC_EDIT_TIME_PER_BLOCK, 0x0081, L"");

  // Static: Number of blocks label
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 32, 100,
                          10, IDC_STATIC_BLOCKS_LABEL, 0x0082,
                          L"Number of blocks:");

  // Edit: Number of blocks
  lpw = AddControlNumeric(
      lpw, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 0, 120,
      30, 40, 14, IDC_EDIT_NUM_BLOCKS, 0x0081, L"");

  // Static: Questions per block label
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 52, 100,
                          10, IDC_STATIC_QUESTIONS_LABEL, 0x0082,
                          L"Questions per block:");

  // Edit: Questions per block
  lpw = AddControlNumeric(
      lpw, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 0, 120,
      50, 40, 14, IDC_EDIT_NUM_QUESTIONS, 0x0081, L"");

  // Static: Transparency label
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 72, 60,
                          10, (WORD)-1, 0x0082, L"Transparency:");

  // Trackbar: Transparency slider (uses string class name)
  lpw = AddControlString(
      lpw, WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_AUTOTICKS, 0, 70,
      70, 100, 18, IDC_SLIDER_TRANSPARENCY, TRACKBAR_CLASSW, L"");

  // Static: Transparency value
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 175, 72, 35,
                          10, IDC_STATIC_TRANSPARENCY, 0x0082, L"75%");

  // Button: OK
  lpw = AddControlNumeric(lpw,
                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
                          0, 50, 100, 50, 14, IDOK, 0x0080, L"Start");

  // Button: Cancel
  lpw =
      AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                        0, 120, 100, 50, 14, IDCANCEL, 0x0080, L"Cancel");

  GlobalUnlock(hgbl);
  return hgbl;
}

bool ShowSetupDialog(HINSTANCE hInstance, HWND hParent, TimerConfig& config) {
  g_isSettings = false;

  HGLOBAL hgbl = CreateSetupDialogTemplate();
  if (!hgbl) return false;

  INT_PTR result =
      DialogBoxIndirectParam(hInstance, (LPDLGTEMPLATE)GlobalLock(hgbl),
                             hParent, SetupDialogProc, (LPARAM)&config);

  GlobalUnlock(hgbl);
  GlobalFree(hgbl);

  return (result == IDOK);
}

bool ShowSettingsDialog(HINSTANCE hInstance, HWND hParent,
                        TimerConfig& config) {
  g_isSettings = true;

  HGLOBAL hgbl = CreateSetupDialogTemplate();
  if (!hgbl) return false;

  INT_PTR result =
      DialogBoxIndirectParam(hInstance, (LPDLGTEMPLATE)GlobalLock(hgbl),
                             hParent, SetupDialogProc, (LPARAM)&config);

  GlobalUnlock(hgbl);
  GlobalFree(hgbl);

  return (result == IDOK);
}
