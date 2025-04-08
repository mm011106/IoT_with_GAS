#define FASTLED_INTERNAL
// 
// AtomS3Lite_EnvIoT_withGAS.ino
//    2025/4/8    miyamoto
// 
// M5Stack AtomS3Lite＋ENV4による 環境計測
//    コントローラ（マイコン）はM5Stack社 AtomS3Liteを使用
//    温湿度、気圧センサとして同社の ENV.IV を使用
// 
// 機能：
//    10分ごとにconfig.hに記載されたwifiに接続し
//    センサからの環境データをGASで構成したwebAPIに送る
// 
//    バッテリ駆動での安定性を検証中

#include <M5Unified.h>
// #include <M5AtomS3.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_SHT4x.h>
#include <Adafruit_BMP280.h> // 気圧センサ用のライブラリを追加
#include "config.h"  // 新しい設定ファイルをインクルード

constexpr bool SLEEP_AND_REPEAT = true;
constexpr uint16_t SLEEP_DURATION = 10;  //スリープ時間設定 [min]
constexpr int16_t SLEEP_DURATION_ADJUST = 0; // スリープ時間調整 [ms]

// 開発中：バッテリ動作での安定度を検討
//    AAAx2で動作させる ２〜３V
//    DC-DCコンバータ使用　5V出力
//    バッテリの電圧をモニタして過放電を防ぐ
constexpr bool BATTERY_OPERATION = true;
constexpr int16_t BATTERY_MONITOR_PIN = 8; // バッテリ電圧モニタ用アナログ入力端子

Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_BMP280 bmp;  // 気圧センサのインスタンスを作成

uint32_t timestamp = 0; // 測定時間を計測して保存する[ms]

void post(const char* sheet_name, float temperature, float humidity, float pressure) {
  HTTPClient http;
  http.begin(GAS_URL);

  http.addHeader("Content-Type", "application/json");
  
  // JSONペイロードを更新
  String payload = "{\"sheet_name\":\"" + String(sheet_name) + "\",\"temperature\":" + String(temperature) + 
                   ",\"humidity\":" + String(humidity) + 
                   ",\"pressure\":" + String(pressure) + "}";
  Serial.println(payload);
  
  http.POST(payload);
  // Serial.println(http.getString());
  http.end();
  return;
}

// バッテリの電圧を読む
float read_battery_level(void){
  uint32_t adcRawData = 0;
  // ADCの読み値を10回平均する
  adcRawData = analogRead(BATTERY_MONITOR_PIN);
  for (uint16_t i = 0; i<9; i++){
    adcRawData = adcRawData + analogRead(BATTERY_MONITOR_PIN) ;  
  }  
  // ADCの入力電圧は 1/2 * V_battery なので2倍している
  return 2.0 * (3.3 / 4095) * static_cast<float>(adcRawData)/10.0 ;  //アナログ電圧（0～3.3V）換算
}

void setup() {
  timestamp = millis();

  // M5.begin(); // enable function of  USB-Serial, I2C, LED
  // M5.Lcd.sleep();; //turn off the LCD backlight

  auto cfg = M5.config();
  M5.begin(cfg);

  Serial.begin(115200);
  Wire.begin(2,1);  //I2Cで使うPINを指定 SH4xだけ使う場合は必要
  Serial.println("Env measure IoT");
  
  pinMode(BATTERY_MONITOR_PIN, ANALOG);

  // M5.Lcd.begin(); // LCDディスプレイの初期化
  // M5.Lcd.printf("Initializing...\n"); // 任意のテキストを表示
  // delay(1000); // テキストを表示するための短い遅延

  // env sensor initialize
  Serial.println("Sensor: SHT4x");
  if (! sht4.begin()) {
    Serial.println("Couldn't find SHT4x");
    while (1) delay(1);
  }
  Serial.println("Found SHT4x sensor");
  Serial.print("Serial number 0x");
  Serial.println(sht4.readSerial(), HEX);

  // Precision, heater settings
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  // 気圧センサの初期化
  Serial.println("Sensor: BMP280");
  if (!bmp.begin(0x76)) {  // I2Cアドレスを指定
    Serial.println("Couldn't find BMP280");
    while (1) delay(1);
  }
  Serial.println("Found BMP280 sensor");

  // wifi setup with timeout
  //    30秒間トライしてその間にwifiが繋がらなければ何もしないで終了する
  // 
  Serial.print("WiFi connecting to :"); Serial.println(ssid);
  WiFi.begin(ssid, password);

  uint32_t startAttemptTime = millis();
  constexpr uint32_t timeout = 30000; // 30 seconds timeout
  bool wifiConnected = false;

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startAttemptTime > timeout) {
      Serial.println("\n\nWiFi connection failed: Timeout");
      wifiConnected = false;
      break;
    }
    Serial.print(".");
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\nWiFi connected");
    wifiConnected = true;
  }

  if (wifiConnected){
    // env measurement
    // 環境測定
    sensors_event_t humidity_event, temp_event;
    sht4.getEvent(&humidity_event, &temp_event);  // 温度と湿度のデータを取得
    float pressure = bmp.readPressure() / 100.0F;  // 気圧データを取得（hPaに変換）

    Serial.print("Temperature: "); Serial.print(temp_event.temperature); Serial.println(" degrees C");
    Serial.print("Humidity: "); Serial.print(humidity_event.relative_humidity); Serial.println("% rH");
    Serial.print("Pressure: "); Serial.print(pressure); Serial.println(" hPa");
    
    // バッテリの電圧を測定（デバグ用）
    if (BATTERY_OPERATION){
      float v_battery = read_battery_level();
      Serial.print("Battery: "); Serial.print(v_battery); Serial.println(" V");
      post(SHEET_NAME, temp_event.temperature, humidity_event.relative_humidity, v_battery);  // データを送信 気圧の代わりにバッテリ電圧を出力
    } else {
      post(SHEET_NAME, temp_event.temperature, humidity_event.relative_humidity, pressure);  // データを送信
    }

  }

  if (BATTERY_OPERATION){
    float v_battery = 0.0;
    // check battery level
    v_battery = read_battery_level();
    Serial.printf("Vbattery = %4.2fV\n", v_battery);

    // バッテリの終止電圧は1cellあたり1.0V。　2cellを想定すると2.0V。
    // 少し余裕を持って1.1V/cellで測定しないようにする。
    if (v_battery < 2.2){
      Serial.printf("Low Battery!!\n");
      // esp_deep_sleep_start();
    }
  }

  timestamp = millis() - timestamp;
  Serial.print("Runtime[ms]: "); Serial.println(timestamp);
}

void loop() {
  uint32_t sleep_time_in_us = (SLEEP_DURATION * 60 * 1000 - timestamp) * 1000 + SLEEP_DURATION_ADJUST * 1000;
  Serial.print("going to sleep for ");Serial.println(sleep_time_in_us);
  if (SLEEP_AND_REPEAT){
    esp_sleep_enable_timer_wakeup(sleep_time_in_us);  // deep sleep for SLEEP_DULATION
    esp_deep_sleep_start();    
  }
  //  測定にかかっている時間が毎回違うので、スリープ時間から測定時間を差し引いて設定しないと正確じゃ無い。

                     //AN = デジタルデータ　VOLT = アナログ入力電圧

  delay(1000);

 }


// https://qiita.com/mitsuoka0423/items/bc8bd589442b5c2279a7