#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>
#include <WebServer.h>

#define SCREEN_HEIGHT 240
#define SCREEN_WIDTH 240
#define TFT_SCLK  18
#define TFT_MOSI  23
#define TFT_RST   4
#define TFT_DC    2
#define TFT_CS    -1

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#define TFT_BLACK   0x0000
#define TFT_WHITE   0xFFFF

#define DHTPIN 19
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

typedef struct struct_message {
    float temperature;
    float humidity;
} struct_message;

struct_message slaveData;

WebServer server(80);

void onReceive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    if (len == sizeof(struct_message)) {
        memcpy(&slaveData, data, sizeof(struct_message));
        Serial.println("✅ Odebrano dane z ESP-NOW:");
        Serial.print("Slave - Temp: "); Serial.print(slaveData.temperature);
        Serial.print(" °C, Humidity: "); Serial.println(slaveData.humidity);
    }
}

void handleRoot() {
    String page = "<html><head><meta http-equiv='refresh' content='2'></head><body>";
    page += "<h2>Dane z czujników</h2>";
    page += "<p><b>Temperatura lokalna:</b> " + String(dht.readTemperature()) + "°C</p>";
    page += "<p><b>Wilgotność lokalna:</b> " + String(dht.readHumidity()) + "%</p>";
    page += "<p><b>Temperatura slave:</b> " + String(slaveData.temperature) + "°C</p>";
    page += "<p><b>Wilgotność slave:</b> " + String(slaveData.humidity) + "%</p>";
    page += "</body></html>";
    server.send(200, "text/html", page);
}

void setup() {
    Serial.begin(115200);
    
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();  
    Serial.print("Host MAC: "); Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Błąd inicjalizacji ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(onReceive);

    tft.init(SCREEN_WIDTH, SCREEN_HEIGHT, SPI_MODE3);
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Czekam na dane...");

    dht.begin();

    WiFi.softAP("ESP32_Server", "12345678");
    server.on("/", handleRoot);
    server.begin();
}

void displayData(float localTemp, float localHum, float remoteTemp, float remoteHum) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 20);
    tft.print("Local Temp: "); tft.print(localTemp, 1); tft.println(" C");

    tft.setCursor(10, 50);
    tft.print("Local Hum: "); tft.print(localHum, 1); tft.println(" %");

    tft.setCursor(10, 100);
    tft.print("Slave Temp: "); tft.print(remoteTemp, 1); tft.println(" C");

    tft.setCursor(10, 130);
    tft.print("Slave Hum: "); tft.print(remoteHum, 1); tft.println(" %");
}

void loop() {
    float localTemp = dht.readTemperature();
    float localHum = dht.readHumidity();

    displayData(localTemp, localHum, slaveData.temperature, slaveData.humidity);
    server.handleClient();

    delay(2000);
}
