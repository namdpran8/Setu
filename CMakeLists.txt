cmake_minimum_required(VERSION 3.20)
project(setu_runtime LANGUAGES C CXX)

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
    "src/ui/AndroidAttrs.cpp"
    "src/ui/XmlAttrs.cpp"
    "src/ui/StateSetInflater.cpp"
    "src/ui/DrawableInflater.cpp"
    "src/ui/ColorStateListInflater.cpp"
    "src/graphics/Paint.cpp"
    "src/graphics/Path.cpp"
    "src/graphics/ColorStateList.cpp"
    "src/graphics/FontManager.cpp"
    "src/graphics/Direct2DCanvas.cpp"
    "src/graphics/RecordingCanvas.cpp"
    "src/graphics/drawable/Drawable.cpp"
    "src/graphics/drawable/ColorDrawable.cpp"
    "src/graphics/drawable/GradientDrawable.cpp"
    "src/graphics/drawable/StateSet.cpp"
    "src/graphics/drawable/DrawableContainer.cpp"
    "src/graphics/drawable/StateListDrawable.cpp"
    "src/graphics/drawable/RippleDrawable.cpp"
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

add_executable(constraint_layout_test src/test_constraint_layout.cpp
    "src/utils/Logger.cpp"
    "src/view/View.cpp"
    "src/view/ViewGroup.cpp"
    "src/view/ConstraintLayout.cpp"
    "src/graphics/Paint.cpp"
    "src/graphics/Path.cpp"
    "src/graphics/RecordingCanvas.cpp"
    "src/graphics/drawable/Drawable.cpp"
    "src/graphics/drawable/ColorDrawable.cpp"
)
target_include_directories(constraint_layout_test PRIVATE include)
target_link_libraries(constraint_layout_test PRIVATE miniz d2d1 dwrite d3d11 dxgi setu_androidfw cassowary)
target_compile_options(constraint_layout_test PRIVATE "/utf-8" "/FI" "${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")


