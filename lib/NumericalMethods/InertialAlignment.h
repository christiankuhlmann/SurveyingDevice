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
 * @param g_in 
 * @param m_in 
 * @return Vector<float,10> 
 */
Vector<float,10> alignMagAcc(const Matrix<float,3,N_ALIGN_MAG_ACC> &g_in, const Matrix<float,3,N_ALIGN_MAG_ACC> &m_in);

void alignMagAcc(const Matrix<float,3,N_ALIGN_MAG_ACC> &g_in, const Matrix<float,3,N_ALIGN_MAG_ACC> &m_in, 
                            Matrix3f &R_align, float &inclination_angle);

}
#endif