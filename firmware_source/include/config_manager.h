#pragma once
#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

struct HudConfig {
    uint8_t  display_rotation = 1;          // Hướng quay màn hình: 0, 1, 2, 3
    bool     hud_mirror = false;            // Chế độ lật gương chiếu kính lái (HUD Mirror Mode)
    uint8_t  usb_side = 0;                  // 0: Cổng sạc bên Trái (Left), 1: Cổng sạc bên Phải (Right)
    bool     show_clock = true;             // Hiện đồng hồ khi rảnh
    uint8_t  sleep_screen_mode = 0;         // 0: Mochi Animation, 1: Digital Clock, 2: Off
    uint8_t  nav_source = 0;                // 0: Auto (Vietmap/Waze), 1: Vietmap, 2: Waze
    int8_t   speed_limit_offset = 0;        // Km chênh lệch cho phép (+/- km/h)
    uint8_t  alert_duration = 5;            // Thời gian duy trì cảnh báo quá tốc độ (giây)
    bool     show_text_kmh = true;          // Hiện chữ km/h
    bool     enable_led_alert = true;       // Bật đèn LED cảnh báo RGB phần cứng
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
            key.trim(); val.trim();

            if (key.equalsIgnoreCase("display_rotation")) config.display_rotation = val.toInt();
            else if (key.equalsIgnoreCase("hud_mirror")) config.hud_mirror = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("usb_side")) config.usb_side = val.toInt();
            else if (key.equalsIgnoreCase("show_clock")) config.show_clock = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("sleep_screen_mode")) config.sleep_screen_mode = val.toInt();
            else if (key.equalsIgnoreCase("nav_source")) config.nav_source = val.toInt();
            else if (key.equalsIgnoreCase("speed_limit_offset")) config.speed_limit_offset = val.toInt();
            else if (key.equalsIgnoreCase("alert_duration")) config.alert_duration = val.toInt();
            else if (key.equalsIgnoreCase("show_text_kmh")) config.show_text_kmh = (val.toInt() != 0);
            else if (key.equalsIgnoreCase("enable_led_alert")) config.enable_led_alert = (val.toInt() != 0);
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
        file.printf("hud_mirror=%d\n", config.hud_mirror ? 1 : 0);
        file.printf("usb_side=%u\n", config.usb_side);
        file.printf("show_clock=%d\n", config.show_clock ? 1 : 0);
        file.printf("sleep_screen_mode=%u\n", config.sleep_screen_mode);
        file.printf("nav_source=%u\n", config.nav_source);
        file.printf("speed_limit_offset=%d\n", config.speed_limit_offset);
        file.printf("alert_duration=%u\n", config.alert_duration);
        file.printf("show_text_kmh=%d\n", config.show_text_kmh ? 1 : 0);
        file.printf("enable_led_alert=%d\n", config.enable_led_alert ? 1 : 0);
        file.printf("welcome_text=%s\n", config.welcome_text.c_str());
        file.printf("boot_screen_duration=%u\n", config.boot_screen_duration);

        file.close();
        Serial.println("[Config] Config saved to /config.txt");
    }

    String toJsonString() {
        StaticJsonDocument<512> doc;
        doc["rotation"] = config.display_rotation;
        doc["mirror"] = config.hud_mirror;
        doc["usb_side"] = config.usb_side;
        doc["clock"] = config.show_clock;
        doc["sleep_mode"] = config.sleep_screen_mode;
        doc["nav_source"] = config.nav_source;
        doc["offset"] = config.speed_limit_offset;
        doc["alert_dur"] = config.alert_duration;
        doc["text_kmh"] = config.show_text_kmh;
        doc["led_alert"] = config.enable_led_alert;
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
        if (doc.containsKey("mirror")) config.hud_mirror = doc["mirror"];
        if (doc.containsKey("usb_side")) config.usb_side = doc["usb_side"];
        if (doc.containsKey("clock")) config.show_clock = doc["clock"];
        if (doc.containsKey("sleep_mode")) config.sleep_screen_mode = doc["sleep_mode"];
        if (doc.containsKey("nav_source")) config.nav_source = doc["nav_source"];
        if (doc.containsKey("offset")) config.speed_limit_offset = doc["offset"];
        if (doc.containsKey("alert_dur")) config.alert_duration = doc["alert_dur"];
        if (doc.containsKey("text_kmh")) config.show_text_kmh = doc["text_kmh"];
        if (doc.containsKey("led_alert")) config.enable_led_alert = doc["led_alert"];
        if (doc.containsKey("welcome")) config.welcome_text = doc["welcome"].as<String>();

        saveConfig();
        return true;
    }

    HudConfig& get() { return config; }

private:
    HudConfig config;
    ConfigManager() {}
};
