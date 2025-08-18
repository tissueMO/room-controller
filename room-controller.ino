#include <HTTPClient.h>
#include <M5Stack.h>
#include "settings.h"

// ネットワーク設定
const char *ssid = SSID;
const char *password = PASSWORD;
const int port = PORT;
const int RESPONSE_TIMEOUT_MILLIS = 5000;

// 動作設定
const int PRESSED_DURATION = 100;
const int FETCH_INTERVAL = 60 * 60 * 1000;
unsigned long lastMillis;
int lastElectricity = -1;

// 描画
TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);

// 関数宣言
void process();
void draw();
bool executeScene(String sceneId);
int fetchElectricity(String deviceId);

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
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Room Controller");

  // ダブルバッファ用スプライト作成
  canvas.setColorDepth(8);
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());

  // Wi-Fi 接続
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP: ");
  M5.Lcd.print("IP: ");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());

  // 時刻同期
  configTzTime("JST-9", "ntp.nict.jp");
  Serial.println("NTP OK.");
  M5.Lcd.println("NTP OK.");
  lastMillis = 0;

  M5.Lcd.print("Init OK.");
  delay(2000);
}

/**
 * メインループ
 */
void loop()
{
  M5.update();
  process();
  delay(1);
}

/**
 * ボタン操作と定期処理を実行します。
 */
void process()
{
  bool handled = false;

  // 消灯
  if (M5.BtnA.pressedFor(PRESSED_DURATION))
  {
    Serial.println("Scene: [OFF]");
    M5.Lcd.fillScreen(BLACK);
    executeScene(SCENE_ID_OFF);
    handled = true;
  }

  // 昼光色
  if (M5.BtnB.pressedFor(PRESSED_DURATION))
  {
    Serial.println("Scene: [DAYLIGHT]");
    M5.Lcd.fillScreen(BLACK);
    executeScene(SCENE_ID_DAYLIGHT);
    handled = true;
  }

  // 電球色
  if (M5.BtnC.pressedFor(PRESSED_DURATION))
  {
    Serial.println("Scene: [WARM]");
    M5.Lcd.fillScreen(BLACK);
    executeScene(SCENE_ID_WARM);
    handled = true;
  }

  // デバイス電力量更新
  if (lastMillis == 0 || millis() - lastMillis > FETCH_INTERVAL)
  {
    Serial.println("Electricity: Fetching...");
    lastMillis = millis();

    String deviceId;
    struct tm currentTime;
    if (getLocalTime(&currentTime))
    {
      deviceId = String((6 <= (currentTime.tm_mon + 1) && (currentTime.tm_mon + 1) <= 10) ? SUMMAR_DEVICE_ID : WINTER_DEVICE_ID);
      lastElectricity = fetchElectricity(deviceId);
    }
    else
    {
      lastElectricity = -1;
    }

    Serial.printf("Electricity: [%s] %d Wh\n", deviceId.c_str(), lastElectricity);
    handled = true;
  }

  // 処理が発生したときだけ再描画する
  if (handled)
  {
    draw();
  }
}

/**
 * サーバーから自機の最新の状態を取得します。
 */
void draw()
{
  canvas.fillSprite(BLACK);
  canvas.setTextColor(WHITE);

  // デバイス名
  String device;
  struct tm currentTime;
  if (getLocalTime(&currentTime) && lastElectricity != -1)
  {
    device = String((6 <= (currentTime.tm_mon + 1) && (currentTime.tm_mon + 1) <= 10) ? "[A/C]" : "[Heater]");
  }
  else
  {
    device = "**ERROR**";
  }

  // デバイス電力量
  char buf[100];
  if (lastElectricity != -1)
  {
    sprintf(buf, "%.2f", lastElectricity / 1000.0);
  }
  else
  {
    sprintf(buf, "---");
  }

  canvas.setTextSize(2);
  canvas.drawString(device, 0, 0, 2);
  canvas.drawCentreString(buf, 320 / 2, 55, 7);
  canvas.drawRightString("kWh", 320 - 20, 160, 2);

  // ボタンガイド
  canvas.setTextFont(1);
  canvas.setTextSize(2);
  canvas.drawLine(0, 240 - 18 - 10, 320, 240 - 18 - 10, WHITE);
  canvas.drawCentreString("OFF", 320 / 2 - (int)(320 / 3), 240 - 18, 1);
  canvas.drawLine((int)(320 / 3) * 1, 240 - 18 - 10, (int)(320 / 3) * 1, 240, WHITE);
  canvas.drawCentreString("WHITE", 320 / 2, 240 - 18, 1);
  canvas.drawLine((int)(320 / 3) * 2, 240 - 18 - 10, (int)(320 / 3) * 2, 240, WHITE);
  canvas.drawCentreString("WARM", 320 / 2 + (int)(320 / 3), 240 - 18, 1);

  canvas.pushSprite(0, 0);
}

/**
 * (SwitchBot API) シーンを実行します。
 */
bool executeScene(String sceneId)
{
  HTTPClient client;
  client.setTimeout(RESPONSE_TIMEOUT_MILLIS);
  String url = "https://" + String(SWITCHBOT_API_HOSTNAME) + "/v1.1/scenes/" + sceneId + "/execute";

  if (!client.begin(url))
  {
    Serial.println("Connection failed.");
    return false;
  }

  client.addHeader("Content-Type", "application/json");
  client.addHeader("Authorization", SWITCHBOT_API_TOKEN);

  int httpCode = client.POST("{}");
  client.end();

  return httpCode == HTTP_CODE_OK;
}

/**
 * (独自API) 指定したデバイスの電力量(Wh)を取得します。
 */
int fetchElectricity(String deviceId)
{
  HTTPClient client;
  client.setTimeout(RESPONSE_TIMEOUT_MILLIS);
  String url = "https://" + String(MYSELF_API_HOSTNAME) + "/?token=" + String(MYSELF_API_TOKEN) + "&device_id=" + deviceId;

  if (!client.begin(url))
  {
    Serial.println("Connection failed.");
    return -1;
  }

  int httpCode = client.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    Serial.printf("Error: Fetch Failed [%d]\n", httpCode);
    client.end();
    return -1;
  }

  String watt = client.getString();
  client.end();

  return watt.toInt();
}
