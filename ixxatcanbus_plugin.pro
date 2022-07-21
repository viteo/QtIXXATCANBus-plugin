TARGET = qtixxatcanbus

QT += core serialbus

TEMPLATE = lib
CONFIG += plugin

DEFINES += IXXATCAN_PLUGIN_LIB BUILD_STATIC USE_SOCKET QTCAN_STATIC_DRIVERS=1 QT_PLUGIN

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += $$(VciSDKDir)\\inc
DEPENDPATH += $$(VciSDKDir)\\inc
LIBS += -L$$(VciSDKDir)\\lib\\x64\\Release -lvciapi

SOURCES += \
    src/IxxatCanBackend.cpp \
    src/uuids.c

HEADERS += \
    src/IxxatCanBackend.h \
    src/IxxatCanBusPlugin.h

DISTFILES += \
    plugin.json

DESTDIR = $$PWD/build/$${BUILD_NAME}
OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.u

message(Building IXXAT SOCKET version)

win32 {
    target.path = $$[QT_INSTALL_PLUGINS]/canbus
    INSTALL += target
    message(install in $$target.path)
}
