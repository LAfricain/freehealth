TARGET = %PluginName%
TEMPLATE = lib

DEFINES += %PluginName:u%_LIBRARY
BUILD_PATH_POSTFIXE = FreeMedForms

include(../fmf_plugins.pri)
include(%PluginName:l%_dependencies.pri)

HEADERS += \
    %PluginName:l%plugin.h \
    %PluginName:l%_exporter.h \
    %PluginName:l%constants.h \
    %PluginName:l%preferences.h
        
SOURCES += \
    %PluginName:l%plugin.cpp \
    %PluginName:l%preferences.cpp

FORMS += \
    %PluginName:l%preferences.ui

OTHER_FILES = %PluginName%.pluginspec

#FREEMEDFORMS_SOURCES=%FreeMedFormsSources%
#IDE_BUILD_TREE=%FreeMedFormsBuild%

PROVIDER = %VendorName%

# include translations
TRANSLATION_NAME = %PluginName:l%
include($${SOURCES_ROOT_PATH}/buildspecs/translations.pri)
