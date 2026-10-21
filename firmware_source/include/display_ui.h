#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "config_pins.h"
#include "config_manager.h"
#include "hud_data.h"
#include "assets_icons.h"

static TFT_eSPI tft = TFT_eSPI();

// Callback cho TJpg_Decoder để đẩy pixel RGB565 ra TFT_eSPI
static bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (y >= tft.height()) return 0;
    tft.pushImage(x, y, w, h, bitmap);
    return 1;
}

class DisplayUI {
public:
    static DisplayUI& getInstance() {
        static DisplayUI instance;
        return instance;
    }

    void begin() {
        Serial.printf("[UI] Initializing Display with TJPGD Assets for %s...\n", BOARD_NAME);

        #if TFT_BL >= 0
        pinMode(TFT_BL, OUTPUT);
        analogWrite(TFT_BL, 200); // 80% độ sáng ban đầu
        #endif

        tft.init();
        #if SCREEN_IS_ROUND
        tft.setRotation(0);
        #else
        tft.setRotation(ConfigManager::getInstance().get().display_rotation);
        #endif
        tft.fillScreen(TFT_BLACK);

        // Khởi tạo bộ giải nén ảnh JPEG TJpgDec
        TJpgDec.setJpgScale(1);
        TJpgDec.setSwapBytes(true); // Đảo byte màu cho chuẩn TFT_eSPI
        TJpgDec.setCallback(tft_output);

        drawBootScreen();
    }

    void drawBootScreen() {
        tft.fillScreen(TFT_BLACK);
        
        #if SCREEN_IS_ROUND
        // Vẽ vòng tròn trang trí cho màn hình tròn
        tft.drawCircle(120, 120, 118, TFT_CYAN);
        tft.drawCircle(120, 120, 115, TFT_NAVY);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextSize(2);
        tft.drawCentreString("VIETMAP HUD", 120, 70, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextSize(1);
        tft.drawCentreString("Round Edition", 120, 115, 2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("BLE: Ready", 120, 145, 2);
        #else
        // Màn hình ngang 320x240
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextSize(3);
        tft.drawCentreString("VIETMAP HUD PRO", 160, 60, 2);
        tft.setTextSize(2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString("CYD 2.4 Edition", 160, 120, 2);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("BLE Name: VIETMAP_HUD", 160, 160, 2);
        #endif

        delay(ConfigManager::getInstance().get().boot_screen_duration);
        tft.fillScreen(TFT_BLACK);
    }

    void update(SystemState& state) {
        // Tự động điều chỉnh độ sáng nếu có quang trở LDR
        #if LDR_SENSOR_PIN >= 0
        if (ConfigManager::getInstance().get().auto_brightness) {
            int ldrVal = analogRead(LDR_SENSOR_PIN);
            state.ambient_light = ldrVal;
            // Ánh sáng mạnh (ban ngày) -> ldrVal thấp hoặc cao tùy mạch, map sang 10..100%
            int duty = map(ldrVal, 200, 3500, ConfigManager::getInstance().get().night_brightness, ConfigManager::getInstance().get().led_brightness);
            duty = constrain(duty, 15, 100);
            #if TFT_BL >= 0
            analogWrite(TFT_BL, (duty * 255) / 100);
            #endif
        }
        #endif

        if (state.ble_phone_connected && state.nav.nav_active) {
            #if SCREEN_IS_ROUND
            renderRoundNavScreen(state);
            #else
            renderLandscapeNavScreen(state);
            #endif
        } else {
            renderStandbyScreen(state);
        }
    }

    void beep(uint16_t freq, uint16_t duration_ms) {
        #if BUZZER_PIN >= 0
        tone(BUZZER_PIN, freq, duration_ms);
        #endif
    }

private:
    int16_t last_speed = -99;
    int16_t last_limit = -99;
    uint8_t last_turn = 255;
    uint32_t last_dist = 999999;
    uint8_t last_mode = 255;

    DisplayUI() {}

    // =========================================================================
    // 1. GIAO DIỆN MÀN HÌNH CHỮ NHẬT 320x240 (ESP32-2432S024)
    // =========================================================================
    void renderLandscapeNavScreen(const SystemState& state) {
        if (last_mode != 1) {
            tft.fillScreen(TFT_BLACK);
            last_mode = 1;
            last_speed = -99;
            last_limit = -99;
            last_turn = 255;
            last_dist = 999999;
        }

        int16_t spd = state.nav.speed;
        int16_t lim = state.nav.speed_limit;
        bool overspeed = (lim > 0 && spd > (lim + ConfigManager::getInstance().get().speed_limit_offset));

        // 1. Biển báo giới hạn tốc độ (Bên trái: x=10..75, y=30)
        if (lim != last_limit) {
            last_limit = lim;
            drawSpeedLimitSign(15, 35, 38, lim);
            if (ConfigManager::getInstance().get().beep_limit_change) {
                beep(2000, 100);
            }
        }

        // 2. Tốc độ hiện tại ở giữa (x=105..220)
        if (spd != last_speed) {
            last_speed = spd;
            tft.fillRect(105, 30, 115, 95, TFT_BLACK);
            uint16_t col = overspeed ? TFT_RED : TFT_GREEN;
            tft.setTextColor(col, TFT_BLACK);
            
            // Vẽ số tốc độ lớn
            tft.setTextSize(1);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", spd);
            tft.drawCentreString(buf, 160, 30, 8); // Font 8 (75px)

            // Đơn vị km/h
            if (ConfigManager::getInstance().get().show_text_kmh) {
                tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
                tft.drawCentreString("km/h", 160, 115, 2);
            }

            if (overspeed && ConfigManager::getInstance().get().beep_overspeed) {
                beep(2500, 80);
            }
        }

        // 3. Mũi tên chỉ đường và khoảng cách (Bên phải: x=235..315)
        if (state.nav.turn_icon != last_turn || state.nav.distance_to_turn != last_dist) {
            last_turn = state.nav.turn_icon;
            last_dist = state.nav.distance_to_turn;
            drawTurnIconAndDist(240, 35, state.nav.turn_icon, state.nav.distance_to_turn);
        }

        // 4. Thanh cảnh báo hoặc tên đường (Dưới đáy: y=190..240)
        if (state.nav.alerts.length() > 0) {
            tft.fillRect(0, 195, 320, 45, TFT_RED);
            tft.setTextColor(TFT_WHITE, TFT_RED);
            tft.drawCentreString(state.nav.alerts, 160, 205, 4);
        } else if (state.nav.street_name.length() > 0) {
            tft.fillRect(0, 195, 320, 45, 0x0011); // Navy blue
            tft.setTextColor(TFT_YELLOW, 0x0011);
            tft.drawCentreString(state.nav.street_name, 160, 205, 4);
        } else {
            tft.fillRect(0, 195, 320, 45, TFT_BLACK);
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            tft.drawCentreString(ConfigManager::getInstance().get().welcome_text, 160, 205, 2);
        }

        // Thanh trạng thái trên đỉnh (y=5..20): Giờ & ETA
        tft.fillRect(0, 0, 320, 22, 0x10A2);
        tft.setTextColor(TFT_WHITE, 0x10A2);
        tft.drawString("BLE CONNECTED", 10, 4, 2);
        if (state.nav.eta.length() > 0) {
            String etaStr = "ETA: " + state.nav.eta;
            tft.drawRightString(etaStr, 310, 4, 2);
        } else {
            tft.drawRightString(state.current_time_str, 310, 4, 2);
        }
    }

    // =========================================================================
    // 2. GIAO DIỆN MÀN HÌNH TRÒN 240x240 (ESP32-2424S012)
    // =========================================================================
    void renderRoundNavScreen(const SystemState& state) {
        if (last_mode != 1) {
            tft.fillScreen(TFT_BLACK);
            last_mode = 1;
            last_speed = -99;
            last_limit = -99;
            last_turn = 255;
            // Vẽ viền tròn thể thao
            tft.drawCircle(120, 120, 119, 0x3186);
            tft.drawCircle(120, 120, 118, 0x2104);
        }

        int16_t spd = state.nav.speed;
        int16_t lim = state.nav.speed_limit;
        bool overspeed = (lim > 0 && spd > (lim + ConfigManager::getInstance().get().speed_limit_offset));

        // 1. Biển báo tốc độ giới hạn (Ở đỉnh vòng tròn: x=120, y=30)
        if (lim != last_limit) {
            last_limit = lim;
            drawSpeedLimitSign(95, 12, 25, lim);
            if (ConfigManager::getInstance().get().beep_limit_change) {
                beep(2000, 100);
            }
        }

        // 2. Tốc độ ở tâm vòng tròn (x=120, y=75..145)
        if (spd != last_speed) {
            last_speed = spd;
            tft.fillRect(45, 75, 150, 75, TFT_BLACK);
            uint16_t col = overspeed ? TFT_RED : TFT_GREEN;
            tft.setTextColor(col, TFT_BLACK);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", spd);
            tft.drawCentreString(buf, 120, 75, 7); // Font 7 7-segment 48px

            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawCentreString("km/h", 120, 128, 2);

            if (overspeed && ConfigManager::getInstance().get().beep_overspeed) {
                beep(2500, 80);
            }
        }

        // 3. Mũi tên chỉ đường và khoảng cách (Ở đáy vòng tròn: y=160..210)
        if (state.nav.turn_icon != last_turn || state.nav.distance_to_turn != last_dist) {
            last_turn = state.nav.turn_icon;
            last_dist = state.nav.distance_to_turn;
            tft.fillRect(50, 155, 140, 55, TFT_BLACK);
            
            // Vẽ biểu tượng rẽ đơn giản
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            String arrow = "";
            if (state.nav.turn_icon == TURN_LEFT || state.nav.turn_icon == TURN_SLIGHT_LEFT) arrow = "<--";
            else if (state.nav.turn_icon == TURN_RIGHT || state.nav.turn_icon == TURN_SLIGHT_RIGHT) arrow = "-->";
            else if (state.nav.turn_icon == TURN_STRAIGHT) arrow = "^^";
            else if (state.nav.turn_icon == TURN_ROUNDABOUT) arrow = "(O)";
            
            if (arrow.length() > 0) {
                tft.drawCentreString(arrow, 120, 155, 4);
            }

            // Khoảng cách
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            if (state.nav.distance_to_turn >= 1000) {
                char dBuf[16];
                snprintf(dBuf, sizeof(dBuf), "%.1f km", state.nav.distance_to_turn / 1000.0f);
                tft.drawCentreString(dBuf, 120, 185, 2);
            } else {
                char dBuf[16];
                snprintf(dBuf, sizeof(dBuf), "%u m", state.nav.distance_to_turn);
                tft.drawCentreString(dBuf, 120, 185, 2);
            }
        }
    }

    // =========================================================================
    // 3. MÀN HÌNH CHỜ STANDBY (ĐỒNG HỒ & THỜI TIẾT KHI XE CHƯA KẾT NỐI APP)
    // =========================================================================
    void renderStandbyScreen(const SystemState& state) {
        if (last_mode != 0) {
            tft.fillScreen(TFT_BLACK);
            last_mode = 0;
        }

        #if SCREEN_IS_ROUND
        tft.drawCircle(120, 120, 118, TFT_CYAN);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString(state.current_time_str, 120, 85, 7);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawCentreString("VIETMAP HUD", 120, 145, 2);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString("Waiting Phone...", 120, 175, 2);
        #else
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString(state.current_time_str, 160, 65, 8);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawCentreString("VIETMAP HUD PRO", 160, 145, 4);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("Connect Vietmap App via BLE", 160, 185, 2);
        #endif
    }

    void drawSpeedLimitSign(int x, int y, int r, int limit) {
        // Ánh xạ giới hạn tốc độ sang icon JPEG chuẩn của Vietmap
        int iconIdx = -1;
        if (limit == 20) iconIdx = 29;
        else if (limit == 30) iconIdx = 30;
        else if (limit == 40) iconIdx = 31;
        else if (limit == 50) iconIdx = 32;
        else if (limit == 60) iconIdx = 33;
        else if (limit == 70) iconIdx = 34;
        else if (limit == 80) iconIdx = 35;
        else if (limit == 90) iconIdx = 36;
        else if (limit == 100) iconIdx = 37;
        else if (limit == 120) iconIdx = 38;

        if (iconIdx >= 0 && iconIdx < HUD_ICON_COUNT) {
            // Vẽ trực tiếp bằng file ảnh JPEG gốc 60x60
            TJpgDec.drawJpg(x, y, HUD_ICONS[iconIdx].data, HUD_ICONS[iconIdx].size);
            return;
        }

        // Fallback: Vẽ vector tròn nếu là giá trị tốc độ lạ
        tft.fillCircle(x + r, y + r, r, TFT_RED);
        tft.fillCircle(x + r, y + r, r - 5, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        if (limit > 0 && limit < 200) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", limit);
            tft.drawCentreString(buf, x + r, y + (r/3), 4);
        } else {
            tft.drawCentreString("--", x + r, y + (r/3), 4);
        }
    }

    void drawTurnIconAndDist(int x, int y, uint8_t turn, uint32_t dist) {
        tft.fillRect(x, y, 75, 90, TFT_BLACK);

        // Ánh xạ mã rẽ sang icon JPEG gốc của Vietmap (icon 0..15)
        int iconIdx = -1;
        if (turn >= 1 && turn <= 12) {
            iconIdx = turn - 1; // Map vào bộ icon JPEG
        }

        if (iconIdx >= 0 && iconIdx < HUD_ICON_COUNT) {
            TJpgDec.drawJpg(x + 5, y, HUD_ICONS[iconIdx].data, HUD_ICONS[iconIdx].size);
        } else {
            tft.setTextColor(TFT_CYAN, TFT_BLACK);
            String arrow = "";
            if (turn == TURN_LEFT || turn == TURN_SLIGHT_LEFT || turn == TURN_SHARP_LEFT) arrow = "<-";
            else if (turn == TURN_RIGHT || turn == TURN_SLIGHT_RIGHT || turn == TURN_SHARP_RIGHT) arrow = "->";
            else if (turn == TURN_STRAIGHT) arrow = "^^";
            else if (turn == TURN_ROUNDABOUT) arrow = "O";
            tft.drawCentreString(arrow, x + 35, y + 5, 4);
        }

        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (dist >= 1000) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f km", dist / 1000.0f);
            tft.drawCentreString(buf, x + 35, y + 65, 2);
        } else {
            char buf[16];
            snprintf(buf, sizeof(buf), "%u m", dist);
            tft.drawCentreString(buf, x + 35, y + 65, 2);
        }
    }
};
