INCLUDEPATH += wallet

HEADERS += wallet/wallet.h
SOURCES += wallet/wallet.cpp

mac {
    DEFINES += USE_OSX_KEYCHAIN
    HEADERS += wallet/osx.h
    SOURCES += wallet/osx.cpp
    LIBS += -framework Foundation -framework Security
} else:win32 {
    DEFINES += USE_WINDOWS_KEYRING
    HEADERS += wallet/windows.h
    SOURCES += wallet/windows.cpp
} else {
    HEADERS += wallet/dummy.h
    SOURCES += wallet/dummy.cpp
}
