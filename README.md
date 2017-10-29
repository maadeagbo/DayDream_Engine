# DayDream_Repo

Linux libs: 
	- libsdl2-dev
	- libsoil-dev
	- libfreetype6-dev

To change cmake build types:
--> Navigate to root of project
	For Debug build
		$ cmake -DCMAKE_BUILD_TYPE=Debug ../
	For Release build
		$ cmake -DCMAKE_BUILD_TYPE=Release ../
		$ cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo
	For windows
		$ cmake ..\ -A x64

All builds are written to the <root>/bin/DayDream_Engine
