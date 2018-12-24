void WIFIsetup()
{
	digitalWrite(3, LOW);
	
	WiFi.mode(WIFI_STA);
	WiFi.begin(wifiSSID, wifiPasswd);
	while (WiFi.waitForConnectResult() != WL_CONNECTED) 
	{
		delay(5000);
		ESP.restart();
	}
	
	
	digitalWrite(3, HIGH);
}

void OTAsetup()
{
	ArduinoOTA.setHostname(controllerName);
	ArduinoOTA.setPassword(otaPasswd);

	ArduinoOTA.onStart([]() 
	{
		String type;
		if(ArduinoOTA.getCommand() == U_FLASH) 
		{
			type = "sketch";
		} 
		else 
		{
			type = "filesystem";
		}
	});
	
	ArduinoOTA.begin();
}
