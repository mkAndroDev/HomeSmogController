#include "DHT.h"

#define DHTPIN D3
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

struct DHT_data {
  float temp;
  float hum;
} DHT_readings;

float dht_temp;

void setup_dht() {
  Serial.println("DHT22 test!");
  dht.begin();
}

void read_DHT() {
  delay(1000);

  DHT_readings.hum = dht.readHumidity();
  DHT_readings.temp = dht.readTemperature();

  Serial.println("Temperatura: " + String(dht.readTemperature()) + ", wilgotność: " + String(dht.readHumidity()));

  if (isnan(DHT_readings.hum) || isnan(DHT_readings.temp) ) {
    Serial.println("Failed to read from DHT sensor!");
    DHT_readings.temp = -90;
    DHT_readings.hum = -10;
    return;
  }

}
