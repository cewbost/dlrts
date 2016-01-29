
#include <SDL.h>

#include <stdint.h>

#include "interface.h"
#include "scenario.h"
#include "editor.h"
#include "gui.h"
#include "entities.h"
#include "app.h"

#include "game.h"

namespace
{
  bool _running;

  inline void _processMessages()
  {
    /**
      HERE!
    **/

    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
      if(GUI::trapEvent(event))
        continue;

      switch(event.type)
      {
        case SDL_KEYDOWN:
        switch(event.key.keysym.sym)
        {
          case SDLK_ESCAPE:
          //AppCtrl::task_stack.pop();
          AppCtrl::task = Editor::mainTask;
          _running = false;

          default: break;
        }
        break;

        case SDL_MOUSEMOTION:

        Interface::Game::mouseMotion(event.motion.x, event.motion.y);
  /**/
        break;

        case SDL_MOUSEBUTTONDOWN:

        if(event.button.button == SDL_BUTTON_LEFT)
          Interface::Game::leftClick(event.button.x, event.button.y);
        else if(event.button.button == SDL_BUTTON_RIGHT)
          Interface::Game::rightClick(event.button.x, event.button.y);
  /**/
        break;

        case SDL_MOUSEBUTTONUP:

        if(event.button.button == SDL_BUTTON_LEFT)
          Interface::Game::leftRelease();
        else if(event.button.button == SDL_BUTTON_RIGHT)
          Interface::Game::rightRelease();
  /**/
        break;

        default: break;
      }
    }

    return;
  }
}

namespace Game
{
  void play()
  {
    _running = true;

    const uint8_t* keystate;
    keystate = SDL_GetKeyboardState(nullptr);

    Scenario::start();
    
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
      
      AppCtrl::delayToFrame(frame_count);
      
      AppCtrl::dumpMessages();
      AppCtrl::collectStats();
    }while(_running);

    Scenario::end();
  }
}
