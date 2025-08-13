#include <HTTPClient.h>
#include <M5Stack.h>
#include "settings.h"

// ネットワーク設定
const char *ssid = SSID;
const char *password = PASSWORD;
const char *host = HOSTNAME;
const int port = PORT;
const int RESPONSE_TIMEOUT_MILLIS = 5000;

// 動作設定
const char *token = TOKEN;

void updateLatest();
bool executeScene(String sceneId);

/**
 * 初回処理
 */
void setup()
{
  // 初期化
  M5.begin();

  // LCD スクリーン初期化
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("Room Controller");

  // Wi-Fi 接続
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // 接続成功後
  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  M5.Lcd.print("IP address: ");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());

  M5.Lcd.print("Init OK.");
  delay(3000);

  // 最新の状態を取得
  updateLatest();
}

/**
 * メインループ
 */
void loop()
{
  M5.update();

  // 消灯
  if (M5.BtnA.wasReleased())
  {
    Serial.println("Scene: [OFF]");
    executeScene(SCENE_ID_OFF);
  }

  // 昼光色
  if (M5.BtnB.wasReleased())
  {
    Serial.println("Scene: [DAYLIGHT]");
    executeScene(SCENE_ID_DAYLIGHT);
  }

  // 電球色
  if (M5.BtnC.wasReleased())
  {
    Serial.println("Scene: [WARM]");
    executeScene(SCENE_ID_WARM);
  }

  updateLatest();
}

/**
 * サーバーから自機の最新の状態を取得します。
 */
void updateLatest()
{
  M5.Lcd.fillScreen(BLACK);
}

/**
 * シーンを実行します。
 */
bool executeScene(String sceneId)
{
  HTTPClient client;
  client.setTimeout(RESPONSE_TIMEOUT_MILLIS);
  String url = "https://" + String(host) + "/v1.1/scenes/" + sceneId + "/execute";

  if (!client.begin(url))
  {
    Serial.println("Connection failed.");
    return false;
  }

  client.addHeader("Content-Type", "application/json");
  client.addHeader("Authorization", token);

  int httpCode = client.POST("{}");
  client.end();

  return httpCode == 200;
}
