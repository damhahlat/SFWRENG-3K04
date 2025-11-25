cmake_minimum_required(VERSION 3.5)

project(DCM_DELIV1 VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -------------------------------------------------------
# Find Qt6
# -------------------------------------------------------
find_package(Qt6 COMPONENTS Core Widgets SerialPort Sql REQUIRED)

# AUTOMOC / AUTOUIC / AUTORCC must be ON or UI files will NOT build
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# -------------------------------------------------------
# Sources
# -------------------------------------------------------
set(SRC_FILES
    main.cpp
    database.cpp
    loginwindow.cpp
    mainwindow.cpp
    parameterform.cpp
    serialtestdialog.cpp
    serialmanager.cpp
)

set(HDR_FILES
    database.h
    loginwindow.h
    mainwindow.h
    parameterform.h
    serialtestdialog.h
    serialmanager.h
)

set(UI_FILES
    loginwindow.ui
    mainwindow.ui
    parameterform.ui
    serialtestdialog.ui
)

# -------------------------------------------------------
# Build executable
# -------------------------------------------------------
add_executable(DCM_DELIV1
    ${SRC_FILES}
    ${HDR_FILES}
    ${UI_FILES}
)

# -------------------------------------------------------
# Link Qt modules
# -------------------------------------------------------
target_link_libraries(DCM_DELIV1
    Qt6::Core
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Sql
)

# -------------------------------------------------------
# Install
# -------------------------------------------------------
install(TARGETS DCM_DELIV1
        RUNTIME DESTINATION bin)
