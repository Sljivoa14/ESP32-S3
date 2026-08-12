#include <Arduino.h>
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);

    Serial.println();
    Serial.println("=== WIFI SCAN ===");

    int count = WiFi.scanNetworks();

    for (int i = 0; i < count; i++) {

        Serial.print(i + 1);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));

        Serial.print(" | RSSI: ");
        Serial.print(WiFi.RSSI(i));

        Serial.print(" | Channel: ");
        Serial.print(WiFi.channel(i));

        Serial.print(" | Encryption: ");
        Serial.println((int)WiFi.encryptionType(i));
    }
}

void loop() {
    delay(1000);
}