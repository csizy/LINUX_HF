/**\
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/*
 * FileName:	bme280_qt_interf_v2.h
 * Co-author:   Adam Csizy
 * 
 * Desc.:		Custom driver interface heavily based on the Bosch Sensortech BME280 sensor driver.
 *              This wrapper like interface offers easy usage for top level user space applications
 *              such as Qt projects to interface the BME280 sensor for environment monitoring purposes.
 */

/******************************************************************************/
/*!                         Own header files                                  */
#include "bme280.h"

/******************************************************************************/
/*!								Own macros									  */
#define I2C_DRIVER_PATH	("/dev/i2c-1")

/******************************************************************************/
/*!                               Structures                                  */

/* Structure that contains identifier details used in example */
struct identifier
{
    /* Variable to hold device address */
    uint8_t dev_addr;

    /* Variable that contains file descriptor */
    int8_t fd;
};

/* Structure that contains ambient data measured by the sensor device  */
struct sensor_data
{
	/* Variable to hold temperature value */
	float temp;
	
	/* Variable to hold pressure value */
	float press;
	
	/* Variable to hold humidity value */
	float hum;
};

/****************************************************************************/
/*!                         Functions                                       */

/*!
 *  @brief Function that creates a mandatory delay required in some of the APIs.
 *
 * @param[in] period              : Delay in microseconds.
 * @param[in, out] intf_ptr       : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs
 *  @return void.
 *
 */
void user_delay_us(uint32_t period, void *intf_ptr);

/*!
 * @brief Function for print the temperature, humidity and pressure data.
 *
 * @param[out] comp_data    :   Structure instance of bme280_data
 *
 * @note Sensor data whose can be read
 *
 * sens_list
 * --------------
 * Pressure
 * Temperature
 * Humidity
 *
 */
void print_sensor_data(struct bme280_data *comp_data);

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[out] data          : Pointer to the data buffer to store the read data.
 *  @param[in] len            : No of bytes to read.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs.
 *
 *  @return Status of execution
 *
 *  @retval 0 -> Success
 *  @retval > 0 -> Failure Info
 *
 */
int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr);

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] reg_addr       : Register address.
 *  @param[in] data           : Pointer to the data buffer whose value is to be written.
 *  @param[in] len            : No of bytes to write.
 *  @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
 *                                  for interface related call backs
 *
 *  @return Status of execution
 *
 *  @retval BME280_OK -> Success
 *  @retval BME280_E_COMM_FAIL -> Communication failure.
 *
 */
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr);

/*!
 * @brief Function reads temperature, humidity and pressure data in forced mode.
 *
 * @param[in] dev   :   Structure instance of bme280_dev.
 *
 * @return Result of API execution status
 *
 * @retval BME280_OK - Success.
 * @retval BME280_E_NULL_PTR - Error: Null pointer error
 * @retval BME280_E_COMM_FAIL - Error: Communication fail error
 * @retval BME280_E_NVM_COPY_FAILED - Error: NVM copy failed
 *
 */
int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev);

/*!
 * @brief Initializes the sensor device with settings optimized for weather monitoring.
 *
 * @param[in, out] id              	: Sensor device identifier structure
 * @param[in, out] dev       		: Sensor device settings structure
 * 
 * @return Status of execution.
 * 
 * @retval 0  -> Success
 * @retval -1 -> Failure
 *
 */
int init_dev_weather(struct identifier *id, struct bme280_dev *dev);

/*!
 * @brief Function that returns the minimal required delay in milliseconds between consecutive measurements based on the sensor settings.
 *		  DO NOT CALL BEFORE SENSOR INITIALIZATION AND MEASUREMENT CONFIGURATIONS !!!
 * 
 * @param[in, out] id              	: Sensor device identifier structure
 * @param[in, out] dev       		: Sensor device settings structure
 *  
 * @return Minimum delay in milliseconds between consecutive measurements.
 *
 */
uint32_t get_min_delay(struct identifier *id, struct bme280_dev *dev);

/*!
 * @brief Function that returns sensor data based on the device settings.
 *
 * @param[in, out] id              	: Sensor device identifier structure
 * @param[in, out] dev       		: Sensor device settings structure
 * @param[in] req_delay_ms			: Delay in milliseconds required for measurement completion
 * @param[out] data					: Ambient data measured by the sensor device
 * 
 * @return Status of execution.
 *
 * @retval 0  -> Success
 * @retval -1 -> Failure
 */
 int get_sensor_data(struct identifier *id, struct bme280_dev *dev, uint32_t req_delay_ms, struct sensor_data *data);
 
 /*!
 * @brief Function that closes communication with the sensor device.
 *
 * @param[in, out] id              	: Sensor device identifier structure
 * @param[in, out] dev       		: Sensor device settings structure
 * 
 * @return Status of execution.
 *
 * @retval 0  -> Success
 * @retval -1 -> Failure
 */
 int close_dev_connection(struct identifier *id, struct bme280_dev *dev);
