#include <M5Unified.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // JSON 処理ライブラリ

#include "config.h"

String formattedTemperature = "";
String formattedDate = "";
String formattedTime = "";

bool dataRequested = false; // データ取得が実行されたかフラグ

unsigned long lastDataTime = 0; // 前回のデータ取得時刻 (millis())
const unsigned long dataInterval = 600000; // データ取得間隔 (1時間)


// メッセージタイプを列挙型で定義
enum MessageType {
  CONNECTING_WIFI,
  DISCONNECTING_WIFI,
  RETRIEVING_DATA,
  HTTP_ERROR,
  WIFI_CONNECTION_FAILED,
  RESULT,
  UNKNOWN_MESSAGE
};

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setBrightness(10);
  M5.Lcd.setCursor(0, 0);

  viewerLcdSetMessage(CONNECTING_WIFI);

  Serial.begin(115200);
  Serial.print("Connecting...");

  // pinMode(M5.BtnA.pin(), INPUT_PULLUP);

  // WiFi 接続
  while (!viewerWifiCtrl(true)){
    // WiFi 接続に失敗した場合、メッセージを表示
    viewerLcdSetMessage(WIFI_CONNECTION_FAILED);
    Serial.println("WiFi connection failed");
    delay(1000);
    viewerLcdSetMessage(CONNECTING_WIFI);
  }

  // GAS Web API を呼び出す
  // getGasData();
  getGasData_gemini();
  dataRequested = true;
  viewerWifiCtrl(false); // WiFi 切断

}

void loop() {

  M5.update();
  if (M5.BtnA.wasClicked()) {
    // ボタンが押された
    Serial.println("BtnA Pressed");

    viewerLcdSetMessage(CONNECTING_WIFI);

    while (!viewerWifiCtrl(true)){
      viewerLcdSetMessage(WIFI_CONNECTION_FAILED);
      Serial.println("WiFi connection failed");
      delay(1000);
      viewerLcdSetMessage(CONNECTING_WIFI);
    }

    if (dataRequested) { // 初回データ取得が完了していれば再取得
      viewerLcdSetMessage(RETRIEVING_DATA);
      getGasData_gemini();
    }
    // チャタリング防止
    delay(500); // 0.5秒待つ
    viewerWifiCtrl(false); // WiFi 切断
  }

}




bool viewerWifiCtrl(boolean wifiCtrl) {

  if (wifiCtrl == true) {
    // WiFi - connect
    WiFi.begin(ssid, password);
    uint16_t waitingCount = 0;
    while (WiFi.status() != WL_CONNECTED && waitingCount < 20) {
      delay(500);
      M5.Lcd.print(".");
      waitingCount++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection failed");
      return false;
    }

  } else {
    // WiFi - disconnect
    WiFi.disconnect();
    uint16_t waitingCount = 0;
    while (WiFi.status() == WL_CONNECTED && waitingCount < 20) {
      delay(500);
      waitingCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi disconnection failed");
      return false;
    }
  }

  return true;
}

void viewerLcdSetMessage(MessageType messageType) {
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);

  switch (messageType) {
    case CONNECTING_WIFI:
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.println("Connecting to WiFi...");
      break;
    case DISCONNECTING_WIFI:
      M5.Lcd.setTextColor(TFT_WHITE);
      M5.Lcd.println("Disconnecting WiFi...");
      break;
    case RETRIEVING_DATA:
      M5.Lcd.setTextColor(TFT_CYAN);
      M5.Lcd.println("Retrieving data...");
      break;
    case HTTP_ERROR:
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("HTTP Error!\nTry again.");
      break;
    case WIFI_CONNECTION_FAILED:
      M5.Lcd.setTextColor(TFT_ORANGE);
      M5.Lcd.println("WiFi connection failed.");
      break;
    case RESULT:
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.printf("%s\n", formattedDate.c_str());
      M5.Lcd.setCursor(60, 20);
      M5.Lcd.printf("%s\n", formattedTime.c_str());

      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(0, 70);
      M5.Lcd.setTextColor(GREEN);
      M5.Lcd.printf("%s C\n", formattedTemperature.c_str());
      break;
    default:
      M5.Lcd.setTextColor(TFT_RED);
      M5.Lcd.println("Unknown message type");
      break;
  }
}


void getGasData_gemini(){

  viewerLcdSetMessage(RETRIEVING_DATA);
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  String currentURL = GAS_URL;
  int redirectCount = 0;
  const int maxRedirects = 5;
  const char* headers[] = {"location"}; // リダイレクトがあった場合、このヘッダにリダイレクト先が入っている。

  while (redirectCount < maxRedirects) {
    Serial.printf("Connecting to: %s\n", currentURL.c_str());
    http.begin(client, currentURL);
    http.collectHeaders(headers, 1);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_FOUND) { // 302 Found - redirect
      Serial.println("Received 302 Redirect");
      String locationHeader = http.header("location");
      http.end(); // 切断

      if (locationHeader.length() > 0) {
        Serial.printf("Redirecting to: %s\n", locationHeader.c_str());
        currentURL = locationHeader;
        redirectCount++;
        delay(100);
      } else {
        Serial.println("Error: Location header not found in redirect response");
        break;
      }
    } else if (httpCode == HTTP_CODE_OK) { // get JSON data
      Serial.println("Successfully retrieved data");
      String payload = http.getString();
      Serial.println(payload);

      // JSON を解析
      DynamicJsonDocument doc(1024); // 必要に応じてサイズを調整
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("JSON Deserialization failed: ");
        Serial.println(error.c_str());
        break;
      }

      // JSON から timestamp と temperature を取得
      if (doc.containsKey("timestamp") && doc.containsKey("temperature")) {
        String timestamp = doc["timestamp"].as<String>();
        String temperatureStr = doc["temperature"].as<String>(); // 一旦文字列として取得

        // timestamp を整形
        String year = timestamp.substring(0, 4);
        String month = timestamp.substring(5, 7);
        String day = timestamp.substring(8, 10);
        String hour = timestamp.substring(11, 13);
        String minute = timestamp.substring(14, 16);
        formattedDate = year + "/" + month + "/" + day + " ";
        formattedTime  = hour + ":" + minute;

        char stringBuffer[10];
        dtostrf(temperatureStr.toFloat(), 3, 1, stringBuffer);
        temperatureStr = String(stringBuffer); // char配列からString型へ変換

        if (temperatureStr.toFloat() >= 0) {
          temperatureStr = "+" + temperatureStr; // 符号を追加
        }
        formattedTemperature = temperatureStr; 

        viewerLcdSetMessage(RESULT);

        Serial.println("\nData from GAS:");
        Serial.printf("Timestamp (Raw): %s\n", timestamp.c_str());
        Serial.printf("Timestamp (Formatted): %s\n", formattedDate.c_str());
        Serial.printf("Temperature (Raw): %s\n", temperatureStr.c_str());
        Serial.printf("Temperature (Formatted): %s\n", formattedTemperature.c_str());
      } else {
        Serial.println("\nError: 'timestamp' or 'temperature' key not found in JSON");
      }
      break;
    } else {
      Serial.printf("HTTP Error Code: %d\n", httpCode);
      viewerLcdSetMessage(HTTP_ERROR);
      break;
    }
    http.end();
  }

  if (redirectCount >= maxRedirects) {
    Serial.println("Error: Too many redirects");
  }
  

}


void getGasData() {
  HTTPClient http;
  WiFiClientSecure client;
  client.setInsecure();

  http.begin(client, GAS_URL);

    //send a GET request
    http.GET();

    //receive a response
    String response = http.getString();
    Serial.println("Done");
    Serial.println(response);

  http.end();

  //redirect
  int from = response.indexOf("<A");
  int to = response.indexOf("A>");
  String destination = response.substring(from + 9, to - 8);
  destination.replace("&amp;", "&");

  Serial.println("RedirectURL:");
  Serial.println(destination);

  http.begin(client, destination);
    http.GET();

    //receive a response
    response = http.getString();
    Serial.println("Done");
    Serial.println(response);

    //close the connection
  http.end();
}