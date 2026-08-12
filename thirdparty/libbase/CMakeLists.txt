add_library(windroid_libbase STATIC
    file.cpp
    logging.cpp
    mapped_file.cpp
    stringprintf.cpp
    strings.cpp
    utf8.cpp
    errors_windows.cpp
    posix_strerror_r.cpp
)
target_include_directories(windroid_libbase PUBLIC include)
target_compile_options(windroid_libbase PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
target_link_libraries(windroid_libbase PUBLIC windroid_fmt)
