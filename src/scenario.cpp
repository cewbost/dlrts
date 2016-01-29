
#define SCENARIO_CPP

#include <cstdio>

#include "editor.h"
#include "app.h"
#include "terrain.h"
#include "entities.h"
#include "fow.h"

#include "scenario.h"

///camera class
const float Camera::_std_fov = 45.0;

void Camera::_update()
{
  Utils::Vec3f pos = _relative + _target;
  Utils::Vec3f angle = (-_relative).toRotation();

  angle.x = Utils::radToDeg(angle.x);
  angle.y = Utils::radToDeg(angle.y);
  angle.z = Utils::radToDeg(angle.z);

  h3dSetNodeTransform(
    handle,
    pos.x, pos.y, pos.z,
    angle.x, angle.y, angle.z,
    //angle.x, angle.y, angle.z,
    1.0, 1.0, 1.0);

  const float* mat_data;
  h3dGetNodeTransMats(handle, nullptr, &mat_data);
  _inv_modelview_mat = Utils::Matrix4f(mat_data).inverted();
  Utils::Matrix4f::fastMult43(_screenspace_mat, _proj_mat,
                              _inv_modelview_mat);

  return;
}

///getters/setters
float Camera::getHeight()
{
  return _relative.y + _target.y;
}

float Camera::getAngle()
{
  return atan2(-_relative.y, -_relative.z);
}

Utils::Vec3f Camera::getRotation()
{
  Utils::Vec3f v;
  Utils::Vec3f &r = _relative;

  if( r.z != 0 ) v.x = atan2f( -r.z, sqrtf( r.x*r.x + r.y*r.y ) );
  if( r.x != 0 || r.y != 0 ) v.z = atan2f( r.x, -r.y );

  return v;
}

Utils::Vec3f Camera::getPosition()
{
  return _target + _relative;
}

const Utils::Matrix4f* Camera::getScreenSpaceTransMat()
{
  return &_screenspace_mat;
}

Utils::Vec3f Camera::getScreenSpaceVec(Utils::Vec3f v)
{
  const Utils::Matrix4f* mat = getScreenSpaceTransMat();
  Utils::Vec3f res = *mat * v;
  return res / res.z;
}

void Camera::getViewingBox(Utils::Vec2f* bl, Utils::Vec2f* br,
                           Utils::Vec2f* tl, Utils::Vec2f* tr)
{
  /*
    bit late now but I have no idea how this function works.
    had to make a tiny change and it might be fucked.
    enjoy, future me.
  */
  float near;
  float left, right, top, bottom;
  Utils::Vec3f results[4];

  near    = h3dGetNodeParamF(handle, H3DCamera::NearPlaneF, 0);
  left    = h3dGetNodeParamF(handle, H3DCamera::LeftPlaneF, 0);
  right   = h3dGetNodeParamF(handle, H3DCamera::RightPlaneF, 0);
  top     = h3dGetNodeParamF(handle, H3DCamera::TopPlaneF, 0);
  bottom  = h3dGetNodeParamF(handle, H3DCamera::BottomPlaneF, 0);

  results[0].x = -left; results[0].y = top; results[0].z = -near;
  results[1].x = -right; results[1].y = top; results[1].z = -near;
  results[2].x = -right; results[2].y = bottom; results[2].z = -near;
  results[3].x = -left; results[3].y = bottom; results[3].z = -near;

  Utils::Vec3f temp = (-_relative).toRotation();
  Utils::Matrix4f rot_mat = Utils::Matrix4f::RotMat(temp.x, temp.y, temp.z);

  results[0] = rot_mat * results[0];
  results[1] = rot_mat * results[1];
  results[2] = rot_mat * results[2];
  results[3] = rot_mat * results[3];

  near = -_relative.y;
  left = _relative.x + _target.x;
  right = _relative.z + _target.z;

  std::printf("%f, %f\n", left, right);

  results[0] = results[0] * (near / results[0].y);
  results[0].x += left; results[0].z += right;
  results[1] = results[1] * (near / results[1].y);
  results[1].x += left; results[1].z += right;
  results[2] = results[2] * (near / results[2].y);
  results[2].x += left; results[2].z += right;
  results[3] = results[3] * (near / results[3].y);
  results[3].x += left; results[3].z += right;

  tl->x = results[0].x; tl->y = results[0].z;
  tr->x = results[1].x; tr->y = results[1].z;
  br->x = results[2].x; br->y = results[2].z;
  bl->x = results[3].x; bl->y = results[3].z;
}

void Camera::move(Utils::Vec3f v)
{
  _target.x += v.x;
  _target.y += v.y;
  _target.z += v.z;

  _update();

  return;
}

void Camera::move(float x, float y, float z)
{
  _target.x += x;
  _target.y += y;
  _target.z += z;

  _update();

  return;
}

void Camera::center(Utils::Vec3f v)
{
  _target.x = v.x;
  _target.y = v.y;
  _target.z = v.z;

  _update();

  return;
}

void Camera::center(float x, float y, float z)
{
  _target.x = x;
  _target.y = y;
  _target.z = z;

  _update();

  return;
}

Camera::Camera(H3DRes pipeline)
{
  _relative.x = -4.0;
  _relative.y = 12.0;
  _relative.z = -4.0;

  _target = Utils::Vec3f(0.0, 0.0, 0.0);

  handle = h3dAddCameraNode(H3DRootNode, nullptr, pipeline);

  h3dSetNodeParamI(handle, H3DCamera::ViewportXI, 0);
  h3dSetNodeParamI(handle, H3DCamera::ViewportYI, 0);
  h3dSetNodeParamI(handle, H3DCamera::ViewportWidthI, AppCtrl::screen_w);
  h3dSetNodeParamI(handle, H3DCamera::ViewportHeightI, AppCtrl::screen_h);


  h3dSetupCameraView(handle,
                      _std_fov,
                      AppCtrl::screen_aspect,
                      0.1,
                      30.0);

  float mat[16];
  h3dGetCameraProjMat(handle, mat);
  _proj_mat = Utils::Matrix4f(mat);
}

Camera::~Camera()
{
  h3dRemoveNode(handle);

  return;
}

namespace
{
  bool _active;

  uint32_t _time_lapsed;

  //important h3d stuff
  H3DRes _water_mat_res;
  
  
  Player* _players[16];
  
  //player internals
  inline int _getNextPlayerIdx()
  {
    int idx = 0;
    for(; idx < 16; ++idx)
    {
      if(_players[idx] == nullptr)
        return idx;
    }
    return -1;
  }
  
  int _next_player;
}

namespace Scenario
{
  int map_width;
  int map_height;
  
  std::unique_ptr<Camera> camera = nullptr;

  void init()
  {
    camera = nullptr;

    _water_mat_res = h3dFindResource(H3DResTypes::Material, "water.xml");
    
    for(auto& p: _players)
      p = nullptr;
  }

  void create(uint16_t x, uint16_t y)
  {
    map_width = x;
    map_height = y;
  
    Terrain::create(x, y);
    Terrain::addWater();
    FogOfWar::create(x, y);
    
    for(auto& p: _players)
    {
      if(p != nullptr) p->resetVision();
    }
    
    //add camera
    camera.reset(new Camera(h3dFindResource(H3DResTypes::Pipeline, "general_p.xml")));
    camera->center(8.0, 0.0, 8.0);
    
    //add a light
    //FogOfWar::insertLightSource(8., 8., 6.);

    _time_lapsed = 0;
    
    FogOfWar::clear();
    
    //minor internals
    _next_player = 1;
    
    newPlayer();
    
    _active = false;
  }

  void destroy()
  {
    camera = nullptr;
    
    Terrain::clear();
    
    for(auto& p: _players)
    {
      delete p;
      p = nullptr;
    }
    
    FogOfWar::removeAllLightSources();
  }
  
  void start()
  {
    _active = true;
    
    FogOfWar::reset();
    FogOfWar::enableVision(_players[0]);
    
    Terrain::initWorldGeo();
    //WorldGeo::displayNavMesh();
    
    for(auto player: _players)
    {
      if(player != nullptr)
        player->resetVision();
    }
  }
  
  void end()
  {
    _active = false;
    
    FogOfWar::disableAllVisions();
    
    Terrain::deinitWorldGeo();
    FogOfWar::clear();
    //WorldGeo::removeNavMesh();
  }

  void proceed()
  {
    //update water animation
    ++_time_lapsed;
    h3dSetMaterialUniform(_water_mat_res, "wavePos",
                    ((float)(_time_lapsed % 400)) / 400.0, 0.0, 0.0, 0.0);

    Entities::update();
    
    if(_active)
    {
      FogOfWar::update();
    }
  }
  
  //player functions
  Player* newPlayer()
  {
    int idx = _getNextPlayerIdx();
    if(idx < 0)
      return nullptr;
    
    _players[idx] = new Player;
    _players[idx]->setName("player %i", idx);
    
    ++_next_player;
    
    return _players[idx];
  }
  Player* newPlayer(const char* name)
  {
    int idx = _getNextPlayerIdx();
    if(idx < 0)
      return nullptr;
    
    _players[idx] = new Player(name);
    
    ++_next_player;
    
    return _players[idx];
  }
  
  int getPlayerFlag(const char* name)
  {
    int idx = 0;
    for(; idx < 16; ++idx)
    {
      if(_players[idx] != nullptr && strcmp(_players[idx]->getName(), name) == 0)
        break;
    }
    
    if(idx == 16)
      return 0;
    return 1 << idx;
  }
  
  Player* getPlayer(int flag)
  {
    for(int idx = 0; idx < 16; ++idx)
    {
      if((1 << idx) & flag)
        return _players[idx];
    }
    return nullptr;
  }
  
  Player* getPlayer(const char* name)
  {
    for(int idx = 0; idx < 16; ++idx)
    {
      if(_players[idx] != nullptr && strcmp(_players[idx]->getName(), name) == 0)
        return _players[idx];
    }
    return nullptr;
  }
  
  void deletePlayers(int flags)
  {
    for(int idx = 0; idx < 16; ++idx)
    {
      if((1 << idx) & flags)
      {
        delete _players[idx];
        _players[idx] = nullptr;
      }
    }
  }
  
  void deletePlayer(const char* name)
  {
    for(int idx = 0; idx < 16; ++idx)
    {
      if(_players[idx] != nullptr && strcmp(_players[idx]->getName(), name) == 0)
      {
        delete _players[idx];
        _players[idx] = nullptr;
        return;
      }
    }
  }
  
  void moveCamera(float x, float z)
  {
    camera->_target.x += x;
    camera->_target.z += z;
    camera->_target.y = Terrain::sampleBilinear(
      camera->_target.x - 2.,
      camera->_target.z - 2.) / 2;
    camera->_update();
  }
  void centerCamera(float x, float z)
  {
    camera->_target.x = x;
    camera->_target.z = z;
    camera->_target.y = Terrain::sampleBilinear(x - 2., z - 2.) / 2;
    camera->_update();
  }
}
