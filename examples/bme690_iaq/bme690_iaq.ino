/**
 * Copyright (C) 2024 Bosch Sensortec GmbH
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * bme690_iaq.ino
 * --------------
 * BSEC v3 IAQ example for the BME690 sensor, modelled on the official
 * bsec_v3-3-0-0 BME690 example. It uses the BME690 selectivity configuration
 * (bsec_config_selectivity, in bsec_selectivity.c/.h alongside this sketch)
 * and subscribes to the full IAQ-mode output set, including bVOC.
 *
 * This library targets the BME690 by default, so no build flag is required.
 * The BME69x sensor library must be installed.
 *
 * The selectivity config shipped here is bme690_sel_33v_300s_28d
 * (3.3 V supply, ULP / 300 s, 28-day calibration). Replace it with the variant
 * matching your supply voltage / sample rate from
 * release_bin/Sel_IAQ/config/bme690/ if needed.
 */

#include <bsec3.h>
#include "bsec_selectivity.h"

#define PANIC_LED       LED_BUILTIN
#define ERROR_DUR       1000
/* LP = Low Power, one sample every ~3 s (responsive, good for testing).
 * For ULP (one sample every ~300 s, lowest power) set BSEC_SAMPLE_RATE_ULP
 * AND replace bsec_selectivity.c with the matching *_300s_* config variant. */
#define SAMPLE_RATE     BSEC_SAMPLE_RATE_LP

/* I2C address: BME6XX_I2C_ADDR_LOW = 0x76 (SDO low), _HIGH = 0x77 (SDO high) */
#define BME6XX_ADDR     BME6XX_I2C_ADDR_LOW

/* If your board's I2C pins are not the default, set them here (e.g. ESP32-C3
 * often uses SDA=8, SCL=9). Leave as -1 to use the board default Wire.begin(). */
#define I2C_SDA_PIN     (-1)
#define I2C_SCL_PIN     (-1)

void errLeds(void);
void checkBsecStatus(Bsec3 bsec);
void newDataCallback(const bme6xxData data, const bsecOutputs outputs, Bsec3 bsec);

Bsec3 envSensor;

void setup(void)
{
    /* IAQ-mode subscription list — matches the official BME690 example.
     * NOTE: the BME690 IAQ solution does NOT provide BREATH_VOC_EQUIVALENT or
     * COMPENSATED_GAS (those are BME688-only). TVOC_EQUIVALENT is available
     * only at the LP sample rate. Subscribing to an unsupported output makes
     * updateSubscription fail with -35. */
    bsecSensor sensorList[] = {
            BSEC_OUTPUT_RAW_PRESSURE,
            BSEC_OUTPUT_RAW_TEMPERATURE,
            BSEC_OUTPUT_RAW_HUMIDITY,
            BSEC_OUTPUT_RAW_GAS,
            BSEC_OUTPUT_IAQ,
            BSEC_OUTPUT_STATIC_IAQ,
            BSEC_OUTPUT_CO2_EQUIVALENT,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
            BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
            BSEC_OUTPUT_STABILIZATION_STATUS,
            BSEC_OUTPUT_RUN_IN_STATUS,
            BSEC_OUTPUT_GAS_PERCENTAGE,
            BSEC_OUTPUT_TVOC_EQUIVALENT   /* BME690-only, available at LP rate */
    };

    Serial.begin(115200);
#if (I2C_SDA_PIN >= 0)
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
#else
    Wire.begin();
#endif
    pinMode(PANIC_LED, OUTPUT);
    while (!Serial) delay(10);

    if (!envSensor.begin(BME6XX_ADDR, Wire))
        checkBsecStatus(envSensor);

    /* Load the BME690 selectivity configuration */
    if (!envSensor.setConfig(bsec_config_selectivity))
        checkBsecStatus(envSensor);

    if (SAMPLE_RATE == BSEC_SAMPLE_RATE_ULP)
        envSensor.setTemperatureOffset(TEMP_OFFSET_ULP);
    else if (SAMPLE_RATE == BSEC_SAMPLE_RATE_LP)
        envSensor.setTemperatureOffset(TEMP_OFFSET_LP);

    if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), SAMPLE_RATE))
        checkBsecStatus(envSensor);

    envSensor.attachCallback(newDataCallback);

    Serial.println("BSEC library version " +
            String(envSensor.version.major) + "." +
            String(envSensor.version.minor) + "." +
            String(envSensor.version.major_bugfix) + "." +
            String(envSensor.version.minor_bugfix));
}

void loop(void)
{
    if (!envSensor.run())
        checkBsecStatus(envSensor);
}

void newDataCallback(const bme6xxData data, const bsecOutputs outputs, Bsec3 bsec)
{
    if (!outputs.nOutputs)
        return;

    Serial.println("BSEC outputs:\n\tTime stamp = " +
            String((int)(outputs.output[0].time_stamp / INT64_C(1000000))));
    for (uint8_t i = 0; i < outputs.nOutputs; i++)
    {
        const bsecData output = outputs.output[i];
        switch (output.sensor_id)
        {
            case BSEC_OUTPUT_IAQ:
                Serial.println("\tIAQ = " + String(output.signal));
                Serial.println("\tIAQ accuracy = " + String((int)output.accuracy));
                break;
            case BSEC_OUTPUT_STATIC_IAQ:
                Serial.println("\tStatic IAQ = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_TEMPERATURE:
                Serial.println("\tTemperature = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_PRESSURE:
                Serial.println("\tPressure = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_HUMIDITY:
                Serial.println("\tHumidity = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RAW_GAS:
                Serial.println("\tGas resistance = " + String(output.signal));
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                Serial.println("\tCompensated temperature = " + String(output.signal));
                break;
            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                Serial.println("\tCompensated humidity = " + String(output.signal));
                break;
            case BSEC_OUTPUT_CO2_EQUIVALENT:
                Serial.println("\tCO2 Equivalent = " + String(output.signal));
                break;
            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                Serial.println("\tbVOC equivalent = " + String(output.signal));
                break;
            case BSEC_OUTPUT_GAS_PERCENTAGE:
                Serial.println("\tGas percentage = " + String(output.signal));
                break;
            case BSEC_OUTPUT_TVOC_EQUIVALENT:
                Serial.println("\tTVOC equivalent = " + String(output.signal));
                break;
            case BSEC_OUTPUT_COMPENSATED_GAS:
                Serial.println("\tCompensated gas = " + String(output.signal));
                break;
            case BSEC_OUTPUT_STABILIZATION_STATUS:
                Serial.println("\tStabilization status = " + String(output.signal));
                break;
            case BSEC_OUTPUT_RUN_IN_STATUS:
                Serial.println("\tRun in status = " + String(output.signal));
                break;
            default:
                break;
        }
    }
}

void errLeds(void)
{
    while (1)
    {
        digitalWrite(PANIC_LED, HIGH);
        delay(ERROR_DUR);
        digitalWrite(PANIC_LED, LOW);
        delay(ERROR_DUR);
    }
}

void checkBsecStatus(Bsec3 bsec)
{
    if (bsec.status < BSEC_OK)
    {
        Serial.println("BSEC error code : " + String(bsec.status));
        errLeds();
    }
    else if (bsec.status > BSEC_OK)
    {
        Serial.println("BSEC warning code : " + String(bsec.status));
    }

    if (bsec.sensor.checkStatus() < BME6XX_OK)
    {
        Serial.println("BME6xx error code : " + String(bsec.sensor.checkStatus()) +
                       " (raw status = " + String(bsec.sensor.status) + ")");
        errLeds();
    }
    else if (bsec.sensor.checkStatus() > BME6XX_OK)
    {
        Serial.println("BME6xx warning code : " + String(bsec.sensor.checkStatus()));
    }
}
