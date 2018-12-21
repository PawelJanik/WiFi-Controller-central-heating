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

bool pumpState = false; 
bool pumpMode; //auto - true; manual - false

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

unsigned long timer10s = 0;
unsigned long timer1m = 0;

void reconnect() 
{
	//Serial.print("Attempting MQTT connection...");
	if (client.connect(controllerName, mqttLogin, mqttPasswd))
	{
		//Serial.println("connected");
		
		client.subscribe("home/centralHeating/pump");
		client.subscribe("home/centralHeating/pump/mode");
		
		digitalWrite(1, HIGH);
	} 
	else
	{
		//Serial.print("failed, rc=");
		//Serial.print(client.state());
		//Serial.println(" try again in 1 seconds");
		delay(1000);
		digitalWrite(1, LOW);
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
}

void setup() 
{
	pinMode(3, OUTPUT);	//wifi
	pinMode(1, OUTPUT);	//mqtt
	
	pinMode(relay, OUTPUT);
	
	//Serial.begin(115200);
	//Serial.println("Booting");
	//Serial.println("Connecting to WiFi...");
	
	WIFIsetup();
	
	//Serial.println("WiFi Ready");
	//Serial.print("IP address: ");
	//Serial.println(WiFi.localIP());
	
	//Serial.println("OTA setup...");
	OTAsetup();
	//Serial.println("OTA ready");

	//Serial.println("MQTT setup...");
	client.setServer(mqttIP, mqttPort);
	client.setCallback(callback);
	reconnect();
	//Serial.println("MQTT ready");

	pinMode(gndPin, OUTPUT); 
	digitalWrite(gndPin, LOW);
  
	//Serial.println("MAX6675 setup...");
	delay(500);
	//Serial.println("MAX6675 ready");
	
	sensors.begin();
	
	pumpMode = true; 
	client.publish("home/centralHeating/pump/mode", "auto");
}

void loop() 
{
	if (!client.connected()) 
	{
		reconnect();
	}
	
	client.loop();

	ArduinoOTA.handle();
	
	if((millis() - timer10s) > 10000)
	{
		timer10s = millis();
		client.publish("home/controllers/1/status", "ok");
		Serial.println("OK");
	}
	
	if((millis() - timer1m) > 2000)
	{
		timer1m = millis();
		
		//client.publish("home/sensors/temperature/3", String(thermocouple.readCelsius()).c_str());
		
		sensors.requestTemperatures();
		client.publish("home/sensors/temperature/4", String(sensors.getTempCByIndex(0)).c_str());
		client.publish("home/sensors/temperature/5", String(sensors.getTempCByIndex(1)).c_str());
		
		if(pumpMode == true)
		{
			if((pumpState == false) && (sensors.getTempCByIndex(0) > 40))
			{
				digitalWrite(relay, HIGH);
			
				pumpState = true; 
				client.publish("home/centralHeating/pump/state", "On");
			}
			else if((pumpState == true) && (sensors.getTempCByIndex(0) < 35))
			{
				digitalWrite(relay, LOW);
				
				pumpState = false; 
				client.publish("home/centralHeating/pump/state", "Off");
			}
		}
		else
		{
			if(sensors.getTempCByIndex(0) > 80)
			{
				digitalWrite(relay, HIGH);
			
				pumpState = true; 
				client.publish("home/centralHeating/pump/state", "On");
				
				pumpMode = true;
				client.publish("home/centralHeating/pump/mode", "auto");
			}
		}
	}
}
