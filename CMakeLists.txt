cmake_minimum_required(VERSION 3.20)
project(setu_runtime LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(thirdparty/miniz)
add_subdirectory(thirdparty/libbase)
add_subdirectory(thirdparty/zlib)
add_subdirectory(thirdparty/libziparchive)
add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/androidfw)
add_subdirectory(src/cassowary)

add_executable(setu_runtime

    src/main.cpp
    "src/apk_extractor/ApkExtractor.cpp"
    "src/dex/DexParser.h" "src/dex/DexParser.cpp"
    "src/dex/MultiDexManager.cpp"
    "src/dex/ResourceManager.cpp"
    "src/utils/Logger.cpp"
    "src/interpreter/Interpreter.cpp"
    "src/dalvik_opcodes.cpp"
    "src/interpreter/StubRegistry.cpp"
    "src/ui/WindowManager.cpp"
    "src/ui/Theme.cpp"
    "src/ui/TypedArray.cpp"
    "src/ui/LayoutInflater.cpp"
    "src/graphics/Paint.cpp"
    "src/graphics/Direct2DCanvas.cpp"
    "src/graphics/RecordingCanvas.cpp"
    "src/view/View.cpp"
    "src/view/ViewGroup.cpp"
    "src/view/Choreographer.cpp"
    "src/view/LinearLayout.cpp"
    "src/view/FrameLayout.cpp"
    "src/view/OverlayPanelLayout.cpp"
    "src/view/RelativeLayout.cpp"
    "src/view/ConstraintLayout.cpp"
    "src/view/GridLayout.cpp"
    "src/widget/TextView.cpp"
    "src/widget/Button.cpp"
    "src/widget/EditText.cpp"
    "src/widget/ImageView.cpp"
    "src/widget/ImageButton.cpp")

target_include_directories(setu_runtime PRIVATE include)
target_link_libraries(setu_runtime PRIVATE miniz d2d1 dwrite d3d11 dxgi setu_androidfw cassowary)
target_compile_options(setu_runtime PRIVATE "/utf-8" "/FI" "${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")
