# -------------------------------------------------
# Project created by QtCreator 2010-03-11T14:14:14
# -------------------------------------------------
TARGET = MotorScanMX
TEMPLATE = app
SOURCES += main.cpp \
    mainwindow.cpp \
    axis.cpp \
    graph.cpp
INCLUDEPATH += /usr/include/qwt-qt4 \
    /usr/local/include/qwt5 \
    /usr/include/qwt5/
HEADERS += mainwindow.h \
    axis.h \
    graph.h
FORMS += mainwindow.ui \
    axis.ui \
    graph.ui
RESOURCES = scanmx.qrc
LIBS += -lqcamotorgui \
    -lqwt
target.files = $$[TARGET]
target.path = $$[INSTALLBASE]/bin
INSTALLS += target
