
#include <cstdio>
#include <ctime>

#include "terrain.h"
#include "interface.h"
#include "gui.h"
#include "entities.h"
#include "scenario.h"
#include "game.h"
#include "app.h"

#include "editor.h"

//these are not in the anonymous namespace because
//they are referenced by interface.cpp
GUI::Window*            _terrainedit_window;
GUI::Window*            _te_head;
GUI::Window*            _te_body;
GUI::ScrollArea*        _te_scroll;
GUI::VerticalContainer* _tes_body;
GUI::TextBox*           _tes_brush_label;
GUI::SelectorGrid*      _tes_brush_selector;
GUI::TextBox*           _tes_slope_label;
GUI::SelectorGrid*      _tes_slope_selector;

GUI::Window*            _unit_selector_window;
GUI::Window*            _us_head;
GUI::Window*            _us_body;
GUI::Button*            _us_unit_button;
GUI::Button*            _us_light_button;

GUI::Window*            _top_bar;
GUI::Button*            _run_scenario_button;

namespace
{
  //variables
  bool _change_task = false;

  //functions
  inline void _processMessages()
  {
    SDL_Event event;

    static int bypass_gui = 0;

    while(SDL_PollEvent(&event))
    {
      if(bypass_gui == 0 && GUI::trapEvent(event))
        continue;

      switch(event.type)
      {
        case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
          //AppCtrl::task_stack.pop();
          AppCtrl::task = Editor::endTask;
          _change_task = true;

          default: break;
        }
        break;

        case SDL_MOUSEMOTION:

        if(Interface::Cursor::getMode()
          != Interface::Cursor::mode_none)
          Interface::Cursor::updatePosition(event.motion.x,
                                            event.motion.y);
        else Interface::Game::mouseMotion(event.motion.x,
                                          event.motion.y);
  /**/
        break;

        case SDL_MOUSEBUTTONDOWN:

        ++bypass_gui;

        if(event.button.button == SDL_BUTTON_LEFT)
        {
          if(Interface::Cursor::getMode() 
            != Interface::Cursor::mode_none)
            Interface::Cursor::setDrawingMode(1);
          else Interface::Game::leftClick(event.button.x,
                                          event.button.y);
        }
        else if(event.button.button == SDL_BUTTON_RIGHT)
        {
          if((Interface::Cursor::getMode() & 0xff00)
            == Interface::Cursor::mode_entity)
          {
            Interface::Cursor::setMode
            (Interface::Cursor::mode_none);
            _us_unit_button->setState(false);
            _us_light_button->setState(false);
          }
          else if(Interface::Cursor::getMode()
            != Interface::Cursor::mode_none)
            Interface::Cursor::setDrawingMode(2);
        }
  /**/
        break;

        case SDL_MOUSEBUTTONUP:

        --bypass_gui;

        if(event.button.button == SDL_BUTTON_LEFT)
        {
          if(Interface::Cursor::getMode()
            != Interface::Cursor::mode_none)
            Interface::Cursor::setDrawingMode(0);
          else Interface::Game::leftRelease();
        }
        else if(event.button.button == SDL_BUTTON_RIGHT)
          Interface::Cursor::setDrawingMode(0);
  /**/
        break;

        default: break;
      }
    }

    return;
  }

  //callbacks
  void _brushSelectCallback(const void* sig)
  {
    Interface::Cursor::setMode(*(int*)sig);
    _tes_slope_selector->deselect();
    _us_unit_button->setState(false);
    _us_light_button->setState(false);
  }

  void _slopeSelectCallback(const void* sig)
  {
    Interface::Cursor::setMode(*(int*)sig);
    _tes_brush_selector->deselect();
    _us_unit_button->setState(false);
    _us_light_button->setState(false);
  }

  void _unitSelectCallback(const void* sig)
  {
    Interface::Cursor::setMode(*(int*)sig);
    _tes_brush_selector->deselect();
    _tes_slope_selector->deselect();
    _us_light_button->setState(false);
  }
  
  void _lightSelectCallback(const void* sig)
  {
    Interface::Cursor::setMode(*(int*)sig);
    _tes_brush_selector->deselect();
    _tes_slope_selector->deselect();
    _us_unit_button->setState(false);
  }

  void _testScenario(const void* sig)
  {
    //AppCtrl::task_stack.push(Scenario::mainTask);
    AppCtrl::task = Game::play;
    _change_task = true;
  }

  inline void _setupInterface()
  {
    using namespace GUI;

    Style* s_body       = findStyle("stdgui.body");
    Style* s_head       = findStyle("stdgui.head");
    Style* s_scroll_bar = findStyle("stdgui.scroll_bar");
    Style* s_button     = findStyle("stdgui.button");

    //terrain edit window
    ContainerWidget* screen = (ContainerWidget*)getScreenPointer();

    _terrainedit_window = screen->attachComponent<Window>
    (nullptr, 0, Pos(100, 100, 210, 250));
    _te_head = _terrainedit_window->attachComponent<Window>
    (s_head, flags_dragable | (flags_resizable & ~flags_resizable_bottom)
      | flags_surrogate, Pos(0.f, 0, 0.f, 0, 1.f, 0, 0.f, 20));
    _te_body = _terrainedit_window->attachComponent<Window>
    (s_body, (flags_resizable & ~flags_resizable_top) | flags_surrogate,
    Pos(0.f, 0, 0.f, 20, 1.f, 0, 1.f, 0));

    _terrainedit_window->setMinimumSize(80, 80);


    _te_scroll = _te_body->attachComponent<ScrollArea>
    (nullptr, nullptr, s_scroll_bar, s_head, flags_vertical, 16,
    Pos(0.0f, 4, 0.0f, 4, 1.0f, -4, 1.0f, -4));

    _tes_body = _te_scroll->attachComponent<VerticalContainer>
    (100, nullptr, flags_autoresize);

    //brushes
    _tes_brush_label = _tes_body->attachComponent<TextBox>
                      (22, s_body, 0, "Brush size.");
    _tes_brush_selector = _tes_body->attachComponent<SelectorGrid>
    (20, nullptr, s_button, flags_autoresize | flags_surrogate, 24, 24);

    _tes_brush_selector->newButton(Interface::Cursor::mode_x1, "x1");
    _tes_brush_selector->newButton(Interface::Cursor::mode_x2, "x2");
    _tes_brush_selector->newButton(Interface::Cursor::mode_x3, "x3");
    _tes_brush_selector->newButton(Interface::Cursor::mode_x4, "x4");

    _tes_brush_selector->setCallback(_brushSelectCallback);

    //cliffs
    _tes_slope_label = _tes_body->attachComponent<TextBox>
    (22, s_body, 0, "Slopes.");
    _tes_slope_selector = _tes_body->attachComponent<SelectorGrid>
    (20, nullptr, s_button, flags_autoresize | flags_surrogate, 24, 24);

    _tes_slope_selector->newButton(Interface::Cursor::mode_slope_ne, "ne");
    _tes_slope_selector->newButton(Interface::Cursor::mode_slope_se, "se");
    _tes_slope_selector->newButton(Interface::Cursor::mode_slope_sw, "sw");
    _tes_slope_selector->newButton(Interface::Cursor::mode_slope_nw, "nw");

    _tes_slope_selector->setCallback(_slopeSelectCallback);

    //unit selector
    _unit_selector_window = screen->attachComponent<Window>
    (nullptr, 0, Pos(300, 100, 400, 200));

    _us_head = _unit_selector_window->attachComponent<Window>
    (s_head, flags_dragable | (flags_resizable & ~flags_resizable_bottom)
      | flags_surrogate, Pos(0.0f, 0, 0.0f, 0, 1.0f, 0, 0.0f, 20));
    _us_body = _unit_selector_window->attachComponent<Window>
    (s_body, (flags_resizable & ~flags_resizable_top) | flags_surrogate,
    Pos(0.0f, 0, 0.0f, 20, 1.0f, 0, 1.0f, 0));

    _unit_selector_window->setMinimumSize(80, 80);

    _us_unit_button = _us_body->attachComponent<Button>
      (s_button, flags_hold, Pos(0.0, 6, 0.0, 6, 1.0, -6, 0.5, -3));
    _us_unit_button->setActivateCallback(_unitSelectCallback);
    _us_unit_button->setSignature(Interface::Cursor::mode_unit);
    _us_unit_button->setText("unit");
    
    _us_light_button = _us_body->attachComponent<Button>
      (s_button, flags_hold, Pos(0.0, 6, 0.5, 3, 1.0, -6, 1.0, -6));
    _us_light_button->setActivateCallback(_lightSelectCallback);
    _us_light_button->setSignature(Interface::Cursor::mode_light);
    _us_light_button->setText("light");

    _top_bar = screen->attachComponent<Window>
    (s_body, (flags_hide_frame & ~flags_hide_bottom),
    Pos(0.0f, 0, 0.0f, 0, 1.0f, 0, 0.0, 30));

    _run_scenario_button = _top_bar->attachComponent<Button>
    (s_button, flags_none, Pos(0.0f, 10, 0.1f, 0, 0.0f, 70, 0.9f, 0));

    _run_scenario_button->setText("  RUN");
    _run_scenario_button->setSignature(0);
    _run_scenario_button->setReleaseCallback(_testScenario);
  }

  inline void _removeInterface()
  {
    ((GUI::ContainerWidget*)GUI::getScreenPointer())->detachAllComponents();
  }
}

namespace Editor
{


void mainTask()
{
  //setup basics
  _change_task = false;

  const uint8_t* keystate;
  keystate = SDL_GetKeyboardState(nullptr);


  Interface::Cursor::setMode(Interface::Cursor::mode_none);

  _setupInterface();

  //h3dSetOption(H3DOptions::DebugViewMode, 1);
  
  unsigned frame_count = 0;
  AppCtrl::startFrameCount();
  
  do
  {
    //render stuff
    h3dRender(Scenario::camera->handle);
    h3dFinalizeFrame();
    SDL_GL_SwapWindow(AppCtrl::screen);
    
    while(frame_count <= AppCtrl::getFrameCount())
    {
      //do sum SDL stuff
      SDL_PumpEvents();
      _processMessages();

      if(keystate[SDL_SCANCODE_S])
        Scenario::moveCamera(-0.05, -0.05);
      if(keystate[SDL_SCANCODE_W])
        Scenario::moveCamera(0.05, 0.05);
      if(keystate[SDL_SCANCODE_A])
        Scenario::moveCamera(0.05, -0.05);
      if(keystate[SDL_SCANCODE_D])
        Scenario::moveCamera(-0.05, 0.05);

      Interface::update();
      Scenario::proceed();
      ++frame_count;
    }
    
    AppCtrl::dumpMessages();
    AppCtrl::collectStats();
    
    AppCtrl::delayToFrame(frame_count);
    
  }while(!_change_task);
  
  //cleanup
  Interface::Cursor::setMode(Interface::Cursor::mode_none);
  _removeInterface();
}

void startTask()
{
  Scenario::create(32, 32);
  AppCtrl::task = mainTask;
}

void endTask()
{
  Scenario::destroy();
  AppCtrl::task = nullptr;
}


}
