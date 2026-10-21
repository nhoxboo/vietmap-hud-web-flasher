#pragma once
#include <Arduino.h>

enum TurnDirection {
    TURN_NONE = 0,
    TURN_STRAIGHT = 1,
    TURN_SLIGHT_RIGHT = 2,
    TURN_RIGHT = 3,
    TURN_SHARP_RIGHT = 4,
    TURN_UTURN_RIGHT = 5,
    TURN_UTURN_LEFT = 6,
    TURN_SHARP_LEFT = 7,
    TURN_LEFT = 8,
    TURN_SLIGHT_LEFT = 9,
    TURN_ROUNDABOUT = 10,
    TURN_EXIT = 11,
    TURN_ARRIVED = 12
};

struct NavigationData {
    bool     nav_active = false;        // Trạng thái dẫn đường
    int16_t  speed = 0;                 // Tốc độ hiện tại (km/h)
    int16_t  speed_limit = 0;           // Giới hạn tốc độ (km/h) (0 = không giới hạn)
    uint8_t  turn_icon = 0;             // Hướng rẽ
    uint32_t distance_to_turn = 0;      // Khoảng cách tới chỗ rẽ (mét)
    uint8_t  exit_num = 0;              // Lối ra
    String   street_name = "";          // Tên đường
    String   eta = "";                  // Giờ đến
    uint32_t remain_min = 0;            // Phút còn lại
    float    remain_km = 0.0f;          // Km còn lại
    int16_t  avg_speed = 0;             // Tốc độ trung bình
    int16_t  avg_speed_limit = 0;       // Giới hạn tốc độ TB
    String   alerts = "";               // Cảnh báo camera / tốc độ / khu dân cư
    uint8_t  lanes = 0;                 // Làn đường
    uint32_t last_updated = 0;          // Thời gian cập nhật cuối (ms)
};

struct SystemState {
    NavigationData nav;
    bool ble_phone_connected = false;
    String current_time_str = "12:00";
    String current_date_str = "";
    int8_t current_temp = 28;
    bool is_overspeed = false;
    uint8_t screen_mode = 0;            // 0: Clock, 1: Nav, 2: Setting
    uint16_t ambient_light = 500;       // Giá trị quang trở LDR
};
