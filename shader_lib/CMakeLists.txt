cmake_minimum_required(VERSION 3.5.1)
project (ShaderReflection)
set(ShaderReflection_VERSION_MAJOR 1)
set(ShaderReflection_VERSION_MINOR 0)

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

# include directories for project
find_path(GLM_PATH glm.hpp
	PATHS
	${CMAKE_SOURCE_DIR}/../glm/glm)
if(NOT GLM_PATH)
	message(FATAL_ERROR "GLM includes not found")
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

# add graphics lib
find_library(GRAPHICS_LIB dd_graphics ${CMAKE_CURRENT_SOURCE_DIR}/../lib)
if(NOT GRAPHICS_LIB)
  message(FATAL_ERROR "dd_graphics library not found")
endif()

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
file(GLOB_RECURSE SOURCES 	"${CMAKE_SOURCE_DIR}/src/*.cpp"
														"${CMAKE_SOURCE_DIR}/utility/*.cpp"
														"${CMAKE_SOURCE_DIR}/include/*.h"
														"${CMAKE_SOURCE_DIR}/src/*.c")

# add defines for engine resources
add_definitions(
	"-DROOT_DIR=\"${CMAKE_SOURCE_DIR}/\""
	"-DSHADER_DIR=\"${CMAKE_SOURCE_DIR}/../Resource/Shaders/\""
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

# add shaders to project
file(GLOB_RECURSE SHADER_SOURCES
	LIST_DIRECTORIES false
	"${CMAKE_SOURCE_DIR}/../Resource/Shaders/*.vert*"
	"${CMAKE_SOURCE_DIR}/../Resource/Shaders/*.frag*"
	"${CMAKE_SOURCE_DIR}/../Resource/Shaders/*.geom*"
	"${CMAKE_SOURCE_DIR}/../Resource/Shaders/*.comp*")

foreach(_proj_source IN ITEMS ${SHADER_SOURCES})
	get_filename_component(_source_path "${_proj_source}" PATH)
	file(RELATIVE_PATH _source_path_rel "${CMAKE_SOURCE_DIR}" "${_source_path}")
	source_group("${_source_path_rel}" FILES "${_proj_source}")
endforeach()

# GLFW 3
# add include directory
find_path( GLFW_INCLUDE_DIR 
	NAMES
			GLFW/glfw3.h
	HINTS
			"${CMAKE_CURRENT_SOURCE_DIR}/../glfw/include"
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
# check if includes where found
if (GLFW_INCLUDE_DIR)
	include_directories(${GLFW_INCLUDE_DIR})
else()
	message(FATAL_ERROR "GLFW includes not found")
endif()

# add project
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${PROJECT_SOURCE_DIR}/../glfw
									"${CMAKE_CURRENT_BINARY_DIR}/glfwlib_build")

include_directories(${CMAKE_SOURCE_DIR}/include
										${CMAKE_SOURCE_DIR}/utility
										${CMAKE_SOURCE_DIR}/../graphics_lib/includes
										${CMAKE_SOURCE_DIR}/../glm/)

# clang-format
if (UNIX)
	add_custom_target(
		clangformat_shader
		COMMAND /usr/bin/clang-format
		-style=google
		-i
		${SOURCES}
	)
endif()

add_executable(dd_shader_reflect ${SOURCES} ${SHADER_SOURCES} )

# set visual studio startup project
set_property(DIRECTORY ${PROJECT_SOURCE_DIR}
			PROPERTY VS_STARTUP_PROJECT
			dd_shader_reflect)

target_link_libraries(dd_shader_reflect ${GRAPHICS_LIB} glfw ${OPENGL_gl_LIBRARY})