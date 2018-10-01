if (NOT TARGET ez)
  add_library(ez INTERFACE)
  target_include_directories(ez INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ez/include)
endif()

if (NOT TARGET kitty)
  add_library(kitty INTERFACE)
  target_include_directories(kitty INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/kitty/include)
endif()

if (NOT TARGET mockturtle)
  add_library(mockturtle INTERFACE)
  target_include_directories(mockturtle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/mockturtle/include)
endif()

if (NOT TARGET sparsepp)
  add_library(sparsepp INTERFACE)
  target_include_directories(sparsepp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/sparsepp/include)
endif()
