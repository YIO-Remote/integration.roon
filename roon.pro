TEMPLATE    = lib
CONFIG      += plugin
QT          += websockets core quick
DEFINES     += DEBUG

include(../remote-software/qmake-target-platform.pri)
include(../remote-software/qmake-destination-path.pri)

HEADERS         = YioRoon.h QtRoonApi.h QtRoonBrowseApi.h QtRoonTransportApi.h QtRoonStatusApi.h QtRoonDiscovery.h BrowseModel.h \
                  ../remote-software/sources/integrations/integration.h \
                  ../remote-software/sources/integrations/plugininterface.h

SOURCES         = YioRoon.cpp QtRoonApi.cpp QtRoonBrowseApi.cpp QtRoonTransportApi.cpp QtRoonStatusApi.cpp QtRoonDiscovery.cpp BrowseModel.cpp \

TARGET          = roon

# Configure destination path by "Operating System/Compiler/Processor Architecture/Build Configuration"
DESTDIR = $$PWD/../binaries/$$DESTINATION_PATH/plugins
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

DISTFILES += \
    todo.txt
