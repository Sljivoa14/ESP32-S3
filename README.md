
# Edge AI Face analysis project
 
# ESP32-S3 + OV3660 Camera Server

A from-scratch PlatformIO/Arduino project turning a Freenove ESP32-S3-N16R8 board (16 MB flash, 8 MB PSRAM) with an OV3660 camera into a Wi-Fi-connected HTTP camera server, viewable from a browser.

## Hardware

- **Board:** Freenove ESP32-WROVER-CAM, actual chip = ESP32-S3-N16R8 (16 MB Flash, 8 MB PSRAM, WiFi, BLE)
- **Camera:** OV3660, connected via onboard FPC ribbon connector (no manual wiring needed)
- **USB-to-serial adapter:** CP2102 six-in-one module — needed for any *bare* ESP32/ESP32-CAM boards without onboard USB (not required for this Freenove board, which has USB built in)

## Status: ✅ Working end-to-end

```
OV3660 → RGB565 capture → ESP32-S3 (PSRAM) → JPEG conversion → WiFi → HTTP server → Browser
```

## Timeline / what we solved

### 1. PlatformIO + board bring-up
Base config:
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
```
First upload succeeded, confirmed via esptool: `Features: WiFi, BLE, Embedded PSRAM 8MB`.

### 2. PSRAM not detected (major blocker)
Arduino reported `PSRAM found: NO` / `0 bytes` despite the chip having 8 MB. Fixed with explicit OPI PSRAM config:
```ini
board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216
board_build.flash_mode = qio
board_build.arduino.memory_type = qio_opi
board_build.psram_type = opi
build_flags = -DBOARD_HAS_PSRAM
monitor_speed = 115200
```
After this: `PSRAM found: YES`, `PSRAM size: 8386279 bytes`.

### 3. Camera detection
OV3660 detected successfully: `Sensor PID: 0x3660`.

Pin mapping used (Freenove ESP32-S3 camera board):
```cpp
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5
#define Y9_GPIO_NUM 16
#define Y8_GPIO_NUM 17
#define Y7_GPIO_NUM 18
#define Y6_GPIO_NUM 12
#define Y5_GPIO_NUM 11
#define Y4_GPIO_NUM 10
#define Y3_GPIO_NUM 9
#define Y2_GPIO_NUM 8
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
```

### 4. JPEG capture kept failing / crashing
Direct `PIXFORMAT_JPEG` capture consistently failed (`Failed to capture frame!`), and one attempt caused a `Guru Meditation Error — Stack canary watchpoint triggered (cam_task)`.

**Fix:** switched to `PIXFORMAT_RGB565` at `FRAMESIZE_QQVGA` (160×120) — this worked reliably and repeatedly:
```
FRAME CAPTURED!
Width: 160, Height: 120, Format: 0
Frame size: 38400 bytes  (= 160×120×2, correct for RGB565)
```
JPEG is now produced on-device from the RGB565 buffer using `fmt2jpg()` from `img_converters.h`, rather than relying on the sensor's native JPEG mode.

### 5. Wi-Fi wouldn't connect to home router
`net_2032` (WPA2/WPA3 mixed, encryption type 7, weak signal ~-76 to -81 dBm) consistently returned `WL_CONNECT_FAILED` (status 4), even though the ESP32 could see it fine in scans.

**Workaround:** connected to an iPhone Personal Hotspot instead (2.4 GHz, "Maximize Compatibility" enabled) — connected immediately and reliably (`WIFI CONNECTED!`, strong RSSI around -15 to -30 dBm).

Likely cause on the router side: WPA2/WPA3 mixed-mode compatibility issue combined with weak signal strength. Not resolved — routed around it for now.

### 6. Wi-Fi + Camera didn't work together at first
Standalone Wi-Fi test worked, standalone camera test worked, but combining them into one program caused Wi-Fi to fail (`status: 6`, stuck disconnected). Fixed by:
- Connecting Wi-Fi **before** initializing the camera (not after)
- Using a simpler/cleaner Wi-Fi init sequence (avoiding aggressive `WiFi.disconnect(true, true)` NVS erase)

### 7. Final working architecture
- ESP32 connects to Wi-Fi (iPhone hotspot) first
- Then initializes camera in RGB565/QQVGA (proven-stable config)
- `WebServer` on port 80 serves:
  - `/` → simple HTML page with an `<img>` tag and Capture button
  - `/capture` → grabs RGB565 frame, converts to JPEG via `fmt2jpg()`, streams JPEG bytes to browser
- Laptop connects to the same iPhone hotspot and loads `http://<esp32-ip>` (e.g. `http://172.20.10.10`)

**Result confirmed working:**
```
RGB565 FRAME: 160x120 | 38400 bytes
JPEG CREATED: XXXXX bytes
JPEG SENT!
```

## Known-good camera config (don't touch without reason)
```cpp
config.pixel_format = PIXFORMAT_RGB565;
config.frame_size = FRAMESIZE_QQVGA;
config.fb_count = 1;
config.fb_location = CAMERA_FB_IN_PSRAM;
config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
```

## Known-good platformio.ini
```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

board_upload.flash_size = 16MB
board_upload.maximum_size = 16777216

board_build.flash_mode = qio
board_build.arduino.memory_type = qio_opi
board_build.psram_type = opi

build_flags =
    -DBOARD_HAS_PSRAM

monitor_speed = 115200
```

## Next steps (roadmap)

- [ ] **Stabilize capture** — verify repeated captures don't leak PSRAM or crash over time; test higher resolutions once QQVGA is rock solid
- [ ] **Live streaming** instead of click-to-capture — MJPEG-style multipart HTTP stream so the browser shows continuous video, not single frames
- [ ] **Better web UI** — live preview, FPS counter, resolution/quality display
- [ ] **Camera controls** — brightness, contrast, saturation, flip (h/v), exposure, white balance via `sensor_t` setters
- [ ] **Photo capture/download** — save JPEG snapshots, later maybe to SD card
- [ ] **Fix or replace home Wi-Fi connectivity** — currently relies on iPhone hotspot; investigate router's WPA2/WPA3 mixed-mode settings or use a dedicated 2.4GHz WPA2-only SSID
- [ ] **Feed into ESPICIEN** — this camera module is meant to become the vision input for the ESPICIEN personal AI assistant (face/object recognition pipeline)
- [ ] **Eventually: on-device inference** — leverage the 8 MB PSRAM for lightweight edge-AI (motion detection, simple object detection) directly on the ESP32-S3

## Useful PlatformIO commands
- **Clean** → wipes build artifacts, use if a build error looks like a stale/corrupted toolchain state
- **Build** → compile only
- **Upload** → builds (if needed) + flashes over USB
- **Monitor** → opens serial monitor at 115200 baud

Normal day-to-day workflow: edit → `Ctrl+S` → Upload → Monitor. No need to Clean every time.
