#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme;

float temperature, humidity, pressure, altitude;

struct BME_data {
  float temperature;
  float humidity;
  float pressure;
  float altitude;
} bmeData;

void setupBme() {
  Serial.begin(115200);
  bme.begin(0x76);
}

void readBme() {
  delay(1000);
  bmeData.temperature = bme.readTemperature();
  bmeData.humidity = bme.readHumidity();
  bmeData.pressure = bme.readPressure() / 100.0F;
  bmeData.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  Serial.println("Temperatura: " + String(bmeData.temperature) + ", wilgotność: " + String(bmeData.humidity) + ", ciśnienie: " + String(bmeData.pressure) + ", wysokość: " + String(bmeData.altitude));

  if (isnan(bmeData.humidity) || isnan(bmeData.temperature) ) {
    Serial.println("Failed to read from DHT sensor!");
    bmeData.temperature = -90;
    bmeData.humidity = -10;
    return;
  }
}
