// SetupDialog.cpp - Setup and settings dialog implementation

#include "SetupDialog.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <cstdio>
#include <cstring>

#include "resource.h"

#pragma comment(lib, "dwmapi.lib")

static const INT_PTR IDSETUP_SQUARE_ONLY = 1001;

// Pointer to config passed via LPARAM
static TimerConfig* g_pConfig = nullptr;
static bool g_isSettings =
    false;  // true if this is settings dialog (pre-populated)
static HBRUSH g_hDialogBackBrush = nullptr;
static HBRUSH g_hEditBackBrush = nullptr;
static HBRUSH g_hButtonBackBrush = nullptr;

static HBRUSH EnsureBrush(HBRUSH& brush, COLORREF color) {
  if (!brush) {
    brush = CreateSolidBrush(color);
  }
  return brush;
}

static void ApplyDarkDialogTheme(HWND hDlg) {
  const BOOL enabled = TRUE;
  // DWMWA_USE_IMMERSIVE_DARK_MODE (20) is available on modern Windows.
  DwmSetWindowAttribute(hDlg, 20, &enabled, sizeof(enabled));

  const int controlIds[] = {
      IDC_EDIT_TIME_PER_BLOCK, IDC_EDIT_NUM_BLOCKS, IDC_EDIT_NUM_QUESTIONS,
      IDC_SLIDER_TRANSPARENCY, IDC_STATIC_TRANSPARENCY, IDOK, IDCANCEL,
      IDC_BTN_SQUARE_ONLY};

  for (int controlId : controlIds) {
    HWND hCtrl = GetDlgItem(hDlg, controlId);
    if (hCtrl) {
      SetWindowTheme(hCtrl, L"DarkMode_Explorer", nullptr);
    }
  }
}

static bool ReadConfigFromDialog(HWND hDlg, TimerConfig* outConfig) {
  BOOL success = TRUE;

  int timePerBlock =
      GetDlgItemInt(hDlg, IDC_EDIT_TIME_PER_BLOCK, &success, FALSE);
  if (!success || timePerBlock <= 0) {
    MessageBox(hDlg, L"Please enter a valid time per block (> 0).",
               L"Invalid Input", MB_OK | MB_ICONWARNING);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_TIME_PER_BLOCK));
    return false;
  }

  int numBlocks = GetDlgItemInt(hDlg, IDC_EDIT_NUM_BLOCKS, &success, FALSE);
  if (!success || numBlocks <= 0) {
    MessageBox(hDlg, L"Please enter a valid number of blocks (> 0).",
               L"Invalid Input", MB_OK | MB_ICONWARNING);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_NUM_BLOCKS));
    return false;
  }

  int numQuestions =
      GetDlgItemInt(hDlg, IDC_EDIT_NUM_QUESTIONS, &success, FALSE);
  if (!success || numQuestions <= 0) {
    MessageBox(hDlg, L"Please enter a valid number of questions (> 0).",
               L"Invalid Input", MB_OK | MB_ICONWARNING);
    SetFocus(GetDlgItem(hDlg, IDC_EDIT_NUM_QUESTIONS));
    return false;
  }

  HWND hSlider = GetDlgItem(hDlg, IDC_SLIDER_TRANSPARENCY);
  int transparency = (int)SendMessage(hSlider, TBM_GETPOS, 0, 0);

  if (outConfig) {
    outConfig->timePerBlock = timePerBlock;
    outConfig->numBlocks = numBlocks;
    outConfig->numQuestions = numQuestions;
    outConfig->transparency = transparency;
    outConfig->ComputeDerivedValues();
  }

  return true;
}

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
      ApplyDarkDialogTheme(hDlg);

      HWND hSquareOnly = GetDlgItem(hDlg, IDC_BTN_SQUARE_ONLY);
      if (hSquareOnly) {
        ShowWindow(hSquareOnly, SW_SHOW);
      }

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
        SetDlgItemText(hDlg, IDOK, L"Apply");
      } else {
        // Defaults
        SetDlgItemInt(hDlg, IDC_EDIT_TIME_PER_BLOCK, 60, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_NUM_BLOCKS, 2, FALSE);
        SetDlgItemInt(hDlg, IDC_EDIT_NUM_QUESTIONS, 40, FALSE);
        SendMessage(hSlider, TBM_SETPOS, TRUE, 75);
        SetDlgItemText(hDlg, IDC_STATIC_TRANSPARENCY, L"75%");
      }

      return TRUE;
    }

    case WM_CTLCOLORDLG: {
      HDC hdc = (HDC)wParam;
      SetBkColor(hdc, RGB(28, 31, 36));
      return (INT_PTR)EnsureBrush(g_hDialogBackBrush, RGB(28, 31, 36));
    }

    case WM_CTLCOLORSTATIC: {
      HDC hdc = (HDC)wParam;
      SetTextColor(hdc, RGB(230, 233, 239));
      SetBkMode(hdc, TRANSPARENT);
      return (INT_PTR)EnsureBrush(g_hDialogBackBrush, RGB(28, 31, 36));
    }

    case WM_CTLCOLOREDIT: {
      HDC hdc = (HDC)wParam;
      SetTextColor(hdc, RGB(244, 246, 250));
      SetBkColor(hdc, RGB(45, 50, 58));
      return (INT_PTR)EnsureBrush(g_hEditBackBrush, RGB(45, 50, 58));
    }

    case WM_CTLCOLORBTN: {
      HDC hdc = (HDC)wParam;
      SetTextColor(hdc, RGB(240, 243, 248));
      SetBkColor(hdc, RGB(62, 68, 78));
      return (INT_PTR)EnsureBrush(g_hButtonBackBrush, RGB(62, 68, 78));
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
          if (ReadConfigFromDialog(hDlg, g_pConfig)) {
            EndDialog(hDlg, IDOK);
          }
          return TRUE;
        }

        case IDC_BTN_SQUARE_ONLY:
          if (ReadConfigFromDialog(hDlg, g_pConfig)) {
            EndDialog(hDlg, IDSETUP_SQUARE_ONLY);
          }
          return TRUE;

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
  lpdt->cdit = 12;  // Number of controls
  lpdt->x = 0;
  lpdt->y = 0;
  lpdt->cx = 220;
  lpdt->cy = 140;

  LPWORD lpw = (LPWORD)(lpdt + 1);
  *lpw++ = 0;  // No menu
  *lpw++ = 0;  // Default dialog class

  // Title
  LPWSTR lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 32, L"Wolf-Timer Setup");
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);

  // Font
  *lpw++ = 9;  // Font size
  lpwsz = (LPWSTR)lpw;
  wcscpy_s(lpwsz, 32, L"Segoe UI");
  lpw = (LPWORD)(lpwsz + wcslen(lpwsz) + 1);

  // Add controls using numeric class IDs (0x0080=Button, 0x0081=Edit,
  // 0x0082=Static)

  // Static: Questions per block label (top row)
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 12, 100,
                          10, IDC_STATIC_QUESTIONS_LABEL, 0x0082,
                          L"Questions per block:");

  // Edit: Questions per block (top row)
  lpw = AddControlNumeric(
      lpw, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 0, 120,
      10, 40, 14, IDC_EDIT_NUM_QUESTIONS, 0x0081, L"");

  // Static: Time per block label (second row)
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 32, 100,
                          10, IDC_STATIC_TIME_LABEL, 0x0082,
                          L"Time per block (min):");

  // Edit: Time per block (second row)
  lpw = AddControlNumeric(
      lpw, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 0, 120,
      30, 40, 14, IDC_EDIT_TIME_PER_BLOCK, 0x0081, L"");

  // Static: Number of blocks label (third row)
  lpw = AddControlNumeric(lpw, WS_CHILD | WS_VISIBLE | SS_LEFT, 0, 10, 52, 100,
                          10, IDC_STATIC_BLOCKS_LABEL, 0x0082,
                          L"Number of blocks:");

  // Edit: Number of blocks (third row)
  lpw = AddControlNumeric(
      lpw, WS_CHILD | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER, 0, 120,
      50, 40, 14, IDC_EDIT_NUM_BLOCKS, 0x0081, L"");

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
                          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON |
                              BS_FLAT,
                          0, 10, 100, 64, 14, IDOK, 0x0080, L"Start");

  // Button: Cover only (visible in settings mode)
  lpw = AddControlNumeric(
      lpw,
      WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT,
      0, 78, 100, 64, 14, IDC_BTN_SQUARE_ONLY, 0x0080, L"Cover only");

  // Button: Cancel
  lpw =
      AddControlNumeric(lpw,
                        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON |
                            BS_FLAT,
                        0, 146, 100, 64, 14, IDCANCEL, 0x0080, L"Cancel");

  GlobalUnlock(hgbl);
  return hgbl;
}

static SetupDialogResult MapDialogResult(INT_PTR result) {
  if (result == IDOK) return SetupDialogResult::Accepted;
  if (result == IDSETUP_SQUARE_ONLY) return SetupDialogResult::SquareOnly;
  return SetupDialogResult::Cancelled;
}

SetupDialogResult ShowSetupDialog(HINSTANCE hInstance, HWND hParent,
                                  TimerConfig& config) {
  g_isSettings = false;

  HGLOBAL hgbl = CreateSetupDialogTemplate();
  if (!hgbl) return SetupDialogResult::Cancelled;

  INT_PTR result =
      DialogBoxIndirectParam(hInstance, (LPDLGTEMPLATE)GlobalLock(hgbl),
                             hParent, SetupDialogProc, (LPARAM)&config);

  GlobalUnlock(hgbl);
  GlobalFree(hgbl);

  return MapDialogResult(result);
}

SetupDialogResult ShowSettingsDialog(HINSTANCE hInstance, HWND hParent,
                                     TimerConfig& config) {
  g_isSettings = true;

  HGLOBAL hgbl = CreateSetupDialogTemplate();
  if (!hgbl) return SetupDialogResult::Cancelled;

  INT_PTR result =
      DialogBoxIndirectParam(hInstance, (LPDLGTEMPLATE)GlobalLock(hgbl),
                             hParent, SetupDialogProc, (LPARAM)&config);

  GlobalUnlock(hgbl);
  GlobalFree(hgbl);

  return MapDialogResult(result);
}
