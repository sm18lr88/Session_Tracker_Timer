// TimerWindow.cpp - Main timer bar window implementation

#include "TimerWindow.h"

#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <cstdio>

#include "CoverSquareWindow.h"
#include "SetupDialog.h"
#include "resource.h"

#pragma comment(lib, "uxtheme.lib")

// Base dimensions at 96 DPI (100% scaling)
static const int BASE_WINDOW_WIDTH = 600;
static const int BASE_WINDOW_HEIGHT = 56;
static const int BASE_ROW_HEIGHT = 26;
static const int BASE_MARGIN = 4;
static const int BASE_BTN_WIDTH = 60;
static const int BASE_BTN_SMALL = 28;
static const int BASE_LABEL_WIDTH = 70;
static const int BASE_TIME_WIDTH = 50;
static const int BASE_FONT_SIZE = 14;
static const int HOTKEY_ID_TOGGLE_COVER = 0x5301;
static const int BASE_SCREEN_MARGIN = 0;

// Window data stored in GWLP_USERDATA
struct TimerWindowData {
  HINSTANCE hInstance;
  TimerState state;
  HFONT hFont;
  HBRUSH hBackBrush;

  // DPI scaling
  UINT dpi;
  float dpiScale;

  // Scaled dimensions
  int windowWidth;
  int windowHeight;
  int rowHeight;
  int margin;
  int btnWidth;
  int btnSmall;
  int labelWidth;
  int timeWidth;

  // Child controls
  HWND hLabelQuestion;
  HWND hLabelQuestionTime;
  HWND hProgressQuestion;
  HWND hBtnStartStop;

  HWND hLabelBlock;
  HWND hLabelBlockTime;
  HWND hProgressBlock;
  HWND hBtnPause;
  HWND hBtnClose;
  HWND hBtnSettings;
  HWND hCoverSquare;
  bool coverHotkeyRegistered;
  bool squareOnlyMode;

  // Scale a value by DPI
  int Scale(int value) const { return MulDiv(value, dpi, 96); }

  void ComputeScaledDimensions() {
    windowWidth = Scale(BASE_WINDOW_WIDTH);
    windowHeight = Scale(BASE_WINDOW_HEIGHT);
    rowHeight = Scale(BASE_ROW_HEIGHT);
    margin = Scale(BASE_MARGIN);
    btnWidth = Scale(BASE_BTN_WIDTH);
    btnSmall = Scale(BASE_BTN_SMALL);
    labelWidth = Scale(BASE_LABEL_WIDTH);
    timeWidth = Scale(BASE_TIME_WIDTH);
  }
};

static TimerWindowData* GetWindowData(HWND hWnd) {
  return reinterpret_cast<TimerWindowData*>(
      GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

static RECT GetTimerVirtualBounds(UINT dpi) {
  int margin = MulDiv(BASE_SCREEN_MARGIN, dpi == 0 ? 96 : dpi, 96);

  RECT bounds = {};
  bounds.left = GetSystemMetrics(SM_XVIRTUALSCREEN) + margin;
  bounds.top = GetSystemMetrics(SM_YVIRTUALSCREEN) + margin;
  bounds.right =
      GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) -
      margin;
  bounds.bottom =
      GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) -
      margin;
  return bounds;
}

static void ClampRectToVirtualBounds(RECT* rect, const RECT& bounds) {
  int width = rect->right - rect->left;
  int height = rect->bottom - rect->top;
  int boundsWidth = bounds.right - bounds.left;
  int boundsHeight = bounds.bottom - bounds.top;

  if (width > boundsWidth) {
    rect->left = bounds.left;
    rect->right = bounds.right;
  } else {
    if (rect->left < bounds.left) {
      rect->left = bounds.left;
      rect->right = rect->left + width;
    }
    if (rect->right > bounds.right) {
      rect->right = bounds.right;
      rect->left = rect->right - width;
    }
  }

  if (height > boundsHeight) {
    rect->top = bounds.top;
    rect->bottom = bounds.bottom;
  } else {
    if (rect->top < bounds.top) {
      rect->top = bounds.top;
      rect->bottom = rect->top + height;
    }
    if (rect->bottom > bounds.bottom) {
      rect->bottom = bounds.bottom;
      rect->top = rect->bottom - height;
    }
  }
}

static bool IsTimingConfigChanged(const TimerConfig& oldConfig,
                                  const TimerConfig& newConfig) {
  return oldConfig.timePerBlock != newConfig.timePerBlock ||
         oldConfig.numBlocks != newConfig.numBlocks ||
         oldConfig.numQuestions != newConfig.numQuestions;
}

static void UpdateUI(HWND hWnd);
static void CreateChildControls(HWND hWnd, TimerWindowData* pData);

static void ToggleCoverSquareWindow(HWND hCoverSquare) {
  if (!hCoverSquare || !IsWindow(hCoverSquare)) {
    return;
  }

  if (IsWindowVisible(hCoverSquare)) {
    ShowWindow(hCoverSquare, SW_HIDE);
  } else {
    ShowWindow(hCoverSquare, SW_SHOWNOACTIVATE);
    SetWindowPos(hCoverSquare, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }
}

static void DestroyChildControls(TimerWindowData* pData) {
  if (!pData) return;

  HWND controls[] = {
      pData->hLabelQuestion,   pData->hLabelQuestionTime, pData->hProgressQuestion,
      pData->hBtnStartStop,    pData->hLabelBlock,        pData->hLabelBlockTime,
      pData->hProgressBlock,   pData->hBtnPause,          pData->hBtnClose,
      pData->hBtnSettings,
  };

  for (HWND hCtrl : controls) {
    if (hCtrl && IsWindow(hCtrl)) {
      DestroyWindow(hCtrl);
    }
  }

  pData->hLabelQuestion = NULL;
  pData->hLabelQuestionTime = NULL;
  pData->hProgressQuestion = NULL;
  pData->hBtnStartStop = NULL;
  pData->hLabelBlock = NULL;
  pData->hLabelBlockTime = NULL;
  pData->hProgressBlock = NULL;
  pData->hBtnPause = NULL;
  pData->hBtnClose = NULL;
  pData->hBtnSettings = NULL;

  if (pData->hFont) {
    DeleteObject(pData->hFont);
    pData->hFont = NULL;
  }
}

static void EnsureCoverVisible(HWND hCoverSquare) {
  if (!hCoverSquare || !IsWindow(hCoverSquare)) return;
  ShowWindow(hCoverSquare, SW_SHOWNOACTIVATE);
  SetWindowPos(hCoverSquare, HWND_TOPMOST, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

static void RebuildTimerControls(HWND hWnd, TimerWindowData* pData) {
  if (!pData) return;
  pData->ComputeScaledDimensions();
  SetWindowPos(hWnd, NULL, 0, 0, pData->windowWidth, pData->windowHeight,
               SWP_NOMOVE | SWP_NOZORDER);
  DestroyChildControls(pData);
  CreateChildControls(hWnd, pData);
}

static void ApplyConfigAndState(HWND hWnd, TimerWindowData* pData,
                                const TimerConfig& newConfig, bool wasStopped,
                                bool wasPaused, bool timingChanged) {
  SetLayeredWindowAttributes(hWnd, 0, (BYTE)(255 * newConfig.transparency / 100),
                             LWA_ALPHA);

  if (timingChanged) {
    pData->state.Initialize(newConfig);
    if (wasStopped) {
      pData->state.Stop();
    } else if (wasPaused) {
      pData->state.paused = true;
    }
    RebuildTimerControls(hWnd, pData);
  } else {
    pData->state.config.transparency = newConfig.transparency;
    if (!wasStopped) {
      pData->state.paused = wasPaused;
    }
  }
}

static void EnterCoverOnlyMode(HWND hWnd, TimerWindowData* pData) {
  if (!pData) return;
  pData->squareOnlyMode = true;
  pData->state.Stop();
  pData->state.paused = false;
  UpdateUI(hWnd);
  ShowWindow(hWnd, SW_HIDE);
  EnsureCoverVisible(pData->hCoverSquare);
}

static void OpenSettingsPanel(HWND hWnd, TimerWindowData* pData) {
  if (!pData) return;

  const bool wasSquareOnly = pData->squareOnlyMode;
  const bool wasStopped = pData->state.stopped;
  const bool wasPaused = pData->state.paused;
  const bool wasRunning = pData->state.IsRunning();

  if (wasRunning) {
    pData->state.TogglePause();
    UpdateUI(hWnd);
  }

  TimerConfig newConfig = pData->state.config;
  const HWND hParent = wasSquareOnly ? NULL : hWnd;
  const SetupDialogResult result =
      ShowSettingsDialog(pData->hInstance, hParent, newConfig);

  if (result == SetupDialogResult::Cancelled) {
    if (wasRunning) {
      pData->state.TogglePause();
    }
    if (wasSquareOnly) {
      pData->squareOnlyMode = true;
      ShowWindow(hWnd, SW_HIDE);
      EnsureCoverVisible(pData->hCoverSquare);
    }
    UpdateUI(hWnd);
    return;
  }

  const bool timingChanged = IsTimingConfigChanged(pData->state.config, newConfig);
  ApplyConfigAndState(hWnd, pData, newConfig, wasStopped, wasPaused, timingChanged);

  if (result == SetupDialogResult::SquareOnly) {
    EnterCoverOnlyMode(hWnd, pData);
    return;
  }

  pData->squareOnlyMode = false;
  if (wasSquareOnly) {
    ShowWindow(hWnd, SW_SHOWNORMAL);
    SetForegroundWindow(hWnd);
  } else if (wasRunning && !timingChanged) {
    pData->state.TogglePause();
  }

  UpdateUI(hWnd);
}

static void UpdateUI(HWND hWnd) {
  TimerWindowData* pData = GetWindowData(hWnd);
  if (!pData) return;

  TimerState& state = pData->state;
  wchar_t buf[64];

  // Update question label
  if (pData->hLabelQuestion) {
    swprintf_s(buf, L"Q: %d/%d", state.currentQuestion, state.config.numQuestions);
    SetWindowText(pData->hLabelQuestion, buf);
  }

  // Update question time
  if (pData->hLabelQuestionTime) {
    TimerState::FormatTime(state.questionTimeElapsed, buf, 64);
    SetWindowText(pData->hLabelQuestionTime, buf);
  }

  // Update question progress bar
  if (pData->hProgressQuestion) {
    SendMessage(pData->hProgressQuestion, PBM_SETPOS, state.GetQuestionProgress(),
                0);
  }

  // Update block label
  if (pData->hLabelBlock) {
    swprintf_s(buf, L"Block %d/%d", state.currentBlock, state.config.numBlocks);
    SetWindowText(pData->hLabelBlock, buf);
  }

  // Update block time (show remaining time in current block)
  if (pData->hLabelBlockTime) {
    int blockRemaining = state.config.timePerBlockSeconds - state.blockTimeElapsed;
    TimerState::FormatTime(blockRemaining, buf, 64);
    SetWindowText(pData->hLabelBlockTime, buf);
  }

  // Update block progress bar
  if (pData->hProgressBlock) {
    SendMessage(pData->hProgressBlock, PBM_SETPOS, state.GetBlockProgress(), 0);
  }

  // Update button states
  if (state.stopped) {
    SetWindowText(pData->hBtnStartStop, L"Start");
    EnableWindow(pData->hBtnPause, FALSE);
  } else {
    SetWindowText(pData->hBtnStartStop, L"Stop");
    EnableWindow(pData->hBtnPause, TRUE);
  }

  if (state.paused) {
    SetWindowText(pData->hBtnPause, L"Resume");
  } else {
    SetWindowText(pData->hBtnPause, L"Pause");
  }
}

static void CreateChildControls(HWND hWnd, TimerWindowData* pData) {
  HINSTANCE hInst = pData->hInstance;

  pData->hLabelQuestion = NULL;
  pData->hLabelQuestionTime = NULL;
  pData->hProgressQuestion = NULL;
  pData->hBtnStartStop = NULL;
  pData->hLabelBlock = NULL;
  pData->hLabelBlockTime = NULL;
  pData->hProgressBlock = NULL;
  pData->hBtnPause = NULL;
  pData->hBtnClose = NULL;
  pData->hBtnSettings = NULL;

  // Create DPI-scaled font
  int fontSize = pData->Scale(BASE_FONT_SIZE);
  pData->hFont =
      CreateFont(-fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                 DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                 CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

  // Use scaled dimensions
  int margin = pData->margin;
  int rowHeight = pData->rowHeight;
  int labelWidth = pData->labelWidth;
  int timeWidth = pData->timeWidth;
  int btnWidth = pData->btnWidth;
  int btnSmall = pData->btnSmall;
  int windowWidth = pData->windowWidth;

  // Row 1: Question info
  int x = margin;
  int y = margin;

  // Question label
  pData->hLabelQuestion =
      CreateWindow(L"STATIC", L"Q: 1/5", WS_CHILD | WS_VISIBLE | SS_LEFT, x,
                   y + pData->Scale(3), labelWidth, rowHeight - pData->Scale(6),
                   hWnd, (HMENU)IDC_STATIC_QUESTION, hInst, NULL);
  x += labelWidth;

  // Question time
  pData->hLabelQuestionTime =
      CreateWindow(L"STATIC", L"00:00", WS_CHILD | WS_VISIBLE | SS_CENTER, x,
                   y + pData->Scale(3), timeWidth, rowHeight - pData->Scale(6),
                   hWnd, (HMENU)IDC_STATIC_QUESTION_TIME, hInst, NULL);
  x += timeWidth;

  // Question progress bar
  int progressWidth = windowWidth - x - btnSmall * 2 - margin * 5;
  pData->hProgressQuestion = CreateWindow(
      PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, x,
      y + pData->Scale(4), progressWidth, rowHeight - pData->Scale(10), hWnd,
      (HMENU)IDC_PROGRESS_QUESTION, hInst, NULL);
  SetWindowTheme(pData->hProgressQuestion, L"", L"");
  SendMessage(pData->hProgressQuestion, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
  SendMessage(pData->hProgressQuestion, PBM_SETBARCOLOR, 0, RGB(0, 180, 0));
  SendMessage(pData->hProgressQuestion, PBM_SETBKCOLOR, 0, RGB(220, 220, 220));
  x += progressWidth + margin;

  // Settings button (gear) - top row
  pData->hBtnSettings = CreateWindow(
      L"BUTTON", L"\u2699", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x,
      y + pData->Scale(1), btnSmall, rowHeight - pData->Scale(4), hWnd,
      (HMENU)IDC_BTN_SETTINGS, hInst, NULL);
  x += btnSmall + margin;

  // Close button (red X) - top row
  pData->hBtnClose =
      CreateWindow(L"BUTTON", L"X", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x,
                   y + pData->Scale(1), btnSmall, rowHeight - pData->Scale(4),
                   hWnd, (HMENU)IDC_BTN_CLOSE, hInst, NULL);

  // Row 2: Block info
  x = margin;
  y = margin + rowHeight;

  pData->hLabelBlock = CreateWindow(
      L"STATIC", L"Block 1/3", WS_CHILD | WS_VISIBLE | SS_LEFT, x,
      y + pData->Scale(3), labelWidth, rowHeight - pData->Scale(6), hWnd,
      (HMENU)IDC_STATIC_BLOCK, hInst, NULL);
  x += labelWidth;

  pData->hLabelBlockTime = CreateWindow(
      L"STATIC", L"00:00", WS_CHILD | WS_VISIBLE | SS_CENTER, x,
      y + pData->Scale(3), timeWidth, rowHeight - pData->Scale(6), hWnd,
      (HMENU)IDC_STATIC_BLOCK_TIME, hInst, NULL);
  x += timeWidth;

  int blockProgressWidth = windowWidth - x - btnWidth * 2 - margin * 5;
  pData->hProgressBlock = CreateWindow(
      PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, x,
      y + pData->Scale(4), blockProgressWidth, rowHeight - pData->Scale(10),
      hWnd, (HMENU)IDC_PROGRESS_BLOCK, hInst, NULL);
  SetWindowTheme(pData->hProgressBlock, L"", L"");
  SendMessage(pData->hProgressBlock, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
  SendMessage(pData->hProgressBlock, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
  SendMessage(pData->hProgressBlock, PBM_SETBKCOLOR, 0, RGB(220, 220, 220));
  x += blockProgressWidth + margin;

  pData->hBtnStartStop = CreateWindow(
      L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x,
      y + pData->Scale(1), btnWidth, rowHeight - pData->Scale(4), hWnd,
      (HMENU)IDC_BTN_START_STOP, hInst, NULL);
  x += btnWidth + margin;

  pData->hBtnPause = CreateWindow(
      L"BUTTON", L"Pause", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
      x, y + pData->Scale(1), btnWidth, rowHeight - pData->Scale(4), hWnd,
      (HMENU)IDC_BTN_PAUSE, hInst, NULL);

  // Apply font to all controls
  HWND controls[] = {pData->hLabelQuestion, pData->hLabelQuestionTime,
                     pData->hLabelBlock,    pData->hLabelBlockTime,
                     pData->hBtnStartStop,  pData->hBtnPause,
                     pData->hBtnClose,      pData->hBtnSettings};
  for (HWND hCtrl : controls) {
    SendMessage(hCtrl, WM_SETFONT, (WPARAM)pData->hFont, TRUE);
  }
}

LRESULT CALLBACK TimerWindowProc(HWND hWnd, UINT message, WPARAM wParam,
                                 LPARAM lParam) {
  TimerWindowData* pData = GetWindowData(hWnd);

  switch (message) {
    case WM_CREATE: {
      CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
      TimerConfig* pConfig =
          reinterpret_cast<TimerConfig*>(pcs->lpCreateParams);

      // Allocate window data
      pData = new TimerWindowData();
      pData->hInstance = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
      pData->state.Initialize(*pConfig);
      pData->hBackBrush = CreateSolidBrush(RGB(45, 45, 48));
      pData->hCoverSquare = NULL;
      pData->coverHotkeyRegistered = false;
      pData->squareOnlyMode = false;

      // Get DPI for this window
      pData->dpi = GetDpiForWindow(hWnd);
      if (pData->dpi == 0) {
        // Fallback for older Windows versions
        HDC hdc = GetDC(hWnd);
        pData->dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(hWnd, hdc);
      }
      pData->dpiScale = pData->dpi / 96.0f;
      pData->ComputeScaledDimensions();

      SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pData);

      // Resize window to DPI-scaled size
      SetWindowPos(hWnd, NULL, 0, 0, pData->windowWidth, pData->windowHeight,
                   SWP_NOMOVE | SWP_NOZORDER);

      // Create child controls
      CreateChildControls(hWnd, pData);

      // Create always-on-top black cover square for hiding answer choices.
      pData->hCoverSquare = CreateCoverSquareWindow(pData->hInstance, hWnd);

      UINT modifiers = MOD_SHIFT;
#ifdef MOD_NOREPEAT
      modifiers |= MOD_NOREPEAT;
#endif
      pData->coverHotkeyRegistered =
          RegisterHotKey(hWnd, HOTKEY_ID_TOGGLE_COVER, modifiers, VK_SPACE);
      if (!pData->coverHotkeyRegistered) {
        pData->coverHotkeyRegistered =
            RegisterHotKey(hWnd, HOTKEY_ID_TOGGLE_COVER, MOD_SHIFT, VK_SPACE);
      }

      // Set transparency
      SetLayeredWindowAttributes(
          hWnd, 0, (BYTE)(255 * pConfig->transparency / 100), LWA_ALPHA);

      // Update UI with initial values
      UpdateUI(hWnd);

      // Start the timer (1 second intervals)
      SetTimer(hWnd, IDT_TIMER, 1000, NULL);

      return 0;
    }

    case WM_TIMER: {
      if (wParam == IDT_TIMER && pData) {
        TickStatus status = pData->state.Tick();
        UpdateUI(hWnd);

        if (status == TickStatus::Completed) {
          KillTimer(hWnd, IDT_TIMER);
          MessageBox(hWnd, L"All blocks completed!", L"Timer Finished",
                     MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
          DestroyWindow(hWnd);
        }
      }
      return 0;
    }

    case WM_HOTKEY:
      if (pData && wParam == HOTKEY_ID_TOGGLE_COVER) {
        ToggleCoverSquareWindow(pData->hCoverSquare);
      }
      return 0;

    case WM_COVER_SQUARE_OPEN_SETTINGS:
      if (pData) {
        OpenSettingsPanel(hWnd, pData);
      }
      return 0;

    case WM_COVER_SQUARE_CLOSE_APP:
      DestroyWindow(hWnd);
      return 0;

    case WM_ENTER_COVER_ONLY_MODE:
      if (pData) {
        EnterCoverOnlyMode(hWnd, pData);
      }
      return 0;

    case WM_COMMAND: {
      if (!pData) break;

      switch (LOWORD(wParam)) {
        case IDC_BTN_START_STOP:
          if (pData->state.stopped) {
            pData->state.Start();
          } else {
            pData->state.Stop();
          }
          UpdateUI(hWnd);
          return 0;

        case IDC_BTN_PAUSE:
          pData->state.TogglePause();
          UpdateUI(hWnd);
          return 0;

        case IDC_BTN_CLOSE:
          DestroyWindow(hWnd);
          return 0;

        case IDC_BTN_SETTINGS: {
          OpenSettingsPanel(hWnd, pData);
          return 0;
        }
      }
      break;
    }

    case WM_UPDATE_TRANSPARENCY: {
      // Live transparency update from settings dialog
      int transparency = (int)wParam;
      SetLayeredWindowAttributes(hWnd, 0, (BYTE)(255 * transparency / 100),
                                 LWA_ALPHA);
      if (pData) {
        pData->state.config.transparency = transparency;
      }
      return 0;
    }

    case WM_DPICHANGED: {
      // Handle DPI change (e.g., moving window to different monitor)
      if (pData) {
        pData->dpi = HIWORD(wParam);
        pData->dpiScale = pData->dpi / 96.0f;
        pData->ComputeScaledDimensions();

        // Get suggested new window rect
        RECT* prcNewWindow = (RECT*)lParam;
        RECT clamped = *prcNewWindow;
        ClampRectToVirtualBounds(&clamped, GetTimerVirtualBounds(pData->dpi));
        SetWindowPos(hWnd, NULL, clamped.left, clamped.top,
                     clamped.right - clamped.left, clamped.bottom - clamped.top,
                     SWP_NOZORDER | SWP_NOACTIVATE);

        // Recreate font with new size
        if (pData->hFont) {
          DeleteObject(pData->hFont);
        }
        int fontSize = pData->Scale(BASE_FONT_SIZE);
        pData->hFont = CreateFont(-fontSize, 0, 0, 0, FW_NORMAL, FALSE, FALSE,
                                  FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                  DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        // Update font on all controls
        HWND controls[] = {pData->hLabelQuestion, pData->hLabelQuestionTime,
                           pData->hLabelBlock,    pData->hLabelBlockTime,
                           pData->hBtnStartStop,  pData->hBtnPause,
                           pData->hBtnClose,      pData->hBtnSettings};
        for (HWND hCtrl : controls) {
          SendMessage(hCtrl, WM_SETFONT, (WPARAM)pData->hFont, TRUE);
        }

        // Force redraw
        InvalidateRect(hWnd, NULL, TRUE);
      }
      return 0;
    }

    case WM_WINDOWPOSCHANGING: {
      WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(lParam);
      if (wp && ((wp->flags & SWP_NOMOVE) == 0 || (wp->flags & SWP_NOSIZE) == 0)) {
        UINT dpi = pData ? pData->dpi : GetDpiForWindow(hWnd);
        if (dpi == 0) dpi = 96;

        RECT pending = {wp->x, wp->y, wp->x + wp->cx, wp->y + wp->cy};
        ClampRectToVirtualBounds(&pending, GetTimerVirtualBounds(dpi));
        wp->x = pending.left;
        wp->y = pending.top;
        wp->cx = pending.right - pending.left;
        wp->cy = pending.bottom - pending.top;
      }
      return 0;
    }

    case WM_NCHITTEST: {
      // Allow dragging from anywhere on the window
      LRESULT hit = DefWindowProc(hWnd, message, wParam, lParam);
      if (hit == HTCLIENT) {
        return HTCAPTION;
      }
      return hit;
    }

    case WM_CTLCOLORSTATIC: {
      // Dark background for static controls
      if (pData) {
        HDC hdcStatic = (HDC)wParam;
        SetTextColor(hdcStatic, RGB(240, 240, 240));
        SetBkColor(hdcStatic, RGB(45, 45, 48));
        return (LRESULT)pData->hBackBrush;
      }
      break;
    }

    case WM_ERASEBKGND: {
      // Dark background
      if (pData) {
        HDC hdc = (HDC)wParam;
        RECT rc;
        GetClientRect(hWnd, &rc);
        FillRect(hdc, &rc, pData->hBackBrush);
        return 1;
      }
      break;
    }

    case WM_DESTROY: {
      KillTimer(hWnd, IDT_TIMER);
      if (pData) {
        if (pData->coverHotkeyRegistered) {
          UnregisterHotKey(hWnd, HOTKEY_ID_TOGGLE_COVER);
          pData->coverHotkeyRegistered = false;
        }
        if (pData->hCoverSquare && IsWindow(pData->hCoverSquare)) {
          DestroyWindow(pData->hCoverSquare);
          pData->hCoverSquare = NULL;
        }
        if (pData->hFont) DeleteObject(pData->hFont);
        if (pData->hBackBrush) DeleteObject(pData->hBackBrush);
        delete pData;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
      }
      PostQuitMessage(0);
      return 0;
    }
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

bool RegisterTimerWindowClass(HINSTANCE hInstance) {
  if (!RegisterCoverSquareWindowClass(hInstance)) {
    return false;
  }

  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = TimerWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
  if (!wc.hIcon) wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;  // We handle painting
  wc.lpszMenuName = NULL;
  wc.lpszClassName = TIMER_WINDOW_CLASS;
  wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP_ICON));
  if (!wc.hIconSm) wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

  return RegisterClassEx(&wc) != 0;
}

HWND CreateTimerWindow(HINSTANCE hInstance, const TimerConfig& config) {
  // Get DPI for primary monitor to calculate initial size
  UINT dpi = 96;
  HDC hdc = GetDC(NULL);
  if (hdc) {
    dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(NULL, hdc);
  }

  // Scale initial dimensions
  int scaledWidth = MulDiv(BASE_WINDOW_WIDTH, dpi, 96);
  int scaledHeight = MulDiv(BASE_WINDOW_HEIGHT, dpi, 96);
  int scaledY = MulDiv(50, dpi, 96);

  // Calculate position (top-center of screen)
  int screenWidth = GetSystemMetrics(SM_CXSCREEN);
  int x = (screenWidth - scaledWidth) / 2;

  // Create with WS_POPUP (no title bar), WS_EX_TOPMOST (always on top),
  // WS_EX_LAYERED (transparency)
  HWND hWnd = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, TIMER_WINDOW_CLASS,
                             L"Wolf-Timer", WS_POPUP | WS_VISIBLE, x,
                             scaledY, scaledWidth, scaledHeight, NULL, NULL,
                             hInstance, (LPVOID)&config);

  return hWnd;
}

TimerState* GetTimerState(HWND hWnd) {
  TimerWindowData* pData = GetWindowData(hWnd);
  return pData ? &pData->state : nullptr;
}
