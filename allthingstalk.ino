#include "allthingstalk_spark.h"
#include "MQTT/MQTT.h"

/*
  AllThingsTalk Makers Spark Example

  ### Instructions

1. Setup the SPARK hardware
	- LED + 1K resistor R1 connected to pin D0
	- LDR + 10K resistor R2 connected to pin A0
	      _____________________________________________
	     |                                            |
	   --|---------------------------------------     |
	  / A0 A1 A2 A3 A4 A5 A6 A7 RX  TX  GND VIN |     |
	  |                             _        ___|     |
	  | ||    |||||||||||||||||    |_| mode |   |     |
	  | ||    ||||||CC3000|||||  ( )_       |   |     |
	  | ||    |||||||||||||||||    |_| rst  |___|     |
	  |                                         |     |
	  \ D0 D1 D2 D3 D4 D5 D6 D7 GND 3V3 RST 3V3 |     |
	   --|-----------------------|---------------     |
	     |               _       |           |        |
	     o---/\/\/\/\---(_)------o           _        |
	            R1      LED      |          (~)  LDR  |
	           (1K)              |           |        |
	                             |           |        |
	                             |-/\/\/\/\--o-------o
	                                  R2
	                                 (10K)

  2. Open the project in the Spark IDE (https://www.spark.io/build/)
  3. Fill in the missing strings (deviceId, clientId, clientKey, mac) and optionally change/add the sensor & actuator names, ids, descriptions, types
	 For extra actuators, make certain to extend the callback code at the end of the sketch.
  4. Compile and flash the Spark Core with the new firmware

*/

void callback(char* topic, byte* payload, unsigned int length);

char clientId[]  = "seppestas";
char clientKey[] = "lfl311jr210";
char deviceId[]  = "T3uhHqCR62z6zvqOroiYP1h";

ATTDevice Device(deviceId, clientId, clientKey);

char httpServer[] = "api.smartliving.io";
char* mqttServer  = "broker.smartliving.io";

char actuatorId = '3';
char sensorId   = '2';

int led = D0;
int ldr = A0;

void callback(char* topic, byte* payload, unsigned int length);
TCPClient tcpClient;
MQTT mqttClient(mqttServer, 1883, callback);

void setup() {
	RGB.control(true);
	pinMode(led, OUTPUT);
	pinMode(ldr, INPUT);

	Serial.begin(9600);

	if(Device.Connect(httpServer)) // connect the device with the IOT platform.
	{
		RGB.color(0, 128, 0);
		Device.AddAsset(actuatorId, F("LED"), F("Carefull. May cause photon emission"), true, F("bool"));
		Device.AddAsset(sensorId, F("Light sensor"), F("A simple light sensor"), false, F("int"));
		Device.Subscribe(mqttClient); // Subsribe to the iot platform (activate mqtt)
	}
	else
	{
		RGB.color(255, 0, 0);
		while(true);
	}
}

unsigned long prevTime; //only send every x amount of time.
unsigned int prevVal =0;
void loop()
{
	unsigned long curTime = millis();
	if (curTime > (prevTime + 1000))
	{
		unsigned int lightRead = analogRead(ldr); // read from light sensor (LDR)
		if(prevVal != lightRead) {
			Device.Send(String(lightRead), sensorId);
			prevVal = lightRead;
		}
		prevTime = curTime;
	}
	Device.Process();
}

// recieve message
void callback(char* topic, byte* payload, unsigned int length) {
	String msgString;
	{ // put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
		char message_buff[length + 1];						//need to copy over the payload so that we can add a /0 terminator, this can then be wrapped inside a string for easy manipulation.
		strncpy(message_buff, (char*)payload, length);		//copy over the data
		message_buff[length] = '\0';

		msgString = (char *) message_buff;
		msgString.toLowerCase();                       // to make certain that our comparison later on works ok (it could be that a 'True' or 'False' was sent)
	}
	char* idOut = NULL;
	{ // put this in a sub block, so any unused memory can be freed as soon as possible, required to save mem while sending data
		int topicLength = strlen(topic);

		Serial.print("Payload: ");			           // show some debugging.
		Serial.println(msgString);
		Serial.print("topic: ");
		Serial.println(topic);

		if (topic[topicLength - 9] == actuatorId)      // warning: the topic will always be lowercase. The id of the actuator to use is near the end of the topic. We can only get actuator commands, so no extra check is required.
		{
			if (msgString == "false") {
				digitalWrite(led, LOW);
				idOut = &actuatorId;
			}
			else if (msgString == "true") {
				digitalWrite(led, HIGH);
				idOut = &actuatorId;
			}
		}
	}
	if(idOut != NULL)                // also let the iot platform know that the operation was succesful: give it some feedback. This also allows the iot to update the GUI's correctly & run scenarios.
		Device.Send(msgString, *idOut);
}
