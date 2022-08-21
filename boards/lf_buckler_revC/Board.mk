# Board-specific configurations for the Berkeley Buckler

# Ensure that this file is only included once
ifndef BOARD_MAKEFILE
BOARD_MAKEFILE = 1

# Board-specific configurations
BOARD = Buckler_revC
USE_BLE = 1

# Get directory of this makefile
BOARD_DIR := $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

# Include any files in this directory in the build process
BOARD_SOURCE_PATHS = $(BOARD_DIR)/.
BOARD_SOURCE_PATHS += $(wildcard $(BOARD_DIR)/../../libraries/*/)

BOARD_HEADER_PATHS = $(BOARD_DIR)/.
BOARD_HEADER_PATHS += $(BOARD_DIR)/../.
BOARD_HEADER_PATHS += $(wildcard $(BOARD_DIR)/../../libraries/*/)

BOARD_LINKER_PATHS = $(BOARD_DIR)/.

BOARD_SOURCES = $(notdir $(wildcard $(BOARD_DIR)/./*.c))
BOARD_SOURCES += $(notdir $(wildcard $(BOARD_DIR)/../../libraries/*/*.c))
BOARD_AS = $(notdir $(wildcard $(BOARD_DIR)/./*.s))

# Convert board to upper case
BOARD_UPPER = $(shell echo $(BOARD) | tr a-z A-Z)

# Additional #define's to be added to code by the compiler
BOARD_VARS = \
	BOARD_$(BOARD_UPPER)\
	CUSTOM_BOARD_INC=buckler\
	USE_APP_CONFIG\
	DEBUG\
	DEBUG_NRF\

# Default SDK source files to be included
BOARD_SOURCES += \
	SEGGER_RTT.c\
	SEGGER_RTT_Syscalls_GCC.c\
	SEGGER_RTT_printf.c\
	app_error.c\
	app_error_handler_gcc.c\
	app_pwm.c\
	app_scheduler.c\
	app_timer.c\
	app_uart.c\
	app_util_platform.c\
	before_startup.c\
	ff.c\
	hardfault_handler_gcc.c\
	hardfault_implementation.c\
	mmc_nrf.c\
	nrf_assert.c\
	nrf_atomic.c\
	nrf_balloc.c\
	nrf_drv_clock.c\
	nrf_drv_ppi.c\
	nrf_drv_spi.c\
	nrf_drv_twi.c\
	nrf_drv_uart.c\
	nrf_fprintf.c\
	nrf_fprintf_format.c\
	nrf_fstorage.c\
	nrf_log_backend_rtt.c\
	nrf_log_backend_serial.c\
	nrf_log_backend_uart.c\
	nrf_log_default_backends.c\
	nrf_log_frontend.c\
	nrf_log_str_formatter.c\
	nrf_memobj.c\
	nrf_pwr_mgmt.c\
	nrf_ringbuf.c\
	nrf_queue.c\
	nrf_section_iter.c\
	nrf_serial.c\
	nrf_strerror.c\
	nrf_twi_mngr.c\
	nrfx_clock.c\
	nrfx_gpiote.c\
	nrfx_ppi.c\
	nrfx_prs.c\
	nrfx_pwm.c\
	nrfx_saadc.c\
	nrfx_spi.c\
	nrfx_spim.c\
	nrfx_timer.c\
	nrfx_twi.c\
	nrfx_twim.c\
	nrfx_uart.c\
	nrfx_uarte.c\
	simple_logger.c\

ifneq ($(SOFTDEVICE_MODEL),blank)
BOARD_SOURCES += \
	ble_advdata.c\
	ble_advertising.c\
	ble_conn_params.c\
	ble_srv_common.c\
	nrf_ble_gatt.c\
	nrf_ble_qwr.c\
	nrf_sdh.c\
	nrf_sdh_ble.c\
	simple_ble.c\

endif

endif

