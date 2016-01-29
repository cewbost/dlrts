#pragma once

#include <stdint.h>

#include "scenario.h"
#include "entities.h"

namespace Interface
{
  void init();
  int loadFiles();
  void deinit();

  void setActiveCamera(Camera*);

  void update();

  namespace Game
  {
    void leftClick(uint16_t, uint16_t);
    void leftRelease();
    void rightClick(uint16_t, uint16_t);
    void rightRelease();
    void mouseMotion(uint16_t, uint16_t);
  }

  namespace Cursor
  {
    enum
    {
      mode_none       = 0x0000,
      mode_brush      = 0x0100,
      mode_x1         = 0x0101,
      mode_x2         = 0x0102,
      mode_x3         = 0x0103,
      mode_x4         = 0x0104,
      mode_slope      = 0x0200,
      mode_slope_nw   = 0x0201,
      mode_slope_sw   = 0x0202,
      mode_slope_ne   = 0x0203,
      mode_slope_se   = 0x0204,
      mode_entity     = 0x0300,
      mode_unit       = 0x0301,
      mode_light      = 0x0302
    };

    void setMode(int);
    void setDrawingMode(uint8_t);
    void updatePosition(uint16_t, uint16_t);
    int getMode();
  }
}
