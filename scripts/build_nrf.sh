#!/usr/bin/env bash

# Project root is one up from the bin directory.
PROJECT_ROOT=$LF_BIN_DIRECTORY/../

echo "starting NRF generation script into $LF_SOURCE_GEN_DIRECTORY"
echo "pwd is $(pwd)"

cp $PROJECT_ROOT/platform/lf_nRF52832_support.c $LF_SOURCE_GEN_DIRECTORY/core/platform/
cp $PROJECT_ROOT/platform/lf_nRF52832_support.h $LF_SOURCE_GEN_DIRECTORY/core/platform/
cp $PROJECT_ROOT/platform/platform.h $LF_SOURCE_GEN_DIRECTORY/core/
cp $PROJECT_ROOT/platform/include_nrf.c $LF_SOURCE_GEN_DIRECTORY/

printf '
# nRF application makefile
PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52832
SDK_VERSION = 15
SOFTDEVICE_MODEL = s132

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES = $(notdir $(wildcard ./*.c))

# Path to base of nRF52-base repo
NRF_BASE_DIR = %s/nrf52x-base/

# Include board Makefile (if any)
include %s/buckler_revC/Board.mk

# Include main Makefile
include $(NRF_BASE_DIR)make/AppMakefile.mk
' $PROJECT_ROOT/buckler/software $PROJECT_ROOT/buckler/software/boards > $LF_SOURCE_GEN_DIRECTORY/Makefile

echo "Created $LF_SOURCE_GEN_DIRECTORY/Makefile"

# Unconditionally flash, which assumes the board is connected.
# read -p "cd and run make flash? y/n: " yn
# case $yn in
#  [Yy]* ) cd $TARGET_DIR; make flash; exit;;
#  * ) exit;;
# esac

cd $LF_SOURCE_GEN_DIRECTORY
make

echo "**** To flash the code onto the device: cd $LF_SOURCE_GEN_DIRECTORY; make flash"
