# include general workbench
include(../config.pri)

win32 {
    DLLDESTDIR = $${BUID_BINARY_PATH}
}

include(rpath.pri)

CONFIG *= dll

DESTDIR  = $${BUILD_LIB_PATH}


# TARGET = $$qtLibraryTarget($$TARGET)
