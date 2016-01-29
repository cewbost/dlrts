#pragma once

#include <string>

#include <stack>

#include <SDL.h>

#include <horde3d.h>

#include <stdint.h>

namespace AppCtrl
{
  enum
  {
    H3DINIT_FLAG    = 0x01
  };

  //variables
  extern SDL_Window* screen;
  extern SDL_GLContext gl_context;
  
  extern uint16_t screen_w, screen_h;
  extern float screen_aspect;

  extern int flags;

  extern std::string app_path;

  //extern std::stack<Task> task_stack;
  extern void (*task)();

  //functions
  int init(const char* app_path);
  void deinit();

  void run();

  void startFrameCount();
  uint32_t getFrameCount(int fps = 60);
  void delayToFrame(int, int fps = 60);

  //debugging info
  void dumpMessages();

  void collectStats();
  void dumpStats();
}
