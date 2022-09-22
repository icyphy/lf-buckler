/**
 * @file romi.c
 * @author Edward A. Lee
 * @brief Implementation of functions for controlling a TI-RLSK Romi robot.
 * @version 0.1
 * 
 * This implementation assumes the Romi robot has an nRF52 with a Berkeley
 * Buckler board attached to its serial port, and that the Romi robot has
 * been flashed to emulate the Kobuki robot protocol
 * (see FIXME: pointer to repo for that firmware).
 * 
 * This code is based on code in the buckler repo orginally developed by
 * Jeff C. Jensen, Joshua Adkins, and Neal Jackson.
 */
#include "lib/romi.h"
#include "kobukiSensor.h"
#include "kobukiSensorTypes.h"
#include "kobukiUtilities.h"  // Defines kobukiUARTInit(). 
#include "buckler.h"
#include <math.h>

// nRF library files.
#include "nrf_drv_clock.h"  // Defines nrf_drv_clock_init() and nrf_drv_clock_lfclk_request().
#include "nrfx_uart.h"
#include "app_timer.h"      // Defines app_timer_init()

// See romi.h for function documentation.


// NOTE: The implementation here uses code in the buckler repo.
// Eventually, it should replace all that code.

///////////////////////////////////////////////////////////////////////////////////////
//// Private functions. Not documented in romi.h. Used in public functions below.

// UART implementation
static nrfx_uart_t nrfx_uart = NRFX_UART_INSTANCE(0);
static nrfx_uart_config_t nrfx_uart_cfg = NRFX_UART_DEFAULT_CONFIG;

// Copied from KobukiSendPayload
// FIXME: Clean
static int32_t _romi_send_payload(uint8_t* payload, uint8_t len) {
    uint8_t writeData[256] = {0};

    // Write move payload
    writeData[0] = 0xAA;
    writeData[1] = 0x55;
    writeData[2] = len;
    memcpy(writeData + 3, payload, len);
	writeData[3+len] = checkSum(writeData, 3 + len);

    nrfx_err_t err = nrfx_uart_tx(&nrfx_uart, writeData, len + 4); 
    return err; 
}
// Copied from kobukiDriveRadius
static int32_t _romi_drive_radius(int16_t radius, int16_t speed){
    uint8_t payload[6];
    payload[0] = 0x01;
    payload[1] = 0x04;
    memcpy(payload+2, &speed, 2);
    memcpy(payload+4, &radius, 2);

    return _romi_send_payload(payload, 6);
}

// copier from kobukuDriveDirect
static int32_t _romi_drive_direct(int16_t leftWheelSpeed, int16_t rightWheelSpeed){
	int32_t CmdSpeed;
	int32_t CmdRadius;

	if (abs(rightWheelSpeed) > abs(leftWheelSpeed)) {
	    CmdSpeed = rightWheelSpeed;
	} else {
	    CmdSpeed = leftWheelSpeed;
	}

	if (rightWheelSpeed == leftWheelSpeed) {
	    CmdRadius = 0;  // Special case 0 commands Kobuki travel with infinite radius.
	} else {
	    CmdRadius = (rightWheelSpeed + leftWheelSpeed) / (2.0 * (rightWheelSpeed - leftWheelSpeed) / 123.0);  // The value 123 was determined experimentally to work, and is approximately 1/2 the wheelbase in mm.
	    CmdRadius = round(CmdRadius);
	    //if the above statement overflows a signed 16 bit value, set CmdRadius=0 for infinite radius.
	    if (CmdRadius>32767) CmdRadius=0;
	    if (CmdRadius<-32768) CmdRadius=0;
	    if (CmdRadius==0) CmdRadius=1;  // Avoid special case 0 unless want infinite radius.
	}

	if (CmdRadius == 1){
		CmdSpeed = CmdSpeed * -1;
	}

	int32_t status = _romi_drive_radius(CmdRadius, CmdSpeed);


	return status;
}


int romi_uart_init() {
  nrfx_uart_cfg.pseltxd            = BUCKLER_UART_TX;
  nrfx_uart_cfg.pselrxd            = BUCKLER_UART_RX;
  nrfx_uart_cfg.parity             = NRF_UART_PARITY_EXCLUDED;
  nrfx_uart_cfg.baudrate           = NRF_UART_BAUDRATE_115200;
  nrfx_uart_cfg.interrupt_priority = NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY;

  // Initialize UART without
  return nrfx_uart_init(&nrfx_uart, &nrfx_uart_cfg, NULL);
}

uint8_t romi_checksum_read(uint8_t * buffer, int length){
    uint8_t cs = 0x00;
    for(int i = 2; i < length; i++ ){
    	 cs ^= buffer[i];
    }
    return cs;
}

/**
 * @brief Read a packet from the serial port.
 * Read from the serial port until a header AA55 is encountered.
 * Then read the length of the packet.
 * Then read the packet into the specified buffer.
 * 
 * @param packetBuffer The buffer into which to put the result.
 * @param len The length of the buffer.
 * @return int32_t 
 */
int32_t romi_read_serial(uint8_t* packetBuffer, uint8_t len){
  // States of the state machine depending on what is read next.
  // Header should start with AA55.
  typedef enum {
    wait_until_header,
    read_length,
    read_payload,
    read_checksum
  } state_type;

  state_type state = wait_until_header;
  uint8_t header_buf[2];
  uint8_t payloadSize = 0;
  size_t paylen;
  uint8_t calcuatedCS = 0;
  uint8_t byteBuffer = 0;
  size_t bytes_read = 0;
  nrfx_err_t ret;
  int err_cnt = 0;
  bool done = false;
  // Enable reception on UART
  nrfx_uart_rx_enable(&nrfx_uart);

  int num_checksum_failures = 0;

  if (len <= 4) {
    ret = NRF_ERROR_NO_MEM;
    done = true;
  }

  while(!done){
   switch(state){
      case wait_until_header:
        ret = nrfx_uart_rx(&nrfx_uart, header_buf, 2);
        if(ret != NRF_SUCCESS) {
          nrfx_uart_errorsrc_get(&nrfx_uart);
          err_cnt++;
          break;
        }

        if (header_buf[0]==0xAA && header_buf[1]==0x55) {
          state = read_length;
        } else {
          state = wait_until_header;
        }
        break;

      case read_length:
        ret = nrfx_uart_rx(&nrfx_uart, &payloadSize, sizeof(payloadSize));
        if(ret != NRF_SUCCESS) {
          nrfx_uart_errorsrc_get(&nrfx_uart);
          err_cnt++;
          state = wait_until_header;
          break;
        }
        if(len < payloadSize+3) {
          done = true;
          ret = NRF_ERROR_NO_MEM;
        }
        state = read_payload;
        break;

      case read_payload:
        ret = nrfx_uart_rx(&nrfx_uart, packetBuffer + 3, payloadSize + 1);
        if(ret != NRF_SUCCESS) {
          err_cnt++;
          nrfx_uart_errorsrc_get(&nrfx_uart);
          state = wait_until_header;
          break;
        }
        state = read_checksum;
        break;

      case read_checksum:
        memcpy(packetBuffer, header_buf, 2);
        packetBuffer[2] = payloadSize;

        calcuatedCS = romi_checksum_read(packetBuffer, payloadSize + 3);
        byteBuffer=(packetBuffer)[payloadSize+3];
        done = true;
        if (calcuatedCS == byteBuffer) {
          num_checksum_failures = 0;
          ret = NRF_SUCCESS;
        } else {
          state = wait_until_header;
          if (num_checksum_failures == 3) {
            ret = -1500;
          }
          num_checksum_failures++;
        }
        break;

      default:
        break;
    }
    // If we get too many errors abort the sensor reading.
    if (ret != NRF_SUCCESS && err_cnt >= 20) {
      done = true;
    }
  }
  nrfx_uart_rx_disable(&nrfx_uart);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////
//// Public functions. Documented in romi.h

int32_t romi_drive_direct(int16_t left_wheel_speed, int16_t right_wheel_speed) {
    int32_t status = 0;

    // FIXME: The following function has a number of problems. Reimplement here.
    status = _romi_drive_direct(left_wheel_speed, right_wheel_speed);

    return status;
}

uint32_t romi_init() {
  uint32_t err_code = nrf_drv_clock_init();
  if (err_code != NRF_ERROR_MODULE_ALREADY_INITIALIZED){
    APP_ERROR_CHECK(err_code);
  }

  nrf_drv_clock_lfclk_request(NULL);
  err_code = app_timer_init();
  if (err_code != NRF_ERROR_MODULE_ALREADY_INITIALIZED){
    APP_ERROR_CHECK(err_code);
  }

  err_code = romi_uart_init();
  APP_ERROR_CHECK(err_code);

  // Make sure the robot is stopped.
  romi_drive_direct(0, 0);

  return err_code;
}

int32_t romi_sensor_poll(KobukiSensors_t* const	sensors) {
	// initialize communications buffer
    // We know that the maximum size of the packet is less than 140 based on documentation
	uint8_t packet[140] = {0};
	int status = romi_read_serial(packet, 140);
    if (status != NRF_SUCCESS) {
        return status;
    }

	// parse response
    kobukiParseSensorPacket(packet, sensors);

	return status;
}
