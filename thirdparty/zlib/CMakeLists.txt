add_library(setu_zlib STATIC
    adler32.c
    compress.c
    crc32.c
    deflate.c
    gzclose.c
    gzlib.c
    gzread.c
    gzwrite.c
    infback.c
    inffast.c
    inflate.c
    inftrees.c
    trees.c
    uncompr.c
    zutil.c
)
configure_file(zconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/zconf.h COPYONLY)
target_include_directories(setu_zlib PUBLIC . ${CMAKE_CURRENT_BINARY_DIR})
