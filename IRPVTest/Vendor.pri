# This file contains qmake directives that controls linking with the Vendor's image recognition API

# In the simplest case Vendor's API is stand alone and it does not depend on any 3rd parties,
# so only API name should be specified here below

# Specify API name according to the naming convention 'irpv_11_[vendor]_[version]_(cpu/gpu)' =======
API_NAME = irpv_11_null_0_cpu

# If Vendor's API depends on any 3rd parties software you may specify this below

# Section for MS Windows ================================================
win32 {
    # My convention for build path on windows
    win32-msvc2013: COMPILER = vc12
    win32-msvc2015: COMPILER = vc14
    win32-msvc2017: COMPILER = vc16
    win32-g++:      COMPILER = mingw
    win32:contains(QMAKE_TARGET.arch, x86_64){
        ARCH = x64
    } else {
        ARCH = x86
    }
    LIB_DIR = $${PWD}/../API_bin/$${API_NAME}/$${ARCH}/$${COMPILER}
    # Specify where linker should seek your libraries and their 3rdparty dependencies
    LIBS += -L$${PWD}/../API_bin/$${API_NAME} \
            -L$${LIB_DIR}

    # Specify libraries that should be linked
    LIBS += -l$${API_NAME}
}

# Section for the Linux ====================================================
linux {
    # Specify where linker should seek your libraries and their 3rdparty dependencies
    LIBS += -L$${PWD}/../API_bin/$${API_NAME}

    # Specify libraries that should be linked
    LIBS += -l$${API_NAME}
}

# Do not modify =============================================
DEFINES += VENDOR_API_NAME=\\\"$${API_NAME}\\\"

INCLUDEPATH += $${PWD}/..

