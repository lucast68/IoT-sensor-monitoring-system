// Reads and publishes BMP085 pressure sensor data to MQTT broker every 5 seconds

#include <Wire.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <arduino_secrets.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// MQTT broker configuration
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char *mqtt_topic = "sensor/bmp085";

// BMP085 air pressure sensor
Adafruit_BMP085_Unified bmp;

void connectToWifi();
void reconnectMQTT();

void setup() {
  Serial.begin(115200);

  connectToWifi();

  // Connects to MQTT broker
  if (mqttClient.connect(mqtt_server, mqtt_port)) {
    Serial.println("Connected to MQTT broker!");
    } else {
    Serial.println(mqttClient.connectError());
  }
  
  // Initializes BMP085 sensor
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) {}
  }
}

void Loop() {
  // Maintains MQTT connection
  if (!mqttClient.connected()) {
    reconnectMQTT();
  }

  mqttClient.poll();

  static unsigned long lastMsg = 0;
  unsigned long now = millis();

  // Publishes sensor data every 5 seconds
  if (now - lastMsg > 5000) {
    lastMsg = now;

    // Reads pressure and temperature from BMP085
    sensor_event_t event;
    bmp.getEvent(&event);

    // Publishes if pressure reading is valid (non-zero)
    if (event.pressure) {
        float temperature = 0;
        bmp.getTemperature(&temperature);
        float pressure = event.pressure / 100.0; // Converts pascals to hPa

        string payload = "Temperature: " + String(temperature) + " C, Pressure: " + String(pressure) + " hPa";

        // Sends payload to MQTT broker
        if mqttClient.beginMessage(mqtt_topic) {
        mqttClient.print(payload);
        mqttClient.endMessage();
        } else {
        Serial.println("Failed to publish message");
        }
      }
    }
}

// Establishes Wi-Fi connection with connection status feedback
void connectToWifi() {
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  // Waits for connection with visual feedback
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

// Reconnects to MQTT broker with retry logic
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT broker...");
    if (mqttClient.connect(mqtt_server, mqtt_port)) {
      Serial.println("Connected!");
    } else {
      Serial.print("failed, rc: ");
      Serial.print(mqttClient.connectError());
      Serial.println(" try again in 5 seconds");
      delay(5000); // Waits before retrying
    }
  }
}
