# TinyTAP

TinyTAP is a lightweight, cross-platform command-line tool designed for TAP-based device discovery and interaction. Written in C for portability and performance, it runs on Linux, macOS, and Windows (via MSYS2-MINGW64).

---

## Features

- UDP broadcast-based network discovery
- Platform-aware socket handling (Linux, macOS, Windows)
- Packed protocol structures for TAP messaging
- Signal handling and timeout management
- Clean and customizable build system with CMake

---

## Build Instructions

### Requirements

- CMake = 3.15
- C compiler (GCC, Clang, or MSVC)
- Optional: MSYS2-MINGW64 for Windows builds

### Build via CMake (Cross-Platform)

```bash
./build.sh           # Release build
./build.sh --debug   # Debug build
./build.sh --clean   # Clean build folder only
