cmake_minimum_required(VERSION 3.16)
project(PackSdk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(PackSdk SHARED
    PackSdk.cpp
    PackSdk.h
)

target_compile_definitions(PackSdk PRIVATE PACKSDK_EXPORTS)

if(WIN32)
    target_link_libraries(PackSdk PRIVATE shell32)
endif()

target_include_directories(PackSdk PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)
