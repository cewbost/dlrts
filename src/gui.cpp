
#include <horde3d.h>

#include <vector>
#include <map>
#include <string>
#include <stack>
#include <algorithm>
#include <functional>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "json.h"
#include "utils.h"

#include "gui.h"

namespace
{
  typedef float __VertsLarge[144];
  typedef float __VertsSmall[16];

  Utils::counted_ptr<GUI::ContainerWidget> _screen;

  Utils::ArenaAllocator<sizeof(__VertsSmall), 64> _allocator_verts_small;
  Utils::ArenaAllocator<sizeof(__VertsLarge), 128> _allocator_verts_large;
}

uint16_t _view_height;
uint16_t _view_width;
float _view_aspect;

namespace GUI
{
  ///globals
  std::map<std::string, GUI::Font*> _fonts;
  std::map<std::string, GUI::Style*> _styles;

  /***Component***/
  //statics
  GUI::Component** Component::_component_stack_ref;
  GUI::Component* Component::_component_stack[_component_stack_size];

  //constructor
  Component::Component(): Pos()
  {
    _mother = nullptr;
  }

  Component::Component(const Pos& pos): Pos(pos)
  {
    _mother = nullptr;
  }

  Component::~Component()
  {
    //nop
  }

  ///methods
  void Component::_setPos(float xr1, int xa1,
                          float yr1, int ya1,
                          float xr2, int xa2,
                          float yr2, int ya2)
  {
    _p1.x_abs = xa1;
    _p1.y_abs = ya1;
    _p1.x_rel = xr1;
    _p1.y_rel = yr1;
    _p2.x_abs = xa2;
    _p2.y_abs = ya2;
    _p2.x_rel = xr2;
    _p2.y_rel = yr2;
  }

  void Component::_updateRelativePos()
  {
    int left, right, bottom, top;
    int m_left, m_right, m_bottom, m_top;
    _mother->_getBoundingBox(&m_left, &m_right, &m_top, &m_bottom);

    left =      _p1.x_abs + m_left + (int)(_p1.x_rel * (m_right - m_left));
    top =       _p1.y_abs + m_top + (int)(_p1.y_rel * (m_bottom - m_top));
    right =     _p2.x_abs + m_left + (int)(_p2.x_rel * (m_right - m_left));
    bottom =    _p2.y_abs + m_top + (int)(_p2.y_rel * (m_bottom - m_top));

    /*left    += left % 2;
    top     += top % 2;
    right   += right % 2;
    bottom  += bottom % 2;*/

    _x_pos = ((double)left + .5) / _view_height;
    _y_pos = ((double)top + .5) / _view_height;
    _width = (double)(right - left) / _view_height;
    _height = (double)(bottom - top) / _view_height;
  }

  void Component::_getBoundingBox(int* left, int* right, int* top, int* bottom)
  {
    *left   = _x_pos * _view_height;
    *right  = (_x_pos + _width) * _view_height/* + 2*/;
    *top    = _y_pos * _view_height;
    *bottom = (_y_pos + _height) * _view_height/* + 2*/;
  }

  void Component::_getBoundingBox(double* left, double* right,
                                  double* top, double* bottom)
  {
    *left   = _x_pos;
    *right  = (_x_pos + _width);
    *top    = _y_pos;
    *bottom = (_y_pos + _height);
  }

  void Component::_getDrawingBox(int* left, int* right, int* top, int* bottom)
  {
    *left   = _x_pos * _view_height;
    *right  = (_x_pos + _width) * _view_height/* + 2*/;
    *top    = _y_pos * _view_height;
    *bottom = (_y_pos + _height) * _view_height/* + 2*/;

    if(_mother != nullptr)
    {
      int l, r, t, b;
      _mother->_getDrawingBox(&l, &r, &t, &b);
      if(*left < l) *left = l;
      if(*right > r) *right = r;
      if(*top < t) *top = t;
      if(*bottom > b) *bottom = b;
    }
  }

  void Component::_getDrawingBox(double* left, double* right,
                                  double* top, double* bottom)
  {
    *left   = _x_pos;
    *right  = (_x_pos + _width);
    *top    = _y_pos;
    *bottom = (_y_pos + _height);

    if(_mother != nullptr)
    {
      double l, r, t, b;
      _mother->_getDrawingBox(&l, &r, &t, &b);
      if(*left < l) *left = l;
      if(*right > r) *right = r;
      if(*top < t) *top = t;
      if(*bottom > b) *bottom = b;
    }
  }

  void Component::_updatePos()
  {
    _updateRelativePos();
    _updateContents();
  }

  void Component::draw()
  {
    //if(_flags & flags_invisible) return;
    _drawContents();
  }

  int Component::_handleMouseEvent(SDL_Event& event, int d){return 0;}
  void Component::_handleSelectionEvent(Component* component){return;}

  Component** Component::_stackSubComponents()
  {
    return _component_stack_ref;
  }

  Component* Component::_getComponent(float x, float y)
  {
    Component** last = _component_stack_ref;
    Component** current = _stackSubComponents();
    Component* found = this;

    while(current != last)
    {
      if(x > (*current)->_x_pos && x < ((*current)->_x_pos +
        (*current)->_width) && y > (*current)->_y_pos && y
        < ((*current)->_y_pos + (*current)->_height))
      {
        found = (*current)->_getComponent(x, y);
        break;
      }

      --current;
    }

    _component_stack_ref = last;

    return found;
  }

  //helper methods(non virtual, get inlined)
  void Component::_updateContents()
  {
    Component** last = _component_stack_ref;
    Component** current = _stackSubComponents();

    while(current != last)
    {
      (*current)->_updatePos();
      --current;
    }

    _component_stack_ref = last;
  }
  void Component::_drawContents()
  {
    Component** last = _component_stack_ref;
    Component** current = _stackSubComponents();

    while(current != last)
    {
      (*current)->draw();
      --current;
    }

    _component_stack_ref = last;
  }

  /***Widget***/
  Widget::Widget(Style* style, uint32_t flags, const Pos& pos): Component(pos)
  {
    _flags = flags;
    _style = style;
    _sides = 0;
    _state = state_normal;

    if(_style == nullptr)
      _verts = nullptr;
    else
    {
      if(style->frame.px_left > 0)     _sides = _sides | frame_left;
      if(style->frame.px_right > 0)    _sides = _sides | frame_right;
      if(style->frame.px_top > 0)      _sides = _sides | frame_top;
      if(style->frame.px_bottom > 0)   _sides = _sides | frame_bottom;

      _sides = _sides & ~((flags & flags_hide_frame) / flags_hide_left);

      if(_sides)
        _verts = (float*)_allocator_verts_large.getAdress();
      else
        _verts = (float*)_allocator_verts_small.getAdress();
    }
  }

  Widget::~Widget()
  {
    if(_verts != nullptr)
    {
      if(_sides)
        _allocator_verts_large.freeAdress(_verts);
      else
        _allocator_verts_small.freeAdress(_verts);
    }
  }

  void Widget::_updatePos()
  {
    _updateRelativePos();
    _updateContents();
    if(_verts != nullptr)_updateVerts();
  }

  void Widget::_updateVerts()
  {
    double x_coords[4];
    double y_coords[4];

    double wrap_Component[2];

    uint8_t &size_multip = _style->resolution;

    double x_min, y_min, x_max, y_max;

    _mother->_getDrawingBox(&x_min, &x_max, &y_min, &y_max);

    /*
      this part selects the screen-space coordinates for the vertices
      and normalizes them to fit inside the mother window
    */
    if(_sides)
    {
      x_coords[0] = x_coords[1] = _x_pos;
      x_coords[2] = x_coords[3] = _x_pos + _width;
      y_coords[0] = y_coords[1] = _y_pos;
      y_coords[2] = y_coords[3] = _y_pos + _height;

      if(_sides & frame_left) x_coords[1] 
        += (float)(_style->frame.px_left * size_multip) / _view_height;
      if(_sides & frame_right) x_coords[2] 
        -= (float)(_style->frame.px_right * size_multip) / _view_height;
      if(_sides & frame_top) y_coords[1] 
        += (float)(_style->frame.px_top * size_multip) / _view_height;
      if(_sides & frame_bottom) y_coords[2] 
        -= (float)(_style->frame.px_bottom * size_multip) / _view_height;

      for(int n = 0; n < 4; ++n)
      {
        x_coords[n] = x_coords[n] > x_max? 
        x_max : (x_coords[n] < x_min? x_min : x_coords[n]);
        y_coords[n] = y_coords[n] > y_max?
        y_max : (y_coords[n] < y_min? y_min : y_coords[n]);
      }
    }
    else
    {
      x_coords[1] = _x_pos;
      x_coords[2] = _x_pos + _width;

      y_coords[1] = _y_pos;
      y_coords[2] = _y_pos + _height;

      x_coords[1] = x_coords[1] > x_max?
      x_max : (x_coords[1] < x_min? x_min : x_coords[1]);
      y_coords[1] = y_coords[1] > y_max?
      y_max : (y_coords[1] < y_min? y_min : y_coords[1]);
      x_coords[2] = x_coords[2] > x_max?
      x_max : (x_coords[2] < x_min? x_min : x_coords[2]);
      y_coords[2] = y_coords[2] > y_max?
      y_max : (y_coords[2] < y_min? y_min : y_coords[2]);
    }

    wrap_Component[0] = x_coords[2] - x_coords[1];
    wrap_Component[1] = y_coords[2] - y_coords[1];

    float* verts = _verts;

    /*
      this part stores the screen-space coordinates and texture coordinates
      in the <code>_verts<\code> array.
      For wrapping skins the texture coordinates
      are stored in the range [0.0 : R+], otherwise [0.0 : 1.0]
    */
    if(_sides)
    for(int x = 0; x < 3; ++x)
    for(int y = 0; y < 3; ++y, verts += 16)
    {
      verts[0] = verts[4] = (float)x_coords[x];
      verts[8] = verts[12] = (float)x_coords[x + 1];
      verts[1] = verts[13] = (float)y_coords[y];
      verts[5] = verts[9] = (float)y_coords[y + 1];
      verts[2] = verts[6] = verts[3] = verts[15] = 0.0;

      //wrapping
      if(_style->wrap > 0)
      {
        if(x == 1)
          verts[10] = verts[14] = (float)wrap_Component[0]
        * _view_height / (_style->body.px_width * size_multip);
        else verts[10] = verts[14] = 1.0;
        if(y == 1)
          verts[7] = verts[11] = (float)wrap_Component[1]
        * _view_height / (_style->body.px_height * size_multip);
        else verts[7] = verts[11] = 1.0;
      }
      else
        verts[10] = verts[14] = verts[7] = verts[11] = 1.0;
    }
    else
    {
      verts[0] = verts[4] = (float)x_coords[1];
      verts[8] = verts[12] = (float)x_coords[2];
      verts[1] = verts[13] = (float)y_coords[1];
      verts[5] = verts[9] = (float)y_coords[2];
      verts[2] = verts[6] = verts[3] = verts[15] = 0.0;

      //wrapping
      if(_style->wrap > 0)
      {
        verts[10] = verts[14] = (float)wrap_Component[0] *
        _view_height / (_style->body.px_width * size_multip);
        verts[7] = verts[11] = (float)wrap_Component[1] *
        _view_height / (_style->body.px_height * size_multip);
      }
    }
  }

  void Widget::draw()
  {
    if(_flags & flags_invisible) return;

    if(_verts != nullptr)
    {
      struct
      {
        float left;
        float right;
        float top;
        float bottom;
      }body, frame;

      //state shifts
      switch(_state)
      {
        case state_excited_active:

        body.left       = _style->body.left
                        + _style->excited_active.x_shift;
        body.right      = _style->body.right
                        + _style->excited_active.x_shift;
        body.top        = _style->body.top
                        + _style->excited_active.y_shift;
        body.bottom     = _style->body.bottom
                        + _style->excited_active.y_shift;

        frame.left      = _style->frame.left
                        + _style->excited_active.x_shift;
        frame.right     = _style->frame.right
                        + _style->excited_active.x_shift;
        frame.top       = _style->frame.top
                        + _style->excited_active.y_shift;
        frame.bottom    = _style->frame.bottom
                        + _style->excited_active.y_shift;

        break;

        case state_excited:

        body.left       = _style->body.left
                        + _style->excited.x_shift;
        body.right      = _style->body.right
                        + _style->excited.x_shift;
        body.top        = _style->body.top
                        + _style->excited.y_shift;
        body.bottom     = _style->body.bottom
                        + _style->excited.y_shift;

        frame.left      = _style->frame.left
                        + _style->excited.x_shift;
        frame.right     = _style->frame.right
                        + _style->excited.x_shift;
        frame.top       = _style->frame.top
                        + _style->excited.y_shift;
        frame.bottom    = _style->frame.bottom
                        + _style->excited.y_shift;

        break;

        case state_active:

        body.left       = _style->body.left
                        + _style->active.x_shift;
        body.right      = _style->body.right
                        + _style->active.x_shift;
        body.top        = _style->body.top
                        + _style->active.y_shift;
        body.bottom     = _style->body.bottom
                        + _style->active.y_shift;

        frame.left      = _style->frame.left
                        + _style->active.x_shift;
        frame.right     = _style->frame.right
                        + _style->active.x_shift;
        frame.top       = _style->frame.top
                        + _style->active.y_shift;
        frame.bottom    = _style->frame.bottom
                        + _style->active.y_shift;

        break;

        case state_normal:
        default:

        body.left       = _style->body.left;
        body.right      = _style->body.right;
        body.top        = _style->body.top;
        body.bottom     = _style->body.bottom;

        frame.left      = _style->frame.left;
        frame.right     = _style->frame.right;
        frame.top       = _style->frame.top;
        frame.bottom    = _style->frame.bottom;

        break;
      }

      //drawing
      if(_sides == 0)
      {
        h3dShowOverlays(_verts, 4,
                        body.left, body.right, body.top, body.bottom,
                        _style->material, 0);
      }
      else
      {
        h3dShowOverlays(_verts + 16 * 4, 4,
                        body.left, body.right, body.top, body.bottom,
                        _style->material, 0);

        //edges
        if(_sides & frame_left)
          h3dShowOverlays(_verts + 16 * 1, 4,
                      frame.left, body.left, body.top, body.bottom,
                      _style->material, 0);
        if(_sides & frame_top)
          h3dShowOverlays(_verts + 16 * 3, 4,
                      body.left, body.right, frame.top, body.top,
                      _style->material, 0);
        if(_sides & frame_bottom)
          h3dShowOverlays(_verts + 16 * 5, 4,
                      body.left, body.right, body.bottom, frame.bottom,
                      _style->material, 0);
        if(_sides & frame_right)
          h3dShowOverlays(_verts + 16 * 7, 4,
                      body.right, frame.right, body.top, body.bottom,
                      _style->material, 0);
        
        //corners
        if(_sides & (frame_left | frame_top))
          h3dShowOverlays(_verts, 4,
                      frame.left, body.left, frame.top, body.top,
                      _style->material, 0);
        if(_sides & (frame_left | frame_bottom))
          h3dShowOverlays(_verts + 16 * 2, 4,
                      frame.left, body.left, body.bottom, frame.bottom,
                      _style->material, 0);
        if(_sides & (frame_right | frame_top))
          h3dShowOverlays(_verts + 16 * 6, 4,
                      body.right, frame.right, frame.top, body.top,
                      _style->material, 0);
        if(_sides & (frame_right | frame_bottom))
          h3dShowOverlays(_verts + 16 * 8, 4,
                      body.right, frame.right, body.bottom, frame.bottom,
                      _style->material, 0);
      }
    }

    _drawContents();
  }


  /***ContainerWidget***/

  Component** ContainerWidget::_stackSubComponents()
  {
    for(auto it: _sub_components)
    {
      ++_component_stack_ref;
      *_component_stack_ref = it.get();
    }

    return _component_stack_ref;
  }

  //detachment functions
  void ContainerWidget::detachComponent(Component* component)
  {
    auto it = std::find_if(_sub_components.begin(), _sub_components.end(),
    [=](const Utils::counted_ptr<Component> ptr)
    {return ptr.get() == component;});

    if(it != _sub_components.end())
      _sub_components.erase(it);
  }

  void ContainerWidget::detachAllComponents()
  {
    _sub_components.clear();
  }

  /***TextComponent methods***/

  void TextComponent::_updatePos()
  {
    _updateRelativePos();
    _updateVerts();
  }

  void TextComponent::_updateVerts()
  {
    if(_verts == nullptr || _font == nullptr)
      return;

    float* vert_p = _verts;
    int x_advance = 0;
    int y_advance = 0;

    float left, right, top, bottom;

    float min_x, min_y, max_x, max_y;
    float x_cutoff, y_cutoff;

    //mother->_getBoundingBox(&min_xi, &max_xi, &min_yi, &max_yi);

    //min_x = (float)min_xi / _view_height;
    //min_y = (float)min_yi / _view_height;
    //max_x = (float)max_xi / _view_height;
    //max_y = (float)max_yi / _view_height;

    min_x = _x_pos;
    min_y = _y_pos;
    max_x = _x_pos + _width;
    max_y = _y_pos + _height;

    int char_idx;

    //uint32_t len = _text.size();
    Font::_CharInfo* info;

    std::string new_text = "";

    const int num_lines = (int)(_height * _view_height)
                            / _font->_line_height;
    const int line_width = (int)(_width * _view_height);

    //format string
    std::string::iterator it = _text.begin();
    std::string::iterator setter = it;

    while(it != _text.end())
    {
      if(y_advance == num_lines)
        break;

      char_idx = (int)*it;

      if(char_idx == (int)' ')
      {
        x_advance += _font->_space_size;
        while(setter != it)
        {
          new_text += *setter;
          ++setter;
        }

        ++it;
        continue;
      }
      if(char_idx == (int)'\n')
      {
        while(setter != it)
        {
          new_text += *setter;
          ++setter;
        }

        ++y_advance;
        x_advance = 0;
        ++it;
        continue;
      }

      if(!_font->_glyphs.count(char_idx))
        char_idx = -1;

      info = _font->_glyphs[char_idx];

      x_advance += info->advance;

      if(x_advance > line_width)
      {
        x_advance = 0;
        ++y_advance;

        new_text += '\n';
        it = setter;
        ++it;
        setter = it;
        continue;
      }

      ++it;
    }

    while(setter != it)         //add last word
    {
      new_text += *setter;
      ++setter;
    }

    //set verts
    x_advance = 0;
    y_advance = 0;

    for(it = new_text.begin(); it != new_text.end(); ++it)
    {
      char_idx = (int)*it;

      if(char_idx == (int)' ')
      {
        x_advance += _font->_space_size;
        continue;
      }
      if(char_idx == (int)'\n')
      {
        y_advance += _font->_line_height;
        x_advance = 0;
        continue;
      }
      if(!_font->_glyphs.count(char_idx))
        char_idx = -1;

      info = _font->_glyphs[char_idx];

      left = (float)(x_advance + info->x_offset) / _view_height + min_x;
      right = left + (float)info->w_texi / _view_height;
      top = (float)(y_advance + info->y_offset) / _view_height + min_y;
      bottom = top + (float)info->h_texi / _view_height;

      vert_p[0] = vert_p[4] = left < max_x? left : max_x;
      vert_p[8] = vert_p[12] = right < max_x? right : max_x;
      vert_p[1] = vert_p[13] = top < max_y? top : max_y;
      vert_p[5] = vert_p[9] = bottom < max_y? bottom : max_y;

      x_cutoff = (vert_p[8] - vert_p[0]) / (right - left);
      y_cutoff = (vert_p[5] - vert_p[1]) / (bottom - top);

      vert_p[2] = vert_p[6] = info->x_texf;
      vert_p[10] = vert_p[14] = info->x_texf + info->w_texf * x_cutoff;
      vert_p[3] = vert_p[15] = info->y_texf;
      vert_p[7] = vert_p[11] = info->y_texf + info->h_texf * y_cutoff;

      vert_p += 16;
      x_advance += info->advance;
    }
  }

  void TextComponent::draw()
  {
    /*
      text clips using a stencil technique in the shader
      clipping area is passed in the overlay color uniform
      keep in mind that cordinates go from top to bottom but
      in opengl they go from bottom to top
    */
    double l, r, t, b;
    _mother->_getDrawingBox(&l, &r, &t, &b);
    l *= _view_height;
    r *= _view_height;
    t *= -_view_height;
    b *= -_view_height;
    t += (double)_view_height;
    b += (double)_view_height;

    if(_verts != nullptr)
      h3dShowOverlays(_verts, 4 * _num_chars,
                      l - 1.0, r - 1.0, b + 1.0, t + 1.0,
                      _font->_material, 0);
  }

  void TextComponent::_allocate()
  {
    if(_text.length() == 0)
    {
      delete[] _verts;
      _verts = nullptr;
      _num_chars = 0;
    }

    uint32_t n = 0;

    for(std::string::iterator it = _text.begin(); it != _text.end(); ++it)
    {
      if(*it == ' ' || *it == '\n')
        continue;
      else ++n;
    }

    if(n != _num_chars)
    {
      delete[] _verts;
      _verts = new float[16 * n];
    }

    _num_chars = n;
  }

  int TextComponent::_handleMouseEvent(SDL_Event& event, int data)
  {return _mother->_handleMouseEvent(event, data);}

  void TextComponent::setText(const char* text)
  {
    _text = text;
    _allocate();
    _updatePos();
  }

  TextComponent::TextComponent(Font* f, const char* t, const Pos& pos):
  Component(pos)
  {
    _num_chars = 0;
    _verts = nullptr;
    _font = f;

    if(t != nullptr)
    {
      _text = t;
      _allocate();
    }else _text = "";
  }

  TextComponent::TextComponent
  (Font* f, const Pos& pos): Component(pos), _font(f){}

  TextComponent::~TextComponent()
  {
    delete[] _verts;
  }

  /***other functions***/
  Component* init(uint16_t w, uint16_t h)
  {
    for(int n = 0; n < _component_stack_size; ++n)
      Component::_component_stack[n] = nullptr;
    Component::_component_stack_ref = Component::_component_stack;

    _view_width = w;
    _view_height = h;
    _view_aspect = (float)w / h;

    //_screen = new ContainerWidget(nullptr, flags_none, Pos());
    _screen = Utils::makeRefCounted<ContainerWidget>
    (nullptr, flags_none, Pos());

    _screen->_x_pos = _screen->_y_pos = 0.0;
    _screen->_width = _view_aspect;
    _screen->_height = 1.0;
    _screen->_mother = nullptr;

    return _screen.get();
  }

  void deinit()
  {
    _screen = nullptr;

    for(std::map<std::string, GUI::Style*>::iterator it = _styles.begin();
      it != _styles.end(); ++it)
      delete it->second;
    _styles.clear();
    for(std::map<std::string, GUI::Font*>::iterator it = _fonts.begin();
      it != _fonts.end(); ++it)
      delete it->second;
    _fonts.clear();

    h3dClearOverlays();

    _allocator_verts_small.deallocate();
    _allocator_verts_large.deallocate();
  }

  //misc functions
  Component* getScreenPointer()
  {return _screen.get();}

  //loading functions
  int loadFont(const char* filename)
  {
    JSonParser json;
    std::string material_name;

    if(json.openFile(filename))
      return 1;

    //read json
    int fault = 1;

    Font* font = new Font;

    do
    {
      if(json.openTable("font"))
      {
        material_name = json.getElementS("material");
        font->_material = h3dFindResource(H3DResTypes::Material,
                                          material_name.c_str());
        if(font->_material == 0)
          break;

        if(json.openTable("info"))
        {
          font->_space_size = json.getElementI("size") / 2;

          if(json.getError())
            break;

          json.back();
        }else break;
        if(json.openTable("common"))
        {
          font->_width = json.getElementI("scaleW");
          font->_height = json.getElementI("scaleH");
          font->_line_height = json.getElementI("lineHeight");
          font->_base = json.getElementI("base");

          if(json.getError())
            break;

          json.back();
        }else break;

        if(json.openTable("chars"))
        {
          if(json.openArray("char"))
          {
            json.beginIteration();

            while(json.openNextTable())
            {
              int id = json.getElementI("id");

              font->_glyphs[id] = new Font::_CharInfo;

              font->_glyphs[id]->x_texi = 
                  json.getElementI("x");
              font->_glyphs[id]->y_texi = 
                  json.getElementI("y");
              font->_glyphs[id]->w_texi = 
                  json.getElementI("width");
              font->_glyphs[id]->h_texi = 
                  json.getElementI("height");

              font->_glyphs[id]->x_texf = 
                  (float)font->_glyphs[id]->x_texi / font->_width;
              font->_glyphs[id]->y_texf = 
                  (float)font->_glyphs[id]->y_texi / font->_height;
              font->_glyphs[id]->w_texf = 
                  (float)font->_glyphs[id]->w_texi / font->_width;
              font->_glyphs[id]->h_texf = 
                  (float)font->_glyphs[id]->h_texi / font->_height;

              font->_glyphs[id]->x_offset = 
                  json.getElementI("xoffset");
              font->_glyphs[id]->y_offset = 
                  json.getElementI("yoffset");
              font->_glyphs[id]->advance =  
                  json.getElementI("xadvance");

              if(json.getError())
                break;

              json.continueIteration();
            }

            json.endIteration();

            json.back();
          }else break;

          json.back();
        }else break;

        json.back();
      }else break;

      fault = 0;
    }while(false);

    if(fault)
    {
      delete font;
      return fault;
    }

    if(_fonts.count(material_name) > 0)
      delete _fonts[material_name];
    _fonts[material_name] = font;

    return 0;
  }

  int loadStyle(const char* filename)
  {
    JSonParser json;
    std::string style_class = "";
    std::string style_name = "";
    std::string full_key = "";
    Style* style = nullptr;
    Font* default_font = nullptr;
    Font* spec_font = nullptr;

    H3DRes c_material;
    uint16_t c_tex_width, c_tex_height;

    struct{uint16_t left, right, top, bottom;}
      _body({0, 0, 0, 0}),
      _frame({0, 0, 0, 0});

    if(json.openFile(filename) != 0)
      return 1;

    c_material = h3dFindResource(H3DResTypes::Material,
        json.getElementS("material").c_str());

    if(c_material == 0)
      return 2;

    json.clearError();
    style_class = json.getElementS("class");
    if(json.getError())
      return 3;

    full_key = json.getElementS("font");
    if(_fonts.count(full_key) > 0)
      default_font = _fonts[full_key];

    //get texture size
    int elem = h3dFindResElem(c_material, H3DMatRes::SamplerElem,
                                H3DMatRes::SampNameStr, "albedoMap");

    if(elem != -1)
    {
      int res = h3dGetResParamI(c_material, H3DMatRes::SamplerElem,
                                  elem, H3DMatRes::SampTexResI);
      c_tex_width = h3dGetResParamI(res, H3DTexRes::ImageElem, 0,
                                      H3DTexRes::ImgWidthI);
      c_tex_height = h3dGetResParamI(res, H3DTexRes::ImageElem, 0,
                                      H3DTexRes::ImgHeightI);
    }
    else
      return 4;

    for(json.beginIteration(); json.openNextTable(&style_name);
      json.continueIteration())
    {
      if(style_name.compare("material") == 0 
        || style_name.compare("class") == 0)
        continue;

      style = new Style;

      //override font
      full_key = json.getElementS("font");
      if(_fonts.count(full_key) > 0)
        spec_font = _fonts[full_key];
      else spec_font = default_font;
      style->font = spec_font;

      style->material = c_material;
      style->tex_width = c_tex_width;
      style->tex_height = c_tex_height;

      style->wrap = json.getElementI("wrap");
      style->resolution = json.getElementI("resolution");
      style->resolution = style->resolution > 0? style->resolution : 1;

      //get body bounds
      if(json.openTable("body"))
      {
        _body.left = json.getElementI("left");
        _body.right = json.getElementI("right");
        _body.top = json.getElementI("top");
        _body.bottom = json.getElementI("bottom");

        style->body.left =      (float)_body.left     / c_tex_width;
        style->body.right =     (float)_body.right    / c_tex_width;
        style->body.top =       (float)_body.top      / c_tex_height;
        style->body.bottom =    (float)_body.bottom   / c_tex_height;

        style->body.px_width = _body.right - _body.left;
        style->body.px_height = _body.bottom - _body.top;

        json.back();
      }

      //get frame bounds
      if(json.openTable("frame"))
      {
        _frame.left = json.getElementI("left");
        _frame.right = json.getElementI("right");
        _frame.top = json.getElementI("top");
        _frame.bottom = json.getElementI("bottom");

        style->frame.left =      (float)_frame.left     / c_tex_width;
        style->frame.right =     (float)_frame.right    / c_tex_width;
        style->frame.top =       (float)_frame.top      / c_tex_height;
        style->frame.bottom =    (float)_frame.bottom   / c_tex_height;

        style->frame.px_left = (_body.left - _frame.left);
        style->frame.px_right = (_frame.right - _body.right);
        style->frame.px_top = (_body.top - _frame.top);
        style->frame.px_bottom = (_frame.bottom - _body.bottom);

        json.back();
      }
      else
      {
        style->frame.px_left = 0;
        style->frame.px_right = 0;
        style->frame.px_top = 0;
        style->frame.px_bottom = 0;
      }

      //state shifts
      if(json.openTable("active_state"))
      {
        style->active.x_shift = 
          (float)json.getElementI("x_shift") / c_tex_width;
        style->active.y_shift = 
          (float)json.getElementI("y_shift") / c_tex_height;

        //if no excited_active_state is found regular active state is used
        style->excited_active.x_shift = style->active.x_shift;
        style->excited_active.y_shift = style->active.y_shift;

        json.back();
      }
      if(json.openTable("excited_state"))
      {
        style->excited.x_shift = 
          (float)json.getElementI("x_shift") / c_tex_width;
        style->excited.y_shift = 
          (float)json.getElementI("y_shift") / c_tex_width;

        json.back();
      }
      if(json.openTable("excited_active_state"))
      {
        style->excited_active.x_shift = 
          (float)json.getElementI("x_shift") / c_tex_width;
        style->excited_active.y_shift = 
          (float)json.getElementI("y_shift") / c_tex_height;

        json.back();
      }

      full_key = style_class;
      full_key += '.';
      full_key += style_name;

      if(_styles.count(full_key) > 0)
        delete _styles[full_key];
      _styles[full_key] = style;
    }
    json.endIteration();

    return 0;
  }

  Style* findStyle(const char* name)
  {
    Style* style = nullptr;

    if(_styles.count(name) > 0)
      style = _styles[name];
    else if(std::strlen(name) > 1) printf("Style: \"%s\" not found\n", name);

    return style;
  }

  Font* findFont(const char* name)
  {
    Font* font = nullptr;

    if(_fonts.count(name) > 0)
      font = _fonts[name];
    else if(std::strlen(name) > 1) printf("Style: \"%s\" not found\n", name);

    return font;
  }

  bool trapEvent(SDL_Event& event)
  {
    static Utils::counted_weak_ptr<Component> old_component;
    static int grabbed = 0;
    static int x = 0.0;
    static int y = 0.0;

    if(!_screen.get())
      return false;

    //Utils::counted_ptr<Component> component;
    Component* component;

    if(old_component.expired())
    {
      //std::printf("expired: %p\n", old_component.get());
      component = _screen;
    }
    else component = old_component.get();

    if(event.type == SDL_MOUSEMOTION && grabbed == 0)
    {
      //std::printf("old_c: %p\n", component.get());
      Component* temp_component = component;

      x = event.motion.x;
      y = event.motion.y;
      component = _screen->_getComponent
        ((float)x / _view_height, (float)y / _view_height);
      //std::printf("%p\n", component.get());
      //Component* temp_component = _screen->_getComponent((float)x 
      //    / _view_height, (float)y / _view_height);

      if(component != temp_component)
      {
        if(temp_component) temp_component->_handleMouseEvent(event, 0);
        old_component = component;
      }
      //std::printf("c: %p, t: %p\n", component, temp_component);
    }
    if(component == (Component*)_screen)
      return false;

    switch(event.type)
    {
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      grabbed = component->_handleMouseEvent(event, grabbed);
      break;

      default: break;
    }

    return true;
  }
}
