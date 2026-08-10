cmake_minimum_required(VERSION 3.19)
project(Camera_for_samsumg LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets OpenGLWidgets Multimedia MultimediaWidgets)

qt_standard_project_setup()

qt_add_executable(Camera_for_samsumg
    WIN32 MACOSX_BUNDLE
    main.cpp
    widget.cpp
    widget.h
    widget.ui
)

target_link_libraries(Camera_for_samsumg
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt6::OpenGLWidgets
        Qt6::Multimedia
        Qt6::MultimediaWidgets
)

install(TARGETS Camera_for_samsumg
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Camera_for_samsumg
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
