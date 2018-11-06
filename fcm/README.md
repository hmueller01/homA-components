# FCM
Firebase Cloud Messaging (FCM) can be used to send messages to clients on Android, iOS and Web.

This node.js JavaScript subscribes to HomA MQTT messages and uses FCM to send messages to clients that are
subscribed to the `news` topic.

### Basic Requirements
* A configured project in the [Firebase Console](https://console.firebase.google.com).
* An e.g. Android App that is configured to receive Firebase Cloud Messages for this project and
is subscribed to the `news` topic.

### Installation
```
$ npm install
```
* Modify ```fcm.js``` to your needs, especially ```PROJECT_ID```.
* Download ```service-account.json``` from [Firebase Console](https://console.firebase.google.com) and save in the folder as configured in ```fcm.js```.
* Copy ```mqtt-options-example.json``` to ```mqtt-options.json``` to setup username and password of the MQTT broker.

### Start
Start the application.
``` 
$ ./fcm.js [--brokerHost 127.0.0.1] [--brokerPort 1883] [--systemId $SYSTEMID]
```
The default $SYSTEMID is ```123456-garage```.