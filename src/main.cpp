#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

// Kết nối WiFi
const char *ssid = "Wokwi-GUEST";
const char *password = "";

// Set server
const char *mqttServer = "66e7cd73955d41119db073a43925b11b.s1.eu.hivemq.cloud";
int port = 8883;
const char *mqttUser = "smart-garden-esp32";
const char *mqttPassword = "Username1";

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

const int wifiLedPin = 16;
void wifiConnect()
{
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  digitalWrite(wifiLedPin, HIGH);
  delay(5000);
  digitalWrite(wifiLedPin, LOW);
}

void mqttConnect()
{
  while (!mqttClient.connected())
  {
    Serial.println("Attempting MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (mqttClient.connect(clientId.c_str(), mqttUser, mqttPassword))
    {
      Serial.println("MQTT connected");
      mqttClient.subscribe("smart-garden/#");
    }
    else
    {
      Serial.print("MQTT connection failed, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

// MQTT Receiver
void callback(char *topic, byte *message, unsigned int length)
{
  Serial.println(topic);
  String strMsg;
  for (int i = 0; i < length; i++)
  {
    strMsg += (char)message[i];
  }
  Serial.println(strMsg);

  //***Code here to process the received package***
}

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

void setup()
{
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

  Serial.begin(115200);
  Serial.println("ESP32 Automation System Starting...");
  Serial.println("Connecting to WiFi...");
  wifiConnect();
  // Cấu hình MQTT Client
  wifiClient.setInsecure();
  mqttClient.setServer(mqttServer, port);

  // Kết nối MQTT
  mqttConnect();
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
  // Nhận ngưỡng từ web
  // ...

  unsigned long currentMillis = millis();

  // Đọc dữ liệu cảm biến
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  double brightness = getBrightness();
  double waterLevel = tankHeight - getDistance() > 0 ? tankHeight - getDistance() : 0;

  // Hiển thị thông số
  Serial.println("Temperature: " + String(data.temperature) + "°C");
  Serial.println("Humidity: " + String(data.humidity) + "%");
  Serial.println("Brightness: " + String(brightness) + " lux");
  Serial.println("Water Level: " + String(waterLevel) + " m");
  Serial.println("----------------------------");

  // Kiểm tra và bật hệ thống nước
  if (data.temperature >= maxTemp || data.humidity <= minHumid)
  {
    Serial.println(currentMillis - lastWateringTime);
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

  //***Check connection to MQTT Server***
  if (!mqttClient.connected())
  {
    mqttConnect();
  }
  mqttClient.loop();

  //***Publish data to MQTT Server***
  if (!isnan(data.temperature))
  {
    char tempBuffer[50];
    sprintf(tempBuffer, "%.1f", data.temperature);
    mqttClient.publish("smart-garden/temperature", tempBuffer);
  }
  if (!isnan(data.humidity))
  {
    char tempBuffer[50];
    sprintf(tempBuffer, "%.1f", data.humidity);
    mqttClient.publish("smart-garden/humidity", tempBuffer);
  }
  if (!isnan(brightness))
  {
    char tempBuffer[50];
    sprintf(tempBuffer, "%.1f", brightness);
    mqttClient.publish("smart-garden/brightness", tempBuffer);
    Serial.println("Brightness: " + String(brightness) + " lux");
  }
  if (!isnan(waterLevel))
  {
    char tempBuffer[50];
    sprintf(tempBuffer, "%.1f", waterLevel);
    mqttClient.publish("smart-garden/water-level", tempBuffer);
  }
  delay(1000);
}