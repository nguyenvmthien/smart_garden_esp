#include <DHTesp.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char *ssid = "Wokwi-GUEST";
const char *password = "";

const String BOT_TOKEN = "7171674667:AAGaMK5_sw0E4Dx7rI8YH3l5ZB1uHsCiXWE"; // Token của bot Telegram
const String CHANNEL_ID = "1501781748";

// Cài đặt chân
const int trigPin = 4;
const int echoPin = 0;
const int photoResistorPin = 32;
DHTesp dhtSensor;
const int dhtPin = 17;
const int lightPin = 25;
const int fanPin = 26;
const int waterPin = 27;
const int heaterPin = 14;
const int pumpPin = 12;
const int wifiLedPin = 16;

// Ngưỡng cài đặt
double minTemp = 25.0;
double maxTemp = 30.0;
double minHumid = 40.0;
double maxHumid = 60.0;
double minWaterLevel = 0.1;
double maxWaterLevel = 0.85;
double minBrightness = 2000.0;
double maxBrightness = 5000.0;

int defaultWateringDuration = 10000; // 10 giây
int defaultHeaterDuration = 10000;   // 10 giây
int defaultFanDuration = 10000;      // 10 giây

double tankHeight = 1.5;

// Biến thời gian
unsigned long lastWateringTime = 0;
unsigned long lastHeaterTime = 0;
unsigned long lastFanTime = 0;

  void wifiConnect()
  {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    digitalWrite(wifiLedPin, HIGH);
    delay(2000);
    digitalWrite(wifiLedPin, LOW);
    Serial.println(" Connected!");
  }

void sendTelegramMessage(String message)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + BOT_TOKEN +
                 "/sendMessage?chat_id=" + CHANNEL_ID +
                 "&text=" + message;

    http.begin(url);
    int httpCode = http.GET();

    if (httpCode > 0)
    {
      Serial.println("Telegram message sent successfully!");
    }
    else
    {
      Serial.println("Error sending message: " + http.errorToString(httpCode));
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi not connected!");
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("ESP32 Automation System Starting...");

  // Cài đặt chân
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(lightPin, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(waterPin, OUTPUT);
  pinMode(heaterPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(wifiLedPin, OUTPUT);
  pinMode(photoResistorPin, INPUT);

  // Cài đặt cảm biến DHT
  dhtSensor.setup(dhtPin, DHTesp::DHT22);

  wifiConnect();
}

double getDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  double distanceM = duration * 0.034 / 200.0;
  return distanceM;
}

double getBrightness()
{
  int analogValue = analogRead(photoResistorPin);
  double voltage = analogValue / 4096.0 * 5;
  double resistance = 2000 * voltage / (1 - voltage / 5);
  double lux = pow(50 * 1e3 * pow(10, 0.7) / resistance, (1 / 0.7));
  return lux;
}

void loop()
{
  unsigned long currentMillis = millis();

  // Đọc dữ liệu cảm biến
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  double brightness = getBrightness();
  double waterLevel = tankHeight - getDistance();

  // Hiển thị thông số
  Serial.println("Temperature: " + String(data.temperature) + "°C");
  Serial.println("Humidity: " + String(data.humidity) + "%");
  Serial.println("Brightness: " + String(brightness) + " lux");
  Serial.println("Water Level: " + String(waterLevel) + " m");
  Serial.println("----------------------------");

  // Kiểm tra và bật hệ thống nước
  if (data.temperature >= maxTemp || data.humidity <= minHumid)
  {         
    if (currentMillis - lastWateringTime >= defaultWateringDuration)
    {
      Serial.println("Watering: OFF");
      digitalWrite(waterPin, LOW);
      lastWateringTime = currentMillis;
    }
    else
    {
      digitalWrite(waterPin, HIGH);
      Serial.println("Watering: ON");
    }
  }
  else
  {
    if (currentMillis - lastWateringTime >= defaultWateringDuration)
    {
      Serial.println("Watering: OFF");
      digitalWrite(waterPin, LOW);
      lastWateringTime = currentMillis;
    }
  }

  // Kiểm tra và bật máy bơm
  if (waterLevel <= tankHeight * minWaterLevel)
  {
    Serial.println("Warning: Water level too low!");
    digitalWrite(pumpPin, HIGH);
  }
  else if (waterLevel >= tankHeight * maxWaterLevel)
  {
    digitalWrite(pumpPin, LOW);
  }

  // Kiểm tra và bật máy sưởi
  if (data.temperature <= minTemp)
  {
    if (currentMillis - lastHeaterTime >= defaultHeaterDuration)
    {
      Serial.println("Heater: OFF");
      digitalWrite(heaterPin, LOW);
      lastHeaterTime = currentMillis;
    }
    else
    {
      digitalWrite(heaterPin, HIGH);
      Serial.println("Heater: ON");
    }
  }
  else
  {
    if (currentMillis - lastHeaterTime >= defaultHeaterDuration)
    {
      Serial.println("Heater: OFF");
      digitalWrite(heaterPin, LOW);
      lastHeaterTime = currentMillis;
    }
  }

  // Kiểm tra và bật quạt
  if (data.humidity >= maxHumid)
  {
    if (currentMillis - lastFanTime >= defaultFanDuration)
    {
      Serial.println("Fan: OFF");
      digitalWrite(fanPin, LOW);
      lastFanTime = currentMillis;
    }
    else
    {
      digitalWrite(fanPin, HIGH);
      Serial.println("Fan: ON");
    }
  }
  else
  {
    if (currentMillis - lastFanTime >= defaultFanDuration)
    {
      Serial.println("Fan: OFF");
      digitalWrite(fanPin, LOW);
      lastFanTime = currentMillis;
    }
  }

  // Kiểm tra và bật đèn
  // --ON-- 2000 --ON-- 5000 --OFF--
  if (brightness <= minBrightness)
  {
    Serial.println("Light: ON");
    digitalWrite(lightPin, HIGH);
  }
  else if (brightness >= minBrightness)
  {
    Serial.println("Light: OFF");
    digitalWrite(lightPin, LOW);
  }
  delay(1000);
}
