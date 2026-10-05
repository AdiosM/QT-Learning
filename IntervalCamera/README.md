# 定时拍照（C++ / Qt 6）

这是一个用 C++17、Qt Widgets 和 Qt Multimedia 实现的 Windows 桌面程序。它支持相机实时预览、选择相机、手动拍照、按设定间隔自动拍照，以及选择照片保存文件夹。照片保存为 JPEG，文件名包含时间和序号。

## 直接运行

完整版本可双击 `bin/IntervalCamera.exe` 运行，请保留 `bin` 目录里的 DLL 和插件文件夹。学习用源码压缩包不包含 `bin`，请按下文自行构建。首次使用时，请在 Windows“设置 → 隐私和安全性 → 相机”中确认桌面应用可访问相机。

学习时建议先阅读 `学习指南.md`，再打开源码。

## 从源码构建

需要 Qt 6.8 或更新版本（包含 Widgets、Multimedia、MultimediaWidgets）、CMake 3.21 或更新版本，以及与 Qt 安装包匹配的 C++ 编译器。用 Qt Creator 打开 `CMakeLists.txt`，选择合适的 Qt 6 Kit 后构建、运行。

也可以在已配置好 Qt 和编译器环境的命令行中执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="D:\QT\6.8.3\msvc2022_64"
cmake --build build --config Release
```

上面是本机已验证的构建配置；在其他电脑上请替换 Qt 路径并选择与 Qt 包匹配的编译器。向其他 Windows 电脑分发可执行文件时，需用同一 Qt Kit 的 `windeployqt` 部署运行库。

## 使用说明

1. 选择相机和保存文件夹。
2. 设置间隔，范围为 0.5 至 3600 秒。
3. 点击“开始自动拍照”。相机就绪后立即拍第一张，之后每到间隔时间发起一次拍照；如果上次照片仍在保存或相机暂未就绪，该次会跳过。
4. 点击“停止”结束自动拍照；已发起的照片会继续保存。也可以用“手动拍一张”。

运行时修改间隔会从修改时重新计时。保存路径和间隔会记住，供下次启动使用。
