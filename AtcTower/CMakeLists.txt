cmake_minimum_required(VERSION 3.19)
project(AtcTower LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Quick
    QuickWidgets
    Qml
    Positioning
    Location
)

qt_standard_project_setup()

qt_add_executable(AtcTower
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    Types.h
    flightmodel.h flightmodel.cpp
)

add_library(radarproto STATIC
    radarclient.h
    radarclient.cpp
)

target_link_libraries(radarproto
    PRIVATE
        Qt::Core
        Qt::Network
)

qt_add_resources(AtcTower "myresources"
    PREFIX "/"
    FILES
        map.qml
        plane.png
)

target_link_libraries(AtcTower
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Quick
        Qt::QuickWidgets
        Qt::Qml
        Qt::Positioning
        Qt::Location
        radarproto
)

include(GNUInstallDirs)

install(TARGETS AtcTower
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AtcTower
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
