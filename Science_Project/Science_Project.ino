#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include "EspMQTTClient.h"
#include <NewPing.h>
#include <time.h>



#define DHTPIN 27     // Digital pin connected to the DHT sensor 
#define DHTTYPE    DHT11     // DHT 22 (AM2302)
#define TRIGGER_PIN  33
#define ECHO_PIN     32
#define MAX_DISTANCE 200

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 36000;
const int   daylightOffset_sec = 0;

//======================================================================
//
const char* versionString="1.1.6";
//
//======================================================================
const char* deviceName = "whateveryouwant";
const char* mqtt_server = "nodered.local";

long lastMsg = 0;
String dataString;

DHT dht(DHTPIN, DHTTYPE);

EspMQTTClient client(
  "WhereWillYouSpendEternity",
  "vxHtbppr6a+_4pv+Zcjjk5-m",
  mqtt_server,  // MQTT Broker server ip
  "admin",   // Can be omitted if not needed
  "Pa55w0rd",   // Can be omitted if not needed
  deviceName,     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
 
void setup() {
  //Serial Port begin
  Serial.begin (115200);
  // Optionnal functionnalities of EspMQTTClient : 
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater("admin", "M1dn1gh7"); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true


  dht.begin();

}


void printLocalTime()
{
  time_t rawtime;
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
   return;
  }
  char timeStringBuff[50]; //50 chars should be enough
  //strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
  strftime(timeStringBuff, sizeof(timeStringBuff), "%d-%b-%Y %H:%M:%S", &timeinfo);
  //print like "const char*"
  Serial.println(timeStringBuff);

  //Optional: Construct String object 
  String asString(timeStringBuff);
  dataString = asString;
}

void onConnectionEstablished()
{
  
  //client.subscribe("fisherst/maxheight", [](const String & payload) {
  //  Serial.println(payload);
  //});
  //client.subscribe("fisherst/airgap", [](const String & payload) {
  //  Serial.println(payload);
  //});
  //client.subscribe("fisherst/radius", [](const String & payload) {
  //  Serial.println(payload);
  //});
  //client.subscribe("fisherst/numbertanks", [](const String & payload) {
  //  Serial.println(payload);
  //});
  //client.subscribe("fisherst/stopsleep", [](const String & payload) {
    //if (payload=="stopsleep") noSleep=true;
  //});
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
   




}
 
void loop() 

{

 client.loop();

  long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;
  
    ////////////////////////////////////////////////////////////////////////////
    // Publish IP Address
    String tempIPAddress=WiFi.localIP().toString();
    client.publish("whateveryouwant/deviceipaddress", tempIPAddress);

    ////////////////////////////////////////////////////////////////////////////
    // Publish Version
    //
    client.publish("whateveryouwant/softwareVersion", versionString);
    
    /////////////////////////////////////////////////////////////////////////////
    // Read ultrasonic
    float time=sonar.ping_median(10);
    client.publish("whateveryouwant/duration", String(time));
    Serial.print("Time: ");
    Serial.print(String(time));
    Serial.println (" us.");

    ///////////////////////////////////////////////////////////////////////////
    //Read ambient temperature and humidity
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(h))
    {
      Serial.println("Failed to read humidity from DHT sensor");
      h=0;
    }
    if (isnan(t))
    {
      Serial.println("Failed to read temperature from DHT sensor");
      t=0;
    }
    client.publish("whateveryouwant/humidity", String(h));
    client.publish("whateveryouwant/ambienttemp", String(t));
    Serial.print("Temperature: ");
    Serial.print(String(t));
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(String(h));
    Serial.println(" %");

    printLocalTime();
    dataString = dataString + "," + String(time) + "," + String(t) + "," +String (h);
    client.publish("whateveryouwant/data", dataString);
 

  }


  
}
