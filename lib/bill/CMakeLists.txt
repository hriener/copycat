# Distributed under the MIT License (See accompanying file /LICENSE)
# CMake build : bill library
cmake_minimum_required(VERSION 3.8)
project(bill LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)

if(UNIX)
  include(CheckCXXCompilerFlag)
  # show quite some warnings (but remove some intentionally)
  add_compile_options(-Wall -Wextra)
  foreach (WARNING unknown-pragmas gnu-anonymous-struct nested-anon-types)
    check_cxx_compiler_flag("-Wno-${WARNING}" HAS_WNO_${WARNING})
    if (HAS_WNO_${WARNING})
      add_compile_options(-Wno-${WARNING})
    endif()
  endforeach()
endif()

add_subdirectory(include)
