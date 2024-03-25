#!/bin/sh
# Sends Home Assistant configuration messages to discover energy data
# MIT License
# Copyright (c) 2024 Holger Mueller

# 2024/03/25 hmueller01	Initial revision

# <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
mosquitto_pub -t homeassistant/sensor/123456-energy-power/config -s << EOF
{
	"device_class":"power",
	"state_topic":"/devices/123456-energy/controls/Current Power",
	"unit_of_measurement":"W",
	"name":"Leistung",
	"unique_id":"123456-energy-power",
	"object_id":"123456-energy-power",
	"device":{
		"identifiers":["123456-energy"],
		"name":"Energie",
		"manufacturer":"volkszaehler.org",
		"model":"IR Kopf",
		"suggested_area":"Energie"
	}
 }
EOF

mosquitto_pub -t homeassistant/sensor/123456-energy-energy/config -s << EOF
{
	"device_class":"energy",
	"state_topic":"/devices/123456-energy/controls/Total Energy",
	"unit_of_measurement":"kWh",
	"name":"Energie",
	"unique_id":"123456-energy-energy",
	"object_id":"123456-energy-energy",
	"device":{
		"identifiers":["123456-energy"]
	}
}
EOF
