# -------------------------------------------------
# Project created by QtCreator 2010-03-11T14:14:14
# -------------------------------------------------
TARGET = MotorScanMX
TEMPLATE = app
CONFIG += qwt
SOURCES += main.cpp \
    mainwindow.cpp \
    axis.cpp \
    graph.cpp
HEADERS += mainwindow.h \
    axis.h \
    graph.h
FORMS += mainwindow.ui \
    axis.ui \
    graph.ui
RESOURCES = scanmx.qrc
LIBS += -lqtpv -lqtpvwidgets  -lqcamotor  -lqcamotorgui
target.files = $$[TARGET]
target.path = $$[INSTALLBASE]/bin
INSTALLS += target
