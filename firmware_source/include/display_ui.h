#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "config_pins.h"
#include "config_manager.h"
#include "hud_data.h"
#include "assets_icons.h"
#include "mochi_anim.h"

// Bảng tra cứu 82 icon gốc trích xuất trực tiếp từ IROM offset 0x2606b0 của Firmware Vietmap HUD
static const uint8_t ORIGINAL_ICON_LOOKUP[82] PROGMEM = {
   0,  1,  4,  5,  2,  3,  3,  4,  5,  6,
   6,  7,  1,  2,  7,  8, 22, 23,  8,  9,
   9, 10, 10, 11, 11, 12, 12, 13, 13, 14,
  14, 15, 15, 16, 16, 17, 17, 18, 18, 19,
  19, 20, 20, 21, 21, 22, 23, 24, 24, 25,
  25, 26, 26, 27, 27, 28, 28, 29, 29, 30,
  30, 31, 31, 32, 32, 33, 33, 34, 34, 35,
  35, 36, 36, 37, 37, 38, 38, 39, 39, 40,
  40,  0,
};

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
        Serial.printf("[UI] Initializing Display (100%% Max Brightness, No Buzzer/LDR) for %s...\n", BOARD_NAME);

        // Khóa độ sáng màn hình ở mức CỰC ĐẠI 100% (255) cho HUD phản xạ kính lái
        #if TFT_BL >= 0
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, HIGH); // Luôn luôn 100% sáng tối đa, chống chói ngoài trời nắng
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
        TJpgDec.setSwapBytes(true);
        TJpgDec.setCallback(tft_output);

        drawBootScreen();
    }

    void drawBootScreen() {
        tft.fillScreen(TFT_BLACK);

        #if SCREEN_IS_ROUND
        tft.drawCircle(120, 120, 118, TFT_CYAN);
        tft.drawCircle(120, 120, 115, TFT_NAVY);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawCentreString(ConfigManager::getInstance().get().welcome_text, 120, 75, 4);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("BLE: Ready", 120, 125, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString("VIETMAP / WAZE", 120, 155, 2);
        #else
        tft.drawRoundRect(10, 10, 300, 220, 10, TFT_BLUE);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawCentreString(ConfigManager::getInstance().get().welcome_text, 160, 45, 4);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("Bluetooth: VIETMAP_HUD (Ready)", 160, 100, 2);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString("Do sang: 100% MAX HUD", 160, 140, 2);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString("Ket noi Vietmap Live hoac Waze Hub", 160, 180, 2);
        #endif
    }

    void update(const SystemState& state) {
        bool has_nav = state.ble_phone_connected && (state.nav.nav_active || state.nav.speed_limit > 0 || state.nav.speed > 0);

        if (has_nav) {
            #if SCREEN_IS_ROUND
            renderRoundNavScreen(state);
            #else
            renderLandscapeNavScreen(state);
            #endif
        } else {
            renderStandbyScreen(state);
        }
    }

    void toggleScreenMode() {
        screen_mode = (screen_mode + 1) % 3;
        tft.fillScreen(TFT_BLACK);
    }

    void rotateScreen() {
        #if !SCREEN_IS_ROUND
        uint8_t rot = (tft.getRotation() + 1) % 4;
        tft.setRotation(rot);
        ConfigManager::getInstance().get().display_rotation = rot;
        ConfigManager::getInstance().saveConfig();
        tft.fillScreen(TFT_BLACK);
        #endif
    }

private:
    int16_t  last_speed = -99;
    int16_t  last_limit = -99;
    uint8_t  last_turn = 255;
    uint32_t last_dist = 999999;
    uint8_t  last_mode = 255;
    uint8_t  screen_mode = 0;
    uint8_t  mochi_cur_frame = 0;
    uint32_t last_anim_frame_time = 0;

    DisplayUI() {}

    // =========================================================================
    // 1. GIAO DIỆN MÀN HÌNH CHỮ NHẬT 320x240 (ESP32-2432S024 CYD)
    // =========================================================================
    void renderLandscapeNavScreen(const SystemState& state) {
        if (last_mode != 1) {
            tft.fillScreen(TFT_BLACK);
            last_mode = 1;
            last_speed = -99;
            last_limit = -99;
            last_turn = 255;
            last_dist = 999999;
            tft.drawFastHLine(0, 205, 320, TFT_DARKGREY);
        }

        int16_t spd = state.nav.speed;
        int16_t lim = state.nav.speed_limit;
        bool overspeed = (lim > 0 && spd > (lim + ConfigManager::getInstance().get().speed_limit_offset));

        // 1. Biển báo giới hạn tốc độ gốc (JPEG 60x60)
        if (lim != last_limit) {
            last_limit = lim;
            drawSpeedLimitSign(15, 25, lim);
        }

        // 2. Số tốc độ hiện tại cực lớn ở giữa
        if (spd != last_speed) {
            last_speed = spd;
            tft.fillRect(95, 15, 130, 95, TFT_BLACK);
            uint16_t col = overspeed ? TFT_RED : TFT_GREEN;
            tft.setTextColor(col, TFT_BLACK);

            char buf[8];
            snprintf(buf, sizeof(buf), "%d", spd);
            tft.drawCentreString(buf, 160, 20, 8); // Font 8 (75px)

            if (ConfigManager::getInstance().get().show_text_kmh) {
                tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
                tft.drawCentreString("km/h", 160, 98, 2);
            }
        }

        // 3. Mũi tên rẽ và khoảng cách gốc (JPEG 60x60)
        if (state.nav.turn_icon != last_turn || state.nav.distance_to_turn != last_dist) {
            last_turn = state.nav.turn_icon;
            last_dist = state.nav.distance_to_turn;
            drawTurnIconAndDist(240, 20, state.nav.turn_icon, state.nav.distance_to_turn, state.nav.exit_num);
        }

        // 4. Dòng cảnh báo Camera hoặc Tên đường
        tft.fillRect(10, 135, 300, 60, TFT_BLACK);
        if (state.nav.alerts.length() > 0) {
            tft.setTextColor(TFT_YELLOW, TFT_BLACK);
            tft.drawCentreString(state.nav.alerts, 160, 138, 4);
            drawAlertCameraIcon(15, 135);
        } else if (state.nav.street_name.length() > 0) {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.drawCentreString(state.nav.street_name, 160, 142, 2);
        }

        // 5. Thanh trạng thái dưới cùng (Footer)
        tft.fillRect(5, 210, 310, 25, TFT_BLACK);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawString(state.current_time_str, 10, 212, 2);

        if (state.nav.remain_km > 0.0f || state.nav.remain_min > 0) {
            char trip_buf[32];
            snprintf(trip_buf, sizeof(trip_buf), "Con: %.1f km (%d p)", state.nav.remain_km, state.nav.remain_min);
            tft.drawRightString(trip_buf, 310, 212, 2);
        } else {
            tft.drawRightString("HUD CONNECTED", 310, 212, 2);
        }
    }

    // =========================================================================
    // 2. GIAO DIỆN MÀN HÌNH TRÒN 240x240 (ESP32-2424S012 GC9A01)
    // =========================================================================
    void renderRoundNavScreen(const SystemState& state) {
        if (last_mode != 2) {
            tft.fillScreen(TFT_BLACK);
            last_mode = 2;
            last_speed = -99;
            last_limit = -99;
            last_turn = 255;
            last_dist = 999999;
        }

        int16_t spd = state.nav.speed;
        int16_t lim = state.nav.speed_limit;

        // Vòng cung tốc độ quanh viền tròn
        uint16_t arc_color = (lim > 0 && spd > lim) ? TFT_RED : TFT_CYAN;
        tft.drawCircle(120, 120, 118, TFT_NAVY);
        tft.drawCircle(120, 120, 117, arc_color);

        // Biển báo tốc độ ở đỉnh trên
        if (lim != last_limit) {
            last_limit = lim;
            drawSpeedLimitSign(90, 15, lim);
        }

        // Số tốc độ ở giữa tâm
        if (spd != last_speed) {
            last_speed = spd;
            tft.fillRect(40, 85, 160, 75, TFT_BLACK);

            bool overspeed = (lim > 0 && spd > (lim + ConfigManager::getInstance().get().speed_limit_offset));
            tft.setTextColor(overspeed ? TFT_RED : TFT_WHITE, TFT_BLACK);

            char buf[8];
            snprintf(buf, sizeof(buf), "%d", spd);
            tft.drawCentreString(buf, 120, 85, 7);

            tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
            tft.drawCentreString("km/h", 120, 145, 2);
        }

        // Mũi tên rẽ và khoảng cách ở nửa dưới
        if (state.nav.turn_icon != last_turn || state.nav.distance_to_turn != last_dist) {
            last_turn = state.nav.turn_icon;
            last_dist = state.nav.distance_to_turn;
            drawTurnIconAndDist(90, 165, state.nav.turn_icon, state.nav.distance_to_turn, state.nav.exit_num);
        }
    }

    // =========================================================================
    // 3. MÀN HÌNH CHỜ STANDBY (MOCHI ANIMATION / ĐỒNG HỒ / TẮT MÀN HÌNH)
    // =========================================================================
    void renderStandbyScreen(const SystemState& state) {
        uint8_t mode = ConfigManager::getInstance().get().sleep_screen_mode;

        // Chế độ 2: Tắt màn hình / Màn hình đen
        if (mode == 2) {
            if (last_mode != 0) {
                tft.fillScreen(TFT_BLACK);
                last_mode = 0;
            }
            return;
        }

        // Chế độ 0: Mochi Animation (Hoạt ảnh Mochi 160x80)
        if (mode == 0) {
            if (last_mode != 0) {
                tft.fillScreen(TFT_BLACK);
                last_mode = 0;
                #if SCREEN_IS_ROUND
                tft.drawCircle(120, 120, 118, TFT_CYAN);
                #else
                tft.setTextColor(TFT_CYAN, TFT_BLACK);
                tft.drawCentreString(ConfigManager::getInstance().get().welcome_text, 160, 20, 4);
                #endif
            }

            uint32_t now = millis();
            if (now - last_anim_frame_time >= 65) { // ~15 FPS
                last_anim_frame_time = now;
                mochi_cur_frame = (mochi_cur_frame + 1) % MOCHI_FRAME_COUNT;

                #if SCREEN_IS_ROUND
                TJpgDec.drawJpg(40, 75, MOCHI_FRAMES[mochi_cur_frame].data, MOCHI_FRAMES[mochi_cur_frame].size);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.drawCentreString(state.current_time_str, 120, 35, 4);
                tft.setTextColor(TFT_GREEN, TFT_BLACK);
                tft.drawCentreString("MOCHI", 120, 175, 2);
                #else
                TJpgDec.drawJpg(80, 65, MOCHI_FRAMES[mochi_cur_frame].data, MOCHI_FRAMES[mochi_cur_frame].size);
                tft.setTextColor(TFT_WHITE, TFT_BLACK);
                tft.drawCentreString(state.current_time_str, 160, 165, 4);
                tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
                tft.drawCentreString("Dang cho Vietmap / Waze...", 160, 205, 2);
                #endif
            }
            return;
        }

        // Chế độ 1: Đồng hồ số lớn (Digital Clock)
        if (last_mode != 0) {
            tft.fillScreen(TFT_BLACK);
            last_mode = 0;
        }

        #if SCREEN_IS_ROUND
        tft.drawCircle(120, 120, 118, TFT_DARKCYAN);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString(state.current_time_str, 120, 80, 7);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString(ConfigManager::getInstance().get().welcome_text, 120, 140, 4);
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawCentreString("Dang cho Vietmap / Waze...", 120, 175, 2);
        #else
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawCentreString(state.current_time_str, 160, 50, 8);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.drawCentreString(ConfigManager::getInstance().get().welcome_text, 160, 135, 4);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.drawCentreString("Vui long mo App Vietmap Live hoac Waze", 160, 175, 2);
        tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
        tft.drawCentreString("Bluetooth ID: VIETMAP_HUD", 160, 205, 2);
        #endif
    }

    // =========================================================================
    // 4. HÀM VẼ ICON CHUẨN XÁC THEO BẢNG TRA CỨU FIRMWARE GỐC
    // =========================================================================
    void drawSpeedLimitSign(int x, int y, int limit) {
        tft.fillRect(x, y, 65, 65, TFT_BLACK);

        int lookup_idx = -1;
        if (limit == 20) lookup_idx = 57;
        else if (limit == 30) lookup_idx = 59;
        else if (limit == 40) lookup_idx = 61;
        else if (limit == 50) lookup_idx = 63;
        else if (limit == 60) lookup_idx = 65;
        else if (limit == 70) lookup_idx = 67;
        else if (limit == 80) lookup_idx = 69;
        else if (limit == 90) lookup_idx = 71;
        else if (limit == 100) lookup_idx = 73;
        else if (limit == 120) lookup_idx = 77;

        if (lookup_idx >= 0 && lookup_idx < 82) {
            uint8_t img_idx = pgm_read_byte(&ORIGINAL_ICON_LOOKUP[lookup_idx]);
            if (img_idx < HUD_ICON_COUNT) {
                TJpgDec.drawJpg(x, y, HUD_ICONS[img_idx].data, HUD_ICONS[img_idx].size);
                return;
            }
        }

        // Fallback: Vẽ biển báo tròn vector nếu limit khác thường
        tft.fillCircle(x + 30, y + 30, 30, TFT_RED);
        tft.fillCircle(x + 30, y + 30, 24, TFT_WHITE);
        tft.setTextColor(TFT_BLACK, TFT_WHITE);
        if (limit > 0 && limit < 200) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", limit);
            tft.drawCentreString(buf, x + 30, y + 15, 4);
        } else {
            tft.drawCentreString("--", x + 30, y + 15, 4);
        }
    }

    void drawTurnIconAndDist(int x, int y, uint8_t turn, uint32_t dist, uint8_t exit_num) {
        tft.fillRect(x, y, 70, 85, TFT_BLACK);

        uint8_t img_idx = 0;
        if (turn < 82) {
            img_idx = pgm_read_byte(&ORIGINAL_ICON_LOOKUP[turn]);
        }

        if (img_idx < HUD_ICON_COUNT) {
            TJpgDec.drawJpg(x + 5, y, HUD_ICONS[img_idx].data, HUD_ICONS[img_idx].size);
        }

        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        if (dist >= 1000) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%.1f km", dist / 1000.0f);
            tft.drawCentreString(buf, x + 35, y + 65, 2);
        } else if (dist > 0) {
            char buf[16];
            snprintf(buf, sizeof(buf), "%u m", dist);
            tft.drawCentreString(buf, x + 35, y + 65, 2);
        }
    }

    void drawAlertCameraIcon(int x, int y) {
        uint8_t cam_idx = 22; // Icon Camera Phạt Nguội gốc
        if (cam_idx < HUD_ICON_COUNT) {
            TJpgDec.drawJpg(x, y, HUD_ICONS[cam_idx].data, HUD_ICONS[cam_idx].size);
        }
    }
};
