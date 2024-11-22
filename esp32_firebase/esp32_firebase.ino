#include <SPI.h>
#include <mcp2515.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

// TokenHelper dan RTDBHelper untuk Firebase
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Network credentials
#define WIFI_SSID "UII Connect"
#define WIFI_PASSWORD "dezaprolink12"
#define RX_PIN 16
#define TX_PIN 17

// Firebase credentials
#define API_KEY "AIzaSyBDcVzwYzdNMusrz5RoYRxnn5b40l6kMh0"
#define USER_EMAIL "aswatama@gmail.com"
#define USER_PASSWORD "12345678"
#define DATABASE_URL "https://pinguin-10391-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Deklarasi fungsi
void readCANData(void * parameter);
void sendDataToFirebase(void * parameter);
void sendToFirebase(String path, int value, int timestamp);
void sendToNextion(HardwareSerial &nextionSerial, const char* component, int value);
long convertHexToDecimalSpeed(byte hexData[]);
long convertHexToDecimalTemp(byte hexData[]);
long convertHexToDecimalVoltage(byte hexData[]);
long convertHexToDecimalCurrent(byte hexData[]);

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// User UID and Database path
String uid;
String databasePath;
String speedPath = "/speed";
String tempPath = "/temperature";
String currentPath = "/current";
String voltagePath = "/voltage";
String timePath = "/timestamp";

// Parent Node path (to be updated in loop)
String parentPath;

FirebaseJson json;

// MCP2515 CAN Bus setup
struct can_frame canMsg;
MCP2515 mcp2515(5);

int speedRead = 0;
int tempValue = 0;
int currentValue = 0;
int voltageIn = 0;
int soc = 100;

// Mutex for shared data
SemaphoreHandle_t dataMutex;

// Task handlers
TaskHandle_t TaskFirebase;
TaskHandle_t TaskCANRead;

void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());
}

unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  initWiFi();

  SPI.begin();
  mcp2515.reset();
  mcp2515.setBitrate(CAN_250KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // Firebase Configuration
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);

  while ((auth.token.uid) == "") {
    delay(500);
  }
  uid = auth.token.uid.c_str();
  databasePath = "/UsersData/" + uid + "/readings";

  Serial.println("------- CAN Read ----------");

  // Initialize mutex
  dataMutex = xSemaphoreCreateMutex();

  // Create tasks for each core
  xTaskCreatePinnedToCore(
    readCANData, "TaskCANRead", 10000, NULL, 1, &TaskCANRead, 1); // Core 1

  xTaskCreatePinnedToCore(
    sendDataToFirebase, "TaskFirebase", 10000, NULL, 1, &TaskFirebase, 1); // Core 1
}

void loop() {
  // Do nothing in the loop as tasks are handled by cores
}

// Task for reading CAN data and updating Nextion on core 1
void readCANData(void * parameter) {
  HardwareSerial nextionSerial(1);
  nextionSerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN);

  for (;;) {
    if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      xSemaphoreTake(dataMutex, portMAX_DELAY);

      if (canMsg.can_id == 0x80000926) {
        speedRead = convertHexToDecimalSpeed(canMsg.data) * 0.0001885 * 32;
      } 
      else if (canMsg.can_id == 0x80001026) {
        tempValue = convertHexToDecimalTemp(canMsg.data) / 10;
        currentValue = convertHexToDecimalCurrent(canMsg.data);
      } 
      else if (canMsg.can_id == 0x80001B26) {
        voltageIn = convertHexToDecimalVoltage(canMsg.data) / 10;
      }

      // Langsung update Nextion dengan data terbaru
      sendToNextion(nextionSerial, "speed", abs(speedRead));
      sendToNextion(nextionSerial, "temperature", abs(tempValue));
      sendToNextion(nextionSerial, "ampere", abs(currentValue));
      sendToNextion(nextionSerial, "voltage", abs(voltageIn));
      sendToNextion(nextionSerial, "value", abs(soc));

      xSemaphoreGive(dataMutex);
    }
    // Tidak ada delay di sini untuk memastikan data langsung ditampilkan
  }
}

// Task for sending data to Firebase on core 1
void sendDataToFirebase(void * parameter) {
  for (;;) {
    int timestamp = getTime();
    
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    sendToFirebase(speedPath, abs(speedRead), timestamp);
    sendToFirebase(tempPath, abs(tempValue), timestamp);
    sendToFirebase(currentPath, abs(currentValue), timestamp);
    sendToFirebase(voltagePath, abs(voltageIn), timestamp);
    xSemaphoreGive(dataMutex);
    
    vTaskDelay(500 / portTICK_PERIOD_MS); // Delay untuk real-time interval (1 detik)
  }
}

// Firebase sending function
void sendToFirebase(String path, int value, int timestamp) {
  parentPath = databasePath + "/0";  // Menggunakan node tetap "0" agar data terus diperbarui

  json.set(speedPath.c_str(), String(speedRead));
  json.set(tempPath.c_str(), String(tempValue));
  json.set(currentPath.c_str(), String(currentValue));
  json.set(voltagePath.c_str(), String(voltageIn));
  json.set(timePath, String(timestamp));  // Menyimpan timestamp terbaru

  Serial.printf("Set json... %s\n", Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
}

// Nextion display function
void sendToNextion(HardwareSerial &nextionSerial, const char* component, int value) {
  nextionSerial.print(component);
  nextionSerial.print(".val=");
  nextionSerial.print(value);
  nextionSerial.write(0xff);
  nextionSerial.write(0xff);
  nextionSerial.write(0xff);
}

// Hex to decimal conversion functions
long convertHexToDecimalSpeed(byte hexData[]) {
  return ((long)hexData[0] << 24) | ((long)hexData[1] << 16) | ((long)hexData[2] << 8) | (long)hexData[3];
}

long convertHexToDecimalTemp(byte hexData[]) {
  return ((long)hexData[0] << 8) | (long)hexData[1];
}

long convertHexToDecimalVoltage(byte hexData[]) {
  return ((long)hexData[4] << 8) | (long)hexData[5];
}

long convertHexToDecimalCurrent(byte hexData[]) {
  String hexString = "";
  for (int i = 4; i < 6; i++) {
    hexString += String(hexData[i], HEX);
  }
  return strtol(hexString.c_str(), NULL, 16);
}
