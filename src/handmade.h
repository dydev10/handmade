#ifndef HANDMADE_H
#define HANDMADE_H

/**
 * shared macros
 */

// Macro to get number of element in array
#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))


/**
 * Services provided by platform layer to the game
 */

// TODO: add platform services here


/**
 * Services provided by game to the platform layer
 */

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

internal void RenderCheckeredGradient(GameOffScreenBuffer *buffer, int xOffset, int yOffset);

// This functions needs four inputs to update on each frame: timing, user input, bitmap buffer, and sound buffer
internal void GameUpdateAndRender(GameInput *input, GameOffScreenBuffer *buffer, GameSoundOutputBuffer *soundBuffer);

#endif /* HANDMADE_H */