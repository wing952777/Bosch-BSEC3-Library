/**
 * Copyright (C) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * basic_config_state.ino sketch :
 * This example illustrates loading a BSEC configuration blob and periodically
 * saving / restoring the BSEC algorithm state through user-provided storage.
 *
 * By default it targets the BME690 with the same configuration and I2C pins
 * as bme690_iaq. To target the BME688, define BSEC3_BME688 for the whole build.
 *
 * NOTE: replace the loadState()/saveState() bodies with your own non-volatile
 * storage (EEPROM, LittleFS, Preferences, flash, ...). The skeleton below does
 * not persist state across resets.
 */

#include <bsec3.h>
#include "bsec_selectivity.h"

/* Macros used */
#define SAMPLE_RATE         BSEC_SAMPLE_RATE_LP
#define STATE_SAVE_PERIOD   UINT32_C(360 * 60 * 1000) /* every 360 minutes */
#define BME6XX_ADDR          BME6XX_I2C_ADDR_LOW

/* Match the wiring verified with bme690_iaq. Set both to -1 for board defaults.
 * No LED pin is configured, since it may share a pin with the I2C bus. */
#define I2C_SDA_PIN          (-1)
#define I2C_SCL_PIN          (-1)

/* Buffer used to persist the algorithm state */
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = { 0 };
uint16_t stateUpdateCounter = 0;

/* Helper functions declarations */
void checkBsecStatus(Bsec3 bsec);
void updateBsecState(Bsec3 bsec);
bool loadState(uint8_t *state);
bool saveState(const uint8_t *state);
void newDataCallback(const bme6xxData data, const bsecOutputs outputs, Bsec3 bsec);

/* Create an object of the class Bsec3 */
Bsec3 envSensor;
bool sensorReady = false;

void setup(void)
{
    bsecSensor sensorList[] = {
            BSEC_OUTPUT_RAW_PRESSURE,
            BSEC_OUTPUT_RAW_TEMPERATURE,
            BSEC_OUTPUT_RAW_HUMIDITY,
            BSEC_OUTPUT_RAW_GAS,
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_STATIC_IAQ,
            BSEC_OUTPUT_CO2_EQUIVALENT,
#ifdef BSEC3_BME688
            BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
#endif
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
#ifndef BSEC3_BME688
            BSEC_OUTPUT_TVOC_EQUIVALENT /* Keep last: only supported at LP. */
#endif
    };
    uint8_t nSensors = ARRAY_LEN(sensorList);
#ifndef BSEC3_BME688
    if (SAMPLE_RATE != BSEC_SAMPLE_RATE_LP)
    {
        nSensors--;
    }
#endif

    Serial.begin(115200);
#if (I2C_SDA_PIN >= 0) && (I2C_SCL_PIN >= 0)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#else
    Wire.begin();
#endif
    while (!Serial) delay(10);

    if (!envSensor.begin(BME6XX_ADDR, Wire))
    {
        checkBsecStatus(envSensor);
        return;
    }

    /* Load the matching IAQ example's configuration before restoring state.
     * A valid configuration can start with 0; do not test its first byte. */
    if (!envSensor.setConfig(bsec_config_state_selectivity))
    {
        checkBsecStatus(envSensor);
        return;
    }

    /* Restore the previously saved state, if any */
    if (loadState(bsecState))
    {
        if (!envSensor.setState(bsecState))
        {
            checkBsecStatus(envSensor);
            return;
        }
    }

    if (SAMPLE_RATE == BSEC_SAMPLE_RATE_LP)
    {
        envSensor.setTemperatureOffset(TEMP_OFFSET_LP);
    }
    else if (SAMPLE_RATE == BSEC_SAMPLE_RATE_ULP)
    {
        envSensor.setTemperatureOffset(TEMP_OFFSET_ULP);
    }

    if (!envSensor.updateSubscription(sensorList, nSensors, SAMPLE_RATE))
    {
        checkBsecStatus(envSensor);
        return;
    }

    envSensor.attachCallback(newDataCallback);
    sensorReady = true;

    Serial.println("BSEC library version " + \
            String(envSensor.version.major) + "." \
            + String(envSensor.version.minor) + "." \
            + String(envSensor.version.major_bugfix) + "." \
            + String(envSensor.version.minor_bugfix));
}

void loop(void)
{
    if (!sensorReady)
    {
        delay(1000);
        return;
    }

    if (!envSensor.run())
    {
        checkBsecStatus(envSensor);
    }
}

void newDataCallback(const bme6xxData data, const bsecOutputs outputs, Bsec3 bsec)
{
    if (!outputs.nOutputs)
    {
        return;
    }

    for (uint8_t i = 0; i < outputs.nOutputs; i++)
    {
        const bsecData output = outputs.output[i];
        switch (output.sensor_id)
        {
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                Serial.println("Temperature = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                Serial.println("Pressure = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                Serial.println("Humidity = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_GAS:
                Serial.println("Gas resistance = " + String(output.signal));
                break;
            case BSEC_OUTPUT_IAQ:
                Serial.println("IAQ = " + String(output.signal) +
                               ", accuracy = " + String((int) output.accuracy));
                /* Save the state once the IAQ accuracy reaches a stable value */
                if (output.accuracy >= 3)
                {
                    updateBsecState(envSensor);
                }
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                Serial.println("Static IAQ = " + String(output.signal));
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                Serial.println("Compensated temperature = " + String(output.signal));
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                Serial.println("Compensated humidity = " + String(output.signal));
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                Serial.println("CO2 Equivalent = " + String(output.signal));
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                Serial.println("bVOC equivalent = " + String(output.signal));
                break;
            case BSEC_OUTPUT_TVOC_EQUIVALENT:
                Serial.println("TVOC equivalent = " + String(output.signal));
                break;
            default:
                break;
        }
    }
}

void updateBsecState(Bsec3 bsec)
{
    bool update = false;

    if (!stateUpdateCounter || (bsec.getTimeMs() - stateUpdateCounter * (int64_t)STATE_SAVE_PERIOD) >= STATE_SAVE_PERIOD)
    {
        update = true;
        stateUpdateCounter++;
    }

    if (update)
    {
        if (!bsec.getState(bsecState))
        {
            checkBsecStatus(bsec);
            return;
        }
        saveState(bsecState);
        Serial.println("Saved BSEC state to non-volatile memory");
    }
}

/* ---- Replace the two functions below with real non-volatile storage ---- */

bool loadState(uint8_t *state)
{
    /* TODO: read BSEC_MAX_STATE_BLOB_SIZE bytes from your storage into `state`.
     * Return true only if a valid previously-saved state was found. */
    (void) state;
    return false;
}

bool saveState(const uint8_t *state)
{
    /* TODO: persist BSEC_MAX_STATE_BLOB_SIZE bytes from `state` to your storage. */
    (void) state;
    return true;
}

void checkBsecStatus(Bsec3 bsec)
{
    if (bsec.status < BSEC_OK)
    {
        Serial.println("BSEC error code : " + String(bsec.status));
    }
    else if (bsec.status > BSEC_OK)
    {
        Serial.println("BSEC warning code : " + String(bsec.status));
    }

    if (bsec.sensor.checkStatus() < BME6XX_OK)
    {
        Serial.println("BME6xx error code : " + String(bsec.sensor.checkStatus()) +
                       " (raw status = " + String(bsec.sensor.status) + ")");
    }
    else if (bsec.sensor.checkStatus() > BME6XX_OK)
    {
        Serial.println("BME6xx warning code : " + String(bsec.sensor.checkStatus()));
    }
}
