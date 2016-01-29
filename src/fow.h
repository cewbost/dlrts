#ifndef FOW_H_INCLUDED
#define FOW_H_INCLUDED

#include <cstdint>

#include <memory>

#include <horde3d.h>

#include "player.h"

#include "utils.h"

class Player;

class LosMap: public Utils::BitMap<uint32_t>
{
public:

  //Utils::BitMap<uint16_t> dvm, lvm;
  
  LosMap(int, int);

  void giveVision(int, int, int, int);
  void takeVision(int, int, int, int);
};

namespace FogOfWar
{
  void init();
  void create(int, int);
  void destroy();
  void reset();
  void enableVision(const char*);
  void enableVision(Player*);
  void disableVision(const char*);
  void disableVision(Player*);
  void disableAllVisions();
  void update();
  void clear();
  
  //light sources are referenced using unsigned as handles
  unsigned insertLightSource(float, float, float);
  void removeLightSource(int);
  void removeAllLightSources();
  
  float sdfDistFunc(float, float);
  
  typedef Utils::SDFMap<uint8_t, float, FogOfWar::sdfDistFunc> SDFMap;
  
  extern std::unique_ptr<FogOfWar::SDFMap> light_map;
}

#endif