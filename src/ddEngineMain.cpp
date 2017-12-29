#include "ddEngine.h"

int main(int argc, char* argv[]) {
  ddEngine engine;
  engine.load();
  engine.startup_lua();
  bool launch_engine = engine.level_select();

  if (launch_engine) {
    engine.run();
  }
  engine.shutdown();

  return 0;
}
