#!/bin/bash
# Build script to prepare GridLAB-D Python bindings for PyPI distribution
# This script should be run before `python -m build`

set -e

echo "Building GridLAB-D Python bindings for PyPI distribution..."

# Ensure we're in the right directory
cd "$(dirname "$0")"

# Check if GridLAB-D is already built
if [ ! -f "../build/lib/libgldapi.so" ]; then
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
    make -j$(nproc) gldapi
    
    cd ../python_bindings
fi

# Create prebuilt directory structure
echo "Creating prebuilt directory structure..."
rm -rf prebuilt
mkdir -p prebuilt/lib
mkdir -p prebuilt/share

# Copy GridLAB-D API library
echo "Copying GridLAB-D libraries..."
cp ../build/lib/libgldapi.so prebuilt/lib/
if [ -f "../build/lib/static/libjsoncpp.a" ]; then
    mkdir -p prebuilt/lib/static
    cp ../build/lib/static/libjsoncpp.a prebuilt/lib/static/
fi

# Copy all GridLAB-D module libraries (.so files) for runtime use
echo "Copying GridLAB-D modules..."
find ../build/lib -name "*.so" -not -name "libgldapi.so" -exec cp {} prebuilt/lib/ \;

# Copy essential data files
echo "Copying data files..."
cp ../gldcore/tzinfo.txt prebuilt/share/
cp ../gldcore/unitfile.txt prebuilt/share/

# Copy header files needed for compilation
echo "Copying header files..."
mkdir -p gldcore
cp ../gldcore/*.h gldcore/

# Copy jsoncpp headers
echo "Copying jsoncpp headers..."
mkdir -p third_party/jsoncpp_lib
cp -r ../third_party/jsoncpp_lib/include third_party/jsoncpp_lib/

echo "Prebuilt files prepared successfully!"
echo "You can now run: python -m build"