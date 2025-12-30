#ifndef HANDMADE_H
#define HANDMADE_H

/**
 * shared macros
 */

/**
 * HANDMADE_INTERNAL:
 *  0: running public build
 *  1: running developer build
 * 
 * HANDMADE_SLOW:
 *  0: running in optimized mode. no slow code paths, no debug/assert stuff
 *  1: running in slow mode. debugs and assertion run
 */

// Assertion Macro
#if HANDMADE_SLOW
#define Assert(expr) if (!(expr)) {*(int *)0 = 0;}
#else
#define Assert(expr)
#endif

// Macro to get number of element in array
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

// Macro to get KB in bytes
#define KiloBytes(value) ((value) * 1024LL)
// Macro to get MB in bytes
#define MegaBytes(value) (KiloBytes(value) * 1024LL)
// Macro to get GB in bytes
#define GigaBytes(value) (MegaBytes(value) * 1024LL)
// Macro to get TB in bytes
#define TeraBytes(value) (GigaBytes(value) * 1024LL)


// Helper inline function to safely convert 64 bit to 32 bit after asserting in dev
inline uint32 SafeTruncateUInt64(uint64 value) {
  // TODO: 0xFFFFFFFF is the max 32 value, use defines to refer to max values like these
  Assert(value <= 0xFFFFFFFF);
  uint32 result = (uint32)value;

  return result;
}

/**
 * Services provided by platform layer to the game
 */

#if HANDMADE_INTERNAL
// NOTE: These DEBUG file services are only for debugging. Dont use in release build because they are blocking and dont write safely. Could corrupt file 
struct DEBUG_ReadFileResult {
    uint32 contentSize;
    void *content;
  };
  internal DEBUG_ReadFileResult DEBUG_PlatformReadEntireFile(char *filename);
  internal void DEBUG_PlatformFreeFileMemory(void *memory);
  internal bool32 DEBUG_PlatformWriteEntireFile(char *filename, uint32 memorySize, void *memory);
#endif

// TODO: add more platform services here


/**
 * Services provided by game to the platform layer
 */

// TODO: add game services here

struct GameOffScreenBuffer {
  void *memory;  
  int width;
  int height;
  int pitch;
};

struct GameSoundOutputBuffer {
  int samplesPerSecond;
  int sampleCount;
  int16 *samples;
};

struct GameButtonState {
  int halfTransitionCount;
  bool endedDown;
};

struct GameControllerInput {
  bool32 isAnalog;

  real32 startX;
  real32 startY;

  real32 minX;
  real32 minY;

  real32 maxX;
  real32 maxY;

  real32 endX;
  real32 endY;

  union {
    GameButtonState buttons[6];
    struct {
      GameButtonState up;
      GameButtonState down;
      GameButtonState left;
      GameButtonState right;
      GameButtonState leftShoulder;
      GameButtonState rightShoulder;
    };
  };
};

struct GameInput {
  GameControllerInput controllers[4];
};

struct GameMemory {
  uint64 permanentStorageSize;
  void *permanentStorage;

  uint64 transientStorageSize;
  void *transientStorage;

  bool isInitialized;
};

struct GameState {
  int toneHz;
  int xOffset;
  int yOffset;
};

internal void RenderCheckeredGradient(GameOffScreenBuffer *buffer, int xOffset, int yOffset);

// This functions needs four inputs to update on each frame: timing, user input, bitmap buffer, and sound buffer
internal void GameUpdateAndRender(GameMemory *gameMemory, GameInput *input, GameOffScreenBuffer *buffer, GameSoundOutputBuffer *soundBuffer);

#endif /* HANDMADE_H */