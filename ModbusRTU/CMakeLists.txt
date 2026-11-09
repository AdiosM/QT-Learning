cmake_minimum_required(VERSION 3.19)
project(HelloQt LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets SerialPort  PrintSupport SerialBus)
#Modbus 功能在 Qt 的 SerialBus 模块

qt_standard_project_setup()

qt_add_executable(HelloQt
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    serialportmanager.h
    serialportmanager.cpp
    protocolparser.h
    protocolparser.cpp
    qcustomplot.h
    qcustomplot.cpp
    modbusmanager.h
    modbusmanager.cpp
)

target_link_libraries(HelloQt
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::SerialPort
        Qt::PrintSupport
        Qt::SerialBus
)

install(TARGETS HelloQt
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET HelloQt
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
