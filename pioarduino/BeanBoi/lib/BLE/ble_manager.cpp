#include "ble_manager.h"

// BLE operation timeout constants
constexpr TickType_t BLE_MUTEX_TIMEOUT_MS = portMAX_DELAY;     // Mutex acquisition timeout
constexpr TickType_t BLE_QUEUE_TIMEOUT_MS = 0;       // Non-blocking queue send

// Globals
static BLEData sharedBLEData;
static QueueHandle_t bleSendQueue;
static SemaphoreHandle_t bleDataMutex;
static bool deviceConnected = false;

static MeasurementRecord dataToSend;


// Server pointer
static NimBLEServer *pServer = nullptr;

// Service pointer
static NimBLEService *pService = nullptr;

// Characteristic pointers
static NimBLECharacteristic* pCmdIDCharacteristic = nullptr;
static NimBLECharacteristic* pCmdDataCharacteristic = nullptr;
static NimBLECharacteristic* pCmdRdyCharacteristic = nullptr;

static NimBLECharacteristic* pHCharacteristic = nullptr;
static NimBLECharacteristic* pICharacteristic = nullptr;
static NimBLECharacteristic* pRCharacteristic = nullptr;
static NimBLECharacteristic* pDCharacteristic = nullptr;
static NimBLECharacteristic* pTSCharacteristic = nullptr;
static NimBLECharacteristic* pRdyCharacteristic = nullptr;
static NimBLECharacteristic* pRcvCharacteristic = nullptr;


static NimBLECharacteristic* pRxCharacteristic = nullptr;

// Advertising pointer
static NimBLEAdvertising *pAdvertising = nullptr;

static volatile bool data_received_successfully = false;

// BLE UUIDs
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"

#define CHARACTERISTIC_HEADING      "12345678-1234-1234-1234-100000000001" 
#define CHARACTERISTIC_INCLINATION  "12345678-1234-1234-1234-100000000002"
#define CHARACTERISTIC_ROLL         "12345678-1234-1234-1234-100000000003"
#define CHARACTERISTIC_DISTANCE     "12345678-1234-1234-1234-100000000004"
#define CHARACTERISTIC_TIMESTAMP    "12345678-1234-1234-1234-100000000005" 
#define CHARACTERISTIC_READY        "12345678-1234-1234-1234-100000000006"
#define CHARACTERISTIC_RECEIVED     "12345678-1234-1234-1234-100000000007"

#define CHARACTERISTIC_COMMAND_ID       "12345678-1234-1234-1234-100000000011"
#define CHARACTERISTIC_COMMAND_DATA     "12345678-1234-1234-1234-100000000012"
#define CHARACTERISTIC_COMMAND_READY    "12345678-1234-1234-1234-100000000013"


// BLE Callbacks
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) { deviceConnected = true; }
    void onDisconnect(NimBLEServer* pServer) { deviceConnected = false; }
};

class CmdRdyCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        // Get value outside critical section to minimize lock time
        std::string value = pCharacteristic->getValue();
        
        // Use timeout instead of portMAX_DELAY to prevent deadlock
        if (xSemaphoreTake(bleDataMutex, pdMS_TO_TICKS(BLE_MUTEX_TIMEOUT_MS)) == pdTRUE) {
            sharedBLEData.setData(value.c_str());
            sharedBLEData.setUpdated(true);
            xSemaphoreGive(bleDataMutex);
        } else {
            // Mutex timeout - log but don't block
            Serial.println("BLE: Mutex timeout in onWrite callback");
        }
    }
};

class DataRcvCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* pCharacteristic) {
        if (pRcvCharacteristic->getValue()) {
            // Reset the flag
            data_received_successfully = true;
            pRcvCharacteristic->setValue(false);
        } else {
            // If the value is false, it means the client has not received the data yet
            data_received_successfully = false;
        }
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

    /*******************************************************************************
     * Create command characteristics
     * These characteristics are used to receive commands from the client.
     * They are read-only and notify the server when data is ready.
     *******************************************************************************/ 
    pCmdIDCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_COMMAND_ID,
        NIMBLE_PROPERTY::READ
    );

    pCmdDataCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_COMMAND_DATA,
        NIMBLE_PROPERTY::READ
    );

    pCmdRdyCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_COMMAND_READY,
        NIMBLE_PROPERTY::NOTIFY
    );

    /*******************************************************************************
     * Create data characteristics
     * These characteristics are used to send data from the server to the client.
     * They are writable and notify the client when data is ready.
     *******************************************************************************/ 
    pHCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_HEADING,
        NIMBLE_PROPERTY::WRITE
    );

    pICharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_INCLINATION,
        NIMBLE_PROPERTY::WRITE
    );

    pRCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_ROLL,
        NIMBLE_PROPERTY::WRITE
    );

    pDCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_DISTANCE,
        NIMBLE_PROPERTY::WRITE
    );

    pTSCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_TIMESTAMP,
        NIMBLE_PROPERTY::WRITE
    );

    pRdyCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_READY,
        NIMBLE_PROPERTY::WRITE
    );

    pRcvCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_RECEIVED,
        NIMBLE_PROPERTY::NOTIFY
    );


    /*******************************************************************************
     * Create callbacks for the data characteristics
     *******************************************************************************/ 
    pCmdRdyCharacteristic->setCallbacks(new CmdRdyCallback());
    pRcvCharacteristic->setCallbacks(new DataRcvCallback());

    pService->start();
    pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();

    for (;;) {
        // Wait for MeasurementRecord from queue (from core 0)
        if (xQueueReceive(bleSendQueue, &dataToSend, 100 / portTICK_PERIOD_MS) == pdTRUE) {
            if (deviceConnected && pRcvCharacteristic) {
                //Keep trying until data is received successfully, with retry limit
                int retries = 0;
                const int MAX_BLE_RETRIES = 10;
                while (!data_received_successfully && deviceConnected && retries < MAX_BLE_RETRIES) {
                    pHCharacteristic->setValue(dataToSend.heading);
                    pICharacteristic->setValue(dataToSend.inclination);
                    pRCharacteristic->setValue(dataToSend.roll);
                    pDCharacteristic->setValue(dataToSend.distance);
                    pTSCharacteristic->setValue(dataToSend.timestamp);
                    pRdyCharacteristic->setValue(true);
                    pRdyCharacteristic->notify();
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                    retries++;
                }
                if (!data_received_successfully) {
                    Serial.println("BLE: Data send failed after retries or disconnect");
                }
                data_received_successfully = false;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// API: Call from Core 0 to queue data for BLE
void sendBLEData(const MeasurementRecord& data) {
    if (!bleSendQueue) {
        Serial.println("BLE: Queue not initialized");
        return;
    }
    
    // Non-blocking send - if queue is full, data is dropped
    if (xQueueSend(bleSendQueue, &data, BLE_QUEUE_TIMEOUT_MS) != pdTRUE) {
        Serial.println("BLE: Queue full - measurement data dropped");
    }
}

// API: Start BLE task on Core 1
void startBLETask() {
    bleSendQueue = xQueueCreate(4, sizeof(MeasurementRecord));
    bleDataMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(
        bleTask,
        "bleTask",
        4096,       // Stack: 4096 words (16KB) for BLE operations
        nullptr,
        1,          // Priority: 1 (same as COMPUTE, background task)
        nullptr,
        1           // Core 1: Dedicated to BLE to avoid interference with sensors
    );
}