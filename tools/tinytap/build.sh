#!/bin/bash

set -e  # Exit immediately if a command fails

# Default build configuration
BUILD_TYPE="Release"
DO_CLEAN=false

# Parse command-line arguments
for arg in "$@"; do
    case $arg in
        --debug)
            BUILD_TYPE="Debug"  # Enable debug build
            ;;
        --clean)
            DO_CLEAN=true       # Trigger clean build
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Usage: $0 [--debug] [--clean]"
            exit 1
            ;;
    esac
done

# Remove previous build directory if --clean is set
echo "Removing old build directory..."
rm -rf build
if [ "$DO_CLEAN" = true ]; then
    exit 0
fi

echo "Starting TinyTAP build (${BUILD_TYPE})..."

# Generate CMake build files
cmake -B build -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

# Compile the project
cmake --build build

# Detect platform using 'uname'
UNAME=$(uname)

# If it's a Release build, strip the binary to reduce size
if [ "$BUILD_TYPE" == "Release" ]; then
    echo "Stripping binary for size optimization..."

    if [[ "$UNAME" == "Linux" || "$UNAME" == "Darwin" ]]; then
        # Linux or macOS
        STRIP_CMD="strip build/tinytap"
    elif [[ "$UNAME" == "MINGW"* || "$UNAME" == "MSYS"* ]]; then
        # Windows via MSYS2 or MinGW
        STRIP_CMD="strip build/tinytap.exe"
    else
        STRIP_CMD=""
        echo "Strip not supported on platform: $UNAME"
    fi

    # Execute the strip command if applicable
    if [ -n "$STRIP_CMD" ]; then
        $STRIP_CMD && echo "Strip completed"
    fi
fi

# Optional: display final binary size
echo "Final binary:"
ls -lh build/tinytap*

echo "Build finished: ${BUILD_TYPE}"

# *** EOF ***
