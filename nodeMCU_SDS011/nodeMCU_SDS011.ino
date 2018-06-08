unsigned long myChannelNumber = 0;
const char *myWriteAPIKey = "XXX";

char ssid[] = "***";
char pass[] = "***";

#define TS_DELAY 60 * 1000
#include <SoftwareSerial.h>
//#include <ThingSpeak.h>

#include <ESP8266WiFi.h>
#include "dht_sensor.h"
#include <SoftwareSerial.h>

bool is_SDS_running = true;

#define SDS_PIN_RX D1
#define SDS_PIN_TX D2

SoftwareSerial serialSDS(SDS_PIN_RX, SDS_PIN_TX, false, 1024);

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
  /* start SDS011 sensor                                           *
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
  /* stop SDS011 sensor                                            *
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
  /* read SDS011 sensor values                                     *
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

  debug_out(F("Start reading SDS011 version date"), 1);

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

  debug_out(F("End reading SDS011 version date"), 1);

  return s;
}

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
          debug_out("temp     : " + Float2String(float(DHT_readings.temp)), 1);
          debug_out("humidity : " + Float2String(float(DHT_readings.hum)), 1);
          //          ThingSpeak.setField(1, float(pm10_serial) / 10);
          //          ThingSpeak.setField(2, float(pm25_serial) / 10);
          //          ThingSpeak.setField(3, DHT_readings.temp);
          //          ThingSpeak.setField(4, DHT_readings.hum);
          //          ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
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

WiFiClient client;

void clearSerial() {
  while (serialSDS.available()) {
    serialSDS.read();
  }
}

void setup()
{
  //  analogReference(EXTERNAL);
  Serial.begin(115200); // Output to Serial at 9600 baud
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
  //  ThingSpeak.begin(client);
  serialSDS.begin(9600);
  delay(10);
  debug_out("\nChipId: ", 0);
  debug_out(esp_chipid, 1);

  setup_dht();
  delay(2000);
  // sometimes parallel sending data and web page will stop nodemcu, watchdogtimer set to 30 seconds
  wdt_disable();
  wdt_enable(30000); // 30 sec

  Serial.println(SDS_version_date());
  set_SDS_duty(0);
}

void loop()
{
  read_DHT();
  sensorSDS();
  ESP.deepSleep(60000000, WAKE_NO_RFCAL); // Sleep for 60 seconds
}
