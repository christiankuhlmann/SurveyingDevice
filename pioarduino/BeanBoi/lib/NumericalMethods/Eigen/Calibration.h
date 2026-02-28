#ifndef NUMERICAL_METHODS_FITTINGFUNCS_H
#define NUMERICAL_METHODS_FITTINGFUNCS_H

#include "utils.h"
namespace NumericalMethods {

/**
 * @brief Finds the parameters of the ellipsoid which best fits the input data
 * 
 * @param samples 3xN matrix of input data
 * @return RowVector<float,10> Ellipsoid parameters
 */
RowVector<float,10> fitEllipsoid(const Ref<const MatrixXf> &samples);

/**
 * @brief Finds the parameters of the ellipsoid which best fits the input data
 * 
 * @param samples 3xN matrix of input data
 * @param M_out Output matrix parameter
 * @param n_out Output vector parameter
 * @param d_out Output scalar parameter
 */
void fitEllipsoid(const Ref<const MatrixXf> &samples, Matrix3f &M_out, Vector3f &n_out, float &d_out);

/**
 * @brief Given a set of unformatted ellipsoid parameters, find the 3x3 rotation matrix and 3x1 offset matrix which transforms the ellipsoid into a sphere
 * @todo Add sources for calculations
 * 
 * @param M Input matrix parameter
 * @param n Input vector parameter
 * @param d Input scalar parameter
 * @return Vector<float,12> 
 */
Vector<float,12> calculateEllipsoidTransformation(const Matrix3f &M, const Vector3f &n, const float &d);

/**
 * @brief Given a set of unformatted ellipsoid parameters, find the 3x3 rotation matrix and 3x1 offset matrix which transforms the ellipsoid into a sphere
 * @todo Add sources for calculations
 * 
 * @param U Unformatted ellipsoid parameters
 * @return Vector<float,12> Output transformation parameters
 */
 Vector<float,12> calculateEllipsoidTransformation(const RowVector<float,10> &U);

 /**
  * @brief Given a set of unformatted ellipsoid parameters, find the 3x3 rotation matrix and 3x1 offset matrix which transforms the ellipsoid into a sphere
  * 
  * @param U 
  * @param R_out 
  * @param b_out 
  */
 void calculateEllipsoidTransformation(const RowVector<float,10> &U, Matrix3f &R_out, Vector3f &b_out);

 /**
  * @brief Given a set of unformatted ellipsoid parameters, find the 3x3 rotation matrix and 3x1 offset matrix which transforms the ellipsoid into a sphere
  * 
 * @param M Input matrix parameter
  * @param n Input vector parameter
  * @param d Input scalar parameter
  * @param R_out Correction matrix
  * @param b_out Correction bias
  */
 void calculateEllipsoidTransformation(const Matrix3f &M, const Vector3f &n, const float &d, Matrix3f &R_out, Vector3f &b_out);

/**
 * @brief Calibrates a set of 3D samples using ellipsoid fitting
 * 
 * @param samples 3xN matrix of input data
 * @param R_out Output rotation matrix
 * @param b_out Output bias vector
 */
void calibrateEllipsoid(const Ref<const MatrixXf> &samples, Matrix3f &R_out, Vector3f &b_out);

/**
 * @brief Computes the RMS deviation of calibrated data from the unit sphere.
 *        Returns sqrt(mean((||col|| - 1)^2)).
 *        Lower values indicate better ellipsoid-fit quality.
 * 
 * @param calibrated 3xN matrix of calibrated (and normalised) data
 * @return float RMS residual
 */
float ellipsoidFitResidual(const Ref<const Matrix3Xf> &calibrated);


}
#endif