# Bosch-BSEC3-Library

Arduino / PlatformIO wrapper for the **Bosch Sensortec Environmental Cluster
(BSEC) v3.3.0.0** algorithm, for use with the **BME688** and **BME690** gas
sensors. It is API-compatible with the
[Bosch-BSEC2-Library](https://github.com/boschsensortec/Bosch-BSEC2-Library):
the wrapper class is simply renamed `Bsec2` → `Bsec3`.

BSEC takes the compensated temperature, pressure, humidity and gas-resistance
readings from the sensor and fuses them into higher-level virtual outputs such
as IAQ, static IAQ, CO₂ equivalent, breath-VOC equivalent, gas percentage and
compensated gas.

## What is different from BSEC2

| | BSEC2 | BSEC3 (this library) |
|---|---|---|
| Wrapper class | `Bsec2` | `Bsec3` |
| Algorithm C API | suffixed multi-instance (`bsec_init_m`, `bsec_do_steps_m`, …) | instance-based, **no `_m` suffix** (`bsec_init`, `bsec_do_steps`, …) — each still takes the instance pointer as the first argument |
| Supported sensors | BME688 | BME688 **and** BME690 |
| Instance size | 3272 bytes | 3272 bytes |

## Selecting the sensor

The library targets the **BME688 (bme68x driver)** by default. To target the
**BME690 (bme69x driver)**, define `BSEC3_BME690` for the whole build:

* **Arduino IDE** — add `-DBSEC3_BME690` via a `build_opt.h` / boards build
  flag, or `#define BSEC3_BME690` before `#include <bsec3.h>` in every
  translation unit.
* **PlatformIO** — add to `platformio.ini`:
  ```ini
  build_flags = -DBSEC3_BME690
  ```

The wrapper exposes neutral aliases (`bme6xxData`, `bme6xxIntf`, …) so example
code compiles unchanged for either sensor.

## Dependencies

* **BME68x Sensor library** (when targeting the BME688) — provides the `Bme68x`
  class.
* **BME69x Sensor library** (when targeting the BME690) — provides the `Bme69x`
  class.

Install the one matching your sensor.

## Installation

Copy this folder into your Arduino `libraries/` directory, or add it as a
PlatformIO `lib_deps` path. The precompiled algorithm (`libalgobsec.a`) is
bundled under `src/<cpu>/` and is linked automatically:

* `precompiled=true` + `ldflags=-lalgobsec` for the Arduino builder.
* `extra_script.py` selects the correct binary by CPU for PlatformIO.

Supported architectures: `esp32`, `esp32s2`, `esp32s3`, `esp32c3`, `esp8266`,
`cortex-m0plus`, `cortex-m3`, `cortex-m4` (soft + `fpv4-sp-d16-hard`).

> If you hit an `undefined reference to bsec_*` linker error, the precompiled
> binary for your CPU was not found / linked — check that your target maps to
> one of the `src/<cpu>/` folders.

## Configuration files

Tuning blobs for each sensor live under:

* `src/config/bme688/bsec_selectivity.{c,h}`
* `src/config/bme690/bsec_selectivity.{c,h}`

Additional IAQ configurations (voltage / sample-rate / calibration-time
variants) are available in the upstream BSEC release package under
`release_bin/IAQ/config/`. Pass a blob to `Bsec3::setConfig()`.

## Quick start

```cpp
#include <bsec3.h>

Bsec3 envSensor;
bsecSensor sensorList[] = { BSEC_OUTPUT_IAQ, BSEC_OUTPUT_CO2_EQUIVALENT };

void setup() {
    Serial.begin(115200);
    Wire.begin();
    envSensor.begin(BME68X_I2C_ADDR_LOW, Wire);
    envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP);
    envSensor.attachCallback(newDataCallback);
}

void loop() {
    envSensor.run();   // call as often as possible
}
```

See `examples/basic` and `examples/basic_config_state`.

## API

`Bsec3` mirrors `Bsec2`:

* `begin(...)` — I2C (`Wire`), SPI, or custom read/write callbacks.
* `updateSubscription(sensorList, n, sampleRate)`
* `run()` — drives the measurement state machine; call frequently.
* `attachCallback(cb)` / `getOutputs()` / `getData(id)`
* `getState()` / `setState()` — persist & restore algorithm state.
* `getConfig()` / `setConfig()` — serialize & load configuration.
* `setTemperatureOffset(°C)`, `allocateMemory()`, `clearMemory()`.

## License

BSD-3-Clause for the wrapper sources. The precompiled `libalgobsec.a` is
distributed under the Bosch Sensortec software license — see `LICENSE.md`.
