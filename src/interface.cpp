
#include <cstdio>
#include <cstdlib>

#include <list>

#include "utils.h"
#include "common.h"

#include "app.h"
#include "gui.h"
#include "terrain.h"
#include "entities.h"
#include "scenario.h"
#include "resources.h"

#include "interface.h"


extern GUI::SelectorGrid*      _tes_brush_selector;
extern GUI::SelectorGrid*      _tes_slope_selector;


namespace
{
  //horde3d nodes and resources
  Common::MeshFactory _mesh_fac;
  
  H3DRes _hmap_cursor_general_mat;
  H3DRes _hmap_cursor_square_mat;
  H3DRes _hmap_cursor_radial_mat;
  H3DRes _dragbox_mat;
  H3DRes _unit_hilite_mat;

  H3DNode _hmap_cursor;
  H3DNode _hmap_click;

  //gui stuff
  const std::string _path = "content/gui/";

  //widgets
  GUI::ContainerWidget* _screen = nullptr;

  //selection stuff
  struct Selection
  {
    Entity* entity;
    H3DNode node;
    Selection(Entity* e): entity(e), node(0){}
    Selection(const Selection& sel)
    {
      entity = sel.entity;
      node = h3dAddModelNode(
        entity->getSceneGraphNode(),
        "", _mesh_fac.res);
      h3dSetNodeTransform(
        node,
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        1.0, 1.0, 1.0);
      H3DNode mesh_node = _mesh_fac(node, "", _unit_hilite_mat);
      h3dSetNodeTransform(
        mesh_node,
        -0.25, 0.0, -0.25,
        0.0, 0.0, 0.0,
        0.5, 1.0, 0.5);
    }
    ~Selection()
    {
      if(node) h3dRemoveNode(node);
    }
  };

  std::vector<Selection> _selection;

  //entity stuff
  struct UnitBox
  {
    Entity* handle;
    int16_t x_cen;
    int16_t y_cen;
    uint32_t size;
    UnitBox(Entity* h, uint16_t x, uint16_t y, uint32_t s)
      : handle(h), x_cen(x), y_cen(y), size(s){}
  };

  std::vector<UnitBox> _entities_in_view;

  inline void _updateEntitiesInView()
  {
    std::vector<Entity*>::iterator it = Entities::getEntityStartIterator();
    std::vector<Entity*>::iterator eit = Entities::getEntityEndIterator();

    const Utils::Matrix4f* mat = Scenario::camera->getScreenSpaceTransMat();
    Utils::Vec3f temp;

    _entities_in_view.clear();

    for(; it != eit; ++it)
    {
      temp = *mat * (*it)->getPosition();
      if(std::fabs(temp.x) < std::fabs(temp.z)
        && std::fabs(temp.y) < std::fabs(temp.z))
      {
        temp.x /= temp.z * 2; temp.y /= temp.z * -2;
        temp.x += .5; temp.y += .5;
        _entities_in_view.push_back(
            UnitBox(*it, temp.x * AppCtrl::screen_w,
                temp.y * AppCtrl::screen_h, 20));
      }
    }
  }

  inline void _selectEntity(short x, short y)
  {
    for(std::vector<UnitBox>::iterator it = _entities_in_view.begin();
      it != _entities_in_view.end(); ++it)
    {
      if((signed)(it->x_cen - it->size) < x
        && (signed)(it->x_cen + it->size) > x &&
        (signed)(it->y_cen - it->size) < y
        && (signed)(it->y_cen + it->size) > y)
      {
        _selection.push_back(it->handle);
        return;
      }
    }
  }

  inline void _selectEntities(short l, short r, short b, short t)
  {
    for(std::vector<UnitBox>::iterator it = _entities_in_view.begin();
      it != _entities_in_view.end(); ++it)
    {
      if(it->x_cen > l && it->x_cen < r && it->y_cen > b && it->y_cen < t)
        _selection.push_back(it->handle);
    }
  }

  inline void _setTarget(float x, float y)
  {
    for(std::vector<Selection>::iterator it = _selection.begin();
        it != _selection.end(); ++it)
        it->entity->setTarget(Utils::fixed32c_t(x), Utils::fixed32c_t(y));
  }

  //mouse control stuff
  struct
  {
    enum
    {
      left_down   = 0x00000001,
      right_down  = 0x00000002
    };

    uint16_t x_pos;
    uint16_t y_pos;
    uint16_t x_click;
    uint16_t y_click;
    uint32_t state;
  }_mouse;

  float _dragbox_verts[16] =
  {
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0
  };

  //cursor stuff
  struct
  {
    int16_t x_pos, y_pos;
    int16_t x_tile, y_tile;
    float x_map_pos, y_map_pos;
    float x_map_point, y_map_point;
    uint16_t mode;
    uint8_t size;
    uint8_t state;

    enum
    {
      low_nibble      = 0x0f,
      drawing         = 0x10,
      blocked         = 0x20
    };
  }_cursor;

  void _calculateCursorPosition()
  {
    Utils::Vec3f origin, direction, helper;

    Utils::pickRayNormalized(
      Scenario::camera->handle,
      (float)_cursor.x_pos / (float)AppCtrl::screen_w,
      1.0 - (float)_cursor.y_pos / (float)AppCtrl::screen_h,
      &origin.x, &origin.y, &origin.z, &direction.x,
      &direction.y, &direction.z);

    direction *= -Scenario::camera->getHeight() / direction.y;

    direction += origin;

    _cursor.x_map_pos = direction.x - 0.5 * _cursor.size;
    _cursor.y_map_pos = direction.z - 0.5 * _cursor.size;
    _cursor.x_tile = (int16_t)_cursor.x_map_pos;
    _cursor.y_tile = (int16_t)_cursor.y_map_pos;

    if((_cursor.mode & 0xff00) == Interface::Cursor::mode_entity)
      h3dSetNodeTransform(
        _hmap_cursor,
        _cursor.x_map_pos, 0.0, _cursor.y_map_pos,
        0.0, 0.0, 0.0, 1.0, 1.0, 1.0);
    else if(_cursor.mode != Interface::Cursor::mode_none)
      h3dSetNodeTransform(
        _hmap_cursor,
        (float)_cursor.x_tile, 0.0, (float)_cursor.y_tile,
        0.0, 0.0, 0.0, 1.0, 1.0, 1.0);

    return;
  }

  void _calculateClickPosition(uint16_t x, uint16_t y)
  {
    Utils::Vec3f origin, direction, helper;

    Utils::pickRayNormalized(
      Scenario::camera->handle,
      (float)x / (float)AppCtrl::screen_w,
      1.0 - (float)y / (float)AppCtrl::screen_h,
      &origin.x, &origin.y, &origin.z,
      &direction.x, &direction.y, &direction.z);

    direction *= -Scenario::camera->getHeight() / direction.y;

    constexpr int precicion = 1000;
    for(int x = precicion; x > 0; --x)
    {
      helper = origin + direction * ((float)x / precicion);
      if(Terrain::heightf(helper.x, helper.z) < helper.y) break;
    }

    _cursor.x_map_point = helper.x;
    _cursor.y_map_point = helper.z;
  }
}

namespace Interface
{
  void init()
  {
    _hmap_cursor_general_mat  = h3dFindResource(
      H3DResTypes::Material,
      "h_map_overlay.xml");
    _hmap_cursor_square_mat   = h3dFindResource(
      H3DResTypes::Material,
      "h_map_overlay_1.xml");
    _hmap_cursor_radial_mat   = h3dFindResource(
      H3DResTypes::Material,
      "h_map_overlay_2.xml");
    _dragbox_mat              = h3dFindResource(
      H3DResTypes::Material,
      "dragbox.xml");
    _unit_hilite_mat          = h3dFindResource(
      H3DResTypes::Material,
      "unit_hilite.xml");

    _mesh_fac.build("_hmaptile", Resources::generateGrid, 1.0, 8);
    
    AppCtrl::dumpMessages();

    _screen = (GUI::ContainerWidget*)GUI::init(
      AppCtrl::screen_w,
      AppCtrl::screen_h);

    _cursor.mode = Cursor::mode_none;
    _cursor.state = 0;

    _mouse.state = 0;

    //entities stuff
    _entities_in_view.reserve(400);
    _selection.reserve(200);
  }

  int loadFiles()
  {
    int ret = 0;

    std::string file_n = "";

    ///load fonts
    file_n = _path;
    file_n += "font.png.json";
    ret = GUI::loadFont(file_n.c_str());
    if(ret)
    {
      printf(
        "Error loading file ./%s\nError id: %x\n",
        file_n.c_str(), ret);
      return 1;
    }

    ///load styles
    file_n = _path;
    file_n += "gui.json";
    ret = GUI::loadStyle(file_n.c_str());
    if(ret)
    {
      printf(
        "Error loading file ./%s\nError id: %x\n",
        file_n.c_str(), ret);
      return 1;
    }

    return 0;
  }

  void deinit()
  {
    if(_hmap_cursor) h3dRemoveNode(_hmap_cursor);
    if(_hmap_click) h3dRemoveNode(_hmap_click);

    _hmap_cursor = _hmap_click = 0;

    _selection.clear();

    GUI::deinit();

    return;
  }

  void setActiveCamera(Camera* cam)
  {
    Scenario::camera.reset(cam);
  }

  void update()
  {
    //updating
    _updateEntitiesInView();

    //drawing
    h3dClearOverlays();
    _screen->draw();
    if(_mouse.state & _mouse.left_down)
      h3dShowOverlays(
        _dragbox_verts, 4,
        _dragbox_verts[0] * AppCtrl::screen_h,
        _dragbox_verts[8] * AppCtrl::screen_h,
        (1. - _dragbox_verts[1]) * AppCtrl::screen_h,
        (1. - _dragbox_verts[9]) * AppCtrl::screen_h,
        _dragbox_mat, 0);
  }

  namespace Game
  {

    ///mouse functions
    void leftClick(uint16_t x, uint16_t y)
    {
      _mouse.x_click = x;
      _mouse.y_click = y;

      _dragbox_verts[0] = (float)x / AppCtrl::screen_h;
      _dragbox_verts[12] = (float)x / AppCtrl::screen_h;
      _dragbox_verts[1] = (float)y / AppCtrl::screen_h;
      _dragbox_verts[5] = (float)y / AppCtrl::screen_h;

      _mouse.state |= _mouse.left_down;
    }

    void leftRelease()
    {
      _mouse.state &= ~_mouse.left_down;

      //selection update
      _selection.clear();

      if(std::abs(_mouse.x_click - _mouse.x_pos) < 10
        && std::abs(_mouse.y_click - _mouse.y_pos) < 10)
        _selectEntity(_mouse.x_pos, _mouse.y_pos);
      else
      {
        int l, r, t, b;
        if(_mouse.x_click < _mouse.x_pos)
        {
          l = _mouse.x_click;
          r = _mouse.x_pos;
        }else
        {
          l = _mouse.x_pos;
          r = _mouse.x_click;
        }
        if(_mouse.y_click < _mouse.y_pos)
        {
          b = _mouse.y_click;
          t = _mouse.y_pos;
        }
        else
        {
          b = _mouse.y_pos;
          t = _mouse.y_click;
        }

        _selectEntities(l, r, b, t);
      }
    }

    void rightClick(uint16_t x, uint16_t y)
    {
      _calculateClickPosition(x, y);

      //dispatch order
      for(auto& x: _selection)
      {
        x.entity->issueMoveCommand(_cursor.x_map_point, _cursor.y_map_point);
      }
    }

    void rightRelease()
    {
      //nop
    }

    void mouseMotion(uint16_t x, uint16_t y)
    {
      _mouse.x_pos = x;
      _mouse.y_pos = y;

      _dragbox_verts[4] = (float)x / AppCtrl::screen_h;
      _dragbox_verts[8] = (float)x / AppCtrl::screen_h;
      _dragbox_verts[9] = (float)y / AppCtrl::screen_h;
      _dragbox_verts[13] = (float)y / AppCtrl::screen_h;
    }
  }

  namespace Cursor
  {
#define __CURSOR_COLOR_TEAL     0.0, 1.0, 0.5, 0.2
#define __CURSOR_COLOR_RED      0.5, 0.0, 0.0, 0.3

    bool _isCliffable();

    //private functions
    inline void _createCursor()
    {
      H3DNode mesh_node = 0;

      _hmap_cursor = h3dAddModelNode(H3DRootNode, "hmap_cursor", _mesh_fac.res);

      if((_cursor.mode & 0xff00) != mode_entity)
      for(int n = 0; n < _cursor.size; ++n)
      for(int m = 0; m < _cursor.size; ++m)
      {
        mesh_node = _mesh_fac(_hmap_cursor, "", _hmap_cursor_square_mat);

        h3dSetNodeTransform(
          mesh_node,
          (float)n, 0., (float)m,
          0., 0., 0., 1., 1., 1.);
      }
      else
      {
        mesh_node = _mesh_fac(_hmap_cursor, "", _hmap_cursor_radial_mat);
        h3dSetNodeTransform(mesh_node, -.25, 0., -.25, 0., 0., 0., .5, 1., .5);
      }
    }

    inline void _draw()
    {
      if((_cursor.mode & 0xff00) == mode_brush)
        Terrain::modifyDistanceFieldMap(
          _cursor.x_tile, _cursor.y_tile,
          _cursor.size, _cursor.size,
          _cursor.state & _cursor.low_nibble);
      else if((_cursor.mode & 0xff00) == mode_slope)
      {
        switch(_cursor.mode)
        {
          case mode_slope_ne:
          Terrain::addSlope(_cursor.x_tile, _cursor.y_tile, Terrain::cliff_northeast);
          break;
          case mode_slope_se:
          Terrain::addSlope(_cursor.x_tile, _cursor.y_tile, Terrain::cliff_southeast);
          break;
          case mode_slope_sw:
          Terrain::addSlope(_cursor.x_tile, _cursor.y_tile, Terrain::cliff_southwest);
          break;
          case mode_slope_nw:
          Terrain::addSlope(_cursor.x_tile, _cursor.y_tile, Terrain::cliff_northwest);
          break;
        }
      }
      else if(_cursor.mode == mode_unit)
      {
        if(!Terrain::isObstacle(_cursor.x_map_pos, _cursor.y_map_pos))
        {
          auto p_ptr = Scenario::getPlayer(1);
      
          Entities::insertEntity(_cursor.x_map_pos, _cursor.y_map_pos, p_ptr);
          _cursor.state = 0;
        }
      }
      else if(_cursor.mode == mode_light)
      {
        FogOfWar::insertLightSource(_cursor.x_map_pos, _cursor.y_map_pos, 6.0);
        _cursor.state = 0;
      }
      else return;
    }

    inline void _updateColor()
    {
      if((_cursor.mode & 0xff00) == mode_brush)
        h3dSetMaterialUniform(
          _hmap_cursor_general_mat,
          "HiliteColor",
          __CURSOR_COLOR_TEAL);
      else if((_cursor.mode & 0xff00) == mode_slope)
      {
        if(_isCliffable())
        {
          h3dSetMaterialUniform(
            _hmap_cursor_general_mat,
            "HiliteColor",
            __CURSOR_COLOR_TEAL);
          _cursor.state &= ~_cursor.blocked;
        }
        else
        {
          h3dSetMaterialUniform(
            _hmap_cursor_general_mat,
            "HiliteColor",
            __CURSOR_COLOR_RED);
          _cursor.state |= _cursor.blocked;
        }
      }
      else if(_cursor.mode == mode_unit)
      {
        if(Terrain::isObstacle(_cursor.x_map_pos, _cursor.y_map_pos))
        {
          h3dSetMaterialUniform(
            _hmap_cursor_general_mat,
            "HiliteColor",
            __CURSOR_COLOR_RED);
          _cursor.state |= _cursor.blocked;
        }
        else
        {
          h3dSetMaterialUniform(
            _hmap_cursor_general_mat,
            "HiliteColor",
            __CURSOR_COLOR_TEAL);
          _cursor.state &= _cursor.blocked;
        }
      }
    }

    bool _isCliffable()
    {
      uint8_t min_height;
      uint8_t max_height;
      uint8_t heights = 0;

      min_height = Terrain::getLowest(_cursor.x_tile, _cursor.y_tile, 2, 2);
      max_height = Terrain::getHighest(_cursor.x_tile, _cursor.y_tile, 2, 2);

      if(min_height == 0 || (max_height - min_height) != 1) return false;

      if(Terrain::getTile(_cursor.x_tile, _cursor.y_tile)
        != min_height) heights |= 0x01;
      if(Terrain::getTile(_cursor.x_tile + 1, _cursor.y_tile)
        != min_height) heights |= 0x02;
      if(Terrain::getTile(_cursor.x_tile, _cursor.y_tile + 1)
        != min_height) heights |= 0x04;
      if(Terrain::getTile(_cursor.x_tile + 1, _cursor.y_tile + 1)
        != min_height) heights |= 0x08;

      //this returns false if there is already a cliff there and
      //it is pointing in a wrong direction
      if((min_height = Terrain::getDoodad(
        _cursor.x_tile, _cursor.y_tile)) &&
        ((min_height & Terrain::Doodad::directionPart) /
        Terrain::Doodad::dirSouthWest) != (_cursor.mode & 0x00ff) - 1)
          return false;
      if((min_height = Terrain::getDoodad(
        _cursor.x_tile + 1, _cursor.y_tile)) &&
        ((min_height & Terrain::Doodad::directionPart) /
        Terrain::Doodad::dirSouthWest) != (_cursor.mode & 0x00ff) - 1)
          return false;
      if((min_height = Terrain::getDoodad(
        _cursor.x_tile, _cursor.y_tile + 1)) &&
        ((min_height & Terrain::Doodad::directionPart) /
        Terrain::Doodad::dirSouthWest) != (_cursor.mode & 0x00ff) - 1)
          return false;
      if((min_height = Terrain::getDoodad(
        _cursor.x_tile + 1, _cursor.y_tile + 1)) &&
        ((min_height & Terrain::Doodad::directionPart) /
        Terrain::Doodad::dirSouthWest) != (_cursor.mode & 0x00ff) - 1)
          return false;
      
      switch(_cursor.mode)
      {
      case mode_slope_ne:
        if(heights == 0x0c) return true;
        break;
      case mode_slope_se:
        if(heights == 0x05) return true;
        break;
      case mode_slope_sw:
        if(heights == 0x03) return true;
        break;
      case mode_slope_nw:
        if(heights == 0x0a) return true;
      default:
        return false;
      }
      return false;
    }

    //public functions
    void setMode(int mode)
    {
      _cursor.mode = mode;
      if(_hmap_cursor != 0)h3dRemoveNode(_hmap_cursor);
      _hmap_cursor = 0;

      _cursor.state = 0;

      switch(mode)
      {
      case mode_x1:
        _cursor.size = 1;
        break;
      case mode_x2:
      case mode_slope_ne: case mode_slope_nw:
      case mode_slope_se: case mode_slope_sw:
        _cursor.size = 2;
        break;
      case mode_x3:
        _cursor.size = 3;
        break;
      case mode_x4:
        _cursor.size = 4;
        break;
      case mode_unit:
      case mode_light:
        _cursor.size = 0;
        break;
      default:
        _cursor.size = 0;
        return;
      }

      _createCursor();
      _updateColor();
    }

    int getMode()
    {
      return _cursor.mode;
    }

    void setDrawingMode(uint8_t mode)
    {
      if((_cursor.mode & 0xff00) & mode_brush)
      {
        int8_t temp;
        if(mode == 1)
        {
          temp = Terrain::getLowest(_cursor.x_tile, _cursor.y_tile,
              _cursor.size, _cursor.size) + 1;
          _cursor.state = _cursor.drawing | (temp > 4? 4 : temp);
          _draw();
        }
        else if(mode == 2)
        {
          temp = Terrain::getHighest(_cursor.x_tile, _cursor.y_tile,
              _cursor.size, _cursor.size) - 1;
          _cursor.state = _cursor.drawing | (temp < 0? 0 : temp);
          _draw();
        }
        else _cursor.state = 0;
      }
      else if((_cursor.mode & 0xff00) & mode_slope)
      {
        if(mode == 1)
        {
          _cursor.state = _cursor.drawing;
          if(!_isCliffable()) _cursor.state |= _cursor.blocked;
          else _draw();
        }
        else _cursor.state = 0;
      }
      else if(_cursor.mode == mode_unit)
      {
        if(mode == 1)
        {
          _cursor.state = _cursor.drawing;
          if(Terrain::isObstacle(_cursor.x_map_pos, _cursor.y_map_pos))
            _cursor.state |= _cursor.blocked;
          else _draw();
        }
        else _cursor.state = 0;
      }
      else if(_cursor.mode == mode_light)
      {
        if(mode == 1)
        {
          _cursor.state = _cursor.drawing;
          _draw();
        }
        else _cursor.state = 0;
      }
      else return;
    }

    void updatePosition(uint16_t x, uint16_t y)
    {
      _cursor.x_pos = x;
      _cursor.y_pos = y;

      if(Scenario::camera != nullptr && _cursor.mode != mode_none)
      {
        _calculateCursorPosition();
        _updateColor();
        if((_cursor.state & _cursor.drawing)
          && !(_cursor.state & _cursor.blocked))
          _draw();
      }
    }

#undef __CURSOR_COLOR_TEAL
#undef __CURSOR_COLOR_RED
  }
}

