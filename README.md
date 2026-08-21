# Setu

**Setu** is a highly experimental, custom Android runtime for Windows built from scratch in C++. 

Rather than running Android inside a heavy hardware emulator or a Linux subsystem, Setu runs Android application packages (`.apk` files) **natively on Windows** without virtualization. It achieves this by directly interpreting Dalvik bytecode (`.dex`), extracting binary XML layouts (`.xml`/`resources.arsc`), and bridging Android framework calls directly to native Win32 UI components in real-time.

---

## 🚀 Key Features

* **Dalvik Bytecode Interpreter:** A custom, lightweight virtual machine written in C++ that dynamically parses and executes Dalvik opcodes (from `classes.dex`) line-by-line.
* **Native Win32 UI Bridging:** Intercepts Android UI inflation (`setContentView`) and seamlessly maps Android XML layouts (`ConstraintLayout`, `TextView`, `Button`) into native Windows `HWND` controls.
* **Multi-DEX Support:** Automatically parses and manages cross-DEX execution for modern, large Android applications containing multiple `classes*.dex` files.
* **AXML & ARSC Parsing:** Parses compiled Android Binary XML (AXML) and the global resource table (`resources.arsc`) to resolve strings, layouts, and control properties (like text, width, height, and IDs).
* **Framework Stubbing Engine:** A powerful interception registry (`StubRegistry`) that detects when the bytecode tries to call the standard Android Java Framework (which doesn't exist on Windows), catches it, and executes a native C++ alternative (e.g., `setOnClickListener`, `startActivity`).
* **Dynamic Activity Lifecycle:** Supports spawning new `Activity` instances, tearing down Win32 layouts, and managing `onCreate()` initialization recursively.

---

## 🏗️ Architecture

1. **APK Parser (`Main.cpp`)**: Unzips the provided `.apk`, extracts `classes.dex`, compiled XMLs, and resource files.
2. **Interpreter (`Interpreter.cpp`)**: The heart of the runtime. It sets up `InterpreterState` (registers, stack) and steps through DEX opcodes. It handles object instantiation, virtual method invocation, and static field resolution.
3. **Stub Registry (`StubRegistry.cpp`)**: Since Setu does not ship with a 2GB Java runtime, all calls to `android.os.*`, `androidx.*`, or `java.*` are intercepted. The registry provides C++ lambda functions that simulate these calls (e.g., converting Android Intents to Win32 window transitions).
4. **Layout Inflater (`LayoutInflater.cpp`)**: Reads the AXML layout nodes requested by the APK and uses the Win32 API (`CreateWindowEx`) to physically render native Windows UI elements matching the Android layout design.

---

## 🛠️ Build Instructions

Setu is built using **CMake** and C++17. It requires a Windows environment due to its heavy reliance on the native Win32 API for rendering.

1. **Prerequisites:** 
   - Windows 10/11
   - Visual Studio 2022 (with "Desktop development with C++" workload)
   - CMake
2. **Build:**
   - Open the project directory in Visual Studio.
   - Wait for CMake to generate the cache.
   - Select the `x64-Debug` or `x64-Release` build configuration.
   - Build `setu_runtime.exe`.
3. **Run:**
   - By default, the executable expects a `testapk/` directory containing a target `test.apk`.
   - The runtime will parse the APK, draw the main window, and start interpreting the `MainActivity` bytecode. Logs are automatically dumped into the `logs/` folder.

---

## 📝 License

This project is for educational and experimental purposes.

---

## 🙏 Acknowledgments

Setu leverages several open-source projects located in the `thirdparty/` folder. We would like to acknowledge and thank the contributors and maintainers of the following:

* **[Android Open Source Project (AOSP)](https://source.android.com/)**: For the foundational components including ART (Android Runtime) structures, `libbase`, `libziparchive`, and `androidfw` which are used heavily for DEX execution and APK resource parsing.
* **[fmt](https://github.com/fmtlib/fmt)**: A modern, safe, and fast formatting library for C++.
* **[miniz](https://github.com/richgel999/miniz)**: A lightweight, drop-in replacement for zlib, used for ZIP and APK extraction.
* **[zlib](https://zlib.net/)**: The widely-used general-purpose data compression library.
* **[ConstraintLayout](https://github.com/androidx/constraintlayout)**: Core solver engine implementations utilized for mimicking complex Android layout structures natively on Windows.