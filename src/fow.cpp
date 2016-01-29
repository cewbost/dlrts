
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>

#include <vector>
#include <bitset>
#include <algorithm>
#include <memory>

#include "resources.h"
#include "entities.h"
#include "terrain.h"
#include "scenario.h"

#include "fow.h"

//using namespace FogOfWar;
using FogOfWar::light_map;

constexpr int TileVisionBits = 33 * 33;
constexpr int TVBSize = 400;

using TileVisionBitset = std::bitset<TileVisionBits>;

namespace
{
  //this is a helper function for debugging
#ifndef NDEBUG
  template<int W, int H>
  inline void _printBitset(const std::bitset<W * H>& bs)
  {
    putchar('\n');
    for(int j = 0; j < H; ++j)
    {
      for(int i = 0; i < W; ++i)
        putchar(bs[i + j * W]? '#' : '.');
      putchar('\n');
    }
  }
#define printBitset(w, h, bs) _printBitset<w, h>(bs);
#else
#define printBitset(...)
#endif

  struct alignas(8) TileVision
  {
    int16_t tx, ty;
    TileVisionBitset bits;
    
    bool isBlocked(int x, int y)
    {
      return bits[(x + 16) + (y + 16) * 33];
    }
    
    TileVisionBitset::reference operator()(int x, int y)
    {
      return bits[x + 16 + (y + 16) * 33];
    }
  };
  
  H3DRes _sight_map_res;
  H3DRes _general_mat_res;
  
  TileVisionBitset _vision_circles[16];
  
  struct: std::unique_ptr<TileVision*[]>
  {
    //using std::unique_ptr;
    using unique_ptr<TileVision*[]>::unique_ptr;
    using unique_ptr<TileVision*[]>::operator=;
    TileVision*& operator()(int x, int y, bool lighted = false)
    {
      return this->operator[]((x + y * Scenario::map_width) * 2 + (lighted? 1 : 0));
    }
  }_tile_vision_map = nullptr;
  
  uint16_t _block_sectors[33 * 33];
  std::bitset<TileVisionBits> _block_sectors_fill[64];
  std::bitset<TileVisionBits> _tile_vision_masks[2];
  
  TileVision _tile_vision_pool[400];
  int _tile_vision_pool_idx;
  
  /*
    _block_sectors contains bitsets for each tile within a +-16 range from the center
    (16;16) where every bit refers to a vision sector blocked if that tile from an
    observer is blocked. 16 sectors in total in clockwise order from the line x=0, y>0.
    0x0001 is the sector between lines (x=0, y>0) and (y=2x, x>0), 0x0002 is the sector
    between lines (y=2x, x>0) and (x=y, x>0), and so on, 0x8000 is sector between lines
    (y=-2x, x<0) and (x=0, y>0)
  */
  
  std::vector<Player*> _enabled_visions;

  void _makeVisionBlockers()
  {
    /*
      the following code is kept for backup purposes. might be unnecessary
    */
  
    //center
    //_vision_blockers[10 + 10 * 21].reset();

    //make quadrants
    /*for(int x = 0; x < 10; ++x)
    for(int y = 0; y < 10; ++y)
    {
      _vision_blockers[(11 + x) + (11 + y) * 21].reset();
      _vision_blockers[(9 - x) + (11 + y) * 21].reset();
      _vision_blockers[(11 + x) + (9 - y) * 21].reset();
      _vision_blockers[(9 - x) + (9 - y) * 21].reset();
      double coeff1 = (.5 + (double)x) / (.5 + (double)(y + 1)) + 1.e-6;
      double coeff2 = (.5 + (double)(x + 1)) / (.5 + (double)y) - 1.e-6;
      
      for(int x2 = x; x2 < 10; ++x2)
      for(int y2 = y; y2 < 10; ++y2)
      {
        double coeff = (double)(x2 + 1) / (double)(y2 + 1);
        if(coeff < coeff2 && coeff > coeff1)
        {
          _vision_blockers[(11 + x) + (11 + y) * 21].set((11 + x2) + (11 + y2) * 21);
          _vision_blockers[(9 - x) + (11 + y) * 21].set((9 - x2) + (11 + y2) * 21);
          _vision_blockers[(11 + x) + (9 - y) * 21].set((11 + x2) + (9 - y2) * 21);
          _vision_blockers[(9 - x) + (9 - y) * 21].set((9 - x2) + (9 - y2) * 21);
        }
      }
    }*/
    
    //make straight lines
    /*for(int i = 0; i < 10; ++i)
    {
      _vision_blockers[(11 + i) + 10 * 21].reset();
      _vision_blockers[(9 - i) + 10 * 21].reset();
      _vision_blockers[10 + (11 + i) * 21].reset();
      _vision_blockers[10 + (9 - i) * 21].reset();
      double coeff1 = .5 / (.5 + (double)i) - 1.e-6;
      
      for(int x = i; x < 10; ++x)
      for(int y = -x; y <= x; ++y)
      {
        if(y != 0)
        {
          double coeff2 = fabs((double)y / (double)(x + 1));
          if(coeff2 > coeff1)
            continue;
        }
        //_vision_blockers[(11 + i) + 10 * 21].set((11 + x) + (10 + y) * 21);
        _vision_blockers[(11 + i) + 10 * 21].set((11 + x) + (10 + y) * 21);
        _vision_blockers[(9 - i) + 10 * 21].set((9 - x) + (10 + y) * 21);
        _vision_blockers[10 + (11 + i) * 21].set((10 + y) + (11 + x) * 21);
        _vision_blockers[10 + (9 - i) * 21].set((10 + y) + (9 - x) * 21);
      }
    }*/
    
    //make _block_sectors
    //center
    _block_sectors[16 + (33 * 16)] = 0;
    //quadrants
    for(int x = 1; x <= 16; ++x)
    for(int y = 1; y <= 16; ++y)
    {
      _block_sectors[(16 + x) + (16 + y) * 33] = 0;
      _block_sectors[(16 + y) + (16 - x) * 33] = 0;
      _block_sectors[(16 - x) + (16 - y) * 33] = 0;
      _block_sectors[(16 - y) + (16 + x) * 33] = 0;
      
      int hi_n = 3 + y * 2;
      int hi_d = 1 + x * 2;
      int lo_n = 1 + y * 2;
      int lo_d = 3 + x * 2;
      
      uint16_t bits;
      if(hi_n > hi_d * 2)
        bits = 1;
      else if(hi_n > hi_d)
        bits = 2;
      else if(hi_n * 2 > hi_d)
        bits = 4;
      else bits = 8;
      
      if(lo_n >= lo_d * 2)
        bits |= 1;
      else if(lo_n >= lo_d)
        bits |= 2;
      else if(lo_n * 2 >= lo_d)
        bits |= 4;
      else bits |= 8;
      
      if(bits == 5) bits = 7;
      if(bits == 9) bits = 15;
      if(bits == 10) bits = 14;
      
      _block_sectors[(16 + x) + (16 + y) * 33] = bits;
      _block_sectors[(16 + y) + (16 - x) * 33] = bits << 4;
      _block_sectors[(16 - x) + (16 - y) * 33] = bits << 8;
      _block_sectors[(16 - y) + (16 + x) * 33] = bits << 12;
    }
    //straights
    _block_sectors[16 + 17 * 33] = 0xc003;
    _block_sectors[17 + 16 * 33] = 0x003c;
    _block_sectors[16 + 15 * 33] = 0x03c0;
    _block_sectors[15 + 16 * 33] = 0x3c00;
    for(int n = 1; n < 16; ++n)
    {
      _block_sectors[16 + (17 + n) * 33] = 0x8001;
      _block_sectors[(17 + n) + 16 * 33] = 0x0018;
      _block_sectors[16 + (15 - n) * 33] = 0x0180;
      _block_sectors[(15 - n) + 16 * 33] = 0x1800;
    }
    
    /*
      the following is post-processing to convert values in _block_sectors from
      bitmasks to _block_sectors_fill indices.
      It processes the results of the previous values stored in _block_sectors simply
      because at time of writing it was not certain which result to keep
    */
    
    for(auto& i: _block_sectors)
    {
      if(i == 0)
      {
        i = 0xffff;
        continue;
      }
      int starting_bit;
      if(i & 0x0001)
      {
        if(i & 0x8000)
        {
          if(i & 0x4000)
          {
            if(i & 0x2000)
              starting_bit = 13;
            else starting_bit = 14;
          }
          else starting_bit = 15;
        }
        else starting_bit = 0;
      }
      else for(starting_bit = 1; !(i & (1 << starting_bit)); ++starting_bit);
      
      int num_bits;
      if(i & (1 << ((starting_bit + 1) % 16)))
      {
        if(i & (1 << ((starting_bit + 2) % 16)))
        {
          if(i & (1 << ((starting_bit + 3) % 16)))
            num_bits = 4;
          else num_bits = 3;
        }
        else num_bits = 2;
      }
      else num_bits = 1;
      
      i = starting_bit + (num_bits - 1) * 16;
    }
    
    //make _block_sectors_fill
    for(int y = 0; y <= 16; ++y)
    for(int x = 0; x <= y; ++x)
    {
      if(x * 2 <= y + 1)
      {
        _block_sectors_fill[0] .set((16 + x) + (16 + y) * 33);
        _block_sectors_fill[3] .set((16 + y) + (16 + x) * 33);
        _block_sectors_fill[7] .set((16 + x) + (16 - y) * 33);
        _block_sectors_fill[4] .set((16 + y) + (16 - x) * 33);
        _block_sectors_fill[8] .set((16 - x) + (16 - y) * 33);
        _block_sectors_fill[11].set((16 - y) + (16 - x) * 33);
        _block_sectors_fill[15].set((16 - x) + (16 + y) * 33);
        _block_sectors_fill[12].set((16 - y) + (16 + x) * 33);
      }
      if(x * 2 >= y - 1)
      {
        _block_sectors_fill[1] .set((16 + x) + (16 + y) * 33);
        _block_sectors_fill[2] .set((16 + y) + (16 + x) * 33);
        _block_sectors_fill[6] .set((16 + x) + (16 - y) * 33);
        _block_sectors_fill[5] .set((16 + y) + (16 - x) * 33);
        _block_sectors_fill[9] .set((16 - x) + (16 - y) * 33);
        _block_sectors_fill[10].set((16 - y) + (16 - x) * 33);
        _block_sectors_fill[14].set((16 - x) + (16 + y) * 33);
        _block_sectors_fill[13].set((16 - y) + (16 + x) * 33);
      }
    }
    for(int n = 0; n < 16; ++n)
    {
      _block_sectors_fill[16 + n] = 
        _block_sectors_fill[n] |
        _block_sectors_fill[(n + 1) % 16];
      _block_sectors_fill[32 + n] =
        _block_sectors_fill[n] |
        _block_sectors_fill[(n + 1) % 16] |
        _block_sectors_fill[(n + 2) % 16];
      _block_sectors_fill[48 + n] = 
        _block_sectors_fill[n] |
        _block_sectors_fill[(n + 1) % 16] |
        _block_sectors_fill[(n + 2) % 16] |
        _block_sectors_fill[(n + 3) % 16];
    }
    
    //make _tile_vision_masks
    for(int n = 0; n < 33; ++n)
    for(int m = 0; m < 16; ++m)
    {
      _tile_vision_masks[0].set((17 + m) + n * 33);
      _tile_vision_masks[1].set((15 - m) + n * 33);
    }
    
    //make vision circles
    for(int n = 0; n < 16; ++n)
    {
      _vision_circles[n].set();
      int r_sq = (n + 2) * (n + 1) * 4;
      for(int x = -1 - n; x <= n + 1; ++x)
      for(int y = -1 - n; y <= n + 1; ++y)
      {
        if((x * x + y * y) * 4 <= r_sq)
          _vision_circles[n].reset((16 + x) + (16 + y) * 33);
      }
    }
  }
  
  inline void _buildTileVisionHelper(
    const std::bitset<TileVisionBits>& mask,
    std::bitset<TileVisionBits>& tvb)
  {
    //begin with vertical line through x=0
    for(int n = 1; n <= 16; ++n)
    {
      if(mask[16 + (16 + n) * 33])
      {
        tvb |= (_block_sectors_fill[_block_sectors[16 + (16 + n) * 33]] << n * 33);
        break;
      }
    }
    for(int n = 1; n <= 16; ++n)
    {
      if(mask[16 + (16 - n) * 33])
      {
        tvb |= (_block_sectors_fill[_block_sectors[16 + (16 - n) * 33]] >> n * 33);
        break;
      }
    }
    //horizontal line through y=0
    for(int n = 1; n <= 16; ++n)
    {
      if(mask[16 + n + 16 * 33])
      {
        tvb |= ((_block_sectors_fill[_block_sectors[16 + n + 16 * 33]] << n)
                & _tile_vision_masks[0]);
        break;
      }
    }
    for(int n  = 1; n <= 16; ++n)
    {
      if(mask[16 - n + 16 * 33])
      {
        tvb |= ((_block_sectors_fill[_block_sectors[16 - n + 16 * 33]] >> n)
                & _tile_vision_masks[1]);
        break;
      }
    }
    //do the halves.
    for(int n = 1; n <= 16; ++n)
    for(int m = 1; m <= 16; ++m)
    {
      if(/*!tvb[(16 + n) + (16 + m) * 33] && */mask[(16 + n) + (16 + m) * 33])
        tvb |= ((_block_sectors_fill[_block_sectors[(16 + n) + (16 + m) * 33]]
                << (n + m * 33)) & _tile_vision_masks[0]);
      if(/*!tvb[(16 - n) + (16 + m) * 33] && */mask[(16 - n) + (16 + m) * 33])
        tvb |= ((_block_sectors_fill[_block_sectors[(16 - n) + (16 + m) * 33]]
                << (m * 33 - n)) & _tile_vision_masks[1]);
      if(/*!tvb[(16 + n) + (16 - m) * 33] && */mask[(16 + n) + (16 - m) * 33])
        tvb |= ((_block_sectors_fill[_block_sectors[(16 + n) + (16 - m) * 33]]
                >> (m * 33 - n)) & _tile_vision_masks[0]);
      if(/*!tvb[(16 - n) + (16 - m) * 33] && */mask[(16 - n) + (16 - m) * 33])
        tvb |= ((_block_sectors_fill[_block_sectors[(16 - n) + (16 - m) * 33]]
                >> (n + m * 33)) & _tile_vision_masks[1]);
    }
  }
  
  void _buildTileVision(int x, int y)
  {
    //static_assert(lighted == 0 || lighted == 1,
    //  "template argument to _buildTileVision must be within range [0, 1]");
  
    int w, h;
    
    w = Scenario::map_width;
    h = Scenario::map_height;
    
    const uint8_t* data = Terrain::getHMap();
    
    int x_min = x - 16 < 0? 0 : x - 16;
    int x_max = x + 16 >= w? w - 1 : x + 16;
    int y_min = y - 16 < 0? 0 : y - 16;
    int y_max = y + 16 >= h? h - 1 : y + 16;
    
    uint8_t ch = data[x + y * w];// + lighted;
    
    std::bitset<TileVisionBits> temp1, temp2;
    
    //set initial bits
    for(int _y = y_min; _y <= y_max; ++_y)
    for(int _x = x_min; _x <= x_max; ++_x)
    {
      if(data[_x + _y * w] > ch + 1)
      {
        temp1.set((_x - x + 16) + (_y - y + 16) * 33);
        temp2.set((_x - x + 16) + (_y - y + 16) * 33);
      }
      else if(data[_x + _y * w] > ch)
      {
        temp1.set((_x - x + 16) + (_y - y + 16) * 33);
      }
    }
    
    if(temp1.none())
    {
      //mark tile as full vision
      _tile_vision_map(x, y) = (TileVision*)1;
      _tile_vision_map(x, y, true) = (TileVision*)1;
      return;
    }
    
    //retrieve __TileVision from pool and construct it
    TileVision* tv = _tile_vision_pool + _tile_vision_pool_idx;
    
    tv->bits.reset();
    _buildTileVisionHelper(temp1, tv->bits);
    
    //set _tile_vision_map element
    if(tv->tx != -1)
      _tile_vision_map(tv->tx, tv->ty) = nullptr;
    tv->tx = x;
    tv->ty = y;
    _tile_vision_map(x, y) = tv;
    ++_tile_vision_pool_idx;
    if(_tile_vision_pool_idx * sizeof(decltype(*_tile_vision_pool))
      >= sizeof(_tile_vision_pool))
      _tile_vision_pool_idx = 0;
    
    //now for lighted _tile_vision_map
    temp1.flip();
    temp1 &= tv->bits;
    tv = _tile_vision_pool + _tile_vision_pool_idx;
    //note that *tv cannot be mutated until it is
    //certain there are no other exit paths
    if(temp2.none())
    {
      if(temp1.none())
      {
        _tile_vision_map(x, y, true) = (TileVision*)1;
        return;
      }
      tv->bits.reset();
      tv->bits |= temp1;
    }
    else
    {
      tv->bits.reset();
      _buildTileVisionHelper(temp2, tv->bits);
      if(!temp1.none())
        tv->bits |= temp1;
    }
    
    //set _tile_vision_map_element
    if(tv->tx != -1)
      _tile_vision_map(tv->ty, tv->ty, true) = nullptr;
    tv->tx = x;
    tv->ty = y;
    _tile_vision_map(x, y, true) = tv;
    ++_tile_vision_pool_idx;
    if(_tile_vision_pool_idx * sizeof(decltype(*_tile_vision_pool))
      >= sizeof(_tile_vision_pool))
      _tile_vision_pool_idx = 0;
  }

  //light source stuff
  H3DRes _light_mat;
  
  struct __LightSource
  {
    //WARNING THIS CLASS BREAKS THE RULE OF FIVE,SEE BELOW
    float x, y, range;
    unsigned handle;
    H3DRes node_handle;
    
    //constructors
    __LightSource(float _x, float _y, float r, unsigned h):
      x(_x), y(_y), range(r), handle(h)
    {
      node_handle = h3dAddLightNode(H3DRootNode, "", _light_mat, "LIGHTING", nullptr);
      h3dSetNodeTransform(
        node_handle,
        x, Terrain::sampleBilinear(x, y), y,
        0., 0., 0.,
        1., 1., 1.);
      h3dSetNodeParamF(node_handle, H3DLight::FovF, 0, 180.);
      h3dSetNodeParamF(node_handle, H3DLight::RadiusF, 0, sqrt(r * r + 4. * 4.));
    }
    __LightSource(const __LightSource&) = delete;
    __LightSource(__LightSource&& other)
      : x(other.x), y(other.y), range(other.range), handle(other.handle)
    {
      node_handle = other.node_handle;
      other.node_handle = 0;
    }
    //assignment operators (Warning: only move permitted)
    __LightSource& operator=(const __LightSource&) = delete;
    __LightSource& operator=(__LightSource&& other)
    {
      x = other.x;
      y = other.y;
      range = other.range;
      handle = other.handle;
      node_handle = other.node_handle;
      other.node_handle = 0;
      return *this;
    }
    //destructor
    ~__LightSource()
    {
      if(node_handle != 0)
        h3dRemoveNode(node_handle);
    }
    
  protected:
    __LightSource(float _x, float _y, float r, unsigned h, H3DNode n):
      x(_x), y(_y), range(r), handle(h), node_handle(n){}
  };
  
  struct __LightSourceWithCopy: public __LightSource
  {
    /*
      note:
        copy constructor and assignment of this class does not copy content.
        copy should be avoided except for quick swapping.
    */
    __LightSourceWithCopy(float _x, float _y, float r, unsigned h):
      __LightSource(_x, _y, r, h){}
    __LightSourceWithCopy(const __LightSource& other):
      __LightSource(other.x, other.y, other.range, other.handle, other.node_handle){}
    __LightSourceWithCopy& operator=(const __LightSource& other)
    {
      x = other.x;
      y = other.y;
      range = other.range;
      handle = other.handle;
      node_handle = other.node_handle;
      return *this;
    }
  };
  
  std::vector<__LightSourceWithCopy> _light_sources;
  unsigned _last_light_source_handle = 0;
  
  void _updateLightMap()
  {
    light_map->clearDistances();
    for(auto& ls: _light_sources)
      light_map->insertPoint(ls.x, ls.y, ls.range);
    light_map->generateDistances();
    light_map->generateBitMap([](float f)->uint8_t
      {if(f >= 2.) return 255; else return (uint8_t)(f * 128);});
  }
}

namespace FogOfWar
{
  std::unique_ptr<FogOfWar::SDFMap> light_map;

  void init()
  {
    _sight_map_res = h3dFindResource(H3DResTypes::Texture, "_visionmap");
    _general_mat_res =
      h3dFindResource(H3DResTypes::Material, "h_map_general.xml");
    _light_mat = h3dFindResource(H3DResTypes::Material, "deferred.xml");
      
    _makeVisionBlockers();
    
    _enabled_visions.reserve(16);
  }

  void create(int w, int h)
  {
    Resources::loadEmptyTexture(_sight_map_res, w, h, 0);
    assert(_tile_vision_map == nullptr);
    _tile_vision_map.reset(new TileVision*[w * h * 2]);
    light_map.reset(new SDFMap(w, h));
    _updateLightMap();
  }
  
  void destroy()
  {
    h3dUnloadResource(_sight_map_res);
    assert(_tile_vision_map != nullptr);
    _tile_vision_map = nullptr;
    light_map = nullptr;
    _light_sources.clear();
  }
  
  void reset()
  {
    for(int n = 0; n < Scenario::map_width * Scenario::map_height * 2; ++n)
      _tile_vision_map[n] = nullptr;
    for(auto& tv: _tile_vision_pool)
    {
      tv.tx = -1;
      tv.ty = -1;
    }
    _tile_vision_pool_idx = 0;
    
    //reset light map
    //_light_sources.clear();
    _updateLightMap();
  }
  
  void enableVision(const char* name)
  {
    Player* player = Scenario::getPlayer(name);
    if(player == nullptr) return;
    if(std::find(_enabled_visions.begin(), _enabled_visions.end(), player)
      != _enabled_visions.end()) return;
    
    _enabled_visions.push_back(player);
  }
  
  void enableVision(Player* player)
  {
    if(player == nullptr) return;
    if(std::find(_enabled_visions.begin(), _enabled_visions.end(), player)
      != _enabled_visions.end()) return;
    
    _enabled_visions.push_back(player);
  }
  
  void disableVision(const char* name)
  {
    Player* player = Scenario::getPlayer(name);
    if(player == nullptr) return;
    auto it = std::find(_enabled_visions.begin(), _enabled_visions.end(), player);
    if(it != _enabled_visions.end())
      _enabled_visions.erase(it);
  }
  
  void disableVision(Player* player)
  {
    if(player == nullptr) return;
    auto it = std::find(_enabled_visions.begin(), _enabled_visions.end(), player);
    if(it != _enabled_visions.end())
      _enabled_visions.erase(it);
  }
  
  void disableAllVisions()
  {
    _enabled_visions.clear();
  }
  
  void update()
  {
    if(_enabled_visions.size() == 0)
      clear();
  
    for(auto ptr: _enabled_visions)
      ptr->updateVision();
    for(auto ptr: _enabled_visions)
      ptr->addVisionContrib(_sight_map_res);
  }
  
  void clear()
  {
    uint8_t* stream = (uint8_t*)h3dMapResStream(
      _sight_map_res,
      H3DTexRes::ImageElem, 0,
      H3DTexRes::ImgPixelStream,
      true, true);
    
    for(int n = 0; n < Scenario::map_width * Scenario::map_height * 4; ++n)
      stream[n] = 255;
    
    h3dUnmapResStream(_sight_map_res);
  }
  
  //light source functions
  unsigned insertLightSource(float x, float y, float range)
  {
    _light_sources.emplace_back(x, y, range, _last_light_source_handle);
    _updateLightMap();
    return _last_light_source_handle++;
  }
  
  void removeLightSource(unsigned handle)
  {
    std::remove_if(_light_sources.begin(), _light_sources.end(),
      [handle](__LightSource& ls){return ls.handle == handle;});
    _light_sources.pop_back();
    _updateLightMap();
  }
  
  void removeAllLightSources()
  {
    _light_sources.clear();
    _updateLightMap();
  }
  
  /*void generateLosMap()
  {
    int w, h;
    
    w = Scenario::map_width;
    h = Scenario::map_height;
    
    const uint8_t* data = Terrain::getHMap();
  
    for(int y = 0; y < h; ++y)
    for(int x = 0; x < w; ++x)
    {
      int min_x, min_y, max_x, max_y;
      min_x = x - 10 >= 0? x - 10 : 0;
      min_y = y - 10 >= 0? y - 10 : 0;
      max_x = x + 10 < w? x + 10 : w - 1;
      max_y = y + 10 < h? y + 10 : h - 1;

      assert(x + y * w < Scenario::map_width * Scenario::map_height);
      TileVision& vis = _tile_vision_map[x + y * w];
      vis.reset();
      int tile_h = data[x + y * w];
      
      for(int j = min_y; j < max_y; ++j)
      for(int i = min_x; i <= max_x; ++i)
      {
        if(data[i + j * w] > tile_h)
        {
          vis |= _vision_blockers[(10 + i - x) + (10 + j - y) * 21];
          
          //block the diagonals
          if(i - x != 0 && j - y != 0)
          {
            if(i > x)
            {
              if(j > y)
              {
                if(data[i - 1 + (j + 1) * w] > tile_h)
                  vis |= _vision_blockers[(10 + i - x) + (11 + j - y) * 21];
              }
              else if(i != max_x)//j < y
              {
                if(data[i + 1 + (j + 1) * w] > tile_h)
                  vis |= _vision_blockers[(11 + i - x) + (10 + j - y) * 21];
              }
            }
            else//i < x
            {
              if(j > y)
              {
                if(data[i + 1 + (j + 1) * w] > tile_h)
                  vis |= _vision_blockers[(10 + i - x) + (11 + j - y) * 21];
              }
              else if(i != min_x)//j < y
              {
                if(data[i - 1 + (j + 1) * w] > tile_h)
                  vis |= _vision_blockers[(9 + i - x) + (10 + j - y) * 21];
              }
            }
          }
        }
      }
      //this is so it doesn't check the diagonals on the last iteration
      for(int i = min_x; i <= max_x; ++i)
      {
        if(data[i + max_y * w] > tile_h)
          vis |= _vision_blockers[(10 + i - x) + (10 + max_y - y) * 21];
      }
    }
  }*/
  
  float sdfDistFunc(float x, float y)
  {
    return sqrt(x * x + y * y);
  }
}

//LosMap class

LosMap::LosMap(int w, int h): Utils::BitMap<uint32_t>(w, h){}

void LosMap::giveVision(int x, int y, int range, int l_range)
{
  //printf("Called takeVision(%i, %i)\n", x, y);
  int neg_x_range = x - l_range < 0? -x : -l_range;
  int neg_y_range = y - l_range < 0? -y : -l_range;
  int pos_x_range = x + l_range >= this->_width? this->_width - 1 - x : l_range;
  int pos_y_range = y + l_range >= this->_height? this->_height - 1 - y : l_range;

  //for optimization these are pass by value, this copy is not necessary,
  //though might be more efficient (dunno lol)
  TileVisionBitset stat_tv = _vision_circles[range - 1];
  TileVisionBitset l_stat_tv = _vision_circles[l_range - 1];
  
  auto& tv_ptr = _tile_vision_map(x, y);
  auto& l_tv_ptr = _tile_vision_map(x, y, true);
  
  if(tv_ptr == nullptr)
    _buildTileVision(x, y);
  
  if((void*)tv_ptr != (void*)1)
    stat_tv |= tv_ptr->bits;
  if((void*)l_tv_ptr != (void*)1)
    l_stat_tv |= l_tv_ptr->bits;
  
  for(int j = neg_y_range; j <= pos_y_range; ++j)
  for(int i = neg_x_range; i <= pos_x_range; ++i)
  {
    if(!stat_tv[(i + 16) + (j + 16) * 33])
      ++this->_map[(x + i) + (y + j) * this->_width];
    if(!l_stat_tv[(i + 16) + (j + 16) * 33])
      this->_map[(x + i) + (y + j) * this->_width] += 0x10000;
  }
}

void LosMap::takeVision(int x, int y, int range, int l_range)
{
  //printf("Called takeVision(%i, %i)\n", x, y);
  int neg_x_range = x - l_range < 0? -x : -l_range;
  int neg_y_range = y - l_range < 0? -y : -l_range;
  int pos_x_range = x + l_range >= this->_width? this->_width - 1 - x : l_range;
  int pos_y_range = y + l_range >= this->_height? this->_height - 1 - y : l_range;

  //for optimization these are pass by value, this copy is not necessary,
  //though might be more efficient (dunno lol)
  TileVisionBitset stat_tv = _vision_circles[range - 1];
  TileVisionBitset l_stat_tv = _vision_circles[l_range - 1];
  
  auto& tv_ptr = _tile_vision_map(x, y);
  auto& l_tv_ptr = _tile_vision_map(x, y, true);
  
  if(tv_ptr == nullptr)
    _buildTileVision(x, y);
  
  if((void*)tv_ptr != (void*)1)
    stat_tv |= tv_ptr->bits;
  if((void*)l_tv_ptr != (void*)1)
    l_stat_tv |= l_tv_ptr->bits;
  
  for(int j = neg_y_range; j <= pos_y_range; ++j)
  for(int i = neg_x_range; i <= pos_x_range; ++i)
  {
    if(!stat_tv[(i + 16) + (j + 16) * 33])
      --this->_map[(x + i) + (y + j) * this->_width];
    if(!l_stat_tv[(i + 16) + (j + 16) * 33])
      this->_map[(x + i) + (y + j) * this->_width] -= 0x10000;
  }
}