#include <SDL.h>
//#undef main //SDL_main.h is included automatically from SDL.h

#include "DD_Engine.h"

int main(int argc, char* argv[]) {
  DD_Engine ddEngine;
  ddEngine.Load();
  ddEngine.startup_lua();
  bool launch_engine = ddEngine.LevelSelect();

  if (launch_engine) {
    ddEngine.Run();
  }
  ddEngine.cleanUp();

  return 0;
}
