#include <SoftwareSerial.h>

//for ota
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

SoftwareSerial serial(3,1);

const char *ssid     = "osiek";
const char *password = "osiek123";

byte p[8];
int i;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Serial.begin(19200);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    //ESP.restart();
  }
  
  // OTA
  ArduinoOTA.setHostname("esp8266_linefollower_Dziezok_Piorko");
  
  // No authentication by default
  ArduinoOTA.setPassword("linefollower");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  //END OF OTA
}

int data_in;

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  Serial.read(p,8);
  for(i=0;i<=7;i++){
    Serial.print(p[i]);
    Serial.print(" ");
  }
  Serial.println();
  delay(1000);
} 
