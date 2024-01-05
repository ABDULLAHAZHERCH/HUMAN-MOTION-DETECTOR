#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>

// Network credentials
const char* ssid = "CH";
const char* password = "uebg8057";

// MQTT broker details
const char* mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
const char* mqttClientID = "abdazhch";

// ThingSpeak Server
const char* thingSpeakApiKey = "JZU7KZSJWG6AWKWB";
const char* thingSpeakUrl = "http://api.thingspeak.com/update?api_key=";

// Alarm Status Topic
const char* alarmOffMessage = "OFF";
const char* alarmOnMessage = "ON";
const char* alarmTopic = "AlarmStatus";

// Manual Alarm Control
const char* alarmControl = "Alarm";

// DAY NIGHT Status Topic
const char* DAY = "DAY";
const char* NIGHT = "NIGHT";
const char* dayNightStatus = "DayNightStatus";

int pirvalue = 0;
int ldrvalue;
int alarmState = 0;

// UART settings
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {

  Serial.begin(9600);
  //Serial2.begin(9600, SERIAL_8N1, 16, 17);  // RX2=16, TX2=17 on ESP32
  Serial2.begin(9600);


  // Connect to WiFi network
  connectToWifi();

  // Connect to MQTT broker
  connectToMQTT();

  // Subscribe to topic
  client.subscribe(alarmControl);  // manaually control the alarm
}

void loop() {

  // Reconnect to wifi if needed
  reconnectToWifi();

  // Reconnect to MQTT Broker if needed
  reconnectToMQTT();

  // Send Messages to client
  dayNightMessage();
  alarmMessage();

  // Send data to ThingSpeak
  updateDataToThingSpeak();

  // Handle MQTT messages
  client.loop();

  // delay in sending messages
  delay(10000);  // 10 seconds
}

void connectToWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("~");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address : ");
  Serial.println(WiFi.localIP());
}
void reconnectToWifi() {
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }
}
void connectToMQTT() {

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(mqttClientID)) {
      Serial.println("Connected to MQTT");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" Retrying in 5 seconds...");
      delay(2000);
    }
  }
}
void reconnectToMQTT() {
  if (!client.connected()) {
    connectToMQTT();
  }
}
void publishMessage(const char* topic, const char* message) {
  if (client.connected()) {
    client.publish(topic, message);
    Serial.println("Message sent! on topic " + String(topic));
  }
}
void dayNightMessage() {
  if (ldrvalue > 200) {
    publishMessage(dayNightStatus, NIGHT);
  } else {
    publishMessage(dayNightStatus, DAY);
  }
}
void alarmMessage() {
  if (alarmState == 1) {
    publishMessage(alarmTopic, alarmOnMessage);
  } else {
    publishMessage(alarmTopic, alarmOffMessage);
  }
}
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message: ");

  for (int i = 0; i < length; i++) {
    Serial.write((char)payload[i]);
    Serial.println();
  }

  String receivedValueStr = "";  // String to accumulate characters
  int receivedValue = 0;         // Integer to store the final value
  int temp;
  for (int i = 0; i < length; i++) {
    receivedValueStr += (char)payload[i];
  }
  if (Serial.available()) {
    if (topic == "Alarm") {
      if (receivedValueStr == "1") {
        Serial2.println(1);
        temp = 1;
      } else if (receivedValueStr == "0") {
        Serial2.println(0);
        temp = 0;
      }
    }
  }
  alarmState = temp;
}
void updateDataToThingSpeak() {
  // Create a HTTP client object
  HTTPClient http;
  // Construct the ThingSpeak URL with your API key and data
  String url = "";

  url = String(thingSpeakUrl) + String(thingSpeakApiKey) + "&field1=" + String(ldrvalue) + "&field2=" + String(pirvalue);

  // Begin the HTTP request
  http.begin(url);

  // Send the HTTP GET request
  int httpCode = http.GET();

  // Check the HTTP response code
  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Data sent to ThingSpeak successfully");
  } else {
    Serial.print("Failed to send data to ThingSpeak. HTTP code: ");
    Serial.println(httpCode);
  }

  // End the HTTP request
  http.end();
}