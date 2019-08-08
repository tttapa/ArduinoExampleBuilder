cmake_minimum_required(VERSION 3.10)

################################################################################
# Set the C++ standard to C++17 and use GCC 9 if available
################################################################################

set(CMAKE_CXX_STANDARD 17)

find_program(GCC_9_CMAKE_C_COMPILER "gcc-9")
find_program(GCC_9_CMAKE_CXX_COMPILER "g++-9")

if(NOT GCC_9_CMAKE_C_COMPILER OR NOT GCC_9_CMAKE_CXX_COMPILER)
    message(WARNING "GCC 9 not found")
else()
    message(STATUS "GCC 9 found")
    set(CMAKE_C_COMPILER ${GCC_9_CMAKE_C_COMPILER})
    set(CMAKE_CXX_COMPILER ${GCC_9_CMAKE_CXX_COMPILER})
endif()

project(ArduinoExampleBuilder)

################################################################################
# Select a default build type if none was specified
################################################################################

set(default_build_type "Release")
if(EXISTS "${CMAKE_SOURCE_DIR}/.git")
    set(default_build_type "Debug")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
    "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

################################################################################
# Change the default output directories
################################################################################

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################################################################
# Set the compiler options
################################################################################

# Basic flags for all builds (warnings)
set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} \
    -Wmissing-include-dirs")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -Wall -Wextra -Werror \
    -fdiagnostics-show-option -Wsuggest-override \
    -Wdouble-promotion \
    -Wswitch-enum -Wimplicit-fallthrough=3 \
    -Wuninitialized \
    -pedantic -pedantic-errors")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall -Wextra -Werror")

# Optimization flags for release
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} \
    -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} \
    -O3")
if (CMAKE_BUILD_TYPE STREQUAL "RELEASE")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO
endif()

# Debugging and optimization flags for debug
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} \
    -Og -g3")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} \
    -Og -g3")

################################################################################
# Find PThread
################################################################################

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

################################################################################
# Find OpenSSL
################################################################################

find_package(OpenSSL REQUIRED)

################################################################################
# Add {fmt} library 
################################################################################

add_subdirectory(lib/fmt EXCLUDE_FROM_ALL)

################################################################################
# Add the executable target
################################################################################

add_subdirectory(src)

################################################################################
# Add Google Test
################################################################################

# add_subdirectory(googletest)
# include_directories(${gmock_SOURCE_DIR}/include)
# include_directories(${gtest_SOURCE_DIR}/include)
# 
# include(GoogleTest)
# enable_testing()