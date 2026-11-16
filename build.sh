#!/usr/bin/env sh
# Configure and build c-chat on macOS / Linux.
#
# Usage:
#   ./build.sh                 # Debug build into ./build
#   ./build.sh release         # Release build
#   ./build.sh clean           # remove ./build and rebuild from scratch
#   ./build.sh release clean   # any combination
#
# Dependencies (libsodium) are fetched by CMake automatically. If cmake
# or git are missing, the script installs them with the system package
# manager (brew / apt / dnf / yum / pacman / zypper / apk).

set -eu

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT/build"
BUILD_TYPE="Debug"
CLEAN=0

for arg in "$@"; do
    case "$arg" in
        release|Release) BUILD_TYPE="Release" ;;
        debug|Debug)     BUILD_TYPE="Debug" ;;
        clean)           CLEAN=1 ;;
        -h|--help)
            sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            echo "unknown argument: $arg" >&2
            exit 1
            ;;
    esac
done

# Install a package with whatever package manager this system has.
install_pkg() {
    pkg="$1"
    echo "==> '$pkg' not found, trying to install it"
    SUDO=""
    if [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
        SUDO="sudo"
    fi
    if command -v brew >/dev/null 2>&1; then
        brew install "$pkg"
    elif command -v apt-get >/dev/null 2>&1; then
        $SUDO apt-get update && $SUDO apt-get install -y "$pkg"
    elif command -v dnf >/dev/null 2>&1; then
        $SUDO dnf install -y "$pkg"
    elif command -v yum >/dev/null 2>&1; then
        $SUDO yum install -y "$pkg"
    elif command -v pacman >/dev/null 2>&1; then
        $SUDO pacman -Sy --noconfirm "$pkg"
    elif command -v zypper >/dev/null 2>&1; then
        $SUDO zypper install -y "$pkg"
    elif command -v apk >/dev/null 2>&1; then
        $SUDO apk add "$pkg"
    elif [ "$(uname -s)" = "Darwin" ]; then
        echo "error: Homebrew is required to install '$pkg' on macOS." >&2
        echo "Install it from https://brew.sh and run this script again." >&2
        exit 1
    else
        echo "error: no supported package manager found, install '$pkg' manually" >&2
        exit 1
    fi
}

for tool in cmake git; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        install_pkg "$tool"
        if ! command -v "$tool" >/dev/null 2>&1; then
            echo "error: '$tool' is still not available after installation" >&2
            exit 1
        fi
    fi
done

# A C compiler is needed too; on macOS this means the Xcode CLI tools.
if ! command -v cc >/dev/null 2>&1 && ! command -v gcc >/dev/null 2>&1 && ! command -v clang >/dev/null 2>&1; then
    if [ "$(uname -s)" = "Darwin" ]; then
        echo "==> No C compiler found, installing Xcode Command Line Tools"
        xcode-select --install || true
        echo "Finish the Xcode CLI tools installation and run this script again." >&2
        exit 1
    else
        install_pkg gcc
    fi
fi

if [ "$CLEAN" -eq 1 ] && [ -d "$BUILD_DIR" ]; then
    echo "==> Removing $BUILD_DIR"
    rm -rf "$BUILD_DIR"
fi

if command -v nproc >/dev/null 2>&1; then
    JOBS="$(nproc)"
elif command -v sysctl >/dev/null 2>&1; then
    JOBS="$(sysctl -n hw.ncpu)"
else
    JOBS=4
fi

echo "==> Configuring ($BUILD_TYPE)"
cmake -S "$ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "==> Building with $JOBS jobs"
cmake --build "$BUILD_DIR" --config "$BUILD_TYPE" -j "$JOBS"

echo
echo "Done. Binaries:"
echo "  $BUILD_DIR/client/client"
echo "  $BUILD_DIR/server/server"
