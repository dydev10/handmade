#ifndef WIN32_HANDMADE_H
#define WIN32_HANDMADE_H

struct Win32OffScreenBuffer{
  BITMAPINFO info;
  void *memory;
  int width;
  int height;
  int pitch;
};

struct Win32WindowDimension {
  int width;
  int height;
};

struct Win32SoundOutput {
  int samplesPerSec;
  uint32 runningSampleIndex;
  int bytesPerSample;
  int dsBufferSize;
  int latencySampleCount;
};


#endif