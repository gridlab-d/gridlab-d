#!/bin/bash
# Build script to prepare GridLAB-D Python bindings for PyPI distribution
# This script should be run before `python -m build`

set -e

echo "Building GridLAB-D Python bindings for PyPI distribution..."

# Detect platform-specific shared library extension.
case "$(uname -s)" in
    Darwin)
        LIB_EXT="dylib"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        LIB_EXT="dll"
        ;;
    *)
        LIB_EXT="so"
        ;;
esac

# Portable CPU count for parallel builds.
if command -v nproc >/dev/null 2>&1; then
    BUILD_JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
    BUILD_JOBS="$(sysctl -n hw.ncpu)"
else
    BUILD_JOBS=4
fi

# Ensure we're in the right directory
cd "$(dirname "$0")"

# Detect where the library is located (Unix: build/lib, Windows: build/bin/Release)
GLD_LIB_DIR="../build/lib"
if [ -f "../build/bin/Release/gldapi.${LIB_EXT}" ]; then
    GLD_LIB_DIR="../build/bin/Release"
elif [ -f "../build/bin/Debug/gldapi.${LIB_EXT}" ]; then
    GLD_LIB_DIR="../build/bin/Debug"
fi

# Check if GridLAB-D is already built
if [ ! -f "${GLD_LIB_DIR}/gldapi.${LIB_EXT}" ] && [ ! -f "${GLD_LIB_DIR}/libgldapi.${LIB_EXT}" ]; then
    echo "GridLAB-D not found. Building GridLAB-D first..."
    cd ..
    
    # Create build directory if it doesn't exist
    if [ ! -d "build" ]; then
        mkdir build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Release ..
    else
        cd build
    fi
    
    # Build GridLAB-D
    cmake --build . --config Release --parallel "${BUILD_JOBS}"
    
    # Re-detect library location after build
    cd ../python_bindings
    if [ -f "../build/bin/Release/gldapi.${LIB_EXT}" ]; then
        GLD_LIB_DIR="../build/bin/Release"
    elif [ -f "../build/bin/Debug/gldapi.${LIB_EXT}" ]; then
        GLD_LIB_DIR="../build/bin/Debug"
    else
        GLD_LIB_DIR="../build/lib"
    fi
fi

# Create prebuilt directory structure
echo "Creating prebuilt directory structure..."
rm -rf prebuilt
mkdir -p prebuilt/lib
mkdir -p prebuilt/lib/static
mkdir -p prebuilt/share

# Copy GridLAB-D API library
echo "Copying GridLAB-D libraries..."
if [ -f "${GLD_LIB_DIR}/gldapi.${LIB_EXT}" ]; then
    cp "${GLD_LIB_DIR}/gldapi.${LIB_EXT}" prebuilt/lib/
elif [ -f "${GLD_LIB_DIR}/libgldapi.${LIB_EXT}" ]; then
    cp "${GLD_LIB_DIR}/libgldapi.${LIB_EXT}" prebuilt/lib/
fi

# On Windows, also copy the import library (.lib file)
if [ -f "../build/lib/static/Release/gldapi.lib" ]; then
    cp ../build/lib/static/Release/gldapi.lib prebuilt/lib/static/
elif [ -f "../build/lib/static/Debug/gldapi.lib" ]; then
    cp ../build/lib/static/Debug/gldapi.lib prebuilt/lib/static/
elif [ -f "../build/lib/static/gldapi.lib" ]; then
    cp ../build/lib/static/gldapi.lib prebuilt/lib/static/
fi

if [ -f "../build/lib/static/libjsoncpp.a" ]; then
    cp ../build/lib/static/libjsoncpp.a prebuilt/lib/static/
fi

# Copy all GridLAB-D module libraries for runtime use
echo "Copying GridLAB-D modules..."
if [ -d "../build/lib" ]; then
    find ../build/lib -name "*.${LIB_EXT}" -not -name "libgldapi.${LIB_EXT}" -exec cp {} prebuilt/lib/ \;
fi
if [ -d "../build/bin/Release" ]; then
    find ../build/bin/Release -name "*.${LIB_EXT}" -not -name "gldapi.${LIB_EXT}" -exec cp {} prebuilt/lib/ \;
fi

# Copy essential data files
echo "Copying data files..."
cp ../gldcore/tzinfo.txt prebuilt/share/
cp ../gldcore/unitfile.txt prebuilt/share/

# Copy header files needed for compilation
echo "Copying header files..."
mkdir -p prebuilt/gldcore
cp ../gldcore/*.h prebuilt/gldcore/

# Copy generated headers (config.h, version.h, build.h)
echo "Copying generated headers..."
mkdir -p prebuilt/headers
cp ../build/headers/*.h prebuilt/headers/

# Copy the fixed gldapi.h from gldcore to headers (overwrite the build one)
cp ../gldcore/gldapi.h prebuilt/headers/

echo "Prebuilt files prepared successfully!"
echo "You can now run: python -m build"