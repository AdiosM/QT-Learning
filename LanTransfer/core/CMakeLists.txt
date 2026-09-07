# lantransfer_core：平台无关核心库，Windows / Android / 单元测试三方共享
qt_add_library(lantransfer_core STATIC
    protocol/packet.h
    protocol/packet.cpp
    protocol/payloads.h
    protocol/payloads.cpp
    protocol/packetcodec.h
    protocol/packetcodec.cpp
    protocol/packetparser.h
    protocol/packetparser.cpp
    discovery/discoverymanager.h
    discovery/discoverymanager.cpp
    device/device.h
    device/devicemanager.h
    device/devicemanager.cpp
    transport/tcpserver.h
    transport/tcpserver.cpp
    transport/tcpclient.h
    transport/tcpclient.cpp
    transfer/transfertypes.h
    transfer/transfertypes.cpp
    transfer/transfersession.h
    transfer/transfersession.cpp
    transfer/transfermanager.h
    transfer/transfermanager.cpp
    transfer/filesource.h
    transfer/filesource.cpp
    transfer/filesink.h
    transfer/filesink.cpp
    security/pairingmanager.h
    security/pairingmanager.cpp
    security/hashcalculator.h
    security/hashcalculator.cpp
)

target_include_directories(lantransfer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lantransfer_core PUBLIC Qt6::Core Qt6::Network)
target_compile_definitions(lantransfer_core PUBLIC
    LANTRANSFER_APP_VERSION="${PROJECT_VERSION}")
