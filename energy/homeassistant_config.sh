#!/bin/sh
# Sends Home Assistant configuration messages to discover energy data
# MIT License
# Copyright (c) 2024 Holger Mueller

# 2024/03/25 hmueller01	Initial revision
# 2024/04/16 hmueller01	Added command line options to delete topics and make topics retained


RETAINED="-r"
while getopts 'nrh' opt; do
	case "$opt" in
	r)
		echo "Remove all retained Home Assistant topics ..."
		mosquitto_pub -r -t homeassistant/sensor/123456-energy-power/config -n
		mosquitto_pub -r -t homeassistant/sensor/123456-energy-energy/config -n
		exit 0
		;;

	n)
		RETAINED=""
		;;

	?|h)
		echo "Usage: $(basename $0) [-n] [-r]"
		echo "  -n Do not send topics with retained flag"
		echo "  -r Remove all retained Home Assistant topics"
		exit 1
		;;
	esac
done

echo "Sending Home Assistant auto config topics ..."
[ "${RETAINED}" == "-r" ] && echo "Using retained flag."

# <discovery_prefix>/<component>/[<node_id>/]<object_id>/config
mosquitto_pub ${RETAINED} -t homeassistant/sensor/123456-energy-power/config -s << EOF
{
	"device_class":"power",
	"state_topic":"/devices/123456-energy/controls/Current Power",
	"unit_of_measurement":"W",
	"name":"Strom Leistung",
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

mosquitto_pub ${RETAINED} -t homeassistant/sensor/123456-energy-energy/config -s << EOF
{
	"device_class":"energy",
	"state_class":"total_increasing",
	"state_topic":"/devices/123456-energy/controls/Total Energy",
	"unit_of_measurement":"kWh",
	"name":"Strom Energie",
	"unique_id":"123456-energy-energy",
	"object_id":"123456-energy-energy",
	"device":{
		"identifiers":["123456-energy"]
	}
}
EOF
