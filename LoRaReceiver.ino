#include <WiFiNINA.h>
#include <Firebase_Arduino_WiFiNINA.h>
#include <LoRa.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// Wi-Fi & Firebase configuration
#define WIFI_SSID "Krish"
#define WIFI_PASSWORD "12345678"
#define FIREBASE_HOST "firefighter-497b0-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "sNC2UT4g8D0mLKn1zeD74IsW6ZfvLpwkDpO16lAX"

FirebaseData firebaseData;
String deviceID = "Arduino1";

// Hardware pins
#define LED_TEMP 2
#define LED_GAS 3
#define BUZZER_PIN 4

// Thresholds
#define TEMP_THRESHOLD 10
#define GAS_THRESHOLD 40

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 36000); // UTC+10

bool ledTempState = false;
bool ledGasState = false;
bool buzzerState = false;

int lastGas = 0, lastTemp = 0, lastHumid = 0;
unsigned long lastFirebaseSync = 0;
const unsigned long firebaseInterval = 30000; // 30 seconds

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    timeClient.begin();
    timeClient.update();

    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH, WIFI_SSID, WIFI_PASSWORD);
    Firebase.reconnectWiFi(true);

    // Initial state from Firebase
    Firebase.getInt(firebaseData, "/" + deviceID + "/ledTemp") && (ledTempState = firebaseData.intData() == 1);
    Firebase.getInt(firebaseData, "/" + deviceID + "/ledGas") && (ledGasState = firebaseData.intData() == 1);
    Firebase.getBool(firebaseData, "/" + deviceID + "/buzzer") && (buzzerState = firebaseData.boolData());
  } else {
    Serial.println("\nWiFi connection failed");
  }

  // Start LoRa
  LoRa.setPins(10, 8, 7);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  Serial.println("LoRa initialized");

  // Setup pins
  pinMode(LED_TEMP, OUTPUT);
  pinMode(LED_GAS, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

void loop() {
  timeClient.update();

  // Process LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String payload = "";
    while (LoRa.available()) {
      payload += (char)LoRa.read();
    }
    Serial.println("Received: " + payload);
    // Variables
    char deviceIdBuf[20];
    int gas, temp, humid;
    // if (sscanf(payload.c_str(), "%d,%d,%d", &gas, &temp, &humid) == 3) {
    if (sscanf(payload.c_str(), "%19[^,],%d,%d,%d", deviceIdBuf, &gas, &temp, &humid) == 4) {
      String deviceID = String(deviceIdBuf);  // now dynamic per sender
      Serial.println("Device: " + deviceID);
      unsigned long epochTime = timeClient.getEpochTime();
      lastGas = gas;
      lastTemp = temp;
      lastHumid = humid;

      // Evaluate threshold logic
      bool tempExceeds = (temp >= TEMP_THRESHOLD);
      bool gasExceeds = (gas >= GAS_THRESHOLD);

      // Auto-set states before possible override
      ledTempState = tempExceeds;
      ledGasState = gasExceeds;
      buzzerState = tempExceeds || gasExceeds;
    }
  }


  unsigned long currentMillis = millis();
  if (WiFi.status() == WL_CONNECTED && currentMillis - lastFirebaseSync >= firebaseInterval) {
    lastFirebaseSync = currentMillis;

    unsigned long epochTime = timeClient.getEpochTime();
    Firebase.setInt(firebaseData, "/" + deviceID + "/gas", lastGas);
    Firebase.setInt(firebaseData, "/" + deviceID + "/temperature", lastTemp);
    Firebase.setInt(firebaseData, "/" + deviceID + "/humidity", lastHumid);
    Firebase.setInt(firebaseData, "/" + deviceID + "/timestamp", epochTime);
    Firebase.setInt(firebaseData, "/" + deviceID + "/ledTemp", ledTempState ? 1 : 0);
    Firebase.setInt(firebaseData, "/" + deviceID + "/ledGas", ledGasState ? 1 : 0);
    Firebase.setBool(firebaseData, "/" + deviceID + "/buzzer", buzzerState);
    Serial.println("Firebase synced");

    // ✅ Always override with Firebase values
    if (Firebase.getInt(firebaseData, "/" + deviceID + "/ledTemp")) {
      ledTempState = firebaseData.intData() == 1;
    }
    if (Firebase.getInt(firebaseData, "/" + deviceID + "/ledGas")) {
      ledGasState = firebaseData.intData() == 1;
    }
    if (Firebase.getBool(firebaseData, "/" + deviceID + "/buzzer")) {
      buzzerState = firebaseData.boolData();
    }
  }

  // Apply final states to hardware
  digitalWrite(LED_TEMP, ledTempState ? HIGH : LOW);
  digitalWrite(LED_GAS, ledGasState ? HIGH : LOW);
  if (buzzerState) {
    tone(BUZZER_PIN, 1000);
  } else {
    noTone(BUZZER_PIN);
  }
}

