// #include "SCA3300SensorConnection.h"

// SCA3300SensorConnection::SCA3300SensorConnection(SCA3300& sca3300):sca3300_connection(sca3300){}

// void SCA3300SensorConnection::init()
// {
//     if (sca3300_connection.begin() != true) {Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Accelerometer initialisation failed!"); }
//     else { Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Accelerometer initialisation succesful!"); }
// }

// Vector3f SCA3300SensorConnection::getMeasurement()
// {
//     Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Getting measurement...");
//     Eigen::Vector3f data;
//     bool available = false;

//     while (!available)
//     {
//         available = sca3300_connection.available();
//         if (!available) {
//             sca3300_connection.reset();
//         }
//     }

//     Debug_csd::debug(Debug_csd::DEBUG_ACCEL,"Requesting data...");
//     data <<
//     (float)sca3300_connection.getCalculatedAccelerometerX(),
//     -(float)sca3300_connection.getCalculatedAccelerometerY(),
//     -(float)sca3300_connection.getCalculatedAccelerometerZ();


//     return data;
// }
