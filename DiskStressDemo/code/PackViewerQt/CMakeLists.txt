cmake_minimum_required(VERSION 3.16)
project(PackViewerQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(Qt6_FOUND)
    set(QT_PACKAGE Qt6)
else()
    find_package(Qt5 COMPONENTS Widgets REQUIRED)
    set(QT_PACKAGE Qt5)
endif()

if(NOT TARGET PackSdk)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../PackSdk ${CMAKE_CURRENT_BINARY_DIR}/PackSdk)
endif()

add_executable(PackViewerQt
    main.cpp
    MainWindow.cpp
    MainWindow.h
    QtMsvcCompat.h
)

target_include_directories(PackViewerQt PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../PackSdk
    ${CMAKE_CURRENT_LIST_DIR}/../PackSdk/qt
)

target_link_libraries(PackViewerQt PRIVATE
    ${QT_PACKAGE}::Widgets
    PackSdk
)

add_custom_command(TARGET PackViewerQt POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:PackSdk>
        $<TARGET_FILE_DIR:PackViewerQt>/
)
