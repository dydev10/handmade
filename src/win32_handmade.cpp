#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// TODO: move running to a better place instead of static global
global_variable bool running;

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  
  switch (message)
  {
    case WM_SIZE: {
      OutputDebugStringA("WM_SIZE\n");
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
      OutputDebugStringA("WM_PAINT\n");

      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);
      int x = paint.rcPaint.left;
      int y = paint.rcPaint.top;
      int width = paint.rcPaint.right - x;
      int height = paint.rcPaint.bottom - y;
      
      local_persist DWORD operation = WHITENESS;
      if (operation == WHITENESS) {
        operation = BLACKNESS;
      } else {
        operation = WHITENESS;
      }

      PatBlt(deviceContext, x, y, width, height, operation);
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
  windowClass.lpfnWndProc = MainWindowProc;
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