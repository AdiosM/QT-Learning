# LanTransferSend：命令行发送工具（脚本化/无头场景）
add_executable(LanTransferSend
    cli_send.cpp
)
target_link_libraries(LanTransferSend PRIVATE lantransfer_core)
