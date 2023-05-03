#include <M5Atom.h>
#include <BLEDevice.h>
#include <BLE2902.h>
#include <FastLED.h>

#define LED_NUMBER 15

// BLE上のこのデバイスの名前
#define BLE_LOCAL_NAME "RGBLED-control"
// BLEのサービスUUID
#define BLE_SERVICE_UUID "ae6e4ee0-da99-11ed-afa1-0242ac120002"
// BLEのCharacteristic UUID
#define BLE_CHARACTERISTIC_BTN_UUID "b8092e20-da99-11ed-afa1-0242ac120002"
#define BLE_CHARACTERISTIC_LED_UUID "e7128b62-da99-11ed-afa1-0242ac120002"

CRGB leds[LED_NUMBER];

static BLEServer* pBleServer = NULL;
static BLECharacteristic* pBleNotifyCharacteristic_BTN = NULL;
static BLECharacteristic* pBleNotifyCharacteristic_LED = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
std::string rxValue;
std::string txValue;
bool bleOn = false;
bool ble_write = false;

uint8_t ledbuf[3];
int     cnt;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Connect.");
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Disconnect.");
    ESP.restart();   // software reset
  }
};

class MyLEDCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    Serial.println("onWrite");
    std::string rxValue = pCharacteristic->getValue();
    Serial.println( rxValue.length() );
    if( rxValue.length() == 3 ){
      for(int i=0; i<rxValue.length(); i++ ){
        Serial.print(rxValue[i],HEX);
        ledbuf[i] = rxValue[i];
        Serial.print(ledbuf[i]&0xff, HEX );
        Serial.print(" ");
      }
    } else {
        Serial.println("Invalid Length.");
    }
    ble_write = true;
  }
};

class MyBTNCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onRead(BLECharacteristic *pCharacteristic) {
        Serial.println("Read request received");
        uint8_t value = cnt;
        pCharacteristic->setValue(&value, 1);
    }
};

void sendCounterVal()
{
  uint8_t value = cnt;
  pBleNotifyCharacteristic_BTN->setValue(&value, 1);
  pBleNotifyCharacteristic_BTN->notify();             //ここでNotiryを送る
}

void setup() {
  // M5Stackの初期化
  M5.begin(true,false,true);

  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  FastLED.addLeds<NEOPIXEL, 32 >(leds, LED_NUMBER);
  FastLED.setBrightness(32);

  // BLE環境の初期化
  BLEDevice::init(BLE_LOCAL_NAME);
  // BLEサーバ生成 
  pBleServer = BLEDevice::createServer();
  pBleServer->setCallbacks(new MyServerCallbacks());

  // BLEのサービス生成。引数でサービスUUIDを設定する。
  BLEService* pBleService = pBleServer->createService(BLE_SERVICE_UUID);
  // BLE Characteristicの生成
  pBleNotifyCharacteristic_BTN = pBleService->createCharacteristic(
                                   BLE_CHARACTERISTIC_BTN_UUID,
                                   BLECharacteristic::PROPERTY_NOTIFY);
                                   //BLECharacteristic::PROPERTY_READ
  pBleNotifyCharacteristic_LED = pBleService->createCharacteristic (
                                   BLE_CHARACTERISTIC_LED_UUID,
                                   //BLECharacteristic::PROPERTY_NOTIFY
                                   BLECharacteristic::PROPERTY_WRITE );

  pBleNotifyCharacteristic_BTN->setCallbacks(new MyBTNCharacteristicCallbacks());       
  pBleNotifyCharacteristic_LED->setCallbacks(new MyLEDCharacteristicCallbacks());


  // BLE Characteristicにディスクリプタを設定
  pBleNotifyCharacteristic_BTN->addDescriptor(new BLE2902());
  pBleNotifyCharacteristic_LED->addDescriptor(new BLE2902());

  // BLEサービスの開始
  pBleService->start();
  // BLEのアドバタイジングを開始
  BLEAdvertising *pAdvertising = pBleServer->getAdvertising();
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->setMaxPreferred(0x12);
  pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  //pAdvertising->setTxPower(ESP_PWR_LVL_P3);

  esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P3);
  pBleServer->getAdvertising()->start();
  Serial.println(BLE_LOCAL_NAME);
  Serial.println(BLE_SERVICE_UUID);
  Serial.println("start BLE");

  for(int j=0; j<LED_NUMBER; j++) {
    leds[j] = CRGB::Blue;
  }
  FastLED.show();
  Serial.println("Started.");
}


void loop() {
  M5.update();
  if (M5.Btn.wasPressed()) {
    cnt++;
    Serial.println(cnt);
    sendCounterVal();
  }

  if (ble_write){
    for(int j=0; j<LED_NUMBER; j++) {
      leds[j] = ledbuf[2]<<16 | ledbuf[1] <<8 | ledbuf[0];
    }
    FastLED.show();      
    ble_write = false;
  }

  delay(200);
}