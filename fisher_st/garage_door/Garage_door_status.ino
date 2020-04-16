//YWROBOT
//Compatible with the Arduino IDE 1.0
//Library version:1.1
//#include <Wire.h>
//#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// Change the credentials below, so your ESP8266 connects to your router
const char* ssid = "WhereWillYouSpendEternity";
const char* password = "vxHtbppr6a+_4pv+Zcjjk5-m";

// Change the variable to your Raspberry Pi IP address, so it connects to your MQTT broker
const char* mqtt_server = "nodered.local";

// Initializes the espClient. You should change the espClient name if you have multiple ESPs running in your home automation system
WiFiClient espClient;
PubSubClient client(espClient);

//#define LcdAddress 0x20
#define bottomHall 0
#define topHall 2
//#define buzzer 5
//#define rst 4

enum doorPositions {
  UP,
  DOWN,
  GOING_UP,
  GOING_DOWN,
};

doorPositions doorPos = DOWN;
doorPositions oldPos = DOWN;
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
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

//LiquidCrystal_I2C lcd(LcdAddress,20,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

void setup()
{
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
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(bottomHall, INPUT_PULLUP);
  pinMode(topHall, INPUT_PULLUP);
  //pinMode(buzzer, OUTPUT);
  //pinMode(rst, OUTPUT);
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  //digitalWrite(rst, HIGH);
}

void callback(String topic, byte* message, unsigned int length) {
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

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="room/lamp"){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "on"){
        
      }
      else if(messageTemp == "off"){
        
      }
  }
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("room/lamp");
    } else {
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
  if(!client.loop())
    client.connect("ESP8266Client");

  //lcd.backlight();
  if(digitalRead(bottomHall) == LOW && digitalRead(topHall) == LOW) {
    client.publish("garage/door", "no_idea");
    Serial.println("Door is lost");
    

  }
  else if(digitalRead(bottomHall) == LOW) {
    
    doorPos = DOWN;
    oldPos = doorPos;
    Serial.println("down");
    client.publish("garage/door","down");
    
  } else if (digitalRead(topHall) == LOW) {
    doorPos = UP;
    oldPos = doorPos;
    Serial.println("up");
    client.publish("garage/door","up");
  } else {
    if(oldPos == DOWN || doorPos == GOING_UP) {
      doorPos == GOING_UP;
      oldPos = doorPos;
      Serial.println("moving up");
      client.publish("garage/door","going_up");
    } else {
      doorPos == GOING_DOWN;
      oldPos = doorPos;
      client.publish("garage/door","going_down");
      Serial.println("moving down");
    }


  }


  delay(1000);
}
