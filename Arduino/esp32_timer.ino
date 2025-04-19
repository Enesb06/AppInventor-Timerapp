#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // Bildirimler (Notifications) için gerekli

// --- UUID'LER ---
// App Inventor'daki UUID'lerinizle AYNI OLMALI!
// Örnek UUID'ler - Kendi benzersiz UUID'lerinizi kullanmanız önerilir.
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_UPTIME "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// ----------------

BLEServer* pServer = NULL;
BLECharacteristic* pUptimeCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false; // Bağlantı durumu değişikliğini takip etmek için
unsigned long startTime = 0; // ESP32 başladığındaki zaman

// --- Bağlantı Durumu Callback'leri ---
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("Cihaz bağlandı");
    }

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("Cihaz bağlantısı kesildi");
      // Bağlantı koptuğunda tekrar advertising başlatmak iyi bir pratiktir
      // Kısa bir gecikme, yeniden bağlanma sorunlarını azaltabilir
      delay(500);
      pServer->startAdvertising();
      Serial.println("Advertising tekrar başlatıldı");
    }
};
// -------------------------------------

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 BLE Uptime Sunucusu Başlatılıyor...");

  startTime = millis(); // Başlangıç zamanını kaydet

  // BLE Cihazını Başlat (App Inventor'da görünecek isim)
  BLEDevice::init("ESP32_Uptime"); // Cihaz adını isterseniz değiştirebilirsiniz

  // BLE Sunucusunu Oluştur
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks()); // Callback'leri ata

  // BLE Servisini Oluştur
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Uptime Karakteristiğini Oluştur
  pUptimeCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_UPTIME,
                      BLECharacteristic::PROPERTY_READ   | // Okunabilir
                      BLECharacteristic::PROPERTY_NOTIFY   // Bildirim gönderebilir
                    );

  // Karakteristik için başlangıç değeri (isteğe bağlı ama iyi bir pratik)
  pUptimeCharacteristic->setValue("0");

  // Bildirimleri etkinleştirmek için BLE2902 Descriptor'ını ekle
  // Bu, istemcinin (App Inventor) bildirimleri alabilmesi için gereklidir
  pUptimeCharacteristic->addDescriptor(new BLE2902());

  // Servisi Başlat
  pService->start();

  // Advertising (Yayın) Ayarları ve Başlatma
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID); // Servis UUID'sini yayına ekle
  pAdvertising->setScanResponse(true);
  // Bağlantı aralıkları için tercihler (isteğe bağlı)
  pAdvertising->setMinPreferred(0x06);  // iOS için önerilen değerler
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising(); // Advertising'i başlat
  Serial.println("Karakteristik tanımlandı, Advertising başlatıldı. Bağlantı bekleniyor...");
}

void loop() {
  // Cihaz yeni bağlandıysa veya bağlantı koptuysa durumu yazdır
  if (deviceConnected && !oldDeviceConnected) {
    // Cihaz yeni bağlandı
    oldDeviceConnected = true;
    Serial.println("Bağlantı durumu: Bağlı");
  }
  if (!deviceConnected && oldDeviceConnected) {
    // Bağlantı yeni koptu
    oldDeviceConnected = false;
    Serial.println("Bağlantı durumu: Kopuk");
    // Belki burada başka işlemler yapılabilir (örn: LED söndürme)
  }

  // Eğer bir cihaz bağlıysa, uptime değerini gönder
  if (deviceConnected) {
    // Geçen süreyi hesapla (milisaniye)
    unsigned long currentMillis = millis();
    // Başlangıçtan beri geçen süre yerine direkt millis() de kullanılabilir
    // unsigned long uptimeMillis = currentMillis - startTime;
   // Bunun yerine:
unsigned long uptimeMillis = currentMillis - startTime;

    // Saniyeye çevir
    uint32_t uptimeSeconds = uptimeMillis / 1000;

    // Değeri String'e çevir
    String uptimeString = String(uptimeSeconds);

    // Karakteristik değerini güncelle
    pUptimeCharacteristic->setValue((uint8_t*)uptimeString.c_str(), uptimeString.length());


    // Bağlı cihaza bildirimi gönder (notify)
    pUptimeCharacteristic->notify();

    Serial.print("Uptime: ");
    Serial.print(uptimeSeconds);
    Serial.println(" saniye - Bildirim gönderildi.");
  }

  // Her saniye güncelleme yap
  delay(1000);
}



