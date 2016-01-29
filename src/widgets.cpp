
#include <SDL.h>

#include <cstdio>

#include "gui.h"

extern uint16_t _view_height;
extern uint16_t _view_width;
extern float _view_aspect;

namespace GUI
{
  ///Window
  int Window::_handleMouseEvent(SDL_Event& event, int x_data)
  {
    int side = 0;

    union{float x; int xi;};
    union{float y; int yi;};

    static int prev_drag_x = 0;
    static int prev_drag_y = 0;

    switch(event.type)
    {
      case SDL_MOUSEBUTTONDOWN:
      {
        if(event.button.button == SDL_BUTTON_LEFT)
        {
          double l, r, t, b;
          _getBoundingBox(&l, &r, &t, &b);

          l += (float)4 / _view_height;
          r -= (float)4 / _view_height;
          t += (float)4 / _view_height;
          b -= (float)4 / _view_height;

          x = (float)event.button.x / _view_height;
          y = (float)event.button.y / _view_height;

          if(x < l && _flags & flags_resizable_left)
            side += frame_left;
          if(x > r && _flags & flags_resizable_right)
            side += frame_right;
          if(y < t && _flags & flags_resizable_top)
            side += frame_top;
          if(y > b && _flags & flags_resizable_bottom)
            side += frame_bottom;
          if(side == 0 && _flags & flags_dragable)
            side = frame_none;

          if(side != 0)
            return side;
          else return 0;
        }
      }
      return x_data;

      case SDL_MOUSEBUTTONUP:
      prev_drag_x = 0;
      prev_drag_y = 0;
      return 0;

      case SDL_MOUSEMOTION:
      {
        if(x_data != 0)
        {
          Window* target;

          if(this->_flags & flags_surrogate)
            target = (Window*)_mother;
          else target = this;

          xi = event.motion.xrel + prev_drag_x;
          yi = event.motion.yrel + prev_drag_y;

          if(x_data & frame_none)
          {
            target->_p1.x_abs += xi;
            target->_p2.x_abs += xi;
            target->_p1.y_abs += yi;
            target->_p2.y_abs += yi;
          }
          else
          {
            if(x_data & frame_left)
              target->_p1.x_abs += xi;
            else if(x_data & frame_right)
              target->_p2.x_abs += xi;
            if(x_data & frame_top)
              target->_p1.y_abs += yi;
            else if(x_data & frame_bottom)
              target->_p2.y_abs += yi;

            if(target->_min_w != 0 &&
              (target->_p1.x_abs + target->_min_w)
              > target->_p2.x_abs)
            {
              if(x_data & frame_left)
              {
                prev_drag_x = target->_p1.x_abs
                - target->_p2.x_abs + target->_min_w;
                target->_p1.x_abs -= prev_drag_x;
              }
              else
              {
                prev_drag_x = target->_p2.x_abs
                - target->_p1.x_abs - target->_min_w;
                target->_p2.x_abs -= prev_drag_x;
              }
            }
            else prev_drag_x = 0;
            if(target->_min_h != 0 &&
              (target->_p1.y_abs + target->_min_h)
              > target->_p2.y_abs)
            {
              if(x_data & frame_top)
              {
                prev_drag_y = target->_p1.y_abs
                - target->_p2.y_abs + target->_min_h;
                target->_p1.y_abs -= prev_drag_y;
              }
              else
              {
                prev_drag_y = target->_p2.y_abs
                - target->_p1.y_abs - target->_min_h;
                target->_p2.y_abs -= prev_drag_y;
              }
            }
            else prev_drag_y = 0;
          }

          target->_updatePos();
        }
      }
      return x_data;

      default:
      return x_data;
    }
  }

  void Window::setMinimumSize(uint32_t w, uint32_t h)
  {
    _min_w = w;
    _min_h = h;
  }

  ///button
  Component** Button::_stackSubComponents()
  {
    switch(_flags & (flags_text | flags_icon))
    {
      case flags_text:
      ++_component_stack_ref;
      *_component_stack_ref = _text.get();
      break;
      case flags_icon:
      ++_component_stack_ref;
      *_component_stack_ref = _icon.get();
      default:
      break;
    }

    return _component_stack_ref;
  }

  int Button::_handleMouseEvent(SDL_Event& event, int x_data)
  {
    double x;
    double y;

    int return_val = 0;
    uint32_t old_state = _state;

    switch(event.type)
    {
      case SDL_MOUSEBUTTONDOWN:
      {
        if(event.button.button != SDL_BUTTON_LEFT)
          return x_data;

        x = (double)event.button.x / _view_height;
        y = (double)event.button.y / _view_height;

        if(_sides)
        {
          double l, r, t, b;
          _getBoundingBox(&l, &r, &t, &b);

          if(_sides & frame_left)     l += (float)4 / _view_height;
          if(_sides & frame_right)    r -= (float)4 / _view_height;
          if(_sides & frame_top)      t += (float)4 / _view_height;
          if(_sides & frame_bottom)   b -= (float)4 / _view_height;

          if(x > l && x < r && y > t && y < b)
          {
            _state = state_excited_active;
            if(_press_callback) _press_callback((void*)&_signature);
            return_val = 1; break;
          }
          return_val = 0; break;
        }
        else
        {
          _state = state_excited_active;
          if(_press_callback) _press_callback((void*)&_signature);
          return_val = 1; break;
        }
      }
      break;

      case SDL_MOUSEBUTTONUP:
      {
        if(event.button.button != SDL_BUTTON_LEFT)
          return x_data;
        if(_state != state_excited_active)
          return x_data;

        x = (double)event.motion.x / _view_height;
        y = (double)event.motion.y / _view_height;

        double l, r, t, b;
        _getBoundingBox(&l, &r, &t, &b);

        if(x > l && x < r && y > t && y < b)
        {
          if(_flags & flags_toggle)
          {
            if(_flags & flags_active)
            {
              _flags &= ~flags_active;
              _state = state_excited;
              if(_release_callback)
                _release_callback((void*)&_signature);
            }
            else
            {
              _state = state_active;
              _flags |= flags_active;
              if(_activate_callback)
              {
                if(_flags & flags_surrogate)
                  _mother->_handleSelectionEvent(this);
                else _activate_callback((void*)&_signature);
              }
              else if(_release_callback) 
                _release_callback((void*)&_signature);
            }
          }
          else if(_flags & flags_hold)
          {
            if(_flags & flags_active)
            {
              _state = state_active;
              //if(_release_callback) 
              //  _release_callback((void*)&_signature); Maybe??
            }
            else
            {
              _state = state_active;
              _flags |= flags_active;
              if(_flags & flags_surrogate) 
                _mother->_handleSelectionEvent(this);
              else if(_activate_callback)  
                _activate_callback((void*)&_signature);
              else if(_release_callback)   
                _release_callback((void*)&_signature);
            }
          }
          else
          {
            _state = state_excited;
            if(_release_callback)
              _release_callback((void*)&_signature);
          }
        }
        else
        {
          if(_flags & flags_active) _state = state_active;
          else _state = state_normal;
        }
        return_val = 0; break;
      }
      break;

      case SDL_MOUSEMOTION:
      {
        x = (double)event.motion.x / _view_height;
        y = (double)event.motion.y / _view_height;

        double l, r, t, b;
        _getBoundingBox(&l, &r, &t, &b);

        if(x > l && x < r && y > t && y < b)
        {
          if(x_data)
          {
            _state = state_excited_active;
            return_val = 1; break;
          }
          else
          {
            if(_flags & flags_active)
              _state = state_active;
            else _state = state_excited;
          }
        }
        else
        {
          if(x_data)
          {
            if(_flags & flags_active)
              _state = state_active;
            else _state = state_normal;
            return_val = 1; break;
          }
          else
          {
            if(_flags & flags_active)
              _state = state_active;
            else _state = state_normal;
          }
        }
        return_val = 0; break;
      }
      break;

      default:
      break;
    }

    if(_flags & (flags_icon | flags_text))
    {
      if(0x01 & (_state ^ old_state))
      {
        Component* label;
        if(_flags & flags_icon) label = (Component*)_icon;
        else label = (Component*)_text;

        if(_state & 0x01)
        {
          ++label->_p1.x_abs;
          ++label->_p1.y_abs;
          ++label->_p2.x_abs;
          ++label->_p2.y_abs;
        }
        else
        {
          --label->_p1.x_abs;
          --label->_p1.y_abs;
          --label->_p2.x_abs;
          --label->_p2.y_abs;
        }

        label->_updatePos();
      }
    }

    return return_val;
  }

  //setters
  void Button::setPressCallback(Callback callback)
  {_press_callback = callback;}
  void Button::setReleaseCallback(Callback callback)
  {_release_callback = callback;}
  void Button::setActivateCallback(Callback callback)
  {_activate_callback = callback;}

  void Button::setSignature(int signature)
  {_signature = signature;}

  void Button::setText(const char* text)
  {
    switch(_flags & (flags_text | flags_icon))
    {
      case flags_text:
      _text->setText(text);
      break;
      case flags_icon:
      _icon = nullptr;
      default:
      _text = Utils::makeRefCounted<TextComponent>
      (_style->getFont(), text, Pos(2, 2, -2, -2));

      _text->_mother = this;
      ((Component*)_text)->_updatePos();

      break;
    }

    _flags &= ~flags_icon;
    _flags |= flags_text;
  }

  void Button::setIcon(Style* style)
  {
    switch(_flags & (flags_text | flags_icon))
    {
      case flags_text:
      _text = nullptr;
      break;
      case flags_icon:
      _icon = nullptr;
      break;
      default:
      break;
    }

    _flags &= ~flags_text;
    _flags |= flags_icon;

    _icon = Utils::makeRefCounted<Icon>(style, Pos(6, 6, -6, -6));
    _icon->_mother = this;

    ((Component*)_icon)->_updatePos();
  }

  void Button::setState(bool depress)
  {
    if((bool)(_flags & flags_active) == depress)
      return;

    Component* label;

    if(_flags & flags_icon) label = (Component*)_icon;
    else if(_flags & flags_text) label = (Component*)_text;
    else label = nullptr;

    if(depress)
    {
      _flags |= flags_active;
      _state = state_active;

      if(label)
      {
        ++label->_p1.x_abs;
        ++label->_p1.y_abs;
        ++label->_p2.x_abs;
        ++label->_p2.y_abs;
        label->_updatePos();
      }
    }
    else
    {
      _flags &= ~flags_active;
      _state = state_normal;

      if(label)
      {
        --label->_p1.x_abs;
        --label->_p1.y_abs;
        --label->_p2.x_abs;
        --label->_p2.y_abs;
        label->_updatePos();
      }
    }
  }

  ///GridContainer superclass
  //protected members
  void GridContainer::_updateSubComponentPositioning()
  {
    uint16_t x_elements = (int)(this->_width * _view_height) / _elem_width;
    uint16_t num_elems;

    if(_flags & flags_autoresize)
    {
      if(x_elements == 0) _p2.y_abs = _p1.y_abs + _elem_height;
      else _p2.y_abs = _p1.y_abs + _elem_height * 
        ((_contents.size() + x_elements - 1) / x_elements);

      if(_flags & flags_surrogate) _mother->_updatePos();
      _updateRelativePos();

      num_elems = _contents.size();
    }else num_elems = x_elements * ((int)(this->_height * _view_height)
                                / _elem_height);

    uint32_t n = 0;
    std::vector<Utils::counted_ptr<Component>>::iterator it = 
    _contents.begin();

    while(it != _contents.end())
    {
      if(n < num_elems)
        (*it)->_setPos(0.0, (n % x_elements) * _elem_width,
                        0.0, (n / x_elements) * _elem_height,
                        0.0, ((n % x_elements) + 1) * _elem_width,
                        0.0, ((n / x_elements) + 1) * _elem_height);
      else (*it)->_setPos(0.0, -20, 0.0, -20, 0.0, 0, 0.0, 0);

      ++n; ++it;
    }
  }

  void GridContainer::_updatePos()
  {
    if(_flags & flags_updating) return;
    _flags |= flags_updating;

    _updateRelativePos();
    _updateSubComponentPositioning();
    if(_verts != nullptr) _updateVerts();
    _updateContents();

    _flags &= ~flags_updating;
  }

  Component** GridContainer::_stackSubComponents()
  {
    for(auto it: _contents)
    {
      ++_component_stack_ref;
      *_component_stack_ref = it.get();
    }

    return _component_stack_ref;
  }

  //public methods
  void GridContainer::setElementSize(uint32_t w, uint32_t h)
  {
    _elem_width = w;
    _elem_height = h;
  }


  ///SelectorGrid
  //protected methods
  void SelectorGrid::_handleSelectionEvent(Component* component)
  {
    if(_current_selected)
      _current_selected->setState(false);
    _current_selected = (Button*)component;
    if(_callback) _callback((void*)&_current_selected->_signature);
  }

  //public methods
  Button* SelectorGrid::newButton(uint32_t signature)
  {
    Button* button = Utils::makeRefCounted<Button>
                    (_button_style, flags_hold | flags_surrogate, Pos());
    button->_signature = signature;
    button->_mother = this;
    _contents.push_back(button);
    _updateSubComponentPositioning();
    button->_updatePos();
    return button;
  }

  Button* SelectorGrid::newButton(uint32_t signature, Style* icon)
  {
    Button* button = Utils::makeRefCounted<Button>
                    (_button_style, flags_hold | flags_surrogate,
                        icon, Pos());
    button->_signature = signature;
    button->_mother = this;
    _contents.push_back(button);
    _updateSubComponentPositioning();
    button->_updatePos();
    return button;
  }

  Button* SelectorGrid::newButton(uint32_t signature, const char* text)
  {
    Button* button = Utils::makeRefCounted<Button>
    (_button_style, flags_hold | flags_surrogate, text, Pos());
    button->_signature = signature;
    button->_mother = this;
    _contents.push_back(button);
    _updateSubComponentPositioning();
    button->_updatePos();
    return button;
  }

  //setters/getters
  void SelectorGrid::setCallback(Callback callback)
  {_callback = callback;}

  void SelectorGrid::setSelection(int32_t selection)
  {
    Button* button;
    for(auto it: _contents)
    {
      button = (Button*)it;
      if(button->_signature == selection)
      {
        if(_current_selected)
            _current_selected->setState(false);
        _current_selected = button;
        _current_selected->setState(true);
        break;
      }
    }
  }

  void SelectorGrid::deselect()
  {
    if(_current_selected)
      _current_selected->setState(false);
    _current_selected = nullptr;
  }

  ///scroll Area
  //main
  ScrollArea::ScrollArea
  (Style* style, Style* b_style, Style* s_style, Style* sa_style,
      uint32_t flags, uint32_t s_size, const Pos& pos)
  : Widget(style, flags, pos)
  {
    //int x_indent = flags & flags_vertical?      -s_size : 0;
    //int y_indent = flags & flags_horizontal?    -s_size : 0;

    _scinfo.scroll_size = s_size;
    _scinfo.scroll_status = 0;
    _scinfo.scroll.x = 0;
    _scinfo.scroll.y = 0;

    if((flags & (flags_horizontal | flags_vertical)) == 0)
      _flags |= flags_horizontal | flags_vertical;

    //body
    _body = Utils::makeRefCounted<__SA_Body>
    (b_style, &_scinfo, _flags, Pos(0.0f, 0.0f, 1.0f, 1.0f));

    _body->_mother = this;
    _body->_updatePos();

    //scroll bars
    //horizontal
    if(_flags & flags_horizontal)
    {
      _scroll_bar_x = Utils::makeRefCounted<__ScrollBar>
      (s_style, sa_style, &_scinfo, flags_horizontal,
      Pos(0., 0, 1., -s_size, 1., 0, 1., 0));

      _scroll_bar_x->_mother = this;
      _scroll_bar_x->_updatePos();
    }else _scroll_bar_x = nullptr;

    //vertical
    if(_flags & flags_vertical)
    {
      _scroll_bar_y = Utils::makeRefCounted<__ScrollBar>
      (s_style, sa_style, &_scinfo, flags_vertical,
      Pos(1., -s_size, 0., 0, 1., 0, 1., 0));

      _scroll_bar_y->_mother = this;
      _scroll_bar_y->_updatePos();
    }else _scroll_bar_y = nullptr;
  }

  Component** ScrollArea::_stackSubComponents()
  {
    if(_flags & flags_horizontal)
    {
      ++_component_stack_ref;
      *_component_stack_ref = _scroll_bar_x;
    }
    if(_flags & flags_vertical)
    {
      ++_component_stack_ref;
      *_component_stack_ref = _scroll_bar_y;
    }

    ++_component_stack_ref;
    *_component_stack_ref = _body;

    return _component_stack_ref;
  }

  //tracking methods
  void ScrollArea::_trackVerticScrollPosition(int32_t y)
  {
    //int scy = _scinfo.scroll.y;
    int new_scroll = (y * _scinfo.virtual_size.y) / _scinfo.viewport_size.y;
    _body->_setScroll(0, new_scroll - _scinfo.scroll.y);
    //_updateContents();
    _body->_updatePos();
  }

  void ScrollArea::_trackHorizScrollPosition(int32_t x)
  {
    //int scy = _scinfo.scroll.y;
    int new_scroll = (x * _scinfo.virtual_size.x) / _scinfo.viewport_size.x;
    _body->_setScroll(new_scroll - _scinfo.scroll.x, 0);
    //_updateContents();
    _body->_updatePos();
  }

  //__SA_Body
  Component** ScrollArea::__SA_Body::_stackSubComponents()
  {
    if(_content != nullptr)
    {
      ++_component_stack_ref;
      *_component_stack_ref = _content;
    }

    return _component_stack_ref;
  }

  void ScrollArea::__SA_Body::_setScroll(int x, int y)
  {
    if(((signed)_scinfo->scroll.x + x) < 0) x = -_scinfo->scroll.x;
    if(((signed)_scinfo->scroll.y + y) < 0) y = -_scinfo->scroll.y;
    if(x % 2) ++x;
    if(y % 2) ++y;

    _scinfo->scroll.x += x;
    _scinfo->scroll.y += y;

    _content->_p1.x_abs -= x;
    _content->_p1.y_abs -= y;
    _content->_p2.x_abs -= x;
    _content->_p2.y_abs -= y;
  }

  void ScrollArea::__SA_Body::__updateVPVA()
  {
    _scinfo->viewport_size.x = (int)(_width * _view_height);
    _scinfo->viewport_size.y = (int)(_height * _view_height);
    if(_content)
    {
      _scinfo->virtual_size.x = (int)(_content->_width * _view_height);
      _scinfo->virtual_size.y = (int)(_content->_height * _view_height);
    }
    else
    {
      _scinfo->virtual_size.x = 0;
      _scinfo->virtual_size.y = 0;
    }
  }

  void ScrollArea::__SA_Body::_updateScrollBars()
  {
    __updateVPVA();


    uint32_t new_status = 0;
    if(_scinfo->viewport_size.x < _scinfo->virtual_size.x
      && (_flags & flags_horizontal))
    {
      this->_p2.y_abs = -_scinfo->scroll_size;
      _scinfo->viewport_size.y -= _scinfo->scroll_size;
      new_status |= flags_horizontal;
      if(_scinfo->viewport_size.y < _scinfo->virtual_size.y
        && (_flags & flags_vertical))
      {
        this->_p2.x_abs = -_scinfo->scroll_size;
        _scinfo->viewport_size.x -= _scinfo->scroll_size;
        new_status |= flags_vertical;
      }
    }
    else if(_scinfo->viewport_size.y < _scinfo->virtual_size.y
      && (_flags & flags_vertical))
    {
      this->_p2.x_abs = -_scinfo->scroll_size;
      _scinfo->viewport_size.x -= _scinfo->scroll_size;
      new_status |= flags_vertical;
      if(_scinfo->viewport_size.x < _scinfo->virtual_size.x
        && (_flags & flags_horizontal))
      {
        this->_p2.y_abs = -_scinfo->scroll_size;
        _scinfo->viewport_size.y -= _scinfo->scroll_size;
        new_status |= flags_horizontal;
      }
    }
    _scinfo->scroll_status = new_status;
    if(!(new_status & flags_horizontal) && _scinfo->scroll.x != 0)
    {
      static_cast<ScrollArea*>(_mother)->_trackHorizScrollPosition(0);
      return;
    }
    if(!(new_status & flags_vertical) && _scinfo->scroll.y != 0)
    {
      static_cast<ScrollArea*>(_mother)->_trackVerticScrollPosition(0);
      return;
    }
  }

  void ScrollArea::__SA_Body::_updateScrollInfo()
  {
    __updateVPVA();

    if(_content  != nullptr)
    {
      if(_scinfo->scroll.x != 0 && _scinfo->scroll.x 
        > (_scinfo->virtual_size.x - _scinfo->viewport_size.x))
        _setScroll(_scinfo->virtual_size.x
          - (_scinfo->viewport_size.x + _scinfo->scroll.x), 0);
      if(_scinfo->scroll.y != 0 && _scinfo->scroll.y
        > (_scinfo->virtual_size.y - _scinfo->viewport_size.y))
        _setScroll(0, _scinfo->virtual_size.y
          - (_scinfo->viewport_size.y + _scinfo->scroll.y));
    }
  }


  void ScrollArea::__SA_Body::_updatePos()
  {
    _p2.x_abs = 0;
    _p2.y_abs = 0;

    if(_content == nullptr)
    {
      _updateRelativePos();
      _updateScrollInfo();
      _updateRelativePos();
    }
    else
    {
      _updateRelativePos();
      _content->_updatePos();
      _updateScrollBars();
      _updateRelativePos();
      _content->_updatePos();
      _updateScrollInfo();
    }
  }

  //__ScrollBar
  ScrollArea::__ScrollBar::__ScrollBar
  (Style* style, Style* a_style, ScrollArea::__ScrollInfo* sc,
    uint32_t flags, const Pos& pos): Widget(style, flags, pos)
  {
    _scinfo = sc;

    _sb_aide = Utils::makeRefCounted<__SB_Aide>
    (a_style, flags, Pos(0.f, 0.f, 1.f, 1.f));

    _sb_aide->_mother = this;
    _sb_aide->_updatePos();
  }

  Component** ScrollArea::__ScrollBar::_stackSubComponents()
  {
    _component_stack_ref++;
    *_component_stack_ref = this->_sb_aide;

    return _component_stack_ref;
  }

  void ScrollArea::__ScrollBar::_updatePos()
  {
    //positioning & invisible flag
    if(_flags & flags_vertical)
    {
      if(_scinfo->scroll_status & flags_vertical)
      {
        _flags &= ~flags_invisible;
        if(_scinfo->scroll_status & flags_horizontal)
          _p2.y_abs = -_scinfo->scroll_size;
        else _p2.y_abs = 0;

        //_sb_aide position
        //warning: horribly long statement
        _sb_aide->_p1.y_abs =
        (_scinfo->scroll.y * _scinfo->viewport_size.y)
        / _scinfo->virtual_size.y;
        _sb_aide->_p2.y_abs =
        ((signed)_scinfo->viewport_size.y *
          (signed)((_scinfo->scroll.y + _scinfo->viewport_size.y)
            - _scinfo->virtual_size.y))
          / (signed)_scinfo->virtual_size.y;
        //old stuff
        //_sb_aide->_p1.y_rel = (float)_scinfo->scroll.y
        // / _scinfo->virtual_size.y;
        //_sb_aide->_p2.y_rel = (float)(_scinfo->scroll.y 
        // + _scinfo->viewport_size.y) / _scinfo->virtual_size.y;
      }
      else _flags |= flags_invisible;
    }
    else
    {
      if(_scinfo->scroll_status & flags_horizontal)
      {
        _flags &= ~flags_invisible;
        if(_scinfo->scroll_status & flags_vertical)
          _p2.x_abs = -_scinfo->scroll_size;
        else _p2.x_abs = 0;

        //_sb_aide positioning
        _sb_aide->_p1.x_abs =
        (_scinfo->scroll.x * _scinfo->viewport_size.x)
        / _scinfo->virtual_size.x;
        _sb_aide->_p2.x_abs =
        ((signed)_scinfo->viewport_size.x *
          (signed)((_scinfo->scroll.x + _scinfo->viewport_size.x)
            - _scinfo->virtual_size.x))
          / (signed)_scinfo->virtual_size.x;
        }
      else _flags |= flags_invisible;
    }

    //finalization
    _updateRelativePos();
    if(_verts != nullptr)_updateVerts();
    _sb_aide->_updatePos();
  }

  //__SB_Aide
  int ScrollArea::__ScrollBar::__SB_Aide::_handleMouseEvent(
                                  SDL_Event& event, int x)
  {
    switch(event.type)
    {
      case SDL_MOUSEBUTTONDOWN:
      if(event.button.button == SDL_BUTTON_LEFT)
        return frame_none;
      return x;

      case SDL_MOUSEBUTTONUP:
      return 0;

      case SDL_MOUSEMOTION:

      if(x != 0)
      {
        if(_flags & flags_horizontal)
        {
          _p1.x_abs += event.motion.xrel;
          _p2.x_abs += event.motion.xrel;

          if(_p1.x_abs < 0)
          {
            _p2.x_abs -= _p1.x_abs;
            _p1.x_abs = 0;
          }
          else if(_p2.x_abs > 0)
          {
            _p1.x_abs -= _p2.x_abs;
            _p2.x_abs = 0;
          }

          static_cast<ScrollArea*>
          (_mother->_mother)->_trackHorizScrollPosition(_p1.x_abs);
        }
        else
        {
          _p1.y_abs += event.motion.yrel;
          _p2.y_abs += event.motion.yrel;

          if(_p1.y_abs < 0)
          {
            _p2.y_abs -= _p1.y_abs;
            _p1.y_abs = 0;
          }
          else if(_p2.y_abs > 0)
          {
            _p1.y_abs -= _p2.y_abs;
            _p2.y_abs = 0;
          }

          static_cast<ScrollArea*>
          (_mother->_mother)->_trackVerticScrollPosition(_p1.y_abs);
        }

        _updatePos();
      }

      return x;

      default:
      return x;
    }
  }

  ///OrderedContainer

  Component** OrderedContainer::_stackSubComponents()
  {
    for(auto it: _contents)
    {
      ++_component_stack_ref;
      *_component_stack_ref = it;
    }
    return _component_stack_ref;
  }

  void OrderedContainer::_updatePos()
  {
    if(_flags & flags_updating)
    {
      _flags |= flags_invalid;
      return;
    }
    _flags |= flags_updating;

    _updateRelativePos();

    _updateContents();

    if(_flags & flags_invalid)
    {
      _flags &= ~(flags_updating | flags_invalid);
      _updateSubComponentPositioning();
      _updateContents();
    }

    if(_verts != nullptr) _updateVerts();

    _flags &= ~(flags_updating | flags_invalid);

    //printf("size: %i\n", (int)(_height * _view_height));
  }

  //VerticalContainer
  void VerticalContainer::_updateSubComponentPositioning()
  {
    int32_t shift;
    uint32_t offset = 0;

    for(auto it: _contents)
    {
      shift = it->_p1.y_abs - offset;
      it->_p1.y_abs -= shift;
      offset = it->_p2.y_abs -= shift;
    }

    if(_flags & flags_autoresize)
    {
      _p2.y_abs = _p1.y_abs + offset;
      _updateRelativePos();
    }
  }
}
