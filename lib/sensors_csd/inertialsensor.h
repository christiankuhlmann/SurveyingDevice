#ifndef HEADER_INERTIALSENSOR
#define HEADER_INERTIALSENSOR
#include <ArduinoEigenDense.h>
#include "sensor_config_csd.h"

using namespace Eigen;

class InertialSensorConnection
{
public:
    virtual Vector3f getRawData();
};

class InertialSensor
{
protected:
    Matrix3f calibration_matrix;
    Vector3f calibration_offset;
    Vector3f calibrated_data;
    Vector3f raw_data;

    InertialSensorConnection* sensor;

    bool calibrate_with_alignment;
    int align_num;

    Eigen::Map<Matrix<float,3,-1>> ref_calibration_data;
    // Ref<MatrixXf> ref_calibration_data;
    //MatrixXf ref_calibration_data;
    bool separate_calib;

public:

    bool getCalibMode();
    void setCalibMode(bool mode);

    InertialSensor(InertialSensorConnection* sc, float* ptr, int size);

    /**
     * @brief Linear calibrataes an inertial sensor via ellipsoid fitting.
     * 1. Sorts all zero-valued columns of the calibration data to the end of the matrix
     * 2. Performs ellipsoid fit with first n elements of sorted matrix where n is the numer of non-zero values
     * 
     */
    void calibrateLinear();  

    Vector3f getSingleSample();

    virtual Vector3f getReading();

    void resetCalibration();

    bool collectAlignmentSample();

    Matrix3f getT();
    Vector3f geth();

    virtual MatrixXf getCalibData()=0;
};

#endif