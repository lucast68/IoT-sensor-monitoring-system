// Reads pressure from BPM085 sensor and temperature from DS18B20 sensor
// Publishes combined readings to MQTT broker

#include <Wire.h>
#include <ArduinoMqttClient.h>
#include <WiFi.h>
#include <arduino_secrets.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

// MQTT broker configuration
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

const char* mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
const char* mqtt_topic = "sensor/bmp085";

// Intialization of sensors
Adafruit_BMP085_Unified bmp; // BMP085 air pressure sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire); // DS18B20 temperature sensor

void setup() {
    Serial.begin(115200);
    Wire.begin();

    connectToWiFi();

    // Connects to MQTT broker
    if (mqttClient.connect(mqtt_server, mqtt_port)) {
        Serial.println("Connected to MQTT broker");
    } else {
        Serial.println(mqttClient.connectError());
    }

    // Initializes BMP085 sensor
    if (!bmp.begin()) {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1);
    }

    // Initializes DS18B20 temperature sensor
    ds18b20.begin();
    Serial.println("DS18B20 sensor initialized!");
}

void loop() {
    // Maintains MQTT connection if disconnected
    if (!mqttClient.connected()) {
        reconnectMQTT();
    }

    mqttClient.poll();

    static unsigned long lastMsg = 0;
    unsigned long now = millis();

    // Publishes sensor data every 10 seconds
    if (now - lastMsg > 10000) {
        lastMsg = now;

        // Reads BMP085 sensor (pressure and temperature)
        sensors_event_t event;
        bmp.getEvent(&event);

        // Process if pressure reading is valid
        if (event.pressure) {
            float temperature = 0;
            bmp.getTemperature(&temperature);
            float pressure = event.pressure / 100.0; // Converts pascals to hPa

            // Reads DS18B20 temperature sensor
            String payload = "Temperature: " + String(temperature) + " C, Pressure: " + String(pressure) + " hPa";
            Serial.println("Publishing message: " + payload);

            ds18b20.requestTemperatures();
            float tempDS18B20 = ds18b20.getTempCByIndex(0);
            payload += ", DS18B20 Temp: " + String(tempDS18B20) + " C";

            Serial.println("Publishing message: " + payload);

            // Publishes combined sensor readings to MQTT
            if (mqttClient.beginMessage(mqtt_topic)) {
                mqttClient.print(payload);
                mqttClient.endMessage();
            } else {
                Serial.println("Failed to publish message");
            }
        }
    }
}

// Establishes Wi-Fi connection with visual feedback
void connectToWiFi() {
    Serail.println();
    Serial.print("Connecting to WiFi...");
    
    WiFi.begin(ssid, password);

    // Wait for connection with dot visual feedback
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serail.println();
    Serial.println("Connected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// Attempts to reconnect to MQTT broker with connection status feedback
void reconnectMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (mqttClient.connect(mqtt_server, mqtt_port)) {
            Serial.println("connected");
        } else {
            Serial.print("failed, rc:");
            Serial.print(mqttClient.connectError());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}