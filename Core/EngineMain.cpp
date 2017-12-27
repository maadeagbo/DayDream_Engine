#include <SDL.h>
//#undef main //SDL_main.h is included automatically from SDL.h

#include "Engine.h"

int main(int argc, char* argv[]) {
  ddEngine engine;
  engine.Load();
  engine.startup_lua();
  bool launch_engine = engine.LevelSelect();

  if (launch_engine) {
    engine.Run();
  }
  engine.cleanUp();

  return 0;
}
