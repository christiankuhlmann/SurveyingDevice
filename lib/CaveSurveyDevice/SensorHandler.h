#ifndef CAVESURVEYDEVICE_SENSORHANDLER_H
#define CAVESURVEYDEVICE_SENSORHANDLER_H

#include <ArduinoEigen.h>
#include <NumericalMethods.h>
#include "Sensors.h"

using namespace Eigen;

// Define constants for general use

const int N_MAG_CAL_HEADING = 25; // Size of magnetometer calibration matrix
const int N_MAG_CAL_INCLINATION = 15; // Size of magnetometer calibration matrix


struct DeviceCalibrationParameters
{
    Matrix3f Ra, Rm , Ralign;
    Vector3f ba, bm;
    float laser_inclination, laser_heading;
};


class SensorHandler
{
private:
    const static int N_SHOT_SMAPLES = 25;
    const static float STDEV_LIMIT = 0.1;
    const static int N_STABILISATION = 5;  
    static bool MAG_COMBINED_CAL; // Calibrate magnetometer separately to alignment

    // Sensor objects
    Accelerometer acc;
    Magnetometer mag;
    Laser las;

    // Calibration and alignment data
    Matrix<float,N_MAG_CAL_HEADING,N_MAG_CAL_INCLINATION,3> mag_calib_data;
    Matrix<float,N_MAG_CAL_HEADING,N_MAG_CAL_INCLINATION> mag_calib_data_filled_indices;

    Matrix<float,3,N_ALIGN_MAG_ACC> mag_align_data;
    Matrix<float,3,N_ALIGN_MAG_ACC> acc_align_data;
    Matrix<float,3,N_LASER_CAL> laser_align_data;

    // Calibration parameters
    DeviceCalibrationParameters calib_parms;

    // Data collected from sensors
    Vector3f acc_data, mag_data, las_data;
    Vector4f shot_data; // HIRD

    Vector2i getMagCalIndex(const Vector3f &m);
    int nFilledMagCalibIndices();

public:
    Vector3f getAccData();
    Vector3f getMagData();
    Vector3f getLasData();

    Vector3f getCardan();
    Vector3f getCartesian();

    // Returns Heading, Inclination, Distance
    int takeShot();

    /**
     * @brief Collects a sample for the magnetometer's calibration.
     * Returns the number of separate segments with calibrated values
     * 
     * @return int 
     */
    int collectMagCalibData();

    /**
     * @brief Collects a sample of alignment data for joint accelerometer and magnetometer alignment.
     * Returns the current progress out of N_MAG_ACC_ALIGN, -1 if complete.
     * 
     * @return int 
     */
    int collectMagAccAlignData();
    int collectLaserAlignData();

    DeviceCalibrationParameters getCalibParms();
    void setCalibParms(const DeviceCalibrationParameters &parms);
};
#endif