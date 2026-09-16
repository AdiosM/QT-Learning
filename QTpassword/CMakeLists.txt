cmake_minimum_required(VERSION 3.19)
project(QTpassword LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Sql)

qt_standard_project_setup()

qt_add_executable(QTpassword
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    EntryDialog.h
    EntryDialog.cpp
    PasswordEntry.h
    PasswordEntry.cpp
    PasswordModel.h
    PasswordModel.cpp
    PasswordFilterProxyModel.h
    PasswordFilterProxyModel.cpp
    DatabaseManager.h
    DatabaseManager.cpp
    BackupManager.h
    BackupManager.cpp
    LoginDialog.h
    LoginDialog.cpp
)

target_link_libraries(QTpassword
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Sql
)

install(TARGETS QTpassword
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET QTpassword
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
