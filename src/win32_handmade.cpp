#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

struct Win32OffScreenBuffer{
  BITMAPINFO info;
  void *memory;  
  int width;
  int height;
  int pitch;
  int bytesPerPixel;
};

struct Win32WindowDimension {
  int width;
  int height;
};

// TODO: move running to a better place instead of static global
global_variable bool running;
global_variable Win32OffScreenBuffer globalBackBuffer;

internal void RenderCheckeredGradient(Win32OffScreenBuffer buffer, int xOffset, int yOffset) {
  // TODO: pass buffer by value or by pointer???
  uint8 *row = (uint8 *)buffer.memory;
  for (int y = 0; y < buffer.height; ++y) {
    uint32 *pixel = (uint32 *)row;  
    for (int x = 0; x < buffer.width; ++x) {
      /*
        offset          : +0 +1 +2 +3
        Pixel in memory : 00 00 00 00
        Channel         : BB GG RR xx (reversed little endian because windows reverses it to look like 0x xxRRGGBB)

        in 32bit Register     : xx RR GG BB
      */
      uint8 b = (uint8)(x + xOffset);
      uint8 g = (uint8)(y + yOffset);
      uint8 r = 0;

      // 0x xxRRGGBB
      *pixel++ = ((g << 8) | b);
    }

    row += buffer.pitch;
  }
}


/**
 * All function win Win32 prefix are custom functions to deal Windows API layer
 */

Win32WindowDimension Win32GetWindowDimension(HWND window) {
  RECT clientRect;
  GetClientRect(window, &clientRect);
  
  Win32WindowDimension result;
  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;

  return result; 
}


internal void Win32ResizeDIBSection(Win32OffScreenBuffer *buffer, int width, int height) {
  // TODO: try allocation before free
  if(buffer->memory) {
    VirtualFree(buffer->memory, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;
  buffer->bytesPerPixel = 4;

  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = -buffer->height;  // negative height ensure the framebuffer uses top left corner as origin when drawing on windows
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = (buffer->width * buffer->height) * buffer->bytesPerPixel;
  buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
  buffer->pitch = buffer->width * buffer->bytesPerPixel;

  // TODO: clear all pixels in memory to black???
}

internal void Win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, Win32OffScreenBuffer buffer, int x, int y, int width, int height) {
  StretchDIBits(
    deviceContext,
    // x, y, width, height,
    // x, y, width, height, 
    0, 0, buffer.width, buffer.height,
    0, 0, windowWidth, windowHeight,
    buffer.memory,
    &buffer.info,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  
  switch (message)
  {
    case WM_SIZE: {
      Win32WindowDimension dimension = Win32GetWindowDimension(window);
      Win32ResizeDIBSection(&globalBackBuffer, dimension.width, dimension.height);
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

      Win32WindowDimension dimension = Win32GetWindowDimension(window);
      Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, globalBackBuffer, x, y, width, height);

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
  
  windowClass.style = CS_HREDRAW | CS_VREDRAW;
  windowClass.lpfnWndProc = Win32MainWindowCallback;
  windowClass.hInstance = instance;
  //windowClass.hIcon = ;
  windowClass.lpszClassName = "HandmadeEngineWindowClass";

  if (RegisterClassA(&windowClass)) {
    HWND window = CreateWindowExA(
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

    if (window != NULL) {
      running = true;
      int xOffset = 0;
      int yOffset = 0;
        
      while (running) {
        MSG message;
        while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
          if (message.message == WM_QUIT) {
            running = false;
          }
          TranslateMessage(&message);
          DispatchMessageA(&message);
        }

        RenderCheckeredGradient(globalBackBuffer, xOffset, yOffset);

        Win32WindowDimension dimension = Win32GetWindowDimension(window);
        HDC deviceContext = GetDC(window);
        Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, globalBackBuffer, 0, 0, dimension.width, dimension.height);
        ReleaseDC(window, deviceContext);

        ++xOffset;
        yOffset += 2;
      }
    } else {
      // TODO: error logging
    }

  } else {
    // TODO: error logging
  }

  return 0;
}