#include <windows.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {  
  MessageBoxA(0, "Testing handmade messageBox.", "Testing handmade caption", MB_OK|MB_ICONINFORMATION);

  return 0;
}