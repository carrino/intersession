# Datasheets and board references

Everything on the CLUE and everything in the parts order, in one place. The I2C address
is the one the day 1 sketch looks for. Each file links back to where it came from, so a
newer revision can be fetched from the same place.

## The CLUE board

From Adafruit's [CLUE PCB repository](https://github.com/adafruit/Adafruit-CLUE-PCB) and
[learn guide](https://learn.adafruit.com/adafruit-clue/downloads), shared by Adafruit
under [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/).

| File | What it shows |
| --- | --- |
| [CLUE_schematic.png](clue/CLUE_schematic.png) | Full schematic, rev D, at full resolution: zoom in. |
| [CLUE_pinout.svg](clue/CLUE_pinout.svg), [CLUE_pinout.pdf](clue/CLUE_pinout.pdf) | Every pin with its Arduino number, nRF52840 port and what it is wired to. |
| [CLUE_fab_print.png](clue/CLUE_fab_print.png) | Board outline and dimensions. |

The processor's own documents are large, so they are links rather than copies: the
[nRF52840 product specification](https://cdn-learn.adafruit.com/assets/assets/000/068/543/original/nRF52840_PS_v1.0.pdf)
and the [Raytac MDBT50Q module](https://cdn-learn.adafruit.com/assets/assets/000/068/544/original/Raytac_MDBT50Q.pdf)
it sits in.

## Sensors on the CLUE

| Part | What it measures | I2C | File | Source |
| --- | --- | --- | --- | --- |
| LSM6DS3TR-C | Accelerometer and gyro, CLUE v2 boards (from January 2025) | 0x6A | [LSM6DS3TR-C.pdf](LSM6DS3TR-C.pdf) | [Adafruit](https://cdn-shop.adafruit.com/product-files/4503/4503_LSM6DS3TR-C_datasheet.pdf) |
| LSM6DS33 | Accelerometer and gyro, earlier boards | 0x6A | [LSM6DS33.pdf](LSM6DS33.pdf) | [Pololu](https://www.pololu.com/file/0J1087/LSM6DS33.pdf) |
| LIS3MDL | Magnetometer | 0x1C | [LIS3MDL.pdf](LIS3MDL.pdf) | [Pololu](https://www.pololu.com/file/0J1089/LIS3MDL.pdf) |
| APDS-9960 | Color, light, proximity and gesture | 0x39 | [APDS-9960.pdf](APDS-9960.pdf) | [SparkFun](https://cdn.sparkfun.com/assets/learn_tutorials/3/2/1/Avago-APDS-9960-datasheet.pdf) |
| SHT30 | Humidity and temperature (the SHT3x family) | 0x44 | [SHT3x.pdf](SHT3x.pdf) | [Sensirion](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf) |
| BMP280 | Air pressure and temperature | 0x77 | [BMP280.pdf](BMP280.pdf) | [Bosch](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf) |
| MP34DT01-M | PDM microphone | not I2C | [MP34DT01-M.pdf](MP34DT01-M.pdf) | [Adafruit](https://cdn-learn.adafruit.com/assets/assets/000/049/977/original/MP34DT01-M.pdf) |

## STEMMA QT plug-ins in the parts order

| Part | What it is | I2C | File | Source |
| --- | --- | --- | --- | --- |
| VL53L1X | Time-of-flight distance, about 3 cm to 4 m | 0x29 | [VL53L1X.pdf](VL53L1X.pdf) | [Adafruit](https://cdn-learn.adafruit.com/assets/assets/000/105/859/original/vl53l1x.pdf) |
| AMG8833 | 8x8 thermal camera (Panasonic Grid-EYE) | 0x69 | [AMG8833.pdf](AMG8833.pdf) | [Adafruit](https://cdn-learn.adafruit.com/assets/assets/000/043/261/original/Grid-EYE_SPECIFICATIONS%28Reference%29.pdf) |
| MPR121 | 12 capacitive touch inputs | 0x5A | [MPR121.pdf](MPR121.pdf) | [NXP](https://www.nxp.com/docs/en/data-sheet/MPR121.pdf) |
| DRV2605L | Haptic motor driver | 0x5A | [DRV2605L.pdf](DRV2605L.pdf) | [TI](https://www.ti.com/lit/ds/symlink/drv2605l.pdf) |
| Vibrating mini motor disc | The motor the DRV2605L drives (datasheet in Chinese) | none | [vibration_motor_disc.pdf](vibration_motor_disc.pdf) | [Adafruit](https://cdn-shop.adafruit.com/product-files/1201/P1012_datasheet.pdf) |
| ANO encoder adapter | Dial, five buttons, through Adafruit's seesaw firmware | 0x49 | [ANO_encoder.jpg](ANO_encoder.jpg) (the encoder's drawing) | [Adafruit](https://cdn-shop.adafruit.com/product-files/5001/ANO+Encoder.jpg), [seesaw library](https://github.com/adafruit/Adafruit_Seesaw) |
| PA1010D | GPS | 0x10 | [PA1010D.pdf](PA1010D.pdf) | [Adafruit](https://cdn-learn.adafruit.com/assets/assets/000/084/295/original/CD_PA1010D_Datasheet_v.03.pdf) |
| SCD-43 | CO2, temperature and humidity (the SCD4x family) | 0x62 | [SCD4x.pdf](SCD4x.pdf) | [Sensirion](https://sensirion.com/media/documents/48C4B7FB/64C134E7/Sensirion_SCD4x_Datasheet.pdf) |
| MLX90632 | Non-contact temperature | 0x3A | [MLX90632.pdf](MLX90632.pdf) | [Melexis](https://www.melexis.com/-/media/files/documents/datasheets/mlx90632-datasheet-melexis.pdf) |
| LTR-390UV | UV and ambient light | 0x53 | [LTR-390UV.pdf](LTR-390UV.pdf) | [Lite-On](https://optoelectronics.liteon.com/upload/download/DS86-2015-0004/LTR-390UV_Final_%20DS_V1%201.pdf) |

The MPR121 and DRV2605L share address 0x5A, so a project with both moves the MPR121 to
0x5B with its ADDR jumper.

The datasheets are their manufacturers' documents, copied here for reference.
