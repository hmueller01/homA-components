# HomA - rcplugs-esp
This component is an ESP8266 project allowing the control of generic 433Mhz wireless power outlet sockets (_rcplugs_).

### Basic Requirements
* ESP8266 board, e.g. ESP12E
* Build a board based on the [schematics](schematics/garage-esp12-v001.pdf) given
* _rcplugs_ based on the supported chipsets (see [rc-switch](https://github.com/sui77/rc-switch) project).

### Prerequisites
* Install the cross compiler [esp-open-sdk](https://github.com/pfalcon/esp-open-sdk)
* Install the [ESP8266 SDK](https://github.com/espressif/ESP8266_NONOS_SDK)
* Install the [libmqtt](https://github.com/hmueller01/esp_mqtt/tree/ssl-auth)
* Install the [libwiringESP](https://github.com/hmueller01/wiringESP)

### Installation
* Modify `Makefile` and `include/user_config.h` to your needs.
* Create a `include/user_secret.h` by copying `include/user_secret.template.h` (modify as needed).
* Compile the project
```bash
$ cd $HOMA_BASEDIR/components/rcplugs-esp
$ make
```
* Start the ESP8266 board in boot mode and flash the firmware
```bash
$ make flash
```
* Once it is running it can be updated by FOTA. Increase the ```APP_VERSION``` in ```include/user_config.h``` and trigger the update by
```bash
$ make publish
```
* Configure `setup.py`, e.g. the installed _rcplugs_ (see `mqtt_arr`) and execute it
```bash
$ python setup.py
```

### Usage
* Use [HomA](https://github.com/binarybucks/homA) app to view and control _rcplugs_.
* To switch all _rcplugs_ on or off use `switch.py`.
