/**
 * Copyright (c) 2024 Bosch Sensortec GmbH. All rights reserved.
 *
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file	bsec3.h
 * @date	2024
 * @version	3.3.0.0
 *
 * @brief   Arduino wrapper for the Bosch Sensortec Environmental Cluster
 *          (BSEC) v3 algorithm. Modelled on the Bosch-BSEC2-Library API.
 *
 *          Supports both the BME688 (bme68x driver) and the BME690
 *          (bme69x driver). By default the wrapper targets the BME688.
 *          Define BSEC3_BME690 (e.g. -DBSEC3_BME690) before including this
 *          header to target the BME690 instead.
 *
 *          Unlike BSEC2 which used the suffixed multi-instance API
 *          (bsec_*_m), BSEC v3 exposes the instance-based API directly,
 *          i.e. every function takes the instance pointer as its first
 *          argument but carries no `_m` suffix.
 */

#ifndef BSEC3_H_
#define BSEC3_H_

/* Includes */
#ifdef ARDUINO
#include "Arduino.h"
#include "Wire.h"
#include "SPI.h"
#endif

/*
 * Select the underlying sensor driver. The BME690 (default) uses the bme69x
 * driver / Bme69x Arduino class; the BME688 uses the bme68x driver / Bme68x
 * Arduino class. Both classes expose an identical method surface, so the
 * wrapper is written once against a set of aliases.
 *
 * The library targets the BME690 by default — no build flag is needed. To
 * target the BME688 instead, define BSEC3_BME688 as a GLOBAL build flag so it
 * reaches every translation unit, including this library's bsec3.cpp. In the
 * Arduino IDE the supported way is a `build_opt.h` file in the sketch folder
 * containing `-DBSEC3_BME688`; in PlatformIO use `build_flags = -DBSEC3_BME688`.
 * A bare `#define` in the .ino does NOT work — the library .cpp is compiled
 * separately and would not see it.
 */
#ifndef BSEC3_BME688

#include "bme69xLibrary.h"

typedef Bme69x                  Bme6xx;
typedef bme69xData              bme6xxData;
typedef bme69xIntf              bme6xxIntf;
typedef bme69x_read_fptr_t      bme6xx_read_fptr_t;
typedef bme69x_write_fptr_t     bme6xx_write_fptr_t;
typedef bme69x_delay_us_fptr_t  bme6xx_delay_us_fptr_t;

#define BME6XX_OK               BME69X_OK
#define BME6XX_SLEEP_MODE       BME69X_SLEEP_MODE
#define BME6XX_FORCED_MODE      BME69X_FORCED_MODE
#define BME6XX_PARALLEL_MODE    BME69X_PARALLEL_MODE
#define BME6XX_ERROR            BME69X_ERROR
#define BME6XX_GASM_VALID_MSK   BME69X_GASM_VALID_MSK
#define BME6XX_I2C_ADDR_LOW     BME69X_I2C_ADDR_LOW
#define BME6XX_I2C_ADDR_HIGH    BME69X_I2C_ADDR_HIGH
#define bme6xxDelayUs           bme69xDelayUs

#ifdef BME69X_USE_FPU
#define BME6XX_USE_FPU
#endif

#else /* BSEC3_BME688 explicitly requested */

#include "bme68xLibrary.h"

typedef Bme68x                  Bme6xx;
typedef bme68xData              bme6xxData;
typedef bme68xIntf              bme6xxIntf;
typedef bme68x_read_fptr_t      bme6xx_read_fptr_t;
typedef bme68x_write_fptr_t     bme6xx_write_fptr_t;
typedef bme68x_delay_us_fptr_t  bme6xx_delay_us_fptr_t;

#define BME6XX_OK               BME68X_OK
#define BME6XX_SLEEP_MODE       BME68X_SLEEP_MODE
#define BME6XX_FORCED_MODE      BME68X_FORCED_MODE
#define BME6XX_PARALLEL_MODE    BME68X_PARALLEL_MODE
#define BME6XX_ERROR            BME68X_ERROR
#define BME6XX_GASM_VALID_MSK   BME68X_GASM_VALID_MSK
#define BME6XX_I2C_ADDR_LOW     BME68X_I2C_ADDR_LOW
#define BME6XX_I2C_ADDR_HIGH    BME68X_I2C_ADDR_HIGH
#define bme6xxDelayUs           bme68xDelayUs

#ifdef BME68X_USE_FPU
#define BME6XX_USE_FPU
#endif

#endif /* BSEC3_BME690 */

/* dependent library header */
#include "inc/bsec_datatypes.h"
#include "inc/bsec_interface.h"

#ifndef ARRAY_LEN
#define ARRAY_LEN(array)				(sizeof(array)/sizeof(array[0]))
#endif

#define BSEC_CHECK_INPUT(x, shift)		    (x & (1 << (shift-1)))
#define BSEC_TOTAL_HEAT_DUR                 UINT16_C(140)
#define BSEC_INSTANCE_SIZE                  3272
#define BSEC_E_INSUFFICIENT_INSTANCE_SIZE   (bsec_library_return_t)-105

/*
 *	The default offset provided has been determined by testing the sensor in LP and ULP mode on application board 3.0
 *	Please update the offset value after testing this on your product
 */
#define TEMP_OFFSET_LP		(1.3255f)
#define TEMP_OFFSET_ULP		(0.466f)

typedef bsec_output_t bsecData;
typedef bsec_virtual_sensor_t bsecSensor;

typedef struct
{
    bsecData output[BSEC_NUMBER_OUTPUTS];
    uint8_t nOutputs;
} bsecOutputs;

class Bsec3;
typedef void (*bsecCallback)(const bme6xxData data, const bsecOutputs outputs, const Bsec3 bsec);

/* BSEC3 class definition */
class Bsec3
{
public:
    Bme6xx sensor;
    /* Stores the version of the BSEC algorithm */
    bsec_version_t version;
    bsec_library_return_t status;

    Bsec3(void);

    /**
     * @brief Function to initialize the sensor based on custom callbacks
     * @param intf     : BME6XX_SPI_INTF or BME6XX_I2C_INTF interface
     * @param read     : Read callback
     * @param write    : Write callback
     * @param idleTask : Delay or Idle function
     * @param millis   : Function to get current time in milliseconds
     * @param intfPtr  : Pointer to the interface descriptor
     * @return True if everything initialized correctly
     */
    bool begin(bme6xxIntf intf, bme6xx_read_fptr_t read, bme6xx_write_fptr_t write,
            bme6xx_delay_us_fptr_t idleTask, void *intfPtr, unsigned long (*millis)());

#ifdef ARDUINO
    /**
     * @brief Function to initialize the sensor based on custom callbacks
     */
    bool begin(bme6xxIntf intf, bme6xx_read_fptr_t read, bme6xx_write_fptr_t write,
            bme6xx_delay_us_fptr_t idleTask, void *intfPtr);

    /**
     * @brief Function to initialize the sensor based on the Wire library
     * @param i2cAddr  : The I2C address the sensor is at
     * @param i2c      : The TwoWire object
     * @param idleTask : Delay or Idle function
     * @return True if everything initialized correctly
     */
    bool begin(uint8_t i2cAddr, TwoWire &i2c, bme6xx_delay_us_fptr_t idleTask = bme6xxDelayUs);

    /**
     * @brief Function to initialize the sensor based on the SPI library
     * @param chipSelect : The chip select pin for SPI communication
     * @param spi        : The SPIClass object
     * @param idleTask   : Delay or Idle function
     * @return True if everything initialized correctly
     */
    bool begin(uint8_t chipSelect, SPIClass &spi, bme6xx_delay_us_fptr_t idleTask = bme6xxDelayUs);
#endif

    /**
     * @brief Function that sets the desired sensors and the sample rates
     * @param sensorList	: The list of output sensors
     * @param nSensors		: Number of outputs requested
     * @param sampleRate	: The sample rate of requested sensors
     * @return	true for success, false otherwise
     */
    bool updateSubscription(bsecSensor sensorList[], uint8_t nSensors, float sampleRate =
    BSEC_SAMPLE_RATE_ULP);

    /**
     * @brief Callback from the user to read data from the BME6xx using parallel/forced mode, process and store outputs
     * @return	true for success, false otherwise
     */
    bool run(void);

    void attachCallback(bsecCallback callback)
    {
        newDataCallback = callback;
    }

    /**
     * @brief Function to get the BSEC outputs
     * @return	pointer to BSEC outputs if available else nullptr
     */
    const bsecOutputs* getOutputs(void)
    {
        if (outputs.nOutputs)
            return &outputs;
        return nullptr;
    }

    /**
     * @brief Function to get the BSEC output by sensor id
     * @return	BSEC output, zeroed struct otherwise
     */
    bsecData getData(bsecSensor id)
    {
        bsecData emp;
        memset(&emp, 0, sizeof(emp));
        for (uint8_t i = 0; i < outputs.nOutputs; i++)
            if (id == outputs.output[i].sensor_id)
                return outputs.output[i];
        return emp;
    }

    /**
     * @brief Function to get the state of the algorithm to save to non-volatile memory
     * @param state			: Pointer to a memory location, to hold the state
     * @return	true for success, false otherwise
     */
    bool getState(uint8_t *state);

    /**
     * @brief Function to set the state of the algorithm from non-volatile memory
     * @param state			: Pointer to a memory location that contains the state
     * @return	true for success, false otherwise
     */
    bool setState(uint8_t *state);

    /**
     * @brief Function to retrieve the current library configuration
     * @param config    : Pointer to a memory location, to hold the serialized config blob
     * @return	true for success, false otherwise
     */
    bool getConfig(uint8_t *config);

    /**
     * @brief Function to set the configuration of the algorithm from memory
     * @param config		: Pointer to a memory location that contains the configuration
     * @return	true for success, false otherwise
     */
    bool setConfig(const uint8_t *config);

    /**
     * @brief Function to set the temperature offset
     * @param tempOffset	: Temperature offset in degree Celsius
     */
    void setTemperatureOffset(float tempOffset)
    {
        extTempOffset = tempOffset;
    }

    /**
     * @brief Function to calculate an int64_t timestamp in milliseconds
     */
    int64_t getTimeMs(void);

    /**
     * @brief Function to assign the memory block to the bsec instance
     *
     * @param[in] memBlock : reference to the memory block
     */
    void allocateMemory(uint8_t (&memBlock)[BSEC_INSTANCE_SIZE]);

    /**
     * @brief Function to de-allocate the dynamically allocated memory
     */
    void clearMemory(void);

private:
    bsec_bme_settings_t bmeConf;

    bsecCallback newDataCallback;

    bsecOutputs outputs;
    /* operating mode of sensor */
    uint8_t opMode;

    float extTempOffset;
    /** Global variables to help create a millisecond timestamp that doesn't overflow every 51 days.
     * If it overflows, it will have a negative value. Something that should never happen.
     */
    uint32_t ovfCounter;

    unsigned long (*bsecMillis)();

    uint32_t lastMillis;
    /* Pointer to hold the address of the instance */
    uint8_t *bsecInstance;

    /**
     * @brief Reads the data from the BME6xx sensor and process it
     * @param currTimeNs: Current time in ns
     * @return true if there are new outputs. false otherwise
     */
    bool processData(int64_t currTimeNs, const bme6xxData &data);

    /**
     * @brief Common code for the begin function
     */
    bool beginCommon();

    /**
     * @brief Set the BME6xx sensor configuration to forced mode
     */
    void setBme6xxConfigForced(void);

    /**
     * @brief Set the BME6xx sensor configuration to parallel mode
     */
    void setBme6xxConfigParallel(void);
};

#endif /* BSEC3_H_ */
