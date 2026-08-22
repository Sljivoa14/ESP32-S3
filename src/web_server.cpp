/*

#include "web_server.h"
#include "camera.h"
#include "wifi.h"

#include <WebServer.h>

WebServer server(80);


// ========================================
// HOME PAGE
// ========================================

void handleHome() {

    String html = R"rawliteral(
<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<title>ESP32 Edge AI</title>

<style>

body {
    background: #111;
    color: white;
    font-family: Arial;
    text-align: center;
    padding: 30px;
}

img {
    width: 320px;
    height: 240px;
    border: 2px solid white;
    display: block;
    margin: 20px auto;
}

button {
    padding: 12px 25px;
    font-size: 18px;
    cursor: pointer;
}

</style>

</head>

<body>

<h1>ESP32-S3 Edge AI</h1>

<h2>OV3660 Camera</h2>

<img id="camera">

<button onclick="capture()">
CAPTURE
</button>

<script>

function capture() {

    document.getElementById("camera").src =
        "/capture?t=" + Date.now();

}

</script>

</body>

</html>
)rawliteral";

    server.send(
        200,
        "text/html",
        html
    );
}


// ========================================
// CAPTURE ENDPOINT
// ========================================

void handleCapture() {

    uint8_t* jpgBuffer = nullptr;
    size_t jpgLength = 0;

    bool success = captureJpeg(
        &jpgBuffer,
        &jpgLength
    );

    if (!success) {

        server.send(
            500,
            "text/plain",
            "Camera capture failed"
        );

        return;
    }

    server.setContentLength(jpgLength);

    server.send(
        200,
        "image/jpeg",
        ""
    );

    WiFiClient client = server.client();

    client.write(
        jpgBuffer,
        jpgLength
    );

    free(jpgBuffer);

    Serial.println("JPEG SENT!");
}


// ========================================
// SERVER INITIALIZATION
// ========================================

void initWebServer() {

    if (!isWiFiConnected()) {

        Serial.println(
            "Web server NOT started: Wi-Fi unavailable."
        );

        return;
    }

    server.on(
        "/",
        HTTP_GET,
        handleHome
    );

    server.on(
        "/capture",
        HTTP_GET,
        handleCapture
    );

    server.begin();

    Serial.println();
    Serial.println("==============================");
    Serial.println("      HTTP SERVER STARTED");
    Serial.println("==============================");

    Serial.print(
        "OPEN: http://"
    );

    Serial.println(
        getIPAddress()
    );

    Serial.println("Ready!");
}


// ========================================
// SERVER LOOP
// ========================================

void handleWebServer() {

    if (isWiFiConnected()) {
        server.handleClient();
    }
}

*/