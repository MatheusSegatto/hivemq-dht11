#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#include "secrets.h"

#define DHT_PIN D2 // pino de dados do DHT11
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastPublish = 0;

void connectWifi()
{
  Serial.printf("Conectando ao Wi-Fi \"%s\"", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\nConectado. IP: %s\n", WiFi.localIP().toString().c_str());
}

void connectMqtt()
{
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  while (!mqtt.connected())
  {
    Serial.printf("Conectando ao broker %s...", MQTT_HOST);
    // sem usr/senha
    if (mqtt.connect(MQTT_CLIENT_ID))
    {
      Serial.println(" OK");
    }
    else
    {
      Serial.printf(" falhou (rc=%d). Tentando novamente em 3s\n", mqtt.state());
      delay(3000);
    }
  }
}

void publishReading()
{
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  if (isnan(temperatura) || isnan(umidade))
  {
    Serial.println("Falha ao ler do DHT11");
    return;
  }

  JsonDocument doc;
  doc["client_id"] = MQTT_CLIENT_ID;
  doc["temperatura"] = temperatura;
  doc["umidade"] = umidade;
  doc["uptime_ms"] = millis();

  char payload[160];
  size_t n = serializeJson(doc, payload, sizeof(payload));

  if (mqtt.publish(MQTT_TOPIC, payload, n))
  {
    Serial.printf("Publicado em %s: %s\n", MQTT_TOPIC, payload);
  }
  else
  {
    Serial.println("Falha ao publicar");
  }
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  dht.begin();

  connectWifi();
  connectMqtt();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    connectWifi();
  if (!mqtt.connected())
    connectMqtt();
  mqtt.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS)
  {
    lastPublish = now;
    publishReading();
  }
}
