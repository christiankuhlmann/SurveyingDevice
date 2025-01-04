#include "LaserAlignment.h"

namespace NumericalMethods{
   
void alignLaser(const MatrixXf &g, const MatrixXf &m, Matrix3f &Racc, Matrix3f &Rmag)
{
    alignToNorm(g, Racc);
    alignToNorm(m, Rmag);
}

void alignToNorm(const Matrix<float,3,N_LASER_CAL> &point_cloud, Matrix3f &R)
{
    // Calculate normal to plane
    Vector3f target_vector;
    target_vector = normalVec(point_cloud);
    
    // Serial.printf("Target_vector: %f %f %f \n", target_vector(0), target_vector(1), target_vector(2));

    target_vector = target_vector/target_vector.norm();
    if (target_vector.dot(point_cloud.col(1)) < 0)
    {
        target_vector = -target_vector;
    }

    /************************************************
     * Translate to new coordinate space (ENU)
     * 1. Set new x axis as target_vector
     * 2. Set new z axis as x_vector crossed with (0 1 0)
     * 2. Set new y axis as x_vector crossed with (0 1 0)
     */
   
    Vector3f vector_x = target_vector;
    Vector3f vector_z = vector_x.cross(Vector3f(0,1,0));
    Vector3f vector_y = vector_z.cross(-vector_x);

    vector_x.normalize();
    vector_y.normalize();
    vector_z.normalize();

    // Transpose of a rotation matrix is its inverse
    R.row(0) = vector_x;
    R.row(1) = vector_y;
    R.row(2) = vector_z;

    // Serial.printf("Rotation Matrix: \n %f %f %f \n %f %f %f \n %f %f %f \n\n", 
    // vector_x(0), vector_x(1), vector_x(2),
    // vector_y(0), vector_y(1), vector_y(2),
    // vector_z(0), vector_z(1), vector_z(2));
}

}