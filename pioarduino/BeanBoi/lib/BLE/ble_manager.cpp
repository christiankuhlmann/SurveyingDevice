#include "ble_manager.h"

// Globals
static BLEData sharedBLEData;
static QueueHandle_t bleSendQueue;
static SemaphoreHandle_t bleDataMutex;
static bool deviceConnected = false;

static MeasurementData dataToSend;
static char payload[64];


// Server pointer
static NimBLEServer *pServer = nullptr;

// Service pointer
static NimBLEService *pService = nullptr;

// Characteristic pointers
static NimBLECharacteristic* pTxCharacteristic = nullptr;
static NimBLECharacteristic* pRxCharacteristic = nullptr;

// Advertising pointer
static NimBLEAdvertising *pAdvertising = nullptr;


// BLE UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_TX   "12345678-1234-1234-1234-1234567890ac" // Notify
#define CHARACTERISTIC_RX   "12345678-1234-1234-1234-1234567890ad" // Write

// BLE Callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(NimBLEServer* pServer) { deviceConnected = false; }
};

class RxCallbacks : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        
        std::string value = pCharacteristic->getValue();
        xSemaphoreTake(bleDataMutex, portMAX_DELAY);

        sharedBLEData.setData(value.c_str());
        sharedBLEData.setUpdated(true);
        
        xSemaphoreGive(bleDataMutex);
    }
};

// BLE task function (runs on core 1)
void bleTask(void* parameter) {
    // Initialize NimBLE
    NimBLEDevice::init("ESP32-BLE");

    // Create server and set callbacks
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    pService = pServer->createService(SERVICE_UUID);

    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_TX,
        NIMBLE_PROPERTY::NOTIFY
    );
    
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_RX,
        NIMBLE_PROPERTY::WRITE
    );

    pRxCharacteristic->setCallbacks(new RxCallbacks());

    pService->start();
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();

    for (;;) {
        // Wait for MeasurementData from queue (from core 0)
        if (xQueueReceive(bleSendQueue, &dataToSend, 100 / portTICK_PERIOD_MS) == pdTRUE) {
            if (deviceConnected && pTxCharacteristic) {
                snprintf(payload, sizeof(payload), "{\"h\":%.2f,\"i\":%.2f,\"r\":%.2f,\"d\":%.2f,\"ts\":%lu}",
                    dataToSend.getHeading(), dataToSend.getInclination(), dataToSend.getRoll(), dataToSend.getDistance(), dataToSend.getTimestamp());
                pTxCharacteristic->setValue((uint8_t*)payload, strlen(payload));
                pTxCharacteristic->notify();
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// API: Call from Core 0 to queue data for BLE
void sendBLEData(const MeasurementData& data) {
    if (bleSendQueue) {
        xQueueSend(bleSendQueue, &data, 0);
    }
}

// API: Start BLE task on Core 1
void startBLETask() {
    bleSendQueue = xQueueCreate(4, sizeof(MeasurementData));
    bleDataMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        bleTask, "bleTask", 4096, nullptr, 1, nullptr, 1  // core 1
    );
}