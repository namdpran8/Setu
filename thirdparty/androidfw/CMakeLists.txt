add_library(setu_androidfw STATIC
    ApkAssets.cpp
    Asset.cpp
    AssetManager2.cpp
    AssetsProvider.cpp
    AttributeResolution.cpp
    ChunkIterator.cpp
    ConfigDescription.cpp
    Idmap.cpp
    LoadedArsc.cpp
    LocaleData.cpp
    LocaleDataLookup.cpp
    misc.cpp
    ResourceTypes.cpp
    ResourceUtils.cpp
    StreamingZipInflater.cpp
    StringPool.cpp
    TypeWrappers.cpp
    Util.cpp
    ZipFileRO.cpp
    ZipUtils.cpp
)
target_include_directories(setu_androidfw PUBLIC "${CMAKE_SOURCE_DIR}/thirdparty/androidfw" "${CMAKE_SOURCE_DIR}/thirdparty/include_compat" incfs_support/include)
target_compile_options(setu_androidfw PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
target_compile_options(setu_androidfw PRIVATE /utf-8)
target_link_libraries(setu_androidfw PUBLIC setu_libbase setu_libziparchive setu_fmt setu_zlib)

add_executable(test_androidfw test_androidfw.cpp)
target_include_directories(test_androidfw PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/androidfw")
target_compile_options(test_androidfw PRIVATE /FI"${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h" /utf-8)
target_link_libraries(test_androidfw PRIVATE setu_androidfw setu_libbase setu_libziparchive setu_fmt setu_zlib)
