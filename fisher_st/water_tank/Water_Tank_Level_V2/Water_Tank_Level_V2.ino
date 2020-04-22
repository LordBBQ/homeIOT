
#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
#include <driver/adc.h>


#define DHTPIN 4
#define DHTTYPE DHT11

// Replace the next variables with your SSID/Password combination
const char* ssid = "WhereWillYouSpendEternity";
const char* password = "vxHtbppr6a+_4pv+Zcjjk5-m";
const char* deviceName = "fisherst-dev";

// MQTT Broker IP address:
const char* mqtt_server = "nodered.local";

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
const float airgap=10.0; //space between sensor and max water level
const float numberTanks=3.0;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;
float temperature = 0;
float humidity = 0;
long duration;
int distance;
float volume;
float battVoltage;

DHT dht(DHTPIN, DHTTYPE);

void setup() 
{
  Serial.begin(115200);
  //Set deep sleep for wakeup after 10 minutes  
  esp_sleep_enable_timer_wakeup(600*1000000);
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input/pinMode(tempPin, INPUT);
  pinMode(tempPin, INPUT);
  dht.begin();
  analogReadResolution(12);
  adc1_config_width(ADC_WIDTH_12Bit);
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_11db);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();
  //Process received messages
  if(String(topic) == "fisherst-dev/sleep")
  {
    //We only want to go to deep sleep once we have been awake for a minute or if we are waking from a previous sleep
    //this is so that if we later add OTA updates there is a chance to get the update happening
    if ((millis() >= 60000) || (esp_sleep_get_wakeup_cause()==3))
    {
      Serial.println("Sleepy time");
      esp_deep_sleep_start();
    }
    else
    {
      Serial.println("Can I stay up another few minutes?");
    }
    
  }
  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("fisherst-dev/sleep");
    } else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) 
  {
    reconnect();
  }
  //client.loop();

  long now = millis();
  if ((now - lastMsg) >= 5000) {
    lastMsg = now;
    
    //Send IP Address to let broker know we are awake
    String tempIPAddress=WiFi.localIP().toString();
    char deviceIPAddressString[20];
    tempIPAddress.toCharArray(deviceIPAddressString,16);
    Serial.println(deviceIPAddressString);
    char tempName[50];
    strcat(tempName, deviceName);
    strcat(tempName, "/");
    strcat (tempName,"deviceipaddress");
    client.publish(tempName, deviceIPAddressString);
    //Serial.print ("Message sent to: ");
    //Serial.println (tempName);
    
    // Temperature in Celsius
    float voltage = (analogRead(tempPin)/4096.0)*3295.0; 
    temperature=(voltage/10.0)-273.3;  
    // Convert the value to a char array
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Temp Sensor Voltage: ");
    Serial.println(voltage);
    Serial.print("Temperature: ");
    Serial.println(tempString);
    client.publish("fisherst-dev/temperature", tempString);
    
    //Read battery voltage
    float rawVoltage = adc1_get_raw(ADC1_CHANNEL_3);
    battVoltage = (rawVoltage / 1088)*2.0;
    char battVoltString[8];
    dtostrf(battVoltage, 1, 2, battVoltString);
    client.publish("fisherst-dev/batteryvolts", battVoltString);
    

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
    //distance= duration*0.034/2;
    distance = duration * speedOfSound / 20000;
    distance = distance - airgap;
    if (distance < 0) distance = 0;
    if (distance > maxHeight) distance=maxHeight;
    char distanceString[8];
    dtostrf(distance, 1, 2, distanceString);
    client.publish("fisherst-dev/distance", distanceString);
    //Serial.println(String(distance));
    char volumeString[8];
    volume=((maxHeight-distance)/100) * radius * radius * PI * numberTanks*1000;
    dtostrf(volume, 1, 2, volumeString);
    client.publish("fisherst-dev/waterlevel", volumeString);
    Serial.print("Volume: ");
    Serial.println(volumeString);
    
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(h) || isnan(t))
    {
      Serial.println("Failed to read from DHT sensor");
    }
    Serial.println("Humidity: " + String(h) + "%");
    Serial.println ("Temperature(DHT11): " + String(t) + " C");
    char humidityString[8];
    dtostrf(h, 1, 2, humidityString);
    client.publish("fisherst-dev/humidity", humidityString);
    char ambientTemperatureString[8];
    dtostrf(t, 1, 2, ambientTemperatureString);
    client.publish("fisherst-dev/ambienttemp", ambientTemperatureString);
    client.loop();
  }
}
