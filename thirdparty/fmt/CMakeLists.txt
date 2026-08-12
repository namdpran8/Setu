add_library(windroid_fmt INTERFACE)
target_compile_definitions(windroid_fmt INTERFACE FMT_HEADER_ONLY)
target_include_directories(windroid_fmt INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
