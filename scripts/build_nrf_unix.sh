#!/usr/bin/env bash

# Project root is one up from the bin directory.
PROJECT_ROOT=$LF_BIN_DIRECTORY/..


echo "starting NRF generation script into $LF_SOURCE_GEN_DIRECTORY"
echo "pwd is $(pwd)"

# Copy platform into /core
cp $PROJECT_ROOT/platform/lf_nrf52_support.c $LF_SOURCE_GEN_DIRECTORY/core/platform/
cp $PROJECT_ROOT/platform/lf_nrf52_support.h $LF_SOURCE_GEN_DIRECTORY/core/platform/
cp $PROJECT_ROOT/platform/platform.h $LF_SOURCE_GEN_DIRECTORY/core/
cp $PROJECT_ROOT/platform/reactor.c $LF_SOURCE_GEN_DIRECTORY/core/
cp $PROJECT_ROOT/platform/reactor_common.c $LF_SOURCE_GEN_DIRECTORY/core/

# Copy platform into /include/core
# TODO: Why are there two generated core dirs
cp $PROJECT_ROOT/platform/lf_nrf52_support.c $LF_SOURCE_GEN_DIRECTORY/include/core/platform/
cp $PROJECT_ROOT/platform/lf_nrf52_support.h $LF_SOURCE_GEN_DIRECTORY/include/core/platform/
cp $PROJECT_ROOT/platform/platform.h $LF_SOURCE_GEN_DIRECTORY/include/core/
cp $PROJECT_ROOT/platform/reactor.c $LF_SOURCE_GEN_DIRECTORY/include/core/
cp $PROJECT_ROOT/platform/reactor_common.c $LF_SOURCE_GEN_DIRECTORY/include/core/

printf '
# nRF application makefile
PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52832
SDK_VERSION = 15
SOFTDEVICE_MODEL = s132

# LF Sources and Headers
APP_SOURCES += $(notdir lf_nrf52_support.c)
APP_SOURCES += $(notdir $(wildcard ./lib/*.c))
APP_SOURCE_PATHS += ./core/platform/
APP_SOURCE_PATHS += ./lib/

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES += $(notdir $(wildcard ./*.c))

# Path to base of nRF52-base repo
NRF_BASE_DIR = %s/nrf52x-base/

# Include board Makefile (if any)
include %s/Board.mk

# Include main Makefile
include $(NRF_BASE_DIR)make/AppMakefile.mk
' $PROJECT_ROOT/buckler/software $PROJECT_ROOT/boards/lf_buckler_revC > $LF_SOURCE_GEN_DIRECTORY/Makefile

echo "Created $LF_SOURCE_GEN_DIRECTORY/Makefile"

cd $LF_SOURCE_GEN_DIRECTORY
make flash

echo ""
echo "**** To get printf outputs:"
echo "cd $LF_SOURCE_GEN_DIRECTORY; make rtt"
echo ""
