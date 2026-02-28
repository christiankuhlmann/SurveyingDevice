#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <SensorHandler.h>   // MeasurementRecord

struct BLEData {
private:
    char data[32];
    bool updated;

public:
    void setData(const char* newData) {
        strncpy(data, newData, sizeof(data) - 1);
        data[sizeof(data) - 1] = '\0'; // Ensure null termination
        updated = true;
    }

    const char* getData() const { return data; }
    bool isUpdated() const { return updated; }
    void setUpdated(bool yn) { updated = yn; }
};


/**
 * @brief Initializes the BLE manager.
 * 
 * This function sets up the BLE device, initializes the BLE service,
 * and starts advertising.
 */
void startBLETask();

/**
 * @brief Sends measurement data over BLE.
 * 
 * This function sends the provided measurement record to the BLE queue.
 * 
 * @param rec The measurement record to send.
 */
void sendBLEData(const MeasurementRecord& rec);

#endif // BLE_MANAGER_H