#!/usr/bin/env bash
# PdfTranslator 命令行构建脚本（在 Git Bash 中运行）
# 用法: ./build.sh          -> Debug 构建
#       ./build.sh test     -> Debug 构建 + 运行单元测试
#       ./build.sh release  -> Release 构建
set -e

# 工具链 PATH（每次调用独立生效）
export PATH="/d/QT/Tools/CMake_64/bin:/d/QT/Tools/Ninja:/d/QT/Tools/mingw1310_64/bin:/d/QT/6.8.3/mingw_64/bin:$PATH"

BUILD_TYPE="Debug"
BUILD_DIR="build"
if [ "$1" = "release" ]; then
    BUILD_TYPE="Release"
    BUILD_DIR="build-release"
fi

cmake -S . -B "$BUILD_DIR" -G Ninja \
    -DCMAKE_PREFIX_PATH="D:/QT/6.8.3/mingw_64" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build "$BUILD_DIR"

if [ "$1" = "test" ]; then
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi
