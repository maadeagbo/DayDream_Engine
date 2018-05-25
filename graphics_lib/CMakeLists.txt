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
set(DD_GRAPHICS_SRC "${CMAKE_SOURCE_DIR}/graphics_lib/source/*.cpp"
										"${CMAKE_SOURCE_DIR}/graphics_lib/source/*.c")

include_directories("${CMAKE_SOURCE_DIR}/graphics_lib/include"
										"${CMAKE_SOURCE_DIR}/imgui")

file(GLOB_RECURSE DD_GRAPHICS_INC "${CMAKE_SOURCE_DIR}/graphics_lib/include/*.h")
file(GLOB IMGUI_SRC "${CMAKE_SOURCE_DIR}/imgui/imgui_draw.cpp"
										"${CMAKE_SOURCE_DIR}/imgui/imgui_tabs.cpp"
										"${CMAKE_SOURCE_DIR}/imgui/imgui.cpp")

# choose glfw or sdl implementation
if (DD_USE_GLFW)
	if (NOT DD_GRAPHICS_API_VULKAN)
		# opengl
		set(GUI_SRC ${CMAKE_SOURCE_DIR}/graphics_lib/gui_lib/imgui_impl_glfw_gl3.h
								${CMAKE_SOURCE_DIR}/graphics_lib/gui_lib/imgui_impl_glfw_gl3.cpp
								${IMGUI_SRC})
	else()
		# vulkan
	endif()
else()
	# only opengl3 gui w/ sdl2 windowing
	if (NOT DD_GRAPHICS_API_VULKAN)
		set(GUI_SRC ${CMAKE_SOURCE_DIR}/graphics_lib/gui_lib/imgui_impl_sdl_gl3.h
								${CMAKE_SOURCE_DIR}/graphics_lib/gui_lib/imgui_impl_sdl_gl3.cpp
								${IMGUI_SRC})
	endif()
endif()

# libraries for shader and graphics layer
if (NOT DD_GRAPHICS_API_VULKAN)
	add_library(dd_shader STATIC
							${CMAKE_SOURCE_DIR}/graphics_lib/source/ddShader_OGL.cpp
							${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.c)
	add_library(dd_graphics STATIC
							${CMAKE_SOURCE_DIR}/graphics_lib/source/Graphics_OGL.cpp
							${CMAKE_SOURCE_DIR}/graphics_lib/source/gl_core_4_3.c
							${GUI_SRC})
endif()

if (DD_USE_GLFW)
	# Shader reflection executable
	add_executable(shader_reflect 
		${CMAKE_SOURCE_DIR}/graphics_lib/source/reflect_main.cpp
		${CMAKE_SOURCE_DIR}/src/Pow2Assert.cpp
		${CMAKE_SOURCE_DIR}/src/StringLib.cpp)
	target_link_libraries(shader_reflect glfw dd_graphics dd_shader 
												${OPENGL_gl_LIBRARY})
else()
endif()