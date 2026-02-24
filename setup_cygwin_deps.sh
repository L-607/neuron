#!/bin/bash
set -e

echo "=== Neuron Cygwin Dependency Setup Helper ==="

# Function to check if a command exists
check_cmd() {
    if ! command -v "$1" &> /dev/null; then
        echo "Error: '$1' not found. Please install it via Cygwin Setup."
        return 1
    fi
}

# 1. Check for build tools
echo "[1/4] Checking build tools..."
check_cmd cmake
check_cmd make
check_cmd gcc
check_cmd g++
check_cmd pkg-config

# 2. Check for libraries available in Cygwin
echo "[2/4] Checking system libraries..."
LIBS_MISSING=0

check_lib() {
    # Simple check via pkg-config or file existence
    if pkg-config --exists "$1" 2>/dev/null; then
        echo "  [OK] $1 found."
    else
        echo "  [MISSING] $1 not found."
        LIBS_MISSING=1
    fi
}

# Note: Package names in Cygwin Setup might differ slightly, but pkg-config names are standard
check_lib openssl      # Package: libssl-devel
check_lib check        # Package: libcheck-devel (for testing, optional)
check_lib jansson      # Package: libjansson-devel
check_lib libxml-2.0   # Package: libxml2-devel
check_lib sqlite3      # Package: libsqlite3-devel

if [ $LIBS_MISSING -eq 1 ]; then
    echo "--------------------------------------------------------"
    echo "WARNING: Some system libraries are missing."
    echo "Please run Cygwin's setup-x86_64.exe and install:"
    echo "  - libssl-devel"
    echo "  - libjansson-devel"
    echo "  - libxml2-devel"
    echo "  - libsqlite3-devel"
    echo "  - libprotobuf-devel (if not compiling protobuf manually)"
    echo "--------------------------------------------------------"
    read -p "Press Enter to continue anyway (build might fail)..."
fi

# 3. Create deps directory
DEPS_DIR="$(pwd)/../neuron-deps"
mkdir -p "$DEPS_DIR"
echo "[3/4] Building custom dependencies in $DEPS_DIR..."

cd "$DEPS_DIR"

# --- ZLOG ---
# zlog 1.2.18+ requires memfd_create (Linux-only syscall); use tag 1.2.12
# (latest-stable) which compiles cleanly on Cygwin.
# GCC 13 turns several old-style warnings into errors, so we override WARNINGS
# to drop -Werror.
if [ ! -d "zlog" ]; then
    echo "  -> Cloning zlog..."
    git clone https://github.com/HardySimpson/zlog.git
fi
if [ ! -f "/usr/local/lib/libzlog.a" ]; then
    echo "  -> Building zlog 1.2.12..."
    cd zlog
    git checkout 1.2.12
    make PREFIX=/usr/local WARNINGS="-Wall -Wstrict-prototypes -fwrapv"
    make PREFIX=/usr/local WARNINGS="-Wall -Wstrict-prototypes -fwrapv" install
    cd ..
else
    echo "  -> zlog found in /usr/local, skipping."
fi

# Check for Cygwin CMake explicitly
if [ -f "/usr/bin/cmake" ]; then
    CMAKE_CMD="/usr/bin/cmake"
else
    if command -v cmake &> /dev/null; then
        echo "Warning: /usr/bin/cmake not found. Using 'cmake' from PATH."
        # Use cygpath to check if it is a windows path, if so, we are in trouble.
        CMAKE_PATH=$(command -v cmake)
        if [[ "$CMAKE_PATH" == *":"* ]]; then
             echo "ERROR: The 'cmake' in PATH appears to be a Windows native binary ($CMAKE_PATH)."
             echo "       You MUST install the 'cmake' package via Cygwin Setup to proceed."
             exit 1
        fi
        CMAKE_CMD="cmake"
    else
        echo "Error: cmake not found. Please install 'cmake' via Cygwin Setup."
        exit 1
    fi
fi

# ...

# --- MbedTLS ---
if [ ! -d "mbedtls" ]; then
    echo "  -> Cloning mbedtls..."
    git clone -b v2.16.12 https://github.com/Mbed-TLS/mbedtls.git
fi
if [ -f "/usr/local/include/mbedtls/version.h" ] && [ -f "/usr/local/lib/libmbedtls.a" ]; then
    echo "  -> mbedtls found in /usr/local, skipping build."
else
    echo "  -> Building mbedtls..."
    cd mbedtls
    rm -rf build
    mkdir -p build && cd build
    $CMAKE_CMD -G "Unix Makefiles" -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DUSE_SHARED_MBEDTLS_LIBRARY=OFF -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_INSTALL_PREFIX=/usr/local ..
    make -j4
    make install
    # Try to fix permissions explicitly
    chmod -R a+r /usr/local/include/mbedtls 2>/dev/null || true
    cd ../..
fi

# Double check
if [ ! -r "/usr/local/include/mbedtls/version.h" ]; then
    echo "WARNING: Check for mbedtls header failed using -r test."
    echo "Permissions might be broken. Debug info:"
    ls -la /usr/local/include/mbedtls 2>/dev/null || echo "Cannot list /usr/local/include/mbedtls"
    
    if ls "/usr/local/include/mbedtls/version.h" >/dev/null 2>&1; then
         echo "  [OK] Header found via ls."
    else
         echo "ERROR: mbedtls header apparently missing, BUT cmake reported success."
         echo "Attempting to continue anyway..."
    fi
fi

# --- NanoSDK (NNG) ---
if [ ! -d "NanoSDK" ]; then
    echo "  -> Cloning NanoSDK..."
    git clone -b neuron https://github.com/neugates/NanoSDK.git
fi
if [ ! -f "/usr/local/lib/libnng.a" ]; then
    echo "  -> Building NanoSDK..."
    cd NanoSDK
    rm -rf build
    mkdir -p build && cd build
    # Disable TLS/SQLite in NNG if you want to use system ones, ensuring we link correctly
    # Point explicitly to /usr/local for finding mbedtls
    # NOTE: Disabling TLS temporarily to bypass path issues with mbedtls headers in Cygwin.
    # If you need TLS, change -DNNG_ENABLE_TLS=ON and ensure mbedtls paths are perfect.
    $CMAKE_CMD -G "Unix Makefiles" \
        -DBUILD_SHARED_LIBS=OFF \
        -DNNG_TESTS=OFF \
        -DNNG_ENABLE_SQLITE=ON \
        -DNNG_ENABLE_TLS=OFF \
        -DCMAKE_SYSTEM_NAME=Linux \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        -DCMAKE_PREFIX_PATH=/usr/local \
        .. || (echo "NanoSDK Configure Failed" && exit 1)
    make -j4
    make install
    cd ../..
else
    echo "  -> NanoSDK (libnng) likely installed, skipping."
fi

# --- libjwt ---
if [ ! -d "libjwt" ]; then
    echo "  -> Cloning libjwt..."
    git clone -b v1.13.1 https://github.com/benmcollins/libjwt.git
fi
if [ ! -f "/usr/local/lib/libjwt.a" ]; then
    echo "  -> Building libjwt..."
    # Create a fake jansson cmake config so libjwt's find_package can find it
    JANSSON_CMAKE_DIR="/usr/lib/cmake/jansson"
    mkdir -p "$JANSSON_CMAKE_DIR"
    JANSSON_LIB=$(pkg-config --libs-only-l jansson | sed 's/-l//')
    JANSSON_LIBDIR=$(pkg-config --variable=libdir jansson)
    JANSSON_INCDIR=$(pkg-config --variable=includedir jansson)
    cat > "$JANSSON_CMAKE_DIR/janssonConfig.cmake" <<EOF
set(jansson_FOUND TRUE)
set(JANSSON_FOUND TRUE)
set(JANSSON_INCLUDE_DIR "$JANSSON_INCDIR")
set(JANSSON_LIBRARIES "$JANSSON_LIBDIR/lib${JANSSON_LIB}.dll.a")
if(NOT TARGET jansson::jansson)
  add_library(jansson::jansson UNKNOWN IMPORTED)
  set_target_properties(jansson::jansson PROPERTIES
    IMPORTED_LOCATION "$JANSSON_LIBDIR/lib${JANSSON_LIB}.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "$JANSSON_INCDIR"
  )
endif()
if(NOT TARGET jansson)
  add_library(jansson UNKNOWN IMPORTED)
  set_target_properties(jansson PROPERTIES
    IMPORTED_LOCATION "$JANSSON_LIBDIR/lib${JANSSON_LIB}.dll.a"
    INTERFACE_INCLUDE_DIRECTORIES "$JANSSON_INCDIR"
  )
endif()
EOF
    cat > "$JANSSON_CMAKE_DIR/janssonConfigVersion.cmake" <<EOF
set(PACKAGE_VERSION "2.14")
set(PACKAGE_VERSION_EXACT TRUE)
set(PACKAGE_VERSION_COMPATIBLE TRUE)
EOF
    cd libjwt
    rm -rf build
    mkdir -p build && cd build
    $CMAKE_CMD -G "Unix Makefiles" \
        -DENABLE_PIC=ON \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_PREFIX_PATH="/usr/lib/cmake/jansson;/usr" \
        -Djansson_DIR="$JANSSON_CMAKE_DIR" \
        ..
    make -j4
    make install
    cd ../..
else
    echo "  -> libjwt likely installed, skipping."
fi

# --- Protobuf-C ---
# Installing protobuf-c is tricky if protobuf is not strictly matched.
# We'll check if it exists.
if ! pkg-config --exists "libprotobuf-c"; then
    if [ -d "protobuf-c/.git" ]; then
        echo "  -> protobuf-c repo exists, updating..."
        git -C protobuf-c fetch --all --tags || true
        git -C protobuf-c checkout main || git -C protobuf-c checkout master || true
        git -C protobuf-c pull --ff-only || true
    elif [ -d "protobuf-c" ]; then
        echo "  -> protobuf-c directory already exists (non-git), reusing."
    else
        echo "  -> Cloning protobuf-c (main branch, has CMake support)..."
        git clone https://github.com/protobuf-c/protobuf-c.git
    fi
    echo "  -> Building protobuf-c (using CMake)..."
    cd protobuf-c
    rm -rf build_out
    mkdir -p build_out && cd build_out
    # protobuf-c's CMake entry may be in build-cmake/ or repo root.
    PROTOBUF_C_CMAKE_SRC="../build-cmake"
    if [ ! -f "${PROTOBUF_C_CMAKE_SRC}/CMakeLists.txt" ]; then
        PROTOBUF_C_CMAKE_SRC=".."
    fi
    if ! pkg-config --exists protobuf 2>/dev/null; then
        echo "WARNING: libprotobuf (C++) not found via pkg-config."
        echo "         Trying anyway; install libprotobuf-devel via Cygwin setup if this fails."
    fi
    $CMAKE_CMD -G "Unix Makefiles" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_PROTOC=OFF \
        -DCMAKE_INSTALL_PREFIX=/usr/local \
        "${PROTOBUF_C_CMAKE_SRC}" || { echo "protobuf-c CMake configure failed. Please install 'libprotobuf-devel' via Cygwin setup."; cd ../..; exit 1; }
    make -j4
    make install
    cd ../..
else
    echo "  -> libprotobuf-c found in system, skipping."
fi

echo "[4/4] Done! You can now compile Neuron."
