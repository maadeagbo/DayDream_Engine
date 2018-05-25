# For 64-bit build---> cmake -G "Visual Studio 15 2017 Win64" ../
# to open solution--> start <project name>.sln
cmake_minimum_required(VERSION 3.5.1)

# use the file(GLOB...) or file(GLOB_RECURSE...) to grab wildcard src files
file(GLOB_RECURSE DD_FBX_SOURCES "${CMAKE_SOURCE_DIR}/fbx/src/*.cpp")
file(GLOB_RECURSE DD_FBX_HEADERS "${CMAKE_SOURCE_DIR}/fbx/include/*.h")

include_directories(${CMAKE_SOURCE_DIR}/fbx/fbx_include 
                    ${CMAKE_SOURCE_DIR}/fbx/include)

# move dll to bin and use
add_definitions("-DFBXSDK_SHARED")
if (WIN32)
  configure_file(${CMAKE_SOURCE_DIR}/fbx/lib/libfbxsdk.dll 
                ${CMAKE_SOURCE_DIR}/bin/libfbxsdk.dll COPYONLY)
  find_library(FBX_LIB libfbxsdk ${CMAKE_SOURCE_DIR}/fbx/lib)
  if(NOT FBX_LIB)
    message(FATAL_ERROR "FBX static library not found")
  endif() 
else()
  configure_file(${CMAKE_SOURCE_DIR}/fbx/lib/libfbxsdk.so
                ${CMAKE_SOURCE_DIR}/bin/libfbxsdk.so COPYONLY)
  set(FBX_LIB ${CMAKE_SOURCE_DIR}/bin/libfbxsdk.so)

  check_cxx_compiler_flag(-ldl HAS_LDL)
  if(NOT HAS_LDL)
      message(FATAL_ERROR "-ldl not found in system")
  endif()
endif()              

# add executable               
add_executable(fbx_converter ${DD_FBX_HEADERS} ${DD_FBX_SOURCES} 
               ${CMAKE_SOURCE_DIR}/src/Pow2Assert.cpp
               ${CMAKE_SOURCE_DIR}/src/StringLib.cpp)

if (WIN32)               
  target_link_libraries(fbx_converter ${FBX_LIB})
else()
  target_link_libraries(fbx_converter ${FBX_LIB} "-ldl")
endif()
