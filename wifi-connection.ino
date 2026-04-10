// Establishes Wi-Fi connection on startup with retry logic. Button on GPIO 0 can disconnect.

#include <WiFi.h>

const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

int btnGPIO = 0; // Button pin (GPI0 - typically BOOT button)
int btnState = false; // Current button state

void setup() {
    Serial.begin(115200);
    delay(10);

    // Configures button GPIO as input
    pinMode(btnGPIO, INPUT);

    // Prints connection banner
    Serial.println();
    Serial.print("[WiFi] Connecting to ");
    Serial.println(ssid);

    // Connection retry parameters
    int tryDelay = 500; // Waits 500ms between status checks
    int numberOfTries = 20; // Tries 20 times before stopping

    // Connection loop - attempts to connect to Wi-Fi
    while (true) {
        switch(WiFi.status()) {
            // SSID not found in available networks
            case WL_NO_SSID_AVAIL:
                Serial.println("[WiFi] SSID not found");
                break;
            // Wi-Fi connection attempt failed
            case WL_CONNECT_FAILED:
                Serial.println("[WiFi] Failed to connect!");
                return;
                break;
            //Connection was lost
            case WL_CONNECITON_LOST:
                Serial.println("[WiFi] Connection was lost");
                break;
            // Not currently connected
            case WL_DISCONNECTED:
                Serial.println("[WiFi] Disconnected");
                break;
            // Successfully connected to Wi-Fi
            case WL_CONNECTED:
                Serial.println("[WiFi] WiFi is connected!");
                Serial.print("[WiFi] IP address: ");
                Serial.println(WiFi.localIP());
                return;
                break;
            // Unknown status
            default:
                Serial.println("[WiFi] WiFI status:");
                Serial.println(WiFi.status());
                break;        
            }
            delay(tryDelay);

            // Checks if retry attempts have been exceeded
            if (numberOfTries <= 0) {
                Serial.println("[WiFi] Failed to connect after multiple attempts");
                return;
            } else {
                numberOfTries--;
            }
        }
}

void loop() {
    // Monitors button state (LOW = pressed)
    btnState = digitalRead(btnGPIO);

    // Disconnects from Wi-Fi if button is pressed
    if (btnState == LOW) {
        Serial.println("[WiFi] Disconnecting WiFi...");
        // Disconnect parameters
        if (WiFi.disconnect(true, false)) {
            Serial.println("[WiFi] Disconnected from WiFi!");
        }
        delay(1000); // Debounce delay before accepting another button press
    }
}