cmake_minimum_required(VERSION 3.19)
project(Reaction LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(Reaction
    WIN32 MACOSX_BUNDLE
    main.cpp
    widget.cpp
    widget.h
    widget.ui
    ColorFlashWidget.h
    ColorFlashWidget.cpp
    resources.qrc
)


target_include_directories(Reaction PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Reaction
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS Reaction
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET Reaction
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
