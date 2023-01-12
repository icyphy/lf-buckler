#!/usr/bin/env bash

# Project root is one up from the bin directory.
PROJECT_ROOT=$LF_BIN_DIRECTORY/..


echo "starting NRF generation script into $LF_SOURCE_GEN_DIRECTORY"
echo "pwd is $(pwd)"

# Copy the lib directory.
cp -r $PROJECT_ROOT/lib/* $LF_SOURCE_GEN_DIRECTORY/lib
# Copy makefile
cp $PROJECT_ROOT/platform/Makefile $LF_SOURCE_GEN_DIRECTORY/

# TODO Push into reactor-c
cp $PROJECT_ROOT/platform/lf_types.h $LF_SOURCE_GEN_DIRECTORY/include/core/
rm $LF_SOURCE_GEN_DIRECTORY/lib/util.c

printf '
PROJECT_ROOT = %s
NRF_BASE_DIR = $(PROJECT_ROOT)/buckler/software/nrf52x-base/

# Include board Makefile (if any)
include $(PROJECT_ROOT)/boards/lf_buckler_revC/Board.mk

# Include main Makefile
include $(PROJECT_ROOT)/buckler/software/nrf52x-base/make/AppMakefile.mk
' $PROJECT_ROOT >> $LF_SOURCE_GEN_DIRECTORY/Makefile


echo "Copied $LF_SOURCE_GEN_DIRECTORY/Makefile"

cd $LF_SOURCE_GEN_DIRECTORY
make flash

echo ""
echo "**** To get printf outputs:"
echo "cd $LF_SOURCE_GEN_DIRECTORY; make rtt"
echo ""
