cmake_minimum_required(VERSION 3.5.1)

# find_path(IMGUI_PATH imgui.h
# 	PATHS
# 	${CMAKE_CURRENT_SOURCE_DIR}/include)
# if(NOT IMGUI_PATH)
# 	message(FATAL_ERROR "IMGUI_PATH IMGUI include not found")
# endif()

# OpenGL
include(FindPackageHandleStandardArgs)
find_package(OpenGL)

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
set(DD_GRAPHICS_SRC "${CMAKE_SOURCE_DIR}/graphics_lib/source/Graphics_OGL.cpp")

include_directories(${CMAKE_SOURCE_DIR}/graphics_lib/include)

file(GLOB_RECURSE DD_GRAPHIC_INC "${CMAKE_SOURCE_DIR}/graphics_lib/include/*.h")

# clang-format
if (UNIX)
	add_custom_target(
		clangformat_gpu
		COMMAND /usr/bin/clang-format
		-style=google
		-i
		${DD_GRAPHIC_INC} ${DD_GRAPHICS_SRC}
	)
endif()

if (DD_USE_GLFW)
	set(IMGUI_INC ${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui_impl_glfw_gl3.cpp)
else()
	set(IMGUI_INC ${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui_impl_sdl_gl3.cpp)
endif()

if (NOT DD_GRAPHICS_API_VULKAN)
	add_library(dd_shader STATIC
							${CMAKE_SOURCE_DIR}/graphics_lib/source/ddShader_OGL.cpp
							${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.c
							${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.h)
	add_library(dd_graphics STATIC
							${CMAKE_SOURCE_DIR}/graphics_lib/include/GPUFrontEnd.h
							${CMAKE_SOURCE_DIR}/graphics_lib/source/Graphics_OGL.cpp
							${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.c
							${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.h
							${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui.cpp
							${CMAKE_SOURCE_DIR}/graphics_lib/source/imgui_draw.cpp
							${IMGUI_INC})
endif()
#install(TARGETS dd_graphics DESTINATION lib)