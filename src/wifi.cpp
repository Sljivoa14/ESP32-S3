/*

#include "wifi.h"
#include <WiFi.h>

// ==============================
// Wi-Fi credentials
// ==============================

const char* SSID = "your wifi";
const char* PASSWORD = "YOUR_PASSWORD";

// ==============================
// Wi-Fi initialization
// ==============================

void initWiFi() {

    Serial.println();
    Serial.println("==============================");
    Serial.println("      INITIALIZING WIFI");
    Serial.println("==============================");

    WiFi.mode(WIFI_STA);

    // Useful for some routers
    WiFi.setSleep(false);

    Serial.printf("Connecting to: %s\n", SSID);

    WiFi.begin(SSID, PASSWORD);

    int attempts = 0;

    while (WiFi.status() != WL_CONNECTED && attempts < 30) {

        delay(500);
        Serial.print(".");

        attempts++;
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {

        Serial.println();
        Serial.println("==============================");
        Serial.println("       WIFI CONNECTED!");
        Serial.println("==============================");

        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

    } else {

        Serial.println();
        Serial.println("==============================");
        Serial.println("   WIFI CONNECTION FAILED");
        Serial.println("==============================");

        Serial.printf(
            "Final status: %d\n",
            WiFi.status()
        );
    }
}

bool isWiFiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String getIPAddress() {
    if (!isWiFiConnected()) {
        return "0.0.0.0";
    }

    return WiFi.localIP().toString();
}

*/