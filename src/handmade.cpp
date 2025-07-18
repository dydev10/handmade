#include "handmade.h"

internal void OutputGameSound(GameSoundOutputBuffer *soundBuffer, int toneHz) {
  local_persist real32 tSine = 0.0f;
  int16 toneVolume = 2000;
  int wavePeriod = soundBuffer->samplesPerSecond / toneHz;

  int16 *sampleOut = soundBuffer->samples;
  for (int sampleIndex = 0; sampleIndex < soundBuffer->sampleCount; ++sampleIndex) {
    real32 sineValue = sinf(tSine);
    int16 sampleValue = (int16)(sineValue * toneVolume);
    *sampleOut++ = sampleValue;
    *sampleOut++ = sampleValue;

    tSine += 2.0f * Pi32 * 1.0f / (real32)wavePeriod;
  }
}

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

internal void GameUpdateAndRender(GameInput *input, GameOffScreenBuffer *buffer, GameSoundOutputBuffer *soundBuffer) {
  local_persist int xOffset= 0;
  local_persist int yOffset = 0;
  local_persist int toneHz = 256;

  GameControllerInput *input0 = &input->controllers[0];
  
  if (input0->isAnalog) {
    // handle analog stick
    xOffset += (int)(4.0f * input0->endX);
    toneHz = 256 + (int)(128.0f * input0->endY);
  } else {
    // handle digital buttons
  }

  if (input0->down.endedDown) {
    yOffset += 1;
  }

  OutputGameSound(soundBuffer, toneHz);
  RenderCheckeredGradient(buffer, xOffset, yOffset);
}