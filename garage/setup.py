#!/usr/bin/env python2
# -*- coding: utf-8
# Setup garage is a MQTT node used by HomA framework.

# Holger Mueller
# 2018/05/22 initial revision
# 2018/09/09 setting cistern_time

import sys
import getopt
import paho.mqtt.client as mqtt
import ssl
import mqtt_config		# defines host, port, user, pwd, ca_certs

# config here ...
debug = False
systemId = "123456-garage"
#room = "Garage"
cistern_time = "60"

def get_topic(t1 = None, t2 = None, t3 = None):
	"Create topic string."
	topic = "/devices/%s" % (systemId)
	if t1:
		topic += "/"+ t1
	if t2:
		topic += "/"+ t2
	if t3:
		topic += "/"+ t3
	#if debug: print("get_topic(): '"+ topic+ "'")
	return topic

def homa_init(mqttc):
	"Publish HomA setup messages to MQTT broker."
	print("Publishing HomA setup data to %s (systemId %s) ..." % (mqtt_config.host, systemId))
	mqttc.publish("/sys/%s/cistern_time" % (systemId), cistern_time, retain=True)
	return

def homa_remove(mqttc):
	"Remove HomA messages from MQTT broker."
	print("Removing HomA data (systemId %s) ..." % systemId)
	mqttc.publish("/sys/%s/cistern_time" % (systemId), "", retain=True)
	mqttc.publish(get_topic("meta/room"), "", retain=True)
	mqttc.publish(get_topic("meta/name"), "", retain=True)
	# setup controls
	for control in ["Garage door", "Cistern", "Cistern level", "Reset reason", "Device id", "Version", "Start time"]:
		mqttc.publish(get_topic("controls", control, "meta/type"), "", retain=True)
		mqttc.publish(get_topic("controls", control, "meta/unit"), "", retain=True)
		mqttc.publish(get_topic("controls", control, "meta/room"), "", retain=True)
		mqttc.publish(get_topic("controls", control, "meta/order"), "", retain=True)
		mqttc.publish(get_topic("controls", control), "", retain=True)
	return

# The callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, rc):
	if debug: print("on_connect(): Connected with result code "+ str(rc))
	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	return

# The callback for when a PUBLISH message is received from the broker.
def on_message(client, userdata, msg):
	if debug: print("on_message(): "+ msg.topic+ ":"+ str(msg.payload))
	return

# The callback for when a message is published to the broker.
def on_publish(client, userdata, mid):
	if debug: print("on_publish(): message send "+ str(mid))
	return

def usage():
	print("Setup MQTT garage node used by HomA framework.")
	print("%s [-h] [--help] [-d] [-r]" % sys.argv[0])
	print("-h, --help        Shows this help")
	print("-d                Enable debug output")
	print("-r                Remove HomA messages")
	return

def main():
	global debug
	remove = False

	# parse command line options
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hdr", ["help"])
	except getopt.GetoptError:
		print("Error. Invalid argument.")
		usage()
		sys.exit(2)
	for opt, arg in opts:
		if opt in ("-h", "--help"):
			usage()
			sys.exit()
		elif opt == '-d':
			debug = True
			print("Debug output enabled.")
		elif opt in ("-r"):
			remove = True
			if debug: print("Remove switch found.")

	# connect to MQTT broker
	mqttc = mqtt.Client()
	mqttc.on_connect = on_connect
	mqttc.on_message = on_message
	mqttc.on_publish = on_publish
	if mqtt_config.ca_certs != "":
		mqttc.tls_set(mqtt_config.ca_certs, certfile=None, keyfile=None, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1, ciphers=None)
	mqttc.username_pw_set(mqtt_config.user, password=mqtt_config.pwd)
	mqttc.connect(mqtt_config.host, port=mqtt_config.port)
	mqttc.loop_start()

	if remove:
		homa_remove(mqttc)      # remove HomA MQTT device and control settings
	else:
		homa_init(mqttc)        # setup HomA MQTT device and control settings

	# wait until all queued topics are published
	mqttc.loop_stop()
	mqttc.disconnect()
	return

if __name__ == "__main__":
	main()
