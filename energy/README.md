# HomA - Energy
This component shows the current power and total energy read by an IR connector (e.g. IR-Kopf by [volkszaehler.org](http://wiki.volkszaehler.org/)) in the framework's interfaces.

There are two possible ways to read and publish the energy values.
1) Using the libsml example `sml_server` and the Python script `sml_mqtt.py`.
2) A C++ program `sml2mqtt` using the lib libsml and mosquittopp.

## 1. Python script approach

### Installation
Install the required dependencies
```bash
$ git clone https://github.com/hmueller01/libsml.git
$ make
$ sudo make install
```
Create a `mqtt_config.py` file with this content:
```python
host = "localhost"
port = 1883
user = ""
pwd = ""
```
Modify `energy` sml_server device `/dev/vzir0` to your needs.

### Usage
Start the application manually 
```bash
$ ./energy
```

### Systemd
If your system supports it, you can start the application as a daemon from systemd by using the provided template.
```bash
$ sudo ln -s $HOMA_BASEDIR/misc/systemd/homa@.service /etc/systemd/system/multi-user.target.wants/homa@energy.service
$ sudo systemctl --system daemon-reload
$ sudo systemctl start homa@energy.service
```

Logs are then available in the systemd journal 
```bash
$ sudo journalctl -u homa@energy.service -n 100 -f
```

## 2. C++ approach
This is based on the work of [Tobias Lorenz](https://bitbucket.org/tobylorenz/sml2mqtt/overview).

### Installation
Install the required dependencies
```bash
$ apt-get install libmosquittopp-dev libyaml-cpp-dev
$ git clone https://github.com/hmueller01/libsml.git
$ cd libsml
$ make
$ sudo make install
$ cd ..
```
Build and install `sml2mqtt`
```bash
$ mkdir build; cd build
$ cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
$ make
$ sudo make install
```

### Usage
Start the application manually
```none
sml2mqtt [-v] [-c config.yaml] [-h host] [-p port] [-q qos] [-t topic] [-i id] [-u username] [-P password] [-d device]
```
You can eighter use the command line parameter or define some or all options in an `config.yaml`:
```yaml
# Config file of sml2mqtt
host: localhost
port: 1883
qos: 1
topic: /devices/123456-energy/controls
id: sml2mqtt
username: user
password: pass
device: /dev/vzir0
```

### Systemd
If your system supports it, you can start the application as a daemon from systemd by using the provided template.

Modify `/usr/local/lib/systemd/system/sml2mqtt.service` and `/etc/homa/homa.conf` to your needs.
```bash
$ sudo systemctl --system daemon-reload
$ sudo systemctl stop sml2mqtt.service # optional
$ sudo systemctl start sml2mqtt.service
```

Logs are then available in the systemd journal and other commands
```bash
$ sudo journalctl -u sml2mqtt.service -n 100 -f
$ sudo systemctl status sml2mqtt.service
```
