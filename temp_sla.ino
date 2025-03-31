#include <esp_now.h>
#include <WiFi.h>
#include <DHT.h>

#define DHTPIN 32       // Pin, do którego podłączony jest czujnik DHT22
#define DHTTYPE DHT22   // Typ czujnika

DHT dht(DHTPIN, DHTTYPE);

typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

struct_message sensorData;
uint8_t hostMAC[] = {0x5C, 0x01, 0x3B, 0x6C, 0x8B, 0x5C};  // MAC hosta

// Zmienne do przechowywania ostatnich poprawnych odczytów
float lastTemp = NAN;
float lastHum = NAN;

void setup() {
  Serial.begin(115200);
  dht.begin();
  WiFi.mode(WIFI_STA);
  Serial.print("Slave MAC: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    while (true) { delay(1000); }
  }
  
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, hostMAC, 6);
  peerInfo.channel = 0;  // Domyślny kanał
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Nie udało się dodać peer (host)");
  }
}

void loop() {
  // Odczyt danych z DHT22
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  
  // Jeśli odczyt jest poprawny, aktualizuj wartości; w przeciwnym wypadku użyj poprzednich poprawnych odczytów
  if (!isnan(temp) && !isnan(hum)) {
    sensorData.temperature = temp;
    sensorData.humidity = hum;
    lastTemp = temp;
    lastHum = hum;
  } else {
    sensorData.temperature = lastTemp;
    sensorData.humidity = lastHum;
    Serial.println("Błąd odczytu czujnika, używam ostatnich wartości.");
  }
  
  // Wysyłanie danych do hosta przez ESP-NOW
  esp_err_t result = esp_now_send(hostMAC, (uint8_t *)&sensorData, sizeof(sensorData));
  if (result == ESP_OK) {
    Serial.print("Wysłano dane ESP-NOW: Temp: ");
    Serial.print(sensorData.temperature);
    Serial.print(" °C, Humidity: ");
    Serial.println(sensorData.humidity);
  } else {
    Serial.print("Błąd wysyłania: ");
    Serial.println(result);
  }
  
  // Wyświetlenie odczytów na Serial Monitorze na slave
  Serial.print("Slave - Odczyt z czujnika: Temp: ");
  Serial.print(sensorData.temperature);
  Serial.print(" °C, Humidity: ");
  Serial.println(sensorData.humidity);
  
  delay(2000);
}