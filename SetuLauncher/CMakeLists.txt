cmake_minimum_required(VERSION 3.20)
project(SetuLauncher CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(SetuLauncher WIN32
    src/main.cpp
    src/WindowManager.cpp
)

target_include_directories(SetuLauncher PRIVATE include)

target_link_libraries(SetuLauncher PRIVATE user32 gdi32 shell32)
