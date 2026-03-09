#include "SensorHandler.h"
#include <stdint.h>

// ---------------------------------------------------------------------------
// MeasurementRecord helpers
// ---------------------------------------------------------------------------

int MeasurementRecord::toBLEPayload(uint8_t* buf, size_t bufLen) const
{
    const size_t needed = 4 * sizeof(float) + sizeof(uint32_t); // 20 bytes
    if (bufLen < needed) return 0;
    size_t off = 0;
    memcpy(buf + off, &heading,     sizeof(float));   off += sizeof(float);
    memcpy(buf + off, &inclination, sizeof(float));   off += sizeof(float);
    memcpy(buf + off, &roll,        sizeof(float));   off += sizeof(float);
    memcpy(buf + off, &distance,    sizeof(float));   off += sizeof(float);
    memcpy(buf + off, &timestamp,   sizeof(uint32_t));off += sizeof(uint32_t);
    return (int)off;
}

// ---------------------------------------------------------------------------
// Shot data serialization – explicit float packing (layout-safe)
// ---------------------------------------------------------------------------

/// Packed layout: h, i, r, d, mag[3], acc[3], dir[3], ID(as float), ts(as float) = 15 floats
static const size_t PACKED_RECORD_FLOATS = 15;
static const size_t PACKED_RECORD_BYTES  = PACKED_RECORD_FLOATS * sizeof(float);

static void packRecord(const MeasurementRecord &rec, float* buf)
{
    buf[0]  = rec.heading;
    buf[1]  = rec.inclination;
    buf[2]  = rec.roll;
    buf[3]  = rec.distance;
    buf[4]  = rec.mag(0);   buf[5]  = rec.mag(1);   buf[6]  = rec.mag(2);
    buf[7]  = rec.acc(0);   buf[8]  = rec.acc(1);   buf[9]  = rec.acc(2);
    buf[10] = rec.direction(0); buf[11] = rec.direction(1); buf[12] = rec.direction(2);
    // Store int/uint32 as float for uniform packing
    buf[13] = static_cast<float>(rec.ID);
    buf[14] = static_cast<float>(rec.timestamp);
}

static void unpackRecord(const float* buf, MeasurementRecord &rec)
{
    rec.heading     = buf[0];
    rec.inclination = buf[1];
    rec.roll        = buf[2];
    rec.distance    = buf[3];
    rec.mag  << buf[4],  buf[5],  buf[6];
    rec.acc  << buf[7],  buf[8],  buf[9];
    rec.direction << buf[10], buf[11], buf[12];
    rec.ID        = static_cast<int>(buf[13]);
    rec.timestamp = static_cast<uint32_t>(buf[14]);
}

// ---------------------------------------------------------------------------
// File-name / counter helpers for shot storage
// ---------------------------------------------------------------------------
static unsigned int counter;

bool getFileName(unsigned int fileID, char (&fname)[FNAME_LENGTH])
{
    if (fileID > 999) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "fileID exceeds 999");
        return false;
    }
    snprintf(fname, FNAME_LENGTH, "SD%03u", fileID);
    return true;
}

bool getVarName(unsigned int counter, char (&varname)[VARNAME_LENGTH])
{
    if (counter >= 999) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "counter exceeds 998");
        return false;
    }
    snprintf(varname, VARNAME_LENGTH, "%03u", counter + 1);
    return true;
}

bool getCounter(unsigned int fileID, unsigned int &counter)
{
    char fname[FNAME_LENGTH];
    if (!getFileName(fileID, fname)) return false;
    if (FileFuncs::readFromFile(fname, "counter", counter)) return true;
    Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Counter not found...");
    return false;
}

bool setCounter(unsigned int fileID, const unsigned int &counter)
{
    char fname[FNAME_LENGTH];
    if (!getFileName(fileID, fname)) return false;
    FileFuncs::writeToFile(fname, "counter", counter);
    return true;
}

bool saveShotData(const MeasurementRecord &rec, unsigned int fileID)
{
    char fname[FNAME_LENGTH];
    char varname[VARNAME_LENGTH];
    Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "Saving shot data to file...");

    if (!getFileName(fileID, fname)) return false;

    if (!getCounter(fileID, counter)) {
        Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "File doesn't exist, creating new one...");
        counter = 0;
        if (!setCounter(fileID, counter)) return false;
    }

    if (!getVarName(counter + 1, varname)) return false;
    if (!setCounter(fileID, counter + 1))  return false;

    float packed[PACKED_RECORD_FLOATS];
    packRecord(rec, packed);
    FileFuncs::writeToFile(fname, varname, packed, (int)PACKED_RECORD_FLOATS);
    return true;
}

bool readShotData(MeasurementRecord &rec, unsigned int fileID, unsigned int shotID)
{
    char fname[FNAME_LENGTH];
    char varname[VARNAME_LENGTH];
    Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "Reading shot data from file...");
    if (!getFileName(fileID, fname))  return false;
    if (!getVarName(shotID, varname)) return false;

    float packed[PACKED_RECORD_FLOATS];
    if (!FileFuncs::readFromFile(fname, varname, packed, (int)PACKED_RECORD_FLOATS)) return false;
    unpackRecord(packed, rec);
    return true;
}

bool readShotData(MeasurementRecord &rec, unsigned int fileID)
{
    Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "Reading latest shot data from file...");
    if (!getCounter(fileID, counter)) return false;
    return readShotData(rec, fileID, counter);
}

// IMPORTANT: References are immutable and must be definied upon initialisation!
SensorHandler::SensorHandler(Accelerometer &a, Magnetometer &m, Laser &l):acc(a), mag(m), las(l)
{
    mutex = xSemaphoreCreateMutex();
}

bool SensorHandler::init()
{
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "SensorHandler initialization starting...");
    
    // Check if accelerometer and magnetometer are different objects
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR, "Accelerometer object address: 0x%08X", (uint32_t)(uintptr_t)&acc);
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR, "Magnetometer object address: 0x%08X", (uint32_t)(uintptr_t)&mag);
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR, "Laser object address: 0x%08X", (uint32_t)(uintptr_t)&las);
    
    if ((void*)&acc == (void*)&mag) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "Accelerometer and Magnetometer are the same object!");
    }

    const int MAX_INIT_RETRIES = 3;
    const int RETRY_DELAY_MS = 500;
    sensors_ready = false;

    // Accelerometer init with retries
    bool acc_ok = false;
    for (int attempt = 0; attempt < MAX_INIT_RETRIES && !acc_ok; attempt++) {
        Debug_csd::logf(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "Acc init attempt %d/%d...", attempt + 1, MAX_INIT_RETRIES);
        acc.init();
        // Verify by attempting a measurement
        Vector3f test = acc.getMeasurement();
        if (test.norm() > 0.01f) {
            acc_ok = true;
        } else {
            Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Accelerometer init returned zero data, retrying...");
            vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
        }
    }
    if (!acc_ok) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "Accelerometer failed to initialise after retries");
    }

    // Magnetometer init with retries
    bool mag_ok = false;
    for (int attempt = 0; attempt < MAX_INIT_RETRIES && !mag_ok; attempt++) {
        Debug_csd::logf(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "Mag init attempt %d/%d...", attempt + 1, MAX_INIT_RETRIES);
        mag.init();
        Vector3f test = mag.getMeasurement();
        if (test.norm() > 0.01f) {
            mag_ok = true;
        } else {
            Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Magnetometer init returned zero data, retrying...");
            vTaskDelay(pdMS_TO_TICKS(RETRY_DELAY_MS));
        }
    }
    if (!mag_ok) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "Magnetometer failed to initialise after retries");
    }

    // Laser init (single attempt — laser may be optional)
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Laser init...");
    las.init();

    sensors_ready = acc_ok && mag_ok;

    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Loading calibration...");
    resetCalibration();
    loadCalibration();

    if (!sensors_ready) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "SENSOR INIT INCOMPLETE - device needs sensor check");
    }
    return sensors_ready;
}

bool SensorHandler::tryLock()
{
    return xSemaphoreTake(mutex, 0) == pdTRUE;
}

void SensorHandler::lock()
{
    xSemaphoreTake(mutex, portMAX_DELAY);
}

void SensorHandler::unlock()
{
    xSemaphoreGive(mutex);
}

void SensorHandler::resetCalibration()
{
    calib_parms.Ra_cal.setIdentity();
    calib_parms.Rm_cal.setIdentity();
    calib_parms.Ra_las.setIdentity();
    calib_parms.Rm_las.setIdentity();
    calib_parms.Rm_align.setIdentity();
    calib_parms.ba_cal.setZero();
    calib_parms.bm_cal.setZero();
    calib_parms.inclination_angle = 0;
    calib_parms.quality = {};
    static_calib_progress = 0;
    las_calib_progress = 0;
}

void SensorHandler::update()
{
    if (!tryLock()) return;  // Skip update if mutex held (e.g. during takeShot)

    mag_data << 0, 0, 0;
    acc_data << 0, 0, 0;
    for (int i = 0; i < N_UPDATE_SAMPLES; i++)
    {
        Vector3f temp_mag = mag.getMeasurement();
        Vector3f temp_acc = acc.getMeasurement();

        if (i == 0) {
            Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR,
                "First sensor reading - Mag: %f %f %f, Acc: %f %f %f",
                temp_mag(0), temp_mag(1), temp_mag(2),
                temp_acc(0), temp_acc(1), temp_acc(2));
        }

        mag_data += temp_mag;
        acc_data += temp_acc;
    }
    mag_data /= N_UPDATE_SAMPLES;
    acc_data /= N_UPDATE_SAMPLES;

    // Apply calibration correction
    correctData(corrected_shot_data.mag, corrected_shot_data.acc);

    // Compute angles & direction vector
    Vector3f HIR = NumericalMethods::inertialToCardan(corrected_shot_data.mag, corrected_shot_data.acc);
    corrected_shot_data.heading     = RAD_TO_DEG * HIR(0);
    corrected_shot_data.inclination = RAD_TO_DEG * HIR(1);
    corrected_shot_data.roll        = RAD_TO_DEG * HIR(2);
    if (corrected_shot_data.heading < 0) corrected_shot_data.heading += 360.0f;
    corrected_shot_data.direction   = NumericalMethods::inertialToVector(corrected_shot_data.mag, corrected_shot_data.acc);

    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR,
        "Raw acc data: X %f   Y %f   Z %f   Norm: %f",
        acc_data(0), acc_data(1), acc_data(2), acc_data.norm());
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR,
        "Raw mag data: X %f   Y %f   Z %f   Norm: %f",
        mag_data(0), mag_data(1), mag_data(2), mag_data.norm());
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR,
        "Corrected acc: X %f   Y %f   Z %f   Norm: %f",
        corrected_shot_data.acc(0), corrected_shot_data.acc(1), corrected_shot_data.acc(2),
        corrected_shot_data.acc.norm());
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR,
        "Corrected mag: X %f   Y %f   Z %f   Norm: %f",
        corrected_shot_data.mag(0), corrected_shot_data.mag(1), corrected_shot_data.mag(2),
        corrected_shot_data.mag.norm());
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_SENSOR,
        "HIR data: H %f   I %f   R %f",
        corrected_shot_data.heading, corrected_shot_data.inclination, corrected_shot_data.roll);

    unlock();
}

MeasurementRecord SensorHandler::getShotData(bool corrected)
{
    if (corrected)
    {
        return corrected_shot_data;
    } else {
        return shot_data;
    }
}

int SensorHandler::takeShot(bool laser_reading, bool use_stabilisation)
{
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Taking measurement...");
    int stabilisation_iters = 0;
    if (use_stabilisation)
    {
        // Wait until both accelerometer and magnetometer readings are steady.
        // Uses ring buffers of norms to detect stability via standard deviation.
        Vector<float,N_STABILISATION> acc_norm_buffer;
        Vector<float,N_STABILISATION> mag_norm_buffer;
        for (int i=0; i<N_STABILISATION; i++)
        {
            acc_norm_buffer(i) = acc.getMeasurement().norm();
            mag_norm_buffer(i) = mag.getMeasurement().norm();
        }

        int i = 0;
        while (NumericalMethods::stDev(acc_norm_buffer) > STDEV_LIMIT ||
               NumericalMethods::stDev(mag_norm_buffer) > STDEV_LIMIT * MAG_STDEV_FACTOR)
        {
            acc_norm_buffer(i%N_STABILISATION) = acc.getMeasurement().norm();
            mag_norm_buffer(i%N_STABILISATION) = mag.getMeasurement().norm();
            i++;

            if (i > MAX_STABILISATION_ITERS) {
                Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR,
                    "Shot failed: device not stable after 1000 iterations");
                return 1;
            }
        }
        stabilisation_iters = i;
    }

    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Stabilised (%d iterations).", stabilisation_iters);

    // Collect all N samples first, then reject outliers based on norm
    float mag_norms[N_SHOT_SAMPLES];
    float acc_norms[N_SHOT_SAMPLES];
    Vector3f mag_samples[N_SHOT_SAMPLES];
    Vector3f acc_samples[N_SHOT_SAMPLES];

    for (int i=0; i<N_SHOT_SAMPLES; i++)
    {
        mag_samples[i] = mag.getMeasurement();
        acc_samples[i] = acc.getMeasurement();
        mag_norms[i] = mag_samples[i].norm();
        acc_norms[i] = acc_samples[i].norm();
    }

    // Compute initial mean and stddev of norms for outlier detection
    float mag_norm_mean = 0, acc_norm_mean = 0;
    for (int i = 0; i < N_SHOT_SAMPLES; i++) {
        mag_norm_mean += mag_norms[i];
        acc_norm_mean += acc_norms[i];
    }
    mag_norm_mean /= N_SHOT_SAMPLES;
    acc_norm_mean /= N_SHOT_SAMPLES;

    float mag_norm_var = 0, acc_norm_var = 0;
    for (int i = 0; i < N_SHOT_SAMPLES; i++) {
        float md = mag_norms[i] - mag_norm_mean;
        float ad = acc_norms[i] - acc_norm_mean;
        mag_norm_var += md * md;
        acc_norm_var += ad * ad;
    }
    float mag_norm_sd = sqrtf(mag_norm_var / N_SHOT_SAMPLES);
    float acc_norm_sd = sqrtf(acc_norm_var / N_SHOT_SAMPLES);

    // Average only samples within 2 sigma of the norm mean
    const float OUTLIER_SIGMA = 2.0f;
    mag_data.setZero();
    acc_data.setZero();
    int mag_count = 0, acc_count = 0;
    for (int i = 0; i < N_SHOT_SAMPLES; i++) {
        if (fabsf(mag_norms[i] - mag_norm_mean) <= OUTLIER_SIGMA * mag_norm_sd) {
            mag_data += mag_samples[i];
            mag_count++;
        }
        if (fabsf(acc_norms[i] - acc_norm_mean) <= OUTLIER_SIGMA * acc_norm_sd) {
            acc_data += acc_samples[i];
            acc_count++;
        }
    }
    // Fallback: if too many rejected, use all samples
    bool low_quality = false;
    if (mag_count < N_SHOT_SAMPLES / 2) {
        mag_data.setZero();
        for (int i = 0; i < N_SHOT_SAMPLES; i++) mag_data += mag_samples[i];
        mag_count = N_SHOT_SAMPLES;
        low_quality = true;
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Mag outlier rejection discarded >50%, using all samples");
    }
    if (acc_count < N_SHOT_SAMPLES / 2) {
        acc_data.setZero();
        for (int i = 0; i < N_SHOT_SAMPLES; i++) acc_data += acc_samples[i];
        acc_count = N_SHOT_SAMPLES;
        low_quality = true;
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Acc outlier rejection discarded >50%, using all samples");
    }
    mag_data /= mag_count;
    acc_data /= acc_count;
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Data collected (%d/%d mag, %d/%d acc samples after outlier rejection).",
                     mag_count, N_SHOT_SAMPLES, acc_count, N_SHOT_SAMPLES);

    
    if (laser_reading) {
        las_data = las.getMeasurement();
        if (las_data <= 0) { return 1; }
    }

    // Update raw shot data
    shot_data.mag = mag_data;
    shot_data.acc = acc_data;
    shot_data.distance = las_data;
    shot_data.timestamp = (uint32_t)millis();
    Vector3f raw_HIR = NumericalMethods::inertialToCardan(shot_data.mag, shot_data.acc);
    shot_data.heading     = RAD_TO_DEG * raw_HIR(0);
    shot_data.inclination = RAD_TO_DEG * raw_HIR(1);
    shot_data.roll        = RAD_TO_DEG * raw_HIR(2);
    if (shot_data.heading < 0) shot_data.heading += 360.0f;
    shot_data.direction   = NumericalMethods::inertialToVector(shot_data.mag, shot_data.acc);

    // Save corrected shot data
    correctData(corrected_shot_data.mag, corrected_shot_data.acc);
    Vector3f cor_HIR = NumericalMethods::inertialToCardan(corrected_shot_data.mag, corrected_shot_data.acc);
    corrected_shot_data.heading     = RAD_TO_DEG * cor_HIR(0);
    corrected_shot_data.inclination = RAD_TO_DEG * cor_HIR(1);
    corrected_shot_data.roll        = RAD_TO_DEG * cor_HIR(2);
    if (corrected_shot_data.heading < 0) corrected_shot_data.heading += 360.0f;
    corrected_shot_data.direction   = NumericalMethods::inertialToVector(corrected_shot_data.mag, corrected_shot_data.acc);
    corrected_shot_data.distance    = las_data + DEVICE_LENGTH;
    corrected_shot_data.timestamp   = (uint32_t)millis();

    Debug_csd::logf(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_SENSOR, "Laser measurement: %f", las_data);
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR,
        "Measurement complete: H=%.1f deg I=%.1f deg R=%.1f deg D=%.3fm",
        corrected_shot_data.heading, corrected_shot_data.inclination,
        corrected_shot_data.roll, corrected_shot_data.distance);

    las.toggleLaser(true);

    return low_quality ? 2 : 0;
}

void SensorHandler::correctData(Vector3f &m, Vector3f &g)
{
    // Apply sensor calibration
    m = calib_parms.Rm_cal * (mag_data - calib_parms.bm_cal);
    g = calib_parms.Ra_cal * (acc_data - calib_parms.ba_cal);

    // Normalise data
    m.normalize();
    g.normalize();

    // Apply alignment
    m = calib_parms.Rm_align * calib_parms.Rm_las * m; 
    g = calib_parms.Ra_las * g;

    // Apply final normalisation
    m.normalize();
    g.normalize();
}

void SensorHandler::eraseFlash()
{
    FileFuncs::erase_flash();
}
void SensorHandler::getFlashStats()
{
    FileFuncs::getStatus();
}

int SensorHandler::getShotCount(unsigned int fileID)
{
    unsigned int count = 0;
    if (!getCounter(fileID, count)) return 0;
    return (int)count;
}

bool SensorHandler::readShotByIndex(MeasurementRecord &rec, unsigned int fileID, unsigned int index)
{
    return readShotData(rec, fileID, index);
}

int SensorHandler::collectStaticCalibData()
{
    if (static_calib_progress >= N_ORIENTATIONS) { return N_ORIENTATIONS; }

    int index = 0;
    int n_avg = 5;
    Vector3f g, m;
    for (int i=0; i<N_SAMPLES_PER_ORIENTATION;i++)
    {
        index = static_calib_progress*N_SAMPLES_PER_ORIENTATION+i;

        g.setZero();
        m.setZero();

        for (int j=0; j<n_avg;j++)
        {
            g += acc.getMeasurement();
            m += mag.getMeasurement();
            // Serial.println(j);
        }
        static_calib_data.acc_data.col(index) = g/n_avg;
        static_calib_data.mag_data.col(index) = m/n_avg;
    }

    static_calib_progress++;
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Static calibration progress %i/%i", static_calib_progress, N_ORIENTATIONS);
    return static_calib_progress;

}
int SensorHandler::collectLaserCalibData()
{
    if (las_calib_progress >= N_LASER_CAL) { return N_LASER_CAL; }

    if (takeShot()) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Shot failed! Try again.");
        return las_calib_progress;
    }

    laser_calib_data.acc_data.col(las_calib_progress) = acc_data;
    laser_calib_data.mag_data.col(las_calib_progress) = mag_data;
    
    las_calib_progress++;
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Laser calibration progress %i/%i", las_calib_progress, N_LASER_CAL);
    return las_calib_progress;
}

int SensorHandler::calibrate()
{
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Starting calibration...");
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "calibrate start - Free heap: %u, Largest block: %u",
                     ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    NumericalMethods::calibrateEllipsoid(static_calib_data.mag_data, calib_parms.Rm_cal, calib_parms.bm_cal);
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Magnetometer ellipsoid fit complete.");
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "After mag calibrateEllipsoid - Free heap: %u, Largest block: %u",
                     ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    NumericalMethods::calibrateEllipsoid(static_calib_data.acc_data, calib_parms.Ra_cal, calib_parms.ba_cal);
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Accelerometer ellipsoid fit complete.");
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "calibrate complete - Free heap: %u, Largest block: %u",
                     ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    return 0;
}
int SensorHandler::align()
{
    // ----- Step 1: Apply calibration to laser data -----
    Matrix<float, 3, N_LASER_CAL> cal_laser_acc =
        calib_parms.Ra_cal * (laser_calib_data.acc_data.colwise() - calib_parms.ba_cal);
    Matrix<float, 3, N_LASER_CAL> cal_laser_mag =
        calib_parms.Rm_cal * (laser_calib_data.mag_data.colwise() - calib_parms.bm_cal);

    // Keep pre-normalisation copies for quality assessment
    Matrix<float, 3, N_LASER_CAL> cal_laser_mag_unnorm = cal_laser_mag;
    Matrix<float, 3, N_LASER_CAL> cal_laser_acc_unnorm = cal_laser_acc;

    cal_laser_acc.colwise().normalize();
    cal_laser_mag.colwise().normalize();

    // ----- Step 2: Unified laser alignment -----
    NumericalMethods::alignLaser(cal_laser_acc, cal_laser_mag,
                                 calib_parms.Ra_las, calib_parms.Rm_las);
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Sensor-laser alignment complete.");

    // ----- Step 3: Apply calibration to static data -----
    Matrix<float, 3, N_ALIGN_MAG_ACC> cal_static_acc =
        calib_parms.Ra_cal * (static_calib_data.acc_data.colwise() - calib_parms.ba_cal);
    Matrix<float, 3, N_ALIGN_MAG_ACC> cal_static_mag =
        calib_parms.Rm_cal * (static_calib_data.mag_data.colwise() - calib_parms.bm_cal);

    // Pre-normalisation copies for quality assessment
    Matrix<float, 3, N_ALIGN_MAG_ACC> cal_static_mag_unnorm = cal_static_mag;
    Matrix<float, 3, N_ALIGN_MAG_ACC> cal_static_acc_unnorm = cal_static_acc;

    cal_static_acc.colwise().normalize();
    cal_static_mag.colwise().normalize();

    // ----- Step 4: Apply laser alignment to static data -----
    Matrix<float, 3, N_ALIGN_MAG_ACC> aligned_static_acc = calib_parms.Ra_las * cal_static_acc;
    Matrix<float, 3, N_ALIGN_MAG_ACC> aligned_static_mag = calib_parms.Rm_las * cal_static_mag;

    // ----- Step 5: CWB mag-acc alignment -----
    float inclination_variance = 0;
    NumericalMethods::alignMagAcc_CWB(
        aligned_static_acc, aligned_static_mag,
        calib_parms.Rm_align, calib_parms.inclination_angle,
        &inclination_variance);
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Magnetometer-accelerometer alignment complete.");

    // ----- Step 6: Quality assessment -----
    calib_parms.quality = NumericalMethods::computeCalibrationQuality(
        cal_static_mag_unnorm, cal_static_acc_unnorm,
        cal_laser_mag_unnorm, cal_laser_acc_unnorm,
        inclination_variance);

    // ----- Structured quality summary -----
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR,
        "Inclination angle: %.4f", calib_parms.inclination_angle);
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR,
        "\n      Calibration Quality Grade: %s"
        "\n        Inclination sigma:       %.2f deg"
        "\n        Mag fit residual:    %.6f"
        "\n        Acc fit residual:    %.6f"
        "\n        Laser plane spread:  %.2f deg",
        NumericalMethods::gradeToString(calib_parms.quality.grade),
        calib_parms.quality.inclination_sigma_deg,
        calib_parms.quality.mag_fit_residual,
        calib_parms.quality.acc_fit_residual,
        calib_parms.quality.laser_plane_spread_deg);

    return 0;
}

void SensorHandler::validateCalibrationQuality()
{
    // Re-run quality assessment from stored raw data + current calib params.
    // Useful after loadRawCalibrationData() + calibrate() + align().
    // Quality is already computed at end of align(), so this is a no-op
    // unless called independently.
    Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Quality grade: %s",
                     NumericalMethods::gradeToString(calib_parms.quality.grade));
}

void SensorHandler::saveCalibration()
{
    // Raw calibration data → "calib_raw"
    EigenFileFuncs::writeToFile("calib_raw", "s_acc", static_calib_data.acc_data);
    EigenFileFuncs::writeToFile("calib_raw", "s_mag", static_calib_data.mag_data);
    EigenFileFuncs::writeToFile("calib_raw", "l_acc", laser_calib_data.acc_data);
    EigenFileFuncs::writeToFile("calib_raw", "l_mag", laser_calib_data.mag_data);

    // Computed parameters → "calib_res"
    EigenFileFuncs::writeToFile("calib_res", "Ra_cal",   calib_parms.Ra_cal);
    EigenFileFuncs::writeToFile("calib_res", "ba_cal",   calib_parms.ba_cal);
    EigenFileFuncs::writeToFile("calib_res", "Rm_cal",   calib_parms.Rm_cal);
    EigenFileFuncs::writeToFile("calib_res", "bm_cal",   calib_parms.bm_cal);
    EigenFileFuncs::writeToFile("calib_res", "Ra_las",   calib_parms.Ra_las);
    EigenFileFuncs::writeToFile("calib_res", "Rm_las",   calib_parms.Rm_las);
    EigenFileFuncs::writeToFile("calib_res", "Rm_align", calib_parms.Rm_align);
    FileFuncs::writeToFile("calib_res", "inc_angle", calib_parms.inclination_angle);

    // Quality metrics → "calib_res"
    FileFuncs::writeToFile("calib_res", "inc_sig",  calib_parms.quality.inclination_sigma_deg);
    FileFuncs::writeToFile("calib_res", "mag_res",  calib_parms.quality.mag_fit_residual);
    FileFuncs::writeToFile("calib_res", "acc_res",  calib_parms.quality.acc_fit_residual);
    FileFuncs::writeToFile("calib_res", "las_spr",  calib_parms.quality.laser_plane_spread_deg);
    FileFuncs::writeToFile("calib_res", "grade",    static_cast<int>(calib_parms.quality.grade));
}
void SensorHandler::loadCalibration()
{
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Loading calibration data from NVS...");
    Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "loadCalibration start - Free heap: %u, Largest block: %u",
                     ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    // Reset all parameters to identity/zero before loading to prevent
    // stale partial state if some NVS keys are missing.
    resetCalibration();

    bool ok = true;

    // Raw data → "calib_raw"
    if (!EigenFileFuncs::readFromFile("calib_raw", "s_acc", static_calib_data.acc_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Static acc calibration data not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_raw", "s_mag", static_calib_data.mag_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Static mag calibration data not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_raw", "l_acc", laser_calib_data.acc_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Laser acc calibration data not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_raw", "l_mag", laser_calib_data.mag_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Laser mag calibration data not found"); ok = false;
    }

    // Computed parameters → "calib_res"
    if (!EigenFileFuncs::readFromFile("calib_res", "Ra_cal", calib_parms.Ra_cal)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Ra_cal not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_res", "ba_cal", calib_parms.ba_cal)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "ba_cal not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_res", "Rm_cal", calib_parms.Rm_cal)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Rm_cal not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_res", "bm_cal", calib_parms.bm_cal)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "bm_cal not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_res", "Ra_las", calib_parms.Ra_las)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Ra_las not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_res", "Rm_las", calib_parms.Rm_las)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Rm_las not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_res", "Rm_align", calib_parms.Rm_align)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Rm_align not found"); ok = false;
    }
    if (!FileFuncs::readFromFile("calib_res", "inc_angle", calib_parms.inclination_angle)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Inclination angle not found"); ok = false;
    }

    // Quality metrics (non-fatal if missing)
    FileFuncs::readFromFile("calib_res", "inc_sig", calib_parms.quality.inclination_sigma_deg);
    FileFuncs::readFromFile("calib_res", "mag_res", calib_parms.quality.mag_fit_residual);
    FileFuncs::readFromFile("calib_res", "acc_res", calib_parms.quality.acc_fit_residual);
    FileFuncs::readFromFile("calib_res", "las_spr", calib_parms.quality.laser_plane_spread_deg);
    int grade_int = 0;
    if (FileFuncs::readFromFile("calib_res", "grade", grade_int)) {
        calib_parms.quality.grade = static_cast<NumericalMethods::Grade>(grade_int);
    }

    if (ok) {
        // Validate loaded calibration data
        auto isValidRotation = [](const Matrix3f &R, const char* name) -> bool {
            float det = R.determinant();
            if (!std::isfinite(det) || fabsf(det - 1.0f) > 0.1f) {
                Debug_csd::logf(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR,
                    "Invalid rotation matrix %s (det=%.4f)", name, det);
                return false;
            }
            return true;
        };
        auto isFiniteVec = [](const Vector3f &v, const char* name) -> bool {
            if (!v.allFinite()) {
                Debug_csd::logf(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR,
                    "Non-finite bias vector %s", name);
                return false;
            }
            return true;
        };

        bool valid = true;
        valid &= isValidRotation(calib_parms.Ra_cal, "Ra_cal");
        valid &= isValidRotation(calib_parms.Rm_cal, "Rm_cal");
        valid &= isValidRotation(calib_parms.Ra_las, "Ra_las");
        valid &= isValidRotation(calib_parms.Rm_las, "Rm_las");
        valid &= isValidRotation(calib_parms.Rm_align, "Rm_align");
        valid &= isFiniteVec(calib_parms.ba_cal, "ba_cal");
        valid &= isFiniteVec(calib_parms.bm_cal, "bm_cal");

        if (valid) {
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "All calibration data loaded and validated");
        } else {
            Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR,
                "Calibration data invalid - resetting to defaults");
            resetCalibration();
        }
    } else {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR,
            "Some calibration data missing - device needs calibration before accurate measurements");
    }
}

void SensorHandler::removePrevCalib(bool static_calib)
{
    if (static_calib) {
        if (static_calib_progress <= 0) return;
        static_calib_progress--;
        int start = static_calib_progress * N_SAMPLES_PER_ORIENTATION;
        static_calib_data.acc_data.block<3, N_SAMPLES_PER_ORIENTATION>(0, start).setZero();
        static_calib_data.mag_data.block<3, N_SAMPLES_PER_ORIENTATION>(0, start).setZero();
        Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Undid static calibration sample, progress now %i/%i",
                         static_calib_progress, N_ORIENTATIONS);
    } else {
        if (las_calib_progress <= 0) return;
        las_calib_progress--;
        laser_calib_data.acc_data.col(las_calib_progress).setZero();
        laser_calib_data.mag_data.col(las_calib_progress).setZero();
        Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Undid laser calibration sample, progress now %i/%i",
                         las_calib_progress, N_LASER_CAL);
    }
}

int SensorHandler::getCalibProgress()
{
    return static_calib_progress + las_calib_progress;
}

int SensorHandler::getCalibProgress(bool static_calib)
{
    if (static_calib)
    {
        return static_calib_progress;
    } else {
        return las_calib_progress;
    }
    
}

Vector3f SensorHandler::getMagData()
{
    return mag_data;
}
Vector3f SensorHandler::getAccData()
{
    return acc_data;
}
float SensorHandler::getLasData()
{
    return las_data;
}

const StaticCalibrationData& SensorHandler::getStaticCalibData()
{
    return static_calib_data;
}
const LaserCalibrationData& SensorHandler::getLaserCalibData()
{
    return laser_calib_data;
}
const DeviceCalibrationParameters& SensorHandler::getCalibParms()
{
    return calib_parms;
}

void SensorHandler::dumpCalibToSerial()
{
    // Incremental streaming — no large static buffer needed
    // Helper lambdas to print Eigen objects as JSON arrays
    auto printVec3 = [](const char* label, const Vector3f& v) {
        Serial.printf("\"%s\":[%.6f,%.6f,%.6f]", label, v(0), v(1), v(2));
    };
    auto printMat3 = [](const char* label, const Matrix3f& m) {
        Serial.printf("\"%s\":[[%.6f,%.6f,%.6f],[%.6f,%.6f,%.6f],[%.6f,%.6f,%.6f]]",
            label,
            m(0,0), m(0,1), m(0,2),
            m(1,0), m(1,1), m(1,2),
            m(2,0), m(2,1), m(2,2));
    };
    auto printMatrixXf = [](const char* label, const Ref<const MatrixXf>& mat) {
        Serial.printf("\"%s\":[", label);
        for (int r = 0; r < mat.rows(); r++) {
            Serial.print("[");
            for (int c = 0; c < mat.cols(); c++) {
                Serial.printf("%.6f", mat(r, c));
                if (c < mat.cols() - 1) Serial.print(",");
            }
            Serial.print("]");
            if (r < mat.rows() - 1) Serial.print(",");
        }
        Serial.print("]");
    };

    Serial.println("{");

    // Raw sample data
    printMatrixXf("static_acc_samples", static_calib_data.acc_data);
    Serial.println(",");
    printMatrixXf("static_mag_samples", static_calib_data.mag_data);
    Serial.println(",");
    printMatrixXf("laser_acc_samples",  laser_calib_data.acc_data);
    Serial.println(",");
    printMatrixXf("laser_mag_samples",  laser_calib_data.mag_data);
    Serial.println(",");

    // Calibration parameters
    printMat3("Ra_static", calib_parms.Ra_cal);  Serial.println(",");
    printVec3("ba_static", calib_parms.ba_cal);  Serial.println(",");
    printMat3("Rm_static", calib_parms.Rm_cal);  Serial.println(",");
    printVec3("bm_static", calib_parms.bm_cal);  Serial.println(",");
    printMat3("Ra_laser",  calib_parms.Ra_las);  Serial.println(",");
    printMat3("Rm_laser",  calib_parms.Rm_las);  Serial.println(",");
    printMat3("Rm_align",  calib_parms.Rm_align); Serial.println(",");

    Serial.printf("\"inclination_angle\":%.6f,\n", calib_parms.inclination_angle);

    // Quality metrics
    Serial.printf("\"quality\":{\"inclination_sigma_deg\":%.6f,\"mag_fit_residual\":%.6f,"
                  "\"acc_fit_residual\":%.6f,\"laser_plane_spread_deg\":%.6f,\"grade\":%d}\n",
        calib_parms.quality.inclination_sigma_deg,
        calib_parms.quality.mag_fit_residual,
        calib_parms.quality.acc_fit_residual,
        calib_parms.quality.laser_plane_spread_deg,
        static_cast<int>(calib_parms.quality.grade));

    Serial.println("}");
}

void SensorHandler::loadRawCalibrationData()
{
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Loading raw calibration data from NVS...");

    bool ok = true;
    if (!EigenFileFuncs::readFromFile("calib_raw", "s_acc", static_calib_data.acc_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Static acc data not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_raw", "s_mag", static_calib_data.mag_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Static mag data not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_raw", "l_acc", laser_calib_data.acc_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Laser acc data not found"); ok = false;
    }
    if (!EigenFileFuncs::readFromFile("calib_raw", "l_mag", laser_calib_data.mag_data)) {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Laser mag data not found"); ok = false;
    }

    if (ok) {
        Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_SENSOR, "Raw calibration data loaded successfully");
    } else {
        Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Some raw calibration data missing - FORCE_CAL may fail");
    }
}

