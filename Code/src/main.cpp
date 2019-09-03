#include "config.h"

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <max6675.h>

#include "setup.h"

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWire(14);
DallasTemperature sensors(&oneWire);

int thermoDO = 2;
int thermoCS = 0;
int thermoCLK = 4;
int gndPin = 5;

int relay = 12;

int buzzer = 13;

bool pumpState = false;
bool pumpMode; //auto - true; manual - false

bool alarm = false;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

unsigned long timer1s = 0;
unsigned long timer10s = 0;
unsigned long timer1m = 0;

void reconnect()
{

	digitalWrite(1, LOW);

	//if (client.connect(controllerName, mqttLogin, mqttPasswd))
	if (client.connect(controllerName))
	{
		client.subscribe("home/centralHeating/pump");
		client.subscribe("home/centralHeating/pump/mode");
		client.subscribe("home/basement/alarm");
		client.subscribe("home/controllers/1/restart");

		digitalWrite(1, HIGH);
	}
}

void callback(char * topic, byte* payload, unsigned int length)
{
	if(strcmp(topic,"home/centralHeating/pump")==0)
	{
		if((char)payload[0] == '1')
		{
			digitalWrite(relay, HIGH);

			pumpState = true;
			client.publish("home/centralHeating/pump/state", "On");
		}
		else
		{
			digitalWrite(relay, LOW);

			pumpState = false;
			client.publish("home/centralHeating/pump/state", "Off");
		}
	}

	if(strcmp(topic,"home/centralHeating/pump/mode")==0)
	{
		if((char)payload[0] == 'a')
		{
			pumpMode = true;
		}
		else
		{
			pumpMode = false;
		}
	}

	if(strcmp(topic,"home/basement/alarm")==0)
	{
		if((char)payload[0] == '1')
		{
			alarm = true;
			client.publish("home/basement/alarm/state", "On", true);
		}
		else
		{
			alarm = false;
			client.publish("home/basement/alarm/state", "Off", true);
		}
	}

	if(strcmp(topic,"home/controllers/1/restart")==0)
	{
		if((char)payload[0] == 'r')
		{
			client.publish("home/controllers/1/condition", "reset");
			delay(500);

			digitalWrite(1, LOW);
			digitalWrite(3, LOW);
			ESP.restart();
		}
	}
}

void setup()
{
	pinMode(3, OUTPUT);	//wifi
	pinMode(1, OUTPUT);	//mqtt

	pinMode(relay, OUTPUT);
	pinMode(buzzer, OUTPUT);

	WIFIsetup();

	OTAsetup();

	client.setServer(mqttIP, mqttPort);
	client.setCallback(callback);
	reconnect();

	pinMode(gndPin, OUTPUT);
	digitalWrite(gndPin, LOW);
	Serial.println("MAX6675 test");
	delay(500);

	sensors.begin();

	pumpMode = true;
	client.publish("home/centralHeating/pump/mode", "auto", true);

	client.publish("home/controllers/1/condition", "ok");
}

void loop()
{
	if (!client.connected())
	{
		reconnect();
	}
	else
	{
		client.loop();
	}

	ArduinoOTA.handle();

	if((millis() - timer1s) > 1000)
	{
		timer1s = millis();

		if(alarm == true)
		{
			digitalWrite(buzzer, !digitalRead(buzzer));
		}
		else
		{
			digitalWrite(buzzer, LOW);
		}
	}

	if((millis() - timer10s) > 10000)
	{
		timer10s = millis();
		client.publish("home/controllers/1/condition", "ok");
	}

	if((millis() - timer1m) > 60000)
	{
		timer1m = millis();

		client.publish("home/sensors/temperature/3", String(thermocouple.readCelsius()).c_str());

		sensors.requestTemperatures();
		client.publish("home/sensors/temperature/4", String(sensors.getTempCByIndex(0)).c_str());
		client.publish("home/sensors/temperature/5", String(sensors.getTempCByIndex(1)).c_str());
		client.publish("home/sensors/temperature/6", String(sensors.getTempCByIndex(2)).c_str());

		if(pumpMode == true)
		{
			if((pumpState == false) && (sensors.getTempCByIndex(0) > 40))
			{
				digitalWrite(relay, HIGH);

				pumpState = true;
				client.publish("home/centralHeating/pump/state", "On", true);
			}
			else if((pumpState == true) && (sensors.getTempCByIndex(0) < 35))
			{
				digitalWrite(relay, LOW);

				pumpState = false;
				client.publish("home/centralHeating/pump/state", "Off", true);
			}
		}
		else
		{
			if(sensors.getTempCByIndex(0) > 80)
			{
				digitalWrite(relay, HIGH);

				pumpState = true;
				client.publish("home/centralHeating/pump/state", "On", true);

				pumpMode = true;
				client.publish("home/centralHeating/pump/mode", "auto", true);
			}
		}

		if(sensors.getTempCByIndex(0) > 90)
		{
			alarm = true;
			client.publish("home/basement/alarm/state", "On");
		}
		else if(alarm == true)
		{
			alarm = false;
			client.publish("home/basement/alarm/state", "Off");
		}
	}
}
