find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

qt_add_executable(LanTransfer WIN32
    main.cpp
    mainwindow.h
    mainwindow.cpp
    dialogs.h
    dialogs.cpp
)

target_link_libraries(LanTransfer PRIVATE
    lantransfer_core
    Qt6::Widgets
)
