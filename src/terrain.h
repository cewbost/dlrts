#pragma once

#include <stdint.h>

#include <string>
#include <bitset>

#include <horde3d.h>

#include "app.h"

namespace Terrain
{
  enum
  {
    cliff_northwest     = 0,
    cliff_southwest     = 1,
    cliff_northeast     = 2,
    cliff_southeast     = 3,

    cliff_south         = 1,
    cliff_east          = 2
  };

  /*
    bits in doodad:
    0:      left side
    1:      right side
    2:      top of slope
    4:      pointed south
    5:      pointed west
    6-7:    height level
  */

  struct Doodad
  {
    enum
    {
      slopeLeft           = 0x01,
      slopeRight          = 0x02,
      slopeCenter         = 0x03,
      slopeBottom         = 0x00,
      slopeTop            = 0x04,
      dirNorthEast        = 0x20,
      dirSouthEast        = 0x30,
      dirSouthWest        = 0x10,
      dirNorthWest        = 0x00,
      height1             = 0x40,
      height2             = 0x80,
      height3             = 0xc0,

      slopePart           = 0x0f,
      directionPart       = 0x30,
      directionSouth      = 0x10,
      directionEast       = 0x20,
      heightPart          = 0xc0
    };
  };

  constexpr size_t blocked_map_max_size = 256 * 256 * 64;
  typedef std::bitset<blocked_map_max_size> BlockedMapT;

  void init();
  void deinit();

  void create(uint16_t, uint16_t);
  void clear();
  
  const uint8_t* getHMap();

  void addWater();

  void addSlope(uint16_t, uint16_t, uint16_t);

  void modifyDistanceFieldMap(int16_t, int16_t, int16_t, int16_t, uint8_t);

  int height(double, double);
  float heightf(double, double);

  void updateDistanceFieldMap(uint16_t, uint16_t, uint16_t, uint16_t);
  void updateHeightMap(uint16_t, uint16_t, uint16_t, uint16_t);

  uint8_t getTile(uint16_t, uint16_t);
  uint8_t getDoodad(uint16_t, uint16_t);

  //getObstacle: returns 0 for obstacle and height level otherwise
  bool isObstacle(float, float);

  /*
    getLowest, getHighest
    arguments: (origin_x, origin_y, size_x, size_y)
    return: height in range [0;4]
  */
  uint8_t getLowest(uint16_t, uint16_t, uint16_t, uint16_t);
  uint8_t getHighest(uint16_t, uint16_t, uint16_t, uint16_t);
  
  double sampleBilinear(double, double);

  void copyHMapMatParams(H3DRes);

  //world geo stuff
  void initWorldGeo();
  void deinitWorldGeo();
}
