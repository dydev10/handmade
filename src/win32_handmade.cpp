#undef UNICODE
#include <windows.h>

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  
  switch (message)
  {
    case WM_SIZE: {
      OutputDebugStringA("WM_SIZE\n");
    } break;
  
    case WM_DESTROY: {
      OutputDebugStringA("WM_DESTROY\n");
    } break;
  
    case WM_CLOSE: {
      OutputDebugStringA("WM_CLOSE\n");
      DestroyWindow(window);
      // TODO: handle proper closing
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
      
      static DWORD operation = WHITENESS;
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
  WNDCLASS windowClass = {};
  windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowProc;
  windowClass.hInstance = instance;
  //windowClass.hIcon = ;
  windowClass.lpszClassName = "HandmadeEngineWindowClass";

  if (RegisterClass(&windowClass)) {
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
      MSG message;
      for (;;) {
        BOOL messageResult = GetMessage(&message, 0, 0, 0);

        if (messageResult > 0) {
          TranslateMessage(&message);
          DispatchMessage(&message);
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