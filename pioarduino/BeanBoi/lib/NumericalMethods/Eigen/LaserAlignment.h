#ifndef NUMERICAL_METHODS_LASER_ALIGNMENT_H
#define NUMERICAL_METHODS_LASER_ALIGNMENT_H

#include "utils.h"
namespace NumericalMethods{
    

/**
 * @brief Calculates the normal vector characterising the best-fit plane fit to a point-cloud
 * 
 * @param point_cloud 3xN matrix of input data
 * @return Vector3f Normal vector characterising best-fit plane
 */
Vector3f normalVec(const Ref<const Matrix3Xf> &point_cloud);


/**
 * @brief Aligns a laser with a set of magnetometer and accelerometer readings
 * 
 * @param g gravitation data
 * @param m magnetic data
 * @param Racc Accelerometer alignment matrix
 * @param Rmag Magnetometer alignment matrix
 */
void alignLaser(const Ref<const Matrix3Xf> &g, const Ref<const Matrix3Xf> &m, Matrix3f &Racc, Matrix3f &Rmag);

/**
 * @brief Aligns the normal vector of a point cloud to the principal axis (x-axis) of the device
 * 
 * @param point_cloud Inertial data
 * @param R Correction matrix
 */
void alignToNorm(const Ref<const Matrix3Xf> &point_cloud, Matrix3f &R);


}

#endif