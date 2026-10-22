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

class BleVietmapServer : public BLEServerCallbacks, public BLECharacteristicCallbacks {
public:
    static BleVietmapServer& getInstance() {
        static BleVietmapServer instance;
        return instance;
    }

    void begin(SystemState* sysState) {
        state = sysState;
        Serial.println("[BLE] Initializing VIETMAP BLE Server (No-OBD Lightweight)...");

        BLEDevice::init(VIETMAP_DEVICE_NAME);
        BLEDevice::setMTU(512);

        pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);

        pService = pServer->createService(VIETMAP_SERVICE_UUID);

        pCharWrite = pService->createCharacteristic(
            VIETMAP_CHAR_WRITE_UUID,
            BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
        );
        pCharWrite->setCallbacks(this);

        pCharNotify = pService->createCharacteristic(
            VIETMAP_CHAR_NOTIFY_UUID,
            BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
        );
        pCharNotify->addDescriptor(new BLE2902());

        pService->start();

        BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(VIETMAP_SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);
        BLEDevice::startAdvertising();

        Serial.printf("[BLE] Ready & Advertising as '%s'\n", VIETMAP_DEVICE_NAME);
    }

    void onConnect(BLEServer* pServer) override {
        deviceConnected = true;
        if (state) state->ble_phone_connected = true;
        Serial.println("[BLE] Client Connected (Phone App / Web Bluetooth)");
        sendHandshake();
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

    void sendHandshake() {
        const char* handshake = "{\"proto\":[1],\"want\":{\"rate\":4,\"fields\":[\"nav\",\"spd\",\"lim\",\"trn\",\"dst\",\"exit\",\"st\",\"eta\",\"rmin\",\"rkm\",\"avg\",\"avgL\",\"alrs\",\"lan\"]},\"can\":[\"speed\",\"limit\",\"turn\",\"street\",\"eta\",\"avgzone\",\"alerts\"],\"transport\":\"ble\"}\n";
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
        if (pCharNotify && deviceConnected) {
            pCharNotify->setValue((uint8_t*)msg, strlen(msg));
            pCharNotify->notify();
        }
    }

private:
    BLEServer* pServer = nullptr;
    BLEService* pService = nullptr;
    BLECharacteristic* pCharWrite = nullptr;
    BLECharacteristic* pCharNotify = nullptr;
    bool deviceConnected = false;
    SystemState* state = nullptr;

    BleVietmapServer() {}

    void parseIncomingPacket(const uint8_t* data, size_t len) {
        // 1. Kiểm tra heartbeat ping
        if (len >= 6 && memmem(data, len, "\"ping\"", 6) != NULL) {
            sendPong();
            return;
        }

        // 2. Kiểm tra lệnh đọc/ghi cấu hình từ Web Config Tool
        if (len >= 10 && memmem(data, len, "\"get_cfg\"", 9) != NULL) {
            sendConfigJson();
            return;
        }

        if (len >= 10 && memmem(data, len, "\"set_cfg\"", 9) != NULL) {
            String s = "";
            for (size_t i = 0; i < len; i++) s += (char)data[i];
            ConfigManager::getInstance().applyJson(s);
            sendConfigJson();
            return;
        }

        // 3. Giải mã gói tin Navigation thời gian thực từ Vietmap App HOẶC Waze Hub
        StaticJsonDocument<1024> doc;
        DeserializationError err = deserializeJson(doc, data, len);
        if (err || !state) return;

        // --- VIETMAP LIVE HOẶC WAZE HUB DATA PARSER ---
        if (doc.containsKey("spd")) state->nav.speed = doc["spd"].as<int16_t>();
        else if (doc.containsKey("speed")) state->nav.speed = doc["speed"].as<int16_t>();

        if (doc.containsKey("lim")) state->nav.speed_limit = doc["lim"].as<int16_t>();
        else if (doc.containsKey("limit")) state->nav.speed_limit = doc["limit"].as<int16_t>();
        else if (doc.containsKey("speedLimit")) state->nav.speed_limit = doc["speedLimit"].as<int16_t>();

        if (doc.containsKey("nav")) state->nav.nav_active = doc["nav"].as<bool>();
        else if (doc.containsKey("active")) state->nav.nav_active = doc["active"].as<bool>();
        else if (doc.containsKey("turn") || doc.containsKey("trn")) state->nav.nav_active = true;

        if (doc.containsKey("trn")) state->nav.turn_icon = doc["trn"].as<uint8_t>();
        else if (doc.containsKey("turn")) state->nav.turn_icon = doc["turn"].as<uint8_t>();
        else if (doc.containsKey("instruction")) state->nav.turn_icon = doc["instruction"].as<uint8_t>();

        if (doc.containsKey("dst")) state->nav.distance_to_turn = doc["dst"].as<uint32_t>();
        else if (doc.containsKey("distance")) state->nav.distance_to_turn = doc["distance"].as<uint32_t>();

        if (doc.containsKey("exit")) state->nav.exit_num = doc["exit"].as<uint8_t>();
        if (doc.containsKey("st")) state->nav.street_name = doc["st"].as<String>();
        else if (doc.containsKey("street")) state->nav.street_name = doc["street"].as<String>();

        if (doc.containsKey("eta")) state->nav.eta = doc["eta"].as<String>();
        if (doc.containsKey("rmin")) state->nav.remain_min = doc["rmin"].as<uint32_t>();
        if (doc.containsKey("rkm")) state->nav.remain_km = doc["rkm"].as<float>();
        if (doc.containsKey("avg")) state->nav.avg_speed = doc["avg"].as<int16_t>();
        if (doc.containsKey("avgL")) state->nav.avg_speed_limit = doc["avgL"].as<int16_t>();

        if (doc.containsKey("alrs")) state->nav.alerts = doc["alrs"].as<String>();
        else if (doc.containsKey("alert")) state->nav.alerts = doc["alert"].as<String>();

        if (doc.containsKey("lan")) state->nav.lanes = doc["lan"].as<uint8_t>();

        // Đồng bộ thời gian thực từ GPS điện thoại (Epoch timestamp)
        if (doc.containsKey("unix")) {
            uint32_t epoch = doc["unix"].as<uint32_t>();
            // Múi giờ Việt Nam GMT+7 (+25200s)
            uint32_t vn_epoch = epoch + 7 * 3600;
            uint8_t h = (vn_epoch / 3600) % 24;
            uint8_t m = (vn_epoch / 60) % 60;
            char t_str[16];
            snprintf(t_str, sizeof(t_str), "%02d:%02d", h, m);
            state->current_time_str = String(t_str);
        }

        state->nav.last_updated = millis();
    }
};
