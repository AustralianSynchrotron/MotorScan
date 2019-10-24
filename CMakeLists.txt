cmake_minimum_required(VERSION 3.0)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(MotorScan
        VERSION 1.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt5 COMPONENTS Widgets REQUIRED)
include_directories(${Qt5Widgets_INCLUDE_DIRS})
find_package(Qt5 COMPONENTS PrintSupport REQUIRED)
include_directories(${Qt5PrintSupport_INCLUDE_DIRS})

find_package(QwtQt5 6.0 REQUIRED)
include_directories(${QWT_INCLUDE_DIRS})

find_path(QEPICSPV_INC qtpv.h)
if(NOT QEPICSPV_INC)
  message(FATAL_ERROR ": Can't find folder containing qtpv.h")
endif()
include_directories(${QEPICSPV_INC})

find_path(QCAMOTOR_INC qcamotorgui.h)
if(NOT QCAMOTOR_INC)
  message(FATAL_ERROR ": Can't find folder containing qcamotorgui.h")
endif()
include_directories(${QCAMOTOR_INC})


add_executable(MotorScanMX
  main.cpp
  mainwindow.ui
  mainwindow.h
  mainwindow.cpp
  axis.h
  axis.cpp
  axis.ui
  graph.h
  graph.cpp
  script.h
  script.cpp
  graph.ui
  scanmx.qrc
)

target_link_libraries(MotorScanMX
  qtpv
  qtpvwidgets
  qcamotor
  qcamotorgui
  Qt5::Widgets
  Qt5::PrintSupport
  ${QWT_LIBRARIES}
)

target_compile_options(MotorScanMX
  PRIVATE -fPIC
)


install(TARGETS MotorScanMX
    DESTINATION bin
)

install(FILES MotorScanIcon.png
    DESTINATION share/icons
)

install(FILES MotorScan.desktop
    DESTINATION share/applications
)
