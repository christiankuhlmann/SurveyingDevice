#include "InertialAlignment.h"
#include "utils.h"
namespace NumericalMethods{

Vector<float,10> alignMagAcc_MAGICAL(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in) {
    /************************************************************************************
     * Given a set of calibrated magnetometer and accelerometer data, this function
     * finds the least squares best fit for the alignment of the sensor axis and outputs
     * a rotation matrix for correcting the magnetometer and the magnetic inclination at
     * the location of measurement.
     *
     * 1. Calculate the design matirx, A
     * 2. Solve Ax=b with least squares
     * 3. Find SVD of least squares solution
     * 4. Calculate U_hat and R_hat
     * 5. Calculate s_hat
     * 
     * R_hat and s_hat could then be used for an iterative approach but this is not used
    ************************************************************************************/

    // Used MAG.I.CAL alignment so only 12 inputs
    static int K = N_ALIGN_MAG_ACC;
    static Matrix<float,N_ALIGN_MAG_ACC,9> A;
    static Matrix<float,3,N_ALIGN_MAG_ACC> m, g;

    m = m_in;
    g = g_in;
    m.colwise().normalize();
    g.colwise().normalize();

    // Statically allocate variables of known size
    static Vector<float,10> out;
    static RowVector<float,9> vecR;
    static Vector3f mk, gk;
    static Matrix3f H, U, V, Sig, Uhat, Rhat;

    // Step 1 - Form the design matrix, A
    for (int i=0; i<K; i++)
    {
        mk = m.col(i);
        gk = g.col(i);
        A.row(i) << kron(mk,gk).transpose();
    }

    // Step 2 - Form H
    H = ((A.transpose()*A).inverse() * A.transpose() * MatrixXf::Ones(K,1)).reshaped(3,3);

    // Step 3 - Solve lstsq
    static JacobiSVD<Matrix3f> svd;

    svd.compute(H,ComputeFullU | ComputeFullV);
    U = svd.matrixU();
    V = svd.matrixV();
    Sig = svd.singularValues().asDiagonal();

    // Step 4
    Uhat = sign(H.determinant()) * U;
    Rhat = Uhat * V.transpose();

    // Step 5
    float shat = 0;
    for (int i=0; i<K; i++)
    {
        gk = g.col(i);
        mk = m.col(i);
        shat += gk.transpose() * Rhat * mk;
    }
    shat = shat * 1/K;
    shat = std::max(-1.0f, std::min(1.0f, shat));

    out.segment(0,9) << Rhat.reshaped(9,1);
    out(9) = asin(shat);
    return out;
}

void alignMagAcc_MAGICAL(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in, 
                            Matrix3f &R_align, float &inclination_angle) {

    
    Vector<float,10> U = alignMagAcc_MAGICAL(g_in, m_in);
    R_align = U.segment(0,9).reshaped(3,3);
    inclination_angle = U(9);
}

/************************************************************************************
 * Constrained Wahba Problem (CWB) Implementation
 * 
 * Finds optimal rotation about x-axis to minimize variance of magnetic inclination. 
 * Uses golden section search for robustness on embedded systems.
 ************************************************************************************/

/**
 * @brief Computes magnetic inclination angle between magnetic field and horizontal plane
 */
static float computeInclination(const Vector3f& mag, const Vector3f& grav) {
    float magNorm = mag.norm();
    float gravNorm = grav. norm();
    
    if (magNorm < 1e-10f || gravNorm < 1e-10f) {
        return 0.0f;
    }
    
    float cosInclination = mag.dot(grav) / (magNorm * gravNorm);
    
    // Clamp to [-1, 1] for numerical safety
    if (cosInclination > 1.0f) cosInclination = 1.0f;
    if (cosInclination < -1.0f) cosInclination = -1.0f;
    
    return acosf(cosInclination);
}

/**
 * @brief Computes variance of inclination angles for a given x-axis rotation
 */
static float inclinationVariance(float theta, const Ref<const Matrix3Xf> &g, const Ref<const Matrix3Xf> &m) {
    
    assert(g.cols() ==  m.cols());
    int n_cols = g.cols();
    assert(n_cols <= N_ALIGN_MAG_ACC);

    Matrix3f R = xRotation(theta);
    
    static float inclinations[N_ALIGN_MAG_ACC];
    float sum = 0.0f;
    
    for (int i = 0; i < n_cols; i++) {
        Vector3f mag_rot = R * m.col(i);
        inclinations[i] = computeInclination(mag_rot, g.col(i));
        sum += inclinations[i];
    }
    
    float mean = sum / static_cast<float>(n_cols);
    
    float varianceSum = 0.0f;
    for (int i = 0; i < n_cols; i++) {
        float diff = inclinations[i] - mean;
        varianceSum += diff * diff;
    }
    
    return varianceSum / static_cast<float>(n_cols);
}

Vector<float,10> alignMagAcc_CWB(const Ref<const Matrix3Xf> &g_in, const Ref<const Matrix3Xf> &m_in)
{
    /*******Vector<float,10> alignMagAcc_CWB(const Ref<const Matrix3Xf> &g_in, 
                                  const Ref<const Matrix3Xf> &m_in)*****************************************************************************
     * Constrained Wahba Problem solver using golden section search. 
     * 
     * After laser calibration has aligned the x-axes of both sensors with the laser,
     * this finds the rotation about x-axis that minimizes variance of measured 
     * magnetic inclination angles. 
     * 
     * Golden section search is used because: 
     * - No derivatives required (robust to noise)
     * - Guaranteed convergence for unimodal functions
     * - Simple implementation suitable for embedded systems
     * - Linear convergence is sufficient for this 1D problem
     ************************************************************************************/
    

    assert(g_in.cols() ==  m_in.cols());
    int n_cols = g_in.cols();
    assert(n_cols <= N_ALIGN_MAG_ACC);

    static Vector<float,10> out;
    static Matrix<float,3,N_ALIGN_MAG_ACC> m, g;
    
    // Normalize input data
    m = m_in;
    g = g_in;
    for (int i = 0; i < n_cols; ++i) {
        m.col(i).normalize();
        g.col(i).normalize();
    }
    
    // Coarse grid pre-scan: evaluate 8 equally-spaced angles to find the
    // basin of the global minimum.  This eliminates failure when the
    // inclination-variance landscape has two symmetric minima (θ and θ+π).
    const int N_GRID = 8;
    float best_theta = 0.0f;
    float best_var   = 1e30f;
    for (int k = 0; k < N_GRID; ++k) {
        float t = -static_cast<float>(M_PI) + k * (2.0f * static_cast<float>(M_PI) / N_GRID);
        float v = inclinationVariance(t, g, m);
        if (v < best_var) {
            best_var   = v;
            best_theta = t;
        }
    }

    // Golden section search parameters
    const float phi = 1.6180339887f;  // Golden ratio
    const float resphi = 0.3819660113f;  // 2 - phi
    const float tol = 1e-6f;
    const int maxIter = 50;
    
    // Bracket around the best coarse-grid point (±π/4)
    float a = best_theta - static_cast<float>(M_PI) / 4.0f;
    float b = best_theta + static_cast<float>(M_PI) / 4.0f;
    
    float x1 = a + resphi * (b - a);
    float x2 = b - resphi * (b - a);
    float f1 = inclinationVariance(x1, g, m);
    float f2 = inclinationVariance(x2, g, m);
    
    int iter = 0;
    while (fabsf(b - a) > tol && iter < maxIter) {
        if (f1 < f2) {
            b = x2;
            x2 = x1;
            f2 = f1;
            x1 = a + resphi * (b - a);
            f1 = inclinationVariance(x1, g, m);
        } else {
            a = x1;
            x1 = x2;
            f1 = f2;
            x2 = b - resphi * (b - a);
            f2 = inclinationVariance(x2, g, m);
        }
        ++iter;
    }
    
    float theta = (a + b) / 2.0f;
    Matrix3f R = xRotation(theta);
    
    // Compute mean inclination at optimal theta
    float inclinationSum = 0.0f;
    for (int i = 0; i < n_cols; ++i) {
        Vector3f mag_rot = R * m.col(i);
        inclinationSum += computeInclination(mag_rot, g. col(i));
    }
    float meanInclination = inclinationSum / static_cast<float>(n_cols);
    
    // Pack output
    out.segment(0,9) << R.reshaped(9,1);
    out(9) = meanInclination;
    
    return out;
}

void alignMagAcc_CWB(const Ref<const Matrix3Xf> &g_in, 
                      const Ref<const Matrix3Xf> &m_in, 
                      Matrix3f &R_align, 
                      float &inclination_angle,
                      float *inclination_variance_out) {
    
    Vector<float,10> U = alignMagAcc_CWB(g_in, m_in);
    R_align = U.segment(0,9).reshaped(3,3);
    inclination_angle = U(9);

    // Optionally report the final inclination variance
    if (inclination_variance_out) {
        // Normalise copies for variance computation (matches internal CWB logic)
        int n_cols = g_in.cols();
        static Matrix<float,3,N_ALIGN_MAG_ACC> m_tmp, g_tmp;
        m_tmp = m_in;
        g_tmp = g_in;
        for (int i = 0; i < n_cols; ++i) {
            m_tmp.col(i).normalize();
            g_tmp.col(i).normalize();
        }
        *inclination_variance_out = inclinationVariance(
            atan2f(R_align(2,1), R_align(1,1)),  // recover theta from R_x
            g_tmp, m_tmp);
    }
}




}