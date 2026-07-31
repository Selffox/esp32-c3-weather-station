#pragma once

// =====================================================
// ESP32-C3 Super Mini 引脚定义
// 请根据你的实际接线修改
// =====================================================

// ----- MAX7219 LED 点阵（SPI）-----
#define PIN_LED_DIN        3   // 数据线
#define PIN_LED_CLK        0   // 时钟线
#define PIN_LED_CS         10  // 片选
#define LED_MAX_DEVICES    1   // 级联数量（1 个模块）

// ----- DHT11 -----
#define PIN_DHT            1   // 数据线
#define DHT_TYPE           DHT11

// ----- SSD1306 OLED（I2C）-----
#define PIN_OLED_SDA       4
#define PIN_OLED_SCL       6
#define OLED_ADDR          0x3C
#define OLED_WIDTH         128
#define OLED_HEIGHT        64
#define OLED_RESET         -1   // 不使用复位引脚
