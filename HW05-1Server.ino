#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// BLE server name
#define bleServerName "DHT_ESP32"
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

float temp;
float hum;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

bool deviceConnected = false;

// UUIDs
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
BLECharacteristic dhtTemperatureCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor dhtTemperatureDescriptor(BLEUUID((uint16_t)0x2902));
BLECharacteristic dhtHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor dhtHumidityDescriptor(BLEUUID((uint16_t)0x2903));

// BLE callback
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setup() {
  Serial.begin(115200);
  dht.begin();

  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *dhtService = pServer->createService(SERVICE_UUID);

  dhtService->addCharacteristic(&dhtTemperatureCharacteristics);
  dhtTemperatureDescriptor.setValue("DHT temperature Celsius");
  dhtTemperatureCharacteristics.addDescriptor(&dhtTemperatureDescriptor);

  dhtService->addCharacteristic(&dhtHumidityCharacteristics);
  dhtHumidityDescriptor.setValue("DHT humidity");
  dhtHumidityCharacteristics.addDescriptor(&dhtHumidityDescriptor);

  dhtService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      temp = dht.readTemperature();
      hum = dht.readHumidity();

      if (isnan(temp) || isnan(hum)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
      }

      // Notify temperature
      static char tempBuffer[6];
      dtostrf(temp, 6, 2, tempBuffer);
      dhtTemperatureCharacteristics.setValue(tempBuffer);
      dhtTemperatureCharacteristics.notify();
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.print(" ºC");

      // Notify humidity
      static char humBuffer[6];
      dtostrf(hum, 6, 2, humBuffer);
      dhtHumidityCharacteristics.setValue(humBuffer);
      dhtHumidityCharacteristics.notify();
      Serial.print(" - Humidity: ");
      Serial.print(hum);
      Serial.println(" %");

      lastTime = millis();
    }
  }
}