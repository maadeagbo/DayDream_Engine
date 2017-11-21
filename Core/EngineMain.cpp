#include <SDL.h>
//#undef main //SDL_main.h is included automatically from SDL.h

#include "DD_Engine.h"

//#include "LevelBuilder.h"
//#include "ReconstructScene.h"
//#include "WaterLevel.h"
//#include "CrowdSim.h"
//#include "PoseView.h"
//#include "LightingLvl.h"

int main(int argc, char * argv[])
{
	DD_Engine ddEngine;
	/*
	ddEngine.AddLevel(new LevelBuilder(), 
					  "LevelBuilder/assets", 
					  "Level Builder");
	ddEngine.AddLevel(new CrowdSim(), 
					  "CrowdSim/assets", 
					  "Crowd Simulation");
	ddEngine.AddLevel(new ReconstructScene(), 
					  "DepthProjection/assets", 
					  "Depth Projection");
	ddEngine.AddLevel(new WaterLevel(),
					  "WaterLevel/assets",
					  "Water Level");
	ddEngine.AddLevel(new PoseView(),
					  "PoseReconstruction/assets",
					  "Pose Reconstruction");
	ddEngine.AddLevel(new LightingLvl(),
					  "LightingTests/assets",
					  "Lighting Tests");
	*/
	bool launch_engine = ddEngine.LevelSelect();

	if( launch_engine ) {
		ddEngine.Launch();
		ddEngine.Run();
	}
	ddEngine.cleanUp();

	return 0;
}
