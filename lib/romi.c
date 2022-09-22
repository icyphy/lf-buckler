/**
 * @file romi.c
 * @author Edward A. Lee
 * @author Erling Jellum
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
#include "buckler.h"
#include <math.h>
#include <string.h> // Defines memcpy
#include <stdlib.h> // Defines abs
#include <stdint.h> // Defines uint8_t, etc.

// nRF library files.
#include "nrf_drv_clock.h"  // Defines nrf_drv_clock_init() and nrf_drv_clock_lfclk_request().
#include "nrfx_uart.h"
#include "app_timer.h"      // Defines app_timer_init()

// See romi.h for function documentation.


// NOTE: The implementation here uses code in the buckler repo.
// Eventually, it should replace all that code.

///////////////////////////////////////////////////////////////////////////////////////
//// Private functions. Not documented in romi.h. Used in public functions below.
//// Not intended to be called by users.

// UART implementation
static nrfx_uart_t nrfx_uart = NRFX_UART_INSTANCE(0);
static nrfx_uart_config_t nrfx_uart_cfg = NRFX_UART_DEFAULT_CONFIG;

/**
 * @brief Calculate and return a checksum for the specified buffer.
 * This is based on checkSum() in the buckler repo.
 * 
 * @param buffer The buffer.
 * @param length The length of the buffer.
 * @return The XOR of the bytes in the buffer. 
 */
uint8_t _romi_checksum(uint8_t* buffer, int length) {
    uint8_t cs = 0x00;
    // printf("\n l = %d \n",length);
    for(int i = 2; i < length; i++ ) {
        // printf("%d ",buffer[i]);
        cs ^= buffer[i];
    }
    // printf("\n last[%d]=%d cs=%d",i,buffer[i],cs);
    return cs;
}

/**
 * @brief Send the specified payload over UART to the Romi robot.
 * This function adds a header (0xAA and 0x55) and a length to
 * the front of the byte stream that is sent, plus a checksum at
 * the end.
 * This is based on kobukiSendPayload from the buckler repo,
 * converted to use lower-level UART functions.
 * 
 * @param payload The payload.
 * @param length The size of the payload.
 * @return An error code that should be checked using the macro APP_ERROR_CHECK.
 */
static int32_t _romi_send_payload(uint8_t* payload, uint8_t length) {
    uint8_t writeData[256] = {0};

    // Write move payload
    writeData[0] = 0xAA;
    writeData[1] = 0x55;
    writeData[2] = length;
    memcpy(writeData + 3, payload, length);
    writeData[3 + length] = _romi_checksum(writeData, 3 + length);

    return nrfx_uart_tx(&nrfx_uart, writeData, length + 4); 
}

/**
 * @brief Send a drive command with specified radius and speed to the Romi.
 * This is based on kobukiDriveRadius from the buckler repo.
 * 
 * @param radius The radius.
 * @param speed The speed.
 * @return An error code that should be checked using the macro APP_ERROR_CHECK.
 */
static int32_t _romi_drive_radius(int16_t radius, int16_t speed){
    uint8_t payload[6];
    payload[0] = 0x01;
    payload[1] = 0x04;
    memcpy(payload+2, &speed, 2);
    memcpy(payload+4, &radius, 2);

    return _romi_send_payload(payload, 6);
}

/**
 * @brief Initialize the UART to a baud rate of 115,200.
 * @return An error code that should be checked using the macro APP_ERROR_CHECK.
 */
int32_t _romi_uart_init() {
  nrfx_uart_cfg.pseltxd            = BUCKLER_UART_TX;
  nrfx_uart_cfg.pselrxd            = BUCKLER_UART_RX;
  nrfx_uart_cfg.parity             = NRF_UART_PARITY_EXCLUDED;
  nrfx_uart_cfg.baudrate           = NRF_UART_BAUDRATE_115200;
  nrfx_uart_cfg.interrupt_priority = NRFX_UART_DEFAULT_CONFIG_IRQ_PRIORITY;

  // Initialize UART.
  return nrfx_uart_init(&nrfx_uart, &nrfx_uart_cfg, NULL);
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
int32_t _romi_read_serial(uint8_t* packetBuffer, uint8_t len){
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
  uint8_t calcuatedCS = 0;
  uint8_t byteBuffer = 0;
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

        calcuatedCS = _romi_checksum(packetBuffer, payloadSize + 3);
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
//// Public functions. Documented in romi.h. Intended to be called by users.

// Based on kobukiDriveDirect from the buckler repo, but with a number of fixes.
int32_t romi_drive_direct(int16_t left_wheel_speed, int16_t right_wheel_speed) {
    // Convert the independent wheel speeds into an approximate radius and speed.
    int32_t speed;
    int32_t radius;
    int32_t left_right;

    // Choose the larger of the two speeds.
    if (abs(right_wheel_speed) > abs(left_wheel_speed)) {
        speed = right_wheel_speed;
        left_right = 1;
    } else {
        speed = left_wheel_speed;
        left_right = -1;
    }

    if (right_wheel_speed == left_wheel_speed) {
        radius = 0;  // Special case 0 commands Kobuki travel with infinite radius.
    } else {
        double estimate = (right_wheel_speed + left_wheel_speed) / (2.0 * (right_wheel_speed - right_wheel_speed) / 123.0);  
        // The value 123 was determined experimentally to work, and is approximately 1/2 the wheelbase in mm.

        radius = round(estimate);
        if (radius == 0) radius = left_right;  // Avoid special case 0 unless want infinite radius.
        //if the above statement overflows a signed 16 bit value, set radius=0 for infinite radius.
        if (radius > 32767) radius = 0;
        if (radius <- 32768) radius = 0;
    }
    /* Original had the following rather odd adjustment with no comment.
    if (radius == 1){
        speed = speed * -1;
    }
    */

    return _romi_drive_radius(radius, speed);
}

uint32_t romi_init() {
  uint32_t err_code = nrf_drv_clock_init();
  if (err_code != NRF_ERROR_MODULE_ALREADY_INITIALIZED){
    APP_ERROR_CHECK(err_code);
    err_code = NRF_SUCCESS; // This is OK.
  }

  nrf_drv_clock_lfclk_request(NULL);
  err_code = app_timer_init();
  if (err_code != NRF_ERROR_MODULE_ALREADY_INITIALIZED){
    APP_ERROR_CHECK(err_code);
    err_code = NRF_SUCCESS; // This is OK.
  }

  err_code = _romi_uart_init();
  if (err_code != NRF_SUCCESS) {
    APP_ERROR_CHECK(err_code);
    return err_code;
  }

  // Make sure the robot is stopped.
  err_code = romi_drive_direct(0, 0);
  APP_ERROR_CHECK(err_code);

  return err_code;
}

int32_t romi_sensor_poll(KobukiSensors_t* const    sensors) {
    // initialize communications buffer
    // We know that the maximum size of the packet is less than 140 based on documentation
    uint8_t packet[140] = {0};
    int status = _romi_read_serial(packet, 140);
    if (status != NRF_SUCCESS) {
        return status;
    }

    // parse response
    kobukiParseSensorPacket(packet, sensors);

    return status;
}
