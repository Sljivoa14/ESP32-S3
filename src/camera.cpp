/*
#include "camera.h";
#include "img_converters.h";

//============================================
//             OV3660 PIN CONFIG
//============================================


#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1

#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       11
#define Y4_GPIO_NUM       10
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM        8

#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM     13

// ========================================
// CAMERA INITIALIZATION
// ========================================

bool initCamera() {

    camera_config_t config = {};

    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    // Data pins
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;

    // Control
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;

    // SCCB
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;

    // Clock
    config.xclk_freq_hz = 20000000;

    // Known working format
    config.pixel_format = PIXFORMAT_RGB565;

    // Start small
    config.frame_size = FRAMESIZE_QQVGA;

    // One frame buffer
    config.fb_count = 1;

    // Use our confirmed 8 MB PSRAM
    config.fb_location = CAMERA_FB_IN_PSRAM;

    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

    Serial.println();
    Serial.println("==============================");
    Serial.println("     INITIALIZING CAMERA");
    Serial.println("==============================");

    esp_err_t result = esp_camera_init(&config);

    if (result != ESP_OK) {

        Serial.printf(
            "Camera initialization FAILED: 0x%x\n",
            result
        );

        return false;
    }

    Serial.println("Camera initialized successfully!");

    printCameraInfo();

    return true;
}


// ========================================
// CAMERA INFORMATION
// ========================================

void printCameraInfo() {

    sensor_t* sensor = esp_camera_sensor_get();

    if (!sensor) {
        Serial.println("Could not get camera sensor.");
        return;
    }

    Serial.printf(
        "Sensor PID: 0x%02X\n",
        sensor->id.PID
    );

    Serial.printf(
        "Frame size: QQVGA (160x120)\n"
    );

    Serial.println(
        "Pixel format: RGB565"
    );

    Serial.printf(
        "Free PSRAM: %u bytes\n",
        ESP.getFreePsram()
    );

    Serial.printf(
        "Free heap: %u bytes\n",
        ESP.getFreeHeap()
    );
}


// ========================================
// CAPTURE + CONVERT TO JPEG
// ========================================

bool captureJpeg(
    uint8_t** jpgBuffer,
    size_t* jpgLength
) {

    if (jpgBuffer == nullptr || jpgLength == nullptr) {
        return false;
    }

    *jpgBuffer = nullptr;
    *jpgLength = 0;

    Serial.println();
    Serial.println("Capturing RGB565 frame...");

    camera_fb_t* fb = esp_camera_fb_get();

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

        return false;
    }

    Serial.printf(
        "RGB565 FRAME: %ux%u | %u bytes\n",
        fb->width,
        fb->height,
        fb->len
    );

    // RGB565 -> JPEG
    bool converted = fmt2jpg(
        fb->buf,
        fb->len,
        fb->width,
        fb->height,
        PIXFORMAT_RGB565,
        30,
        jpgBuffer,
        jpgLength
    );

    // Return camera buffer immediately
    esp_camera_fb_return(fb);

    if (!converted || *jpgBuffer == nullptr) {

        Serial.println(
            "RGB565 -> JPEG CONVERSION FAILED"
        );

        return false;
    }

    Serial.printf(
        "JPEG CREATED: %u bytes\n",
        *jpgLength
    );

    return true;
}

*/