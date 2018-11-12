# AWS IoT on mongoose OS

## Overview
Guide for ESP32 and BME280.\
 [AWS IoT on mongoose OS](https://aws.amazon.com/blogs/apn/aws-iot-on-mongoose-os-part-1/).

## Setup

#### Hardware
Standard connections between ESP32 and BME280

| ESP32 | BME280 |
|:-------:|:--------:|
| 3V3   | 3.3V   |
| GND   | GND    |
| D18   | SDA    |
| D19   | SCL    |

#### Install mos
* [mos](https://mongoose-os.com/docs/quickstart/setup.md)

#### Edit GENERAL CONFIG  in mos.yml
* set *wifi.sta.ssid* and *wifi.sta.pass*
* change *i2c.sda_gpio* and *i2c.scl_gpio* if you're using others pins than 19 and 18 on the esp32

#### Build, flash and config
Navigate to the *.../digibygg-poc/esp32_bme280* directory
* $ mos build --arch esp32
* $ mos flash
* $ mos config-set device.id="DEVICE_ID"
* $ mos config-set location.name="LOCATION" location.floor="FLOOR"
* $ mos aws-iot-setup --aws-region REGION --aws-iot-policy mos-default


## Changing device data

#### Changing wifi
* Serial Connection: $ mos wifi YOUR_WIFI_SSID YOUR_WIFI_PASSWORD

#### Changing device location
* Serial Connection: $ mos config-set location.name="LOCATION" location.floor="FLOOR"