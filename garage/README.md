# HomA - Garage
This component is an ESP8266 project of a garage unit (e.g. to check garage door status, switch the pump of the cistern, and so on).

### Basic Requirements
* ESP8266 board, e.g. ESP12E
* Build a board based on the [schematics](schematics/garage-esp12-v003.pdf) given

### Prerequisites
* Install the cross compiler [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)
* Install the [ESP8266 SDK](https://github.com/espressif/ESP8266_NONOS_SDK)
* Install the [libmqtt](https://github.com/hmueller01/esp_mqtt/tree/ssl-auth)
* Install the [libwiringESP](https://github.com/hmueller01/wiringESP)

### Installation
* Copy `include/user_config.template.h` to `include/user_config.h`.
* Modify `include/user_config.h` as needed. Take special care for these settings:
```c
// WiFi settings
#define WPS // define to enable WPS
#define STA_SSID	"" // if WPS is not used
#define STA_PASS	"" // if WPS is not used
// configure FOTA IP or domain name
#define OTA_HOST	"UPDATE.YOURDOMAIN"
// MQTT broker host settings
#define MQTT_HOST	"MQTT.YOURDOMAIN" // MQTT IP or domain name
#define MQTT_USER	"" // MQTT username, set NULL if anonymous is allowed
#define MQTT_PASS	"" // MQTT password, set NULL if anonymous is allowed
```
* Edit `setup.py` if needed and execute it.
* Create `esp_ca_cert.bin`: Copy your `ca.crt` to `ESP8266_NONOS_SDK/tools` directory and execute `makefile.sh`. Copy the generated `bin/esp_ca_cert.bin` to the rhts bin folder.
See also [ESP8266 Non-OS SDK SSL User Manual](https://www.espressif.com/sites/default/files/documentation/5a-esp8266_sdk_ssl_user_manual_en.pdf) in the Espressif documentation.

* Compile the project
```bash
$ cd $HOMA_BASEDIR/components/garage
$ make ESP_ROOT=/opt/Espressif
```
Start the ESP8266 board in boot mode and flash the firmware
```bash
$ make ESP_ROOT=/opt/Espressif flash
```
Once it is running it can be updated by FOTA. Increase the `APP_VERSION` in `include/user_config.h` and trigger the update by
```bash
$ make ESP_ROOT=/opt/Espressif publish
```

### Usage
* Use [HomA](https://github.com/binarybucks/homA) app to view and control garage unit.
* To set max. cistern pump time in minutes use
```bash
$ mosquitto_pub -r -t "/sys/123456-garage/cistern_time" -m "60"
```
To disable max. shut off time set `cistern_time` = 0 (default).
