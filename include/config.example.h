#pragma once

// =====================================================
// 配置文件模板
// 使用方法：
//   1. 复制本文件为 include/config.h
//   2. 填入你自己的 WiFi / MQTT 信息
//   （config.h 已被 .gitignore 忽略，不会上传到 GitHub）
// =====================================================

// ----- WiFi -----
const char* WIFI_SSID       = "your_wifi_ssid";
const char* WIFI_PASSWORD   = "your_wifi_password";

// ----- MQTT -----
const char* MQTT_BROKER     = "broker.emqx.io";   // 公共测试 Broker
const int   MQTT_PORT       = 1883;
const char* MQTT_CLIENT_ID  = "esp32c3-dht11";    // 客户端标识，改唯一
const char* MQTT_TOPIC      = "selffox/dht11";    // 发布主题

// ----- 采集间隔（ms）-----
// DHT11 采样间隔必须 >= 1000ms，否则会返回 nan
const unsigned long SENSOR_INTERVAL = 2000;
