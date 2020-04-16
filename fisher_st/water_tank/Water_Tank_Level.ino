/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"
//#include <Wire.h>
//#include <Adafruit_BME280.h>
//#include <Adafruit_Sensor.h>
#define DHTPIN 4
#define DHTTYPE DHT11
// Replace the next variables with your SSID/Password combination
const char* ssid = "WhereWillYouSpendEternity";
const char* password = "vxHtbppr6a+_4pv+Zcjjk5-m";

// MQTT Broker IP address:
const char* mqtt_server = "nodered.local";

const int trigPin = 2;
const int echoPin = 5;
const int tempPin = 34;
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

DHT dht(DHTPIN, DHTTYPE);
void setup() 
{
  Serial.begin(115200);
  
  esp_sleep_enable_timer_wakeup(600*1000000);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input/pinMode(tempPin, INPUT);
  dht.begin();
  analogReadResolution(12);
  pinMode(tempPin, INPUT);
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

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic fisherst/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message

  if(String(topic) == "fisherst/sleep")
  {
    Serial.println("Sleepy time");
    esp_deep_sleep_start();
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
      //client.subscribe("fisherst/output");
      //client.subscribe("fisherst/level");
      client.subscribe("fisherst/sleep");
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
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    
    // Temperature in Celsius
    float voltage = (analogRead(34)/4096.0)*3340.0; 
    temperature=(voltage/10.0)-273.3;  
    // Convert the value to a char array
    char tempString[8];
    dtostrf(temperature, 1, 2, tempString);
    Serial.print("Temperature: ");
    Serial.println(tempString);
    client.publish("fisherst/temperature", tempString);

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
    //Serial.println(String(distance));
    char volumeString[8];
    volume=((maxHeight-distance)/100) * radius * radius * PI * numberTanks*1000;
    dtostrf(volume, 1, 2, volumeString);
    client.publish("fisherst/waterlevel", volumeString);

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
    client.publish("fisherst/humidity", humidityString);
    char ambientTemperatureString[8];
    dtostrf(t, 1, 2, ambientTemperatureString);
    client.publish("fisherst/ambienttemp", ambientTemperatureString);
  }
}
