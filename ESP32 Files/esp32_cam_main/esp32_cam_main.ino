#include <WiFi.h>
#include <WebServer.h>
#include <esp_camera.h>
#include <HardwareSerial.h>

// Replace with your network credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// GPS Configuration (HardwareSerial - UART2)
HardwareSerial gpsSerial(2);  // Use Serial2 (default pins: RX2=16, TX2=17)
const int BAUD_RATE_GPS = 9600;

// Camera configuration (adjust according to your ESP32-CAM model)
#define PWDN_GPIO_NUM     32  // Power down (may not be needed, check schematic)
#define RESET_GPIO_NUM    -1  // -1 if not used, 1 for some modules
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

// Global variables to store GPS data
String gpsData = "Waiting for GPS fix...";
bool gpsDataValid = false;

// Function to capture and send a JPEG image
void handleJpg() {
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  // Capture a picture
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  // Send the JPEG image as a response
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Content-Length", String(fb->len));
  server.sendHeader("Cache-Control", "no-cache");  // Prevent caching
  server.send_P(200, "image/jpeg", (const uint8_t*)fb->buf, fb->len);

  esp_camera_fb_return(fb);
}

// Function to parse GPS data (NMEA sentences)  - Very basic parsing!
void parseGPSData() {
    if (gpsSerial.available() > 0) {
        String gpsRawData = gpsSerial.readStringUntil('\n');

        // Very basic NMEA parsing.  Extracts latitude and longitude from GPGGA sentence.
        // This is *highly simplified* and prone to issues if the GPS data isn't what's expected.
        // A robust solution would use a proper NMEA parsing library (like TinyGPS++).

        if (gpsRawData.startsWith("$GPGGA")) {
            int commaIndex[14]; // Indices of commas
            int commaCount = 0;

            for (int i = 0; i < gpsRawData.length() && commaCount < 14; i++) {
                if (gpsRawData[i] == ',') {
                    commaIndex[commaCount++] = i;
                }
            }

            if (commaCount >= 9) { // Ensure enough data fields

              // Extract latitude
              String latitudeStr = gpsRawData.substring(commaIndex[1] + 1, commaIndex[2]);
              String latitudeDir = gpsRawData.substring(commaIndex[2] + 1, commaIndex[3]);
              
              //Extract Longitude
              String longitudeStr = gpsRawData.substring(commaIndex[3] + 1, commaIndex[4]);
              String longitudeDir = gpsRawData.substring(commaIndex[4] + 1, commaIndex[5]);
              
              // Convert to decimal degrees (simplified - no error checking!)
              float latitude = latitudeStr.substring(0, 2).toFloat() + (latitudeStr.substring(2).toFloat() / 60.0);
              if (latitudeDir == "S") latitude = -latitude;

              float longitude = longitudeStr.substring(0, 3).toFloat() + (longitudeStr.substring(3).toFloat() / 60.0);
              if (longitudeDir == "W") longitude = -longitude;
               
              //Number of Satellites
              String satellites = gpsRawData.substring(commaIndex[6] + 1, commaIndex[7]);   

              //Altitude
              String altitudeStr = gpsRawData.substring(commaIndex[8] + 1, commaIndex[9]);  // Get altitude string
              String altitudeUnits = gpsRawData.substring(commaIndex[9] + 1, commaIndex[10]);
                
              gpsData = "Latitude: " + String(latitude, 6) + ", Longitude: " + String(longitude, 6) + ", Satellites: " + satellites + ", Altitude: " + altitudeStr + " " + altitudeUnits;
              gpsDataValid = true;
            } else {
               gpsData = "Waiting for GPS fix...";
                gpsDataValid = false;
            }
        }
    }
}


// Handles requests for the root URL (/) - sends HTML with GPS data
void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>ESP32-CAM with GPS</title>";
    html += "<meta http-equiv=\"refresh\" content=\"5\"></head>"; // Auto-refresh every 5 seconds
    html += "<body><h1>ESP32-CAM with GPS</h1>";
    html += "<p>GPS Data: " + gpsData + "</p>";
    html += "<img src=\"/jpg\" alt=\"Camera Image\" style=\"max-width:100%; height:auto;\">";  // Display image
    html += "</body></html>";
    server.send(200, "text/html", html);
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found");
}


void setup() {
  Serial.begin(115200);
  gpsSerial.begin(BAUD_RATE_GPS);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;  // Important: Set to JPEG

  // Frame size and quality (adjust as needed)
   if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA; // or other sizes
        config.jpeg_quality = 10;           // Quality (lower is better, 0-63)
        config.fb_count = 2;                // Use 2 frame buffers if PSRAM available
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }
  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    // Handle initialization failure (e.g., restart ESP32)
    delay(1000);
    ESP.restart(); //Or other error handling
    return;
  }

  // Route for serving the JPEG image
  server.on("/jpg", HTTP_GET, handleJpg);
  server.on("/", HTTP_GET, handleRoot);    // Handle root requests
  server.onNotFound(handleNotFound);

  // Start the web server
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  parseGPSData();
  delay(10); // Short delay
}
