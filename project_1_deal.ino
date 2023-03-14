#include <Arduino.h>  
#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProTemperaturesensor.h"
#include "SinricProSwitch.h"
#include "DHT.h"
 
#define WIFI_SSID         "AndroidAP"    
#define WIFI_PASS         "phka8539"
#define APP_KEY           "b3deda77-fd63-44b0-a2ce-3ee7bbb6e2b1"     
#define APP_SECRET        "0d22f1ef-5373-434f-8606-fe7f068e4bc2-a0af938b-e461-428a-a768-6ec472c000d2"   
#define TEMP_SENSOR_ID    "63f1f4825ec7d92a470f5f01"   
#define SWITCH_ID1        "63ef9fca1bb4e19c119ecb47"    
#define wifiLed 2
 
#define BAUD_RATE         9600                
#define EVENT_WAIT_TIME   6000              
#define DHT_PIN    26
#define RELAY_PIN         12                
#define DHTTYPE DHT11       //DHT 21  (AM2301)
DHT dht(DHT_PIN, DHTTYPE);   
 
bool deviceIsOn;                              // Temeprature sensor on/off state
float temperature;                            // actual temperature
float humidity;                               // actual humidity
float lastTemperature;                        // last known temperature (for compare)
float lastHumidity;                           // last known humidity (for compare)
unsigned long lastEvent = (-EVENT_WAIT_TIME); // last time event has been sent


//FUNCTION FOR TELLING STATE  
bool onPowerState(const String &deviceId, bool &state) {
  Serial.printf("Temperaturesensor turned %s (via SinricPro) \r\n", state?"on":"off");
  deviceIsOn = state; // turn on / off temperature sensor
   digitalWrite(RELAY_PIN, !state);            
  return true; // request handled properly
}
 
//FUNCTION FOR HANDLING TEMPERATURE SENSOR
void handleTemperaturesensor() {
  if (deviceIsOn == false) return; // device is off...do nothing
 
  unsigned long actualMillis = millis();
  if (actualMillis - lastEvent < EVENT_WAIT_TIME) return; //only check every EVENT_WAIT_TIME milliseconds
 
  temperature = dht.readTemperature();          // get actual temperature in °C
  humidity = dht.readHumidity();                // get actual humidity
 
  if (isnan(temperature) || isnan(humidity)) { // reading failed... 
    Serial.printf("DHT reading failed!\r\n");  // print error message
    return;                                    // try again next time
  } 
 
  if (temperature == lastTemperature || humidity == lastHumidity) return; // if no values changed do nothing...
 
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];  // get temperaturesensor device
  bool success = mySensor.sendTemperatureEvent(temperature, humidity); // send event
  if (success) {  
    Serial.printf("Temperature: %2.1f Celsius\tHumidity: %2.1f%%\r\n", temperature, humidity);
  } else {  
    Serial.printf("Something went wrong...could not send Event to server!\r\n");
  }
 
  lastTemperature = temperature;  // save actual temperature for next compare
  lastHumidity = humidity;        // save actual humidity for next compare
  lastEvent = actualMillis;       // save actual time for next compare
}
 
 
// setup function for WiFi connection
void setupWiFi() {
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, HIGH);
  IPAddress localIP = WiFi.localIP();
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %d.%d.%d.%d\r\n", localIP[0], localIP[1], localIP[2], localIP[3]);
}
 
// setup function for SinricPro
void setupSinricPro() {
  SinricProTemperaturesensor &mySensor = SinricPro[TEMP_SENSOR_ID];
  SinricProSwitch &mySwitch = SinricPro[SWITCH_ID1];   
  mySensor.onPowerState(onPowerState);
 mySwitch.onPowerState(onPowerState);              
  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);  
}
 
// main setup function
void setup() {
  Serial.begin(BAUD_RATE); Serial.printf("\r\n\r\n");
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);                 
      SinricProSwitch& mySwitch = SinricPro[SWITCH_ID1];   
      mySwitch.onPowerState(onPowerState);              
    dht.begin();
 
  
  setupWiFi();
  setupSinricPro();
}
 
void loop() {
  SinricPro.handle();
  handleTemperaturesensor();}
