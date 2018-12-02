#!/usr/bin/env python2
# -*- coding: utf-8
# Switch rcPlugs on/off

# Holger Mueller
# 2018/12/02 initial revision

import sys
import getopt
import paho.mqtt.client as mqtt
import ssl
import mqtt_config		# defines host, port, user, pwd, ca_certs

# config here ...
debug = False
systemId = "123456-rcplugs"
# config plugs here
# topic systemId is build from <systemId>-<systemCode>-<unitCode>
mqtt_arr = [
	{'topic': '11111-10000', 'control': 'Power A'},
	{'topic': '11111-01000', 'control': 'Power B'},
	{'topic': '11111-00100', 'control': 'Power C'},
	{'topic': '11111-00010', 'control': 'Power D'},
	{'topic': '11111-00001', 'control': 'Power E'}]


def get_topic(systemUnitCode, t1 = None, t2 = None, t3 = None):
	"Create topic string."
	topic = "/devices/%s-%s" % (systemId, systemUnitCode)
	if t1:
		topic += "/"+ t1
	if t2:
		topic += "/"+ t2
	if t3:
		topic += "/"+ t3
	#if debug: print("get_topic(): '"+ topic+ "'")
	return topic

def switch(mqttc, state):
	"Switch rcPlugs on/off."
	print("Switch all rcPlugs %s @ %s (systemId %s) ..." % (state, mqtt_config.host, systemId))
	for mqtt_dict in mqtt_arr:
		topic = get_topic(mqtt_dict['topic'], "controls", mqtt_dict['control'], "on")
		if debug: print(topic)
		mqttc.publish(topic, state, qos=1)
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
	print("Switch all rcPlugs on/off.")
	print("%s [-h] [--help] [-d] [-r]" % sys.argv[0])
	print("-h, --help        Shows this help")
	print("--on              Switch rcPlugs on")
	print("--off             Switch rcPlugs off")
	return

def main():
	global debug
	state = None

	# parse command line options
	try:
		opts, args = getopt.getopt(sys.argv[1:], "hd", ["help", "on", "off"])
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
		elif opt in ("--on"):
			state = "1"
			if debug: print("Switch on.")
		elif opt in ("--off"):
			state = "0"
			if debug: print("Switch off.")

	# connect to MQTT broker
	mqttc = mqtt.Client()
	mqttc.on_connect = on_connect
	mqttc.on_message = on_message
	mqttc.on_publish = on_publish
	if mqtt_config.ca_certs != "":
		#mqttc.tls_insecure_set(True) # Do not use this "True" in production!
		mqttc.tls_set(mqtt_config.ca_certs, certfile=None, keyfile=None, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1, ciphers=None)
	mqttc.username_pw_set(mqtt_config.user, password=mqtt_config.pwd)
	mqttc.connect(mqtt_config.host, port=mqtt_config.port)
	mqttc.loop_start()

	if state != None:
		switch(mqttc, state)

	# wait until all queued topics are published
	mqttc.loop_stop()
	mqttc.disconnect()
	return

if __name__ == "__main__":
	main()
