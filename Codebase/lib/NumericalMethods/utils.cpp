#include "utils.h"
namespace NumericalMethods {

float angle(Vector3f u, Vector3f v)
{
    return acos(u.dot(v)/(u.norm()*v.norm()));
}

Matrix3f quatRot(const Vector3f &ax, float rads)
{
    Vector3f ax_normalised = ax.normalized();
    Quaternionf Q(cos(rads/2),ax_normalised(0)*sin(rads/2), ax_normalised(1)*sin(rads/2), ax_normalised(2)*sin(rads/2));
    return Q.toRotationMatrix();
}

Matrix3f xRotation(const float rad)
{
    return quatRot(Vector3f(1,0,0),rad);
}

Matrix3f yRotation(const float rad)
{
    return quatRot(Vector3f(0,1,0),rad);
}

Matrix3f zRotation(const float rad)
{
    return quatRot(Vector3f(0,0,1),rad);

}

MatrixXf kron(const MatrixXf &m1, const MatrixXf &m2)
{
    int m = m1.rows();
    int n = m1.cols();
    int p = m2.rows();
    int q = m2.cols();

    MatrixXf out(m*p, n*q);
    for(int i=0; i<m; i++)
    {
        for(int j=0; j<n; j++) {
            out.block(i*p,j*q,p,q) = m1(i,j) * m2;
        }
    }
    return out;
}

Matrix3f inertialToENU(const Vector3f &m, const Vector3f &g)
{
    /*****************************************************
     * 1. Sensor calibration and alignment aligns the laser axis with the x-axis.
     *    This makes the x-axis of the device the laser axis after calibrations are applied
     * 2. East is located at the gravoty vector crossed with the magnetic bector
     * 3. North is located at the gravity vector crossed with the East vector
     * 4. Up is located at the North vector crossed with the East vector
     */
    Matrix3f ENU;
    

    Serial.println("ENU");
    Serial.printf("g data: X %f   Y %f   Z %f   Norm: %f\n", g(0), g(1), g(2), g.norm());
    Serial.printf("m data: X %f   Y %f   Z %f   Norm: %f\n", m(0), m(1), m(2), m.norm());
    

    // Use cross product to generate set of real world axis in body frame
    Vector3f E, N, U;
    E = g.cross(m);
    E.normalize();

    N = E.cross(g);
    N.normalize();

    U = E.cross(N);
    U.normalize();

    Serial.printf("E data: X %f   Y %f   Z %f   Norm: %f\n", E(0), E(1), E(2), E.norm());
    Serial.printf("N data: X %f   Y %f   Z %f   Norm: %f\n", N(0), N(1), N(2), N.norm());
    Serial.printf("U data: X %f   Y %f   Z %f   Norm: %f\n", U(0), U(1), U(2), U.norm());

    ENU << E, N, U;
    return ENU;
}

Vector3f inertialToVector(const Vector3f &m, const Vector3f &g)
{
    Vector3f V;
    Matrix3f ENU = inertialToENU(m, g);

    // Extract the x values of each axis to find the x-axis in the world frame
    V << ENU(0,0), ENU(1,0), ENU(2,0);
    return V;
}

Vector3f inertialToCardan(const Vector3f &m, const Vector3f &g)
{
    Vector3f HIR;
    Matrix3f ENU = inertialToENU(m, g);

    /****************************************************************************************
     * atan2(Ex,Nx) -> atan2 of north and east components of sensor x-axis in world frame
     * Ex -> How much am I facing East?
     * Nx -> How much am I facing North?
     * atan2 (Ex, Nx) = Angle of device from North
     ****************************************************************************************/
    
    // atan2 Ex, Nx
    Vector3f E, N, U;
    E = ENU.col(0);
    N = ENU.col(1);
    U = ENU.col(2);

    HIR(0) = atan2(E(0),N(0));

    // atan2(Ux,sqrt(Ex^2 + Nx^2)) -> Inclination of U above XZ plane
    // Alternatively atan2(Ux*cos(heading), Nx) works as sqrt(Ex^2 + Nx^2) = Nx/cos(heading)
    HIR(1) = atan2(U(0), sqrt(pow(E(0),2) + pow(N(0),2))); // atand(y,x) -> atan2(Ux, sqrt(Ex^2 + Nx^2))

    // Angle between device z axis and actual g measurement when projected into the YZ plane
    // atan2(-Uy, Uz)
    HIR(2) = atan2(-U(1),U(2));

    // Bind data to 0 to 2*pi
    if (fabs(HIR(0)) > 2*M_PI){         HIR(0) = HIR(0) - 2*M_PI;           }
    if (fabs(HIR(1)) > 2*M_PI){         HIR(1) = HIR(1) - 2*M_PI;           }
    if (fabs(HIR(2)) > 2*M_PI){         HIR(2) = HIR(2) - 2*M_PI;           }
    
    return HIR;
}

int sign(const float &f)
{
    if (f>=0)
    {
        return 1;
    } else {
        return -1;
    }
}

float stDev(const VectorXf &vec)
{
    return sqrt((vec.array() - vec.mean()).square().sum()/(vec.size()-1));
}

template <typename Derived>
int removeNullData(MatrixBase<Derived> &mat)
{
    // Initialise blank cols mat to -1
    VectorXi blank_cols(mat.cols());
    blank_cols.setOnes();
    blank_cols *= -1;
    int index = 0;
    int n_zeros = 0; // Number of zeroes found

    int i;
    // Index zero values in reverse order
    for (int i=mat.cols()-1; i>-1; i--)
    {
        if (mat.col(i).norm() == 0)
        {
            blank_cols(index) = i;
            index++;
        }
    }
    n_zeros = index;

    // Push index back due to index++ happening AFTER assignment
    index--;
    if (index == -1)
    {
        // No zeroes found
        return 0;
    }

    // Iterate in reverse through matrix, replacing zero valued sections with non-zero valued elements nearest the end of the matrix, replacing those with zero
    for (int i=mat.cols()-1; i>-1; i--)
    {
        // Check if value is non-zero
        if (mat.col(i).norm() > 0)
        {
            // Replace zero value closest to start or matrix with non-zero value
            mat.col(blank_cols(index)) = mat.col(i);
            // Replace non-zero value with zero
            mat.col(i) << 0, 0, 0;
            // Decrease index
            index--;

            // If all zero-valued sections have been replaced, break
            if (index < 0)
            {
                break;
            }
        }
    }

    return n_zeros;
}

}