# TODO
# lf_types.h needs header change from "modes.h" to "modal_models/modes.h" due to conflicting header in sdk
# find ways to keep priority
# delete deprecated util.c in lib 

# nRF application makefile
PROJECT_NAME = $(shell basename "$(realpath ./)")

# Configurations
NRF_IC = nrf52832
SDK_VERSION = 15
SOFTDEVICE_MODEL = s132

# LF Paths
APP_HEADER_PATHS += \
	./include/ \
	./include/api/ \
	./include/core/  \
	./include/core/utils/ \
	./include/core/platform/ \

APP_SOURCE_PATHS += \
	./lib/ \
	./core/ \
	./core/platform/ \
	./core/utils/ \
	./core/modal_models/ \

#  LF Sources
APP_SOURCES += \
	lf_nrf52_support.c \
	reactor.c \
	reactor_common.c \
	mixed_radix.c \
	port.c \
	tag.c \
	utils/pqueue.c \
	utils/vector.c \
	utils/util.c \
	modal_models/modes.c \
	schedule.c \
	filter.c \
	romi.c \


override CFLAGS += -DLF_UNTHREADED
override CFLAGS += -DPLATFORM_NRF52
override CFLAGS += -DINITIAL_EVENT_QUEUE_SIZE=10
override CFLAGS += -DINITIAL_REACT_QUEUE_SIZE=10

# Main source and header files
APP_HEADER_PATHS += .
APP_SOURCE_PATHS += .
APP_SOURCES += $(notdir $(wildcard ./*.c))

NRF_BASE_DIR = $(PROJECT_ROOT)/buckler/software/nrf52x-base/

# Include board Makefile (if any)
include $(PROJECT_ROOT)/boards/lf_buckler_revC/Board.mk

# Include main Makefile
include $(PROJECT_ROOT)/buckler/software/nrf52x-base/make/AppMakefile.mk