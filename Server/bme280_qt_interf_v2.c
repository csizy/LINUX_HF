/**\
 * Copyright (c) 2020 Bosch Sensortec GmbH. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

/*
 * FileName:    bme280_qt_interf_v2.c
 * Co-author:   Adam Csizy
 * 
 * Desc.:       Custom driver interface heavily based on the Bosch Sensortech BME280 sensor driver.
 *              This wrapper like interface offers easy usage for top level user space applications
 *              such as Qt projects to interface the BME280 sensor for environment monitoring purposes.
 */

#ifdef __KERNEL__
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#endif

/******************************************************************************/
/*!                         System header files                               */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
// Explicit kernel specific includes for this version of I2C read/write implementation:
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "bme280_qt_interf_v2.h"

/*!
 * @brief This function reads the sensor's registers through I2C bus.
 */
int8_t user_i2c_read(uint8_t reg_addr, uint8_t *data, uint32_t len, void *intf_ptr)
{
    
    const struct identifier *id;
    
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data msgSet[1];
    
    id = (const struct identifier*)intf_ptr;
    
    msgs[0].addr = id->dev_addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg_addr;
    
    msgs[1].addr = id->dev_addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = len;
    msgs[1].buf = data;
    
    msgSet[0].msgs = msgs;
    msgSet[0].nmsgs = 2;
    
    if(ioctl(id->fd, I2C_RDWR, msgSet) < 0) {
     
        perror("ioctl(I2C_RDWR) in user_i2c_read");
        return BME280_E_COMM_FAIL;
    }
    
    return 0;
}

/*!
 * @brief This function provides the delay for required time (Microseconds) as per the input provided in some of the
 * APIs
 */
void user_delay_us(uint32_t period, void *intf_ptr)
{
    usleep(period);
}

/*!
 * @brief This function for writing the sensor's registers through I2C bus.
 */
int8_t user_i2c_write(uint8_t reg_addr, const uint8_t *data, uint32_t len, void *intf_ptr)
{
    uint8_t *buf;
    const struct identifier *id;
    
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data msgSet[1];
    
    id = (const struct identifier*)intf_ptr;
    
    buf = (uint8_t*)calloc(len + 1, sizeof(uint8_t));
    buf[0] = reg_addr;
    memcpy(buf + 1, data, len);
    
    msgs[0].addr = id->dev_addr;
    msgs[0].flags = 0;
    msgs[0].len = len + 1;
    msgs[0].buf = buf;
    
    msgSet[0].msgs = msgs;
    msgSet[0].nmsgs = 1;
    
    if(ioctl(id->fd, I2C_RDWR, msgSet) < 0) {
     
        perror("ioctl(I2C_RDWR) in user_i2c_write");
        return BME280_E_COMM_FAIL;
    }

    free(buf);

    return BME280_OK;
}

/*!
 * @brief This API used to print the sensor temperature, pressure and humidity data.
 */
void print_sensor_data(struct bme280_data *comp_data)
{
    float temp, press, hum;

#ifdef BME280_FLOAT_ENABLE
    temp = comp_data->temperature;
    press = 0.01 * comp_data->pressure;
    hum = comp_data->humidity;
#else
#ifdef BME280_64BIT_ENABLE
    temp = 0.01f * comp_data->temperature;
    press = 0.0001f * comp_data->pressure;
    hum = 1.0f / 1024.0f * comp_data->humidity;
#else
    temp = 0.01f * comp_data->temperature;
    press = 0.01f * comp_data->pressure;
    hum = 1.0f / 1024.0f * comp_data->humidity;
#endif
#endif
    printf("%0.2lf deg C, %0.2lf hPa, %0.2lf%%\n", temp, press, hum);
}

/*!
 * @brief This API reads the sensor temperature, pressure and humidity data in forced mode.
 */
int8_t stream_sensor_data_forced_mode(struct bme280_dev *dev)
{
    /* Variable to define the result */
    int8_t rslt = BME280_OK;

    /* Variable to define the selecting sensors */
    uint8_t settings_sel = 0;

    /* Variable to store minimum wait time between consecutive measurement in force mode */
    uint32_t req_delay;

    /* Structure to get the pressure, temperature and humidity values */
    struct bme280_data comp_data;

    /* Recommended mode of operation: Advanced Weather Monitoring */
    dev->settings.osr_h = BME280_OVERSAMPLING_1X;
    dev->settings.osr_p = BME280_OVERSAMPLING_4X;
    dev->settings.osr_t = BME280_OVERSAMPLING_2X;
    dev->settings.filter = BME280_FILTER_COEFF_4;

    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    /* Set the sensor settings */
    rslt = bme280_set_sensor_settings(settings_sel, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to set sensor settings (code %+d).", rslt);

        return rslt;
    }

    /*Calculate the minimum delay required between consecutive measurement based upon the sensor enabled
     *  and the oversampling configuration. */
    req_delay = bme280_cal_meas_delay(&dev->settings);

    printf("Temperature, Pressure, Humidity\n");
    /* Continuously stream sensor data */
    while (1)
    {
        /* Set the sensor to forced mode */
        rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
        if (rslt != BME280_OK)
        {
            fprintf(stderr, "Failed to set sensor mode (code %+d).", rslt);
            break;
        }

        /* Wait for the measurement to complete and print data */
        dev->delay_us(req_delay*1000, dev->intf_ptr);
        rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
        if (rslt != BME280_OK)
        {
            fprintf(stderr, "Failed to get sensor data (code %+d).", rslt);
            break;
        }

        print_sensor_data(&comp_data);
    }

    return rslt;
}

/*!
 * @brief This API initializes the given device structure and the sensor for weather monitoring.
 */
int init_dev_weather(struct identifier *id, struct bme280_dev *dev) {
    
    /* Variable to define the result */
    int8_t rslt = BME280_OK;
    
    /* Variable to define the selecting sensors */
    uint8_t settings_sel = 0;
    
    /* Open I2C bus driver as character device */
    if ((id->fd = open(I2C_DRIVER_PATH, O_RDWR)) < 0)
    {
        fprintf(stderr, "Failed to open the i2c bus %s\n", I2C_DRIVER_PATH);
        return -1;
    }
    
    /* Initialize device structure and interface data */
    id->dev_addr = BME280_I2C_ADDR_PRIM;
    
    dev->intf = BME280_I2C_INTF;
    dev->read = user_i2c_read;
    dev->write = user_i2c_write;
    dev->delay_us = user_delay_us;
    
    /* Update interface pointer */
    dev->intf_ptr = id;
    
    /* Initialize the BME280 sensor */
    rslt = bme280_init(dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to initialize the device (code %+d).\n", rslt);
        return -1;
    }
    
    /* Recommended mode of operation: Advanced Weather Monitoring */
    dev->settings.osr_h = BME280_OVERSAMPLING_1X;
    dev->settings.osr_p = BME280_OVERSAMPLING_4X;
    dev->settings.osr_t = BME280_OVERSAMPLING_2X;
    dev->settings.filter = BME280_FILTER_COEFF_4;
    
    settings_sel = BME280_OSR_PRESS_SEL | BME280_OSR_TEMP_SEL | BME280_OSR_HUM_SEL | BME280_FILTER_SEL;

    /* Set the sensor settings */
    rslt = bme280_set_sensor_settings(settings_sel, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to set sensor settings (code %+d).", rslt);
        return -1;
    }
    
    return 0;
}

/*!
 * @brief This API returns the minimal required delay in milliseconds between consecutive measurements based on the sensor settings.
 */
uint32_t get_min_delay(struct identifier *id, struct bme280_dev *dev) {
    
    return bme280_cal_meas_delay(&dev->settings);
}

/*!
 * @brief This API returns sensor data based on the device settings.
 */
int get_sensor_data(struct identifier *id, struct bme280_dev *dev, uint32_t req_delay_ms, struct sensor_data *data) {
    
    /* Variable to define the result */
    int8_t rslt = BME280_OK;
    
    /* Structure to get the pressure, temperature and humidity values */
    struct bme280_data comp_data;
    
    /*
     * Note:
     * 
     * Initially the sensor enters into SLEEP MODE on calling the
     * init_dev_weather function (see implementation of function 
     * bme280_set_sensor_settings). To acquire new sensor data in
     * FORCED MODE we must set the sensor mode each time a measurement
     * is carried out. See the sequential chart below:
     * 
     * #1 Sensor is in SLEEP MODE
     * #2 Function sets sensor mode to FORCED MODE
     * #3 Sensor wakes up and completes the measurments
     *    based on the device settings
     * #4 Sensor returns to SLEEP MODE
     * #5 Function acquires the results of the latest measurement
     *    from the appropriate sensor data registers
     * #6 Function returns
     *  */
     
    /* Set the sensor to forced mode (to start new measurement) */
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to set sensor mode (code %+d).", rslt);
        return -1;
    }

    /* Wait for the sensor to complete measurement */
    dev->delay_us(req_delay_ms*1000, dev->intf_ptr);
    
    /* Get the latest measurement data from the sensor */
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);
    if (rslt != BME280_OK)
    {
        fprintf(stderr, "Failed to get sensor data (code %+d).", rslt);
        return -1;
    }
    
    /* Convert/format the sensor datas based on the enabled settings */
#ifdef BME280_FLOAT_ENABLE
    data->temp = comp_data.temperature;
    data->press = 0.01 * comp_data.pressure;
    data->hum = comp_data.humidity;
#else
#ifdef BME280_64BIT_ENABLE
    data->temp = 0.01f * comp_data.temperature;
    data->press = 0.0001f * comp_data.pressure;
    data->hum = 1.0f / 1024.0f * comp_data.humidity;
#else
    data->temp = 0.01f * comp_data.temperature;
    data->press = 0.01f * comp_data.pressure;
    ata->hum = 1.0f / 1024.0f * comp_data.humidity;
#endif
#endif

    return 0;
}

/*!
 * @brief This API closes communication with the sensor device.
 */
int close_dev_connection(struct identifier *id, struct bme280_dev *dev) {
    
    /* Close I2C bus file descriptor */
    if (close(id->fd) < 0)
    {
        fprintf(stderr, "Failed to close I2C bus file descriptor.\n");
        return -1;
    }
    
    return 0;
}
