#!/usr/bin/env bash

# This is a custom build script for the interrupts app, which seems
# require a completely different infrastructure from other apps
# in order to not get a naming collision on the magic function
# GPIOTE_IRQHandler which handles interrupts.

# Project root is one up from the bin directory.
PROJECT_ROOT=$LF_BIN_DIRECTORY/..

echo "starting NRF generation script into $LF_SOURCE_GEN_DIRECTORY"
echo "pwd is $(pwd)"

# Copy platform into /core
cp $PROJECT_ROOT/platform/lf_nrf52_support.c $LF_SOURCE_GEN_DIRECTORY/core/platform/
cp $PROJECT_ROOT/platform/lf_nrf52_support.h $LF_SOURCE_GEN_DIRECTORY/core/platform/
cp $PROJECT_ROOT/platform/platform.h $LF_SOURCE_GEN_DIRECTORY/core/

# Copy platform into /include/core
# TODO: Why are there two generated core dirs
cp $PROJECT_ROOT/platform/lf_nrf52_support.c $LF_SOURCE_GEN_DIRECTORY/include/core/platform/
cp $PROJECT_ROOT/platform/lf_nrf52_support.h $LF_SOURCE_GEN_DIRECTORY/include/core/platform/
cp $PROJECT_ROOT/platform/platform.h $LF_SOURCE_GEN_DIRECTORY/include/core/

# Copy custom build files into src-gen directory.
cp app_config.h buckler.h Board.mk gpio.c gpio.h software_interrupt.h $LF_SOURCE_GEN_DIRECTORY

printf '
# nRF application makefile
PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52832
SDK_VERSION = 15
SOFTDEVICE_MODEL = blank

# LF Sources and Headers
APP_SOURCES += $(notdir lf_nrf52_support.c)
APP_SOURCES += $(notdir $(wildcard ./lib/*.c))
APP_SOURCE_PATHS += ./core/platform/
APP_SOURCE_PATHS += ./lib/
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .

# Source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES += $(notdir $(wildcard ./*.c))

# Path to base of nRF52-base repo
NRF_BASE_DIR = %s/nrf52x-base/

# Include custom board Makefile
include Board.mk

# Include main Makefile
include $(NRF_BASE_DIR)make/AppMakefile.mk
' $PROJECT_ROOT/buckler/software $PROJECT_ROOT/buckler/software/boards > $LF_SOURCE_GEN_DIRECTORY/Makefile

echo "Created $LF_SOURCE_GEN_DIRECTORY/Makefile"

cd $LF_SOURCE_GEN_DIRECTORY
make flash

echo ""
echo "**** To flash the code onto the device:"
echo "cd $LF_SOURCE_GEN_DIRECTORY; make flash"
echo ""
