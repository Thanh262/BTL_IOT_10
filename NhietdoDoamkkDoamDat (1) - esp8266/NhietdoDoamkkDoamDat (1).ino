#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DHT22.h>
#include <ESP8266WiFiMulti.h>

#define pinDATA D2
DHT22 dht22(pinDATA);

const char* ssid = "O";
const char* password = "99999999";

const int soilmoisturePin = A0;
const int relayPin = D1;
const int soilmoistureThresholdDefault = 40;

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);

int soilmoistureThreshold = soilmoistureThresholdDefault;

void handleRoot() {
  String html = "<html><body style='padding-left: 20%;'>";
  html += "<h1 style='margin-top: 100px; padding-left: 270px; color: coral;'>He thong theo doi cay</h1>";
  html += "<p>Do am dat ban dau: " + String(soilmoistureThreshold) + "</p>";
  html += "<p>Do am dat moi: <input type='number' id='threshold' min='0' max='100' step='1' value='" + String(soilmoistureThreshold) + "'></p>";
  html += "<button onclick='setThreshold()'>Set do am dat</button>";
  html += "<div style='width: 200px; height: 100px; margin: 10px; background-color: lightblue; display: inline-block; text-align: center; line-height: 100px; border-radius: 10px;' id='status'></div>";
  html += "<div style='width: 200px; height: 100px; margin: 10px; background-color: yellowgreen; display: inline-block; text-align: center; line-height: 100px; border-radius: 10px;' id='soilMoisture'></div>";
  html += "<div style='width: 200px; height: 100px; margin: 10px; background-color: yellow; display: inline-block; text-align: center; line-height: 100px; border-radius: 10px;' id='temperature'></div>";
  html += "<div style='width: 200px; height: 100px; margin: 10px; background-color: orange; display: inline-block; text-align: center; line-height: 100px; border-radius: 10px;' id='humidity'></div>";
  html += "<script>function setThreshold() { var threshold = document.getElementById('threshold').value; fetch('/set?threshold=' + threshold).then(response => response.text()).then(data => console.log(data)); }</script>";
  html += "<script>function updateStatus() { fetch('/status').then(response => response.text()).then(data => { document.getElementById('status').innerHTML = data; }); }</script>";
  html += "<script>function updateSensorData() { fetch('/sensor-data').then(response => response.text()).then(data => { var sensorData = data.split(','); document.getElementById('soilMoisture').innerHTML = 'Soil Moisture: ' + sensorData[0] + '%'; document.getElementById('temperature').innerHTML = 'Temperature: ' + sensorData[1] + ' oC'; document.getElementById('humidity').innerHTML = 'Humidity: ' + sensorData[2] + '%'; }); }</script>";
  html += "<script>setInterval(updateStatus, 1000);</script>";  // Cập nhật trạng thái mỗi 1 giây
  html += "<script>setInterval(updateSensorData, 1000);</script>";  // Cập nhật dữ liệu cảm biến mỗi 1 giây
  
   html += "<form action='/trigger' method='get'><input type='submit' value='Link to Gallery'></form>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSetThreshold() {
  String thresholdValue = server.arg("threshold");
  soilmoistureThreshold = thresholdValue.toInt();

   // Debug print to Serial Monitor
  Serial.print("Đã nhận giá trị độ ẩm đất mới: ");
  Serial.println(soilmoistureThreshold);
  
  server.send(200, "text/plain", "Threshold set to " + String(soilmoistureThreshold));
}

void handleStatus() {
  String status = "Soil Moisture Threshold: " + String(soilmoistureThreshold) + "%";
  server.send(200, "text/plain", status);
}

void handleSensorData() {
  int soilmoistureLevel = analogRead(soilmoisturePin);
  int SoilMoisturePercentage = map(soilmoistureLevel, 0, 1023, 0, 100);

  float airtemparature = dht22.getTemperature();
  float airhumidity = dht22.getHumidity();

  String sensorData = String(SoilMoisturePercentage) + "," + String(airtemparature, 1) + "," + String(airhumidity, 1);
  server.send(200, "text/plain", sensorData);
}

void handleTrigger() {
  if (WiFiMulti.run() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "https://drive.google.com/drive/folders/11oQJjD5h4p0iehBJNscEYe720HhrAncl?usp=sharing")) {
      Serial.print("[HTTP] GET...\n");
      int httpCode = http.GET();

      if (httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.println("[HTTP] Unable to connect");
    }
  }
   server.sendHeader("Location", "https://drive.google.com/drive/folders/11oQJjD5h4p0iehBJNscEYe720HhrAncl?usp=sharing", true);
   server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  pinMode(relayPin, OUTPUT);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/set", HTTP_GET, handleSetThreshold);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/sensor-data", HTTP_GET, handleSensorData);
  server.on("/trigger", HTTP_GET, handleTrigger);

  server.begin();
}

void loop() {
  server.handleClient();

  int soilmoistureLevel = analogRead(soilmoisturePin);
  int SoilMoisturePercentage = map(soilmoistureLevel, 0, 1023, 0, 100);

  float airtemparature = dht22.getTemperature();
  float airhumidity = dht22.getHumidity();

  if (dht22.getLastError() != dht22.OK) {
    Serial.print("last error :");
    Serial.println(dht22.getLastError());
  }

  Serial.print("Air temperature = ");
  Serial.print(airtemparature, 1);
  Serial.print(" degrees Celsius \t");
  Serial.println(" ");
  Serial.print("Air humidity = ");
  Serial.print(airhumidity, 1);
  Serial.println("%");

  Serial.print("Soil Moisture Level: ");
  Serial.print(SoilMoisturePercentage);
  Serial.println("%");

  if (SoilMoisturePercentage < soilmoistureThreshold) {
    digitalWrite(relayPin, HIGH);
    Serial.println("Low soil moisture");
    Serial.println("Pumping");
    delay(5000);
    digitalWrite(relayPin, LOW);
    Serial.println("Pump deactivate");
    delay(1000);
  }

  delay(2000);
}