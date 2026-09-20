cmake_minimum_required(VERSION 3.16)
project(PackReadBenchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT TARGET PackSdk)
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../PackSdk ${CMAKE_CURRENT_BINARY_DIR}/PackSdk)
endif()

add_executable(PackReadBenchmark
    PackReadBenchmark.cpp
)

target_include_directories(PackReadBenchmark PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../PackSdk
)

target_link_libraries(PackReadBenchmark PRIVATE PackSdk)

if(WIN32)
    target_link_libraries(PackReadBenchmark PRIVATE shell32)
endif()

add_custom_command(TARGET PackReadBenchmark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:PackSdk>
        $<TARGET_FILE_DIR:PackReadBenchmark>/
)
