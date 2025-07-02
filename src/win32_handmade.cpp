#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO: move running to a better place instead of static global
global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;  
global_variable HBITMAP bitmapHandle;
global_variable HDC bitmapDeviceContext;

/**
 * All function win Win32 prefix are custom functions to deal Windows API layer
 */

internal void Win32ResizeDIBSection(int width, int height) {
  // TODO: try creating new one before deleting current DIB Section, maybe
  
  if (bitmapHandle) {
    DeleteObject(bitmapHandle);
  }

  if (!bitmapDeviceContext) {
    // TODO: should be re-created for few cases like switching monitors, resolution change, etc
    bitmapDeviceContext = CreateCompatibleDC(0);
  }
  
  bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth = width;
  bitmapInfo.bmiHeader.biHeight = height;
  bitmapInfo.bmiHeader.biPlanes = 1;
  bitmapInfo.bmiHeader.biBitCount = 32;
  bitmapInfo.bmiHeader.biCompression = BI_RGB;
  
  bitmapHandle = CreateDIBSection(
    bitmapDeviceContext,
    &bitmapInfo,
    DIB_RGB_COLORS,
    &bitmapMemory,
    0,
    0
  );
}

internal void Win32UpdateWindow(HDC deviceContext, int x, int y, int width, int height) {
  StretchDIBits(
    deviceContext,
    x, y, width, height,
    x, y, width, height, 
    bitmapMemory,
    &bitmapInfo,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

LRESULT CALLBACK Win32MainWindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  
  switch (message)
  {
    case WM_SIZE: {
      RECT clientRect;
      GetClientRect(window, &clientRect);
      int width = clientRect.right - clientRect.left;
      int height = clientRect.bottom - clientRect.top;
      Win32ResizeDIBSection(width, height);
    } break;
  
    case WM_DESTROY: {
      // TODO: treat destroy event as error and recovering by recreating window
      running = false;
    } break;
  
    case WM_CLOSE: {
      // TODO: ask user to confirm before closing window
      running = false;
    } break;
  
    case WM_ACTIVATE: {
      OutputDebugStringA("WM_ACTIVATE\n");
    } break;
  
    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);
      int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int width = paint.rcPaint.right - x;
      int height = paint.rcPaint.bottom - y;
      Win32UpdateWindow(deviceContext, x, y, width, height);
      EndPaint(window, &paint);
    } break;
  
    default: {
//      OutputDebugStringA("default\n");
      result = DefWindowProc(window, message, wParam, lParam);
    } break;
  }

  return result;
}


int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, int cmdShow) {  
  WNDCLASSA windowClass = {};
  
  // windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = Win32MainWindowProcedure;
  windowClass.hInstance = instance;
  //windowClass.hIcon = ;
  windowClass.lpszClassName = "HandmadeEngineWindowClass";

  if (RegisterClassA(&windowClass)) {
    HWND windowHandle = CreateWindowExA(
      0,
      windowClass.lpszClassName,
      "Handmade Engine",
      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      CW_USEDEFAULT,
      0,
      0,
      instance,
      0
    );

    if (windowHandle != NULL) {
      running = true;
      while (running) {
        MSG message;
        BOOL messageResult = GetMessageA(&message, 0, 0, 0);

        if (messageResult > 0) {
          TranslateMessage(&message);
          DispatchMessageA(&message);
        } else {
          break;
        }
      }
      
    } else {
      // TODO: error logging
    }

  } else {
    // TODO: error logging
  }

  return 0;
}