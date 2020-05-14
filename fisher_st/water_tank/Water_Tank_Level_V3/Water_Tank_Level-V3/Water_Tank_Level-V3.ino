/*
  SimpleMQTTClient.ino
  The purpose of this exemple is to illustrate a simple handling of MQTT and Wifi connection.
  Once it connects successfully to a Wifi network and a MQTT broker, it subscribe to a topic and send a message to it.
  It will also send a message delayed 5 seconds later.
*/

#include "EspMQTTClient.h"
#include "DHT.h"
#include <driver/adc.h>

#define DHTPIN 4
#define DHTTYPE DHT11
const char* deviceName = "fisherst";
const char* mqtt_server = "nodered.local";

const char* topicDistance = "/distance";
long lastMsg = 0;
bool noSleep=false;

// Settings for ultrasonics
const int trigPin = 2;
const int echoPin = 5;

// Settings for temperature sensor
const int tempPin = 35;

//Settings for battery monitor
const int battVPin=39;

//Settings for water tank
const float radius=0.364;
const float maxHeight=180;
const float airgap=8.0; //space between sensor and max water level
const float numberTanks=3.0;

int value = 0;
float temperature = 0;
float humidity = 0;
long duration;
int distance;
float volume;
float battVoltage;
char* tempTopic;


DHT dht(DHTPIN, DHTTYPE);


EspMQTTClient client(
  "WhereWillYouSpendEternity",
  "vxHtbppr6a+_4pv+Zcjjk5-m",
  mqtt_server,  // MQTT Broker server ip
  "admin",   // Can be omitted if not needed
  "M1dn1gh7",   // Can be omitted if not needed
  deviceName,     // Client name that uniquely identify your device
  1883              // The MQTT port, default to 1883. this line can be omitted
);

void setup()
{
  Serial.begin(115200);
  //Set deep sleep for wakeup after 10 minutes  
  esp_sleep_enable_timer_wakeup(600*1000000);

  // Optionnal functionnalities of EspMQTTClient : 
  client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  client.enableHTTPWebUpdater("admin", "M1dn1gh7"); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overrited with enableHTTPWebUpdater("user", "password").
  client.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input/pinMode(tempPin, INPUT);
  pinMode(tempPin, INPUT);
  dht.begin();
  analogReadResolution(12);
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_11db);
}

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  // Subscribe to "mytopic/test" and display received message to Serial
  client.subscribe("fisherst/sleep", [](const String & payload) {
    Serial.println(payload);
    if ((millis() >= 300000) ) // || (esp_sleep_get_wakeup_cause()==3))
    {
      if (noSleep==false) 
      {
        Serial.println("Sleepy time");
        esp_deep_sleep_start();
      }
    }
  });
  client.subscribe("fisherst/maxheight", [](const String & payload) {
    Serial.println(payload);
  });
  client.subscribe("fisherst/airgap", [](const String & payload) {
    Serial.println(payload);
  });
  client.subscribe("fisherst/radius", [](const String & payload) {
    Serial.println(payload);
  });
  client.subscribe("fisherst/numbertanks", [](const String & payload) {
    Serial.println(payload);
  });
  //client.subscribe("fisherst/stopsleep", [](const String & payload) {
    //if (payload=="stopsleep") noSleep=true;
  //});
   




}

void loop()
{
  client.loop();

  long now = millis();
  if (now - lastMsg > 150000)
  {
    lastMsg = now;
    ////////////////////////////////////////////////////////////////////////////
    // Publish IP Address
    String tempIPAddress=WiFi.localIP().toString();
    client.publish("fisherst/deviceipaddress", tempIPAddress);
    
    ////////////////////////////////////////////////////////////////////////////
    //Read battery voltage
    float rawVoltage = adc1_get_raw(ADC1_CHANNEL_3);
    battVoltage = (rawVoltage / 1088)*1.0;
    client.publish("fisherst/batteryvoltage", String(battVoltage));
    
    ////////////////////////////////////////////////////////////////////////////
    // Internal tanke temperature in Celsius
    float voltage = (analogRead(tempPin)/4096.0)*3295.0; 
    temperature=(voltage/10.0)-273.3;  
    client.publish("fisherst/tamktemperature", String(temperature));

    /////////////////////////////////////////////////////////////////////////////
    // Read ultrasonic
    // Clears the trigPin
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    float speedOfSound=331.4 +0.6*temperature;
    distance = duration * speedOfSound / 20000;
    ///////client.publish("fisherst/rawdistance", String(distance));
    distance = distance - airgap;
    if (distance < 0) distance = 0;
    if (distance > maxHeight) distance=maxHeight;
    volume=((maxHeight-distance)/100) * radius * radius * PI * numberTanks*1000;
    client.publish("fisherst/waterlevel", String(volume));

    ///////////////////////////////////////////////////////////////////////////
    //Read ambient temperature and humidity
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(h) || isnan(t))
    {
      Serial.println("Failed to read from DHT sensor");
    }
    client.publish("fisherst/humidity", String(h));
    client.publish("fisherst/ambienttemp", String(t));
    
  }
  
  
}
