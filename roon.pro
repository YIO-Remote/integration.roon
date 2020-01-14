TEMPLATE    = lib
CONFIG      += plugin
QT          += websockets core quick
DEFINES     += DEBUG

INTG_LIB_PATH = $$(YIO_SRC)
isEmpty(INTG_LIB_PATH) {
    INTG_LIB_PATH = $$clean_path($$PWD/../integrations.library)
    message("Environment variables YIO_SRC not defined! Using '$$INTG_LIB_PATH' for integrations.library project.")
} else {
    INTG_LIB_PATH = $$(YIO_SRC)/integrations.library
    message("YIO_SRC is set: using '$$INTG_LIB_PATH' for integrations.library project.")
}

! include($$INTG_LIB_PATH/qmake-destination-path.pri) {
    error( "Cannot find the qmake-destination-path.pri file!" )
}

! include($$INTG_LIB_PATH/yio-plugin-lib.pri) {
    error( "Cannot find the yio-plugin-lib.pri file!" )
}

! include($$INTG_LIB_PATH/yio-model-mediaplayer.pri) {
    error( "Cannot find the yio-model-mediaplayer.pri file!" )
}

HEADERS  += \
    src/YioRoon.h \
    src/QtRoonApi.h \
    src/QtRoonBrowseApi.h \
    src/QtRoonTransportApi.h \
    src/QtRoonStatusApi.h \
    src/QtRoonDiscovery.h

SOURCES  += \
    src/YioRoon.cpp \
    src/QtRoonApi.cpp \
    src/QtRoonBrowseApi.cpp \
    src/QtRoonTransportApi.cpp \
    src/QtRoonStatusApi.cpp \
    src/QtRoonDiscovery.cpp

TARGET    = roon

# Configure destination path. DESTDIR is set in qmake-destination-path.pri
DESTDIR = $$DESTDIR/plugins
OBJECTS_DIR = $$PWD/build/$$DESTINATION_PATH/obj
MOC_DIR = $$PWD/build/$$DESTINATION_PATH/moc
RCC_DIR = $$PWD/build/$$DESTINATION_PATH/qrc
UI_DIR = $$PWD/build/$$DESTINATION_PATH/ui

#DISTFILES += roon.json

# install
unix {
    target.path = /usr/lib
    INSTALLS += target
}

