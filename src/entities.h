#ifndef ENTITIES_H_INCLUDED
#define ENTITIES_H_INCLUDED

#include <stdint.h>

#include <horde3d.h>

#include <vector>

#include "utils.h"

#include "scenario.h"
#include "app.h"
#include "world_geo.h"
#include "player.h"

namespace Entities
{
  using _ctype_t = Utils::fixed32m_t;
  using _vec2_t = Utils::Vec2_t<_ctype_t>;
  using _vec3_t = Utils::Vec3_t<_ctype_t>;
  
  void update();
}

class Player;

class Entity
{
protected:

  using _ctype_t = Entities::_ctype_t;
  using _vec2_t = Entities::_vec2_t;
  using _vec3_t = Entities::_vec3_t;
  
  friend class Player;

  Player* _player_ptr;
  Utils::IntrusiveNode _player_node;
  
  _vec3_t _pos;
  _vec2_t _next_pos;
  _vec2_t _target;
  PathNode* _path_node;
  H3DNode _scene_graph_node;

  void _updateTarget();
  
  void _giveVision();
  void _takeVision();
  void _updateVision();
  
  static void _pushApart(Entity*, Entity*);
  
  friend void Entities::update();

public:

  Entity(_ctype_t x, _ctype_t y, Player*);
  ~Entity();

  void update();
  void finalize();
  void updatePosition();

  void getPosition(float*, float*);
  void getPosition(float*, float*, float*);
  Utils::Vec3f getPosition();
  void getPosition(int*, int*);
  void getPosition(int*, int*, int*);

  H3DNode getSceneGraphNode();

  void setTarget(float, float);
  void issueMoveCommand(float, float);
  
  void* operator new(size_t);
  void operator delete(void*);
};

namespace Entities
{
  void init();
  void deinit();

  //void setActiveCamera(Camera*);

  void insertEntity(float, float, Player*);

  void updatePassive(float, float, float, float);
  Entity* rayPickEntity(Camera*, float, float);
  void getEntitiesInView(std::vector<Entity*>*, const Utils::Matrix4f*);

  std::vector<Entity*>::iterator getEntityStartIterator();
  std::vector<Entity*>::iterator getEntityEndIterator();

  void tempGetEntityPos(float*, float*, float*);
}

#endif // ENTITIES_H_INCLUDED
