##-*-makefile-*-########################################################################################################
# Copyright 2016 - 2022 Inesonic, LLC
# 
# This file is licensed under two licenses.
#
# Inesonic Commercial License, Version 1:
#   All rights reserved.  Inesonic, LLC retains all rights to this software, including the right to relicense the
#   software in source or binary formats under different terms.  Unauthorized use under the terms of this license is
#   strictly prohibited.
#
# GNU Public License, Version 2:
#   This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public
#   License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later
#   version.
#   
#   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
#   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
#   details.
#   
#   You should have received a copy of the GNU General Public License along with this program; if not, write to the Free
#   Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
########################################################################################################################

########################################################################################################################
# Basic build characteristics
#

TEMPLATE = app
QT += core testlib gui widgets network
CONFIG += testcase c++14

HEADERS = application_wrapper.h \
          test_usage_data.h \

SOURCES = test_ineud.cpp \
          application_wrapper.cpp \
          test_usage_data.cpp \

########################################################################################################################
# ineud library:
#

UD_BASE = $${OUT_PWD}/../ineud/
INCLUDEPATH = $${PWD}/../ineud/include/

unix {
    CONFIG(debug, debug|release) {
        LIBS += -L$${UD_BASE}/build/debug/ -lineud
        PRE_TARGETDEPS += $${UD_BASE}/build/debug/libineud.so
    } else {
        LIBS += -L$${UD_BASE}/build/release/ -lineud
        PRE_TARGETDEPS += $${UD_BASE}/build/release/libineud.so
    }
}

win32 {
    CONFIG(debug, debug|release) {
        LIBS += $${UD_BASE}/build/Debug/ineud.lib
        PRE_TARGETDEPS += $${UD_BASE}/build/Debug/ineud.lib
    } else {
        LIBS += $${UD_BASE}/build/Release/ineud.lib
        PRE_TARGETDEPS += $${UD_BASE}/build/Release/ineud.lib
    }
}

########################################################################################################################
# Libraries
#

INCLUDEPATH += $${INECRYPTO_INCLUDE}
INCLUDEPATH += $${INEWH_INCLUDE}
INCLUDEPATH += $${BOOST_INCLUDE}

LIBS += -L$${INECRYPTO_LIBDIR} -linecrypto
LIBS += -L$${INEWH_LIBDIR} -linewh

########################################################################################################################
# Locate build intermediate and output products
#

TARGET = test_ineud

CONFIG(debug, debug|release) {
    unix:DESTDIR = build/debug
    win32:DESTDIR = build/Debug
} else {
    unix:DESTDIR = build/release
    win32:DESTDIR = build/Release
}

OBJECTS_DIR = $${DESTDIR}/objects
MOC_DIR = $${DESTDIR}/moc
RCC_DIR = $${DESTDIR}/rcc
UI_DIR = $${DESTDIR}/ui
