# HomA - Gas Meter
This component shows the counter in m^3 of a gas meter. This is done by counting pulses of a given resolution. The start counter value needs to be set manually.


### Installation
Install a hall sensor, reed contact or retro-reflective sensor on the gas meter as works best for you.
The use of a Schmitt-Trigger is recommended. Connect the pulse sensor to a GPIO of the Raspberry Pi.

Create a ```mqtt_config.py``` file with this content:
```none
host = "localhost"
port = 1883
user = ""
pwd = ""
```
Modify ```gas_meter.py``` settings to your needs.

### Usage
Start the application manually 
```none
$ ./gas_meter.py
```

### Systemd
If your system supports it, you can start the application as a daemon from systemd by using the provided template.
```none
$ sudo ln -s $HOMA_BASEDIR/misc/systemd/homa@.service /etc/systemd/system/multi-user.target.wants/homa@gas_meter.service
$ sudo systemctl --system daemon-reload
$ sudo systemctl start homa@gas_meter.service
```

Logs are then availble in the systemd journal 
```
$ sudo journalctl -u homa@gas_meter.service -n 100 -f
```
