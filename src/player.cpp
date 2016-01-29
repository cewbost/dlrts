#include "scenario.h"

#include "player.h"

namespace
{
  
}

//public methods

//constructors/destructor
Player::Player(): _units(&Entity::_player_node)
{
  _vision_map = nullptr;
  _vision_fade_map = nullptr;
  _vsdf_map = nullptr;
  
  std::strcpy(_name, "");
  
  this->resetVision();
}

Player::Player(const char* name): _units(&Entity::_player_node)
{
  _vision_map = nullptr;
  _vision_fade_map = nullptr;
  _vsdf_map = nullptr;
  
  std::strcpy(_name, name);
  
  this->resetVision();
}

Player::~Player()
{
  _vision_map = nullptr;
  _vision_fade_map = nullptr;
  _vsdf_map = nullptr;
  _vsdfl_map = nullptr;
}

void Player::takeUnit(Entity* unit)
{
  _units.push_back(*unit);
}

void Player::resetVision()
{
  if(_vision_map != nullptr) _vision_map = nullptr;
  if(_vision_fade_map != nullptr) _vision_fade_map = nullptr;
  if(_vsdf_map != nullptr) _vsdf_map = nullptr;

  _vision_map.reset(new LosMap(Scenario::map_width, Scenario::map_height));
  _vision_map->clear(0);
  _vision_fade_map.reset(new Utils::BitMap<uint16_t>
    (Scenario::map_width, Scenario::map_height));
  _vision_fade_map->clear(75 | (75 << 8));
  _vsdf_map.reset(new FogOfWar::SDFMap(Scenario::map_width, Scenario::map_height));
  _vsdfl_map.reset(new FogOfWar::SDFMap(Scenario::map_width, Scenario::map_height));
  
  for(auto unit: _units)
    unit->_giveVision();
}

void Player::updateVision()
{
  if(_vsdf_map == nullptr || this->_vision_map == nullptr) return;
  _vsdf_map->clearDistances();
  _vsdfl_map->clearDistances();
  for(auto unit: _units)
  {
    auto x = unit->_pos.x - .5;
    auto z = unit->_pos.z - .5;
    _vsdf_map->insertPoint(x, z, 2.75);
    _vsdfl_map->insertPoint(x, z, 6.75);
  }
  _vsdf_map->generateDistances();
  _vsdfl_map->generateDistances();
  _vsdf_map->generateBitMap([](float f)->uint8_t
    {if(f >= 2.) return 255; else return (uint8_t)(f * 128);});
  _vsdfl_map->generateBitMap([](float f)->uint8_t
    {if(f >= 2.) return 255; else return (uint8_t)(f * 128);});
  _vsdfl_map->apply(*FogOfWar::light_map,
    [](uint8_t a, uint8_t b){return a < b? a : b;});
}

void Player::addVisionContrib(H3DRes res)
{
  /*
    regular vision sdf-map stored in first color-channel
    luminated vision sdf-map stored in second color-channel
  */

  uint8_t* stream = (uint8_t*)h3dMapResStream(res, H3DTexRes::ImageElem, 0,
                    H3DTexRes::ImgPixelStream, true, true);
  uint8_t* stream_cpy = stream;
  
  _vsdf_map->for_each([&stream_cpy](uint8_t c){*stream_cpy = c; stream_cpy += 4;});
  stream_cpy = stream;
  _vsdfl_map->for_each([&stream_cpy](uint8_t c){stream_cpy[1] = c; stream_cpy += 4;});
  
  //* turn of line-of-sight blocking here
  //this loop adjusts the fade map
  //the fade map implements a gradual fading of the line-of-sight map
  uint16_t* fade_data = _vision_fade_map->getData();
  _vision_map->for_each([&fade_data](uint32_t c)
  {
    if((c & 0x0000ffff) == 0)
    {
      if((*fade_data & 0x00ff) > 75) *fade_data -= 5;
    }
    else if((*fade_data & 0x00ff) < 200)
      *fade_data += 5;
    
    if((c & 0xffff0000) == 0)
    {
      if((*fade_data >> 8) > 75) *fade_data -= 0x0500;
    }
    else if((*fade_data >> 8) < 200)
      *fade_data += 0x0500;
    
    ++fade_data;
  });
  //this loop implements the intersection of the signed-distance-field map
  //and the fade-map
  _vision_fade_map->for_each([&stream](uint16_t c)
  {
    if((c & 0x00ff) < *stream) *stream = (uint8_t)(c & 0x00ff);
    if((c >> 8) < stream[1]) stream[1] = (uint8_t)(c >> 8);
    //stream[0] = (uint8_t)(c & 0x00ff);
    //stream[1] = (uint8_t)(c >> 8);
    stream += 4;
  });
  /**/
  
  h3dUnmapResStream(res);
}