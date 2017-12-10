cmake_minimum_required(VERSION 3.5.1)
project (DayDream_Engine_alpha)

# set release or debug builds
if(CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo Debug)
   set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
     "Reset the configurations to what we need"
     FORCE)
 endif()
#SET(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 11)

# set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# set flags for debug and release mode
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-g HAS_DEBUG)
check_cxx_compiler_flag(-Wall HAS_WALL)
check_cxx_compiler_flag(-O0 HAS_OPT0)
check_cxx_compiler_flag(-O3 HAS_OPT3)
check_cxx_compiler_flag(-fmessage-length=0 HAS_MSG)
check_cxx_compiler_flag(-std=c++11 HAS_CNEW)
check_cxx_compiler_flag(-lstdc++fs HAS_FS)
if(HAS_DEBUG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")
endif()
if(HAS_WALL)
	if(UNIX)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
	else()
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -W4")
	endif()
endif()
if(HAS_OPT0)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
endif()
if(HAS_OPT3)
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
endif()
if(HAS_MSG)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fmessage-length=0")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fmessage-length=0")
endif()
if(HAS_CNEW)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++11")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11")
endif()


# add paths to freetype lib and sdl2
if(NOT UNIX)
	set(ENV{SDL2DIR} ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.7)
	set(ENV{FREETYPE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.7)
endif()

# includes cmake/FindSDL2.cmake and sets module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})

# include directories for project
find_path(GLM_PATH glm.hpp
	PATHS
	${CMAKE_SOURCE_DIR}/glm/glm)
if(NOT GLM_PATH)
	message(FATAL_ERROR "GLM includes not found")
endif()
find_path(IMGUI_PATH imgui.h
	PATHS
	${CMAKE_CURRENT_SOURCE_DIR}/imgui)
if(NOT IMGUI_PATH)
	message(FATAL_ERROR "IMGUI_PATH IMGUI includes not found")
endif()
find_path(VRPN_PATH vrpn_Tracker.h
	PATHS
	${CMAKE_CURRENT_SOURCE_DIR}/vrpn_lib_files)
if(NOT VRPN_PATH)
	message(FATAL_ERROR "VRPN_PATH VRPN includes not found")
endif()
if(NOT UNIX)
	find_path(SOIL_PATH SOIL.h
		PATHS
		${CMAKE_CURRENT_SOURCE_DIR}/SOIL/src)
	if(NOT SOIL_PATH)
		message(FATAL_ERROR "SOIL includes not found")
	endif()
	find_path(LUA_PATH lua.hpp PATHS ${CMAKE_SOURCE_DIR}/lua53_Win64_lib/include)
	if(NOT LUA_PATH)
		message(FATAL_ERROR "Lua includes not found")
	endif()
else()
	find_path(SOIL_PATH SOIL.h PATHS /usr/include/SOIL)
	if(NOT SOIL_PATH)
		message(FATAL_ERROR "SOIL unix includes not found")
	endif()
	find_path(LUA_PATH lua.hpp PATHS /usr/include/lua5.3)
	if(NOT LUA_PATH)
		message(FATAL_ERROR "Lua includes not found")
	endif()
endif()

# OpenGL
include(FindPackageHandleStandardArgs)
find_package(OpenGL)

# OpenMP
find_package(OpenMP)
if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Freetype
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIRS})

# includes zlib (unix)
#find_package(ZLIB REQUIRED)
#if(ZLIB_FOUND)
	#message(STATUS ${ZLIB_LIBRARY})
	#target_link_libraries(DayDream_Engine ${ZLIB_LIBRARY})
#endif()

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
file(GLOB_RECURSE SOURCES 	"${CMAKE_SOURCE_DIR}/Core/*.cpp"
							"${CMAKE_SOURCE_DIR}/includes/*.hpp"
							"${CMAKE_SOURCE_DIR}/includes/*.h"
							"${CMAKE_SOURCE_DIR}/Core/*.c")
file(GLOB IMGUI_SRC "${CMAKE_SOURCE_DIR}/imgui/*.cpp")

# add defines for engine resources
add_definitions(
	"-DROOT_DIR=\"${CMAKE_SOURCE_DIR}/\""
	"-DRESOURCE_DIR=\"${CMAKE_SOURCE_DIR}/Resource/\""
	"-DSHADER_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Shaders/\""
	"-DMESH_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Meshes/\""
	"-DTEX_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Textures/\""
	"-DPROJECT_DIR=\"${CMAKE_SOURCE_DIR}/Projects/\""
	"-DFONTS_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Fonts/\""
)

# use file(GLOB_RECURSE...), add_library(DayDream_Engine...), and
# source_group(<group name> FILES...) to create folder structure in project
# LIST_DIRECTORIES remove directory name from files
file(GLOB_RECURSE PROJECT_SOURCES
	LIST_DIRECTORIES false
    "${CMAKE_SOURCE_DIR}/Projects/*.cpp*"
    "${CMAKE_SOURCE_DIR}/Core/AssetViewer/*.cpp*"
	"${CMAKE_SOURCE_DIR}/Core/AssetViewer/*.h*"
	"${CMAKE_SOURCE_DIR}/Projects/*.h*")

foreach(_proj_source IN ITEMS ${PROJECT_SOURCES})
	get_filename_component(_source_path "${_proj_source}" PATH)
	file(RELATIVE_PATH _source_path_rel "${CMAKE_SOURCE_DIR}" "${_source_path}")
	source_group("${_source_path_rel}" FILES "${_proj_source}")
endforeach()

# include all necessary headers
set(PROJ_HEADERS "")
file(GLOB_RECURSE OTHER_HEADERS
	LIST_DIRECTORIES false
	"${CMAKE_SOURCE_DIR}/Projects/*.h*")
foreach(_header IN ITEMS ${OTHER_HEADERS})
	get_filename_component(_h_path "${_header}" PATH)
	file(RELATIVE_PATH _h_path_rel "${CMAKE_SOURCE_DIR}" "${_h_path}")
	list(APPEND PROJ_HEADERS ${CMAKE_SOURCE_DIR}/${_h_path_rel})
endforeach()
list(REMOVE_DUPLICATES PROJ_HEADERS)

# add shaders to project
file(GLOB_RECURSE SHADER_SOURCES
	LIST_DIRECTORIES false
	"${CMAKE_SOURCE_DIR}/Resource/Shaders/*.vert*"
	"${CMAKE_SOURCE_DIR}/Resource/Shaders/*.frag*"
	"${CMAKE_SOURCE_DIR}/Resource/Shaders/*.geom*"
	"${CMAKE_SOURCE_DIR}/Resource/Shaders/*.comp*")

foreach(_proj_source IN ITEMS ${SHADER_SOURCES})
	get_filename_component(_source_path "${_proj_source}" PATH)
	file(RELATIVE_PATH _source_path_rel "${CMAKE_SOURCE_DIR}" "${_source_path}")
	source_group("${_source_path_rel}" FILES "${_proj_source}")
endforeach()

# add lua scripts
file(GLOB_RECURSE SCRIPTS 
	LIST_DIRECTORIES false
	"${CMAKE_SOURCE_DIR}/scripts/*.lua*")

foreach(_proj_source IN ITEMS ${SCRIPTS})
	get_filename_component(_source_path "${_proj_source}" PATH)
	file(RELATIVE_PATH _source_path_rel "${CMAKE_SOURCE_DIR}" "${_source_path}")
	source_group("${_source_path_rel}" FILES "${_proj_source}")
endforeach()

include_directories(${CMAKE_SOURCE_DIR}/includes
	${CMAKE_SOURCE_DIR}/glm/
	${SOIL_PATH}
	${PROJ_HEADERS}
	${IMGUI_PATH}
	${LUA_PATH}
	${VRPN_PATH})


# clang-format
if (UNIX)
	add_custom_target(
		clangformat
		COMMAND /usr/bin/clang-format
		-style=google
		-i
		${SOURCES} ${PROJECT_SOURCES}
	)
endif()

add_executable(DayDream_Engine
				${SOURCES} ${PROJECT_SOURCES} ${SHADER_SOURCES} ${IMGUI_SRC})

if(MSVC)
	# set stack size
	set_property(TARGET DayDream_Engine APPEND PROPERTY LINK_FLAGS /STACK:2000000)
endif()

# set visual studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			PROPERTY VS_STARTUP_PROJECT
			DayDream_Engine)
# add libraries
find_library(VRPN_LIB vrpn ${CMAKE_CURRENT_SOURCE_DIR}/vrpn_lib_files)
if(NOT VRPN_LIB)
  message(FATAL_ERROR "VRPN Release library not found")
endif()
if(UNIX)
	find_library(SOIL_LIB SOIL)
	if(NOT SOIL_LIB)
		message(FATAL_ERROR "SOIL library not found")
	endif()
	find_library(TTF_LIB freetype)
	if(NOT TTF_LIB)
		message(FATAL_ERROR "Freetype Release library not found")
	endif()
	find_library(FS_LIB stdc++fs /usr/lib/gcc/x86_64-linux-gnu/*)
	if(NOT FS_LIB)
		message(FATAL_ERROR "C++17 File System library not found")
	endif()
	find_library(LUA_LIB NAMES lua53 lua5.3 PATHS /usr/lib/x86_64-linux-gnu)
	if(NOT LUA_LIB)
		message(FATAL_ERROR "Lua library not found")
	endif()
elseif(MSVC)
	find_library(SOIL_LIB SOIL ${CMAKE_CURRENT_SOURCE_DIR}/SOIL/lib)
	if(NOT SOIL_LIB)
		message(FATAL_ERROR "SOIL library not found")
	endif()
	find_library(TTF_LIB freetype ${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.7/lib)
	if(NOT TTF_LIB)
		message(FATAL_ERROR "Freetype Release library not found")
	endif()
	find_library(LUA_LIB lua53 ${CMAKE_CURRENT_SOURCE_DIR}/lua53_Win64_lib)
	if(NOT LUA_LIB)
		message(FATAL_ERROR "Lua library not found")
	endif()
endif()
if(MINGW)
	find_library(FS_LIB stdc++fs ${MY_GCC_LIB})
	if(NOT FS_LIB)
		message(FATAL_ERROR "C++17 File System library not found")
	endif()
# elseif(WIN32)
# 	find_library(FS_LIB stdc++fs)
# 	if(NOT FS_LIB)
# 		message(FATAL_ERROR "C++17 File System library not found")
# 	endif()
endif()

#find_library(SDL2_DYNAMIC SDL2
#	PATHS
#	${SDL2_LIBRARY}
#	${SDL2_LIBRARY_TEMP})
#if(NOT SDL2_DYNAMIC)
#	message(FATAL_ERROR "NOOOOOOOO")
#4endif()

# filesystem goes last
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -lstdc++fs")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -lstdc++fs")

if(NOT WIN32)
	target_link_libraries(DayDream_Engine ${SOIL_LIB} ${OPENGL_gl_LIBRARY}
	${SDL2_LIBRARY} ${VRPN_LIB} ${FREETYPE_LIBRARIES} ${FS_LIB} ${LUA_LIB})
else()
	target_link_libraries(DayDream_Engine ${SOIL_LIB} ${OPENGL_gl_LIBRARY}
	${SDL2_LIBRARY} ${VRPN_LIB} ${FREETYPE_LIBRARIES} ${LUA_LIB})
endif()
