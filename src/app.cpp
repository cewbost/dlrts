#define APP_CPP

#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <ctime>

#include <stdint.h>

#include <horde3d.h>

#include "mathutils.h"
#include "scenario.h"
#include "interface.h"
#include "json.h"
#include "gui.h"
#include "resources.h"
#include "terrain.h"
#include "editor.h"
#include "entities.h"
#include "fow.h"

#include "app.h"

//some opengl linking stuff
#if defined(WINCE) || defined(WIN32) || defined(_WINDOWS)
# define GLAPI __declspec(dllimport)
# define GLAPIENTRY _stdcall
#else
# define GLAPI
# define GLAPIENTRY
#endif

typedef unsigned int    GLbitfield;
typedef float           GLclampf;

#define GL_COLOR_BUFFER_BIT               0x00004000

extern "C"
{
GLAPI void GLAPIENTRY glClear
  (GLbitfield mask);
GLAPI void GLAPIENTRY glClearColor
  (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
}



namespace
{
  uint32_t _tri_count_peak = 0;
  //uint32_t _frames        = 0;
  //uint32_t _seconds       = 0;
  
  uint32_t _ticks;
  
  inline void _updateRenderTargets(uint32_t w, uint32_t h)
  {
    H3DRes r = h3dGetNextResource(H3DResTypes::Pipeline, 0);

    while(r != 0)
    {
      h3dResizePipelineBuffers(r, w, h);
      r = h3dGetNextResource(H3DResTypes::Pipeline, r);
    }

    return;
  }
}

namespace AppCtrl
{
  SDL_Window* screen = nullptr;
  SDL_GLContext gl_context = nullptr;

  uint16_t screen_w = 0;
  uint16_t screen_h = 0;

  int flags = 0;

  float screen_aspect = 0.0;

  std::string app_path = "";

  //std::stack<Task> task_stack;
  void (*task)();
}

                                            ///INIT FUNCTION///
int AppCtrl::init(const char* _app_path)
{
  //sdl initialization
  if(SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
  {
    std::printf("SDL_Init failed.\nSDL error: %s\n", SDL_GetError());
    return 1;
  }

  SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  
  SDL_DisplayMode dm;
  if(SDL_GetDesktopDisplayMode(0, &dm) != 0)
  {
    std::printf("SDL_GetDesktopDisplayMode failed.\nSDL error: %s\n", SDL_GetError());
    return 1;
  }
  
  screen_w = dm.w;
  screen_h = dm.h;

  #ifndef NDEBUG
  screen_w /= 2;
  screen_h /= 2;
  #endif

  screen_aspect = (float)screen_w / (float)screen_h;

  #ifndef NDEBUG
  if((screen = SDL_CreateWindow("dlrts",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    screen_w, screen_h, SDL_WINDOW_OPENGL)) == nullptr)
  #else
  if((screen = SDL_CreateWindow("dlrts",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    screen_w, screen_h, SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN_DESKTOP)) == nullptr)
  #endif
  {
    std::printf("SDL_CreateWindow failed.\nSDL error: %s\n", SDL_GetError());
    return 1;
  }
  
  if((gl_context = SDL_GL_CreateContext(screen)) == nullptr)
  {
    std::printf("SDL_GL_CreateContext failed.\nSDL error: %s\n", SDL_GetError());
  }
  
  //clear the screen before anything else
  glClearColor(0., 0., 0., 0.);
  glClear(GL_COLOR_BUFFER_BIT);

  //horde3d shit
  if(!h3dInit())
  {
    std::printf("h3dInit failed.\n");
    return 1;
  }
  flags |= H3DINIT_FLAG;

  h3dSetOption(H3DOptions::LoadTextures, 0);
  //h3dSetOption(H3DOptions::DebugViewMode, 1);

  app_path = _app_path;

  if( app_path.find( "/" ) != std::string::npos )
    app_path.erase(app_path.rfind("/") + 1);
    //return s.substr( 0, s.rfind( "/" ) ) + "/";
  else if( app_path.find( "\\" ) != std::string::npos )
    app_path.erase(app_path.rfind("\\") + 1);
    //return s.substr( 0, s.rfind( "\\" ) ) + "\\";
  else
    app_path.clear();

  //add all the resources
  Resources::registerAll();
  
  _updateRenderTargets(screen_w, screen_h);

  JSonParser::setPath(&app_path);

  //rand init
  std::srand(std::time(nullptr));
  std::rand();

  //init horde dependent subsystems
  Terrain::init();
  Interface::init();
  FogOfWar::init();
  Entities::init();
  Scenario::init();
  
  if(Resources::loadAll() != 0)
  {
    printf("Loading resources failed.\n");
    return 1;
  }
  printf("Resources loaded.\n");

  return 0;
}

void AppCtrl::deinit()
{
  if(flags & H3DINIT_FLAG)
  {
    Entities::deinit();
    Terrain::deinit();
    Interface::deinit();
  }

  dumpMessages();

  if(flags & H3DINIT_FLAG)
  {
    h3dClear();
    h3dRelease();
  }
  
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(screen);

  SDL_Quit();

  return;
}


                                            ///RUN FUNCTION///
void AppCtrl::run()
{
  task = Editor::startTask;

  while(task != nullptr)
  {
    task();
  }
}

void AppCtrl::startFrameCount()
{
  _ticks = SDL_GetTicks();
}

uint32_t AppCtrl::getFrameCount(int fps)
{
  return (SDL_GetTicks() - _ticks) * fps / 1000;
}

void AppCtrl::delayToFrame(int frame, int fps)
{
  int diff = _ticks + (frame * 1000 / fps) - SDL_GetTicks();
  if(diff > 0) SDL_Delay(diff);
}

///debug info functions

void AppCtrl::dumpMessages()
{
  if(!(flags & H3DINIT_FLAG)) return;

  std::string message;

  for(message = h3dGetMessage(nullptr, nullptr); message.length() > 0;
    message = h3dGetMessage(nullptr, nullptr))
    std::printf("%s\n", message.c_str());

  std::fflush(stdout);

  return;
}

void AppCtrl::collectStats()
{
  uint32_t i;

  i = h3dGetStat(H3DStats::TriCount, true);

  _tri_count_peak = _tri_count_peak > i? _tri_count_peak : i;

  return;
}

void AppCtrl::dumpStats()
{
  printf("Triangle count peak: %i\n", _tri_count_peak);
  //printf("Average fps: %i\n", _frames / (SDL_GetTicks() / 1000));
  fflush(stdout);

  return;
}
