#ifndef NUMERICAL_METHODS_FITTINGFUNCS_H
#define NUMERICAL_METHODS_FITTINGFUNCS_H

#include "utils.h"

// MUST include ArduinoEigenExtension if using ArduinoEigen

/**
 * @brief Given a point cloud, find the best fit ellipsoid to fit these values and return a vector of the ellipsoid parameters
 * @todo Add sources for calculations
 *
 * @param samples 
 * @return RowVector<float,10> 
 */
RowVector<float,10> fit_ellipsoid(const MatrixXf &samples)
{
    // Uses ~400 floats
    static Matrix<float,6,6> C;
    static Matrix<float,10,10> S;
    static Matrix<float,6,6> S11;
    static Matrix<float,6,4> S12;
    static Matrix<float,4,6> S21;
    static Matrix<float,4,4> S22;
    static Matrix<float,6,6> M;
    static EigenSolver<Matrix<float,6,6>> es;
    static Vector<float,6> eval;
    static Matrix<float,6,6> evec;
    static Vector<std::complex<float>,6> eigenvalues;
    static Matrix<std::complex<float>,6,6> eigenvectors;
    static Vector<float,6> u1;
    static Vector<float,4> u2;
    static Vector<float,10> U;

    static int n_samples = samples.cols();

    const VectorXf &x = samples.row(0).segment(0,n_samples);
    const VectorXf &y = samples.row(1).segment(0,n_samples);
    const VectorXf &z = samples.row(2).segment(0,n_samples);

    // This could end up being very large! Be careful!
    // Check limits on available memory!
    // Remaining stack size MUST be greater than  n_samples x 10 x 4bytes!
    MatrixXf D_T(n_samples,10);

    // Create design matrix
    D_T.setZero();
    C.setZero();

    D_T.col(0) << x.array().pow(2);
    D_T.col(1) << y.array().pow(2);
    D_T.col(2) << z.array().pow(2);
    D_T.col(3) << 2*y.array()*z.array();
    D_T.col(4) << 2*x.array()*z.array();
    D_T.col(5) << 2*x.array()*y.array();
    D_T.col(6) << 2*x.array();
    D_T.col(7) << 2*y.array();
    D_T.col(8) << 2*z.array();
    D_T.col(9) << VectorXf::Ones(n_samples);

    int k = 4;

    // Create constrain matrix C - Eq(7)
    C <<   -1, 0.5*k-1, 0.5*k-1, 0, 0, 0,
            0.5*k-1, -1, 0.5*k-1, 0, 0, 0,
            0.5*k-1, 0.5*k-1, -1, 0, 0, 0,
            0, 0, 0, -k, 0, 0,
            0, 0, 0, 0, -k, 0,
            0, 0, 0, 0, 0, -k;

    // Create S matrix from D*D.T - Eqn(11)
    S = D_T.transpose() * D_T;
    S11 = S.block<6,6>(0,0);
    S12 = S.block<6,4>(0,6);
    S21 = S.block<4,6>(6,0);
    S22 = S.block<4,4>(6,6);

    // Solve least squares - Eqn(14) and Eqn(15)
    M  = C.inverse() * (S11 - S12*S22.inverse() * S21);
    es.compute(M);
    eigenvalues = es.eigenvalues();
    eigenvectors = es.eigenvectors();

    eval = eigenvalues.array().real();
    evec = eigenvectors.array().real();

    // Find eigenvector corresponding to largest eigenvalue
    float max_eval = eval[0];
    for (int i=0;i<6;i++)
    {
        if(eval[i] > max_eval) {
            max_eval = eval[i];
            u1 = evec.col(i);
        } else if (i == 5) {
            // No eigenvalues found
            // TODO: Add error throw that is safe with ESP
        }
    }

    // To preserve stabikity of calculations. Use pseudoinverse it determinant too small
    if (S22.determinant() < 0.05)
    {
        u2 = -(pseudoInverse(S22) * S21) * u1;
    } else {
        u2 = (-(S22.inverse() * S21) * u1);
    }

    // Form output vector
    U << u1, u2;
    return U;
}

/**
 * @brief Given a set of formatted ellipsoid parameters, find the 3x3 rotation matrix and 3x1 offset matrix which transforms the ellipsoid into a sphere
 * @todo Add sources for calculations
 * 
 * @param M 
 * @param n 
 * @param d 
 * @return Vector<float,12> 
 */
Vector<float,12> calculate_ellipsoid_transformation(Matrix3f &M, Vector3f &n, float d)
{
    /*
    * Statics are not used here as the fit_ellipsoid function executing will ensure that there is enough memory left on the stack.
    * If for some reason there is not enough, one should consider running this immediately after calculating the fitting parameters
    */ 
    

    // Inverse of M
    Matrix3f M_ = M.inverse();
    // Calculate offset vector
    Vector3f b = -M_ * n;

    /******************************************************************************************
     * Equation (17) of https://teslabs.com/articles/magnetometer-calibration/
     * Used to convert ellipsoid parameters to a transformation matrix back to a unit sphere
     * Matrix square root follows https://math.stackexchange.com/a/59391
     ******************************************************************************************/
    EigenSolver<MatrixXf> es(M);
    Matrix3cf eval = es.eigenvalues().asDiagonal();
    Matrix3cf evec = es.eigenvectors();

    // Calculate diagonal matrix of square roots of eigenvectors
    Matrix3cf eval_sqrt;
    eval_sqrt << eval.array().sqrt();

    // Calculate square root of matrix
    Matrix3cf M_sqrt;
    M_sqrt << evec * eval_sqrt * evec.inverse();

    // Calculate inner square root
    std::complex<float> inner = n.transpose() * M_ * n - d;
    std::complex<float> sqrtinv = (std::complex<float>)pow(sqrt(inner),-1);


    // Calculate final transformation matrix
    Matrix3cf A_1;
    A_1 << (sqrtinv * M_sqrt).array();
    Matrix3f A_1real = A_1.array().real();

    // Return as a vector
    Map<VectorXf> v1(A_1real.data(),9);
    Map<VectorXf> v2(b.data(), 3);
    Vector<float,12> V;
    V << v1, v2;
    return V;
}

/**
 * @brief Given a set of unformatted ellipsoid parameters, find the 3x3 rotation matrix and 3x1 offset matrix which transforms the ellipsoid into a sphere
 * @todo Add sources for calculations
 * 
 * @param U Unformatted ellipsoid parameters
 * @return Vector<float,12> 
 */
Vector<float,12> calculate_ellipsoid_transformation(const RowVector<float,10> &U)
{
    /*
    * Statics are not used here as the fit_ellipsoid function executing will ensure that there is enough memory left on the stack.
    * If for some reason there is not enough, one should consider running this immediately after calculating the fitting parameters
    */ 

    Matrix3f M;
    Vector3f n;
    float d;

    M << U[0], U[5], U[4], U[5], U[1], U[3], U[4], U[3], U[2];
    n << U[6], U[7], U[8];
    d = U[9];

    return calculate_ellipsoid_transformation(M,n,d);
}
#endif