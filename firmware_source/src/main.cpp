#include <Arduino.h>
#include "config_pins.h"
#include "config_manager.h"
#include "hud_data.h"
#include "ble_vietmap_server.h"
#include "display_ui.h"

SystemState g_sysState;
uint32_t g_lastUiUpdate = 0;
uint32_t g_lastTimeUpdate = 0;
uint8_t  g_curMinute = 0;
uint8_t  g_curHour = 12;

void setup() {
    Serial.begin(115200);
    delay(400);
    Serial.println("\n=============================================");
    Serial.printf("   VIETMAP HUD PRO (NO-OBD) - %s\n", BOARD_NAME);
    Serial.println("=============================================");

    // 1. Khởi tạo Nút bấm
    #if BUTTON_PIN >= 0
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    #endif

    // 2. Khởi tạo LED RGB nếu có
    #if LED_RED_PIN >= 0
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    digitalWrite(LED_RED_PIN, HIGH);   // Tắt LED (Active Low)
    digitalWrite(LED_GREEN_PIN, HIGH);
    digitalWrite(LED_BLUE_PIN, HIGH);
    #endif

    // 3. Khởi tạo Filesystem & Cấu hình
    ConfigManager::getInstance().begin();

    // 4. Khởi tạo Màn hình TFT (100% Backlight, HUD Mirror, Speed Halo)
    DisplayUI::getInstance().begin();

    // 5. Khởi tạo BLE Server VIETMAP_HUD & WAZE
    BleVietmapServer::getInstance().begin(&g_sysState);

    Serial.println("[System] Ready for navigation (Max Brightness, HUD Mirror, Speed Halo)!");
}

void loop() {
    uint32_t now = millis();

    // Cập nhật đồng hồ giả lập nội bộ (hoặc từ điện thoại qua unix timestamp)
    if (now - g_lastTimeUpdate > 60000) {
        g_lastTimeUpdate = now;
        g_curMinute++;
        if (g_curMinute >= 60) {
            g_curMinute = 0;
            g_curHour = (g_curHour + 1) % 24;
        }
        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", g_curHour, g_curMinute);
        g_sysState.current_time_str = String(timeBuf);
    }

    // Cập nhật giao diện màn hình (10 FPS)
    if (now - g_lastUiUpdate >= 100) {
        g_lastUiUpdate = now;
        DisplayUI::getInstance().update(g_sysState);
    }

    // Xử lý nút bấm thông minh:
    // - Bấm ngắn (<1.5s): Đổi chế độ màn hình chờ (Mochi / Đồng hồ / Tắt)
    // - Bấm giữ (1.5s - 3.5s): Bật / Tắt chế độ Lật Gương HUD Mirror (Chiếu kính lái)
    // - Bấm giữ rất lâu (>3.5s): Đổi bên Cổng Sạc USB (Trái <-> Phải)
    #if BUTTON_PIN >= 0
    static uint32_t pressStart = 0;
    static bool isPressed = false;
    bool btnState = (digitalRead(BUTTON_PIN) == LOW);

    if (btnState && !isPressed) {
        isPressed = true;
        pressStart = now;
    } else if (!btnState && isPressed) {
        isPressed = false;
        uint32_t pressDur = now - pressStart;
        if (pressDur > 3500) {
            // Bấm giữ > 3.5 giây: Đổi bên cổng sạc
            uint8_t cur_side = ConfigManager::getInstance().get().usb_side;
            ConfigManager::getInstance().get().usb_side = (cur_side == 0) ? 1 : 0;
            ConfigManager::getInstance().saveConfig();
            DisplayUI::getInstance().applyRotationAndMirror();
            Serial.printf("[Button] Switched USB Side to: %s\n", (ConfigManager::getInstance().get().usb_side == 0) ? "LEFT" : "RIGHT");
        } else if (pressDur > 1200) {
            // Bấm giữ 1.2s - 3.5s: Bật / Tắt Lật gương HUD Mirror
            Serial.println("[Button] Toggle HUD Mirror Mode");
            DisplayUI::getInstance().toggleMirror();
        } else if (pressDur > 50) {
            // Bấm ngắn: Đổi chế độ hiển thị
            Serial.println("[Button] Toggle Standby Screen Mode");
            DisplayUI::getInstance().toggleScreenMode();
        }
    }
    #endif

    delay(10);
}
