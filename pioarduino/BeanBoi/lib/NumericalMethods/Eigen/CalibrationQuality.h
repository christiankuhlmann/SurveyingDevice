#ifndef NUMERICAL_METHODS_CALIBRATION_QUALITY_H
#define NUMERICAL_METHODS_CALIBRATION_QUALITY_H

#include "utils.h"

namespace NumericalMethods {

/**
 * @brief Calibration quality grades (inspired by BCRA/UIS cave survey grading)
 *
 *  Excellent ≈ BCRA Grade 5+  (compass ±1°, clino ±0.5°)
 *  VeryGood  ≈ BCRA Grade 5   (compass ±2°, clino ±1°)
 *  Good      ≈ BCRA Grade 3–4
 *  Fair      ≈ BCRA Grade 2
 *  Poor      ≈ BCRA Grade 1
 *  Unusable  — calibration did not converge or data is unreliable
 */
enum class Grade {
    Unusable  = 0,
    Poor      = 1,
    Fair      = 2,
    Good      = 3,
    VeryGood  = 4,
    Excellent = 5
};

/**
 * @brief Convert a Grade enum to a human-readable string
 */
const char* gradeToString(Grade g);

/**
 * @brief Collected calibration-quality metrics
 */
struct CalibrationQuality {
    float inclination_sigma_deg;     ///< σ of inclination angles after CWB (°)
    float mag_fit_residual;          ///< RMS (||col||-1) of calibrated mag data
    float acc_fit_residual;          ///< RMS (||col||-1) of calibrated acc data
    float laser_plane_spread_deg;    ///< Angular spread of laser cone normals (°)
    Grade grade;                     ///< Overall quality grade
};

/**
 * @brief Compute calibration quality from pipeline outputs
 *
 * @param calibrated_mag    3xN calibrated (pre-normalisation) magnetometer data
 * @param calibrated_acc    3xN calibrated (pre-normalisation) accelerometer data
 * @param laser_mag         3xM calibrated laser magnetometer cone data
 * @param laser_acc         3xM calibrated laser accelerometer cone data
 * @param cwb_inclination_variance  Variance of inclination angles at optimal θ (rad²)
 * @return CalibrationQuality  Struct with all metrics and overall grade
 */
CalibrationQuality computeCalibrationQuality(
    const Ref<const Matrix3Xf> &calibrated_mag,
    const Ref<const Matrix3Xf> &calibrated_acc,
    const Ref<const Matrix3Xf> &laser_mag,
    const Ref<const Matrix3Xf> &laser_acc,
    float cwb_inclination_variance);

}

#endif
