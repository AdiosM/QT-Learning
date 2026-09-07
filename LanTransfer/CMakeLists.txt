cmake_minimum_required(VERSION 3.21)

project(LanTransfer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 元对象/资源/UI 自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 源文件统一 UTF-8（含中文界面文案）
if(MSVC)
    add_compile_options(/utf-8)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Network)

# Windows 桌面端（Android 构建时不参与）
option(BUILD_WINDOWS_APP "Build the Windows desktop app" ON)
# 命令行工具（桌面平台）
option(BUILD_TOOLS "Build command-line tools" ON)

add_subdirectory(core)

if(BUILD_WINDOWS_APP AND WIN32 AND NOT ANDROID)
    add_subdirectory(windows)
endif()

if(BUILD_TOOLS AND NOT ANDROID)
    add_subdirectory(tools)
endif()
