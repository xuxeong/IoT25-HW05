#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

// BLE 서버 이름
#define bleServerName "SujeongChoi"

// DHT 센서 설정
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// 전역 변수
float temp;
float hum;
unsigned long lastTime = 0;
unsigned long timerDelay = 30000; // 30초 간격

bool deviceConnected = false;

// UUID 설정
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
BLECharacteristic dhtTemperatureCharacteristics(
  "cba1d466-344c-4be3-ab3f-189f80dd7518",
  BLECharacteristic::PROPERTY_NOTIFY
);
BLECharacteristic dhtHumidityCharacteristics(
  "ca73b3ba-39f6-4ab3-91ae-186dc9577d99",
  BLECharacteristic::PROPERTY_NOTIFY
);

// BLE 서버 콜백
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

void setup() {
  Serial.begin(115200);
  dht.begin();

  // BLE 초기화
  BLEDevice::init(bleServerName);
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // 서비스 생성
  BLEService* dhtService = pServer->createService(SERVICE_UUID);

  // 온도 characteristic 등록 및 descriptor 추가
  dhtService->addCharacteristic(&dhtTemperatureCharacteristics);
  dhtTemperatureCharacteristics.addDescriptor(new BLE2902()); // ✅ Android 호환

  // 습도 characteristic 등록 및 descriptor 추가
  dhtService->addCharacteristic(&dhtHumidityCharacteristics);
  dhtHumidityCharacteristics.addDescriptor(new BLE2902()); // ✅ Android 호환

  // 서비스 시작
  dhtService->start();

  // 광고 시작
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("Waiting for client connection...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      temp = dht.readTemperature();
      hum = dht.readHumidity();

      // 센서 오류 확인
      if (isnan(temp) || isnan(hum)) {
        Serial.println("Failed to read from DHT sensor!");
        return;
      }

      // 온도 전송
      static char tempBuffer[6];
      dtostrf(temp, 6, 2, tempBuffer);
      dhtTemperatureCharacteristics.setValue(tempBuffer);
      dhtTemperatureCharacteristics.notify();
      Serial.print("Temperature: ");
      Serial.print(temp);
      Serial.print(" ºC");

      // 습도 전송
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
