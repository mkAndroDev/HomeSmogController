unsigned long myChannelNumber = ***; //your_thingspeak_channel_number
const String myWriteAPIKey = "***"; //your_thigspeak_API_key
const char* server = "api.thingspeak.com";

char ssid[] = "***"; //your_wifi_name
char pass[] = "***"; //your_wifi_password

#define TS_DELAY 60 * 1000
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include "dht_sensor.h"
#include "bme_sensor.h"
#include <SoftwareSerial.h>

bool is_SDS_running = true;

#define SDS_PIN_RX D3
#define SDS_PIN_TX D4

SoftwareSerial serialSDS(SDS_PIN_RX, SDS_PIN_TX, false, 1024);

int RELAY_PIN = 10;

unsigned long lastwrite = 0;

String esp_chipid;

String server_name;

float sds_display_values_10[5];
float sds_display_values_25[5];
unsigned int sds_display_value_pos = 0;
bool send_now = false;
bool will_check_for_update = false;

String last_result_SDS = "";
String last_value_SDS_P1 = "";
String last_value_SDS_P2 = "";

/*****************************************************************
  /* convert float to string with a                                *
  /* precision of two decimal places                               *
  /*****************************************************************/
String Float2String(const float value)
{
  // Convert a float to String with two decimals.
  char temp[15];
  String s;

  dtostrf(value, 13, 2, temp);
  s = String(temp);
  s.trim();
  return s;
}

/*****************************************************************
  /* Debug output                                                  *
  /*****************************************************************/
void debug_out(const String &text, int linebreak = 1) {
  if (linebreak) {
    Serial.println(text);
  }  else {
    Serial.print(text);
  }
}

/*****************************************************************
  /* start SDS018 sensor                                           *
  /*****************************************************************/
void start_SDS() {
  const uint8_t start_SDS_cmd[] =
  {
    0xAA, 0xB4, 0x06, 0x01, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x05, 0xAB
  };
  serialSDS.write(start_SDS_cmd, sizeof(start_SDS_cmd));
  is_SDS_running = true;
}

// set duty cycle
void set_SDS_duty(uint8_t d) {
  uint8_t cmd[] =
  {
    0xaa, 0xb4, 0x08, 0x01, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x0a, 0xab
  };
  cmd[4] = d;
  unsigned int checksum = 0;
  for (int i = 2; i <= 16; i++)
    checksum += cmd[i];
  checksum = checksum % 0x100;
  cmd[17] = checksum;

  serialSDS.write(cmd, sizeof(cmd));
}
/*****************************************************************
  /* stop SDS018 sensor                                            *
  /*****************************************************************/
void stop_SDS() {
  const uint8_t stop_SDS_cmd[] =
  {
    0xAA, 0xB4, 0x06, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x05, 0xAB
  };
  serialSDS.write(stop_SDS_cmd, sizeof(stop_SDS_cmd));
  is_SDS_running = false;
}

//set initiative mode
void set_initiative_SDS() {
  //aa, 0xb4, 0x08, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x0a, 0xab
  const uint8_t stop_SDS_cmd[] =
  {
    0xAA, 0xB4, 0x08, 0x01, 0x03,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0xFF, 0xFF, 0x0A, 0xAB
  };
  serialSDS.write(stop_SDS_cmd, sizeof(stop_SDS_cmd));
  is_SDS_running = false;
}

/*****************************************************************
  /* read SDS018 sensor values                                     *
  /*****************************************************************/
String SDS_version_date() {
  const uint8_t version_SDS_cmd[] = {0xAA, 0xB4, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x05, 0xAB};
  String s = "";
  String value_hex;
  char buffer;
  int value;
  int len = 0;
  String version_date = "";
  String device_id = "";
  int checksum_is;
  int checksum_ok = 0;
  int position = 0;

  debug_out(F("Start reading SDS018 version date"), 1);

  start_SDS();

  delay(100);

  serialSDS.write(version_SDS_cmd, sizeof(version_SDS_cmd));

  delay(100);

  Serial.println("Serial available: " + String(serialSDS.available()));

  while (serialSDS.available() > 0)
  {
    buffer = serialSDS.read();
    //    debug_out(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", 1);
    //    "aa" = 170, "ab" = 171, "c0" = 192
    value = int(buffer);
    switch (len)
    {
      case (0):
        if (value != 170)
        {
          len = -1;
        };
        break;
      case (1):
        if (value != 197)
        {
          len = -1;
        };
        break;
      case (2):
        if (value != 7)
        {
          len = -1;
        };
        break;
      case (3):
        version_date = String(value);
        checksum_is = 7 + value;
        break;
      case (4):
        version_date += "-" + String(value);
        checksum_is += value;
        break;
      case (5):
        version_date += "-" + String(value);
        checksum_is += value;
        break;
      case (6):
        if (value < 0x10)
        {
          device_id = "0" + String(value, HEX);
        }
        else
        {
          device_id = String(value, HEX);
        };
        checksum_is += value;
        break;
      case (7):
        if (value < 0x10)
        {
          device_id += "0";
        };
        device_id += String(value, HEX);
        checksum_is += value;
        break;
      case (8):
        debug_out(F("Checksum is: "), 0);
        debug_out(String(checksum_is % 256), 0);
        debug_out(F(" - should: "), 0);
        debug_out(String(value), 1);
        if (value == (checksum_is % 256))
        {
          checksum_ok = 1;
        }
        else
        {
          len = -1;
        };
        break;
      case (9):
        if (value != 171)
        {
          len = -1;
        };
        break;
    }
    len++;
    if (len == 10 && checksum_ok == 1)
    {
      s = version_date + "(" + device_id + ")";
      debug_out(F("SDS version date : "), 0);
      debug_out(version_date, 1);
      debug_out(F("SDS device ID:     "), 0);
      debug_out(device_id, 1);
      len = 0;
      checksum_ok = 0;
      version_date = "";
      device_id = "";
      checksum_is = 0;
    }
    yield();
  }

  debug_out(F("End reading SDS018 version date"), 1);

  return s;
}

WiFiClient client;

String sensorSDS()
{
  String s = "";
  String value_hex;
  char buffer;
  int value;
  int len = 0;
  int pm10_serial = 0;
  int pm25_serial = 0;
  int checksum_is;
  int checksum_ok = 0;
  int position = 0;

  while (serialSDS.available() > 0)
  {
    buffer = serialSDS.read();
    //    debug_out(String(len) + " - " + String(buffer, DEC) + " - " + String(buffer, HEX) + " - " + int(buffer) + " .", 1);
    //          "aa" = 170, "ab" = 171, "c0" = 192
    value = unsigned(buffer);
    switch (len)
    {
      case (0):
        if (value != 170)
        {
          len = -1;
        };
        break;
      case (1):
        if (value != 192)
        {
          len = -1;
        };
        break;
      case (2):
        pm25_serial = value;
        checksum_is = value;
        break;
      case (3):
        pm25_serial += (value << 8);
        checksum_is += value;
        break;
      case (4):
        pm10_serial = value;
        checksum_is += value;
        break;
      case (5):
        pm10_serial += (value << 8);
        checksum_is += value;
        break;
      case (6):
        checksum_is += value;
        break;
      case (7):
        checksum_is += value;
        break;
      case (8):
        //        debug_out("Checksum is: " + String(checksum_is % 256) + "/" + String(checksum_is) + " - should: " + String(value), 1);
        if (value == (checksum_is % 256))
        {
          checksum_ok = 1;
        }
        else
        {
          len = -1;
        };
        break;
      case (9):
        if (value != 171)
        {
          len = -1;
        };
        break;
    }
    len++;
    if (len == 10 && checksum_ok == 1)
    {
      //      debug_out("Len 10 and checksum OK", 1);
      if ((!isnan(pm10_serial)) && (!isnan(pm25_serial)))
      {
        //        debug_out("Both values are numbers", 1);
        if (lastwrite == 0 || millis() > lastwrite + TS_DELAY) //lastwrite jest zero gdy pierwszy raz
        {
          lastwrite = millis();

          debug_out("PM10     : " + Float2String(float(pm10_serial) / 10), 1);
          debug_out("PM2.5    : " + Float2String(float(pm25_serial) / 10), 1);
          debug_out("temp     : " + Float2String(float(bmeData.temperature)), 1);
          debug_out("humidity : " + Float2String(float(bmeData.humidity)), 1);
          debug_out("pressure : " + Float2String(float(bmeData.pressure)), 1);
          debug_out("altitude : " + Float2String(float(bmeData.altitude)), 1);

          if (client.connect(server, 80)) {
            Serial.println("Client connected!");
            String upadteData = "api_key=" + myWriteAPIKey;

            upadteData += "&field1=";
            upadteData += Float2String(float(pm10_serial) / 10);
            upadteData += "&field2=";
            upadteData += Float2String(float(pm25_serial) / 10);
            upadteData += "&field3=";
            upadteData += Float2String(float(bmeData.temperature));
            upadteData += "&field4=";
            upadteData += Float2String(float(bmeData.humidity));
            upadteData += "&field5=";
            upadteData += Float2String(float(bmeData.pressure));
            upadteData += "&field6=";
            upadteData += Float2String(float(bmeData.altitude));
            upadteData += "\r\n\r\n";

            Serial.println("wywołanie: " + upadteData);

            client.print("GET /update?" + upadteData + "HTTP/1.1");
            client.print(String(server));
            client.println("Host: " + String(server));
            client.println("Connection: close");
            client.println();

            Serial.println("%. Send to Thingspeak.");
          } else {
            Serial.println("Connection failed");
          }
          client.stop();
        }
      }
      len = 0;
      checksum_ok = 0;
      pm10_serial = 0.0;
      pm25_serial = 0.0;
      checksum_is = 0;
    }
    yield();
  }

  return s;
}

void clearSerial() {
  while (serialSDS.available()) {
    serialSDS.read();
  }
}

void setup()
{
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Serial.begin(115200);
  esp_chipid = String(ESP.getChipId());
  Serial.println("esp_chipid: " + esp_chipid);
  WiFi.persistent(false);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  int status = WL_IDLE_STATUS;
  serialSDS.begin(9600);
  delay(10);
  debug_out("\nChipId: ", 0);
  debug_out(esp_chipid, 1);

//  setup_dht();
  setupBme();
  delay(2000);
  // sometimes parallel sending data and web page will stop nodemcu, watchdogtimer set to 30 seconds
  wdt_disable();
  wdt_enable(30000); // 30 sec

  Serial.println(SDS_version_date());
  set_SDS_duty(0);

//  read_DHT();
  readBme();
  sensorSDS();
  digitalWrite(RELAY_PIN, HIGH);
  ESP.deepSleep(20 * 60 *1000000, WAKE_NO_RFCAL); // Sleep for 5 minutes (1000000 is 1 second)
}

void loop()
{}
