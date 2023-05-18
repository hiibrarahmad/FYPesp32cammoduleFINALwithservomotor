#include "esp_camera.h"
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include <ezButton.h>
#include <Servo.h>

// WARNING!!! Make sure that you have either selected ESP32 Wrover Module,
//            or another board which has PSRAM enabled
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER

#include "camera_pins.h"
ezButton button(14);
const int servoPin = 12; // Pin for the servo

const char* ssid = "ibrar";
const char* password = "ibrarahmad";

const char* websockets_server_host = "192.168.4.1";
const uint16_t websockets_server_port = 8888;

Servo servo;
bool rotateServo = false;       // Flag to indicate servo rotation
unsigned long rotationStartTime; // Time when rotation started

using namespace websockets;
WebsocketsClient client;

void setup() {
  Serial.begin(115200);
  button.setDebounceTime(50); // set debounce time to 50 milliseconds
  Serial.setDebugOutput(true);
  Serial.println();
  servo.attach(servoPin);

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
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }


  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");

  while(!client.connect(websockets_server_host, websockets_server_port, "/")){
    delay(500);
    Serial.print(".");
  }

  Serial.println("Socket Connected!");  

  client.onMessage([](WebsocketsMessage msg) {
    Serial.println("Got Message: " + msg.data());

    // Check if the message is "Button pressed!"
    if (msg.data() == "Button pressed!") {
      Serial.println("Rotating servo...");
      rotateServo = true;
      rotationStartTime = millis(); // Record the start time of rotation
    }

    // Send echo message
    client.send("Echo: " + msg.data());
  });
}

void loop() {

  button.loop(); // MUST call the loop() function first

  if(button.isPressed())
    Serial.println("The button olddddd");

  if(button.isReleased())
    Serial.println("The buttonoldddd");

     if (rotateServo) {
    // Check if 10 seconds have passed since rotation started
    if (millis() - rotationStartTime >= 10000) {
      Serial.println("Returning servo to initial position...");
      servo.write(0); // Set the servo to the initial position
      rotateServo = false; // Reset the flag
    } else {
      // Rotate the servo to the desired position
      servo.write(90);
    }
  }

  client.poll();

  delay(100);

    
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  fb = esp_camera_fb_get();
  if(!fb){
    Serial.println("Camera capture failed");
    esp_camera_fb_return(fb);
    return;
  }

  size_t fb_len = 0;
  if(fb->format != PIXFORMAT_JPEG){
    Serial.println("Non-JPEG data not implemented");
    return;
  }

  client.sendBinary((const char*) fb->buf, fb->len);



  
  esp_camera_fb_return(fb);
}
