add_library(setu_fmt INTERFACE)
target_compile_definitions(setu_fmt INTERFACE FMT_HEADER_ONLY)
target_include_directories(setu_fmt INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
