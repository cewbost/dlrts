#ifndef SCENARIO_H_INCLUDED
#define SCENARIO_H_INCLUDED

#include <horde3d.h>

#include "app.h"

#include "mathutils.h"

//this is one of the ugliest hacks I have ever used
#ifdef SCENARIO_CPP
#define private public
#endif

class Camera
{
private:

  Utils::Vec3f _relative;
  Utils::Vec3f _target;
  const float* _abs_trans_mat;

  Utils::Matrix4f _inv_modelview_mat;
  Utils::Matrix4f _proj_mat;
  Utils::Matrix4f _screenspace_mat;

  void _update();

public:

  static const float _std_fov;

  H3DRes handle;

  float getHeight();
  float getAngle();
  Utils::Vec3f getPosition();
  Utils::Vec3f getRotation();

  const Utils::Matrix4f* getScreenSpaceTransMat();
  Utils::Vec3f getScreenSpaceVec(Utils::Vec3f);
  //arguments: (bottom_left, bottom_right, top_left, top_right)
  void getViewingBox(Utils::Vec2f*, Utils::Vec2f*, Utils::Vec2f*, Utils::Vec2f*);

  void move(Utils::Vec3f v);
  void move(float, float, float);
  void center(Utils::Vec3f v);
  void center(float, float, float);

  Camera(H3DRes pipeline);
  ~Camera();
};

#undef private

#include "player.h"

namespace Scenario
{
  extern int map_width;
  extern int map_height;

  void init();
  void create(uint16_t, uint16_t);
  void destroy();
  void start();
  void end();
  void proceed();
  
  Player* newPlayer();
  Player* newPlayer(const char*);
  int getPlayerFlag(const char*);
  Player* getPlayer(int);
  Player* getPlayer(const char*);
  void deletePlayers(int);
  void deletePlayer(const char*);
  
  extern std::unique_ptr<Camera> camera;
  
  void moveCamera(float, float);
  void centerCamera(float, float);
}

#endif // SCENARIO_H_INCLUDED
