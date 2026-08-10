cmake_minimum_required(VERSION 3.20)
project(winandroid_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty/miniz)

add_executable(winandroid_runtime

    src/main.cpp
    "src/apk_extractor/ApkExtractor.cpp"
    "src/AxmlPraserer/AxmlParser.cpp"
    "src/dex/DexParser.h" "src/dex/DexParser.cpp"
    "src/dex/MultiDexManager.cpp"
    "src/dex/ArscParser.cpp"
    "src/dex/ResourceManager.cpp"
    "src/utils/Logger.cpp"
    "src/interpreter/Interpreter.cpp"
    "src/interpreter/StubRegistry.cpp"
    "src/ui/WindowManager.cpp"
    "src/ui/LayoutInflater.cpp"
    "src/graphics/Paint.cpp"
    "src/graphics/Direct2DCanvas.cpp"
    "src/graphics/RecordingCanvas.cpp"
    "src/view/View.cpp"
    "src/view/ViewGroup.cpp"
    "src/view/Choreographer.cpp"
    "src/view/LinearLayout.cpp"
    "src/view/FrameLayout.cpp"
    "src/view/RelativeLayout.cpp"
    "src/view/ConstraintLayout.cpp"
    "src/widget/TextView.cpp"
    "src/widget/Button.cpp"
    "src/widget/EditText.cpp")

target_include_directories(winandroid_runtime PRIVATE include)
target_link_libraries(winandroid_runtime PRIVATE miniz d2d1 dwrite d3d11 dxgi)