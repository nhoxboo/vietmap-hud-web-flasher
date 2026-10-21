# VIETMAP HUD Web Flasher & Configurator Pro

Trang web mã nguồn mở tùy biến và nạp Firmware VIETMAP HUD trực tiếp qua trình duyệt web (Web Serial & Web Bluetooth API) cho các dòng thiết bị ESP32 màn hình màu:
- **ESP32-2432S024** (CYD Cheap Yellow Display 2.4" / 2.8" ILI9341 320x240)
- **ESP32-2424S012** (ESP32 Màn hình tròn 1.28" GC9A01 240x240)

---

## 🌟 TÍNH NĂNG NỔI BẬT

1. **⚡ Nạp Firmware 1-Click (Web Serial)**:
   - Nạp trực tiếp qua Google Chrome / Microsoft Edge mà không cần cài đặt Arduino IDE, Python hay driver phức tạp.
   - Hỗ trợ cả 2 dòng màn hình phổ biến nhất hiện nay.
2. **🎛️ Cấu hình HUD qua Web Bluetooth (BLE Configurator)**:
   - Kết nối không dây từ trình duyệt đến HUD.
   - Cài đặt độ sáng ngày/đêm, tự động điều chỉnh qua quang trở LDR.
   - Bù trừ tốc độ cảnh báo (+/- km/h), bật/tắt tiếng bíp quá tốc độ và đổi biển báo.
   - Thay đổi logo khởi động, xoay hướng màn hình ngang/dọc.
3. **🖥️ Live HUD Simulator**:
   - Trực quan hóa giao diện HUD thực tế trên cả màn hình tròn và màn hình chữ nhật.
   - Giả lập điều chỉnh tốc độ, đổi hướng rẽ, khoảng cách ngã rẽ và các cảnh báo giao thông (Camera phạt nguội, khu dân cư, bắn tốc độ).
4. **🚀 Tối ưu hóa cực nhẹ (No-OBD)**:
   - Loại bỏ hoàn toàn module đọc OBD-II giúp tiết kiệm 45KB RAM và hơn 300KB Flash.
   - Khởi động tức thì, tốc độ phản hồi BLE đạt 4Hz mượt mà, không bị giật lag khi lái xe.

---

## 🔌 SƠ ĐỒ CHÂN PHẦN CỨNG (PINOUT)

| Chức năng | ESP32-2432S024 (CYD 2.4") | ESP32-2424S012 (Round 1.28") | Ghi chú |
| :--- | :---: | :---: | :--- |
| **Màn hình LCD** | **ILI9341 SPI (320x240)** | **GC9A01 SPI (240x240)** | Hardware SPI 40MHz |
| **SPI SCLK** | GPIO 14 | GPIO 14 | Xung nhịp Clock |
| **SPI MOSI** | GPIO 13 | GPIO 13 | Dữ liệu hình ảnh |
| **TFT CS** | GPIO 15 | GPIO 15 | Chip Select (Active LOW) |
| **TFT DC** | GPIO 2 | GPIO 2 | Data / Command |
| **TFT BL** | GPIO 21 | GPIO 21 | Đèn nền Backlight PWM |
| **Còi Buzzer** | GPIO 26 | GPIO 25 | Còi bíp cảnh báo |
| **Quang trở LDR**| GPIO 34 | - | Tự giảm sáng ban đêm |

---

## 📱 HƯỚNG DẪN KẾT NỐI VỚI APP VIETMAP TRÊN ĐIỆN THOẠI

1. Cấp nguồn cho ESP32 qua cổng USB Type-C trên ô tô / xe máy.
2. Màn hình HUD sẽ khởi động và thông báo `BLE: Ready`.
3. Mở ứng dụng **Vietmap Live** trên điện thoại (iPhone hoặc Android).
4. Vào **Cài đặt** &rarr; **Kết nối HUD** &rarr; Chọn thiết bị **VIETMAP_HUD**.
5. Màn hình HUD sẽ lập tức hiển thị tốc độ, biển báo và dẫn đường thời gian thực!

---

## 🛠️ BIÊN DỊCH BẰNG PLATFORMIO (OFFLINE)

Nếu muốn tự biên dịch hoặc tùy biến mã nguồn:
```bash
cd firmware_source

# Build cho ESP32-2432S024 (CYD 2.4")
pio run -e esp32_2432s024 -t upload

# Build cho ESP32-2424S012 (Màn hình tròn 1.28")
pio run -e esp32_2424s012 -t upload
```

---
*Phát triển bởi nhoxboo - 2026*
