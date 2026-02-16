#!/bin/bash
# cross-build-win64.sh
#
# Cross-compile msolve for Windows x86_64 from Linux using MinGW-w64.
# Produces a distributable folder with DLLs, import libraries, and headers.
#
# Prerequisites (Ubuntu/Debian):
#   sudo apt install mingw-w64 autoconf automake libtool wget
#
# Usage:
#   ./cross-build-win64.sh
#
# Output:
#   win64-build/msolve-win64/   - Distribution directory

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/win64-build"
DEPS_PREFIX="${BUILD_DIR}/deps-prefix"
INSTALL_PREFIX="${BUILD_DIR}/install"
DIST_DIR="${BUILD_DIR}/msolve-win64"
HOST=x86_64-w64-mingw32
BUILD=x86_64-linux-gnu
JOBS=$(nproc)

echo "=== Cross-compiling msolve for Windows x86_64 ==="
echo "Build dir: ${BUILD_DIR}"
echo "Using ${JOBS} parallel jobs"
echo

mkdir -p "${BUILD_DIR}"

# ---- Build GMP ----
if [ ! -f "${DEPS_PREFIX}/lib/libgmp.dll.a" ]; then
    echo "--- Building GMP ---"
    cd "${BUILD_DIR}"
    [ -f gmp-6.3.0.tar.xz ] || wget -q https://gmplib.org/download/gmp/gmp-6.3.0.tar.xz
    [ -d gmp-6.3.0 ] || tar xf gmp-6.3.0.tar.xz
    cd gmp-6.3.0
    CC_FOR_BUILD=gcc ./configure \
        --host=${HOST} --build=${BUILD} \
        --prefix="${DEPS_PREFIX}" \
        --enable-shared --disable-static
    make -j${JOBS}
    make install
    echo "GMP built successfully."
else
    echo "GMP already built, skipping."
fi

# ---- Build MPFR ----
if [ ! -f "${DEPS_PREFIX}/lib/libmpfr.dll.a" ]; then
    echo "--- Building MPFR ---"
    cd "${BUILD_DIR}"
    [ -f mpfr-4.2.1.tar.xz ] || wget -q https://www.mpfr.org/mpfr-4.2.1/mpfr-4.2.1.tar.xz
    [ -d mpfr-4.2.1 ] || tar xf mpfr-4.2.1.tar.xz
    cd mpfr-4.2.1
    ./configure \
        --host=${HOST} --build=${BUILD} \
        --prefix="${DEPS_PREFIX}" \
        --enable-shared --disable-static \
        --with-gmp="${DEPS_PREFIX}"
    make -j${JOBS}
    make install
    echo "MPFR built successfully."
else
    echo "MPFR already built, skipping."
fi

# ---- Build FLINT ----
if [ ! -f "${DEPS_PREFIX}/lib/libflint.dll.a" ]; then
    echo "--- Building FLINT ---"
    cd "${BUILD_DIR}"
    [ -f flint-3.1.3.tar.gz ] || wget -q https://github.com/flintlib/flint/releases/download/v3.1.3/flint-3.1.3.tar.gz
    [ -d flint-3.1.3 ] || tar xf flint-3.1.3.tar.gz
    cd flint-3.1.3
    ./configure \
        --host=${HOST} --build=${BUILD} \
        --prefix="${DEPS_PREFIX}" \
        --enable-shared --disable-static \
        --with-gmp="${DEPS_PREFIX}" \
        --with-mpfr="${DEPS_PREFIX}"
    make -j${JOBS}
    make install
    echo "FLINT built successfully."
else
    echo "FLINT already built, skipping."
fi

# ---- Build msolve ----
echo "--- Building msolve ---"
cd "${SCRIPT_DIR}"

# Regenerate build system if needed
if [ ! -f configure ] || [ configure.ac -nt configure ]; then
    autoreconf -fi
fi

rm -rf "${BUILD_DIR}/msolve-build"
mkdir -p "${BUILD_DIR}/msolve-build"
cd "${BUILD_DIR}/msolve-build"

"${SCRIPT_DIR}/configure" \
    --host=${HOST} --build=${BUILD} \
    --prefix="${INSTALL_PREFIX}" \
    --enable-shared \
    --disable-openmp \
    CFLAGS="-O2 -I${DEPS_PREFIX}/include -I${DEPS_PREFIX}/include/flint -include ${SCRIPT_DIR}/src/msolve/compat.h" \
    LDFLAGS="-L${DEPS_PREFIX}/lib" \
    CPPFLAGS="-I${DEPS_PREFIX}/include -I${DEPS_PREFIX}/include/flint"

make -j${JOBS}
make install

echo "msolve built successfully."

# ---- Create distribution ----
echo "--- Creating distribution ---"
rm -rf "${DIST_DIR}"
mkdir -p "${DIST_DIR}"/{bin,lib,include}

# DLLs and EXE
cp "${INSTALL_PREFIX}/bin"/*.dll "${INSTALL_PREFIX}/bin"/*.exe "${DIST_DIR}/bin/"
cp "${DEPS_PREFIX}/bin"/*.dll "${DIST_DIR}/bin/"

# MinGW runtime DLLs
for dll in libgcc_s_seh-1.dll libwinpthread-1.dll; do
    dllpath=$(${HOST}-gcc -print-file-name=${dll} 2>/dev/null)
    if [ -f "${dllpath}" ]; then
        cp "${dllpath}" "${DIST_DIR}/bin/"
    fi
done

# Import libraries
cp "${INSTALL_PREFIX}/lib"/*.dll.a "${DIST_DIR}/lib/"
cp "${DEPS_PREFIX}/lib"/*.dll.a "${DIST_DIR}/lib/"

# Headers
cp -r "${INSTALL_PREFIX}/include/msolve" "${DIST_DIR}/include/"

echo
echo "=== Build complete! ==="
echo "Distribution directory: ${DIST_DIR}"
echo
echo "Contents:"
find "${DIST_DIR}" -type f | sort | sed "s|${DIST_DIR}/|  |"
