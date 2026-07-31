#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "pins.h"
#include "config.h"

// ===== 全局对象 =====
MD_Parola ledMatrix(MD_MAX72XX::PAROLA_HW, PIN_LED_DIN, PIN_LED_CLK, PIN_LED_CS, LED_MAX_DEVICES);
DHT dht(PIN_DHT, DHT_TYPE);
Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
WiFiClient espClient;
PubSubClient mqtt(espClient);

// ===== 爱心动画 =====
// 双帧尺寸缩放模拟心跳节拍：lub-dub ... 停顿 ... lub-dub ...
// 亮度恒定 10，不闪烁
const uint8_t heartBig[8] = {
    0b00000000,
    0b01100110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000,
    0b00000000,
};

const uint8_t heartSmall[8] = {
    0b00000000,
    0b00000000,
    0b01100110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00000000,
    0b00000000,
};

// 心跳节奏状态机（ms）
// 小心 → 大心 → 小心 → 大心 → 小心(长停顿) → 循环
//  =beat=      =beat=
const unsigned long heartbeatDurations[] = { 300, 300, 300, 300, 800 };
const uint8_t heartbeatStateCount = 5;

// ===== LED 矩阵：心跳动画（非阻塞） =====
void updateHeart() {
    static unsigned long lastBeat = 0;
    static uint8_t state = 0;     // 0/1/2/3/4

    unsigned long now = millis();
    if (now - lastBeat < heartbeatDurations[state]) return;
    lastBeat = now;
    state = (state + 1) % heartbeatStateCount;

    // state 1、3 时显示大心，其余显示小心
    const uint8_t* frame = (state == 1 || state == 3) ? heartBig : heartSmall;
    MD_MAX72XX* mx = ledMatrix.getGraphicObject();
    ledMatrix.displayClear();
    for (int row = 0; row < 8; row++) {
        mx->setRow(row, frame[row]);
    }
    mx->update();
}

// ===== OLED 绘图 =====
void drawScreen(float t, float h) {
    oled.clearDisplay();

    // 顶部标题栏
    oled.fillRect(0, 0, 128, 16, SSD1306_WHITE);
    oled.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    oled.setTextSize(1);
    oled.setCursor(28, 4);
    oled.print("selffox.xyz");

    // 状态亮点：2 个圆点，实心=已连接，空心=未连接
    // 左侧 = WiFi，右侧 = MQTT
    if (WiFi.status() == WL_CONNECTED) {
        oled.fillCircle(114, 6, 2, SSD1306_BLACK);
    } else {
        oled.drawCircle(114, 6, 2, SSD1306_BLACK);
    }
    if (mqtt.connected()) {
        oled.fillCircle(124, 6, 2, SSD1306_BLACK);
    } else {
        oled.drawCircle(124, 6, 2, SSD1306_BLACK);
    }

    oled.setTextColor(SSD1306_WHITE);

    // 温度
    oled.setTextSize(3);
    oled.setCursor(4, 30);
    oled.print((int)t);

    oled.setTextSize(2);
    oled.setCursor(42, 36);
    oled.write(247);           // ° 符号
    oled.print(F("C"));

    oled.drawFastVLine(72, 18, 44, SSD1306_WHITE);

    // 湿度
    oled.setTextSize(1);
    oled.setCursor(82, 27);
    oled.print(F("HUMID"));

    oled.setTextSize(2);
    oled.setCursor(82, 37);
    oled.print((int)h);
    oled.print(F("%"));

    oled.display();
}

// ===== WiFi：断线自动重连（每 5s 尝试一次） =====
void ensureWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt < 5000) return;
    lastAttempt = millis();

    Serial.print("WiFi reconnecting... ");
    WiFi.disconnect();
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.println(WIFI_SSID);
}

// ===== MQTT：断线自动重连（每 5s 尝试一次） =====
void ensureMQTT() {
    if (WiFi.status() != WL_CONNECTED) return;
    if (mqtt.connected()) return;

    static unsigned long lastTry = 0;
    if (millis() - lastTry < 5000) return;
    lastTry = millis();

    Serial.print("Connecting MQTT... ");
    if (mqtt.connect(MQTT_CLIENT_ID)) {
        Serial.println("OK");
    } else {
        Serial.printf("failed, rc=%d\n", mqtt.state());
    }
}

// ===== 读取传感器并更新 OLED + 发布 MQTT =====
void readAndDisplay() {
    float temp = dht.readTemperature();
    float hum  = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
        Serial.println("DHT read fail");
        return;
    }

    Serial.printf("Temp: %.1f C  Humi: %.1f %%\n", temp, hum);
    drawScreen(temp, hum);

    if (mqtt.connected()) {
        char payload[64];
        snprintf(payload, sizeof(payload),
                 "{\"temperature\":%.1f,\"humidity\":%.1f}", temp, hum);
        mqtt.publish(MQTT_TOPIC, payload);
    }
}

// ===== setup =====
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n===== ESP32-C3 Weather Station =====");

    // LED 矩阵
    ledMatrix.begin();
    ledMatrix.setIntensity(10);         // 亮度恒定 0~15（不随动画变化）

    // 开机立刻显示大心
    MD_MAX72XX* mx = ledMatrix.getGraphicObject();
    for (int row = 0; row < 8; row++) {
        mx->setRow(row, heartBig[row]);
    }
    mx->update();

    // DHT11
    dht.begin();

    // OLED
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("OLED init failed!");
        // 继续运行，LED 矩阵与 MQTT 不受影响
    } else {
        drawScreen(0, 0);
    }

    // WiFi（后台连接，不阻塞）
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("WiFi connecting... ");

    // MQTT
    mqtt.setServer(MQTT_BROKER, MQTT_PORT);
}

// ===== loop =====
void loop() {
    mqtt.loop();          // 维持 MQTT 心跳，必须每轮调用

    ensureWiFi();
    ensureMQTT();
    updateHeart();

    // 非阻塞定时采集（默认 2s，DHT11 要求 >= 1s）
    static unsigned long lastRead = 0;
    unsigned long now = millis();
    if (now - lastRead >= SENSOR_INTERVAL) {
        lastRead = now;
        readAndDisplay();
    }
}
