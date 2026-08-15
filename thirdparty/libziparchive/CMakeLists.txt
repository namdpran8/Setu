add_library(setu_libziparchive STATIC
    zip_archive_stream_entry.cc
    zip_archive.cc
    zip_cd_entry_map.cc
    zip_writer.cc
    zip_error.cpp
)
target_include_directories(setu_libziparchive PUBLIC include "${CMAKE_SOURCE_DIR}/thirdparty/include_compat" incfs_support/include)
target_compile_options(setu_libziparchive PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
target_link_libraries(setu_libziparchive PUBLIC setu_libbase setu_zlib)

add_executable(test_libziparchive test_zip.cpp)
target_compile_options(test_libziparchive PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
target_link_libraries(test_libziparchive PRIVATE setu_libziparchive setu_libbase setu_zlib)
