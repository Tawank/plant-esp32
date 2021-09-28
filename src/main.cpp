#include <Arduino.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <HTTPClient.h>

#include "config.h"

DHTesp dht;
#define INPUT_SOIL_1 36
#define INPUT_SOIL_2 39
#define OUTPUT_LED_WIFI_CONNECTED 2

void WiFiStationConnect();
void WiFiStationGotIP(WiFiEvent_t, WiFiEventInfo_t);
void WiFiStationLostIP(WiFiEvent_t, WiFiEventInfo_t);
void WiFiStationDisconnected(WiFiEvent_t, WiFiEventInfo_t);

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.setAutoReconnect(false);
  WiFi.onEvent(WiFiStationGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.onEvent(WiFiStationLostIP, SYSTEM_EVENT_STA_LOST_IP);
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFiStationConnect();

  Serial.println("Status\tWilgotnosc (%)\tTemperatura (C)\tHeatIndex (C)\tGleba");

  dht.setup(17, DHTesp::RHT03);
}
float humidity = 0;
float temperature = 0;
float heatIndex = 0;
int soil_result_1 = 0;
int soil_result_2 = 0;

void loop() {
  delay(dht.getMinimumSamplingPeriod());

  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
  heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  soil_result_1 = map(analogRead(INPUT_SOIL_1), 3680, 1370, 0, 100);
  soil_result_2 = map(analogRead(INPUT_SOIL_2), 3680, 1370, 0, 100);

  Serial.print(dht.getStatusString());
  Serial.print("\t");
  Serial.print(humidity, 1);
  Serial.print("\t");
  Serial.print(temperature, 1);
  Serial.print("\t");
  Serial.print(heatIndex, 1);
  Serial.print("\t");
  Serial.println(soil_result_1, 1);
  Serial.print("\t");
  Serial.println(soil_result_2, 1);
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(database_url);
    http.addHeader("Content-Type", "application/json");
    char payload[100];
    snprintf(
      payload,
      sizeof(payload),
      "{\"humidity\":\"%f\",\"temperature\":\"%f\",\"heatIndex\":\"%f\",\"soil_1\":\"%i\",\"soil_2\":\"%i\",\"updated_at\":{\".sv\":\"timestamp\"}}",
      humidity,
      temperature,
      heatIndex,
      soil_result_1,
      soil_result_2
    );
    http.PUT(payload);

    http.end();
  }
 
  delay(120000);
}

uint8_t currentWiFi = 0;

void WiFiStationConnect() {
  Serial.print(F("Connecting to "));
  Serial.println(ssid[currentWiFi]);
  WiFi.begin(ssid[currentWiFi], password[currentWiFi]);
}

void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  digitalWrite(OUTPUT_LED_WIFI_CONNECTED, HIGH);
}

void WiFiStationLostIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print(F("IP lost"));
  digitalWrite(OUTPUT_LED_WIFI_CONNECTED, LOW);
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print(F("WiFi lost connection. Reason: "));
  Serial.println(info.disconnected.reason);
  if (info.disconnected.reason == WIFI_REASON_NO_AP_FOUND) {
    currentWiFi = (currentWiFi + 1) % maxWiFiCount;
  }
  WiFi.disconnect();
  WiFiStationConnect();
  digitalWrite(OUTPUT_LED_WIFI_CONNECTED, LOW);
}
