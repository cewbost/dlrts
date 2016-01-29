#ifndef PLAYER_H_INCLUDED
#define PLAYER_H_INCLUDED

#include <cstdint>
#include <cmath>
#include <cstring>
#include <cstdio>

#include <memory>

#include <horde3d.h>

#include "fow.h"
#include "entities.h"
#include "utils.h"

class Entity;
class LosMap;

class Player
{
private:
  
  Utils::IntrusiveList<Entity> _units;
  
  std::unique_ptr<FogOfWar::SDFMap> _vsdf_map, _vsdfl_map;
  std::unique_ptr<LosMap> _vision_map;
  std::unique_ptr<Utils::BitMap<uint16_t>> _vision_fade_map;
  
  /*FogOfWar::SDFMap *_vsdf_map, *_vsdfl_map;
  LosMap* _vision_map;
  Utils::BitMap<uint16_t> *_vision_fade_map;*/
  
  char _name[128];

public:

  Player();
  Player(const char*);
  ~Player();
  
  void takeUnit(Entity*);
  
  void resetVision();
  void updateVision();
  void addVisionContrib(H3DRes);
  
  //naming functions
  const char* getName()
  {
    return _name;
  }
  template<typename... T>
  void setName(const char* format, T... args)
  {
    std::sprintf(_name, format, args...);
  }
  
  friend class Entity;
};

#endif