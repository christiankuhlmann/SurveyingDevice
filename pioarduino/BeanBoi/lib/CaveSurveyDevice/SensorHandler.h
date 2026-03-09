#ifndef CAVESURVEYDEVICE_SENSORHANDLER_H
#define CAVESURVEYDEVICE_SENSORHANDLER_H

#include <ArduinoEigen.h>
#include <NumericalMethods>
#include "Sensors.h"
#include <EigenFileFuncs.h>
#include <debug_csd.h>
#include <freertos/semphr.h>

using namespace Eigen;

// ---------------------------------------------------------------------------
// Device-runtime constants (calibration-algorithm constants come from
// NumericalMethods config.h: N_ALIGN_MAG_ACC, N_LASER_CAL, N_ORIENTATIONS,
// N_SAMPLES_PER_ORIENTATION, DEVICE_LENGTH)
// ---------------------------------------------------------------------------
const int   N_SHOT_SAMPLES   = 100;    ///< Samples averaged per measurement shot
const int   N_UPDATE_SAMPLES = 5;      ///< Samples averaged per display-refresh reading
const int   N_STABILISATION  = 10;     ///< Ring-buffer depth for stabilisation check
const float STDEV_LIMIT      = 0.05f;  ///< Accelerometer-norm σ threshold for stability
const float MAG_STDEV_FACTOR = 10.0f;  ///< Mag σ threshold multiplier (mag norm varies more)
const int   MAX_STABILISATION_ITERS = 1000;  ///< Max stabilisation iterations before abort

// ---------------------------------------------------------------------------
// Data structures
// ---------------------------------------------------------------------------

/// Calibration & alignment parameters persisted to NVS
struct DeviceCalibrationParameters
{
    Matrix3f Ra_cal, Rm_cal, Ra_las, Rm_las, Rm_align;
    Vector3f ba_cal, bm_cal;
    float inclination_angle;
    NumericalMethods::CalibrationQuality quality;
};

/// Raw laser calibration samples (3 × N_LASER_CAL per sensor)
struct LaserCalibrationData {
    Matrix<float, 3, N_LASER_CAL> mag_data;
    Matrix<float, 3, N_LASER_CAL> acc_data;
};

/// Raw static calibration samples (3 × N_ALIGN_MAG_ACC per sensor)
struct StaticCalibrationData {
    Matrix<float, 3, N_ALIGN_MAG_ACC> mag_data;
    Matrix<float, 3, N_ALIGN_MAG_ACC> acc_data;
};

/// Unified measurement record – replaces the old ShotData and BLE MeasurementData
struct MeasurementRecord {
    // Corrected scalar angles (degrees)
    float heading     = 0.0f;
    float inclination = 0.0f;
    float roll        = 0.0f;

    // Distance (metres, includes DEVICE_LENGTH offset)
    float distance = 0.0f;

    // Raw corrected sensor vectors
    Vector3f mag;
    Vector3f acc;

    // Cartesian direction unit vector
    Vector3f direction;

    // Metadata
    int      ID        = 0;
    uint32_t timestamp = 0;

    /// Pack the scalar fields into a flat byte buffer for BLE transmission.
    /// Returns the number of bytes written (20).
    int toBLEPayload(uint8_t* buf, size_t bufLen) const;
};

// ---------------------------------------------------------------------------
// Shot file I/O helpers (namespace "SD000" – "SD999")
// ---------------------------------------------------------------------------
bool getFileName(unsigned int fileID, char (&fname)[FNAME_LENGTH]);
bool getVarName(unsigned int counter, char (&varname)[VARNAME_LENGTH]);
bool getCounter(unsigned int fileID, unsigned int &counter);
bool setCounter(unsigned int fileID, const unsigned int &counter);
bool saveShotData(const MeasurementRecord &rec, unsigned int fileID);
bool readShotData(MeasurementRecord &rec, unsigned int fileID, unsigned int shotID);
bool readShotData(MeasurementRecord &rec, unsigned int fileID);

// ---------------------------------------------------------------------------
// SensorHandler
// ---------------------------------------------------------------------------
class SensorHandler
{
private:
    int static_calib_progress;  // 0 → N_ORIENTATIONS
    int las_calib_progress;     // 0 → N_LASER_CAL

    // Mutex protecting shared sensor/calibration state
    SemaphoreHandle_t mutex;

    // Sensor objects (references – no extra allocation)
    Accelerometer &acc;
    Magnetometer  &mag;
    Laser         &las;

    // Raw calibration sample buffers
    LaserCalibrationData  laser_calib_data;
    StaticCalibrationData static_calib_data;

    // Computed calibration / alignment parameters
    DeviceCalibrationParameters calib_parms;

    // Live sensor readings & corrected measurement
    MeasurementRecord shot_data;           ///< Raw (uncorrected) shot
    MeasurementRecord corrected_shot_data; ///< Corrected shot / live reading
    Vector3f acc_data, mag_data;
    float    las_data = 0.0f;
    bool     sensors_ready = false;        ///< True after all sensors init successfully

public:
    SensorHandler(Accelerometer &a, Magnetometer &m, Laser &l);

    bool init();

    /// Try to acquire the mutex (non-blocking). Returns true if acquired.
    bool tryLock();
    /// Acquire the mutex (blocking).
    void lock();
    /// Release the mutex.
    void unlock();

    /// Returns true if all sensors initialised successfully.
    bool isSensorsReady() const { return sensors_ready; }

    // --- Accessors ---
    Vector3f getAccData();
    Vector3f getMagData();
    float    getLasData();

    const StaticCalibrationData        &getStaticCalibData();
    const LaserCalibrationData         &getLaserCalibData();
    const DeviceCalibrationParameters  &getCalibParms();

    // --- Live measurement ---
    void update();
    void correctData(Vector3f &m, Vector3f &g);
    MeasurementRecord getShotData(bool corrected = true);

    // --- Flash helpers ---
    void eraseFlash();
    void getFlashStats();

    // --- Shot history ---
    int  getShotCount(unsigned int fileID);
    bool readShotByIndex(MeasurementRecord &rec, unsigned int fileID, unsigned int index);

    // --- Calibration lifecycle ---
    void resetCalibration();
    void saveCalibration();
    void loadCalibration();
    void loadRawCalibrationData();
    void removePrevCalib(bool static_calib);
    int  getCalibProgress();
    int  getCalibProgress(bool static_calib);

    // --- Serial dump ---
    void dumpCalibToSerial();

    /**
     * @brief Take a shot (optionally with laser).
     * @return 0 on success, non-zero on error.
     */
    int takeShot(bool laser_reading = true, bool use_stabilisation = true);

    /**
     * @brief Collect one orientation of static calibration data.
     * @return current progress (1 → N_ORIENTATIONS), or N_ORIENTATIONS when full.
     */
    int collectStaticCalibData();

    /**
     * @brief Collect one orientation of laser calibration data.
     * @return current progress (1 → N_LASER_CAL), or N_LASER_CAL when full.
     */
    int collectLaserCalibData();

    /**
     * @brief Run ellipsoid calibration on static data.
     */
    int calibrate();

    /**
     * @brief Run laser + CWB alignment, then compute calibration quality.
     */
    int align();

    /**
     * @brief Evaluate calibration quality from stored intermediate data.
     */
    void validateCalibrationQuality();
};

#endif