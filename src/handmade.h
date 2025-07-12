#ifndef HANDMADE_H
#define HANDMADE_H

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

internal void RenderCheckeredGradient(GameOffScreenBuffer *buffer, int xOffset, int yOffset);

// This functions needs four inputs to update on each frame: timing, user input, bitmap buffer, and sound buffer
internal void GameUpdateAndRender(GameOffScreenBuffer *buffer, int xOffset, int yOffset);

#endif /* HANDMADE_H */