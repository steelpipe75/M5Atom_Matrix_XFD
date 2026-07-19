#include <M5Unified.h>
#include <FastLED.h>
#include <WiFi.h>
#include <WebServer.h>
#include "BluetoothSerial.h"

#define LED_PIN     27
#define NUM_LEDS    25
#define RELAY_PIN   26 // Grove端子の黄色の線(G26)
#define BT_DEVICE_NAME "M5Atom_Matrix_XFD"
#define MAX_BRIGHTNESS 20
#define MIN_BRIGHTNESS 1

// WiFi設定は別ファイルから読み込む
#include "wifi_config.h"

WebServer server(80);
BluetoothSerial SerialBT;

CRGB leds[NUM_LEDS];
bool green = false;
int currentBrightness = 3;

// 動作モードの定義
enum AppMode {
  MODE_SHOW_IP, // IPアドレス表示モード
  MODE_NORMAL   // 通常の赤・緑表示モード
};
AppMode currentMode = MODE_SHOW_IP;

// 5x5フォントデータ (数字 0-9 と ピリオド .)
const uint8_t font5x5[11][5] = {
  {0x0E, 0x11, 0x11, 0x11, 0x0E}, // 0
  {0x04, 0x0C, 0x04, 0x04, 0x0E}, // 1
  {0x0E, 0x11, 0x02, 0x04, 0x1F}, // 2
  {0x0E, 0x11, 0x06, 0x11, 0x0E}, // 3
  {0x02, 0x06, 0x0A, 0x1F, 0x02}, // 4
  {0x1F, 0x10, 0x1E, 0x01, 0x1E}, // 5
  {0x0E, 0x10, 0x1E, 0x11, 0x0E}, // 6
  {0x1F, 0x01, 0x02, 0x04, 0x04}, // 7
  {0x1F, 0x11, 0x1F, 0x11, 0x1F}, // 8
  {0x0E, 0x11, 0x0F, 0x01, 0x0E}, // 9
  {0x00, 0x00, 0x00, 0x00, 0x04}  // .
};

// 1文字をLEDマトリクスに描画する関数
void drawChar(char c, CRGB color) {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  int idx = -1;
  if (c >= '0' && c <= '9') {
    idx = c - '0';
  } else if (c == '.') {
    idx = 10;
  }
  
  if (idx >= 0) {
    for (int y = 0; y < 5; y++) {
      for (int x = 0; x < 5; x++) {
        // ビットが立っている箇所を指定色で点灯
        if (font5x5[idx][y] & (1 << (4 - x))) {
          leds[y * 5 + x] = color;
        }
      }
    }
  }
  FastLED.show();
}

void updateState(bool newState) {
  green = newState;
  
  // リレーのON/OFF切り替え
  digitalWrite(RELAY_PIN, green ? HIGH : LOW);

  // 通常モード時のみLEDを更新する
  if (currentMode == MODE_NORMAL) {
    fill_solid(
      leds,
      NUM_LEDS,
      green ? CRGB::Green : CRGB::Red
    );
    FastLED.show();
  }

  // シリアル出力
  Serial.println(green ? "State = 1 (Green)" : "State = 0 (Red)");
  if (SerialBT.hasClient()) {
    SerialBT.println(green ? "State = 1 (Green)" : "State = 0 (Red)");
  }
}

#include "index_html.h"

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleGetState() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "1" || state == "G" || state == "g") {
      updateState(true);
    } else if (state == "0" || state == "R" || state == "r") {
      updateState(false);
    }
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/plain", green ? "State = 1 (Green)" : "State = 0 (Red)");
}

void handlePostState() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    body.trim();
    if (body == "1" || body == "G" || body == "g") {
      updateState(true);
      server.send(200, "text/plain", "OK");
    } else if (body == "0" || body == "R" || body == "r") {
      updateState(false);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid state in body");
    }
  } 
  else if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "1" || state == "G" || state == "g") {
      updateState(true);
      server.send(200, "text/plain", "OK");
    } else if (state == "0" || state == "R" || state == "r") {
      updateState(false);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid state parameter");
    }
  } else {
    server.send(400, "text/plain", "Bad Request: No state provided");
  }
}

void handleGetBrightness() {
  server.send(200, "text/plain", "Brightness = " + String(currentBrightness));
}

void handlePostBrightness() {
  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    body.trim();
    int val = body.toInt();
    if (val >= MIN_BRIGHTNESS && val <= MAX_BRIGHTNESS) {
      currentBrightness = val;
      FastLED.setBrightness(currentBrightness);
      FastLED.show();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid brightness in body");
    }
  } else if (server.hasArg("brightness")) {
    String valStr = server.arg("brightness");
    int val = valStr.toInt();
    if (val >= MIN_BRIGHTNESS && val <= MAX_BRIGHTNESS) {
      currentBrightness = val;
      FastLED.setBrightness(currentBrightness);
      FastLED.show();
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Invalid brightness parameter");
    }
  } else {
    server.send(400, "text/plain", "Bad Request: No brightness provided");
  }
}

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // BluetoothSerial 初期化
  SerialBT.begin(BT_DEVICE_NAME);

  // 明示的にシリアル通信を初期化し、少し待つ
  Serial.begin(115200);
  delay(100);

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(currentBrightness); // まぶしすぎないように明るさを調整

  pinMode(RELAY_PIN, OUTPUT);
  
  // 初期状態の設定 (green = false なので OFF 状態からスタート)
  updateState(false);

  // WiFi接続
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // WebServerのエンドポイント設定
  server.on("/", HTTP_GET, handleRoot);
  server.on("/state", HTTP_GET, handleGetState);
  server.on("/state", HTTP_POST, handlePostState);
  server.on("/brightness", HTTP_GET, handleGetBrightness);
  server.on("/brightness", HTTP_POST, handlePostBrightness);
  server.begin();
  Serial.println("HTTP server started");

  // 起動時はIPアドレス表示モード
  currentMode = MODE_SHOW_IP;
}

void handleSerialInput(Stream& stream) {
  if (stream.available()) {
    char c = stream.read();
    if (c == '1' || c == 'G' || c == 'g') {
      updateState(true);
    } else if (c == '0' || c == 'R' || c == 'r') {
      updateState(false);
    } else if (c == '?') {
      stream.println(green ? "State = 1 (Green)" : "State = 0 (Red)");
    } else if (c == '+') {
      stream.setTimeout(100);
      long val = stream.parseInt();
      if (val <= 0) val = 1;
      currentBrightness += val;
      if (currentBrightness > MAX_BRIGHTNESS) currentBrightness = MAX_BRIGHTNESS;
      FastLED.setBrightness(currentBrightness);
      FastLED.show();
      stream.println("Brightness = " + String(currentBrightness));
    } else if (c == '-') {
      stream.setTimeout(100);
      long val = stream.parseInt();
      if (val <= 0) val = 1;
      currentBrightness -= val;
      if (currentBrightness < MIN_BRIGHTNESS) currentBrightness = MIN_BRIGHTNESS;
      FastLED.setBrightness(currentBrightness);
      FastLED.show();
      stream.println("Brightness = " + String(currentBrightness));
    } else if (c == '*' || c == 'B' || c == 'b') {
      stream.setTimeout(100);
      long val = stream.parseInt();
      if (val > 0) {
        currentBrightness = val;
        if (currentBrightness > MAX_BRIGHTNESS) currentBrightness = MAX_BRIGHTNESS;
        if (currentBrightness < MIN_BRIGHTNESS) currentBrightness = MIN_BRIGHTNESS;
        FastLED.setBrightness(currentBrightness);
        FastLED.show();
      }
      stream.println("Brightness = " + String(currentBrightness));
    } else if (c == '@' || c == 'i' || c == 'I') {
      stream.println(WiFi.localIP().toString());
    }
  }
}

void loop() {
  M5.update();
  server.handleClient(); // WebServerのクライアント処理

  // 本体ボタン(BtnA)による操作
  if (M5.BtnA.wasPressed()) {
    if (currentMode == MODE_SHOW_IP) {
      // IP表示モードから通常モードへ遷移
      currentMode = MODE_NORMAL;
      updateState(green); // 現在の状態(赤/緑)を描画
    } else {
      // 通常モードでは状態をトグル
      updateState(!green);
    }
  }

  // IP表示モードのアニメーション処理 (青色と白色で交互に1文字ずつ表示)
  if (currentMode == MODE_SHOW_IP) {
    static unsigned long lastUpdate = 0;
    static int ipCharIdx = 0;
    
    // 500msごとに次の文字へ
    if (millis() - lastUpdate > 500) {
      lastUpdate = millis();
      String ipStr = WiFi.localIP().toString();
      
      if (ipCharIdx == 0) {
        // ループの先頭でシリアルモニタにもIPを出力（見逃し防止）
        Serial.print("Current IP address: ");
        Serial.println(ipStr);
      }

      if (ipCharIdx < ipStr.length()) {
        // 青と白を交互に切り替え
        CRGB color = (ipCharIdx % 2 == 0) ? CRGB::Blue : CRGB::White;
        drawChar(ipStr[ipCharIdx], color);
      } else {
        // 文字列の終わりで一瞬消灯する
        drawChar(' ', CRGB::Black); 
      }
      
      ipCharIdx++;
      // 文字列の長さ+1までいったら最初に戻る
      if (ipCharIdx > ipStr.length()) {
        ipCharIdx = 0;
      }
    }
  }

  // 有線Serial および BluetoothSerial 経由の受信処理
  handleSerialInput(Serial);
  handleSerialInput(SerialBT);
}
