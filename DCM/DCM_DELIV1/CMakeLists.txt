cmake_minimum_required(VERSION 3.21)
project(DCM_Qt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql SerialPort)

add_executable(DCM_Qt
    main.cpp
    mainwindow.cpp
    mainwindow.h
    loginwindow.h
    loginwindow.cpp
    loginwindow.ui
    database.h
    database.cpp
    mainwindow.ui
    parameterform.h parameterform.cpp
    parameterform.ui

    serialmanager.cpp
    serialmanager.h
    serialtestdialog.h serialtestdialog.cpp serialtestdialog.ui
)

target_link_libraries(DCM_Qt PRIVATE Qt6::Widgets Qt6::Sql Qt6::SerialPort)
qt_finalize_executable(DCM_Qt)
