#include <stdint.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// custom bool type to avoid implicit int to bool conversion
typedef int32 bool32;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

// TODO: move globalRunning to a better place instead of static global
global_variable bool32 globalRunning;
global_variable Win32OffScreenBuffer globalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER globalDSoundBuffer;

// XInput function typedef and macros to support dynamic loading of functionality from dll
// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_ 

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
  return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// DirectSound typedef and macros to support dynamic loading from dll
// DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// Debug File I/O platform services
internal void *DEBUG_PlatformReadEntireFile(char *filename) {
  void *result = 0;

  HANDLE fileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
  if (fileHandle != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(fileHandle, &fileSize)) {
      uint32 fileSize32 = SafeTruncateUInt64(fileSize.QuadPart);

      // Allocation only for debugging, real I/O will use pre allocated game memory
      result = VirtualAlloc(0, fileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if (result) {
        DWORD bytesRead;
        if (ReadFile(fileHandle, result, fileSize32, &bytesRead, NULL) && (bytesRead == fileSize32)) {
          // Log Success. Or not. its done
        } else {
          DEBUG_PlatformFreeFileMemory(result);
          result = 0;
          // Log error
        }
      } else {
        // Log error
      }
    } else {
      // Log error
    }

    CloseHandle(fileHandle);
  }

  return result;
}

internal void DEBUG_PlatformFreeFileMemory(void *bitmapMemory) {
  VirtualFree(bitmapMemory, 0, MEM_RELEASE);
}

internal bool32 *DEBUG_PlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory) {

}


internal void Win32LoadXInput(void) {
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  // use xinput1_3.dll as fallback
  if (!XInputLibrary) {
    // TODO: print diagnostic/warnings
    HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }
  // use xinput9_1_0.dll as fallback
  if (!XInputLibrary) {
    // TODO: print diagnostic/warnings
    HMODULE XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }

  if (XInputLibrary) {
    XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
    XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
  } else {
    // TODO: print diagnostic/warnings
  }
}

internal void Win32InitDSound(HWND window, int32 samplesPerSec, int32 bufferSize) {
  // Step: Load the library
  HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

  if (DSoundLibrary) {
    // Step: Get a DirectSound object
    direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
    
    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
      WAVEFORMATEX waveFormat = {};
      waveFormat.wFormatTag = WAVE_FORMAT_PCM;
      waveFormat.nChannels = 2;
      waveFormat.nSamplesPerSec = samplesPerSec;
      waveFormat.wBitsPerSample = 16;
      waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
      waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
      waveFormat.cbSize = 0;

      // Step: Set cooperative level of DirectSound object
      if (SUCCEEDED(DirectSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
        // Step: Create primary buffer
        DSBUFFERDESC bufferDescription = {}; // should call ZeroMemory or always use zero init
        // TODO: wanna play sound in background? use DSBCAPS_GLOBALFOCUS flag in dwFlags
        bufferDescription.dwSize = sizeof(bufferDescription);
        bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
        LPDIRECTSOUNDBUFFER primaryBuffer;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) {
          if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) {
            OutputDebugStringA("Primary buffer format was set\n");
          } else {
            // TODO: print diagnostics/warnings
          }
        } else {
          // TODO: print diagnostics/warnings
        }
      } else {
        // TODO: print diagnostic/warnings
      }

      // Step: Create secondary buffer
      DSBUFFERDESC bufferDescription = {};
      // TODO: maybe use DSBCAPS_GETCURRENTPOSITION2
      bufferDescription.dwSize = sizeof(bufferDescription);
      bufferDescription.dwFlags = 0;
      bufferDescription.dwBufferBytes = bufferSize;
      bufferDescription.lpwfxFormat = &waveFormat;
      if (SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDescription, &globalDSoundBuffer, 0))) {
        OutputDebugStringA("Secondary buffer was created\n");
      } else {
        // TODO: print diagnostics/warnings
      }

      // Step: Start playing
    } else {
      // TODO: print diagnostic/warnings
    }
  } else {
    // TODO: print diagnostic/warnings
  }
}

internal void Win32ClearSoundBuffer(Win32SoundOutput *soundOutput) {
  void *region1;
  DWORD region1Size;
  void *region2;
  DWORD region2Size;
  HRESULT dsBufferLock = globalDSoundBuffer->Lock(
    0,
    soundOutput->dsBufferSize,
    &region1, &region1Size,
    &region2, &region2Size,
    0
  );

  if (SUCCEEDED(dsBufferLock)) {
    uint8 *destSample = (uint8 *)region1;
    for (DWORD byteIndex = 0; byteIndex < region1Size; ++byteIndex) {
      *destSample++ = 0;
    }

    destSample = (uint8 *)region2;
    for (DWORD byteIndex = 0; byteIndex < region2Size; ++byteIndex) {
      *destSample++ = 0;
    }
  }

  globalDSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
}

internal void Win32ProcessXInputDigitalButton(WORD xInputButtonState, WORD buttonBit, GameButtonState *oldState, GameButtonState *newState) {
  newState->endedDown = (xInputButtonState & buttonBit) == buttonBit;
  newState->halfTransitionCount = (oldState->endedDown != newState->endedDown) ? 1 : 0;
}

internal void Win32FillSoundBuffer(Win32SoundOutput *soundOutput, DWORD byteToLock, DWORD bytesToWrite, GameSoundOutputBuffer *sourceBuffer) {
  /*
    buffer with 2 channel : [left right] left right ..
    data                  : int16 int16  int16  int16
    sample can sometimes mean both left and right values together(32 bit) or sometimes just a single channel(16 bit)  

  */
  void *region1;
  DWORD region1Size;
  void *region2;
  DWORD region2Size;
  // TODO: more testing needed, still weird breaks when resizing window rapidly

  HRESULT dsBufferLock = globalDSoundBuffer->Lock(
    byteToLock, bytesToWrite,
    &region1, &region1Size,
    &region2, &region2Size,
    0
  );
  if (SUCCEEDED(dsBufferLock)) {
    // TODO: assert region sizes are valid(multiples of 32 bit for 2 channels)
    int16 *destSample = (int16 *)region1;
    DWORD region1SampleCount = region1Size / soundOutput->bytesPerSample;
    int16 *sourceSample = sourceBuffer->samples;
    for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex) {
      *destSample++ = *sourceSample++;
      *destSample++ = *sourceSample++;

      ++soundOutput->runningSampleIndex;
    }
    destSample = (int16 *)region2;
    DWORD region2SampleCount = region2Size / soundOutput->bytesPerSample; 
    for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex) {
      *destSample++ = *sourceSample++;
      *destSample++ = *sourceSample++;

      ++soundOutput->runningSampleIndex;
    }

    // Unlock sound buffer after writing samples
    globalDSoundBuffer->Unlock(region1, region1Size, region2, region2Size);
  }
}


/**
 * All function win Win32 prefix are custom functions to deal Windows API layer
 */

internal Win32WindowDimension Win32GetWindowDimension(HWND window) {
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

  int bytesPerPixel = 4; // 1 byte for padding, 3 bytes for RGB

  buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
  buffer->info.bmiHeader.biWidth = buffer->width;
  buffer->info.bmiHeader.biHeight = -buffer->height;  // negative height ensure the framebuffer uses top left corner as origin when drawing on windows
  buffer->info.bmiHeader.biPlanes = 1;
  buffer->info.bmiHeader.biBitCount = 32;
  buffer->info.bmiHeader.biCompression = BI_RGB;

  int bitmapMemorySize = (buffer->width * buffer->height) * bytesPerPixel;
  buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  buffer->pitch = buffer->width * bytesPerPixel;

  // TODO: clear all pixels in memory to black???
}

internal void Win32DisplayBufferInWindow(HDC deviceContext, int windowWidth, int windowHeight, Win32OffScreenBuffer *buffer) {
  // TODO: correct the aspect ratio for stretch
  StretchDIBits(
    deviceContext,
    // x, y, width, height,
    // x, y, width, height, 
    0, 0, windowWidth, windowHeight,
    0, 0, buffer->width, buffer->height,
    buffer->memory,
    &buffer->info,
    DIB_RGB_COLORS,
    SRCCOPY
  );
}

LRESULT CALLBACK Win32MainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;
  
  switch (message) {
    case WM_DESTROY: {
      // TODO: treat destroy event as error and recovering by recreating window
      globalRunning = false;
    } break;
  
    case WM_CLOSE: {
      // TODO: ask user to confirm before closing window
      globalRunning = false;
    } break;
  
    case WM_ACTIVATE: {
      OutputDebugStringA("WM_ACTIVATE\n");
    } break;
  
    case WM_SYSKEYDOWN: // fallthrough
    case WM_SYSKEYUP: // fallthrough
    case WM_KEYDOWN:  // fallthrough
    case WM_KEYUP: {
      uint32 vKCode = wParam;
      bool32 wasDown = ((lParam & (1 << 30)) != 0);
      bool32 isDown = ((lParam & (1 << 31)) == 0);

      if (isDown != wasDown) {  // ignore repeating key pressed messages
        if (vKCode == 'W') {
        } else if (vKCode == 'A') {
        } else if (vKCode == 'S') {
        } else if (vKCode == 'D') {
        } else if (vKCode == 'Q') {
        } else if (vKCode == 'E') {
        } else if (vKCode == VK_UP) {
        } else if (vKCode == VK_DOWN) {
        } else if (vKCode == VK_LEFT) {
        } else if (vKCode == VK_RIGHT) {
        } else if (vKCode == VK_ESCAPE) {
        } else if (vKCode == VK_SPACE) {
          OutputDebugStringA("Pressed SpaceBar:");
          if (isDown) {
            OutputDebugStringA(" :isDown");
          }
          if (wasDown) {
            OutputDebugStringA(" :wasDown");
          }
          OutputDebugStringA("\n");
        }
      }

      // support alt+f4 to close app
      bool32 altKeyWasDown = (lParam & (1 << 29));
      if (altKeyWasDown && vKCode == VK_F4) {
        globalRunning = false;
      }
    } break;
  
    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);

      Win32WindowDimension dimension = Win32GetWindowDimension(window);
      Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, &globalBackBuffer);

      EndPaint(window, &paint);
    } break;
  
    default: {
//      OutputDebugStringA("default\n");
      result = DefWindowProcA(window, message, wParam, lParam);
    } break;
  }

  return result;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prevInstance, PSTR cmdLine, int cmdShow) {
  Win32LoadXInput();

  WNDCLASSA windowClass = {};
 
  Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

  windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  windowClass.lpfnWndProc = Win32MainWindowCallback;
  windowClass.hInstance = instance;
  //windowClass.hIcon = ;
  windowClass.lpszClassName = "HandmadeEngineWindowClass";

  LARGE_INTEGER perfCountFrequencyResult;
  QueryPerformanceFrequency(&perfCountFrequencyResult);
  int64 perfCountFrequency = perfCountFrequencyResult.QuadPart;

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
      // CS_OWNDC flag in windowClass.style means it can get its own DC not shared by anyone. Get it once and use it forever. 
      HDC deviceContext = GetDC(window);
      
      // Sound test state
      Win32SoundOutput soundOutput = {};
      soundOutput.samplesPerSec = 48000;
      soundOutput.runningSampleIndex = 0;
      soundOutput.bytesPerSample = 2 * sizeof(int16);
      soundOutput.dsBufferSize = soundOutput.samplesPerSec * soundOutput.bytesPerSample;
      soundOutput.latencySampleCount = soundOutput.samplesPerSec / 15;
      Win32InitDSound(window, soundOutput.samplesPerSec, soundOutput.dsBufferSize);
      Win32ClearSoundBuffer(&soundOutput);
      // Start playing test sound
      globalDSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);

      // allocate memory for sound buffer
      int16 *samples = (int16 *)VirtualAlloc(0, soundOutput.dsBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

      // Performance CPU Cycle counter
      uint64 lastCycleCounter = __rdtsc();
      // Performance time counter
      LARGE_INTEGER lastCounter;
      QueryPerformanceCounter(&lastCounter);

#if HANDMADE_INTERNAL
      LPVOID baseAddress = (LPVOID)TeraBytes(2);
#else
      LPVOID baseAddress = 0;
#endif

      GameMemory gameMemory = {};
      gameMemory.permanentStorageSize = MegaBytes(64);
      gameMemory.transientStorageSize = GigaBytes(4);
      uint64 totalStorageSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
      gameMemory.permanentStorage = VirtualAlloc(baseAddress, totalStorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      gameMemory.transientStorage = (uint8 *)gameMemory.permanentStorage + gameMemory.permanentStorageSize;

      if (samples && gameMemory.permanentStorage && gameMemory.transientStorage) {
        GameInput input[2] = {};
        GameInput *oldInput = &input[0];
        GameInput *newInput = &input[1];

        globalRunning = true;
        while (globalRunning) {
          MSG message;
          while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) {
              globalRunning = false;
            }
            TranslateMessage(&message);
            DispatchMessageA(&message);
          }

          // TODO: more frequent polling??
          int maxControllerCount = XUSER_MAX_COUNT;
          if (maxControllerCount > ArrayCount(newInput->controllers)) {
            maxControllerCount = ArrayCount(newInput->controllers);
          }
          for (DWORD controllerIndex=0; controllerIndex< XUSER_MAX_COUNT; ++controllerIndex) {
            GameControllerInput *oldController = &oldInput->controllers[controllerIndex];
            GameControllerInput *newController = &newInput->controllers[controllerIndex];

            XINPUT_STATE controllerState;
            // ZeroMemory( &controllerState, sizeof(XINPUT_STATE) );
              
            if(XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS) {
              // Controller is connected
              // TODO: check if controllerState.dwPacketNumber is not increasing too much, should be same or +1 for each poll
              XINPUT_GAMEPAD *gamepad = &controllerState.Gamepad;

              bool32 up = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
              bool32 down = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
              bool32 left = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
              bool32 right = (gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
              bool32 start = (gamepad->wButtons & XINPUT_GAMEPAD_START);
              bool32 back = (gamepad->wButtons & XINPUT_GAMEPAD_BACK);
              bool32 leftThumb = (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
              bool32 rightThumb = (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
              bool32 leftShoulder = (gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
              bool32 rightShoulder = (gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
              bool32 aButton = (gamepad->wButtons & XINPUT_GAMEPAD_A);
              bool32 bButton = (gamepad->wButtons & XINPUT_GAMEPAD_B);
              bool32 xButton = (gamepad->wButtons & XINPUT_GAMEPAD_X);
              bool32 yButton = (gamepad->wButtons & XINPUT_GAMEPAD_Y);

              Win32ProcessXInputDigitalButton(gamepad->wButtons, XINPUT_GAMEPAD_A, &oldController->down, &newController->down);
              Win32ProcessXInputDigitalButton(gamepad->wButtons, XINPUT_GAMEPAD_B, &oldController->right, &newController->right);
              Win32ProcessXInputDigitalButton(gamepad->wButtons, XINPUT_GAMEPAD_X, &oldController->left, &newController->left);
              Win32ProcessXInputDigitalButton(gamepad->wButtons, XINPUT_GAMEPAD_Y, &oldController->up, &newController->up);
              Win32ProcessXInputDigitalButton(gamepad->wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, &oldController->leftShoulder, &newController->leftShoulder);
              Win32ProcessXInputDigitalButton(gamepad->wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, &oldController->rightShoulder, &newController->rightShoulder);

              newController->isAnalog = true;
              newController->startX = oldController->startX;
              newController->startY = oldController->startY;

              real32 lStickX;
              if (gamepad->sThumbLX < 0) {
                lStickX = (real32)gamepad->sThumbLX / 32768.0f;
              } else {
                lStickX = (real32)gamepad->sThumbLX / 32767.0f;
              }
              // TODO: min/max for sticks
              newController->minX = newController->maxX = newController->endX = lStickX;

              real32 lStickY;
              if (gamepad->sThumbLY < 0) {
                lStickY = (real32)gamepad->sThumbLY / 32768.0f;
              } else {
                lStickY = (real32)gamepad->sThumbLY / 32767.0f;
              }
              // TODO: min/max for sticks
              newController->minY = newController->maxY = newController->endY = lStickY;


              // TODO: handle triggers
              uint8 lTrigger = gamepad->bLeftTrigger;
              uint8 rTrigger = gamepad->bRightTrigger;

              // TODO: handle right stick too
              int16 rStickX = gamepad->sThumbRX;
              int16 rStickY = gamepad->sThumbRY;

              // TODO: handle deadzone properly, using X_INPUT deadzone values
            } else {
              // Controller is not connected
            }
          }

          // // Test Vibration on first controller
          // XINPUT_VIBRATION vibration;
          // vibration.wLeftMotorSpeed = 30000;  // max value ~6.5k
          // vibration.wRightMotorSpeed = 30000;
          // XInputSetState(0, &vibration);

          // Output game sound
          bool32 isSoundValid = false;
          DWORD playCursor;
          DWORD writeCursor;
          DWORD targetCursor;
          DWORD byteToLock;
          DWORD bytesToWrite;
          // TODO: improve targetCursor logic to anticipate time spent in GameUpdateAndRender call, better syncing
          if (SUCCEEDED(globalDSoundBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
            byteToLock = (soundOutput.runningSampleIndex * soundOutput.bytesPerSample) % soundOutput.dsBufferSize;
            targetCursor = (playCursor + (soundOutput.latencySampleCount * soundOutput.bytesPerSample)) % soundOutput.dsBufferSize;
            if (byteToLock > targetCursor) {
              bytesToWrite = soundOutput.dsBufferSize - byteToLock;
              bytesToWrite += targetCursor;
            } else {
              bytesToWrite = targetCursor - byteToLock;
            }

            isSoundValid = true;
          }

          GameSoundOutputBuffer soundBuffer = {};
          soundBuffer.samplesPerSecond = soundOutput.samplesPerSec;
          soundBuffer.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
          soundBuffer.samples = samples;

          // Render buffer output test
          GameOffScreenBuffer buffer = {};
          buffer.memory = globalBackBuffer.memory;
          buffer.width = globalBackBuffer.width;
          buffer.height = globalBackBuffer.height;
          buffer.pitch = globalBackBuffer.pitch;
          GameUpdateAndRender(&gameMemory, newInput, &buffer, &soundBuffer);

          // DirectSound output test
          if (isSoundValid) {
            Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
          }

          Win32WindowDimension dimension = Win32GetWindowDimension(window);
          Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height, &globalBackBuffer);

          // Timing this loop to track performance
          uint64 endCycleCounter = __rdtsc();
          LARGE_INTEGER endCounter;
          QueryPerformanceCounter(&endCounter);

          uint64 cyclesElapsed = endCycleCounter - lastCycleCounter;
          int64 counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
          real64 msPerFrame =  (1000.0f * (real64)counterElapsed) / (real64)perfCountFrequency;
          real64 fps =  (real64)perfCountFrequency / (real64)counterElapsed;
          real64 megaCyclePerFrame = (real64)cyclesElapsed / (1000.0f * 1000.0f);

          char msPrintBuffer[256];
          sprintf(msPrintBuffer, "%0.2fms/f,  %0.2ff/s,  %0.2fmc/f\n", msPerFrame, fps, megaCyclePerFrame);
          OutputDebugStringA(msPrintBuffer);

          lastCycleCounter = endCycleCounter;
          lastCounter = endCounter;

          // using ping-pong buffers to store and compare inputs between states
          GameInput *temp = newInput;
          newInput = oldInput;
          oldInput = temp;
        }
      } else {
        // NO memory allocated for game. TODO: error logging
      }
    } else {
      // TODO: error logging
    }

  } else {
    // TODO: error logging
  }

  return 0;
}