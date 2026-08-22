
# Edge AI Face analysis project
 
# ESP32-S3 + OV3660 Camera Server

A from-scratch PlatformIO(VS code extension)/Arduino project turning a Freenove ESP32-S3-N16R8 board (16 MB flash, 8 MB PSRAM) with an OV3660 camera into a Wi-Fi-connected HTTP camera server, viewable from a browser.

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

# ESP32-S3-N16R8 + OV3660 Edge AI Face Recognition

## Project Goal

Build an ESP32-S3-N16R8 camera system using the OV3660 that:

1. Connects to a normal Wi-Fi router.
2. Serves a web interface over HTTP.
3. Captures images from the OV3660.
4. Streams the camera over the local network.
5. Runs face detection locally on the ESP32.
6. Eventually performs face recognition locally as Edge AI.
7. Displays the camera and AI results through a browser.

The long-term objective is to keep the AI inference on the ESP32 rather than sending camera images to a cloud service.

---

# Hardware

- ESP32-S3-N16R8
  - 16 MB Flash
  - 8 MB PSRAM
- Freenove ESP32-WROVER CAM
- OV3660 camera
- USB connection to PC
- Wi-Fi router

The 8 MB PSRAM has already been confirmed working.

---

# Current Development Environment

- VS Code
- PlatformIO
- Arduino framework
- ESP32-S3
- PlatformIO environment:

```ini
[env:esp32-s3-devkitc-1]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino
monitor_speed = 115200
```

---

# Current Status

## Completed

- [x] VS Code installed
- [x] PlatformIO installed
- [x] ESP32-S3 project created
- [x] ESP32-S3 successfully compiled
- [x] ESP32-S3 successfully uploaded
- [x] Serial monitor working at 115200
- [x] ESP32-S3-N16R8 hardware identified
- [x] 8 MB PSRAM confirmed
- [x] OV3660 detected
- [x] Camera initialization succeeded
- [x] Phone hotspot successfully connected to ESP32
- [x] ESP32 HTTP server successfully starts
- [x] ESP32 receives an IP address

## Current Blocker

The ESP32 connects successfully to the phone hotspot but fails to associate with the normal Wi-Fi router.

Current router-side Wi-Fi test:

```text
2.4 GHz
802.11n / Wi-Fi 4
WPA2-Personal
```

The ESP32 currently reports:

```text
WiFi disconnected. Reason: 202
Final status: 4
```

This indicates an association failure.

Do not modify the camera code while troubleshooting this. The network problem must be solved first.

---

# PHASE 1 — Wi-Fi Networking

## Step 1 — Connect ESP32 to the normal router

Update the local Wi-Fi credentials:

```cpp
const char* SSID = "YOUR_WIFI_NAME";
const char* PASSWORD = "YOUR_WIFI_PASSWORD";
```

Never publish the real password.

The ESP32 should eventually print:

```text
WIFI CONNECTED!
IP address: 192.168.1.xxx
```

## Step 2 — Confirm the PC and ESP32 are on the same LAN

Example:

```text
Router
├── PC     192.168.1.x
└── ESP32  192.168.1.x
```

## Step 3 — Ping the ESP32

From Windows CMD:

```cmd
ping ESP32_IP
```

Expected:

```text
Reply from ESP32_IP
```

## Step 4 — Open the HTTP server

In a browser:

```text
http://ESP32_IP
```

The ESP32 camera page should appear.

---

# PHASE 2 — OV3660 Camera Capture

Once HTTP networking works, test the camera.

The capture endpoint should be:

```text
/capture
```

The browser should show:

```text
ESP32-S3 OV3660

[ camera image ]

[ CAPTURE ]
```

Clicking CAPTURE should produce serial output similar to:

```text
Capturing RGB565 frame...
RGB565 FRAME: 160x120 | XXXXX bytes
JPEG CREATED: XXXXX bytes
JPEG SENT!
```

If capture fails, debug the camera before adding AI.

---

# PHASE 3 — Reliable Camera Pipeline

The target pipeline is:

```text
OV3660
  ↓
Camera frame
  ↓
PSRAM frame buffer
  ↓
RGB565 / JPEG
  ↓
HTTP
  ↓
Browser
```

Test and optimize:

- QQVGA
- QVGA
- RGB565
- JPEG
- PSRAM frame buffers
- Frame buffer count
- JPEG quality
- Frame rate
- Free heap
- Free PSRAM

Do not increase resolution until lower-resolution capture is reliable.

---

# PHASE 4 — Live Camera Streaming

Replace the single-image CAPTURE workflow with a live local stream.

Target architecture:

```text
OV3660
   ↓
JPEG frames
   ↓
HTTP/MJPEG stream
   ↓
Browser
```

The browser should continuously display the camera instead of requiring a CAPTURE button.

---

# PHASE 5 — Edge AI Face Detection

Only after the camera is reliable should AI be added.

The first AI milestone is **face detection**, not recognition.

Target:

```text
Camera
  ↓
Frame
  ↓
Resize / preprocessing
  ↓
AI model
  ↓
Face detection
  ↓
YES / NO
```

Example result:

```text
Face detected: YES
Confidence: 92%
```

Start with low-resolution input because embedded AI has limited CPU and memory resources.

---

# PHASE 6 — Face Recognition

Face detection and face recognition are different.

## Detection

Answers:

> Is there a face in this image?

```text
Camera
  ↓
Face detector
  ↓
Face found
```

## Recognition

Answers:

> Which enrolled person does this face most closely match?

```text
Camera
  ↓
Face detection
  ↓
Face crop
  ↓
Preprocessing
  ↓
Face feature / embedding model
  ↓
Compare with enrolled identities
  ↓
Identity + similarity/confidence
```

The recognition system must be designed carefully to avoid confusing similar faces.

---

# PHASE 7 — Edge AI Optimization

Once inference works, optimize:

- Model size
- Input resolution
- Quantization
- Inference latency
- RAM usage
- PSRAM usage
- CPU usage
- Frame rate
- False positives
- False negatives
- Power consumption

The 8 MB PSRAM is especially useful for camera buffers and ML workloads.

---

# Final Target Architecture

```text
                 ESP32-S3-N16R8
              ┌───────────────────┐
              │                   │
              │      OV3660       │
              │         ↓         │
              │   Image Capture   │
              │         ↓         │
              │  Preprocessing    │
              │         ↓         │
              │   Face Detection  │
              │         ↓         │
              │ Face Recognition  │
              │         ↓         │
              │      Result       │
              │                   │
              └─────────┬─────────┘
                        │
                       Wi-Fi
                        │
              ┌─────────▼─────────┐
              │   Web Interface   │
              │                   │
              │ Camera + Results  │
              └───────────────────┘
```

---

# Recommended Software Architecture

Keep the ESP32 project primarily in C++.

Example:

```text
ESP32-Edge-AI/
│
├── platformio.ini
│
├── src/
│   ├── main.cpp
│   ├── camera.cpp
│   ├── camera.h
│   ├── wifi.cpp
│   ├── wifi.h
│   ├── web_server.cpp
│   ├── web_server.h
│   ├── ai.cpp
│   └── ai.h
│
├── include/
│
├── lib/
│
└── README.md
```

Do not create a Python AI file just because we are adding AI.

For true **on-device Edge AI**, the inference code should run on the ESP32. That means C/C++ and an embedded ML runtime/model are the main path.

Python can still be useful on the PC for:

- Preparing/training a model
- Converting a trained model
- Creating a dataset
- Testing recognition algorithms
- Evaluating accuracy
- Generating embeddings if we choose a PC-assisted architecture

But the final ESP32 inference path should be independent of Python if the goal is fully local Edge AI.

---

# Python vs ESP32 AI

## Fully on-device Edge AI

```text
OV3660
  ↓
ESP32
  ↓
ML inference
  ↓
Recognition
```

No Python runtime is required on the ESP32.

## PC-assisted AI

```text
OV3660
  ↓
ESP32
  ↓
Wi-Fi
  ↓
Python program on PC
  ↓
AI inference
  ↓
Result
```

This is easier initially but is not fully Edge AI.

For this project, prefer the first architecture once the basic camera system works.

---

# Suggested Development Order

Do not skip stages.

```text
1. Wi-Fi connection
       ↓
2. ESP32 IP address
       ↓
3. PC can ping ESP32
       ↓
4. HTTP server reachable
       ↓
5. OV3660 capture
       ↓
6. JPEG capture
       ↓
7. Live camera stream
       ↓
8. Face detection
       ↓
9. Face recognition
       ↓
10. Edge AI optimization
```

This layered approach makes debugging much easier because each subsystem is validated before the next one is introduced.

---

# Cybersecurity / Engineering Notes

The project naturally touches several useful cybersecurity concepts:

- Wi-Fi authentication
- IP addressing
- TCP/IP
- HTTP
- Local network security
- Device discovery
- Embedded attack surfaces
- Secure firmware
- Input validation
- Authentication for the web interface
- Protecting camera data
- Model/data privacy
- Edge processing versus cloud processing

A future version should not leave the camera HTTP endpoint openly accessible to every device on the LAN. Once the prototype works, add appropriate authentication and access controls.

---

# Immediate Next Step

When the normal router is available again:

1. Fix ESP32 association with the router.
2. Confirm `WIFI CONNECTED`.
3. Get the ESP32's `192.168.1.x` IP.
4. Ping it from the PC.
5. Open the HTTP server.
6. Test OV3660 capture.
7. Only then begin the live stream.
8. Then integrate face detection.
9. Finally build face recognition and optimize the model for the ESP32-S3.