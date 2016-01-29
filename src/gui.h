#pragma once

#include <string>
#include <list>
#include <map>
#include <vector>

#include <stdint.h>

#include <horde3d.h>

#include <SDL.h>

#include "utils.h"

namespace GUI
{
                                  ///enumerations
  //random constants
  enum
  {
    _component_stack_size = 64
  };

  //frame_flags
  enum
  {
    frame_left      = 0x01,
    frame_right     = 0x02,
    frame_top       = 0x04,
    frame_bottom    = 0x08,
    frame_none      = 0x10
  };

  //bit flags
  enum
  {
    flags_none              = 0x00000000,
    flags_mother            = 0x00000001,

    flags_resizable_left    = 0x00000002,
    flags_resizable_right   = 0x00000004,
    flags_resizable_top     = 0x00000008,
    flags_resizable_bottom  = 0x00000010,
    flags_resizable         = 0x0000001e,

    flags_dragable          = 0x00000020,
    flags_surrogate         = 0x00000040,

    flags_hide_left         = 0x00000080,
    flags_hide_right        = 0x00000100,
    flags_hide_top          = 0x00000200,
    flags_hide_bottom       = 0x00000400,
    flags_hide_frame        = 0x00000780,

    flags_horizontal        = 0x00000800,
    flags_vertical          = 0x00001000,

    flags_invisible         = 0x00002000,

    flags_updating          = 0x00004000,
    flags_invalid           = 0x00008000,

    flags_autoresize        = 0x00010000,

    flags_hold              = 0x00020000,
    flags_toggle            = 0x00040000,
    flags_active            = 0x00080000,

    flags_text              = 0x00100000,
    flags_icon              = 0x00200000
  };

  //states
  enum
  {
    state_normal            = 0x00,
    state_active            = 0x01,
    state_excited           = 0x02,
    state_excited_active    = 0x03
  };

  enum
  {
    align_top,
    align_bottom,
    align_left,
    align_right
  };


  class Widget;

  //other declarations
  typedef void (*Callback)(const void*);

                                  ///Font class
  class Font
  {
    private:

    struct _CharInfo
    {
      float x_texf, y_texf, w_texf, h_texf;
      uint16_t x_texi, y_texi, w_texi, h_texi;
      uint8_t x_offset, y_offset;
      uint8_t advance;
    };

    H3DRes _material;

    //common info
    uint16_t _width;
    uint16_t _height;
    uint16_t _line_height;
    uint16_t _base;
    uint16_t _space_size;

    std::map<int, _CharInfo*> _glyphs;

    void _clearGlyphs()
    {
      for(std::map<int, _CharInfo*>::iterator it = _glyphs.begin();
        it != _glyphs.end(); ++it)
      {
        delete it->second;
      }
      _glyphs.clear();
    }

    public:

    Font()
    {
      _material = 0;
    }

    ~Font()
    {
      if(_material != 0)
        h3dRemoveResource(_material);

      _clearGlyphs();
    }

    friend class TextComponent;

    friend int loadFont(const char*);
    friend Font* findFont(const char*);
  };

                                  ///Style class
  class Style
  {
    private:

    friend class Widget;

    H3DRes material;
    Font* font;

    uint16_t tex_width;
    uint16_t tex_height;

    struct
    {
      float left;
      float right;
      float top;
      float bottom;

      uint16_t px_left;
      uint16_t px_right;
      uint16_t px_top;
      uint16_t px_bottom;
    }frame;

    struct
    {
      float left;
      float right;
      float top;
      float bottom;

      uint16_t px_width;
      uint16_t px_height;
    }body;

    struct
    {
      float x_shift;
      float y_shift;
    }active, excited, excited_active;



    uint8_t wrap;
    uint8_t resolution;

    public:

    Font* getFont(){return this->font;}

    Style()
    {
      material = 0;
      font = nullptr;

      tex_width = 0;
      tex_height = 0;

      frame.left = 0.0;
      frame.right = 0.0;
      frame.top = 0.0;
      frame.bottom = 0.0;

      frame.px_left = 0;
      frame.px_right = 0;
      frame.px_top = 0;
      frame.px_bottom = 0;

      body.left = 0.0;
      body.right = 0.0;
      body.top = 0.0;
      body.bottom = 0.0;

      body.px_width = 0;
      body.px_height = 0;

      active.x_shift = 0.0;
      active.y_shift = 0.0;
      excited.x_shift = 0.0;
      excited.y_shift = 0.0;
      excited_active.x_shift = 0.0;
      excited_active.y_shift = 0.0;

      wrap = 0;
      resolution = 1;
    }

    friend int loadStyle(const char*);
    friend Style* findStyle(const char*);
  };

  extern std::map<std::string, GUI::Font*> _fonts;
  extern std::map<std::string, GUI::Style*> _styles;

  ///position class
  class Pos
  {
    private:

    struct __Pos
    {
      float x_rel;
      int x_abs;
      float y_rel;
      int y_abs;

      __Pos(float xr, int xa, float yr, int ya)
      : x_rel(xr), x_abs(xa), y_rel(yr), y_abs(ya){}
    };

    public:

    __Pos _p1, _p2;

    Pos()
    : _p1(0., 0, 0., 0), _p2(0., 0, 0., 0) {}
    Pos(const Pos& other)
    : _p1(other._p1), _p2(other._p2){}

    Pos(float xr1, int xa1, float yr1, int ya1, float xr2,
        int xa2, float yr2, int ya2)
    : _p1(xr1, xa1, yr1, ya1), _p2(xr2, xa2, yr2, ya2){}
    Pos(float xr1, float yr1, float xr2, float yr2)
    : _p1(xr1, 0, yr1, 0), _p2(xr2, 0, yr2, 0){}
    Pos(int xa1, int ya1, int xa2, int ya2)
    : _p1(xa1 < 0? 1.0 : 0.0, xa1, ya1 < 0? 1.0 : 0.0, ya1),
    _p2(xa2 < 0? 1.0 : 0.0, xa2, ya2 < 0? 1.0 : 0.0, ya2){}
    Pos(int orient, float rel)
    : _p1(0., 0, 0., 0), _p2(0., 0, 0., 0)
    {
      switch(orient)
      {
        case align_top:     _p2.y_rel = rel;        break;
        case align_bottom:  _p1.y_rel = 1.0 - rel;  break;
        case align_left:    _p2.x_rel = rel;        break;
        case align_right:   _p1.x_rel = 1.0 - rel;  break;
        default: break;
      }
    }
    Pos(int orient, int rel)
    : _p1(0., 0, 0., 0), _p2(0., 0, 0., 0)
    {
      switch(orient)
      {
        case align_top:     _p2.y_abs = rel;    _p2.y_rel = 0.0; break;
        case align_bottom:  _p1.y_abs = -rel;   _p1.y_rel = 1.0; break;
        case align_left:    _p2.x_abs = rel;    _p2.x_rel = 0.0; break;
        case align_right:   _p1.x_abs = -rel;   _p1.x_rel = 1.0; break;
        default: break;
      }
    }
  };

                                  ///Component class
  class Container;

  class Component: public Utils::RefCountedStd, protected Pos
  {
    protected:

    static Component* _component_stack[_component_stack_size];
    static Component** _component_stack_ref;

    double _x_pos, _y_pos, _width, _height;
    Component* _mother;

    uint32_t _flags;

    /*struct
    {
      int x_abs;
      int y_abs;
      float x_rel;
      float y_rel;
    }_p1, _p2;*/

    void _updateContents();
    void _drawContents();

    void _updateRelativePos();
    void _setPos(float xr1, int xa1,
                 float yr1, int ya1,
                 float xr2, int xa2,
                 float yr2, int ya2);
    virtual void _updatePos();

    virtual void _getBoundingBox(int*, int*, int*, int*);
    virtual void _getBoundingBox(double*, double*, double*, double*);
    virtual void _getDrawingBox(int*, int*, int*, int*);
    virtual void _getDrawingBox(double*, double*, double*, double*);

    virtual int _handleMouseEvent(SDL_Event&, int = 0);
    virtual void _handleSelectionEvent(Component*);

    virtual Component** _stackSubComponents();

    Component* _getComponent(float, float);

    public:

    Component();
    Component(const Pos&);
    virtual ~Component();

    //Component* getComponent(float, float);
    virtual void draw();

    //behold the consequences of class incest
    friend class Icon;
    friend class TextBox;
    friend class Widget;
    friend class ContainerWidget;

    friend class Window;
    friend class Button;
    friend class TextComponent;

    friend class GridContainer;
    friend class GridPanel;
    friend class SelectorGrid;

    friend class ScrollArea;
    friend class OrderedContainer;
    friend class VerticalContainer;

    friend Component* init(uint16_t, uint16_t);
    friend bool trapEvent(SDL_Event&);
  };


  class Widget: public Component
  {
    protected:

    Style *_style;
    float* _verts;
    uint8_t _sides;
    uint8_t _state;
    uint16_t _indent;

    virtual void _updatePos();

    void _updateVerts();

    public:

    void draw();

    Widget(Style* style, uint32_t flags, const Pos&);
    virtual ~Widget();

    friend class ScrollArea;
    friend class SelectorGrid;
    friend class ContainerWidget;
    friend class VerticalContainer;
  };

  class ContainerWidget: public Widget
  {
    protected:

    std::vector<Utils::counted_ptr<Component>> _sub_components;

    Component** _stackSubComponents();

    public:
    ContainerWidget(Style* style, uint32_t flags, const Pos& pos)
                    : Widget(style, flags, pos){};
    virtual ~ContainerWidget()
    {
      _sub_components.clear();
    };

    template<class C, class... T>
    C* attachComponent(T... t)
    {
      C* component = Utils::makeRefCounted<C>(t...);
      component->_mother = this;
      component->_updatePos();
      _sub_components.push_back(component);
      return component;
    }

    void detachComponent(Component*);
    void detachAllComponents();
  };

  class TextComponent: public Component
  {
    protected:

    float* _verts;
    Font* _font;
    uint32_t _num_chars;
    std::string _text;

    void _updateVerts();
    void _allocate();

    void _updatePos();

    int _handleMouseEvent(SDL_Event&, int = 0);

    public:

    TextComponent(Font*, const char*, const Pos&);
    TextComponent(Font*, const Pos&);
    ~TextComponent();

    void draw();
    void setText(const char*);

    friend class TextBox;
  };
}

#include "widgets.h"

namespace GUI
{
  Component* init(uint16_t, uint16_t);
  void deinit();
  Component* getScreenPointer();

  int loadStyle(const char*);
  Style* findStyle(const char*);
  int loadFont(const char*);
  Font* findFont(const char*);

  bool trapEvent(SDL_Event&);
}
