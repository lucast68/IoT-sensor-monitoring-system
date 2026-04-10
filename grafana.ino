// Publishes JSON-formatted data to MQTT broker for Grafana visualization every 10 seconds

#include <Wire.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <arduino_secrets.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoJson.h>

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// MQTT broker configuration
const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/bmp085";

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

// DS18B20 temperature sensor on GPIO 33
#define ONE_WIRE_BUS 33
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);

void ConnectToWiFi();
void reconnectMQTT();

void setup() {
  Serial.begin(115200);
  Wire.begin();

  connectToWiFi();

  // Connects to MQTT broker
  if (mqttClient.connect(mqtt_server, mqtt_port)) {
    Serial.print("Connected to MQTT broker!");
  } else {
    Serial.println(mqttClient.connectError());
  }

  // Intializes DS18B20
  ds18b20.begin();
  Serial.println("DS18B20 sensor initialized.");
}

void loop() {
  // Maintains active MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.poll();

  static unsigned long lastMsg = 0;
  unsigned long now = millis();

  // Publishes sensor data every 10 seconds
  if (now - lastMsg > 10000) {
    lastMsg = now;

    // Requests temperature reading from DS18B20
    ds18b20.requestTemperatures();
    float tempC = ds18b20.getTempCByIndex(0);

    // Checks for sensor error (sensor returns -127°C when disconnected)
    if (tempC == -127.00) {
      Serial.println("ERROR: DS18B20 not detected! Check wiring.");
      return;
    }

    // Creates JSON document with sensor data for Grafana ingestion
    // Format: {"sensor": "DS18B20", "temperature": 23.45}
    StaticJsonDocument<200> doc;
    doc["sensor"] = "DS18B20";
    doc["temperature"] = tempC;

    // Serializes JSON to string
    String payload;
    serializeJson(doc, payload);

    // Publishes JSON payload to MQTT broker
    if (mqttClient.beginMessage(mqtt_topic)) {
      mqttClient.print(payload);
      mqttClient.endMessage();
      Serial.print("Published: ");
      Serial.println(payload);
    } else {
      Serial.println("Failed to publish message");
    }
  }
}

// Establishes Wi-Fi connection with connection feedback
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  // Polls connection status with visual feedback
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Reconnects to MQTT broker with retry logic
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (mqttClient.connect(mqtt_server, mqtt_port)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc: ");
      Serial.print(mqttClient.connectError());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Waits before retrying
    }
  }
}