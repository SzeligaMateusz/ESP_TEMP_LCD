#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 240

#define TFT_SCLK  18  // SCK
#define TFT_MOSI  23  // SDA
#define TFT_RST   4   // RES
#define TFT_DC    2   // DC
#define TFT_CS    -1  // CS nieużywany

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define DHTPIN 19
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

#define TFT_RED     0xF800
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF

// Struktura do przechowywania danych z ESP-NOW
typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

struct_message slaveData;
uint8_t slaveMAC[] = {0x8C, 0x4F, 0x00, 0x27, 0xD0, 0x2C};

void onReceive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  if (len == sizeof(struct_message)) {
    memcpy(&slaveData, data, sizeof(struct_message));
    Serial.println("Odebrano dane z ESP-NOW:");
    Serial.print("Slave - Temp: ");
    Serial.print(slaveData.temperature);
    Serial.print(" °C, Humidity: ");
    Serial.println(slaveData.humidity);
  }
}

void setup() {
  Serial.begin(115200);
  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MODE3);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Inicjalizacja...");

  dht.begin();
  WiFi.mode(WIFI_STA);
  Serial.print("Host MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    tft.setCursor(10, 30);
    tft.println("ESP-NOW error");
    while (1) { delay(1000); }
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, slaveMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Nie udało się dodać peer (slave)");
    tft.setCursor(10, 50);
    tft.println("Peer error");
  }
}

void displayData(float localTemp, float localHum, float remoteTemp, float remoteHum) {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 20);
  tft.print("Local Temp: ");
  tft.print(localTemp, 1);
  tft.println(" C");

  tft.setCursor(10, 50);
  tft.print("Local Hum: ");
  tft.print(localHum, 1);
  tft.println(" %");

  tft.setCursor(10, 100);
  tft.print("Slave Temp: ");
  tft.print(remoteTemp, 1);
  tft.println(" C");

  tft.setCursor(10, 130);
  tft.print("Slave Hum: ");
  tft.print(remoteHum, 1);
  tft.println(" %");
}

void loop() {
  float localTemp = dht.readTemperature();
  float localHum = dht.readHumidity();
  
  Serial.print("Host - Local Temp: ");
  Serial.print(localTemp);
  Serial.print(" °C, Humidity: ");
  Serial.println(localHum);

  displayData(localTemp, localHum, slaveData.temperature, slaveData.humidity);
  delay(2000);
}
