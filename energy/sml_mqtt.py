#!/usr/bin/env python2
# Reads SML data created by sml_server from stdin and sends it to a 
# MQTT broker used by HomA framework.
# Holger Mueller
# 2017/03/02, 2017/03/09, 2017/04/04
# 2017/04/06 added meta/unit topic and removed unit from payload of actual value

import sys
import os.path
import paho.mqtt.client as mqtt
import ssl
import mqtt_config		# defines secret host, port, user, pwd, ca_certs

# config here ...
DEBUG = False
SYSTEMID = "123456-energy"
DEVICE = "Energy"
ROOM = "Sensors"
# Reminder if HomA setup messages have been send, delete and restart to resend
INIT_FILE = "/dev/shm/homa_init."+ SYSTEMID


obis_arr = [
	{'obis': '1-0:16.7.0*255', 'scale': 1, 'unit': ' W', 'topic': 'Current Power'},
	{'obis': '1-0:1.8.0*255', 'scale': 1000, 'unit': ' kWh', 'topic': 'Total Energy'}]

def get_topic(t1 = None, t2 = None, t3 = None):
	"Create topic string."
	topic = "/devices/"+ SYSTEMID
	if t1:
		topic += "/"+ t1
	if t2:
		topic += "/"+ t2
	if t3:
		topic += "/"+ t3
	if DEBUG: print("topic string = '"+ topic+ "'")
	return topic

def scan_line(line):
	"Scan input line and if found publish MQTT topic/value. Returns True if success, otherwise False."
	for obis_dict in obis_arr:
		if line.find(obis_dict['obis']) <> -1:
			# find value between "#"
			pos_start = line.find("#") + 1
			pos_end = line.find("#", pos_start)
			value = float(line[pos_start:pos_end])
			if DEBUG: print("Found obis topic '"+ obis_dict['topic']+ "' = line["+ str(pos_start)+ ":"+ str(pos_end)+ "] = '"+ str(value)+ "'")
			# scale value as specified
			value = round(value / obis_dict['scale'], 1)
			mqttc.publish(get_topic("controls", obis_dict['topic']), str(value), retain=True)
			return True
	return False

def homa_init():
	"Publish HomA setup messages to MQTT broker."
	# check if we need to init HomA
	if os.path.isfile(INIT_FILE):
		print("HomA setup data not reloaded, to do so delete "+ INIT_FILE+ " and restart.")
		return
	print("Publishing HomA setup data ...")
	# set ROOM name
	mqttc.publish(get_topic("meta/ROOM"), ROOM, retain=True)
	# set DEVICE name
	mqttc.publish(get_topic("meta/name"), DEVICE, retain=True)
	# setup controls
	order = 1
	for obis_dict in obis_arr:
		mqttc.publish(get_topic("controls", obis_dict['topic'], "meta/type"), "text", retain=True)
		mqttc.publish(get_topic("controls", obis_dict['topic'], "meta/unit"), obis_dict['unit'], retain=True)
		mqttc.publish(get_topic("controls", obis_dict['topic'], "meta/order"), order, retain=True)
		order += 1
	# create init file
	file = open(INIT_FILE, 'w')
	file.close()
	return

# The callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, rc):
	if DEBUG: print("Connected with result code "+ str(rc))

	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	#client.subscribe("$SYS/#")
	return

# The callback for when a PUBLISH message is received from the broker.
def on_message(client, userdata, msg):
	if DEBUG: print(msg.topic+ " "+ str(msg.payload))
	return

# The callback for when a message is published to the broker.
def on_publish(client, userdata, mid):
	if DEBUG: print("message send "+ str(mid))
	return


# connect to MQTT broker
mqttc = mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.on_publish = on_publish
if mqtt_config.ca_certs != "":
	#mqttc.tls_insecure_set(True) # Do not use this "True" in production!
	mqttc.tls_set(mqtt_config.ca_certs, certfile=None, keyfile=None, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2, ciphers=None)
mqttc.username_pw_set(mqtt_config.user, password=mqtt_config.pwd)
mqttc.connect(mqtt_config.host, port=mqtt_config.port)
mqttc.loop_start()

homa_init() # setup MQTT DEVICE and control settings

# read from stdin and scan for obis data
print("Reading sml obis data from stdin forever. Press Ctrl-C to break.")
#for line in sys.stdin: # does not work with piping, because reads to eof!
while True:
	try:
#		line = sys.stdin.readline()
		line = raw_input()
		if DEBUG: print(line)
		scan_line(line)
	except (KeyboardInterrupt, SystemExit, EOFError):
		print '\nKeyboardInterrupt or EOF found! Stopping program.'
		break

# wait until all queued topics are published
print 'Flushing MQTT queue ...'
mqttc.loop_stop()
mqttc.disconnect()
