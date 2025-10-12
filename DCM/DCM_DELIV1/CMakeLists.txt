cmake_minimum_required(VERSION 3.21)
project(DCM_Qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Make CMake run Qt's code generators automatically (moc)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(DCM_Qt
    main.cpp
    mainwindow.cpp
    mainwindow.h   # list the header so AUTOMOC sees Q_OBJECT
)

target_link_libraries(DCM_Qt PRIVATE Qt6::Widgets)
