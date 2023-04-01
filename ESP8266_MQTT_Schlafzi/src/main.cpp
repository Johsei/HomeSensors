#include "PubSubClient.h" // Connect and publish to the MQTT broker
#include <Arduino.h>
#include "MHZ19.h"                                        
#include <SoftwareSerial.h>
#include "ESP8266WiFi.h"
#include "SdsDustSensor.h"
#include <Wire.h>
#include "DHT20.h"

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

// DHT20 Temperature & Humidity Sensor
DHT20 DHT;

// WiFi
const char* ssid = "Martin Router King";                 // Your personal network SSID
const char* wifi_password = "I have a stream!42(0)"; // Your personal network password

// MQTT
const char* mqtt_server = "192.168.178.82";  // IP of the MQTT broker
const char* co2_topic = "home/schlafzimmer/co2";
const char* temperature_topic = "home/schlafzimmer/temperature";
const char* humidity_topic = "home/schlafzimmer/humidity";
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
  DHT.begin(0, 2); // D3 Data, D4 Clock
  Serial.print("DHT20 LIBRARY VERSION: ");
  Serial.println(DHT20_LIB_VERSION);                 
  myMHZ19.begin(mySerial);
  sds.begin(); // this line will begin Serial1 with given baud rate (9600 by default)
  Serial.println(sds.queryFirmwareVersion().toString()); // prints firmware version
  Serial.println(sds.setActiveReportingMode().toString()); // ensures sensor is in 'active' reporting mode
  }

void loop() {
  sds.wakeup();
  delay(30000); // working 30 seconds

  connect_MQTT();
  Serial.setTimeout(2000);

  // MHZ19 CO2
  int CO2 = myMHZ19.getCO2();
  Serial.print("CO2 (ppm): ");
  Serial.println(CO2);

  // DHT20 Temperature & Humidity
  int dht_status = DHT.read();
  int temperature = DHT.getTemperature();
  int humidity = DHT.getHumidity();
  Serial.print("Temperature: ");
  Serial.println(temperature);
  Serial.print("Humidity: ");
  Serial.println(humidity);
  switch (dht_status)
    {
      case DHT20_OK:
        Serial.print("DHTR20 OK");
        break;
      case DHT20_ERROR_CHECKSUM:
        Serial.print("CDHTR20 hecksum error");
        break;
      case DHT20_ERROR_CONNECT:
        Serial.print("DHTR20 Connect error");
        break;
      case DHT20_MISSING_BYTES:
        Serial.print("DHTR20 Missing bytes");
        break;
      case DHT20_ERROR_BYTES_ALL_ZERO:
        Serial.print("DHTR20 All bytes read zero");
        break;
      case DHT20_ERROR_READ_TIMEOUT:
        Serial.print("DHTR20 Read time out");
        break;
      case DHT20_ERROR_LASTREAD:
        Serial.print("DHTR20 Error read too fast");
        break;
      default:
        Serial.print("DHTR20 Unknown error");
        break;
    }
    Serial.print("\n");

  // SDS011 Dust Sensor
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

  // PUBLISH to the MQTT Broker (topic = temperature, defined at the beginning)
  if (client.publish(temperature_topic, String(temperature).c_str())) {
    Serial.println("temperature sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("temperature failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(temperature_topic, String(temperature).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = humidity, defined at the beginning)
  if (client.publish(humidity_topic, String(humidity).c_str())) {
    Serial.println("humidity sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    Serial.println("humidity failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(humidity_topic, String(humidity).c_str());
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

  // PUBLISH to the MQTT Broker (topic = pm25, defined at the beginning)
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

  // PUBLISH to the MQTT Broker (topic = pm10, defined at the beginning)
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
    Serial.println("Problem with sleeping the dust sensor.");
  } else {
    Serial.println("Dust sensor is sleeping");
  }

  delay(60000); // wait 1 minute
}