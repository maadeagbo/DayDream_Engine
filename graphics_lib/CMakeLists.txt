cmake_minimum_required(VERSION 3.5.1)
project (Graphics_Lib)
set(Graphics_Lib_VERSION_MAJOR 1)
set(Graphics_Lib_VERSION_MINOR 0)

# set release or debug builds
if(CMAKE_CONFIGURATION_TYPES)
   set(CMAKE_CONFIGURATION_TYPES Release RelWithDebInfo Debug)
   set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
     "Reset the configurations to what we need"
     FORCE)
 endif()
set(CMAKE_CXX_STANDARD 11)


option(USE_OPENGL "Either OpenGL or Vulkan"	ON)


# find_path(IMGUI_PATH imgui.h
# 	PATHS
# 	${CMAKE_CURRENT_SOURCE_DIR}/includes)
# if(NOT IMGUI_PATH)
# 	message(FATAL_ERROR "IMGUI_PATH IMGUI includes not found")
# endif()

# OpenGL
include(FindPackageHandleStandardArgs)
find_package(OpenGL)

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
file(GLOB_RECURSE GPU_LIB_SOURCES "${CMAKE_SOURCE_DIR}/graphics_lib/includes/*.h"
																	"${CMAKE_SOURCE_DIR}/graphics_lib/source/*.h"
																	"${CMAKE_SOURCE_DIR}/graphics_lib/source/*.cpp"
																	"${CMAKE_SOURCE_DIR}/graphics_lib/source/*.c")

include_directories(${CMAKE_SOURCE_DIR}/graphics_lib/includes)

file(GLOB_RECURSE DD_GRAPHIC_INC "${CMAKE_SOURCE_DIR}/graphics_lib/includes/*.h")

# clang-format
# if (UNIX)
# 	add_custom_target(
# 		clangformat
# 		COMMAND /usr/bin/clang-format
# 		-style=google
# 		-i
# 		${GPU_LIB_SOURCES}
# 	)
# endif()

# add_executable(dd_graphics ${GPU_LIB_SOURCES})
if (USE_OPENGL)
	add_library(dd_graphics STATIC ${GPU_LIB_SOURCES})
							# ${CMAKE_SOURCE_DIR}/graphics_lib/source/Graphics_OGL.cpp
							# ${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.c
							# ${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.h
							# ${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui.cpp
							# ${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui_draw.cpp
							# ${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui_impl_sdl_gl3.cpp)
endif()
#install(TARGETS dd_graphics DESTINATION lib)
