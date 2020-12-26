# HomA - rcplugs-esp
This component is an ESP8266 project allowing the control of generic 433Mhz wireless power outlet sockets (__rcplugs__).

### Basic Requirements
* ESP8266 board, e.g. ESP12E
* Build a board based on the [schematics](schematics/garage-esp12-v001.pdf) given
* __rcplugs__ based on the supported chipsets (see [rc-switch](https://github.com/sui77/rc-switch) project).

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

* Compile the project
```bash
$ cd $HOMA_BASEDIR/components/rcplugs-esp
$ make ESP_ROOT=/opt/Espressif
```
* Start the ESP8266 board in boot mode and flash the firmware
```bash
$ make ESP_ROOT=/opt/Espressif flash
```
* Once it is running it can be updated by FOTA. Increase the ```APP_VERSION``` in ```include/user_config.h``` and trigger the update by
```bash
$ make ESP_ROOT=/opt/Espressif publish
```
* Configure `setup.py`, e.g. the installed __rcplugs__ (see `mqtt_arr`) and execute it
```bash
$ python setup.py
```

### Usage
* Use [HomA](https://github.com/binarybucks/homA) app to view and control __rcplugs__.
* To switch all __rcplugs__ on or off use `switch.py`.
