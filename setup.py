#!/usr/bin/env python
# -*- coding: utf-8
# Setup room names for all componentes in my configuration.

# Holger Mueller
# 2017/10/16 initial revision
# 2017/03/12 added gas-meter
# 2020/10/15 made script Python3 compatible

import paho.mqtt.client as mqtt
import ssl
import mqtt_config		# defines host, port, user, pwd, ca_certs

# config here ...
debug = False
# config components device name and room name here
mqtt_devices_arr = [
	{'device': '123456-energy', 'room': 'Sensors', 'name': 'Energie'},
	{'device': '123456-solar', 'room': 'Sensors', 'name': 'Solar'},
	{'device': '123456-vito', 'room': 'Sensors', 'name': 'Heizung'},
	{'device': '123456-windsensor', 'room': 'Sensors', 'name': 'Windsensor'},
	{'device': '123456-gas-meter', 'room': 'Sensors', 'name': 'Gas Meter'}]
#	{'device': '123456-rcplugs-11111-10000', 'room': 'Plugs', 'name': 'rcPlug A'},
#	{'device': '123456-rcplugs-11111-01000', 'room': 'Plugs', 'name': 'rcPlug B'},
#	{'device': '123456-rcplugs-11111-00100', 'room': 'Plugs', 'name': 'rcPlug C'}]

# config components control room name (if wanted) here
mqtt_controls_arr = [
	{'device': '123456-energy', 'control': 'Current Power', 'room': 'Home'},
	{'device': '123456-solar', 'control': 'Kollektortemperatur', 'room': 'Home'},
	{'device': '123456-vito', 'control': 'Aussentemperatur', 'room': 'Home'},
	{'device': '123456-windsensor', 'control': 'Wind speed', 'room': 'Home'},
	{'device': '123456-gas-meter', 'control': 'Count', 'room': 'Home'}]
#	{'device': '123456-rcplugs-11111-10000', 'control': 'Power 1', 'room': 'Home'},
#	{'device': '123456-rcplugs-11111-01000', 'control': 'Power 2', 'room': 'Home'},
#	{'device': '123456-rcplugs-11111-00100', 'control': 'Power 3', 'room': 'Home'}]


def get_topic(device, t1 = None, t2 = None, t3 = None):
	"Create topic string."
	topic = "/devices/%s" % (device)
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
	print("Publishing HomA setup data to "+mqtt_config.host+" ...")
	for mqtt_dict in mqtt_devices_arr:
		mqttc.publish(get_topic(mqtt_dict['device'], "meta/room"), mqtt_dict['room'], retain=True)
		mqttc.publish(get_topic(mqtt_dict['device'], "meta/name"), mqtt_dict['name'], retain=True)
	for dict in mqtt_controls_arr:
		mqttc.publish(get_topic(dict['device'], "controls", dict['control'], "meta/room"), dict['room'], retain=True)
		#mqttc.publish(get_topic(mqtt_dict['device'], "controls/Power/meta/room"), "switch", retain=True)
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

def main():
	# connect to MQTT broker
	mqttc = mqtt.Client()
	mqttc.on_connect = on_connect
	mqttc.on_message = on_message
	mqttc.on_publish = on_publish
	if mqtt_config.ca_certs != "":
		# avoid "CertificateError: hostname" error by monkey patching ssl
		#ssl.match_hostname = lambda cert, hostname: True
		#mqttc.tls_insecure_set(True) # Do not use this "True" in production!
		mqttc.tls_set(mqtt_config.ca_certs, certfile=None, keyfile=None, cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2, ciphers=None)
	mqttc.username_pw_set(mqtt_config.user, password=mqtt_config.pwd)
	mqttc.connect(mqtt_config.host, port=mqtt_config.port)
	mqttc.loop_start()

	homa_init(mqttc)        # setup HomA MQTT device and control settings

	# wait until all queued topics are published
	mqttc.loop_stop()
	mqttc.disconnect()
	return

if __name__ == "__main__":
	main()
	exec(open("min_max_saver/setup.py").read())
	exec(open("rcplugs-esp/setup.py").read())
