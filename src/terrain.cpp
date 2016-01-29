
#include <memory>
#include <vector>
#include <stack>
#include <queue>
#include <utility>
#include <bitset>
#include <algorithm>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cassert>

#include <stdint.h>

#include <horde3d.h>

#include "utils.h"
#include "common.h"

#include "resources.h"
#include "entities.h"
#include "world_geo.h"
#include "fow.h"

#include "terrain.h"

using namespace Terrain;

using BlockedMapT = std::bitset<blocked_map_max_size>;
using NavMeshVert = std::pair<float, float>;

namespace
{
  H3DRes _heightmap_res       = 0;
  H3DRes _terrain_mat_res     = 0;
  H3DRes _terrain_top_mat_res = 0;
  H3DRes _water_mat_res       = 0;
  H3DRes _general_mat_res     = 0;
  
  Common::MeshFactory _hmaptile_fac;
  Common::MeshFactory _flattile_fac;
  Common::MeshFactory _watertile_fac;
  
  //H3DRes _tiny_cube           = 0;

  H3DNode _terrain_hr_node    = 0;
  H3DNode _terrain_lr_node    = 0;
  std::unique_ptr<H3DNode[]> _terrain_mesh_nodes;
  
  H3DNode _water_node         = 0;
  
  std::unique_ptr<uint8_t[]> _map_data = nullptr;     //height per tile(range: 0:4)
  std::unique_ptr<uint8_t[]> _doodad_map = nullptr;   //doodads
  std::unique_ptr<uint8_t[]> _distfieldmap = nullptr; //signed distance fields per tile
                                                      //per height level
  std::unique_ptr<uint8_t[]> _large_map = nullptr;    //height x8x8 per tile
  std::unique_ptr<uint8_t[]> _random_map = nullptr;   //just noise
  
  /*struct __MipmapSingleton
  {
    uint8_t* _m[4];
    uint8_t* operator[](int i)
    {
      return _m[i];
    }
    
    void reset(int w, int h)
    {
      _m[0] = _map_data.get();
      for(int i = 1; i < 4; ++i)
      {
        if(_m[i]) delete[] _m[i];
        _m[i] = new uint8_t[(w / (1 << i)) * (h / (1 << i))];
      }
    }
    void reset(nullptr_t)
    {
      _m[0] = nullptr;
      for(int i = 1; i < 4; ++i)
      {
        if(_m[i]) delete[] _m[i];
        _m[i] = nullptr;
      }
    }
    
    void update(int xsrs, int ysrs, int xdest, int ydest)
    {
      for(int i = 1; i < 4; ++i)
      {
        xsrs >>= 1;
        ysrs >>= 1;
        ++xdest; xdest >>= 1;
        ++ydest; ydest >>= 1;
        
        for(int y = ysrs; ysrs < ydest; ++y)
        for(int x = xsrs; xsrs < xdest; ++x)
        {
          
        }
      }
    }
    
    __MipmapSingleton()
    {
      for(auto& ptr: _m)
        ptr = nullptr;
    }
    ~__MipmapSingleton()
    {
      reset(nullptr);
    }
  }_mipmaps; //this object is a mipmap of _map_data*/

  BlockedMapT _blocked_map;

  std::vector<NavMeshVert>    _navmesh_verts;
  std::vector<int>            _navmesh_indices;
  std::vector<H3DNode>        _navmesh_markers;

  uint16_t _width, _length;

  ///constants
  enum: int
  {
    __grid_divisions = 8
  };

  ///helper functions
  inline uint8_t _sampleDistMap(uint16_t x, uint16_t y, uint16_t f)
  {
    int16_t real_x = (x + 4) / 8 - 1;
    int16_t real_y = (y + 4) / 8 - 1;

    int results = 0x00;
    if(real_x < 0) results |= 0x01;
    else if(real_x + 1 >= _width) results |= 0x02;
    if(real_y < 0) results |= 0x04;
    else if(real_y + 1 >= _length) results |= 0x08;

    uint8_t samples[4];

    if(!((results & 0x01) || (results & 0x04))) samples[0] =
      _distfieldmap[(real_x + real_y * _width) * 4 + f];
    else samples[0] = 0;
    if(!((results & 0x02) || (results & 0x04))) samples[1] =
      _distfieldmap[((real_x + 1) + real_y * _width) * 4 + f];
    else samples[1] = 0;
    if(!((results & 0x01) || (results & 0x08))) samples[2] =
      _distfieldmap[(real_x + (real_y + 1) * _width) * 4 + f];
    else samples[2] = 0;
    if(!((results & 0x02) || (results & 0x08))) samples[3] =
      _distfieldmap[((real_x + 1) + (real_y + 1) * _width) * 4 + f];
    else samples[3] = 0;

    float x_fract = ((float)((x + 4) % 8) + 0.5) / 8.0;
    float y_fract = ((float)((y + 4) % 8) + 0.5) / 8.0;

    float f_samples[2];

    f_samples[0] = x_fract * samples[1] + (1 - x_fract) * samples[0];
    f_samples[1] = x_fract * samples[3] + (1 - x_fract) * samples[2];

    x_fract = y_fract * f_samples[1] + (1 - y_fract) * f_samples[0] + 0.5;
    return (uint8_t)x_fract;
  }

  inline uint8_t _sampleDistMapf(float x, float y, uint16_t f)
  {
    int16_t real_x = (int)(x - .5);
    int16_t real_y = (int)(y - .5);

    int results = 0x00;
    if(real_x < 0) results |= 0x01;
    else if(real_x + 1 >= _width) results |= 0x02;
    if(real_y < 0) results |= 0x04;
    else if(real_y + 1 >= _length) results |= 0x08;

    uint8_t samples[4];

    if(!((results & 0x01) || (results & 0x04))) samples[0] =
      _distfieldmap[(real_x + real_y * _width) * 4 + f];
    else samples[0] = 0;
    if(!((results & 0x02) || (results & 0x04))) samples[1] =
      _distfieldmap[((real_x + 1) + real_y * _width) * 4 + f];
    else samples[1] = 0;
    if(!((results & 0x01) || (results & 0x08))) samples[2] =
      _distfieldmap[(real_x + (real_y + 1) * _width) * 4 + f];
    else samples[2] = 0;
    if(!((results & 0x02) || (results & 0x08))) samples[3] =
      _distfieldmap[((real_x + 1) + (real_y + 1) * _width) * 4 + f];
    else samples[3] = 0;

    float x_fract = (x - .5) - (float)real_x;
    float y_fract = (y - .5) - (float)real_y;

    float f_samples[2];

    f_samples[0] = x_fract * samples[1] + (1 - x_fract) * samples[0];
    f_samples[1] = x_fract * samples[3] + (1 - x_fract) * samples[2];

    x_fract = y_fract * f_samples[1] + (1 - y_fract) * f_samples[0] + 0.5;
    return (uint8_t)x_fract;
  }

  uint8_t _sampleDoodadMap(uint16_t x, uint16_t y)
  {
    using namespace Terrain;

    uint16_t tile_x = x / 8;
    uint16_t tile_y = y / 8;

    uint8_t doodad = _doodad_map[tile_x + tile_y * _width];

    if(doodad == 0) return 0;

    uint8_t div_x = x % 8;
    uint8_t div_y = y % 8;

    //update orientation
    uint8_t temp;

    switch(doodad & Doodad::directionPart)
    {
      case Doodad::dirNorthWest:
      break;
      case Doodad::dirSouthWest:
      temp = div_x;
      div_x = 7 - div_y;
      div_y = temp;
      break;
      case Doodad::dirSouthEast:
      div_x = 7 - div_x;
      div_y = 7 - div_y;
      break;
      case Doodad::dirNorthEast:
      temp = div_x;
      div_x = div_y;
      div_y = 7 - temp;
      break;
    }

    if(doodad & Doodad::slopeTop) div_x += 8;
    if(div_x > 11) return 0;

    if((doodad & Doodad::slopeCenter)
      == Doodad::slopeLeft && div_y > 5) return 0;
    else if((doodad & Doodad::slopeCenter)
      == Doodad::slopeRight && div_y < 2) return 0;

    uint8_t result;

    result = (div_x + 1) * 5;
    if(doodad & Doodad::height1) result += 64;
    if(doodad & Doodad::height2) result += 128;
    return result;
  }
  
  inline int _height(double x, double y)
  {
    /**
        This is a helper function
    **/
    //x -= 0.5;
    //y -= 0.5;

    uint32_t xi = (int)(x * 8);
    uint32_t yi = (int)(y * 8);

    uint8_t s, w, e, n;

    s = _large_map[xi + yi * _width * 8];
    w = _large_map[xi + 1 + yi * _width * 8];
    e = _large_map[xi + (yi + 1) * _width * 8];
    n = _large_map[xi + 1 + (yi + 1) * _width * 8];

    double xf  = x * 8; xf -= std::floor(xf);
    double yf  = y * 8; yf -= std::floor(yf);

    s = w * xf + s * (1. - xf);
    e = n * xf + e * (1. - xf);
    s = e * yf + s * (1. - yf);

    return s;
  }

  inline bool _testIfBlocked(int x, int y)
  {
    return _blocked_map[x + y * _width * 8];
  }


  inline void _cleanBlockedMap(int xsrs, int ysrs, int xdest, int ydest)
  {
    if(xsrs <= 0) ++xsrs;
    if(ysrs <= 0) ++ysrs;
    if(xdest >= _width - 1) --xdest;
    if(ydest >= _length - 1) --ydest;
  
    //smoothen
    int n;
    for(int x = xsrs; x < xdest; ++x)
    for(int y = ysrs; y < ydest; ++y)
    {
      n = 0;
      if(_blocked_map[x - 1 + y * 8 * _width]) n |= 0x1;
      if(_blocked_map[x + 1 + y * 8 * _width]) n |= 0x2;
      if(_blocked_map[x + (y - 1) * 8 * _width]) n |= 0x4;
      if(_blocked_map[x + (y + 1) * 8 * _width]) n |= 0x8;

      if(_blocked_map[x + y * 8 * _width])
      {
        if(!(n & 0x3)) _blocked_map.reset(x + y * 8 * _width);
        else if(!(n & 0xc)) _blocked_map.reset(x + y * 8 * _width);
      }
      else
      {
        if((n & 0x3) == 0x3) _blocked_map.set(x + y * 8 * _width);
        else if((n & 0xc) == 0xc) _blocked_map.set(x + y * 8 * _width);
      }
    }
  }

  inline void _doodadsToBlockedMap(int xsrs, int ysrs, int xdest, int ydest)
  {
    using namespace Terrain;

    uint16_t reset_mask;
    for(int x = xsrs; x < xdest; ++x)
    for(int y = ysrs; y < ydest; ++y)
    {
      auto samp = _doodad_map[x + y * _width];
      if(samp == 0) continue;

      reset_mask = 0x0000;

      if(samp & Doodad::slopeLeft)
        reset_mask |= 0x00ff;
      if(samp & Doodad::slopeRight)
        reset_mask |= 0xff00;
      if(samp & Doodad::slopeTop)
        reset_mask |= 0xeffe;
      else reset_mask |= 0x1111;

      //rotate mask
      switch(samp & Doodad::directionPart)
      {
      case Doodad::dirNorthWest:
        for(int _y = 0; _y < 4; ++_y)
        for(int _x = 0; _x < 4; ++_x)
        {
          if(reset_mask & (1 << (_y * 4 + _x)))
            _blocked_map.reset(x * 4 + _x + 
              (y * 4 + _y) * _width * 4);
          else _blocked_map.set(x * 4 + _x + 
              (y * 4 + _y) * _width * 4);
        }
        break;

      case Doodad::dirSouthWest:
        for(int _y = 0; _y < 4; ++_y)
        for(int _x = 0; _x < 4; ++_x)
        {
          if(reset_mask & (1 << (_x * 4 + (3 - _y))))
            _blocked_map.reset(x * 4 + _x + 
              (y * 4 + _y) * _width * 4);
          else _blocked_map.set(x * 4 + _x +  
              (y * 4 + _y) * _width * 4);
        }

        break;
      case Doodad::dirSouthEast:
        for(int _y = 0; _y < 4; ++_y)
        for(int _x = 0; _x < 4; ++_x)
        {
          if(reset_mask & (1 << ((3 - _y) * 4 + (3 - _x))))
            _blocked_map.reset(x * 4 + _x + 
              (y * 4 + _y) * _width * 4);
          else _blocked_map.set(x * 4 + _x +  
              (y * 4 + _y) * _width * 4);
        }
        break;

      case Doodad::dirNorthEast:
        for(int _y = 0; _y < 4; ++_y)
        for(int _x = 0; _x < 4; ++_x)
        {
          if(reset_mask & (1 << ((3 - _x) * 4 + _y)))
            _blocked_map.reset(x * 4 + _x + 
              (y * 4 + _y) * _width * 4);
          else _blocked_map.set(x * 4 + _x +  
              (y * 4 + _y) * _width * 4);
        }

        break;
      }
    }
  }
  
  void _updateTerrainMesh(
    uint16_t xsrs, uint16_t ysrs,
    uint16_t xdest, uint16_t ydest)
  {
    //remove previous meshes
    for(int y = ysrs; y <= ydest; ++y)
    for(int x = xsrs; x <= xdest; ++x)
    {
      if(_terrain_mesh_nodes[x + y * _width] != 0)
      {
        h3dRemoveNode(_terrain_mesh_nodes[x + y * _width]);
        _terrain_mesh_nodes[x + y * _width] = 0;
      }
    }
    
    //set border fields
    if(xsrs != 0)
    {
      for(int y = ysrs + 1; y < ydest; ++y)
      {
        int temp = _map_data[xsrs - 1 + y * _width];
        if(_map_data[xsrs + y * _width] != temp)
          _terrain_mesh_nodes[xsrs + y * _width] = 1;
        if(_map_data[xsrs + (y - 1) * _width] != temp)
          _terrain_mesh_nodes[xsrs + (y - 1) * _width] = 1;
        if(_map_data[xsrs + (y + 1) * _width] != temp)
          _terrain_mesh_nodes[xsrs + (y + 1) * _width] = 1;
      }
      if(_map_data[xsrs - 1 + ysrs * _width] != _map_data[xsrs + ysrs * _width])
        _terrain_mesh_nodes[xsrs + ysrs * _width] = 1;
      if(_map_data[xsrs - 1 + ysrs * _width] != _map_data[xsrs + (ysrs + 1) * _width])
        _terrain_mesh_nodes[xsrs + (ysrs + 1) * _width] = 1;
      if(_map_data[xsrs - 1 + ydest * _width] != _map_data[xsrs + ydest * _width])
        _terrain_mesh_nodes[xsrs + ydest * _width] = 1;
      if(_map_data[xsrs - 1 + ydest * _width] != _map_data[xsrs + (ydest - 1) * _width])
        _terrain_mesh_nodes[xsrs + (ydest - 1) * _width] = 1;
      
      //corners
      if(ysrs != 0 &&
        _map_data[xsrs - 1 + (ysrs - 1) * _width] !=
        _map_data[xsrs + ysrs * _width])
        _terrain_mesh_nodes[xsrs + ysrs * _width] = 1;
      if(ydest != _length - 1 &&
        _map_data[xsrs - 1 + (ydest + 1) * _width] !=
        _map_data[xsrs + ydest * _width])
        _terrain_mesh_nodes[xsrs + ydest * _width] = 1;
    }
    if(xdest != _width - 1)
    {
      for(int y = ysrs + 1; y < ydest; ++y)
      {
        int temp = _map_data[xdest + 1 + y * _width];
        if(_map_data[xdest + y * _width] != temp)
          _terrain_mesh_nodes[xdest + y * _width] = 1;
        if(_map_data[xdest + (y - 1) * _width] != temp)
          _terrain_mesh_nodes[xdest + (y - 1) * _width] = 1;
        if(_map_data[xdest + (y + 1) * _width] != temp)
          _terrain_mesh_nodes[xdest + (y + 1) * _width] = 1;
      }
      if(_map_data[xdest + 1 + ysrs * _width] != _map_data[xdest + ysrs * _width])
        _terrain_mesh_nodes[xdest + ysrs * _width] = 1;
      if(_map_data[xdest + 1 + ysrs * _width] != _map_data[xdest + (ysrs + 1) * _width])
        _terrain_mesh_nodes[xdest + (ysrs + 1) * _width] = 1;
      if(_map_data[xdest + 1 + ydest * _width] != _map_data[xdest + ydest * _width])
        _terrain_mesh_nodes[xdest + ydest * _width] = 1;
      if(_map_data[xdest + 1 + ydest * _width] != _map_data[xdest + (ydest - 1) * _width])
        _terrain_mesh_nodes[xdest + (ydest - 1) * _width] = 1;
      
      //corners
      if(ysrs != 0 &&
        _map_data[xdest + 1 + (ysrs - 1) * _width] !=
        _map_data[xdest + ysrs * _width])
        _terrain_mesh_nodes[xdest + ysrs * _width] = 1;
      if(ydest != _length - 1 &&
        _map_data[xdest + 1 + (ydest + 1) * _width] !=
        _map_data[xdest + ydest * _width])
        _terrain_mesh_nodes[xdest + ydest * _width] = 1;
    }
    if(ysrs != 0)
    {
      for(int x = xsrs + 1; x < xdest; ++x)
      {
        int temp = _map_data[x + (ysrs - 1) * _width];
        if(_map_data[x + ysrs * _width] != temp)
          _terrain_mesh_nodes[x + ysrs * _width] = 1;
        if(_map_data[x + 1 + ysrs * _width] != temp)
          _terrain_mesh_nodes[x + 1 + ysrs * _width] = 1;
        if(_map_data[x - 1 + ysrs * _width] != temp)
          _terrain_mesh_nodes[x - 1 + ysrs * _width] = 1;
      }
      if(_map_data[xsrs + (ysrs - 1) * _width] != _map_data[xsrs + ysrs * _width])
        _terrain_mesh_nodes[xsrs + ysrs * _width] = 1;
      if(_map_data[xsrs + (ysrs - 1) * _width] != _map_data[xsrs + 1 + ysrs * _width])
        _terrain_mesh_nodes[xsrs + 1 + ysrs * _width] = 1;
      if(_map_data[xdest + (ysrs - 1) * _width] != _map_data[xdest + ysrs * _width])
        _terrain_mesh_nodes[xdest + ysrs * _width] = 1;
      if(_map_data[xdest + (ysrs - 1) * _width] != _map_data[xdest - 1 + ysrs * _width])
        _terrain_mesh_nodes[xdest - 1 + ysrs * _width] = 1;
    }
    if(ydest != _length - 1)
    {
      for(int x = xsrs + 1; x < xdest; ++x)
      {
        int temp = _map_data[x + (ydest + 1) * _width];
        if(_map_data[x + ydest * _width] != temp)
          _terrain_mesh_nodes[x + ydest * _width] = 1;
        if(_map_data[x + 1 + ydest * _width] != temp)
          _terrain_mesh_nodes[x + 1 + ydest * _width] = 1;
        if(_map_data[x - 1 + ydest * _width] != temp)
          _terrain_mesh_nodes[x - 1 + ydest * _width] = 1;
      }
      if(_map_data[xsrs + (ydest + 1) * _width] != _map_data[xsrs + ydest * _width])
        _terrain_mesh_nodes[xsrs + ydest * _width] = 1;
      if(_map_data[xsrs + (ydest + 1) * _width] != _map_data[xsrs + 1 + ydest * _width])
        _terrain_mesh_nodes[xsrs + 1 + ydest * _width] = 1;
      if(_map_data[xdest + (ydest + 1) * _width] != _map_data[xdest + ydest * _width])
        _terrain_mesh_nodes[xdest + ydest * _width] = 1;
      if(_map_data[xdest + (ydest + 1) * _width] != _map_data[xdest - 1 + ydest * _width])
        _terrain_mesh_nodes[xdest - 1 + ydest * _width] = 1;
    }
    
    //set inner fields
    for(int y = ysrs; y < ydest; ++y)
    for(int x = xsrs; x < xdest; ++x)
    {
      if(_map_data[x + y * _width] != _map_data[x + 1 + y * _width])
      {
        _terrain_mesh_nodes[x + y * _width] = 1;
        _terrain_mesh_nodes[x + 1 + y * _width] = 1;
      }
      if(_map_data[x + (y + 1) * _width] != _map_data[x + 1 + (y + 1) * _width])
      {
        _terrain_mesh_nodes[x + (y + 1) * _width] = 1;
        _terrain_mesh_nodes[x + 1 + (y + 1) * _width] = 1;
      }
      if(_map_data[x + y * _width] != _map_data[x + (y + 1) * _width])
      {
        _terrain_mesh_nodes[x + y * _width] = 1;
        _terrain_mesh_nodes[x + (y + 1) * _width] = 1;
      }
      if(_map_data[x + 1 + y * _width] != _map_data[x + 1 + (y + 1) * _width])
      {
        _terrain_mesh_nodes[x + 1 + y * _width] = 1;
        _terrain_mesh_nodes[x + 1 + (y + 1) * _width] = 1;
      }
      if(_map_data[x + y * _width] != _map_data[x + 1 + (y + 1) * _width])
      {
        _terrain_mesh_nodes[x + y * _width] = 1;
        _terrain_mesh_nodes[x + 1 + (y + 1) * _width] = 1;
      }
      if(_map_data[x + (y + 1) * _width] != _map_data[x + 1 + y * _width])
      {
        _terrain_mesh_nodes[x + (y + 1) * _width] = 1;
        _terrain_mesh_nodes[x + 1 + y * _width] = 1;
      }
    }
    
    //add new mesh nodes
    for(int y = ysrs; y <= ydest; ++y)
    for(int x = xsrs; x <= xdest; ++x)
    {
      if(_terrain_mesh_nodes[x + y * _width] != 0 || _doodad_map[x + y * _width] != 0)
      {
        _terrain_mesh_nodes[x + y * _width] =
          _hmaptile_fac(_terrain_hr_node, "", _terrain_mat_res);
        h3dSetNodeTransform(_terrain_mesh_nodes[x + y * _width],
          (float)x, 0., (float)y,
          0., 0., 0., 1., 1., 1.);
      }
      else if(_map_data[x + y * _width] != 0)
      {
        _terrain_mesh_nodes[x + y * _width] =
          _flattile_fac(_terrain_lr_node, "", _terrain_top_mat_res);
        h3dSetNodeTransform(_terrain_mesh_nodes[x + y * _width],
          (float)x, 0., (float)y,
          0., 0., 0., 1., 1., 1.);
      }
    }
  }
  
}//end anonymous namespace

namespace Terrain
{
  void init()
  {
    _terrain_hr_node = 0;
    _terrain_lr_node = 0;
    _water_node = 0;

    _terrain_mat_res = 
      h3dFindResource(H3DResTypes::Material, "h_map.xml");
    _terrain_top_mat_res =
      h3dFindResource(H3DResTypes::Material, "h_map_top.xml");
    _water_mat_res   = 
      h3dFindResource(H3DResTypes::Material, "water.xml");
    _general_mat_res = 
      h3dFindResource(H3DResTypes::Material, "h_map_general.xml");
    //_hmaptile.node = 
    //  h3dFindResource(H3DResTypes::Geometry, "_hmaptile");
    //_flattile.node = 
    //  h3dFindResource(H3DResTypes::Geometry, "_flattile");
    _heightmap_res   = 
      h3dFindResource(H3DResTypes::Texture, "_heightmap");

    ///create procedural geometry
    //_hmaptile.vni_pair = Resources::generateGrid(_hmaptile.node, 1.0, __grid_divisions);
    //_flattile.vni_pair = Resources::generateGrid(_flattile.node, 1.0, 1);
    
    _hmaptile_fac.build
      ("_hmaptile_oh", Resources::generateGridOH, 1.0, (int)__grid_divisions);
    _flattile_fac.build
      ("_flattile_oh", Resources::generateGridOH, 1.0, 1);
    _watertile_fac.build
      ("_flattile", Resources::generateGrid, 2.0, 1);

    ///this here for testing
    //_tiny_cube = h3dAddResource(H3DResTypes::Geometry, "_tinycube", 0);
    //Resources::generateCube(_tiny_cube, 0.1);

    //create random map
    _random_map.reset(new uint8_t[1024]);
    std::srand(0); std::rand();
    for(int n = 0; n < 1024; ++n)
      _random_map[n] = std::rand() % 9;

    AppCtrl::dumpMessages();

    return;
  }

  void deinit()
  {
    clear();
    _random_map = nullptr;

    return;
  }


  void create(uint16_t w, uint16_t l)
  {
    if(_map_data != nullptr)
      clear();

    _width = w;
    _length = l;

    //create texture
    Resources::loadEmptyTexture(_heightmap_res, w * 8, l * 8,
                              (128 << 8) | (128 << 16));

    //setup map_data
    uint32_t wxl = w * l;

    _map_data.reset(new uint8_t[wxl]);
    _doodad_map.reset(new uint8_t[wxl]);
    for(uint32_t n = 0; n < wxl; ++n)
    {
      _map_data[n] = 0;
      _doodad_map[n] = 0;
    }

    _distfieldmap.reset(new uint8_t[wxl * 4]);
    for(uint32_t n = 0; n < wxl * 4; ++n)
      _distfieldmap[n] = 0;
    wxl = w * 8 * l * 8;
    _large_map.reset(new uint8_t[wxl]);
    for(uint32_t n = 0; n < wxl; ++n)
      _large_map[n] = 0;

    _blocked_map.set();

    //material setup
    h3dSetMaterialUniform(_general_mat_res, "hmap_size",
                        (float)w, (float)l, 0.0, 0.0);
    
    //reset node map
    _terrain_hr_node = h3dAddModelNode(H3DRootNode, "terrain_hr", _hmaptile_fac.res);
    _terrain_lr_node = h3dAddModelNode(H3DRootNode, "terrain_lr", _flattile_fac.res);
    _terrain_mesh_nodes.reset(new H3DNode[w * l]);
    
    for(int n = 0; n < w * l; ++n)
      _terrain_mesh_nodes[n] = 0;

    return;
  }

  void clear()
  {
    //gon write to a file
    /*char* buffer = new char[_width * _length * 64];
    for(int x = 0; x < _width * 8; ++x)
    for(int y = 0; y < _length * 8; ++y)
    {
      if(_testIfBlocked(x, y)) buffer[x + y * _width * 8] = 255;
      else buffer[x + y * _width * 8] = 0;
    }
    Resources::writeBWBitmap(_width * 8, _length * 8, buffer);
    delete[] buffer;*/
    //end file writing

    if(_terrain_hr_node)
    {
      h3dRemoveNode(_terrain_hr_node);
      h3dRemoveNode(_terrain_lr_node);
      _terrain_hr_node = 0;
      _terrain_lr_node = 0;
    }
    if(_water_node)
    {
      h3dRemoveNode(_water_node);
      _water_node = 0;
    }
    _terrain_mesh_nodes = nullptr;

    _map_data = nullptr;
    _doodad_map = nullptr;
    _distfieldmap = nullptr;
    _large_map = nullptr;

    _navmesh_verts.clear();
    _navmesh_markers.clear();

    if(_heightmap_res)
      h3dUnloadResource(_heightmap_res);

    return;
  }
  
  const uint8_t* getHMap()
  {
    return _map_data.get();
  }

  void addWater()
  {
    H3DNode mesh_node = 0;

    _water_node = h3dAddModelNode(H3DRootNode, "terrain", _watertile_fac.res);

    uint32_t wxl = ((_width + 1) / 2) * ((_length + 1) / 2);

    for(uint32_t n = 0; n < wxl; ++n)
    {
      mesh_node = _watertile_fac(_water_node, "", _water_mat_res);

      h3dSetNodeTransform
        (mesh_node,
        (float)(n / ((_width + 1) / 2)) * 2.0, 0.0,
        (float)(n % ((_width + 1) / 2)) * 2.0,
        0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    }

    h3dSetNodeTransform
      (_water_node,
      -0.5, 0.75, -0.5,
      0.0, 0.0, 0.0,
      1.0, 1.0, 1.0);

    return;
  }

  void addSlope(uint16_t x, uint16_t y, uint16_t type)
  {
    int base = getLowest(x, y, 2, 2);
    base = base > 3? 3 : (base < 1? 1 : base);
    base *= Doodad::height1;
    base |= type * Doodad::dirSouthWest;

    //set height and direction in 2x2 box
    _doodad_map[x + y * _width] &= 
                    ~(Doodad::directionPart | Doodad::heightPart);
    _doodad_map[x + y * _width] |= 
                    base;
    _doodad_map[(x + 1) + y * _width] &= 
                    ~(Doodad::directionPart | Doodad::heightPart);
    _doodad_map[(x + 1) + y * _width] |= 
                    base;
    _doodad_map[x + (y + 1) * _width] &= 
                    ~(Doodad::directionPart | Doodad::heightPart);
    _doodad_map[x + (y + 1) * _width] |= 
                    base;
    _doodad_map[(x + 1) + (y + 1) * _width] &= 
                    ~(Doodad::directionPart | Doodad::heightPart);
    _doodad_map[(x + 1) + (y + 1) * _width] |= 
                    base;

    //set slope part
    if(base & Doodad::directionSouth)
    {
      _doodad_map[x + y * _width] |= Doodad::slopeTop;
      _doodad_map[(x + 1) + (y + 1) * _width] &= ~Doodad::slopeTop;

      _doodad_map[(x + 1) + y * _width] |= Doodad::slopeLeft;
      _doodad_map[x + (y + 1) * _width] |= Doodad::slopeRight;
    }
    else
    {
      _doodad_map[(x + 1) + (y + 1) * _width] |= Doodad::slopeTop;
      _doodad_map[x + y * _width] &= ~Doodad::slopeTop;

      _doodad_map[(x + 1) + y * _width] |= Doodad::slopeRight;
      _doodad_map[x + (y + 1) * _width] |= Doodad::slopeLeft;
    }

    if(base & Doodad::directionEast)
    {
      _doodad_map[x + (y + 1) * _width] |= Doodad::slopeTop;
      _doodad_map[(x + 1) + y * _width] &= ~Doodad::slopeTop;

      _doodad_map[x + y * _width] |= Doodad::slopeLeft;
      _doodad_map[(x + 1) + (y + 1) * _width] |= Doodad::slopeRight;
    }
    else
    {
      _doodad_map[(x + 1) + y * _width] |= Doodad::slopeTop;
      _doodad_map[x + (y + 1) * _width] &= ~Doodad::slopeTop;

      _doodad_map[x + y * _width] |= Doodad::slopeRight;
      _doodad_map[(x + 1) + (y + 1) * _width] |= Doodad::slopeLeft;
    }

    updateHeightMap((x - 1) * 8, (y - 1) * 8, (x + 3) * 8, (y + 3) * 8);
  }

  void modifyDistanceFieldMap(int16_t xsrs, int16_t ysrs,
                              int16_t xsize, int16_t ysize,
                              uint8_t set)
  {
    if(xsrs <= 0 || ysrs <= 0 ||
      xsrs + xsize >= _width - 1 || ysrs + ysize >= _length - 1)
      return;
  
    int doodad = 0;
    int index = 0;
    int temp = 0;
    int comp = 0;

    for(uint16_t x = 0; x < xsize; ++x)
    for(uint16_t y = 0; y < ysize; ++y)
    {
      index = (x + xsrs) + (y + ysrs) * _width;

      if((doodad = _doodad_map[index]))
      {
        if(set != (doodad & Doodad::heightPart) / Doodad::height1
                + (doodad & Doodad::slopeTop? 1 : 0))
        {
          switch(doodad & Doodad::directionPart)
          {
            case Doodad::dirNorthWest:
            temp = 1;       comp = _width;
            break;
            case Doodad::dirSouthWest:
            temp = -_width; comp = 1;
            break;
            case Doodad::dirSouthEast:
            temp = -1;      comp = -_width;
            break;
            case Doodad::dirNorthEast:
            temp = _width;  comp = -1;
            break;
          }

          if(doodad & Doodad::slopeTop) temp = -temp;

          _doodad_map[index] = 0;
          _doodad_map[index + temp] = 0;

          _doodad_map[index + comp] &= ~Doodad::slopeLeft;
          if((_doodad_map[index + comp] & Doodad::slopeCenter)
            == 0) _doodad_map[index + comp] = 0;
          _doodad_map[index - comp] &= ~Doodad::slopeRight;
          if((_doodad_map[index - comp] & Doodad::slopeCenter)
            == 0) _doodad_map[index - comp] = 0;
          _doodad_map[index + temp + comp] &= ~Doodad::slopeLeft;
          if((_doodad_map[index + temp + comp] & Doodad::slopeCenter)
            == 0) _doodad_map[index + temp + comp] = 0;
          _doodad_map[index + temp - comp] &= ~Doodad::slopeRight;
          if((_doodad_map[index + temp - comp] & Doodad::slopeCenter)
            == 0) _doodad_map[index + temp - comp] = 0;
        }
      }

      _map_data[index] = set;
    }

    updateDistanceFieldMap(xsrs, ysrs, xsrs + xsize, ysrs + ysize);

    xsize = (xsrs + xsize) * 8 + 12;
    ysize = (ysrs + ysize) * 8 + 12;
    xsrs = xsrs * 8 - 12;
    ysrs = ysrs * 8 - 12;

    if(xsrs < 0) xsrs = 0;
    if(ysrs < 0) ysrs = 0;
    if(xsize >= _width * 8) xsize = _width * 8 - 1;
    if(ysize >= _length * 8) ysize = _length * 8 - 1;

    updateHeightMap(xsrs, ysrs, xsize, ysize);
    Entities::updatePassive(xsrs / 8.f, ysrs / 8.f,
                            xsize / 8.f, ysize / 8.f);

    return;
  }

  /**old interface functions**/

  uint8_t getTile(uint16_t x, uint16_t y)
  {
    return _map_data[x + y * _width];
  }

  uint8_t getDoodad(uint16_t x, uint16_t y)
  {
    return _doodad_map[x + y * _width];
  }

  bool isObstacle(float x, float y)
  {
    return _testIfBlocked((int)(x * 8), (int)(y * 8));
  }

  uint8_t getLowest(uint16_t x, uint16_t y, uint16_t xs, uint16_t ys)
  {
    if(x < 0 || y < 0 || x + xs >= _width || y + ys >= _length)
      return 0;
  
    uint8_t ret = 255;
    uint8_t temp;

    for(int n = 0; n < xs; ++n)
    for(int m = 0; m < ys; ++m)
    {
        temp = _map_data[x + n + (y + m) * _width];
        ret = temp < ret? temp : ret;
    }

    return ret;
  }

  uint8_t getHighest(uint16_t x, uint16_t y, uint16_t xs, uint16_t ys)
  {
    uint8_t ret = 0;
    uint8_t temp;

    for(int n = 0; n < xs; ++n)
    for(int m = 0; m < ys; ++m)
    {
        temp = _map_data[x + n + (y + m) * _width];
        ret = temp > ret? temp : ret;
    }

    return ret;
  }

  int height(double x, double y)
  {
    return _height(x, y);
  }

  float heightf(double x, double y)
  {
    return (float)_height(x, y) / 51.;
  }
  
  double sampleBilinear(double x, double y)
  {
    //holy shit this has like a shitty mipmapping
    constexpr int mip_l = 2;
    
    x /= mip_l;
    y /= mip_l;
    
    int ix = (int)x;
    int iy = (int)y;
    
    double sx = x - (double)ix;
    double sy = y - (double)iy;
    
    ix *= mip_l;
    iy *= mip_l;
    
    double samples[4];
    samples[0] = (double)_map_data[ix + iy * _width];
    samples[1] = (double)_map_data[ix + mip_l + iy * _width];
    samples[2] = (double)_map_data[ix + (iy + mip_l) * _width];
    samples[3] = (double)_map_data[ix + mip_l + (iy + mip_l) * _width];
    
    samples[0] = samples[1] * sx + samples[0] * (1. - sx);
    samples[2] = samples[3] * sx + samples[2] * (1. - sx);
    
    return samples[2] * sy + samples[0] * (1. - sy);
  }

#define __MAX(x, y) (x > y? x : y)
#define __MIN(x, y) (x < y? x : y)

  void updateDistanceFieldMap(uint16_t xsrs, uint16_t ysrs,
                              uint16_t xdest, uint16_t ydest)
  {
    xsrs = __MAX(xsrs - 1, 0);
    ysrs = __MAX(ysrs - 1, 0);
    xdest = __MIN(xdest + 1, _width);
    ydest = __MIN(ydest + 1, _length);

    uint8_t* dp;

    for(int x = xsrs; x < xdest; ++x)
    for(int y = ysrs; y < ydest; ++y)
    for(int h = 0; h < 4; ++h)
    {
      dp = _distfieldmap.get() + (x + y * _width) * 4 + h;

      //find nearby borders
      bool horiz = false;
      bool vertic = false;

      if(_map_data[x + y * _width] > h)
      {
        *dp = 255;
        continue;
      }

      if(x != 0 && _map_data[(x - 1) + y * _width] > h)
        horiz = true;
      else if(x != _width - 1 && _map_data[(x + 1) + y * _width] > h)
        horiz = true;
      if(y !=  0 && _map_data[x + (y - 1) * _width] > h)
        vertic = true;
      else if(y != _length && _map_data[x + (y + 1) * _width] > h)
        vertic = true;

      *dp = 0;
      if(horiz && vertic)
        *dp += 180;
      else if(horiz || vertic)
        *dp += 127;
      else if((x != 0 && y != 0 &&
            (_map_data[(x - 1) + (y - 1) * _width] > h)) ||
            (x != (_width - 1) && y != 0 &&
            (_map_data[(x + 1) + (y - 1) * _width] > h)) ||
            (y != 0 && y != (_length - 1) &&
            (_map_data[(x - 1) + (y + 1) * _width] > h)) ||
            (y != (_length - 1) && y != (_length - 1) &&
            (_map_data[(x + 1) + (y + 1) * _width] > h)))
        *dp += 90;
    }
  }

  void updateHeightMap(uint16_t xsrs, uint16_t ysrs,
                      uint16_t xdest, uint16_t ydest)
  {
    uint8_t* h_map = (uint8_t*)h3dMapResStream(_heightmap_res,
        H3DTexRes::ImageElem, 0, H3DTexRes::ImgPixelStream, true, true);
    int32_t samp;
    uint32_t height;

    //height map
    for(int x = xsrs; x < xdest; ++x)
    for(int y = ysrs; y < ydest; ++y)
    {
      //distance map
      height = 0;

      for(int n = 0; n < 4; ++n)
      {
        samp = _sampleDistMap(x, y, n) - 200;
        //add noise
        samp += _random_map[((x / 2) % 32)
                            + ((y / 2) % 32) * 32];
        samp += _random_map[(((x + 1) / 2) % 32)
                            + ((y / 2) % 32) * 32];
        samp += _random_map[((x / 2) % 32)
                            + (((y + 1) / 2) % 32) * 32];
        samp += _random_map[(((x + 1) / 2) % 32)
                            + (((y + 1) / 2) % 32) * 32];

        samp = samp < 0? 0 : (samp > 16? 16 : samp);
        height += samp * 4;
      }
      if(height > 255) height = 255;

      /*if(height & 63)
          _blocked_map.set(x + y * 8 * _width);
      else _blocked_map.reset(x + y * 8 * _width);*/

      //doodad map
      samp = _sampleDoodadMap(x, y);
      if(samp != 0) height = samp;

      samp = x + y * 8 * _width;
      h_map[samp * 4] = height;
      _large_map[samp] = height;
    }

    //update _blocked_map
    for(int x = xsrs; x < xdest; ++x)
    for(int y = ysrs; y < ydest; ++y)
    {
      int samp;

      _blocked_map.set(x + y * 8 * _width);

      samp = _large_map[x + y * 8 * _width];

      if(samp == 0) continue;
      if(std::abs(samp - _large_map[(x + 1) + (y - 1) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x + 1) + (y    ) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x + 1) + (y + 1) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x    ) + (y + 1) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x - 1) + (y + 1) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x - 1) + (y    ) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x - 1) + (y - 1) * 8 * _width])
        > 6) continue;
      if(std::abs(samp - _large_map[(x    ) + (y - 1) * 8 * _width])
        > 6) continue;

      _blocked_map.reset(x + y * 8 * _width);
    }

    _cleanBlockedMap(xsrs, ysrs, xdest, ydest);

    //normals
    ++xsrs; ++ysrs; --xdest; --ydest;

    uint8_t h_points[4];

    for(int x = xsrs; x < xdest; ++x)
    for(int y = ysrs; y < ydest; ++y)
    {
      h_points[0] = h_map[((x - 1) + y * 8 * _width) * 4];
      h_points[1] = h_map[((x + 1) + y * 8 * _width) * 4];
      h_points[2] = h_map[(x + (y - 1) * 8 * _width) * 4];
      h_points[3] = h_map[(x + (y + 1) * 8 * _width) * 4];

      h_map[(x + y * 8 * _width) * 4 + 2]
          = (uint8_t)(128l + (h_points[0] - h_points[1]) / 2);
      h_map[(x + y * 8 * _width) * 4 + 1]
          = (uint8_t)(128l + (h_points[2] - h_points[3]) / 2);
    }

    h3dUnmapResStream(_heightmap_res);
    
    _updateTerrainMesh(xsrs / 8, ysrs / 8, xdest / 8, ydest / 8);
  }

  //pathfinding stuff
  void initWorldGeo()
  {
    //write _blocked_map to file
    /*char* buffer = new char[_width * _length * 16];
    for(int n = 0; n < _width * _length * 16; ++n)
    {
      if(_blocked_map[n]) buffer[n] = 0;
      else buffer[n] = 255;
    }
    Resources::writeBWBitmap(_width * 4, _length * 4, buffer);
    delete[] buffer;*/

    WorldGeo::setupNavMesh(_width * 8, _length * 8, &_blocked_map);
  }

  void deinitWorldGeo()
  {
    WorldGeo::deleteNavMesh();
  }

#undef __MAX
#undef __MIN
}
