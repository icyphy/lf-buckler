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
#include "kobukiActuator.h"
#include "kobukiSensor.h"
#include "kobukiSensorTypes.h"
#include "kobukiUtilities.h"  // Defines kobukiUARTInit(). 

// nRF library files.
#include "nrf_drv_clock.h"  // Defines nrf_drv_clock_init() and nrf_drv_clock_lfclk_request().
#include "nrf_uarte.h"
#include "nrf_serial.h"
#include "app_timer.h"      // Defines app_timer_init()

// See romi.h for function documentation.

// The following is defined in kobukiUtilities.c, which should probably go away.
extern const nrf_serial_t * serial_ref;

// NOTE: The implementation here uses code in the buckler repo.
// Eventually, it should replace all that code.

///////////////////////////////////////////////////////////////////////////////////////
//// Private functions. Not documented in romi.h. Used in public functions below.

/**
 * @brief Flush and drain the serial port.
 * @return NRF_SUCCESS if successful.
 */
int32_t romi_flush_drain_serial() {
  int32_t status = nrf_serial_flush(serial_ref, NRF_SERIAL_MAX_TIMEOUT);
  if(status != NRF_SUCCESS) {
    printf("flush error: %ld\n", status);
    return status;
  }
  status = nrf_serial_rx_drain(serial_ref);
  if(status != NRF_SUCCESS) {
    printf("rx drain error: %ld\n", status);
    return status;
  }
  return status;
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
  int32_t status = 0;
  uint8_t payloadSize = 0;
  size_t paylen;
  size_t aa_count = 0;
  uint8_t calcuatedCS = 0;
  uint8_t byteBuffer = 0;
  size_t bytes_read = 0;

  // Initialization is now occurring in romi_init().
  // APP_ERROR_CHECK(kobukiUARTInit());

  romi_flush_drain_serial();

  int num_checksum_failures = 0;

  if (len <= 4) return NRF_ERROR_NO_MEM;

  while(1){
   switch(state){
      case wait_until_header:
        bytes_read = 0;
        status = nrf_serial_read(serial_ref, header_buf, 2, &bytes_read, 100);
        if(status != NRF_SUCCESS) {
          printf("UART error: %ld. Bytes read: %d\n", status, bytes_read);
          if (aa_count++ < 20) {
            printf("\ttrying again...\n");
            romi_flush_drain_serial();
            break;
          } else {
            printf("Failed to receive from robot.\n\tIs robot powered on?\n\tTry unplugging buckler from USB and power cycle robot\n");
          }
          return status;
        }
        if (header_buf[0]==0xAA && header_buf[1]==0x55) {
          state = read_length;
        } else {
          state = wait_until_header;
        }
        aa_count = 0;
        break;

      case read_length:
        status = nrf_serial_read(serial_ref, &payloadSize, sizeof(payloadSize), NULL, 100);
        if(status != NRF_SUCCESS) {
          return status;
        }
        if(len < payloadSize+3) return NRF_ERROR_NO_MEM;
        state = read_payload;
        break;

      case read_payload:
        status = nrf_serial_read(serial_ref, packetBuffer+3, payloadSize+1, &paylen, 100);
        if(status != NRF_SUCCESS) {
          return status;
        }
        state = read_checksum;
        break;

      case read_checksum:
        memcpy(packetBuffer, header_buf, 2);
        packetBuffer[2] = payloadSize;

        calcuatedCS = romi_checksum_read(packetBuffer, payloadSize + 3);
        byteBuffer=(packetBuffer)[payloadSize+3];
        if (calcuatedCS == byteBuffer) {
          num_checksum_failures = 0;
          return NRF_SUCCESS;
        } else {
          state = wait_until_header;
          if (num_checksum_failures == 3) {
            return -1500;
          }
          num_checksum_failures++;
        }
        printf("checksum fails: %d\n", num_checksum_failures);
        break;

      default:
        break;
    }
  }
  // Initialization is now occurring in romi_init().
  // kobukiUARTUnInit();
  return status;
}

///////////////////////////////////////////////////////////////////////////////////////
//// Public functions. Documented in romi.h

int32_t romi_drive_direct(int16_t left_wheel_speed, int16_t right_wheel_speed) {
    int32_t status = 0;

    // APP_ERROR_CHECK(kobukiUARTInit());

    // Clear the serial port before using to drive the motors.
    romi_flush_drain_serial();

    // FIXME: The following function has a number of problems. Reimplement here.
    status = kobukiDriveDirect(left_wheel_speed, right_wheel_speed);
    // kobukiUARTUnInit();

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

  // The original version of this did not initialize the UART. Now we do that here.
  // This is needed in order to be able to send data to the Romi via romi_drive_direct.
  APP_ERROR_CHECK(kobukiUARTInit());

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

