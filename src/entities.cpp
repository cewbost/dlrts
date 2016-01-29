
#include <cstdio>
#include <csignal>

#include <vector>

#include <horde3d.h>

#include "resources.h"
#include "terrain.h"
#include "world_geo.h"

#include "entities.h"

//using Utils::fixed32c_t;
//using Utils::Vec2f;

using Entities::_ctype_t;
using Entities::_vec2_t;
using Entities::_vec3_t;

///private namespace

namespace
{
  //resources
  H3DRes _cube_geo;
  H3DRes _cube_mat;

  Utils::ArenaAllocator<sizeof(Entity), 200> _entity_allocator;

  std::vector<Entity*> _entities;
  uint32_t _entity_count = 0;         //the entity count must always be updated
  
  inline void _removenullptrEntities()
  {
    std::vector<Entity*>::iterator stepper;
    std::vector<Entity*>::iterator it;

    for(stepper = it = _entities.begin(); it != _entities.end(); ++it)
    {
      if(*it != nullptr)
      {
        *stepper = *it;
        ++stepper;
      }
    }

    _entities.resize(_entity_count);
  }

  inline void _removeEntity(Entity** entity)
  {
    delete *entity;
    *entity = nullptr;
    --_entity_count;
  }
}

///Entity class

//protected functions
void Entity::_updateTarget()
{
  constexpr _ctype_t margin(0.1);

  if(_path_node == nullptr)
  {
    _target.x = _pos.x;
    _target.y = _pos.z;
    return;
  }
  _target.x = _ctype_t(_path_node->x);
  _target.y = _ctype_t(_path_node->y);

  if((_pos.x - _target.x).abs() < margin && (_pos.z - _target.y).abs() < margin)
    _path_node = _path_node->advance();
}

void Entity::_pushApart(Entity* first, Entity* second)
{
  if((float)(second->_next_pos - first->_next_pos).magnitudeSquared() > .25)
    return;

  _vec2_t vec = (second->_next_pos - first->_next_pos).normalize() / 4;
  _vec2_t center = (second->_next_pos + first->_next_pos) / 2;
  second->_next_pos = center + vec;
  first->_next_pos = center - vec;
}

//these constexprs should be replaced with entity specific members
constexpr int __s_range = 2;
constexpr int __l_range = 6;

void Entity::_giveVision()
{
  _player_ptr->_vision_map->giveVision
    ((int)_pos.x, (int)_pos.z, __s_range, __l_range);
}

void Entity::_takeVision()
{
  _player_ptr->_vision_map->takeVision
    ((int)_pos.x, (int)_pos.z, __s_range, __l_range);
}

void Entity::_updateVision()
{
  _player_ptr->_vision_map->takeVision
    ((int)_pos.x, (int)_pos.z, __s_range, __l_range);
  _player_ptr->_vision_map->giveVision
    ((int)_next_pos.x, (int)_next_pos.y, __s_range, __l_range);
}

//public functions
Entity::Entity(_ctype_t x, _ctype_t y, Player* player):
_pos(x, _ctype_t(0), y), _target(x, y), _path_node(nullptr)
{
  _player_ptr = player;
  player->takeUnit(this);
  //player->_vision_map->giveVision((int)x, (int)y, 6);

  _scene_graph_node = h3dAddModelNode(H3DRootNode, "", _cube_geo);
  H3DNode mesh = h3dAddMeshNode(_scene_graph_node, "", _cube_mat, 0, 36, 0, 23);
  h3dSetNodeTransform(
    mesh,
    0.0, .5, 0.0,
    0.0, 0.0, 0.0,
    1.0, 1.0, 1.0);
  
  updatePosition();
}

Entity::~Entity()
{
  //_player_ptr->_vision_map->takeVision((int)_pos.x, (int)_pos.z, 6);
  h3dRemoveNode(_scene_graph_node);
}

void Entity::update()
{
  _updateTarget();
  Utils::Vec2_t<_ctype_t> direction =
  _target - Utils::Vec2_t<_ctype_t>(_pos.x, _pos.z);
  direction = direction.normalize() / 20;
  //                                  ^
  //that is a constant which controls movement speed(horrible magic numbers)
  _next_pos.x = _pos.x + direction.x;
  _next_pos.y = _pos.z + direction.y;

  //updatePosition();
}

void Entity::finalize()
{
  float x = (float)_next_pos.x;
  float y = (float)_next_pos.y;
  if(WorldGeo::isBlocked(x, y))
  {
    WorldGeo::pushIn(x, y, (float)_pos.x, (float)_pos.z);
    _next_pos.x = x;
    _next_pos.y = y;
    
    while(WorldGeo::isBlocked((float)_next_pos.x, (float)_next_pos.y))
    {
      _next_pos.x = _pos.x + (_next_pos.x - _pos.x) / 2;
      _next_pos.y = _pos.z + (_next_pos.y - _pos.z) / 2;
    }
  }
  
  //update vision if entity changes tiles
  if((int)_next_pos.x != (int)_pos.x ||
    (int)_next_pos.y != (int)_pos.z)
    _updateVision();
  
  _pos.x = _next_pos.x;
  _pos.z = _next_pos.y;
  
  updatePosition();
}

void Entity::updatePosition()
{
  _pos.y = (_ctype_t)Terrain::heightf((double)_pos.x, (double)_pos.z);
  h3dSetNodeTransform(_scene_graph_node,
                      (float)_pos.x, (float)_pos.y, (float)_pos.z,
                      0.0, 0.0, 0.0,
                      1., 1., 1.);
}

//getters
void Entity::getPosition(float* x, float* y)
{
  *x = (float)_pos.x;
  *y = (float)_pos.z;
}

void Entity::getPosition(float* x, float* y, float * z)
{
  *x = (float)_pos.x;
  *y = (float)_pos.y;
  *z = (float)_pos.z;
}

Utils::Vec3f Entity::getPosition()
{
  return Utils::Vec3f((float)_pos.x, (float)_pos.y, (float)_pos.z);
}

void Entity::getPosition(int* x, int* y)
{
  *x = (int)_pos.x;
  *y = (int)_pos.z;
}

void Entity::getPosition(int* x, int* y, int* z)
{
  *x = (int)_pos.x;
  *y = (int)_pos.y;
  *z = (int)_pos.z;
}


H3DNode Entity::getSceneGraphNode()
{
  return _scene_graph_node;
}

//setters
void Entity::setTarget(float x, float y)
{
  _target.x = _ctype_t(x);
  _target.y = _ctype_t(y);
}

//command functions
void Entity::issueMoveCommand(float x, float y)
{
  delete _path_node;
  _path_node = WorldGeo::findPath((float)_pos.x, (float)_pos.z, x, y);
}

//memory allocation overloads
void* Entity::operator new(size_t)
{
  return _entity_allocator.getAdress();
}

void Entity::operator delete(void* p)
{
  _entity_allocator.freeAdress(p);
}

///Interface

namespace Entities
{
  void init()
  {
    //generate procedural geometry
    _cube_geo = h3dFindResource(H3DResTypes::Geometry, "_cube");
    Resources::generateCube(_cube_geo, 0.2);

    //find materials
    _cube_mat = h3dFindResource(H3DResTypes::Material, "model.xml");
  }

  void deinit()
  {
    for(std::vector<Entity*>::iterator it = _entities.begin();
      it != _entities.end(); ++it)
    {
      delete *it;
    }
    _entities.clear();

    _entity_allocator.deallocate();
  }

  void update()
  {
    for(auto ptr: _entities)
      ptr->update();
    
    for(auto it1 = _entities.begin(); it1 != _entities.end(); ++it1)
    {
      for(auto it2 = it1 + 1; it2 != _entities.end(); ++it2)
      {
        Entity::_pushApart(*it1, *it2);
      }
    }
    
    for(auto ptr: _entities)
      ptr->finalize();
  }

  void insertEntity(float x, float y, Player* player)
  {
    Entity* entity;

    entity = new Entity(_ctype_t(x), _ctype_t(y), player);
    _entities.push_back(entity);

    ++_entity_count;
  }

  void updatePassive(float xsrs, float ysrs, float xdest, float ydest)
  {
    //this function is for updating positions when game is not running
    float x, y;

    for(std::vector<Entity*>::iterator it = _entities.begin();
      it != _entities.end(); ++it)
    {
      (*it)->getPosition(&x, &y);
      if(Terrain::isObstacle(x, y))
      {
        _removeEntity(&(*it));
      }
      else (*it)->updatePosition();
    }

    _removenullptrEntities();
  }

  Entity* rayPickEntity(Camera* cam, float x, float y)
  {
    Utils::Vec3f origin, cam_direction, unit_direction;
    float dot;

    Utils::pickRayNormalized(cam->handle, x, y,
    &origin.x, &origin.y, &origin.z, &cam_direction.x,
    &cam_direction.y, &cam_direction.z);

    for(std::vector<Entity*>::iterator it = _entities.begin();
      it != _entities.end(); ++it)
    {
      unit_direction = ((*it)->getPosition() - origin);
      unit_direction.normalize();
      dot = unit_direction.dot(cam_direction);
      if(dot > 0.9997) return *it;
    }
    return nullptr;
  }

  void getEntitiesInView(std::vector<Entity*>* container,
                          const Utils::Matrix4f* mat)
  {
    for(std::vector<Entity*>::iterator it = _entities.begin();
      it != _entities.end(); ++it)
    {
      Utils::Vec3f vec = *mat * (*it)->getPosition();
      vec /= vec.z;
      if(vec.x > -1. && vec.x < 1. && vec.y > -1. && vec.y < 1.)
        container->push_back(*it);
    }
  }

  std::vector<Entity*>::iterator getEntityStartIterator()
  {return _entities.begin();}
  std::vector<Entity*>::iterator getEntityEndIterator()
  {return _entities.end();}

  void tempGetEntityPos(float* x, float* y, float* z)
  {
    std::vector<Entity*>::iterator it = _entities.begin();
    if(it != _entities.end())
    {
      (*it)->getPosition(x, y, z);
    }
    else
    {
      *x = *y = *z = 0.0;
    }
  }
}
