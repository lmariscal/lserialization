cmake_minimum_required(VERSION 3.11)

project(test CXX)

if(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

add_subdirectory(third)
add_subdirectory(src)
