# LanTransfer 局域网文件互传（电脑 ↔ 电脑）

基于 Qt 6 / C++ 的局域网文件互传工具。两台 Windows 电脑安装本软件后，可在同一局域网内
自动发现对方、双向互传文件，支持手动 IP 连接降级、人工确认配对（6 位 PIN）、
流式 SHA-256 完整性校验。

**单文件硬限制：500 MB = 500,000,000 字节**（十进制，UI 与帮助中统一标注）。
超过上限的文件在发送端直接拒绝，接收端即使收到异常报文也会二次拦截。

## 功能特性

- Windows ↔ Windows 双向文件传输（发送方为 TCP Client，接收方为 TCP Server，每任务一条连接）
- UDP 广播自动发现设备（2s 周期，8s 无更新判离线）；失败时可手动输入 IP 连接
- 接收方人工确认 + 6 位随机 PIN 配对（PIN 仅用于配对确认，**不加密数据**）
- 流式传输：1 MiB 分块、发送端背压流控（8 MiB 高水位/2 MiB 低水位），内存占用与文件大小无关
- 边传边算 SHA-256，接收端校验一致才落盘（临时 `.part` 文件 → 原子重命名）
- 进度/速度/剩余时间实时显示；支持取消；断连/磁盘满/同名冲突均有明确提示
- 文件名安全：仅接受 basename，拒绝 `..`、Windows 保留名、非法字符
- 中文/空格/括号文件名正常；0 字节文件正常；空文件也可传输
- 支持文件拖拽；待发送列表超限文件标红不可发送
- Windows 防火墙首次运行提示，可一键尝试添加放行规则（需管理员权限）

## 环境要求

| 组件 | 版本 | 路径 |
|---|---|---|
| Qt | 6.8.3（MinGW 64 位套件） | `D:\QT\6.8.3\mingw_64` |
| CMake | 3.30+ | `D:\QT\Tools\CMake_64` |
| MinGW | 13.1（Qt 自带） | `D:\QT\Tools\mingw1310_64` |
| Ninja | 1.12+ | `D:\QT\Tools\Ninja` |

## 构建（命令行）

```bat
set PATH=D:\QT\Tools\CMake_64\bin;D:\QT\Tools\Ninja;D:\QT\Tools\mingw1310_64\bin;%PATH%
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=D:/QT/6.8.3/mingw_64 -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

产物：
- `build\windows\LanTransfer.exe` —— 图形界面主程序
- `build\tools\LanTransferSend.exe` —— 命令行发送工具（脚本化场景）

## 构建（Qt Creator）

1. 打开 Qt Creator → 文件 → 打开文件或项目 → 选择 `e:\Python_Files\QT\LanTransfer\CMakeLists.txt`
   （只选这一个文件即可，工程的所有源文件会自动加载）
2. Kit 选择 **Qt 6.8.3 MinGW 64-bit**（本机已自动检测，无需配置 JDK/Android）
3. 点击左下角锤子构建（Ctrl+B），绿色三角运行（Ctrl+R）

### 命令行参数

```
LanTransfer.exe [--tcp-port 50000] [--udp-port 50001] [--receive-dir 目录] [--device-id 标识]
```

- 端口被占用时程序会自动改用临时端口（弹窗提示）
- `--device-id`：同一台电脑跑两个实例时用来区分身份（见下方「同一台电脑自测」）

## 使用说明

1. 两台电脑安装并启动 LanTransfer，首次运行按提示放行防火墙（TCP 50000 / UDP 50001）
2. 左侧「局域网设备」列表自动出现对方设备（约 2 秒内）
   - 若未出现：确认同一局域网/防火墙放行；或点「手动连接…」输入对方 IP
3. 发送：选中设备 → 拖拽文件进窗口或点「选择文件…」→ 点「发送到选中设备」
4. 接收：对端弹出「收到文件传输请求」窗口，显示 6 位配对确认码；
   把该确认码告知发送方（当面/口头），发送方在输入框填入后开始传输
5. 传输列表实时显示进度/速度/剩余时间，可随时取消
6. 完成后自动 SHA-256 校验，校验失败的文件不会保存
7. 接收目录默认为系统「下载」文件夹，可在「接收目录…」中更改

### 同一台电脑自测（无两台电脑时）

两次启动同一个 exe，用不同参数区分实例：

```
实例A：LanTransfer.exe --device-id PC-A
实例B：LanTransfer.exe --device-id PC-B --tcp-port 50100 --receive-dir D:\测试接收
```

- `--device-id` 必须不同，否则会被当成"自己"过滤掉
- 实例 B 的 TCP 端口与 A 错开（A 占用 50000 后 B 也会自动回退临时端口）
- 两个窗口的「局域网设备」列表会在约 2 秒内互相出现对方
- 若未出现：点「手动连接…」输入 `127.0.0.1`、端口选「自动探测」即可
- 传输测试：A 窗口拖文件发送给 B（或反之），两端各确认一次 PIN

### 命令行发送（可选）

```bat
LanTransferSend <ip> <port> <文件> [--pin 123456] [--trust]
```

- `--pin`：直接给出对端屏幕显示的 6 位确认码
- `--trust`：信任模式自动确认（仅建议脚本/受控环境使用）

## 目录结构

```
LanTransfer/
├─ CMakeLists.txt           # 顶层构建
├─ core/                    # 平台无关核心库（lantransfer_core）
│  ├─ protocol/             #   帧编解码(packet/packetcodec/packetparser) + JSON 载荷
│  ├─ discovery/            #   UDP 设备发现
│  ├─ device/               #   设备表（DeviceModel 供 Widgets/QML 复用）
│  ├─ transport/            #   TCP Server/Client
│  ├─ transfer/             #   会话状态机/任务管理/流式文件读写/文件名安全
│  └─ security/             #   PIN 配对 + 流式 SHA-256
├─ windows/                 # Windows 桌面端（Qt Widgets）
└─ tools/                   # 命令行发送工具 LanTransferSend
```

## 协议摘要（应用层自定义协议 V1.0）

- 帧：42 字节固定头（Magic "LNFT" / Version / Type / Flags / HeaderLength /
  PayloadLength / MessageId / TransferId，Big-Endian）+ Payload
- 控制消息 Payload 为 UTF-8 JSON；FILE_DATA 为原始字节，默认 1 MiB 分块
- 时序：TCP 连接 → 双向 HELLO → PAIR_REQUEST → PAIR_RESPONSE(人工确认+PIN)
  → FILE_INFO → FILE_ACCEPT → FILE_DATA×N → FILE_END(携带发送端流式 SHA-256)
  → FILE_VERIFY → SESSION_CLOSE
- 端口：TCP 50000（可配置/自动回退），UDP 50001；发现报文携带当前 TCP 端口

## 已知限制与后续路线

- V1.0 不支持断点续传（断开即任务失败，V1.1 增加 Resume）
- 数据未加密：PIN 只解决配对确认；TLS + 设备指纹校验列入 V1.1
- 同一对设备同时仅 1 条活动传输任务；多文件仅支持排队逐个发送
- Android 端未包含在本次交付（产品定位为电脑 ↔ 电脑）；core 库保持平台无关，
  未来接入 Android（QML + SAF）时无需改动协议与传输逻辑
