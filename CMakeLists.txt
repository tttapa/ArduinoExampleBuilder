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
# Add Google Test
################################################################################

# add_subdirectory(googletest)
# include_directories(${gmock_SOURCE_DIR}/include)
# include_directories(${gtest_SOURCE_DIR}/include)
# 
# include(GoogleTest)
# enable_testing()

################################################################################
# Change the default output directories
################################################################################

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################################################################
# Set the compiler options
################################################################################

set(CMAKE_CPP_FLAGS "${CMAKE_CPP_FLAGS} \
    -Wmissing-include-dirs")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
    -g3 -Og -Wall -Wextra -Werror \
    -fdiagnostics-show-option -Wsuggest-override \
    -Wdouble-promotion \
    -Wswitch-enum -Wimplicit-fallthrough=3 \
    -Wstrict-overflow=5 \
    -Wuninitialized \
    -pedantic -pedantic-errors")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -g3 -Og -Wall -Wextra -Werror")

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE) # LTO

################################################################################
# Find PThread
################################################################################

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

################################################################################
# Add {fmt} library 
################################################################################

add_subdirectory(lib/fmt EXCLUDE_FROM_ALL)

################################################################################
# Add the executable target
################################################################################

add_executable(arduino-example-builder ArduinoExampleBuilder.cpp)
target_include_directories(arduino-example-builder 
    PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(arduino-example-builder Threads::Threads fmt::fmt)