cmake_minimum_required(VERSION 3.5.1)
project (DayDream_Engine_alpha)
set(DayDream_Engine_alpha_VERSION_MAJOR 3)
set(DayDream_Engine_alpha_VERSION_MINOR 1)

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
# for multi-config builds (e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
		set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/bin )
		set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/lib )
		set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} 
				${PROJECT_SOURCE_DIR}/lib )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

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
set (CMAKE_CXX_STANDARD 11)

option(USE_STATIC_LINKAGE "Use static libraries when linking executable"	ON)
if(USE_STATIC_LINKAGE)
	# statically link to reduce dependancies
	foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE 
					CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS
					CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL 
					CMAKE_C_FLAGS_RELWITHDEBINFO )
		if(${flag_var} MATCHES "/MD")
			STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/MD")
		if(${flag_var} MATCHES "/MDd")
			STRING(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
		endif(${flag_var} MATCHES "/MDd")
	endforeach(flag_var)
endif()

# sets module path
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# add paths to freetype lib and sdl2
if(NOT UNIX)
	set(ENV{FREETYPE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.7)
	set(ENV{BULLET_ROOT} ${CMAKE_CURRENT_SOURCE_DIR}/bullet3_287)
endif()

# set windowing environment
set(DD_USE_GLFW ON CACHE BOOL "Sets engine to use GLFW3 instead of SDL2" FORCE)

if(NOT DD_USE_GLFW)
	if (MSVC)
		# add path to SDL2
		set(ENV{SDL2DIR} ${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.7)
	endif()
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIR})
else()
	# GLFW 3
	# add include directory
	find_path( GLFW_INCLUDE_DIR 
    NAMES
        GLFW/glfw3.h
    HINTS
        "${CMAKE_CURRENT_SOURCE_DIR}/glfw/include"
    PATHS
        "$ENV{PROGRAMFILES}/GLFW/include"
        "${OPENGL_INCLUDE_DIR}"
        /usr/openwin/share/include
        /usr/openwin/include
        /usr/X11R6/include
        /usr/include/X11
        /opt/graphics/OpenGL/include
        /opt/graphics/OpenGL/contrib/libglfw
        /usr/local/include
        /usr/include/GL
        /usr/include
    DOC 
        "The directory where GLFW/glfw3.h resides"
	)
	# check if include where found
	if (GLFW_INCLUDE_DIR)
		include_directories(${GLFW_INCLUDE_DIR})
	else()
		message(FATAL_ERROR "GLFW include not found")
	endif()
	# add project
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
	add_subdirectory(${PROJECT_SOURCE_DIR}/glfw)
endif()

# add bullet library
if(MSVC)
	add_subdirectory(${PROJECT_SOURCE_DIR}/bullet3_287)
endif()

# include directories for project
find_path(GLM_PATH glm.hpp
	PATHS
	${CMAKE_SOURCE_DIR}/glm/glm)
if(NOT GLM_PATH)
	message(FATAL_ERROR "GLM include not found")
endif()
find_path(BULLET_PATH 
	NAMES btBulletCollisionCommon.h btBulletDynamicsCommon.h
	PATHS
	${CMAKE_SOURCE_DIR}/bullet3_287/src)
if(NOT BULLET_PATH)
	message(FATAL_ERROR "BULLET3 include not found")
endif()
find_path(VRPN_PATH vrpn_Tracker.h
	PATHS
	${CMAKE_CURRENT_SOURCE_DIR}/vrpn_lib_files)
if(NOT VRPN_PATH)
	message(FATAL_ERROR "VRPN_PATH VRPN include not found")
endif()

find_path(EIGEN_PATH Eigen/Core
	PATHS
	${CMAKE_CURRENT_SOURCE_DIR}/eigen)
if(NOT EIGEN_PATH)
	message(FATAL_ERROR "EIGEN_PATH Eigen Core include not found")
else()
	message(STATUS "EIGEN_PATH Eigen Core include found")
endif()

if(NOT UNIX)
	find_path(LUA_PATH lua.hpp PATHS ${CMAKE_SOURCE_DIR}/lua53_Win64_lib/include)
	if(NOT LUA_PATH)
		message(FATAL_ERROR "Lua include not found")
	endif()
else()
	find_path(LUA_PATH lua.hpp PATHS /usr/include/lua5.3)
	if(NOT LUA_PATH)
		message(FATAL_ERROR "Lua include not found")
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

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
file(GLOB_RECURSE SOURCES 	"${CMAKE_SOURCE_DIR}/src/*.cpp"
							"${CMAKE_SOURCE_DIR}/include/*.hpp"
							"${CMAKE_SOURCE_DIR}/include/*.h"
							"${CMAKE_SOURCE_DIR}/src/*.c")
# file(GLOB IMGUI_SRC "${CMAKE_SOURCE_DIR}/imgui/*.cpp")

# add defines for engine resources
add_definitions(
	"-DROOT_DIR=\"${CMAKE_SOURCE_DIR}\""
	"-DRESOURCE_DIR=\"${CMAKE_SOURCE_DIR}/Resource\""
	"-DSHADER_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Shaders\""
	"-DMESH_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Meshes\""
	"-DTEX_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Textures\""
	"-DPROJECT_DIR=\"${CMAKE_SOURCE_DIR}/Projects\""
	"-DFONTS_DIR=\"${CMAKE_SOURCE_DIR}/Resource/Fonts\""
)
if (MSVC)
	# warning that pop up for strtok & fopen & std::copy
	add_definitions("-D_SCL_SECURE_NO_WARNINGS")
	add_definitions("-D_CRT_SECURE_NO_WARNINGS")
endif()
if (DD_GRAPHICS_API_VULKAN)
	add_definitions("-DDD_GRAPHICS_API=1")
else()
	add_definitions("-DDD_GRAPHICS_API=0")
endif()

# use file(GLOB_RECURSE...), add_library(dd_engine...), and
# source_group(<group name> FILES...) to create folder structure in project
# LIST_DIRECTORIES remove directory name from files
file(GLOB_RECURSE PROJECT_SOURCES
	LIST_DIRECTORIES false
    "${CMAKE_SOURCE_DIR}/Projects/*.cpp*"
    "${CMAKE_SOURCE_DIR}/src/AssetViewer/*.cpp*"
		"${CMAKE_SOURCE_DIR}/src/AssetViewer/*.h*"
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

include_directories(${CMAKE_SOURCE_DIR}/include
	${PROJECT_SOURCE_DIR}/graphics_lib/include
	${CMAKE_SOURCE_DIR}/glm/
	${PROJ_HEADERS}
	${LUA_PATH}
	${BULLET_PATH}
	${EIGEN_PATH}
	${VRPN_PATH})

#  add graphic library and set API
set(DD_GRAPHICS_API_VULKAN OFF CACHE BOOL "Sets engine to use vulkan api" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/graphics_lib)
#include(${PROJECT_SOURCE_DIR}/graphics_lib/CMakeLists.txt)

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

# add fbx library
add_subdirectory(${PROJECT_SOURCE_DIR}/fbx)

# # Shader reflection executable
# add_executable(dd_shader_reflect 
# 	${CMAKE_SOURCE_DIR}/graphics_lib/source/reflect_main.cpp
# 	${CMAKE_SOURCE_DIR}/src/Pow2Assert.cpp)
# target_link_libraries(dd_shader_reflect 
# 	glfw dd_graphics dd_shader ${OPENGL_gl_LIBRARY})

add_custom_target(
	build_reflection
	COMMAND ${PROJECT_SOURCE_DIR}/create_shader_enums.sh 
		${PROJECT_SOURCE_DIR}/ ${PROJECT_SOURCE_DIR}/
	DEPENDS shader_reflect
)

# Engine executable
add_executable(dd_engine 
				${SOURCES} ${PROJECT_SOURCES} ${SHADER_SOURCES}
				${SCRIPTS})
# make sure every other executable and .h file is built before the engine
add_dependencies(dd_engine shader_reflect fbx_converter build_reflection)


if(MSVC)
	# set stack size (too small on windows for lua)
	set_property(TARGET dd_engine APPEND PROPERTY LINK_FLAGS /STACK:2000000)
endif()


# add libraries
find_library(VRPN_LIB vrpn ${CMAKE_CURRENT_SOURCE_DIR}/vrpn_lib_files)
if(NOT VRPN_LIB)
  message(FATAL_ERROR "VRPN Release library not found")
endif()
if(UNIX)
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
	find_library(TTF_LIB freetype ${CMAKE_CURRENT_SOURCE_DIR}/freetype-2.7/lib)
	if(NOT TTF_LIB)
		message(FATAL_ERROR "Freetype Release library not found")
	endif()
	find_library(LUA_LIB lua53 ${CMAKE_CURRENT_SOURCE_DIR}/lua53_Win64_lib)
	if(NOT LUA_LIB)
		message(FATAL_ERROR "Lua library not found")
	endif()
endif()

# bullet physics and graphics libraries
if(MSVC)
	set(BULLET_LIB 
		${PROJECT_SOURCE_DIR}/lib/BulletDynamics.lib
		${PROJECT_SOURCE_DIR}/lib/BulletCollision.lib
		${PROJECT_SOURCE_DIR}/lib/LinearMath.lib
		${PROJECT_SOURCE_DIR}/lib/BulletSoftBody.lib
	)
else()
	set(BULLET_LIB 
		${PROJECT_SOURCE_DIR}/lib/libBulletDynamics.a
		${PROJECT_SOURCE_DIR}/lib/libBulletCollision.a
		${PROJECT_SOURCE_DIR}/lib/libLinearMath.a
		${PROJECT_SOURCE_DIR}/lib/libBulletSoftBody.a
	)
	find_library(BL_1 BulletDynamics)
	if(NOT BL_1)
		message(FATAL_ERROR "BulletDynamics library not found")
	endif()
	find_library(BL_2 BulletCollision)
	if(NOT BL_2)
		message(FATAL_ERROR "BulletCollision library not found")
	endif()
	find_library(BL_3 LinearMath)
	if(NOT BL_3)
		message(FATAL_ERROR "LinearMath library not found")
	endif()
	find_library(BL_4 BulletSoftBody)
	if(NOT BL_4)
		message(FATAL_ERROR "BulletSoftBody library not found")
	endif()
	set(BULLET_LIB ${BL_1} ${BL_2} ${BL_3} ${BL_4})
endif()

# set visual studio startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR}
			PROPERTY VS_STARTUP_PROJECT
			dd_engine)

# set windowing library
if(DD_USE_GLFW)
	set(WIN_LIB glfw)
else()
	set(WIN_LIB ${SDL2_LIBRARY})
endif()

if(NOT WIN32)
	# filesystem goes last
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -lstdc++fs")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -lstdc++fs")

	target_link_libraries(dd_engine ${WIN_LIB} dd_graphics dd_shader 
		${OPENGL_gl_LIBRARY} ${BULLET_LIB} ${VRPN_LIB} ${FREETYPE_LIBRARIES} 
		${FS_LIB} ${DD_GPULIB} ${LUA_LIB})
else()
	target_link_libraries(dd_engine ${WIN_LIB} dd_graphics dd_shader 
		${OPENGL_gl_LIBRARY} ${BULLET_LIB} ${VRPN_LIB} ${FREETYPE_LIBRARIES} 
		${LUA_LIB} ${DD_GPULIB})
endif()