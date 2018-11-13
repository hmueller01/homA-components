#!/usr/bin/env node
/**
 * Firebase Cloud Messaging (FCM) can be used to send messages to clients on iOS, Android and Web.
 *
 * This script subscribes to HomA MQTT messages and uses FCM to send messages to clients
 * that are subscribed to the FCM_TOPIC.
 */
var debug = false;
const https = require('https');
var fs = require('fs');
var {google} = require('googleapis');
var homa = require('homa');
const key = require('./service-account.json');
const options = require('./fcm-options.json');
const HOST = 'fcm.googleapis.com';
const PATH = '/v1/projects/' + key.project_id + '/messages:send';
const MESSAGING_SCOPE = 'https://www.googleapis.com/auth/firebase.messaging';
const SCOPES = [MESSAGING_SCOPE];
const FCM_TOPIC = options.topic;

// for homa.params.brokerPort use 
//   1. command line parameter, 2. options.brokerPort, 3. env HOMA_BROKER_PORT, 4. 1883
homa.params = homa.params.default("brokerPort", options.brokerPort == 0 ? process.env.HOMA_BROKER_PORT ? process.env.HOMA_BROKER_PORT : 1883 : options.brokerPort);
// for homa.params.brokerHost use 
//   1. command line parameter, 2. options.brokerHost, 3. env HOMA_BROKER_HOST
// if all three are empty demand brokerHost as required
homa.params = homa.params.default("brokerHost", options.brokerHost == "" ? process.env.HOMA_BROKER_HOST : options.brokerHost);
if (homa.params.brokerHost == "")
	homa.params = homa.params.demand("brokerHost");
var systemId = homa.paramsWithDefaultSystemId(options.systemId); // Load command-line parameters and set the default SystemId

/**
 * Get a valid OAuth access token.
 */
function getAccessToken() {
	return new Promise(function(resolve, reject) {
		var jwtClient = new google.auth.JWT(
			key.client_email,
			null,
			key.private_key,
			SCOPES,
			null);
		jwtClient.authorize(function(err, tokens) {
			if (err) {
				reject(err);
				return;
			}
			resolve(tokens.access_token);
		});
	});
}

/**
 * Send HTTP request to FCM with given message.
 *
 * @param {JSON} fcmMessage will make up the body of the request.
 */
function sendFcmMessage(fcmMessage) {
	getAccessToken().then(function(accessToken) {
		var options = {
			hostname: HOST,
			path: PATH,
			method: 'POST',
			headers: {
				'Authorization': 'Bearer ' + accessToken
			}
		};
		if (debug) console.log("accessToken: %s", accessToken);
		var request = https.request(options, function(resp) {
			resp.setEncoding('utf8');
			resp.on('data', function(data) {
				var response = JSON.parse(data);
				console.log("Message sent to FCM, response: %s", response.name);
			});
		});

		request.on('error', function(err) {
			console.log('Unable to send message to Firebase');
			console.log(err);
		});

		request.write(JSON.stringify(fcmMessage));
		request.end();
	});
}

/**
 * Construct a JSON object that will be used to define the
 * common parts of a notification message that will be sent
 * to any app instance subscribed as well as to Android devices.
 *
 * @param topic to send the message to
 * @param title to send
 * @param body to send
 * @param color of message (null to use default)
 * @param tag notification id to use (null to use default)
 */
function buildMessage(topic, title, body, color, tag) {
	var fcmMessage = {
		'message': {
			'topic': topic,
//			'data': { 'string': 'string' },
			'notification': {
				'title': title,
				'body': body
			}
		}
	};
	var androidConfig = {
		'notification': {
//			'click_action': 'android.intent.action.MAIN',
//			'icon': '',
		}
	};
	if (color != null) {
		androidConfig['notification']['color'] = color;
	}
	if (tag != null) {
		androidConfig['notification']['tag'] = tag;
	}
	fcmMessage['message']['android'] = androidConfig;
	//fcmMessage['message']['apns'] = apnsConfig;
	if (debug) console.log("fcmMessage:\n%s" , JSON.stringify(fcmMessage, null, 2));
	return fcmMessage;
}


// Connects to the broker when the application is started
(function connect() {
	homa.mqttHelper.connect(null, null, options.mqtt_connect);
})();

// Called after the connection to the broker is established
homa.mqttHelper.on('connect', function(packet) {
	// When a user interacts with a component's control through an interface, the value is published to the '/on' subtopic
	// The component has to echo the value back to the topic without the '/on' so the change is reflected in all other interfaces
	homa.mqttHelper.subscribe("/devices/"+ systemId + "/controls/Cistern/on"); // Subscribe to changes of the cistern switch
	homa.mqttHelper.subscribe("/devices/"+ systemId + "/controls/Garage door");
});

// Called when a MQTT message for a subscribed topic is received
homa.mqttHelper.on('message', function(packet) {
	console.log("MQTT received message: %s = %s", packet.topic, packet.payload);
	if (packet.topic == "/devices/" + systemId + "/controls/Garage door") {
		var color = '#00FF00'; // green
		if (packet.payload == "open") {
			color = '#FF0000'; // red
		}
		sendFcmMessage(buildMessage(FCM_TOPIC, 'Garage door', packet.payload.toString('utf8'), color, '2'));
	}
});