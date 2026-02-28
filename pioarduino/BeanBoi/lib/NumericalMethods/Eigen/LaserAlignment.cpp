#include "LaserAlignment.h"

namespace NumericalMethods{
   
Vector3f normalVec(const Ref<const Matrix3Xf> &point_cloud)
{
    Vector3f normal;
    MatrixXf left_singular_mat;

    // Subtract mean from each point otherwise its wrong XD
    // https://www.ltu.se/cms_fs/1.51590!/svd-fitting.pdf
    MatrixXf mean_adj_point_cloud = point_cloud;
    mean_adj_point_cloud = mean_adj_point_cloud.colwise()-mean_adj_point_cloud.rowwise().mean();

    JacobiSVD<MatrixXf> svd(mean_adj_point_cloud, ComputeThinU | ComputeThinV);
    left_singular_mat = svd.matrixU();
    // U_cols = left_singular_mat.cols();
    // 3rd col of U contains normal vec
    normal << left_singular_mat(0,2), left_singular_mat(1,2), left_singular_mat(2,2);

    if (normal.dot(point_cloud.col(0)) < 0.0){ normal = -normal; }

    return normal;
};


void alignLaser(const Ref<const Matrix3Xf> &g, const Ref<const Matrix3Xf> &m, Matrix3f &Racc, Matrix3f &Rmag)
{
    alignToNorm(g, Racc);
    alignToNorm(m, Rmag);
}

void alignToNorm(const Ref<const Matrix3Xf> &point_cloud, Matrix3f &R)
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

    // Degeneracy guard: when vector_x is near-parallel to (0,1,0) the
    // cross product degenerates to a zero vector.  Switch reference.
    Vector3f ref = (fabsf(vector_x.dot(Vector3f(0,1,0))) > 0.9f)
                 ? Vector3f(0,0,1)
                 : Vector3f(0,1,0);
    Vector3f vector_z = vector_x.cross(ref);
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