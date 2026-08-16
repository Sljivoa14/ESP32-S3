#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"

// ========================================
// WIFI
// ========================================

const char* SSID = "your wifi";
const char* PASSWORD = "your passsord";

WebServer server(80);

// ========================================
// OV3660 PINS
// ========================================

#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1

#define XCLK_GPIO_NUM    15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5

#define Y9_GPIO_NUM      16
#define Y8_GPIO_NUM      17
#define Y7_GPIO_NUM      18
#define Y6_GPIO_NUM      12
#define Y5_GPIO_NUM      11
#define Y4_GPIO_NUM      10
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       8

#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM    13


// ========================================
// START CAMERA
// ========================================

bool startCamera() {

    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    // Camera data pins
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    // Camera control pins
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

    // SCCB
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    // Camera clock
    config.xclk_freq_hz = 20000000;

    // ========================================
    // IMPORTANT
    // RGB565 is the mode that we KNOW works
    // on your OV3660.
    // ========================================

    config.pixel_format = PIXFORMAT_RGB565;

    config.frame_size = FRAMESIZE_QQVGA;

    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    Serial.println();
    Serial.println("Initializing camera...");

    esp_err_t result = esp_camera_init(&config);

    if (result != ESP_OK) {

        Serial.printf(
            "Camera initialization FAILED: 0x%x\n",
            result
        );

        return false;
    }

    Serial.println("Camera initialized!");

    // ========================================
    // SENSOR INFORMATION
    // ========================================

    sensor_t* sensor = esp_camera_sensor_get();

    if (sensor) {

        Serial.printf(
            "Sensor PID: 0x%02X\n",
            sensor->id.PID
        );

        // Explicitly keep RGB565
        sensor->set_pixformat(
            sensor,
            PIXFORMAT_RGB565
        );

        // Explicitly keep QQVGA
        sensor->set_framesize(
            sensor,
            FRAMESIZE_QQVGA
        );
    }

    return true;
}


// ========================================
// CAPTURE IMAGE
// ========================================

void captureImage() {

    Serial.println();
    Serial.println("================================");
    Serial.println("Capturing RGB565 frame...");
    Serial.println("================================");

    camera_fb_t* fb = esp_camera_fb_get();

    // ========================================
    // CAMERA CAPTURE FAILED
    // ========================================

    if (!fb) {

        Serial.println("CAMERA CAPTURE FAILED");

        Serial.printf(
            "Free PSRAM: %u\n",
            ESP.getFreePsram()
        );

        Serial.printf(
            "Free heap: %u\n",
            ESP.getFreeHeap()
        );

        server.send(
            500,
            "text/plain",
            "Camera capture failed"
        );

        return;
    }

    // ========================================
    // FRAME INFORMATION
    // ========================================

    Serial.printf(
        "RGB565 FRAME: %ux%u | %u bytes\n",
        fb->width,
        fb->height,
        fb->len
    );

    Serial.printf(
        "Free PSRAM: %u\n",
        ESP.getFreePsram()
    );


    // ========================================
    // RGB565 -> JPEG
    // ========================================

    uint8_t* jpg_buf = nullptr;
    size_t jpg_len = 0;

    bool converted = fmt2jpg(
        fb->buf,
        fb->len,
        fb->width,
        fb->height,
        PIXFORMAT_RGB565,
        30,
        &jpg_buf,
        &jpg_len
    );

    // Return camera buffer immediately
    esp_camera_fb_return(fb);

    // ========================================
    // CONVERSION FAILED
    // ========================================

    if (!converted || jpg_buf == nullptr) {

        Serial.println(
            "RGB565 -> JPEG CONVERSION FAILED"
        );

        server.send(
            500,
            "text/plain",
            "JPEG conversion failed"
        );

        return;
    }


    // ========================================
    // JPEG SUCCESS
    // ========================================

    Serial.printf(
        "JPEG CREATED: %u bytes\n",
        jpg_len
    );


    // ========================================
    // SEND JPEG TO BROWSER
    // ========================================

    server.setContentLength(jpg_len);

    server.send(
        200,
        "image/jpeg",
        ""
    );

    WiFiClient client = server.client();

    client.write(
        jpg_buf,
        jpg_len
    );

    // Free converted JPEG
    free(jpg_buf);

    Serial.println("JPEG SENT!");
    Serial.println("================================");
}


// ========================================
// WEB PAGE
// ========================================

void homePage() {

    String html = R"rawliteral(
<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<title>ESP32 Camera</title>

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
    image-rendering: auto;
    border: 2px solid white;
    display: block;
    margin: 20px auto;
}

button {
    display: block;
    margin: 20px auto;
    padding: 12px 25px;
    font-size: 18px;
    cursor: pointer;
}

</style>

</head>

<body>

<h1>ESP32-S3 OV3660</h1>

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
// SETUP
// ========================================

void setup() {

    Serial.begin(115200);

    delay(2000);

    Serial.println();
    Serial.println("================================");
    Serial.println(" ESP32-S3 OV3660 CAMERA SERVER");
    Serial.println("================================");


    // ========================================
    // PSRAM
    // ========================================

    Serial.printf(
        "PSRAM: %s\n",
        psramFound() ? "YES" : "NO"
    );

    Serial.printf(
        "PSRAM size: %u\n",
        ESP.getPsramSize()
    );

    Serial.printf(
        "Free PSRAM: %u\n",
        ESP.getFreePsram()
    );


    // ========================================
    // WIFI
    // ========================================

    Serial.println();
    Serial.println("Initializing Wi-Fi...");
    Serial.printf(
        "Connecting to: %s\n",
        SSID
    );

    WiFi.mode(WIFI_STA);

    WiFi.begin(
        SSID,
        PASSWORD
    );

    int attempts = 0;

    while (
        WiFi.status() != WL_CONNECTED &&
        attempts < 30
    ) {

        delay(500);

        Serial.print(".");

        attempts++;
    }

    Serial.println();


    // ========================================
    // WIFI FAILED
    // ========================================

    if (WiFi.status() != WL_CONNECTED) {

        Serial.println();
        Serial.println("================================");
        Serial.println(" WIFI CONNECTION FAILED");
        Serial.println("================================");

        Serial.printf(
            "Final status: %d\n",
            WiFi.status()
        );

        return;
    }

    // ========================================
    // WIFI SUCCESS
    // ========================================

    Serial.println();
    Serial.println("================================");
    Serial.println("       WIFI CONNECTED!");
    Serial.println("================================");

    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    // ========================================
    // CAMERA
    // ========================================

    if (!startCamera()) {

        Serial.println(
            "Camera startup failed!"
        );

        return;
    }


    // ========================================
    // WEB SERVER ROUTES
    // ========================================

    server.on(
        "/",
        HTTP_GET,
        homePage
    );

    server.on(
        "/capture",
        HTTP_GET,
        captureImage
    );


    // ========================================
    // START SERVER
    // ========================================

    server.begin();

    Serial.println();
    Serial.println("================================");
    Serial.println("       HTTP SERVER STARTED");
    Serial.println("================================");

    Serial.print("OPEN: http://");        // server port HTTP
    Serial.println(WiFi.localIP());

    Serial.println();
    Serial.println("Ready!");
}


// ========================================
// LOOP
// ========================================

void loop() {

    server.handleClient();

    delay(2);
}
