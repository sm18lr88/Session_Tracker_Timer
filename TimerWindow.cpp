// TimerWindow.cpp - Main timer bar window implementation

#include "TimerWindow.h"

#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>

#include <cstdio>

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

static void UpdateUI(HWND hWnd) {
  TimerWindowData* pData = GetWindowData(hWnd);
  if (!pData) return;

  TimerState& state = pData->state;
  wchar_t buf[64];

  // Update question label
  swprintf_s(buf, L"Q: %d/%d", state.currentQuestion,
             state.config.numQuestions);
  SetWindowText(pData->hLabelQuestion, buf);

  // Update question time
  TimerState::FormatTime(state.questionTimeElapsed, buf, 64);
  SetWindowText(pData->hLabelQuestionTime, buf);

  // Update question progress bar
  SendMessage(pData->hProgressQuestion, PBM_SETPOS, state.GetQuestionProgress(),
              0);

  // Update block label
  swprintf_s(buf, L"Block %d/%d", state.currentBlock, state.config.numBlocks);
  SetWindowText(pData->hLabelBlock, buf);

  // Update block time (show remaining time in current block)
  int blockRemaining =
      state.config.timePerBlockSeconds - state.blockTimeElapsed;
  TimerState::FormatTime(blockRemaining, buf, 64);
  SetWindowText(pData->hLabelBlockTime, buf);

  // Update block progress bar
  SendMessage(pData->hProgressBlock, PBM_SETPOS, state.GetBlockProgress(), 0);

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
  int progressWidth = windowWidth - x - btnWidth - margin * 3;
  pData->hProgressQuestion = CreateWindow(
      PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, x,
      y + pData->Scale(4), progressWidth, rowHeight - pData->Scale(10), hWnd,
      (HMENU)IDC_PROGRESS_QUESTION, hInst, NULL);
  // Set green color for question progress
  SetWindowTheme(pData->hProgressQuestion, L"", L"");
  SendMessage(pData->hProgressQuestion, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
  SendMessage(pData->hProgressQuestion, PBM_SETBARCOLOR, 0, RGB(0, 180, 0));
  SendMessage(pData->hProgressQuestion, PBM_SETBKCOLOR, 0, RGB(220, 220, 220));
  x += progressWidth + margin;

  // Start/Stop button
  pData->hBtnStartStop = CreateWindow(
      L"BUTTON", L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x,
      y + pData->Scale(1), btnWidth, rowHeight - pData->Scale(4), hWnd,
      (HMENU)IDC_BTN_START_STOP, hInst, NULL);

  // Row 2: Block info
  x = margin;
  y = margin + rowHeight;

  // Block label
  pData->hLabelBlock =
      CreateWindow(L"STATIC", L"Block 1/3", WS_CHILD | WS_VISIBLE | SS_LEFT, x,
                   y + pData->Scale(3), labelWidth, rowHeight - pData->Scale(6),
                   hWnd, (HMENU)IDC_STATIC_BLOCK, hInst, NULL);
  x += labelWidth;

  // Block time
  pData->hLabelBlockTime =
      CreateWindow(L"STATIC", L"00:00", WS_CHILD | WS_VISIBLE | SS_CENTER, x,
                   y + pData->Scale(3), timeWidth, rowHeight - pData->Scale(6),
                   hWnd, (HMENU)IDC_STATIC_BLOCK_TIME, hInst, NULL);
  x += timeWidth;

  // Block progress bar - same width calculation but account for more buttons
  int blockProgressWidth =
      windowWidth - x - btnWidth - btnSmall * 2 - margin * 5;
  pData->hProgressBlock = CreateWindow(
      PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, x,
      y + pData->Scale(4), blockProgressWidth, rowHeight - pData->Scale(10),
      hWnd, (HMENU)IDC_PROGRESS_BLOCK, hInst, NULL);
  SetWindowTheme(pData->hProgressBlock, L"", L"");
  SendMessage(pData->hProgressBlock, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
  SendMessage(pData->hProgressBlock, PBM_SETBARCOLOR, 0, RGB(0, 120, 215));
  SendMessage(pData->hProgressBlock, PBM_SETBKCOLOR, 0, RGB(220, 220, 220));
  x += blockProgressWidth + margin;

  // Pause button
  pData->hBtnPause = CreateWindow(
      L"BUTTON", L"Pause", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
      x, y + pData->Scale(1), btnWidth, rowHeight - pData->Scale(4), hWnd,
      (HMENU)IDC_BTN_PAUSE, hInst, NULL);
  x += btnWidth + margin;

  // Close button (red X)
  pData->hBtnClose =
      CreateWindow(L"BUTTON", L"X", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x,
                   y + pData->Scale(1), btnSmall, rowHeight - pData->Scale(4),
                   hWnd, (HMENU)IDC_BTN_CLOSE, hInst, NULL);
  x += btnSmall + margin;

  // Settings button (gear)
  pData->hBtnSettings = CreateWindow(
      L"BUTTON", L"\u2699",  // Gear unicode character
      WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, x, y + pData->Scale(1), btnSmall,
      rowHeight - pData->Scale(4), hWnd, (HMENU)IDC_BTN_SETTINGS, hInst, NULL);

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
          // Pause timer while settings dialog is open
          bool wasRunning = pData->state.IsRunning();
          if (wasRunning) {
            pData->state.TogglePause();
            UpdateUI(hWnd);
          }

          TimerConfig newConfig = pData->state.config;
          if (ShowSettingsDialog(pData->hInstance, hWnd, newConfig)) {
            // Apply new settings
            // Note: Changing time settings mid-session can cause issues,
            // so we only update transparency immediately
            pData->state.config.transparency = newConfig.transparency;
            SetLayeredWindowAttributes(
                hWnd, 0, (BYTE)(255 * newConfig.transparency / 100), LWA_ALPHA);
          }

          if (wasRunning) {
            pData->state.TogglePause();
            UpdateUI(hWnd);
          }
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
        SetWindowPos(hWnd, NULL, prcNewWindow->left, prcNewWindow->top,
                     prcNewWindow->right - prcNewWindow->left,
                     prcNewWindow->bottom - prcNewWindow->top,
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
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = TimerWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;  // We handle painting
  wc.lpszMenuName = NULL;
  wc.lpszClassName = TIMER_WINDOW_CLASS;
  wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

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
                             L"Session Timer", WS_POPUP | WS_VISIBLE, x,
                             scaledY, scaledWidth, scaledHeight, NULL, NULL,
                             hInstance, (LPVOID)&config);

  return hWnd;
}

TimerState* GetTimerState(HWND hWnd) {
  TimerWindowData* pData = GetWindowData(hWnd);
  return pData ? &pData->state : nullptr;
}
