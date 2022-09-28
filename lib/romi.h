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
#include <stdint.h> // Defines uint8_t, etc.

//////////////////////////////////////////////////////////////
//// Data structures

/**
 * @brief Bumpers.
 */
typedef struct
{
    // Indicates if bumper is currently pressed
    bool left;   // Or of two left bump sensors.
    bool center; // Or of two center bump sensors.
    bool right;  // Or of two right bump sensors.
} romi_bumps_t;

/**
 * @brief Buttons.
 */
typedef struct
{
    bool left;
    bool right;
} romi_buttons_t;

/**
 * @brief Reflectance, which is 0 for light color, 1 for dark color.
 */
typedef struct {
    bool left;     // Left two sensors.
    bool center;   // Middle four sensors.
    bool right;    // Right two sensors.
} romi_reflectance_t;

/**
 * @brief Motor encoder data.
 */
typedef struct
{
    // 16 bit unsigned roll over. Forward is positive.
    uint16_t left;
    uint16_t right;
} romi_encoder_t;

/**
 * @brief Collection of sensor data from the Romi.
 */
typedef struct
{
    uint16_t time_stamp;

    // Bump sensors.
    romi_bumps_t bumps;

    // Buttons.
    romi_buttons_t buttons;

    // Reflectance sensor.
    romi_reflectance_t reflectance;

    // Wheel encoders.
    romi_encoder_t encoders;

} romi_sensors_t;

//////////////////////////////////////////////////////////////
//// Functions

/**
 * @brief Return true if the left or right button changes from not pressed to pressed.
 * This function compares the state of the buttons to what they were on
 * the previous call to this function, and if either button is now pressed and
 * was not previously pressed, this returns true.
 *
 * @param sensors The sensor struct for the Romi.
 * @return true If a button has been pressed.
 */
bool romi_button_pressed(romi_sensors_t *const sensors);

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
int32_t romi_sensors_poll(romi_sensors_t *const sensors);

#endif