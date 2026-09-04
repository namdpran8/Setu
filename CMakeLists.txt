cmake_minimum_required(VERSION 3.20)
project(setu_runtime LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    add_compile_options("/FS")
endif()

add_subdirectory(thirdparty/miniz)
add_subdirectory(thirdparty/libbase)
add_subdirectory(thirdparty/zlib)
add_subdirectory(thirdparty/libziparchive)
add_subdirectory(thirdparty/fmt)
add_subdirectory(thirdparty/androidfw)
add_subdirectory(src/cassowary)



add_library(skia STATIC IMPORTED GLOBAL)
set_target_properties(skia PROPERTIES
    IMPORTED_LOCATION_DEBUG "${CMAKE_SOURCE_DIR}/thirdparty/skia_build/skia/out/Static/skia.lib"
    IMPORTED_LOCATION_RELEASE "${CMAKE_SOURCE_DIR}/thirdparty/skia_build/skia/out/Static/skia_release.lib"
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/thirdparty/skia_build/skia/out/Static/skia.lib" # Default fallback
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/skia_build/skia"
)

add_executable(setu_runtime
    src/resource.rc
    src/main.cpp
    "src/apk_extractor/ApkExtractor.cpp"
    "src/dex/DexParser.h" "src/dex/DexParser.cpp"
    "src/dex/MultiDexManager.cpp"
    "src/dex/ResourceManager.cpp"
    "src/utils/Logger.cpp"
    "src/interpreter/Interpreter.cpp"
    "src/dalvik_opcodes.cpp"
    "src/interpreter/StubRegistry.cpp"
    "src/Permission/PermissionManager.cpp"
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
    "src/graphics/Bitmap.cpp"
    "src/graphics/BitmapFactory.cpp"
    "src/graphics/ColorStateList.cpp"
    "src/graphics/FontManager.cpp"
    "src/graphics/Direct2DCanvas.cpp"
    "src/graphics/SkiaCanvas.cpp"
    "src/graphics/RecordingCanvas.cpp"
    "src/graphics/drawable/Drawable.cpp"
    "src/graphics/drawable/ColorDrawable.cpp"
    "src/graphics/drawable/GradientDrawable.cpp"
    "src/graphics/drawable/LayerDrawable.cpp"
    "src/graphics/drawable/LevelListDrawable.cpp"
    "src/graphics/drawable/VectorDrawable.cpp"
    "src/graphics/drawable/StateSet.cpp"
    "src/graphics/drawable/DrawableContainer.cpp"
    "src/graphics/drawable/StateListDrawable.cpp"
    "src/graphics/drawable/InsetDrawable.cpp"
    "src/graphics/drawable/RippleDrawable.cpp"
    "src/graphics/drawable/BitmapDrawable.cpp"
    "src/graphics/drawable/NinePatchDrawable.cpp"
    "src/os/Looper.cpp"
    "src/os/MessageQueue.cpp"
    "src/os/Handler.cpp"
    "src/os/Looper.cpp"
    "src/os/MessageQueue.cpp"
    "src/os/Handler.cpp"
    "src/view/View.cpp"
    "src/view/ViewGroup.cpp"
    "src/view/Choreographer.cpp"
    "src/view/LinearLayout.cpp"
    "src/view/FrameLayout.cpp"
    "src/view/OverlayPanelLayout.cpp"
    "src/view/RelativeLayout.cpp"
    "src/view/ConstraintLayout.cpp"
    src/animation/PropertyRegistry.cpp
    src/animation/ValueAnimator.cpp
    src/animation/ObjectAnimator.cpp
    src/animation/AnimatorInflater.cpp
    src/animation/LayoutTransition.cpp

    "src/view/GridLayout.cpp"
    "src/widget/TextView.cpp"
    "src/widget/Button.cpp"
    "src/widget/EditText.cpp"
    "src/widget/ImageView.cpp"
    "src/widget/ImageButton.cpp")

target_include_directories(setu_runtime PRIVATE include)
# windowscodecs: WIC, for BitmapFactory's decoders. ole32: CoInitializeEx and
# CoCreateInstance, which WIC needs and which nothing else here was using.
target_link_libraries(setu_runtime PRIVATE miniz d2d1 dwrite d3d11 dxgi windowscodecs ole32 setu_androidfw cassowary skia)
# target_link_options
target_compile_options(setu_runtime PRIVATE "/utf-8" "/FI" "${CMAKE_SOURCE_DIR}/thirdparty/androidfw_compat.h")

# A deliberately minimal subset: only what the constraint solver test actually
# touches, so it stays fast to build and independent of the resource pipeline. Note
# what is absent - ResourceManager, Theme, TypedArray, the inflaters, Bitmap and the
# bitmap drawables. RecordingCanvas::drawBitmap is compiled in here, but it only
# stores a shared_ptr and re-dispatches through Canvas, so it pulls in no Bitmap
# symbol; nothing in this target names BitmapDrawable or NinePatchDrawable at all.
# Adding them would drag in Bitmap.cpp, BitmapFactory.cpp and the WIC link libraries
# for code the test never runs.


# --- Install & Packaging Configuration ---

# Install the main runtime executable
install(TARGETS setu_runtime DESTINATION bin)

# Install the framework resources folder
# We install it to bin/apkresources because the runtime searches relative to the executable
install(DIRECTORY ${CMAKE_SOURCE_DIR}/apkresources/ DESTINATION bin/apkresources)

# CPack configuration to generate a distributable ZIP file
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_NAME "SetuRuntime")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Setu")

# Use ZIP as the default generator for Windows
if(WIN32)
    set(CPACK_GENERATOR "ZIP")
endif()

include(CPack)


# add_executable(test_avd "C:/Users/namde/.gemini/antigravity/brain/2793ac40-3b54-4c10-bfb3-aac1f5ab48f5/scratch/test_avd.cpp"
#     "src/os/Looper.cpp"
#     "src/os/MessageQueue.cpp"
#     "src/os/Handler.cpp"
#     "src/view/Choreographer.cpp"
#     "src/animation/ValueAnimator.cpp"
#     "src/animation/ObjectAnimator.cpp"
#     "src/animation/PropertyRegistry.cpp"
#     "src/graphics/drawable/VectorDrawable.cpp"
#     "src/graphics/Path.cpp"
#     "src/graphics/Paint.cpp"
#     "src/graphics/drawable/Drawable.cpp"
# )
# target_include_directories(test_avd PRIVATE src include "${CMAKE_SOURCE_DIR}/thirdparty/skia_build/skia")
# target_link_libraries(test_avd PRIVATE setu_androidfw skia)
