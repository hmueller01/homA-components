#!/usr/bin/env python3
# -*- coding: utf-8
# min_max_saver is a universal min/max saver used by HomA framework.
# Listens to the following topics:
# /sys/<systemId>/min/<minSystemId>/<minControlId>, payload: <time>
# /sys/<systemId>/max/<maxSystemId>/<maxControlId>, payload: <time>
# /devices/<minSystemId>/controls/<minControlId>, payload: <value>
# /devices/<minSystemId>/controls/<minControlId>meta/unit, payload: <value>
# Creates the following retained topics:
# /devices/<minSystemId>/controls/<minControlId> min, payload: min value
# /devices/<minSystemId>/controls/<minControlId> min/meta/unit, payload: unit
# /devices/<minSystemId>/controls/<minControlId> max, payload: max value
# /devices/<minSystemId>/controls/<minControlId> max/meta/unit, payload: unit
#
# Holger Mueller
# 2017/10/24 initial revision
# 2017/10/28 setting/coping unit of min/max value from controlId
# 2018/03/14 Changed constants to caps, changes order of functions for easier reading
# 2019/02/15 Fixed a bug with utf-8 payload messages and Python string functions,
#            fixed bug that messages are not resubscribed after a broker restart (reboot)
# 2020/10/15 made script Python3 compatible

import sys
import time
import getopt
import paho.mqtt.client as mqtt
import ssl
import mqtt_config		# defines host, port, user, pwd, ca_certs
import ssl
import setup

# config here ...
debug = False
systemId = setup.systemId # "123456-min-max-saver"

# use utf8 encoding in all string related functions (e.g. str())
# only Python2 relevant, since the default on Python3 is UTF-8 already
if sys.version[0] == '2':
	reload(sys)
	sys.setdefaultencoding('utf8')

saver_arr = [] # buffer of registered saver, contents:
# {'saver': min/max, 'system': <systemId>, 'control': <controlId>,
#  'time': reset interval in seconds, 'nextReset': next reset time in seconds,
#  'value': min/max value}

def build_topic(systemId, t1 = None, t2 = None, t3 = None):
	"Create topic string."
	if not t1:
		print("ERROR get_topic(): t1 not specified!")
		sys.exit
	topic = "/devices/%s" % (systemId)
	if t1:
		topic += "/"+ t1
	if t2:
		topic += "/"+ t2
	if t3:
		topic += "/"+ t3
	if debug: print("build_topic(): '"+ topic+ "'")
	return topic

def getNextResetTime(timeValue):
	currentTime = time.time()
	nextReset = time.localtime(currentTime)
	nextResetTime = currentTime - ((nextReset.tm_hour * 60 + nextReset.tm_min) * 60 + nextReset.tm_sec)
	while nextResetTime < currentTime:
		nextResetTime += timeValue
	timeStr = time.strftime("%a, %d %b %Y %H:%M:%S", time.localtime(nextResetTime))
	if debug: print("getNextResetTime(): %s" % timeStr)
	return nextResetTime

def getSaver(saver, system, control):
	for saver_dict in saver_arr:
		if saver_dict['saver'] == saver and saver_dict['system'] == system and saver_dict['control'] == control:
			if debug: print("getSaver(): %s, system: %s, control: %s found." % (saver, system, control))
			return saver_dict
	if debug: print("getSaver(): %s, system: %s, control: %s NOT found." % (saver, system, control))
	return False

def addSaver(client, saver, system, control, timeStr):
	timeValue = float(timeStr) * 3600 # convert time string in hours to seconds
	saver_dict = getSaver(saver, system, control)
	if saver_dict == False:
		if debug: print("addSaver(): %s, system: %s, control: %s, time %s added." % (saver, system, control, timeValue))
		nextResetTime = getNextResetTime(timeValue)
		saver_arr.append({'saver': saver, 'system': system, 'control': control, 'time': timeValue, 'nextReset': nextResetTime, 'value': 'SNA'})
		if debug: print(saver_arr)
	else:
		if debug: print("addSaver(): %s, system: %s, control: %s, time %s updated." % (saver, system, control, timeValue))
		saver_dict['time'] = timeValue
		if debug: print(saver_arr)
	# subscribe topic "/devices/<system>/controls/<control>"
	# e.g. "/devices/123456-energy/controls/Current Power"
	client.subscribe(build_topic(system, "controls", control))
	client.subscribe(build_topic(system, "controls", control, "meta/unit"))
	return

def removeSaver(client, saver, system, control):
	# Topic and array cleanup:
	# unsubscribe("/devices/<system>/controls/<control>")
	# publish("/devices/<system>/controls/<control> <saver>", "")
	saver_dict = getSaver(saver, system, control)
	if saver_dict != False:
		if debug: print("removeSaver(): %s, system: %s, control: %s removed." % (saver, system, control))
		client.unsubscribe(build_topic(system, "controls", control))
		client.unsubscribe(build_topic(system, "controls", control, "meta/unit"))
		# remove topic
		client.publish(build_topic(system, "controls", control + " " + saver), "", retain=True)
		client.publish(build_topic(system, "controls", control + " " + saver, "meta/unit"), "", retain=True)
		saver_arr.remove(saver_dict)
	return

def updateSaver(client, system, control, value):
	currentTime = time.time()
	saver_dict = getSaver("min", system, control)
	if saver_dict != False:
		# check if max. save time is over
		if currentTime > float(saver_dict['nextReset']):
			saver_dict['nextReset'] = float(saver_dict['nextReset']) + float(saver_dict['time'])
			saver_dict['value'] = "SNA" # set SNA, to make next if true
		if saver_dict['value'] == "SNA" or float(value) < float(saver_dict['value']):
			# new value is less than last min value
			if debug: print("updateSaver(): min, system: %s, control: %s, value: %s updated." % (system, control, value))
			saver_dict['value'] = value
			client.publish(build_topic(system, "controls", control + " min"), value, retain=True)

	saver_dict = getSaver("max", system, control)
	if saver_dict != False:
		# check if max save time is over
		if currentTime > float(saver_dict['nextReset']):
			saver_dict['nextReset'] = float(saver_dict['nextReset']) + float(saver_dict['time'])
			saver_dict['value'] = "SNA" # set SNA, to make next if true
		if saver_dict['value'] == "SNA" or float(value) > float(saver_dict['value']):
			# new value is greater than last max value
			if debug: print("updateSaver(): max, system: %s, control: %s, value: %s updated." % (system, control, value))
			saver_dict['value'] = value
			client.publish(build_topic(system, "controls", control + " max"), value, retain=True)
	return

def updateSaverUnit(client, system, control, unit):
	saver_dict = getSaver("min", system, control)
	if saver_dict != False:
		if debug: print("updateSaverUnit(): min, system: %s, control: %s, unit: %s updated." % (system, control, unit))
		client.publish(build_topic(system, "controls", control + " min", "meta/unit"), unit, retain=True)

	saver_dict = getSaver("max", system, control)
	if saver_dict != False:
		if debug: print("updateSaverUnit(): max, system: %s, control: %s, unit: %s updated." % (system, control, unit))
		client.publish(build_topic(system, "controls", control + " max", "meta/unit"), unit, retain=True)
	return

# The callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, rc):
	if debug: print("on_connect(): Connected with result code "+ str(rc))
	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	# subscribe topic "/sys/<systemId>/<min/max>/<minSystemId>/<minControlId>"
	# e.g. "/sys/123456-min-max-saver/min/123456-energy/Current Power"
	client.subscribe("/sys/%s/+/+/+" % systemId)
	return

# The callback for when a PUBLISH message is received from the broker.
def on_message(client, userdata, msg):
	if debug: print("on_message(): "+ msg.topic+ ":"+ str(msg.payload))
	# subscribed topics:
	# /sys/<systemId>/<min/max>/<minSystemId>/<minControlId>, payload: time in hours
	# /devices/<minSystemId>/controls/<minControlId>
	# /devices/<minSystemId>/controls/<minControlId>/meta/unit
	# e.g. "/sys/123456-min-max-saver/min/123456-energy/Current Power", payload: 24
	# "/devices/123456-energy/controls/Current Power", payload: 123
	topic = msg.topic.split("/") # topic[0] is "" (string before first "/")
	topic.remove("") #  remove this first empty topic
	if topic[0] == "sys" and topic[1] == systemId:
		if msg.payload == "":
			removeSaver(client, topic[2], topic[3], topic[4])
		else:
			addSaver(client, topic[2], topic[3], topic[4], msg.payload)
	elif topic[0] == "devices" and topic[2] == "controls":
		if len(topic) == 6 and topic[4] == "meta":
			if topic[5] == "unit":
				updateSaverUnit(client, topic[1], topic[3], msg.payload)
		else:
			updateSaver(client, topic[1], topic[3], msg.payload)
	else:
		print("on_message(): Unkown topic '%s'." % msg.topic)
	return

# The callback for when a message is published to the broker.
def on_publish(client, userdata, mid):
	if debug: print("on_publish(): message send "+ str(mid))
	return

def usage():
	print("Universal min/max saver.")
	print("%s [-h] [--help] [-d] [--brokerHost 127.0.0.1] [--brokerPort 1883]" % sys.argv[0])
	print("-h, --help        Shows this help")
	print("-d                Enable debug output")
	print("--brokerHost      Set MQTT broker host")
	print("--brokerPort      Set MQTT broker port")
	return

def cmd_line_args(argv):
	global debug # use global debug if we set it inside here
	try:
		opts, args = getopt.getopt(argv, "hd", ["help", "brokerHost=", "brokerPort="])
	except getopt.GetoptError:
		usage()
		sys.exit(2)
	for opt, arg in opts:
		if opt in ("-h", "--help"):
			usage()
			sys.exit()
		elif opt == '-d':
			debug = True
			print("Debug output enabled.")
		elif opt in ("--brokerHost"):
			if debug: print("set mqtt_config.host = %s" % arg)
			mqtt_config.host = arg
		elif opt in ("--brokerPort"):
			if debug: print("set mqtt_config.port = %s" % arg)
			mqtt_config.port = arg
	return


# handle arguments on command line
cmd_line_args(sys.argv[1:])

# connect to MQTT broker
mqttc = mqtt.Client()
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.on_publish = on_publish
if mqtt_config.ca_certs != "":
	#mqttc.tls_insecure_set(True) # Do not use this "True" in production!
	mqttc.tls_set(mqtt_config.ca_certs, certfile=None, keyfile=None, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2, ciphers=None)
mqttc.username_pw_set(mqtt_config.user, password=mqtt_config.pwd)
if debug: print("Connecting to host '%s', port '%s'" % (mqtt_config.host, mqtt_config.port))
mqttc.connect(mqtt_config.host, port=mqtt_config.port)
mqttc.loop_start()

while True:
	# endless loop
	try:
		time.sleep(1000)
	except (KeyboardInterrupt, SystemExit):
		print('\nKeyboardInterrupt found! Stopping program.')
		break

# wait until all queued topics are published
mqttc.loop_stop()
mqttc.disconnect()
