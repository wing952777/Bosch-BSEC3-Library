/**
 * Copyright (C) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * basic_config_state.ino sketch :
 * This example illustrates loading a BSEC configuration blob and periodically
 * saving / restoring the BSEC algorithm state to non-volatile memory (here
 * emulated with the ESP non-volatile preferences / a RAM buffer).
 *
 * By default it targets the BME688 (bme68x driver). To target the BME690,
 * build with the BSEC3_BME690 macro defined.
 *
 * NOTE: replace the loadState()/saveState() bodies with your own non-volatile
 * storage (EEPROM, LittleFS, Preferences, flash, ...). The skeleton below keeps
 * the state in RAM only so the example compiles on every platform.
 */

#include <bsec3.h>

/* Macros used */
// #define PANIC_LED   LED_BUILTIN
// #define ERROR_DUR   1000

#define SAMPLE_RATE         BSEC_SAMPLE_RATE_LP
#define STATE_SAVE_PERIOD   UINT32_C(360 * 60 * 1000) /* every 360 minutes */

/*
 * Paste a configuration blob generated for your sensor / voltage / sample-rate
 * here. The matching bsec_iaq.c / bsec_selectivity.c arrays are shipped under
 * src/config/bme688 and src/config/bme690. Leaving it empty uses the library's
 * built-in defaults.
 */
const uint8_t bsecConfig[BSEC_MAX_PROPERTY_BLOB_SIZE] = { 0 };

/* Buffer used to persist the algorithm state */
uint8_t bsecState[BSEC_MAX_STATE_BLOB_SIZE] = { 0 };
uint16_t stateUpdateCounter = 0;

/* Helper functions declarations */
// void errLeds(void);
void checkBsecStatus(Bsec3 bsec);
void updateBsecState(Bsec3 bsec);
bool loadState(uint8_t *state);
bool saveState(const uint8_t *state);
void newDataCallback(const bme6xxData data, const bsecOutputs outputs, Bsec3 bsec);

/* Create an object of the class Bsec3 */
Bsec3 envSensor;

void setup(void)
{
    bsecSensor sensorList[] = {
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_STATIC_IAQ,
            BSEC_OUTPUT_CO2_EQUIVALENT,
            BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
    };

    Serial.begin(115200);
    Wire.begin();
    // pinMode(PANIC_LED, OUTPUT);
    while (!Serial) delay(10);

    if (!envSensor.begin(BME6XX_I2C_ADDR_LOW, Wire))
    {
        checkBsecStatus(envSensor);
    }

    /* Load a configuration blob if one was supplied */
    if (bsecConfig[0] != 0)
    {
        if (!envSensor.setConfig(bsecConfig))
        {
            checkBsecStatus(envSensor);
        }
    }

    /* Restore the previously saved state, if any */
    if (loadState(bsecState))
    {
        if (!envSensor.setState(bsecState))
        {
            checkBsecStatus(envSensor);
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

    if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), SAMPLE_RATE))
    {
        checkBsecStatus(envSensor);
    }

    envSensor.attachCallback(newDataCallback);

    Serial.println("BSEC library version " + \
            String(envSensor.version.major) + "." \
            + String(envSensor.version.minor) + "." \
            + String(envSensor.version.major_bugfix) + "." \
            + String(envSensor.version.minor_bugfix));
}

void loop(void)
{
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
            case BSEC_OUTPUT_IAQ:
                Serial.println("IAQ = " + String(output.signal) +
                               ", accuracy = " + String((int) output.accuracy));
                /* Save the state once the IAQ accuracy reaches a stable value */
                if (output.accuracy >= 3)
                {
                    updateBsecState(envSensor);
                }
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                Serial.println("CO2 Equivalent = " + String(output.signal));
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                Serial.println("bVOC equivalent = " + String(output.signal));
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

// void errLeds(void)
// {
//     while (1)
//     {
//         digitalWrite(PANIC_LED, HIGH);
//         delay(ERROR_DUR);
//         digitalWrite(PANIC_LED, LOW);
//         delay(ERROR_DUR);
//     }
// }

void checkBsecStatus(Bsec3 bsec)
{
    if (bsec.status < BSEC_OK)
    {
        Serial.println("BSEC error code : " + String(bsec.status));
        // errLeds();
    }
    else if (bsec.status > BSEC_OK)
    {
        Serial.println("BSEC warning code : " + String(bsec.status));
    }

    if (bsec.sensor.checkStatus() < BME6XX_OK)
    {
        Serial.println("BME6xx error code : " + String(bsec.sensor.checkStatus()));
        // errLeds();
    }
    else if (bsec.sensor.checkStatus() > BME6XX_OK)
    {
        Serial.println("BME6xx warning code : " + String(bsec.sensor.checkStatus()));
    }
}
