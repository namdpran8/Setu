# Setu

**Setu** is a highly experimental, custom Android runtime for Windows built from scratch in C++. 

Rather than running an entire Android OS inside a heavy hardware emulator or a subsystem (like WSA or QEMU), Setu runs Android application packages (`.apk` files) **natively as a standard Win32 process**. It achieves this by extracting binary XML layouts (`.xml`/`resources.arsc`), directly interpreting Dalvik bytecode (`.dex`), and bridging Android framework calls to native Win32 UI components in real-time.

---

## 🚀 Key Features

* **Dalvik Execution Engine:** A custom, lightweight bytecode interpreter written in C++ that dynamically parses and executes Dalvik opcodes (from `classes.dex`) line-by-line, without needing a full Java Virtual Machine.
* **Native Win32 UI Bridging:** Intercepts Android UI inflation (`setContentView`) and seamlessly maps Android XML layouts (`ConstraintLayout`, `TextView`, `Button`, etc.) directly into native Windows rendering contexts (like Direct2D).
* **Multi-DEX Support:** Automatically parses and manages cross-DEX execution for modern, large Android applications containing multiple `classes*.dex` files.
* **AXML & ARSC Parsing:** Parses compiled Android Binary XML (AXML) and the global resource table (`resources.arsc`) to resolve strings, layouts, dimensions, and control properties.
* **Framework Stubbing Engine:** A powerful interception registry (`StubRegistry`) that detects when the bytecode tries to call the standard Android Java Framework (which doesn't exist on Windows), catches it, and executes a native C++ alternative (e.g., UI callbacks, `setOnClickListener`, `startActivity`).
* **ConstraintLayout Solver:** Integrates a native C++ Cassowary solver to accurately recreate complex Android `ConstraintLayout` constraints.

---

## 🏗️ Architecture

1. **APK Parser (`apk_extractor/ApkExtractor.cpp`)**: Unzips the provided `.apk`, extracts `classes.dex`, compiled XMLs, and resource files.
2. **Interpreter (`interpreter/Interpreter.cpp`)**: The core execution engine. It sets up an `InterpreterState` (registers, stack) and steps through DEX opcodes. It handles object instantiation, virtual method invocation, and static field resolution.
3. **Stub Registry (`interpreter/StubRegistry.cpp`)**: Since Setu does not ship with a 2GB Java runtime, all calls to `android.os.*`, `androidx.*`, or `java.*` are intercepted. The registry provides C++ lambda functions that simulate these calls (e.g., converting Android Intents to Win32 window transitions).
4. **Layout Inflater (`ui/LayoutInflater.cpp`)**: Reads the AXML layout nodes requested by the APK, extracts attributes (padding, margins, constraints), and creates the corresponding native view tree using custom rendering components.
5. **Cassowary Engine (`cassowary/`)**: Resolves view bounds mathematically for constraint-based layouts.

---

## 🛠️ Build Instructions

Setu is built using **CMake** and C++17/C++20. It requires a Windows environment due to its heavy reliance on the native Win32 API.

1. **Prerequisites:** 
   - Windows 10/11
   - Visual Studio 2022 (with "Desktop development with C++" workload)
   - CMake
2. **Build:**
   - Open the project directory in Visual Studio or use standard CMake commands.
   - Wait for CMake to generate the cache.
   - Select the `x64-Debug` or `x64-Release` build configuration.
   - Build the project target.
3. **Run:**
   - You can launch Setu via CLI (e.g., `setu_runtime.exe --package=com.example.app`) or simply double-click the executable to open an APK file picker.
   - The runtime will parse the APK, draw the main window, and start interpreting the `MainActivity` bytecode.
   - **Note:** Ensure `framework-res.apk` is available in `apkresources/framework-res.apk` for standard Android resource resolution.

---

## 📝 License

This project is for educational and experimental purposes.

---

## 🙏 Acknowledgments

Setu leverages several open-source projects and concepts:
* **[Android Open Source Project (AOSP)](https://source.android.com/)**: For the foundational components including ART (Android Runtime) structures, `libbase`, `libziparchive`, and `androidfw`.
* **[fmt](https://github.com/fmtlib/fmt)**: A modern, safe, and fast formatting library for C++.
* **[miniz](https://github.com/richgel999/miniz)**: A lightweight, drop-in replacement for zlib, used for ZIP and APK extraction.
* **[ConstraintLayout](https://github.com/androidx/constraintlayout)** & **Cassowary**: Core solver engine implementations utilized for mimicking complex Android layout structures natively on Windows.