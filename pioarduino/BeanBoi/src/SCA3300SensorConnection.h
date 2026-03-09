#ifndef HEADER_SCA3300SC
#define HEADER_SCA3300SC

#include <CaveSurveyDevice.h>
#include <SCA3300.h>
#include <debug_csd.h>

class SCA3300SensorConnection: public Accelerometer
{
public:
    SCA3300SensorConnection(SCA3300 &sca3300);
    Vector3f getMeasurement();
    void init();
    bool hasError() const { return _error; }
private:
    SCA3300& sca3300_connection;
    bool _error = false;
};

inline SCA3300SensorConnection::SCA3300SensorConnection(SCA3300& sca3300):sca3300_connection(sca3300){}

inline void SCA3300SensorConnection::init()
{
    if (sca3300_connection.begin() != true) {Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Accelerometer initialisation failed!"); }
    else { Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Accelerometer initialisation succesful!"); }
}

inline Vector3f SCA3300SensorConnection::getMeasurement()
{
    Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Getting measurement...");
    Eigen::Vector3f data;
    bool available = false;
    int retries = 0;
    const int MAX_SCA_RETRIES = 50;

    while (!available && retries < MAX_SCA_RETRIES)
    {
        available = sca3300_connection.available();
        if (!available) {
            sca3300_connection.reset();
            retries++;
        }
    }

    if (!available) {
        Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"ERROR: SCA3300 not responding after retries");
        _error = true;
        data << 0, 0, 0;
        return data;
    }
    _error = false;

    Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Requesting data...");
    data <<
    (float)sca3300_connection.getCalculatedAccelerometerX(),
    -(float)sca3300_connection.getCalculatedAccelerometerY(),
    -(float)sca3300_connection.getCalculatedAccelerometerZ();


    return data;
}

#endif