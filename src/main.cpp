
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <csignal>

#include "app.h"
#include "resources.h"

//*
#ifdef main
#undef main
#endif          //Because fuck SDL that's why...
/**/

void deinit()
{
  AppCtrl::dumpMessages();

  Resources::unloadAll();
  AppCtrl::dumpStats();
  AppCtrl::deinit();
}

void sig_handler(int sig)
{
  switch(sig)
  {
  case SIGABRT:
    printf("program recieved a SIGABRT signal.\n");
    break;

  case SIGSEGV:
    printf("program recieved a SIGSEGV signal.\n");
    break;

  case SIGFPE:
    printf("program recieved a SIGFPE signal.\n");
    break;

  default:
    printf("program recieved an undefined signal.\n");
    break;
  }

  signal(SIGABRT, SIG_DFL);
  signal(SIGSEGV, SIG_DFL);
  signal(SIGFPE, SIG_DFL);

  deinit();

  exit(sig);
}

int main(int argc, char** argv)
{
  //atexit(deinit);

  /*signal(SIGABRT, sig_handler);
  signal(SIGSEGV, sig_handler);
  signal(SIGFPE, sig_handler);*/
  
  //* turn of main flow to perform tests
  do
  {
    if(AppCtrl::init(argv[0]) != 0) break;
    AppCtrl::dumpMessages();
    printf("Init done.\n");

    AppCtrl::run();
  }while(false);
  /**/

  deinit();

  return 0;
}
