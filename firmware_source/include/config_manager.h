#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct HudConfig {
    uint8_t  display_rotation = 1;          // Hướng quay màn hình
    bool     show_clock = true;             // Hiện đồng hồ khi rảnh
    uint8_t  sleep_screen_mode = 0;         // 0: Mochi Animation, 1: Digital Clock, 2: Off
    uint8_t  nav_source = 0;                // 0: Auto (Vietmap/Waze), 1: Vietmap, 2: Waze
    uint8_t  led_brightness = 80;           // Độ sáng màn hình ban ngày (10-100%)
    uint8_t  night_brightness = 25;         // Độ sáng ban đêm (10-100%)
    bool     auto_brightness = true;        // Tự động chỉnh độ sáng qua quang trở LDR
    int8_t   speed_limit_offset = 0;        // Bù trừ tốc độ (+/- km/h)
    bool     beep_limit_change = true;      // Kêu bíp khi thay đổi biển báo tốc độ
    bool     beep_overspeed = true;         // Kêu bíp khi chạy quá tốc độ
    bool     show_text_kmh = true;          // Hiện chữ km/h
    String   welcome_text = "VIETMAP HUD PRO";
    uint32_t boot_screen_duration = 2000;   // Thời gian khởi động (ms)
};

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    bool begin() {
        if (!LittleFS.begin(true)) {
            Serial.println("[FS] LittleFS Mount Failed");
            return false;
        }
        Serial.println("[FS] LittleFS Mounted Successfully");
        loadConfig();
        return true;
    }

    void loadConfig() {
        if (!LittleFS.exists("/config.txt")) {
            Serial.println("[Config] /config.txt not found, writing default");
            saveConfig();
            return;
        }

        File file = LittleFS.open("/config.txt", "r");
        if (!file) return;

        while (file.available()) {
            String line = file.readStringUntil('\n');
            line.trim();
            if (line.length() == 0 || line.startsWith("#")) continue;

            int sep = line.indexOf('=');
            if (sep == -1) sep = line.indexOf(':');
            if (sep == -1) continue;

            String key = line.substring(0, sep);
            String val = line.substring(sep + 1);
            key.trim();
            val.trim();

            if (key.equalsIgnoreCase("display_rotation")) config.display_rotation = val.toInt();
            else if (key.equalsIgnoreCase("show_clock")) config.show_clock = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("sleep_screen_mode")) config.sleep_screen_mode = val.toInt();
            else if (key.equalsIgnoreCase("nav_source")) config.nav_source = val.toInt();
            else if (key.equalsIgnoreCase("led_brightness")) config.led_brightness = val.toInt();
            else if (key.equalsIgnoreCase("night_brightness")) config.night_brightness = constrain(val.toInt(), 5, 100);
            else if (key.equalsIgnoreCase("auto_brightness")) config.auto_brightness = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("speed_limit_offset")) config.speed_limit_offset = val.toInt();
            else if (key.equalsIgnoreCase("beep_limit_change")) config.beep_limit_change = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("beep_overspeed")) config.beep_overspeed = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("show_text_kmh")) config.show_text_kmh = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("welcome_text")) config.welcome_text = val;
            else if (key.equalsIgnoreCase("boot_screen_duration")) config.boot_screen_duration = val.toInt();
        }
        file.close();
        Serial.println("[Config] Config loaded from /config.txt");
    }

    void saveConfig() {
        File file = LittleFS.open("/config.txt", "w");
        if (!file) return;

        file.printf("display_rotation=%u\n", config.display_rotation);
        file.printf("show_clock=%d\n", config.show_clock ? 1 : 0);
        file.printf("sleep_screen_mode=%u\n", config.sleep_screen_mode);
        file.printf("nav_source=%u\n", config.nav_source);
        file.printf("led_brightness=%u\n", config.led_brightness);
        file.printf("night_brightness=%u\n", config.night_brightness);
        file.printf("auto_brightness=%d\n", config.auto_brightness ? 1 : 0);
        file.printf("speed_limit_offset=%d\n", config.speed_limit_offset);
        file.printf("beep_limit_change=%d\n", config.beep_limit_change ? 1 : 0);
        file.printf("beep_overspeed=%d\n", config.beep_overspeed ? 1 : 0);
        file.printf("show_text_kmh=%d\n", config.show_text_kmh ? 1 : 0);
        file.printf("welcome_text=%s\n", config.welcome_text.c_str());
        file.printf("boot_screen_duration=%u\n", config.boot_screen_duration);

        file.close();
        Serial.println("[Config] Config saved to /config.txt");
    }

    String toJsonString() {
        StaticJsonDocument<512> doc;
        doc["rotation"] = config.display_rotation;
        doc["clock"] = config.show_clock;
        doc["sleep_mode"] = config.sleep_screen_mode;
        doc["nav_source"] = config.nav_source;
        doc["bright_day"] = config.led_brightness;
        doc["bright_night"] = config.night_brightness;
        doc["auto_bright"] = config.auto_brightness;
        doc["offset"] = config.speed_limit_offset;
        doc["beep_limit"] = config.beep_limit_change;
        doc["beep_speed"] = config.beep_overspeed;
        doc["text_kmh"] = config.show_text_kmh;
        doc["welcome"] = config.welcome_text;
        String out;
        serializeJson(doc, out);
        return out;
    }

    bool applyJson(const String& jsonStr) {
        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, jsonStr);
        if (err) return false;

        if (doc.containsKey("rotation")) config.display_rotation = doc["rotation"];
        if (doc.containsKey("clock")) config.show_clock = doc["clock"];
        if (doc.containsKey("sleep_mode")) config.sleep_screen_mode = doc["sleep_mode"];
        if (doc.containsKey("nav_source")) config.nav_source = doc["nav_source"];
        if (doc.containsKey("bright_day")) config.led_brightness = doc["bright_day"];
        if (doc.containsKey("bright_night")) config.night_brightness = doc["bright_night"];
        if (doc.containsKey("auto_bright")) config.auto_brightness = doc["auto_bright"];
        if (doc.containsKey("offset")) config.speed_limit_offset = doc["offset"];
        if (doc.containsKey("beep_limit")) config.beep_limit_change = doc["beep_limit"];
        if (doc.containsKey("beep_speed")) config.beep_overspeed = doc["beep_speed"];
        if (doc.containsKey("welcome")) config.welcome_text = doc["welcome"].as<String>();

        saveConfig();
        return true;
    }

    HudConfig& get() { return config; }

private:
    HudConfig config;
    ConfigManager() {}
};
