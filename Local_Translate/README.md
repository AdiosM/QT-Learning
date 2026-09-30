# Local Translate

使用 Qt Widgets 和本机 llama.cpp 服务调用 Hy-MT2 GGUF 模型的翻译软件。

## 文件结构

- `main.cpp`：启动应用。
- `mainwindow.h`：窗口类及控件、模型状态成员声明。
- `mainwindow.cpp`：界面布局、翻译请求和窗口初始化/清理。
- `mainwindow_model.cpp`：模型启动、就绪检测和错误处理。
- `CMakeLists.txt`：构建配置，只有一个程序目标 `Local_Translate`。

两个 `.cpp` 文件实现的是同一个 `MainWindow` 类，只拆分文件，没有新增类或功能。

## 构建与使用

用 Qt Creator 打开 `CMakeLists.txt`，选择 Qt 6.5 或更新版本的 Kit，重新运行 CMake 并构建。
需要 Qt Core、Widgets、Network 模块。

路径以 **Local_Translate.exe 所在目录**为基准，与启动程序时的工作目录无关。

llama-server 按以下顺序查找：`LOCAL_TRANSLATE_SERVER` 环境变量、程序目录中的
`runtime/llama-server.exe`、程序同目录的 `llama-server.exe`、PATH、当前用户的 WinGet 安装目录。
使用 runtime 文件夹时，应放入完整的 llama.cpp 运行时（包括配套 DLL）。

模型按以下顺序查找：`LOCAL_TRANSLATE_MODEL` 环境变量、程序目录中的
`models/Hy-MT2-1.8B-Q4_K_M.gguf`、程序同目录的同名模型，最后兼容当前开发机的
`D:/AI/Models/Hy-MT2/Hy-MT2-1.8B-Q4_K_M.gguf`。因此当前电脑无需移动模型。
换电脑时使用 models 文件夹或环境变量即可，无需修改源码。

环境变量可指定其他文件名或位置；相对路径同样相对于程序目录。
显式指定的路径无效时会报错，不会使用其他候选文件。示例（PowerShell）：

```powershell
$env:LOCAL_TRANSLATE_SERVER = 'E:/llama.cpp/llama-server.exe'
$env:LOCAL_TRANSLATE_MODEL = 'E:/models/Hy-MT2-1.8B-Q4_K_M.gguf'
& 'E:/LocalTranslate/Local_Translate.exe'
```

当前保留原来的 `Vulkan0`、`-ngl all` 和 `127.0.0.1:8080` 配置。
如果修改端口，也需同步修改该文件中的健康检查地址以及 `mainwindow.cpp` 中的翻译地址。

运行应用后自动启动模型，加载完成后点击“翻译”。关闭窗口时停止由本程序启动的模型进程。
llama.cpp 运行库和模型文件需自行准备，不包含在源码中。

`build/` 和 `.qtcreator/` 是本机生成文件，不需要上传 GitHub。项目不包含测试代码或测试目标。
