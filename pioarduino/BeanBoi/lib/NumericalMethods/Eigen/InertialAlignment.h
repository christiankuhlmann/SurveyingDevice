#ifndef NUMERICAL_METHODS_INERTIAL_ALIGNMENT_H
#define NUMERICAL_METHODS_INERTIAL_ALIGNMENT_H

#include "utils.h"
namespace NumericalMethods{
    
/**
 * @brief Given a set of calibrated magnetometer and accelerometer data, this function
     * finds the least squares best fit for the alignment of the sensor axis and outputs
     * a rotation matrix for correcting the magnetometer and the magnetic inclination at
     * the location of measurement.
 * 
 * @param g_in Input gravitational data
 * @param m_in Input magnetometer data
 * @return Vector<float,10> 
 */
Vector<float,10> alignMagAcc_MAGICAL(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in);

/**
 * @brief Finds the matrix required to align the magnetometer and accelerometer with the principal laser axis
 * 
 * @param g_in Input gravitational data
 * @param m_in Input magnetometer data
 * @param R_align Output correction matrix
 * @param inclination_angle Calculated inclination angle
 */
void alignMagAcc_MAGICAL(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in, 
                            Matrix3f &R_align, float &inclination_angle);


/**
 * @brief Given a set of calibrated magnetometer and accelerometer data, this function
 * finds the best fit x-rotation for the magnetometer that reduces the standard deviation of the
 * measured inclination angle of the magnetic field.
 * 
 * This is a constrained version of the Wahba problem where only rotation about the x-axis is allowed.
 * Uses golden section search to find optimal rotation angle.
 * 
 * THIS STEP MUST FOLLOW LASER CALIBRATION
 * 
 * @param g_in Input gravitational data
 * @param m_in Input magnetometer data
 * @return Vector<float,10> [R_align(0: 8), inclination_angle(9)]
 */
Vector<float,10> alignMagAcc_CWB(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in);

/**
 * @brief Given a set of calibrated magnetometer and accelerometer data, this function
 * finds the best fit x-rotation for the magnetometer that reduces the standard deviation of the
 * measured inclination angle of the magnetic field.
 * 
 * This is a constrained version of the Wahba problem where only rotation about the x-axis is allowed.
 * Uses golden section search to find optimal rotation angle. 
 * 
 * THIS STEP MUST FOLLOW LASER CALIBRATION
 * 
 * @param g_in Input gravitational data
 * @param m_in Input magnetometer data
 * @param R_align Output correction matrix (rotation about x-axis)
 * @param inclination_angle Calculated mean inclination angle in radians
 * @param inclination_variance_out Optional pointer; if non-null, receives the final inclination variance (rad²)
 */
void alignMagAcc_CWB(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in, 
                            Matrix3f &R_align, float &inclination_angle,
                            float *inclination_variance_out = nullptr);

static float computeInclination(const Vector3f& mag, const Vector3f& grav);

static float inclinationVariance(float theta, const Ref<const Matrix3Xf> &g, const Ref<const Matrix3Xf> &m);

}
#endif