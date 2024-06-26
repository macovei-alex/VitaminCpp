QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ChatThread.cpp \
    GameStateThread.cpp \
    ImageThread.cpp \
    OnlineData.cpp \
    canvaspaint.cpp \
    choosewordwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    Line.cpp

HEADERS += \
    ChatThread.h \
    GameStateThread.h \
    ImageThread.h \
    OnlineData.h \
    canvaspaint.h \
    choosewordwindow.h \
    macro.h \
    mainwindow.h \
    ui_canvaspaint.h \
    ui_mainwindow.h \
    DrawingState.h \
    Line.h

FORMS += \
    canvaspaint.ui \
    choosewordwindow.ui \
    mainwindow.ui
	
INCLUDEPATH += \
	../../Common \
	../../ClientServices

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resource.qrc
