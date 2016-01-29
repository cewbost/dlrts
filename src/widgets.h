
#pragma once

/*
    Some documentation on constructors:

  Icon(Style* style)
  TextBox(Style* style, uint32_t flags, const char* text)
  Window(Style* style, uint32_t flags)
  Button(Style* style, uint32_t flags)
        (Style* style, uint32_t flags, Style* icon_style)
        (Style* style, uint32_t flags, const char* text)
  GridPanel(Style* style, uint32_t flags,
            uint16_t tile_width, uint16_t tile_height)
  SelectorGrid(Style* style, Style* button_style, uint32_t flags,
              uint16_t tile_width, uint16_t tile_height)
  ScrollArea(Style* style, Style* body_style, Style* scrollbar_style,
              Style* scrollbar_aide_style, uint32_t flags,
              uint32_t scrollbar_width)
  VerticalContainer(Style* style, uint32_t flags)
*/

namespace GUI
{
  ///widget constants
  enum
  {
    grid_elem_width     = 1,
    grid_elem_height    = 2
  };

  ///user events
  enum
  {
    event_resize    = 1,
    event_scroll    = 2
  };

  ///Icon class
  //This class simply wraps mouse events to the mother
  class Icon: public Widget
  {
    int _handleMouseEvent(SDL_Event& event, int data = 0)
    {return _mother->_handleMouseEvent(event, data);}

    public:

    Icon(Style* style, const Pos& pos): Widget(style, 0, pos){}
    virtual ~Icon(){}
  };

  //this is a widget containing a text component
  class TextBox: public Widget
  {
    protected:

    Utils::counted_ptr<TextComponent> _text_component;

    Component** _stackSubComponents()
    {
      ++_component_stack_ref;
      *_component_stack_ref = _text_component.get();
      return _component_stack_ref;
    }

    public:
    TextBox(Style* style, uint32_t flags, const char* text, const Pos& pos)
    : Widget(style, flags, pos)
    {
      _text_component = Utils::makeRefCounted<TextComponent>
        (style->getFont(), text, Pos(0.f, 2, 0.f, 2, 1.f, -2, 1.f, -2));
      _text_component->_mother = this;
      _text_component->_updatePos();
    }
    virtual ~TextBox()
    {
      _text_component = nullptr;
    }
  };

  class Window: public ContainerWidget
  {
    protected:

    int32_t _min_w, _min_h;

    int _handleMouseEvent(SDL_Event&, int);
    public:
    Window(Style* style, uint32_t flags, const Pos& pos)
    : ContainerWidget(style, flags, pos){}
    ~Window(){}

    void setMinimumSize(uint32_t, uint32_t);
  };

  class Button: public Widget
  {
    protected:
    int _handleMouseEvent(SDL_Event&, int);

    Callback _press_callback;
    Callback _release_callback;
    Callback _activate_callback;
    int _signature;

    //union
    //{
      //Utils::counted_ptr<Component>       _label;
      Utils::counted_ptr<TextComponent>   _text;
      Utils::counted_ptr<Icon>            _icon;
    //};

    Component** _stackSubComponents();

    //this method is called by all constructors
    void __constructor()
    {
      _press_callback = nullptr;
      _release_callback = nullptr;
      _activate_callback = nullptr;
      _signature = 0;
    }

    public:

    void setSignature(int);
    void setPressCallback(Callback);
    void setReleaseCallback(Callback);
    void setActivateCallback(Callback);

    void setText(const char*);
    void setIcon(Style* style);

    void setState(bool);

    //constructors/destructors
    Button(Style* style, uint32_t flags, const Pos& pos)
    : Widget(style, flags, pos)
    {
      __constructor();
      //_label = nullptr;
      _flags &= ~(flags_icon | flags_text);
    }
    Button(Style* style, uint32_t flags, Style* icon, const Pos& pos)
    : Widget(style, flags, pos)
    {
      __constructor();
      setIcon(icon);
    }
    Button(Style* style, uint32_t flags, const char* text, const Pos& pos)
    : Widget(style, flags, pos)
    {
      __constructor();
      setText(text);
    }
    virtual ~Button(){}

    friend class SelectorGrid;
  };

  ///GridContainer superclass

  class GridContainer: public Widget
  {
    protected:

    std::vector<Utils::counted_ptr<Component>> _contents;
    uint16_t _elem_width;
    uint16_t _elem_height;

    void _updateSubComponentPositioning();

    void _updatePos();

    Component** _stackSubComponents();

    public:
    GridContainer(Style* style, uint32_t flags, uint16_t w,
                  uint16_t h, const Pos& pos): Widget(style, flags, pos)
    {
      _elem_width = w;
      _elem_height = h;
    }
    GridContainer(Style* style, uint32_t flags, const Pos& pos)
    : Widget(style, flags, pos)
    {
      _elem_width = 1;
      _elem_height = 1;
    }
    virtual ~GridContainer()
    {
      _contents.clear();
    }

    void setElementSize(uint32_t, uint32_t);
  };

  //GridContainer sub-classes

  class GridPanel: public GridContainer
  {
    public:
    GridPanel(Style* style, uint32_t flags,
              uint32_t w, uint32_t h, const Pos& pos)
    : GridContainer(style, flags, w, h, pos){}
    GridPanel(Style* style, uint32_t flags, const Pos& pos)
    : GridContainer(style, flags, pos){}
    virtual ~GridPanel(){}

    template<class C, class... T>
    C* attachComponent(T... t)
    {
      C* component = Utils::makeRefCounted<C>(t...);
      component->_mother = this;
      _contents.push_back(component);
      _updateSubComponentPositioning();
      component->_updatePos();
      return component;
    }
  };


  class SelectorGrid: public GridContainer
  {
    protected:

    Callback _callback;
    Style* _button_style;
    Button* _current_selected;

    void _handleSelectionEvent(Component*);

    public:
    SelectorGrid(Style* style, Style* b_style, uint32_t flags,
        uint32_t w, uint32_t h, const Pos& pos)
    : GridContainer(style, flags, w, h, pos)
    {
      _callback = nullptr;
      _button_style = b_style;
      _current_selected = nullptr;
    }
    SelectorGrid(Style* style, Style* b_style, uint32_t flags,
        const Pos& pos)
    : GridContainer(style, flags, pos)
    {
      _callback = nullptr;
      _button_style = b_style;
      _current_selected = nullptr;
    }
    virtual ~SelectorGrid(){}

    Button* newButton(uint32_t);
    Button* newButton(uint32_t, Style*);
    Button* newButton(uint32_t, const char*);
    void setCallback(Callback);
    void setSelection(int32_t);
    void deselect();
  };

  ///scroll Component major class

  class ScrollArea: public Widget
  {
    protected:

    struct __ScrollInfo
    {
      struct
      {
        uint32_t x;
        uint32_t y;
      }
      viewport_size,
      virtual_size,
      scroll;

      uint32_t scroll_size;
      uint32_t scroll_status;
    };

    class __SA_Body: public Widget
    {
      private:

      void __updateVPVA();

      public:

      Utils::counted_ptr<Component> _content;
      __ScrollInfo* _scinfo;

      struct
      {
        uint32_t x;
        uint32_t y;
      }_scroll;

      void _updateScrollBars();
      void _updateScrollInfo();

      void _setScroll(int, int);

      virtual Component** _stackSubComponents();
      virtual void _updatePos();

      //public:
      __SA_Body(Style* style, __ScrollInfo* sc,
          uint32_t flags, const Pos& pos)
      : Widget(style, flags, pos)
      {
        _content = nullptr;
        _scroll.x = 0;
        _scroll.y = 0;
        _scinfo = sc;
      }
      ~__SA_Body()
      {
        _content = nullptr;
      }

      //void resize
    };

    class __ScrollBar: public Widget
    {
      public:

      class __SB_Aide: public Widget
      {
        int _handleMouseEvent(SDL_Event&, int);

        public:
        __SB_Aide(Style* style, uint32_t flags, const Pos& pos)
        : Widget(style, flags, pos){}
        ~__SB_Aide(){}
      };

      Utils::counted_ptr<__SB_Aide> _sb_aide;
      __ScrollInfo* _scinfo;

      Component** _stackSubComponents();

      void _updatePos();

      __ScrollBar(Style*, Style*, __ScrollInfo*, uint32_t, const Pos&);
      ~__ScrollBar()
      {
        _sb_aide = nullptr;
      }
    };

    Utils::counted_ptr<__SA_Body> _body;
    Utils::counted_ptr<__ScrollBar> _scroll_bar_x;
    Utils::counted_ptr<__ScrollBar> _scroll_bar_y;

    uint32_t _scroll_size;

    __ScrollInfo _scinfo;

    virtual Component** _stackSubComponents();

    void _trackVerticScrollPosition(int32_t);
    void _trackHorizScrollPosition(int32_t);

    public:
    ScrollArea(Style*, Style*, Style*, Style*, uint32_t, uint32_t, const Pos&);
    ~ScrollArea()
    {
      _body = nullptr;
      _scroll_bar_x = nullptr;
      _scroll_bar_y = nullptr;
    }

    //attachment functions
    template<class C, class... T>
    C* attachComponent(uint32_t w, uint32_t h, T... t)
    {
      C* component = Utils::makeRefCounted<C>
      (t..., Pos(0.0, 0, 0.0, 0, 0.0, w, 0.0, h));
      component->_mother = _body;
      component->_updatePos();
      _body->_content = component;
      return component;
    }
    template<class C, class... T>
    C* attachComponent(uint32_t dim, T... t)
    {
      C* component;
      if(_flags & flags_horizontal)
        component = Utils::makeRefCounted<C>
        (t..., Pos(0.0, 0, 0.0, 0, 0.0, dim, 1.0, 0));
      else component = Utils::makeRefCounted<C>
        (t..., Pos(0.0, 0, 0.0, 0, 1.0, 0, 0.0, dim));

      component->_mother = _body;
      ((Component*)component)->_updatePos();
      _body->_content = component;
      return component;
    }
  };

  //containers
  class OrderedContainer: public Widget
  {
    protected:

    std::vector<Utils::counted_ptr<Component>> _contents;

    Component** _stackSubComponents();
    void _updatePos();

    virtual void _updateSubComponentPositioning() = 0;

    public:

    OrderedContainer(Style* style, uint32_t flags, const Pos& pos)
    : Widget(style, flags, pos){}
    virtual ~OrderedContainer()
    {
      _contents.clear();
    }
  };

  class VerticalContainer: public OrderedContainer
  {
    protected:

    void _updateSubComponentPositioning();

    public:

    VerticalContainer(Style* style, uint32_t flags, const Pos& pos)
    : OrderedContainer(style, flags, pos){}
    ~VerticalContainer(){}

    //Component* attachComponent(Component*, uint32_t);
    template<class C, class... T>
    C* attachComponent(uint32_t size, T... t)
    {
      uint32_t offset;
      if(_contents.empty()) offset = 0;
      else offset = _contents.back()->_p2.y_abs;

      C* component = Utils::makeRefCounted<C>
      (t..., Pos(0.0, 0, 0.0, offset, 1.0, 0, 0.0, offset + size));

      _contents.push_back(component);
      component->_mother = this;

      if(_flags & flags_autoresize)
      {
        _p2.y_abs = _p1.y_abs + offset + size;
        _updatePos();
      }else ((Widget*)component)->_updatePos();

      return component;
    }
  };
}
