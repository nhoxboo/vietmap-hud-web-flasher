#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include "config_pins.h"
#include "config_manager.h"
#include "hud_data.h"

// =============================================================================
// VIETMAP BLE PROTOCOL DEFINITIONS (HỖ TRỢ SONG SONG VIETMAP LIVE & WAZE HUB)
// =============================================================================
// 1. Vietmap Standard 16-bit Services & Characteristics (App Vietmap Live bắt buộc)
#define BLE_UUID_DEVINFO_SERVICE      "180A"
#define BLE_UUID_DEVINFO_MODEL        "2A24"
#define BLE_UUID_DEVINFO_FW_REV       "2A26"
#define BLE_UUID_DEVINFO_MANUF        "2A29"

#define BLE_UUID_VIETMAP_16_SERVICE   "FFF0"
#define BLE_UUID_VIETMAP_16_NOTIFY    "FFF1"
#define BLE_UUID_VIETMAP_16_WRITE     "FFF2"
#define BLE_UUID_VIETMAP_16_CONFIG    "FFF3"

// 2. Vietmap 128-bit Extended Services (Dành cho Waze Hub & Web Bluetooth)
#define BLE_UUID_VIETMAP_128_SERVICE  "8a7e0001-4d6e-4c48-9a9d-484c504c0001"
#define BLE_UUID_VIETMAP_128_WRITE    "8a7e0002-4d6e-4c48-9a9d-484c504c0001"
#define BLE_UUID_VIETMAP_128_NOTIFY   "8a7e0003-4d6e-4c48-9a9d-484c504c0001"

class BleVietmapServer : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    static BleVietmapServer& getInstance() {
        static BleVietmapServer instance;
        return instance;
    }

    void begin(SystemState* sysState) {
        state = sysState;
        Serial.println("[BLE] Initializing VIETMAP Dual BLE Server (Vietmap Live 0xFFF0 + Waze Hub 128-bit)...");

        BLEDevice::init(VIETMAP_DEVICE_NAME);
        BLEDevice::setMTU(512);

        pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);

        // ---------------------------------------------------------------------
        // 1. Device Information Service (0x180A) - Bắt buộc để Vietmap Live nhận diện H1V
        // ---------------------------------------------------------------------
        BLEService* pDevInfoSvc = pServer->createService(BLE_UUID_DEVINFO_SERVICE);
        BLECharacteristic* pCharModel = pDevInfoSvc->createCharacteristic(
            BLE_UUID_DEVINFO_MODEL, BLECharacteristic::PROPERTY_READ
        );
        pCharModel->setValue("H1V");

        BLECharacteristic* pCharFw = pDevInfoSvc->createCharacteristic(
            BLE_UUID_DEVINFO_FW_REV, BLECharacteristic::PROPERTY_READ
        );
        pCharFw->setValue("v@7.8");

        BLECharacteristic* pCharManuf = pDevInfoSvc->createCharacteristic(
            BLE_UUID_DEVINFO_MANUF, BLECharacteristic::PROPERTY_READ
        );
        pCharManuf->setValue("VIETMAP");
        pDevInfoSvc->start();

        // ---------------------------------------------------------------------
        // 2. Vietmap 16-bit Custom Service (0xFFF0) - Kênh giao tiếp của App Vietmap Live
        // ---------------------------------------------------------------------
        BLEService* pVietmap16Svc = pServer->createService(BLE_UUID_VIETMAP_16_SERVICE);

        pChar16Notify = pVietmap16Svc->createCharacteristic(
            BLE_UUID_VIETMAP_16_NOTIFY,
            BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
        );
        pChar16Notify->addDescriptor(new BLE2902());

        pChar16Write = pVietmap16Svc->createCharacteristic(
            BLE_UUID_VIETMAP_16_WRITE,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
        );
        pChar16Write->setCallbacks(this);

        pChar16Config = pVietmap16Svc->createCharacteristic(
            BLE_UUID_VIETMAP_16_CONFIG,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY
        );
        pChar16Config->addDescriptor(new BLE2902());
        pChar16Config->setCallbacks(this);
        pVietmap16Svc->start();

        // ---------------------------------------------------------------------
        // 3. Vietmap 128-bit Extended Service - Kênh giao tiếp của Waze Hub & Web Bluetooth
        // ---------------------------------------------------------------------
        BLEService* pVietmap128Svc = pServer->createService(BLE_UUID_VIETMAP_128_SERVICE);

        pChar128Write = pVietmap128Svc->createCharacteristic(
            BLE_UUID_VIETMAP_128_WRITE,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
        );
        pChar128Write->setCallbacks(this);

        pChar128Notify = pVietmap128Svc->createCharacteristic(
            BLE_UUID_VIETMAP_128_NOTIFY,
            BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
        );
        pChar128Notify->addDescriptor(new BLE2902());
        pVietmap128Svc->start();

        // ---------------------------------------------------------------------
        // 4. Advertising: Quảng bá song song cả 2 Service UUID
        // ---------------------------------------------------------------------
        BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(BLE_UUID_VIETMAP_16_SERVICE);
        pAdvertising->addServiceUUID(BLE_UUID_VIETMAP_128_SERVICE);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();

        Serial.printf("[BLE] Ready & Advertising as '%s' (Services: 0xFFF0, 0x180A, 8a7e0001-...)\n", VIETMAP_DEVICE_NAME);
    }

    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        if (state) state->ble_phone_connected = true;
        Serial.println("[BLE] Client Connected! Sending Binary & JSON Handshakes...");

        // Gửi chủ động cả 2 dạng handshake để đảm bảo tương thích 100%:
        // 1. Handshake nhị phân 92 bytes cho App Vietmap Live
        sendBinaryHandshake();

        // 2. Handshake JSON cho Waze Hub & Web Configurator
        sendJsonHandshake();
    }

    void onDisconnect(BLEServer* pServer) override {
        deviceConnected = false;
        if (state) {
            state->ble_phone_connected = false;
            state->nav.nav_active = false;
        }
        Serial.println("[BLE] Client Disconnected. Restarting Advertising...");
        BLEDevice::startAdvertising();
    }

    void onWrite(BLECharacteristic* pCharacteristic) override {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            parseIncomingPacket((const uint8_t*)rxValue.data(), rxValue.length());
        }
    }

    // =========================================================================
    // 1. HANDSHAKE NHỊ PHÂN 92 BYTES CHUẨN CỦA VIETMAP LIVE H1V
    // =========================================================================
    void sendBinaryHandshake() {
        uint8_t handshakeResp[92];
        memset(handshakeResp, 0, sizeof(handshakeResp));

        // Header magic
        handshakeResp[0] = 0xA5;
        handshakeResp[1] = 0x5A;
        handshakeResp[2] = 0x37;
        handshakeResp[3] = 0xC3;
        handshakeResp[7] = 0x5C; // 92 bytes

        handshakeResp[8]  = 0x64;
        handshakeResp[9]  = 0xE2;
        handshakeResp[10] = 0x2D;
        handshakeResp[11] = 0xCA;
        handshakeResp[12] = 0x0E;

        // Chuỗi nhận diện phiên bản firmware chuẩn H1V
        const char* infoStr = "MODEL:H1V,HW:1.6.4,FW:1.3.6,PROTOCOL:2.2.2, OBDV:,COMPILE:Feb 27 2021-12:13:56";
        size_t infoLen = strlen(infoStr);
        if (infoLen > 67) infoLen = 67;
        memcpy(&handshakeResp[16], infoStr, infoLen);

        // Chữ ký bảo mật ở đuôi gói tin
        uint8_t sig[] = { 0xC0, 0xFE, 0xB7, 0x7E, 0x6B, 0x94, 0x21, 0x5F };
        memcpy(&handshakeResp[84], sig, sizeof(sig));

        // Bắn notify qua cả 16-bit và 128-bit
        if (pChar16Notify && deviceConnected) {
            pChar16Notify->setValue(handshakeResp, sizeof(handshakeResp));
            pChar16Notify->notify();
        }
        if (pChar16Config && deviceConnected) {
            pChar16Config->setValue(handshakeResp, sizeof(handshakeResp));
            pChar16Config->notify();
        }
        if (pChar128Notify && deviceConnected) {
            pChar128Notify->setValue(handshakeResp, sizeof(handshakeResp));
            pChar128Notify->notify();
        }
    }

    // =========================================================================
    // 2. HANDSHAKE JSON (WAZE HUB & WEB BLUETOOTH)
    // =========================================================================
    void sendJsonHandshake() {
        const char* handshake = "{\"v\":1,\"t\":\"dev\",\"name\":\"Promax\",\"fw\":\"1.0.0\",\"proto\":[1],\"want\":{\"rate\":4,\"fields\":[\"nav\",\"spd\",\"lim\",\"trn\",\"dst\",\"exit\",\"st\",\"eta\",\"rmin\",\"rkm\",\"avg\",\"avgL\",\"alrs\",\"lan\"]},\"can\":[\"speed\",\"limit\",\"turn\",\"street\",\"eta\",\"avgzone\",\"alerts\"],\"transport\":\"ble\"}\n";
        sendNotification(handshake);
    }

    void sendPong() {
        const char* pong = "{\"v\":1,\"t\":\"pong\"}\r\n";
        sendNotification(pong);
    }

    void sendConfigJson() {
        String json = "{\"type\":\"cfg_resp\"," + ConfigManager::getInstance().toJsonString().substring(1) + "\n";
        sendNotification(json.c_str());
    }

    void sendNotification(const char* msg) {
        if (!deviceConnected) return;
        size_t len = strlen(msg);
        if (pChar16Notify) {
            pChar16Notify->setValue((uint8_t*)msg, len);
            pChar16Notify->notify();
        }
        if (pChar128Notify) {
            pChar128Notify->setValue((uint8_t*)msg, len);
            pChar128Notify->notify();
        }
    }

private:
    BleVietmapServer() : pServer(nullptr),
                         pChar16Notify(nullptr), pChar16Write(nullptr), pChar16Config(nullptr),
                         pChar128Write(nullptr), pChar128Notify(nullptr),
                         deviceConnected(false), state(nullptr) {}

    BLEServer* pServer;
    BLECharacteristic* pChar16Notify;
    BLECharacteristic* pChar16Write;
    BLECharacteristic* pChar16Config;
    BLECharacteristic* pChar128Write;
    BLECharacteristic* pChar128Notify;
    bool deviceConnected;
    SystemState* state;

    void parseIncomingPacket(const uint8_t* data, size_t len) {
        if (len == 0 || !state) return;
        state->ble_phone_connected = true;

        // ---------------------------------------------------------------------
        // A. XỬ LÝ GÓI TIN NHỊ PHÂN VIETMAP LIVE GỐC (Magic: 0xA5 0x5A)
        // ---------------------------------------------------------------------
        if (len >= 4 && data[0] == 0xA5 && data[1] == 0x5A) {
            // Handshake request: 0xA5 0x5A 0x37 0xC3
            if (data[2] == 0x37 && data[3] == 0xC3) {
                Serial.println("[BLE] Vietmap Handshake Request (0xA5 0x5A 0x37 0xC3) received! Responding...");
                sendBinaryHandshake();
                return;
            }

            // Giải mã gói tin Vietmap Live Binary
            parseVietmapBinaryPayload(data, len);
            return;
        }

        // ---------------------------------------------------------------------
        // B. XỬ LÝ LỆNH JSON TỪ WEB BLUETOOTH / WAZE HUB
        // ---------------------------------------------------------------------
        // 1. Ping / Pong
        if (len >= 10 && memmem(data, len, "\"t\":\"ping\"", 10) != NULL) {
            sendPong();
            return;
        }

        // 2. Web Bluetooth Config Query (get_cfg) & Save (set_cfg)
        if (len >= 10 && (memmem(data, len, "\"get_cfg\"", 9) != NULL || memmem(data, len, "\"op\":\"get_cfg\"", 14) != NULL)) {
            sendConfigJson();
            return;
        }

        if (len >= 10 && (memmem(data, len, "\"set_cfg\"", 9) != NULL || memmem(data, len, "\"op\":\"set_cfg\"", 14) != NULL)) {
            String s = "";
            for (size_t i = 0; i < len; i++) s += (char)data[i];
            ConfigManager::getInstance().applyJson(s);
            sendConfigJson();
            return;
        }

        // 3. Giải mã gói tin Navigation JSON từ Waze Hub HOẶC Vietmap App (phiên bản mới)
        StaticJsonDocument<1024> doc;
        DeserializationError err = deserializeJson(doc, data, len);
        if (err) return;

        // --- TỐC ĐỘ HIỆN TẠI ---
        if (doc.containsKey("spd")) state->nav.speed = doc["spd"].as<int16_t>();
        else if (doc.containsKey("speed")) state->nav.speed = doc["speed"].as<int16_t>();

        // --- TỐC ĐỘ GIỚI HẠN ---
        if (doc.containsKey("lim")) state->nav.speed_limit = doc["lim"].as<int16_t>();
        else if (doc.containsKey("limit")) state->nav.speed_limit = doc["limit"].as<int16_t>();
        else if (doc.containsKey("speedLimit")) state->nav.speed_limit = doc["speedLimit"].as<int16_t>();

        // --- TRẠNG THÁI DẪN ĐƯỜNG ---
        if (doc.containsKey("nav")) state->nav.nav_active = doc["nav"].as<bool>();
        else if (doc.containsKey("active")) state->nav.nav_active = doc["active"].as<bool>();
        else if (doc.containsKey("turn") || doc.containsKey("trn")) state->nav.nav_active = true;

        // --- ICON HƯỚNG RẼ ---
        if (doc.containsKey("trn")) state->nav.turn_icon = doc["trn"].as<uint8_t>();
        else if (doc.containsKey("turn")) state->nav.turn_icon = doc["turn"].as<uint8_t>();
        else if (doc.containsKey("instruction")) state->nav.turn_icon = doc["instruction"].as<uint8_t>();

        // --- KHOẢNG CÁCH TỚI NGÃ RẼ ---
        if (doc.containsKey("dst")) state->nav.distance_to_turn = doc["dst"].as<uint32_t>();
        else if (doc.containsKey("distance")) state->nav.distance_to_turn = doc["distance"].as<uint32_t>();

        // --- LỐI RA BÙNG BINH ---
        if (doc.containsKey("exit")) state->nav.exit_num = doc["exit"].as<uint8_t>();

        // --- TÊN ĐƯỜNG ĐANG ĐI ---
        if (doc.containsKey("st")) state->nav.street_name = doc["st"].as<String>();
        else if (doc.containsKey("street")) state->nav.street_name = doc["street"].as<String>();

        // --- THỜI GIAN ĐỒNG BỘ RTC TỪ GPS (UNIX EPOCH) ---
        if (doc.containsKey("unix")) {
            time_t t = doc["unix"].as<time_t>() + (7 * 3600); // Múi giờ GMT+7
            struct tm* tm_info = gmtime(&t);
            if (tm_info) {
                char buf[10];
                snprintf(buf, sizeof(buf), "%02d:%02d", tm_info->tm_hour, tm_info->tm_min);
                state->current_time_str = String(buf);
            }
        }
    }

    // =========================================================================
    // GIẢI MÃ PAYLOAD NHỊ PHÂN VIETMAP LIVE GỐC
    // =========================================================================
    void parseVietmapBinaryPayload(const uint8_t* data, size_t len) {
        if (len < 13 || !state) return;

        // Opcode của gói tin nằm ở offset 12
        uint8_t opcode = data[12];

        switch (opcode) {
            case 0x01: // Tốc độ thời gian thực (Live Speed)
                if (len >= 15) {
                    uint16_t raw_speed = (data[13] << 8) | data[14];
                    state->nav.speed = raw_speed;
                }
                break;

            case 0x10: // Biển báo giới hạn tốc độ (Speed Limit)
                if (len >= 14) {
                    state->nav.speed_limit = data[13];
                }
                break;

            case 0x11: // Cảnh báo Camera / POI
                if (len >= 17) {
                    state->nav.alerts = (data[13] > 0) ? "CAMERA PHAT NGUOI" : "";
                }
                break;

            case 0x40: // Hướng dẫn rẽ Turn-by-Turn
                if (len >= 18) {
                    state->nav.nav_active = true;
                    state->nav.turn_icon = data[13];
                    state->nav.distance_to_turn = (data[14] << 8) | data[15];

                    // Tên đường bằng chuỗi text nếu có
                    if (len >= 23) {
                        uint8_t text_len = data[22];
                        if (text_len > 0 && len >= (size_t)(23 + text_len)) {
                            char street[128];
                            size_t c_len = (text_len < 127) ? text_len : 127;
                            memcpy(street, &data[23], c_len);
                            street[c_len] = '\0';
                            state->nav.street_name = String(street);
                        }
                    }
                }
                break;

            default:
                break;
        }
    }
};
