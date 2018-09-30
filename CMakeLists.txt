cmake_minimum_required(VERSION 3.8)
project(copycat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(COPYCAT_TEST "Build tests" OFF)

if(UNIX)
  include(CheckCXXCompilerFlag)
  add_compile_options(-W -Wall -Wextra)
endif()

add_subdirectory(include)
add_subdirectory(lib)

if(COPYCAT_TEST)
  add_subdirectory(test)
endif()
