#include <SPI.h>
#include <LoRa.h>
#include <DFRobot_DHT11.h>

// Sensor pin definitions
#define MQ2_PIN A1      // MQ2 gas sensor analog pin
#define DHT11_PIN 2     // DHT11 data pin

// LoRa pin definitions for Arduino Nano 33 IoT
#define LORA_NSS 10     // Chip select
#define LORA_RST 8      // Reset
#define LORA_DIO0 7     // DIO0

DFRobot_DHT11 DHT;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial); // Wait for Serial to be ready

  // Initialize LoRa
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) { // Set frequency to 433 MHz
    Serial.println("LoRa initialization failed!");
    while (1);
  }
  Serial.println("LoRa initialized successfully");

  // Configure LoRa parameters (optional, adjust as needed)
  LoRa.setTxPower(20); // Set to max power (20 dBm)
  LoRa.setSpreadingFactor(7); // Default SF7 for balance between range and speed
  LoRa.setSignalBandwidth(125E3); // 125 kHz bandwidth
}

void loop() {
  // Read MQ2 gas sensor
  int gasLevel = analogRead(MQ2_PIN);
  
  // Read DHT11 sensor
  DHT.read(DHT11_PIN);
  int temperature = DHT.temperature;
  int humidity = DHT.humidity;

  String deviceID = "Arduino1";  // change for each sender (e.g., NodeB, NodeC)
  String data = deviceID + "," + String(gasLevel) + "," + String(temperature) + "," + String(humidity);

  // Format sensor data into a string
  //String data = String(gasLevel) + "," + String(temperature) + "," + String(humidity);

  // Send data via LoRa
  LoRa.beginPacket();
  LoRa.print(data);
  LoRa.endPacket();

  // Print data to Serial for debugging
  Serial.print("MQ-2 Gas Level: ");
  Serial.print(gasLevel);
  Serial.print(" | Temp: ");
  Serial.print(temperature);
  Serial.print("C | Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  
  delay(1000); // Send every second
}
