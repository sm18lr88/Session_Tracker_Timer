#include "CoverSquareWindow.h"

#include <windowsx.h>

#include <string>

namespace {

constexpr wchar_t kCoverSquareClassName[] = L"WolfTimerCoverSquareClass";
constexpr wchar_t kSettingsSection[] = L"CoverSquare";
constexpr wchar_t kSettingsKeyX[] = L"x";
constexpr wchar_t kSettingsKeyY[] = L"y";
constexpr wchar_t kSettingsKeyWidth[] = L"width";
constexpr wchar_t kSettingsKeyHeight[] = L"height";
constexpr wchar_t kSettingsKeyLegacySize[] = L"size";
constexpr wchar_t kFallbackSettingsFile[] = L".\\session_timer_cover_square.ini";

constexpr int kBaseInitialSize = 260;     // @96 DPI
constexpr int kBaseMinSize = 120;         // @96 DPI
constexpr int kBaseResizeGrip = 12;       // @96 DPI
constexpr int kBaseScreenMargin = 8;      // @96 DPI
constexpr UINT_PTR kCoverMenuSettings = 1;
constexpr UINT_PTR kCoverMenuClose = 2;

enum class DragMode {
  None,
  Move,
  Left,
  Right,
  Top,
  Bottom,
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight
};

struct CoverSquareData {
  bool dragging = false;
  DragMode dragMode = DragMode::None;
  POINT dragStartCursor = {0, 0};  // Screen coordinates
  RECT dragStartRect = {0, 0, 0, 0};
  HWND hController = nullptr;
};

struct CoverSquareCreateParams {
  HWND hController = nullptr;
};

int ScaleForDpi(int value, UINT dpi) { return MulDiv(value, dpi, 96); }

std::wstring GetSettingsFilePath() {
  wchar_t appDataPath[MAX_PATH] = {};
  const DWORD pathLen =
      GetEnvironmentVariableW(L"APPDATA", appDataPath, _countof(appDataPath));
  if (pathLen == 0 || pathLen >= _countof(appDataPath)) {
    return kFallbackSettingsFile;
  }

  std::wstring settingsDir = std::wstring(appDataPath) + L"\\WolfTimer";
  CreateDirectoryW(settingsDir.c_str(), nullptr);
  return settingsDir + L"\\cover_square.ini";
}

bool ReadIntSetting(const std::wstring& settingsFile, const wchar_t* key,
                    int* value) {
  wchar_t text[32] = {};
  GetPrivateProfileStringW(kSettingsSection, key, L"", text, _countof(text),
                           settingsFile.c_str());
  if (text[0] == L'\0') {
    return false;
  }

  *value = _wtoi(text);
  return true;
}

void WriteIntSetting(const std::wstring& settingsFile, const wchar_t* key,
                     int value) {
  wchar_t text[32] = {};
  swprintf_s(text, L"%d", value);
  WritePrivateProfileStringW(kSettingsSection, key, text, settingsFile.c_str());
}

UINT GetWindowDpi(HWND hWnd) {
  UINT dpi = GetDpiForWindow(hWnd);
  if (dpi != 0) return dpi;

  HDC hdc = GetDC(hWnd);
  if (!hdc) return 96;
  dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX));
  ReleaseDC(hWnd, hdc);
  return dpi == 0 ? 96 : dpi;
}

RECT GetVirtualDesktopBounds(HWND hWnd) {
  const UINT dpi = GetWindowDpi(hWnd);
  const int margin = ScaleForDpi(kBaseScreenMargin, dpi);

  RECT bounds = {};
  bounds.left = GetSystemMetrics(SM_XVIRTUALSCREEN) + margin;
  bounds.top = GetSystemMetrics(SM_YVIRTUALSCREEN) + margin;
  bounds.right =
      GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN) - margin;
  bounds.bottom =
      GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN) - margin;

  if (bounds.right <= bounds.left) bounds.right = bounds.left + 1;
  if (bounds.bottom <= bounds.top) bounds.bottom = bounds.top + 1;
  return bounds;
}

struct WindowLimits {
  RECT bounds;
  int minWidth;
  int minHeight;
};

WindowLimits GetWindowLimits(HWND hWnd) {
  WindowLimits limits = {};
  limits.bounds = GetVirtualDesktopBounds(hWnd);

  const UINT dpi = GetWindowDpi(hWnd);
  limits.minWidth = ScaleForDpi(kBaseMinSize, dpi);
  limits.minHeight = ScaleForDpi(kBaseMinSize, dpi);

  const int maxWidth = limits.bounds.right - limits.bounds.left;
  const int maxHeight = limits.bounds.bottom - limits.bounds.top;

  if (limits.minWidth > maxWidth) limits.minWidth = maxWidth;
  if (limits.minHeight > maxHeight) limits.minHeight = maxHeight;

  return limits;
}

void ClampRectToBounds(RECT* rect, const RECT& bounds) {
  const int width = rect->right - rect->left;
  const int height = rect->bottom - rect->top;
  const int boundsWidth = bounds.right - bounds.left;
  const int boundsHeight = bounds.bottom - bounds.top;

  if (width >= boundsWidth) {
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

  if (height >= boundsHeight) {
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

bool ResizingLeft(DragMode mode) {
  return mode == DragMode::Left || mode == DragMode::TopLeft ||
         mode == DragMode::BottomLeft;
}

bool ResizingRight(DragMode mode) {
  return mode == DragMode::Right || mode == DragMode::TopRight ||
         mode == DragMode::BottomRight;
}

bool ResizingTop(DragMode mode) {
  return mode == DragMode::Top || mode == DragMode::TopLeft ||
         mode == DragMode::TopRight;
}

bool ResizingBottom(DragMode mode) {
  return mode == DragMode::Bottom || mode == DragMode::BottomLeft ||
         mode == DragMode::BottomRight;
}

void EnforceRectConstraints(const WindowLimits& limits, RECT* rect) {
  int width = rect->right - rect->left;
  int height = rect->bottom - rect->top;

  if (width < limits.minWidth) rect->right = rect->left + limits.minWidth;
  if (height < limits.minHeight) rect->bottom = rect->top + limits.minHeight;

  width = rect->right - rect->left;
  height = rect->bottom - rect->top;

  const int maxWidth = limits.bounds.right - limits.bounds.left;
  const int maxHeight = limits.bounds.bottom - limits.bounds.top;
  if (width > maxWidth) rect->right = rect->left + maxWidth;
  if (height > maxHeight) rect->bottom = rect->top + maxHeight;

  ClampRectToBounds(rect, limits.bounds);
}

void EnforceResizeConstraints(HWND hWnd, RECT* rect, DragMode mode) {
  const WindowLimits limits = GetWindowLimits(hWnd);

  if (ResizingLeft(mode)) {
    if (rect->left < limits.bounds.left) rect->left = limits.bounds.left;
    if (rect->right - rect->left < limits.minWidth) {
      rect->left = rect->right - limits.minWidth;
    }
  } else if (ResizingRight(mode)) {
    if (rect->right > limits.bounds.right) rect->right = limits.bounds.right;
    if (rect->right - rect->left < limits.minWidth) {
      rect->right = rect->left + limits.minWidth;
    }
  }

  if (ResizingTop(mode)) {
    if (rect->top < limits.bounds.top) rect->top = limits.bounds.top;
    if (rect->bottom - rect->top < limits.minHeight) {
      rect->top = rect->bottom - limits.minHeight;
    }
  } else if (ResizingBottom(mode)) {
    if (rect->bottom > limits.bounds.bottom) rect->bottom = limits.bounds.bottom;
    if (rect->bottom - rect->top < limits.minHeight) {
      rect->bottom = rect->top + limits.minHeight;
    }
  }

  EnforceRectConstraints(limits, rect);
}

RECT GetDefaultRect(HWND hWnd) {
  const UINT dpi = GetWindowDpi(hWnd);
  const int defaultSize = ScaleForDpi(kBaseInitialSize, dpi);
  const WindowLimits limits = GetWindowLimits(hWnd);
  const RECT bounds = limits.bounds;

  RECT rect = {};
  rect.left = bounds.left + ((bounds.right - bounds.left) - defaultSize) / 2;
  rect.top = bounds.top + ((bounds.bottom - bounds.top) - defaultSize) / 2;
  rect.right = rect.left + defaultSize;
  rect.bottom = rect.top + defaultSize;

  EnforceRectConstraints(limits, &rect);
  return rect;
}

void SavePlacement(HWND hWnd) {
  RECT rect = {};
  if (!GetWindowRect(hWnd, &rect)) {
    return;
  }

  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;
  const std::wstring settingsFile = GetSettingsFilePath();
  WriteIntSetting(settingsFile, kSettingsKeyX, rect.left);
  WriteIntSetting(settingsFile, kSettingsKeyY, rect.top);
  WriteIntSetting(settingsFile, kSettingsKeyWidth, width);
  WriteIntSetting(settingsFile, kSettingsKeyHeight, height);
}

void RestorePlacement(HWND hWnd) {
  RECT rect = GetDefaultRect(hWnd);
  const std::wstring settingsFile = GetSettingsFilePath();

  int savedX = 0;
  int savedY = 0;
  int savedWidth = 0;
  int savedHeight = 0;
  int savedLegacySize = 0;
  const bool hasX = ReadIntSetting(settingsFile, kSettingsKeyX, &savedX);
  const bool hasY = ReadIntSetting(settingsFile, kSettingsKeyY, &savedY);
  const bool hasWidth = ReadIntSetting(settingsFile, kSettingsKeyWidth, &savedWidth);
  const bool hasHeight =
      ReadIntSetting(settingsFile, kSettingsKeyHeight, &savedHeight);
  const bool hasLegacySize =
      ReadIntSetting(settingsFile, kSettingsKeyLegacySize, &savedLegacySize);

  if (hasX) {
    rect.left = savedX;
  }
  if (hasY) {
    rect.top = savedY;
  }

  int width = rect.right - rect.left;
  int height = rect.bottom - rect.top;

  if (hasWidth && savedWidth > 0) {
    width = savedWidth;
  } else if (hasLegacySize && savedLegacySize > 0) {
    width = savedLegacySize;
  }

  if (hasHeight && savedHeight > 0) {
    height = savedHeight;
  } else if (hasLegacySize && savedLegacySize > 0) {
    height = savedLegacySize;
  }

  rect.right = rect.left + width;
  rect.bottom = rect.top + height;
  EnforceRectConstraints(GetWindowLimits(hWnd), &rect);

  SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top,
               rect.right - rect.left, rect.bottom - rect.top, SWP_NOACTIVATE);
}

DragMode HitTestDragMode(HWND hWnd, int x, int y) {
  RECT rc = {};
  GetClientRect(hWnd, &rc);

  const UINT dpi = GetWindowDpi(hWnd);
  const int grip = ScaleForDpi(kBaseResizeGrip, dpi);

  const bool left = x <= rc.left + grip;
  const bool right = x >= rc.right - grip;
  const bool top = y <= rc.top + grip;
  const bool bottom = y >= rc.bottom - grip;

  if (top && left) return DragMode::TopLeft;
  if (top && right) return DragMode::TopRight;
  if (bottom && left) return DragMode::BottomLeft;
  if (bottom && right) return DragMode::BottomRight;
  if (left) return DragMode::Left;
  if (right) return DragMode::Right;
  if (top) return DragMode::Top;
  if (bottom) return DragMode::Bottom;
  return DragMode::Move;
}

HCURSOR CursorForMode(DragMode mode) {
  switch (mode) {
    case DragMode::TopLeft:
    case DragMode::BottomRight:
      return LoadCursor(nullptr, IDC_SIZENWSE);
    case DragMode::TopRight:
    case DragMode::BottomLeft:
      return LoadCursor(nullptr, IDC_SIZENESW);
    case DragMode::Left:
    case DragMode::Right:
      return LoadCursor(nullptr, IDC_SIZEWE);
    case DragMode::Top:
    case DragMode::Bottom:
      return LoadCursor(nullptr, IDC_SIZENS);
    case DragMode::Move:
      return LoadCursor(nullptr, IDC_SIZEALL);
    case DragMode::None:
      return LoadCursor(nullptr, IDC_ARROW);
  }
  return LoadCursor(nullptr, IDC_ARROW);
}

CoverSquareData* GetCoverData(HWND hWnd) {
  return reinterpret_cast<CoverSquareData*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

void PaintSolidBlack(HWND hWnd) {
  PAINTSTRUCT ps = {};
  HDC hdc = BeginPaint(hWnd, &ps);
  RECT rc = {};
  GetClientRect(hWnd, &rc);
  FillRect(hdc, &rc, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
  EndPaint(hWnd, &ps);
}

void ShowCoverContextMenu(HWND hWnd, CoverSquareData* data, POINT ptScreen) {
  if (!data || !data->hController || !IsWindow(data->hController)) {
    return;
  }

  HMENU menu = CreatePopupMenu();
  if (!menu) return;

  AppendMenu(menu, MF_STRING, kCoverMenuSettings, L"Settings...");
  AppendMenu(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenu(menu, MF_STRING, kCoverMenuClose, L"Close");

  SetForegroundWindow(hWnd);
  UINT selected = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON, ptScreen.x,
                                 ptScreen.y, 0, hWnd, nullptr);
  DestroyMenu(menu);

  if (selected == kCoverMenuSettings) {
    PostMessage(data->hController, WM_COVER_SQUARE_OPEN_SETTINGS, 0, 0);
  } else if (selected == kCoverMenuClose) {
    PostMessage(data->hController, WM_COVER_SQUARE_CLOSE_APP, 0, 0);
  }
}

LRESULT CALLBACK CoverSquareWindowProc(HWND hWnd, UINT message, WPARAM wParam,
                                       LPARAM lParam) {
  CoverSquareData* data = GetCoverData(hWnd);

  switch (message) {
    case WM_CREATE: {
      auto* createdData = new CoverSquareData();
      CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
      if (pcs && pcs->lpCreateParams) {
        const auto* params =
            reinterpret_cast<const CoverSquareCreateParams*>(pcs->lpCreateParams);
        createdData->hController = params->hController;
      }
      SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createdData));
      RestorePlacement(hWnd);
      return 0;
    }

    case WM_SETCURSOR: {
      if (LOWORD(lParam) == HTCLIENT) {
        POINT pt = {};
        GetCursorPos(&pt);
        ScreenToClient(hWnd, &pt);
        const DragMode hoverMode = HitTestDragMode(hWnd, pt.x, pt.y);
        SetCursor(CursorForMode(hoverMode));
        return TRUE;
      }
      break;
    }

    case WM_LBUTTONDOWN: {
      if (!data) break;

      data->dragging = true;
      data->dragMode = HitTestDragMode(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
      GetCursorPos(&data->dragStartCursor);
      GetWindowRect(hWnd, &data->dragStartRect);
      SetCapture(hWnd);
      return 0;
    }

    case WM_MOUSEMOVE: {
      if (!data || !data->dragging) break;

      POINT currentCursor = {};
      GetCursorPos(&currentCursor);

      const int dx = currentCursor.x - data->dragStartCursor.x;
      const int dy = currentCursor.y - data->dragStartCursor.y;

      RECT nextRect = data->dragStartRect;

      switch (data->dragMode) {
        case DragMode::Move:
          OffsetRect(&nextRect, dx, dy);
          ClampRectToBounds(&nextRect, GetWindowLimits(hWnd).bounds);
          break;
        case DragMode::Left:
          nextRect.left += dx;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::Left);
          break;
        case DragMode::Right:
          nextRect.right += dx;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::Right);
          break;
        case DragMode::Top:
          nextRect.top += dy;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::Top);
          break;
        case DragMode::Bottom:
          nextRect.bottom += dy;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::Bottom);
          break;
        case DragMode::TopLeft:
          nextRect.left += dx;
          nextRect.top += dy;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::TopLeft);
          break;
        case DragMode::TopRight:
          nextRect.right += dx;
          nextRect.top += dy;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::TopRight);
          break;
        case DragMode::BottomLeft:
          nextRect.left += dx;
          nextRect.bottom += dy;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::BottomLeft);
          break;
        case DragMode::BottomRight:
          nextRect.right += dx;
          nextRect.bottom += dy;
          EnforceResizeConstraints(hWnd, &nextRect, DragMode::BottomRight);
          break;
        case DragMode::None:
          break;
      }

      SetWindowPos(hWnd, HWND_TOPMOST, nextRect.left, nextRect.top,
                   nextRect.right - nextRect.left, nextRect.bottom - nextRect.top,
                   SWP_NOACTIVATE);
      return 0;
    }

    case WM_LBUTTONUP:
      if (data && data->dragging) {
        data->dragging = false;
        data->dragMode = DragMode::None;
        ReleaseCapture();
        SavePlacement(hWnd);
      }
      return 0;

    case WM_CAPTURECHANGED:
      if (data) {
        data->dragging = false;
        data->dragMode = DragMode::None;
      }
      return 0;

    case WM_GETMINMAXINFO: {
      MINMAXINFO* mmi = reinterpret_cast<MINMAXINFO*>(lParam);
      if (mmi) {
        const WindowLimits limits = GetWindowLimits(hWnd);
        mmi->ptMinTrackSize.x = limits.minWidth;
        mmi->ptMinTrackSize.y = limits.minHeight;
      }
      return 0;
    }

    case WM_WINDOWPOSCHANGING: {
      WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(lParam);
      if (!wp) break;

      if ((wp->flags & SWP_NOMOVE) == 0 || (wp->flags & SWP_NOSIZE) == 0) {
        RECT pending = {wp->x, wp->y, wp->x + wp->cx, wp->y + wp->cy};
        EnforceRectConstraints(GetWindowLimits(hWnd), &pending);
        wp->x = pending.left;
        wp->y = pending.top;
        wp->cx = pending.right - pending.left;
        wp->cy = pending.bottom - pending.top;
      }
      return 0;
    }

    case WM_DPICHANGED: {
      RECT* suggestedRect = reinterpret_cast<RECT*>(lParam);
      if (suggestedRect) {
        RECT nextRect = *suggestedRect;
        EnforceRectConstraints(GetWindowLimits(hWnd), &nextRect);
        SetWindowPos(hWnd, HWND_TOPMOST, nextRect.left, nextRect.top,
                     nextRect.right - nextRect.left, nextRect.bottom - nextRect.top,
                     SWP_NOACTIVATE);
        SavePlacement(hWnd);
      }
      return 0;
    }

    case WM_DISPLAYCHANGE: {
      RECT rect = {};
      GetWindowRect(hWnd, &rect);
      EnforceRectConstraints(GetWindowLimits(hWnd), &rect);
      SetWindowPos(hWnd, HWND_TOPMOST, rect.left, rect.top, rect.right - rect.left,
                   rect.bottom - rect.top, SWP_NOACTIVATE);
      SavePlacement(hWnd);
      return 0;
    }

    case WM_PAINT:
      PaintSolidBlack(hWnd);
      return 0;

    case WM_CONTEXTMENU: {
      if (!data) return 0;

      POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
      if (pt.x == -1 && pt.y == -1) {
        RECT rc = {};
        GetWindowRect(hWnd, &rc);
        pt.x = rc.left + 20;
        pt.y = rc.top + 20;
      }
      ShowCoverContextMenu(hWnd, data, pt);
      return 0;
    }

    case WM_ERASEBKGND:
      return 1;

    case WM_NCHITTEST:
      return HTCLIENT;

    case WM_DESTROY:
      if (data) {
        SavePlacement(hWnd);
        delete data;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
      }
      return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

}  // namespace

bool RegisterCoverSquareWindowClass(HINSTANCE hInstance) {
  WNDCLASSEX wc = {};
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
  wc.lpfnWndProc = CoverSquareWindowProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = nullptr;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = kCoverSquareClassName;
  wc.hIconSm = nullptr;

  return RegisterClassEx(&wc) != 0;
}

HWND CreateCoverSquareWindow(HINSTANCE hInstance, HWND hController) {
  UINT dpi = 96;
  HDC hdc = GetDC(nullptr);
  if (hdc) {
    dpi = static_cast<UINT>(GetDeviceCaps(hdc, LOGPIXELSX));
    ReleaseDC(nullptr, hdc);
  }

  const int size = ScaleForDpi(kBaseInitialSize, dpi);

  RECT bounds = {};
  bounds.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
  bounds.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
  bounds.right = bounds.left + GetSystemMetrics(SM_CXVIRTUALSCREEN);
  bounds.bottom = bounds.top + GetSystemMetrics(SM_CYVIRTUALSCREEN);

  const int x = bounds.left + ((bounds.right - bounds.left) - size) / 2;
  const int y = bounds.top + ((bounds.bottom - bounds.top) - size) / 2;

  CoverSquareCreateParams params = {};
  params.hController = hController;

  return CreateWindowEx(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kCoverSquareClassName,
                        L"CoverSquare", WS_POPUP | WS_VISIBLE, x, y, size, size,
                        nullptr, nullptr, hInstance, &params);
}
