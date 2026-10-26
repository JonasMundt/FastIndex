
cmake_minimum_required(VERSION 3.21)

project(FastIndex VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.12 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(FastIndex
    src/main.cpp
)

qt_add_qml_module(FastIndex
    URI FastIndex
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(FastIndex PRIVATE Qt6::Quick)
