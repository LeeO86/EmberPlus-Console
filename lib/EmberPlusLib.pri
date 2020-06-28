QT       += core network xml


INCLUDEPATH +=  lib/libember_slim/Source

HEADERS += \
    lib/libember_slim/Source/api.h \
    lib/libember_slim/Source/ber.h \
    lib/libember_slim/Source/berio.h \
    lib/libember_slim/Source/berreader.h \
    lib/libember_slim/Source/bertag.h \
    lib/libember_slim/Source/bertypes.h \
    lib/libember_slim/Source/bytebuffer.h \
    lib/libember_slim/Source/ember.h \
    lib/libember_slim/Source/emberasyncreader.h \
    lib/libember_slim/Source/emberframing.h \
    lib/libember_slim/Source/emberinternal.h \
    lib/libember_slim/Source/emberplus.h \
    lib/libember_slim/Source/glow.h \
    lib/libember_slim/Source/glowrx.h \
    lib/libember_slim/Source/glowtx.h

SOURCES += \
    lib/libember_slim/Source/ber.c \
    lib/libember_slim/Source/berio.c \
    lib/libember_slim/Source/berreader.c \
    lib/libember_slim/Source/bertag.c \
    lib/libember_slim/Source/bytebuffer.c \
    lib/libember_slim/Source/ember.c \
    lib/libember_slim/Source/emberasyncreader.c \
    lib/libember_slim/Source/emberasyncreader.h \
    lib/libember_slim/Source/emberframing.c \
    lib/libember_slim/Source/emberinternal.c \
    lib/libember_slim/Source/glow.c \
    lib/libember_slim/Source/glowrx.c \
    lib/libember_slim/Source/glowtx.c

macx {
    message("using libember_slim compiled for MacOS")

    LIBS += \
        -L$$PWD/libember_slim/MacOS \
        -lember_slim-static
}
