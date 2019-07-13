add_library(copycat INTERFACE)
target_include_directories(copycat INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(copycat INTERFACE bill ez kitty mockturtle lorina sparsepp json)
