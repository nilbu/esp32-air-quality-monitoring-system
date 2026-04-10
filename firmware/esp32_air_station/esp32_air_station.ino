/*
  ESP32 Air Quality Station

  Features:
  - PMS5003 (PM1, PM2.5, PM10)
  - BME280 (temperature, humidity, pressure)
  - Median filter (PM)
  - EMA filter (environmental data)
  - Humidity correction
  - Sensor duty cycle + clean cycle
  - Google Sheets integration via HTTP

  Hardware:
  ESP32 DevKit
  PMS5003 (UART)
  BME280 (I2C)

  Author: Tomek B
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <time.h>

#define PMS_RX 16
#define PMS_TX 17
#define PMS_SET 4

#define BUFFER_SIZE 20
#define SAMPLE_COUNT 5

HardwareSerial pmsSerial(2);
Adafruit_BME280 bme;

const char* ssid = "";
const char* password = "";

String server = "";

float emaTemp, emaHum, emaPress;
float alpha = 0.2;
float tempOffset = 0.8;

uint16_t pm1Values[SAMPLE_COUNT];
uint16_t pm25Values[SAMPLE_COUNT];
uint16_t pm10Values[SAMPLE_COUNT];

uint16_t pm1, pm25, pm10;

String buffer[BUFFER_SIZE];
int bufferIndex = 0;

////////////////////////////////////////////////////////

void flushPMS() {
  while (pmsSerial.available()) {
    pmsSerial.read();
  }
}

////////////////////////////////////////////////////////

bool readPMS() {

  while (pmsSerial.available() >= 32) {

    if (pmsSerial.read() == 0x42 && pmsSerial.read() == 0x4D) {

      uint8_t data[30];
      pmsSerial.readBytes(data, 30);

      pm1 = (data[4] << 8) | data[5];
      pm25 = (data[6] << 8) | data[7];
      pm10 = (data[8] << 8) | data[9];

      return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////

uint16_t median5(uint16_t *v) {

  uint16_t t[5];

  for (int i = 0; i < 5; i++) t[i] = v[i];

  for (int i = 0; i < 4; i++)
    for (int j = i + 1; j < 5; j++)
      if (t[j] < t[i]) {
        uint16_t tmp = t[i];
        t[i] = t[j];
        t[j] = tmp;
      }

  return t[2];
}

////////////////////////////////////////////////////////

void ensureWiFi() {

  if (WiFi.status() == WL_CONNECTED) return;

  Serial.println("WiFi reconnecting...");

  WiFi.disconnect();
  WiFi.begin(ssid, password);

  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
}

////////////////////////////////////////////////////////

void addToBuffer(String data) {

  if (bufferIndex < BUFFER_SIZE) {
    buffer[bufferIndex] = data;
    bufferIndex++;
  }

  Serial.println("Data stored in buffer");
}

////////////////////////////////////////////////////////

void sendBuffer() {

  if (bufferIndex == 0) return;

  Serial.println("Sending buffered data");

  for (int i = 0; i < bufferIndex; i++) {

    HTTPClient http;
    http.begin(buffer[i]);

    int code = http.GET();

    Serial.print("Buffered HTTP code: ");
    Serial.println(code);

    http.end();

    delay(200);
  }

  bufferIndex = 0;
}

////////////////////////////////////////////////////////

String getTime() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "0000-00-00T00:00:00";
  }

  char timeString[25];
  strftime(timeString, 25, "%Y-%m-%dT%H:%M:%S", &timeinfo);

  return String(timeString);
}

////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);

  Serial.println();
  Serial.println("==== WEATHER STATION START ====");

  WiFi.begin(ssid, password);

  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");

  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "CET-1CEST,M3.5.0/02,M10.5.0/03", 1);
  tzset();

  Serial.print("Waiting for NTP time");

  time_t now = time(nullptr);

  while (now < 100000) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }

  Serial.println();
  Serial.println("Time synchronized");

  Wire.begin(21, 22);

  if (!bme.begin(0x76)) {
    Serial.println("BME280 ERROR");
    while (1);
  }

  Serial.println("BME280 OK");

  emaTemp = bme.readTemperature();
  emaHum = bme.readHumidity();
  emaPress = bme.readPressure() / 100.0F;

  pmsSerial.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);

  pinMode(PMS_SET, OUTPUT);
  digitalWrite(PMS_SET, LOW);

  Serial.println("PMS5003 initialized");

  Serial.println("Station ready");
}

////////////////////////////////////////////////////////

void loop() {

  Serial.println();
  Serial.println("---- CYCLE START ----");

  ensureWiFi();

  Serial.print("Time: ");
  Serial.println(getTime());

  Serial.print("WiFi RSSI: ");
  Serial.println(WiFi.RSSI());

  digitalWrite(PMS_SET, HIGH);

  Serial.println("PMS warmup");
  delay(30000);

  Serial.println("Flushing PMS buffer");
  flushPMS();

  Serial.println("Collecting PM samples");

  int samples = 0;
  unsigned long start = millis();

  while (samples < SAMPLE_COUNT && millis() - start < 10000) {

    if (readPMS()) {

      pm1Values[samples] = pm1;
      pm25Values[samples] = pm25;
      pm10Values[samples] = pm10;

      samples++;

      Serial.print("Sample ");
      Serial.print(samples);
      Serial.println(" OK");
    }
  }

  if (samples < SAMPLE_COUNT) {

    Serial.println("WARNING: PMS timeout -> restarting sensor");

    digitalWrite(PMS_SET, LOW);
    delay(2000);
    digitalWrite(PMS_SET, HIGH);
    delay(30000);

    return;
  }

  uint16_t pm1Med = median5(pm1Values);
  uint16_t pm25Med = median5(pm25Values);
  uint16_t pm10Med = median5(pm10Values);

  float hum = bme.readHumidity();
  float temp = bme.readTemperature() - tempOffset;
  float pres = bme.readPressure() / 100.0F;

  emaTemp = alpha * temp + (1 - alpha) * emaTemp;
  emaHum = alpha * hum + (1 - alpha) * emaHum;
  emaPress = alpha * pres + (1 - alpha) * emaPress;

  Serial.println("---- MEASUREMENTS ----");

  Serial.print("Temp: ");
  Serial.println(emaTemp);

  Serial.print("Hum: ");
  Serial.println(emaHum);

  Serial.print("Press: ");
  Serial.println(emaPress);

  Serial.print("PM1: ");
  Serial.println(pm1Med);

  Serial.print("PM2.5: ");
  Serial.println(pm25Med);

  Serial.print("PM10: ");
  Serial.println(pm10Med);

  Serial.println("----------------------");

  String deviceTime = getTime();

  String url = server +
               "?device_time=" + deviceTime +
               "&temp=" + String(emaTemp) +
               "&hum=" + String(emaHum) +
               "&press=" + String(emaPress) +
               "&pm1=" + String(pm1Med) +
               "&pm25=" + String(pm25Med) +
               "&pm10=" + String(pm10Med) +
               "&rssi=" + String(WiFi.RSSI());

  Serial.println("Sending HTTP request");

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;
    http.begin(url);

    int httpCode = http.GET();

    Serial.print("HTTP code: ");
    Serial.println(httpCode);

    http.end();

    sendBuffer();

  } else {

    Serial.println("WiFi not connected, buffering data");
    addToBuffer(url);
  }

  digitalWrite(PMS_SET, LOW);

  Serial.println("Cycle finished");
  Serial.println("Sleeping...");

  delay(90000);
}

