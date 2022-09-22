/**
 * @file romi.h
 * @author Edward A. Lee
 * @brief Header file for functions for controlling a TI-RLSK Romi robot.
 * @version 0.1
 */
#ifndef ROMI_H
#define ROMI_H

// To define APP_ERROR_CHECK, a macro that flashes the lights on a robot when
// something goes wrong.
#include "app_error.h"
#include "nrfx_uart.h"

// KobokiSensor_t is defined in the following file.
#include "kobukiSensorTypes.h"

extern nrfx_uart_t nrfx_uart;
/**
 * @brief Set the speed of the left and right wheels.
 * The speed is in units of mm/s.
 * A good starting point for experimentations is 75 mm/s.
 * This function assumes that romi_init() has been called.
 * 
 * @param left_wheel_speed The left wheel speed in mm/s.
 * @param right_wheel_speed The right wheel speed in mm/s.
 * @return int32_t An error code that should be checked using the macro APP_ERROR_CHECK.
 */
int32_t romi_drive_direct(int16_t leftWheelSpeed, int16_t rightWheelSpeed);

/**
 * @brief Initialize the Romi robot.
 * @return int32_t An error code that should be checked using the macro APP_ERROR_CHECK.
 */
uint32_t romi_init();

/**
 * @brief Read the sensors from the Romi robot.
 * 
 * @param sensors The struct into which to write the sensor values.
 * @return int32_t An error code that should be checked using the macro APP_ERROR_CHECK.
 */
int32_t romi_sensor_poll(KobukiSensors_t* const	sensors);

#endif