#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include <NimBLEDevice.h>

/** @brief MeasurementData class to hold measurement data.
 * 
 * This class encapsulates the measurement data including heading, inclination,
 * roll, distance, and timestamp. It provides setter and getter methods for each
 * of these attributes.
 */
struct MeasurementData {
private:
    float heading;
    float inclination;
    float roll;
    float distance;
    uint32_t timestamp;

public:
    void setHeading(float h) { heading = h; }
    void setInclination(float i) { inclination = i; }
    void setRoll(float r) { roll = r; }
    void setDistance(float d) { distance = d; }
    void setTimestamp(uint32_t ts) { timestamp = ts; }

    float getHeading() const { return heading; }
    float getInclination() const { return inclination; }
    float getRoll() const { return roll; }
    float getDistance() const { return distance; }
    uint32_t getTimestamp() const { return timestamp; }
};


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
 * 
 * @return true if initialization was successful, false otherwise.
 */
void startBLETask();

/**
 * @brief Sends measurement data over BLE.
 * 
 * This function sends the provided measurement data to the BLE queue.
 * 
 * @param data The measurement data to send.
 */
void sendBLEData(const MeasurementData& data);

#endif // BLE_MANAGER_H