#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#include "secrets.h"

// ===== Sensor DHT =====
#define DHT_PIN D2 // GPIO4 — pino de dados do DHT11
#define DHT_TYPE DHT11

// Alimentar o DHT pelos pinos GPIO:
//   1 -> VCC do sensor no D6 e GND no D7
//   0 -> sensor alimentado pelos trilhos 3V3/GND da placa
#define DHT_POWER_VIA_GPIO 0
#define DHT_PIN_VCC D6 // GPIO12
#define DHT_PIN_GND D7 // GPIO13

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
    // client_id precisa ser unico
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
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();

  if (isnan(temp) || isnan(umid))
  {
    Serial.println("Falha ao ler do DHT11");
    return;
  }

  // {"sensor_id": "...", "temp": ..., "umid": ...}
  JsonDocument doc;
  doc["sensor_id"] = SENSOR_ID;
  doc["temp"] = temp;
  doc["umid"] = umid;

  char payload[128];
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

#if DHT_POWER_VIA_GPIO
  // Liga o sensor antes
  pinMode(DHT_PIN_VCC, OUTPUT);
  pinMode(DHT_PIN_GND, OUTPUT);
  digitalWrite(DHT_PIN_VCC, HIGH);
  digitalWrite(DHT_PIN_GND, LOW);
  delay(1000);
#endif

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
