#!/usr/bin/env python2
# Reads gas meter pules from Raspberry pin and sends it to a 
# MQTT broker used by HomA framework.
# Holger Mueller
# 2018/03/12

import sys
import os.path
import time
import RPi.GPIO as GPIO
import paho.mqtt.client as mqtt
import mqtt_config		# defines secret host, port, user, pwd, ca_certs

# config here ...
DEBUG = False
SYSTEMID = "123456-gas-meter"
ROOM = "Sensors"
DEVICE = "Gas Meter"
CONTROL = "Count"
# Reminder if HomA setup messages have been send, delete and restart to resend
INIT_FILE = "/dev/shm/homa_init."+ SYSTEMID
# pinout see https://de.pinout.xyz
GPIO_PIN = 17 # BCM pin number 17 = pin 11 = WiringPi 0
RESOLUTION = 0.01 # m^3 / pulse
# config components control room name (if wanted) here
mqtt_controls_arr = [
	{'control': CONTROL, 'room': 'Home', 'unit': ' m^3'},
	{'control': 'Flow rate', 'room': '', 'unit': ' m^3/h'},
	{'control': 'Timestamp', 'room': '', 'unit': ''}]


# global variables
gas_counter = 0.0 # counter of gas amount [m^3]
lmillis = 0 # time of last pulse [ms]


def get_topic(t1 = None, t2 = None, t3 = None):
	"Create topic string."
	topic = "/devices/"+ SYSTEMID
	if t1:
		topic += "/"+ t1
	if t2:
		topic += "/"+ t2
	if t3:
		topic += "/"+ t3
	#if DEBUG: print("get_topic(): topic string = '"+ topic+ "'")
	return topic

def homa_init():
	"Publish HomA setup messages to MQTT broker."
	# check if we need to init HomA
	if os.path.isfile(INIT_FILE):
		print("HomA setup data not reloaded, to do so delete "+ INIT_FILE+ " and restart.")
		return
	print("Publishing HomA setup data ...")
	# set room name
	mqttc.publish(get_topic("meta/room"), ROOM, retain=True)
	# set device name
	mqttc.publish(get_topic("meta/name"), DEVICE, retain=True)
	# setup controls
	order = 1
	for dict in mqtt_controls_arr:
		mqttc.publish(get_topic("controls", dict['control'], "meta/type"), "text", retain=True)
		mqttc.publish(get_topic("controls", dict['control'], "meta/unit"), dict['unit'], retain=True)
		mqttc.publish(get_topic("controls", dict['control'], "meta/order"), order, retain=True)
		mqttc.publish(get_topic("controls", dict['control'], "meta/room"), dict['room'], retain=True)
		order += 1
	# create init file
	file = open(INIT_FILE, 'w')
	file.close()
	return

# The callback for when the client receives a CONNACK response from the broker.
def on_connect(client, userdata, flags, rc):
	if DEBUG: print("on_connect(): Connected with result code "+ str(rc))

	# Subscribing in on_connect() means that if we lose the connection and
	# reconnect then subscriptions will be renewed.
	#client.subscribe("$SYS/#")
	client.subscribe(get_topic("controls", CONTROL))
	return

# The callback for when a PUBLISH message is received from the broker.
def on_message(client, userdata, msg):
	global gas_counter
	if DEBUG: print("on_message(): "+ msg.topic+ " "+ str(msg.payload))
	if msg.topic == get_topic("controls", CONTROL):
		if abs(gas_counter - float(msg.payload)) >= RESOLUTION and gas_counter != 0.0:
			print("WARNING: Setting new gas_counter which differs from main() "+ str(gas_counter))
		gas_counter = float(msg.payload)
		if DEBUG: print("setting gas_counter = "+ str(gas_counter))
	return

# The callback for when a message is published to the broker.
def on_publish(client, userdata, mid):
	if DEBUG: print("on_publish(): message send "+ str(mid))
	return


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

homa_init() # setup MQTT device and control settings

# configure GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(GPIO_PIN, GPIO.IN)

# Wait for signal rise on pin. Make it interrupt driven ...
# http://raspi.tv/2013/how-to-use-interrupts-with-python-on-the-raspberry-pi-and-rpi-gpio
while True:
	try:
		# without timeout [ms] ^C does not interrupt GPIO.wait_for_edge()!
		while GPIO.wait_for_edge(GPIO_PIN, GPIO.RISING, timeout=2000) is not None:
			millis = int(round(time.time() * 1000))
			timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
			# debounce and check for HIGH
			time.sleep(0.05) # [s]
			if GPIO.input(GPIO_PIN) == GPIO.LOW:
				if DEBUG: print("ERROR: Input pin is LOW after a RISING edge!")
				break
			gas_counter += RESOLUTION
			rate = RESOLUTION / (millis - lmillis) * 1000 * 3600
			rate = round(rate * 1000) / 1000 # do limit precision 3 digits after dot
			mqttc.publish(get_topic("controls", CONTROL), gas_counter, retain=True)
			mqttc.publish(get_topic("controls", "Timestamp"), timestamp, retain=True)
			mqttc.publish(get_topic("controls", "Flow rate"), rate, retain=True)
			lmillis = millis
			if DEBUG: print("Rising edge detected. gas_counter = "+ str(gas_counter))
			time.sleep(1) # debounce timer [s]
	except (KeyboardInterrupt, SystemExit):
		print '\nKeyboardInterrupt. Stopping program.'
		GPIO.cleanup() # clean up GPIO on CTRL+C exit
		break

# wait until all queued topics are published
print 'Flushing MQTT queue ...'
mqttc.loop_stop()
mqttc.disconnect()
