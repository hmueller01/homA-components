#!/bin/sh
# Sends Home Assistant configuration messages to discover gas meter data
# MIT License
# Copyright (c) 2024 Holger Mueller

# 2024/03/25 hmueller01	Initial revision
# 2024/04/16 hmueller01	Added command line options to delete topics and make topics retained

RETAINED="-r"
while getopts 'nrh' opt; do
	case "$opt" in
	r)
		echo "Remove all retained Home Assistant topics ..."
		mosquitto_pub -r -t homeassistant/sensor/123456-gas-meter/config -n
		mosquitto_pub -r -t homeassistant/sensor/123456-gas-energy/config -n
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
mosquitto_pub ${RETAINED} -t homeassistant/sensor/123456-gas-meter/config -s << EOF
{
	"device_class":"gas",
	"state_class":"total_increasing",
	"state_topic":"/devices/123456-gas-meter/controls/Count",
	"unit_of_measurement":"m³",
	"name":"Gas Volumen",
	"unique_id":"123456-gas-meter",
	"object_id":"123456-gas-meter",
	"device":{
		"identifiers":["123456-gas-meter"],
		"name":"Gas",
		"manufacturer":"Holger Müller",
		"model":"Reflexlichtschranke",
		"suggested_area":"Energie"
	}
}
EOF

mosquitto_pub ${RETAINED} -t homeassistant/sensor/123456-gas-energy/config -s << EOF
{
	"device_class":"energy",
	"state_class":"total_increasing",
	"state_topic":"/devices/123456-gas-meter/controls/Count",
	"value_template": "{{ (value | float * 11.317) | round(2) }}",
	"unit_of_measurement":"kWh",
	"name":"Gas Energie",
	"unique_id":"123456-gas-energy",
	"object_id":"123456-gas-energy",
	"device":{
		"identifiers":["123456-gas-meter"]
	}
}
EOF
