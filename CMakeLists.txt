cmake_minimum_required(VERSION 3.10)
include(CheckCXXCompilerFlag)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules/")

message("Generated with config types: ${CMAKE_CONFIGURATION_TYPES}")

# set the project name
project(Phyno VERSION 0.1)

#This is where you can set flags such as -Wall -pedantic etc...
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

# Add build directory to include path
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#A function created to perform the same as in set(CMAKE_CXX...
function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
  if(flag_already_set EQUAL -1)
   check_cxx_compiler_flag("${flag}" flag_supported)
   if(flag_supported)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
   endif()
   unset(flag_supported CACHE)
  endif()
endfunction()

find_package(PkgConfig REQUIRED)
pkg_check_modules(Mosquitto REQUIRED IMPORTED_TARGET libmosquitto)


enable_cxx_compiler_flag_if_supported("-Wall")
enable_cxx_compiler_flag_if_supported("-Wextra")
enable_cxx_compiler_flag_if_supported("-pedantic")

#this method points to the include folder for your header files
include_directories(include)

#add subdirectory for the src files
add_subdirectory(src)

enable_cxx_compiler_flag_if_supported("-O2")
enable_cxx_compiler_flag_if_supported("D_NDEBUG")

find_package(OpenSSL REQUIRED)

# Find all source files with file extension .c/.cpp inside src folder
file(GLOB_RECURSE SOURCES "src/*.cc" "src/*.cpp")


# Fetching external libraries
include(FetchContent)
FetchContent_Declare(
  Poco
  GIT_REPOSITORY https://github.com/pocoproject/poco
)

FetchContent_MakeAvailable(Poco)

#include(cmake_TBBGet.cmake)
#tbb_get(TBB_ROOT tbb_root CONFIG_DIR TBB_DIR)
find_package(TBB )

# find physx libraries
find_library(PHYSX_COMMON NAMES libPhysXCommon_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX NAMES libPhysX_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX_FOUNDATION NAMES libPhysXFoundation_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX_EXTENSIONS NAMES libPhysXExtensions_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX_PVSDK NAMES libPhysXPvdSDK_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX_COOKING NAMES libPhysXCooking_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX_CHARAC_CINE NAMES libPhysXCharacterKinematic_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)
find_library(PHYSX_VEHICULE NAMES libPhysXVehicle_static_64.a HINTS "${CMAKE_SOURCE_DIR}/libs/PhysX/bin/linux.clang/debug" REQUIRED)


# add the executable
add_executable(phyno ${SOURCES})

# Link everything
target_link_libraries(phyno
  PUBLIC Poco::Foundation 
  PUBLIC Poco::Util 
  PkgConfig::Mosquitto
  ${PHYSX}
  ${PHYSX_PVSDK}
  ${PHYSX_EXTENSIONS}
  ${PHYSX_COOKING}
  ${PHYSX_COMMON}
  ${PHYSX_FOUNDATION}

  TBB::tbb
  )

# Set the directories that should be included in the build command for this target
# when running g++ these will be included as -I/directory/path/
target_include_directories(phyno
    PRIVATE 
        ${PROJECT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}/_deps/poco-src/Util/include
        ${CMAKE_SOURCE_DIR}/libs/PhysX/include
        ${CMAKE_SOURCE_DIR}/libs/PxShared/include
        ${CMAKE_SOURCE_DIR}/libs/PhysX/include/extensions
)