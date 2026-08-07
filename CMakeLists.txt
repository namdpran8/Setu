cmake_minimum_required(VERSION 3.20)
project(winandroid_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty/miniz)

add_executable(winandroid_runtime

    src/main.cpp
    "src/apk_extractor/ApkExtractor.cpp"
)

target_include_directories(winandroid_runtime PRIVATE include)