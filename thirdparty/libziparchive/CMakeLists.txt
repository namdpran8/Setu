add_library(windroid_libziparchive STATIC
    zip_archive_stream_entry.cc
    zip_archive.cc
    zip_cd_entry_map.cc
    zip_writer.cc
    zip_error.cpp
)
target_include_directories(windroid_libziparchive PUBLIC include "${CMAKE_SOURCE_DIR}/thirdparty/include_compat" incfs_support/include)
target_compile_options(windroid_libziparchive PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
target_link_libraries(windroid_libziparchive PUBLIC windroid_libbase windroid_zlib)

add_executable(test_libziparchive test_zip.cpp)
target_compile_options(test_libziparchive PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
target_link_libraries(test_libziparchive PRIVATE windroid_libziparchive windroid_libbase windroid_zlib)
