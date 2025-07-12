#include "handmade.h"

internal void RenderCheckeredGradient(GameOffScreenBuffer *buffer, int xOffset, int yOffset) {
  uint8 *row = (uint8 *)buffer->memory;
  for (int y = 0; y < buffer->height; ++y) {
    uint32 *pixel = (uint32 *)row;
    for (int x = 0; x < buffer->width; ++x) {
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

    row += buffer->pitch;
  }
}

internal void GameUpdateAndRender(GameOffScreenBuffer *buffer, int xOffset, int yOffset) {
  RenderCheckeredGradient(buffer, xOffset, yOffset);
}