#include "PubSubClient.h" // Connect and publish to the MQTT broker
#include <Arduino.h>
#include "MHZ19.h"                                        
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"
#include "SdsDustSensor.h"

#define BAUDRATE 9600    //Terminal Baudrate

// MHZ19 CO2 Sensor
#define RX_PIN 5         //MH-Z19 RX-PIN, D1 on NodeMCU board                                     
#define TX_PIN 4         //MH-Z19 TX-PIN, D2 on NodeMCU board
MHZ19 myMHZ19;                               
SoftwareSerial mySerial(RX_PIN, TX_PIN); 
unsigned long getDataTimer = 0;                

// SDS011 Dust Sensor
int rxPin = 14; // D5
int txPin = 12; // D6
SdsDustSensor sds(rxPin, txPin);

// WiFi
const char* ssid = "Martin Router King";                 // Your personal network SSID
const char* wifi_password = "I have a stream!42(0)"; // Your personal network password

// MQTT
const char* mqtt_server = "192.168.178.82";  // IP of the MQTT broker
const char* co2_topic = "home/schlafzimmer/co2";
const char* pm25_topic = "home/schlafzimmer/pm25"; // This is pm2.5!
const char* pm10_topic = "home/schlafzimmer/pm10";
const char* mqtt_username = "jojo"; // MQTT username
const char* mqtt_password = "JaJoLaKa122"; // MQTT password
const char* clientID = "client_schlafzimmer"; // MQTT client ID

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient);

// Custom function to connet to the MQTT broker via WiFi
void connect_MQTT(){
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}


void setup() {
  Serial.begin(BAUDRATE);
  mySerial.begin(BAUDRATE);                   
  myMHZ19.begin(mySerial);
  sds.begin(); // this line will begin Serial1 with given baud rate (9600 by default)
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
}

void loop() {
  sds.wakeup();
  connect_MQTT();
  Serial.setTimeout(2000);
  
  delay(30000); // working 30 seconds

  int CO2 = myMHZ19.getCO2();
  Serial.print("CO2 (ppm): ");
  Serial.println(CO2);

  PmResult pm = sds.queryPm(); // Read from dust sensor
  if (pm.isOk()) {
    Serial.print("Dust: PM2.5 = ");
    Serial.print(pm.pm25);
    Serial.print(", PM10 = ");
    Serial.println(pm.pm10);
  } else {
    Serial.print("Could not read values from sensor, reason: ");
    Serial.println(pm.statusToString());
  }

  // PUBLISH to the MQTT Broker (topic = CO2, defined at the beginning)
  if (client.publish(co2_topic, String(CO2).c_str())) {
    Serial.println("CO2 sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("CO2 failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(co2_topic, String(CO2).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = CO2, defined at the beginning)
  if (client.publish(pm25_topic, String(pm.pm25).c_str())) {
    Serial.println("PM25 sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("PM25 failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(pm25_topic, String(pm.pm25).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = CO2, defined at the beginning)
  if (client.publish(pm10_topic, String(pm.pm10).c_str())) {
    Serial.println("PM10 sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("PM10 failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(pm10_topic, String(pm.pm10).c_str());
  }

  client.disconnect();  // disconnect from the MQTT broker

  WorkingStateResult state = sds.sleep();
  if (state.isWorking()) {
    Serial.println("Problem with sleeping the sensor.");
    delay(1000*60); // wait 1 minute
  } else {
    Serial.println("Sensor is sleeping");
    delay(60000); // wait 1 minute
  }
}