TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += /home/shivy/Downloads/live/liveMedia/include     \
    /home/shivy/Downloads/live/BasicUsageEnvironment/include    \
    /home/shivy/Downloads/live/groupsock/include                \
    /home/shivy/Downloads/liveMedia/UsageEnvironment/include


LIBS += /usr/local/lib/libliveMedia.a \
        /usr/local/lib/libBasicUsageEnvironment.a \
        /usr/local/lib/libUsageEnvironment.a \
        /usr/local/lib/libgroupsock.a

SOURCES += main.cpp \
    dbsink.cpp \
    dbconnection.cpp

HEADERS += \
    dbsink.h \
    dbconnection.h

#MongoDB-Config Stuff
INCLUDEPATH += /usr/include \
    /usr/local/include
DEPENDPATH += /usr/include  \
    /usr/local/include

LIBS += /usr/local/lib/libmongoclient.a \
    /usr/lib/libboost_thread.a  \
    /usr/lib/libboost_system.a  \
    /usr/lib/libboost_program_options.a \
    /usr/lib/libboost_filesystem.a

LIBS += -L/usr/lib/ -lpthread
LIBS += -L/usr/lib/ -lboost_filesystem -lboost_thread -lboost_program_options -lboost_system

unix:!macx: LIBS += -L$$PWD/../../../../usr/local/lib/ -lmongoclient

INCLUDEPATH += $$PWD/../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../usr/local/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/local/lib/libmongoclient.a

LIBS += -lpthread -static

unix:!macx: LIBS += -L$$PWD/../../../../usr/lib/ -lboost_thread-mt

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/lib/libboost_thread-mt.a

unix:!macx: LIBS += -L$$PWD/../../../../usr/lib/ -lboost_filesystem

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/lib/libboost_filesystem.a


unix:!macx: LIBS += -L$$PWD/../../../../usr/lib/ -lboost_program_options

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/lib/libboost_program_options.a


unix:!macx: LIBS += -L$$PWD/../../../../usr/lib/ -lboost_system

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../../../../usr/lib/libboost_system.a


