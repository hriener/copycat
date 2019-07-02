cmake_minimum_required(VERSION 3.8)
project(copycat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(COPYCAT_TEST "Build tests" OFF)
option(COPYCAT_EXAMPLES "Build examples" ON)

if(UNIX)
  include(CheckCXXCompilerFlag)
  add_compile_options(-W -Wall -Wextra)

  # turn off region warnings from mockturtle
  foreach (WARNING unknown-pragmas gnu-anonymous-struct nested-anon-types)
    check_cxx_compiler_flag("-Wno-${WARNING}" HAS_WNO_${WARNING})
    if (HAS_WNO_${WARNING})
      add_compile_options(-Wno-${WARNING})
    endif()
  endforeach()
endif()

add_subdirectory(lib)
add_subdirectory(include)

if(COPYCAT_EXAMPLES)
  add_subdirectory(examples)
endif()

if(COPYCAT_TEST)
  add_subdirectory(test)
endif()
