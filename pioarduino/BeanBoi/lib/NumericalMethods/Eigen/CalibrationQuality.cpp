#include "CalibrationQuality.h"
#include "Calibration.h"
#include <cmath>

namespace NumericalMethods {

const char* gradeToString(Grade g) {
    switch (g) {
        case Grade::Excellent: return "Excellent";
        case Grade::VeryGood:  return "VeryGood";
        case Grade::Good:      return "Good";
        case Grade::Fair:      return "Fair";
        case Grade::Poor:      return "Poor";
        case Grade::Unusable:  return "Unusable";
        default:               return "Unknown";
    }
}

/**
 * @brief Compute the angular spread (degrees) of laser cone normals.
 *
 * Recomputes the 3×N SVD for each sensor's laser cone data and converts the
 * ratio of smallest to second-smallest singular value into an angular spread.
 * A well-defined cone (good plane fit) has a very small ratio → small spread.
 */
static float laserPlaneSpread(const Ref<const Matrix3Xf> &laser_data) {
    // Mean-centre the data (same approach as normalVec in LaserAlignment.cpp)
    MatrixXf centred = laser_data;
    centred = centred.colwise() - centred.rowwise().mean();

    JacobiSVD<MatrixXf> svd(centred, ComputeThinU | ComputeThinV);
    Vector3f sv = svd.singularValues();

    // sv(2) is the smallest — perpendicular to the plane.
    // sv(1) is the second-smallest — in-plane.
    // Their ratio measures how "thick" the cone is relative to its spread.
    if (sv(1) < 1e-10f) return 90.0f;  // degenerate

    return atanf(sv(2) / sv(1)) * RAD_TO_DEG;
}

CalibrationQuality computeCalibrationQuality(
    const Ref<const Matrix3Xf> &calibrated_mag,
    const Ref<const Matrix3Xf> &calibrated_acc,
    const Ref<const Matrix3Xf> &laser_mag,
    const Ref<const Matrix3Xf> &laser_acc,
    float cwb_inclination_variance)
{
    CalibrationQuality q;

    // 1. Inclination σ from CWB variance (convert rad → deg)
    q.inclination_sigma_deg = sqrtf(cwb_inclination_variance) * RAD_TO_DEG;

    // 2. Ellipsoid fit residuals (RMS deviation from unit sphere)
    q.mag_fit_residual = ellipsoidFitResidual(calibrated_mag);
    q.acc_fit_residual = ellipsoidFitResidual(calibrated_acc);

    // 3. Laser plane spread — average of mag and acc cone spreads
    float mag_spread = laserPlaneSpread(laser_mag);
    float acc_spread = laserPlaneSpread(laser_acc);
    q.laser_plane_spread_deg = (mag_spread + acc_spread) * 0.5f;

    // 4. Grade assignment
    //    Thresholds (upper bounds for each grade level):
    //                          incl_σ°   fit_res   laser_spread°
    //    Excellent (5):        ≤ 0.5     ≤ 0.005   ≤ 1.0
    //    VeryGood  (4):        ≤ 1.0     ≤ 0.01    ≤ 2.0
    //    Good      (3):        ≤ 2.0     ≤ 0.02    ≤ 5.0
    //    Fair      (2):        ≤ 5.0     ≤ 0.05    ≤ 10.0
    //    Poor      (1):        ≤ 10.0    ≤ 0.10    ≤ 20.0
    //    Unusable  (0):        > 10.0    > 0.10    > 20.0

    const float incl_thresh[]   = { 0.5f, 1.0f,  2.0f,   5.0f,  10.0f };
    const float fit_thresh[]    = { 0.005f, 0.01f, 0.02f, 0.05f, 0.10f };
    const float laser_thresh[]  = { 1.0f, 2.0f,  5.0f,  10.0f,  20.0f };

    float worst_fit = (q.mag_fit_residual > q.acc_fit_residual)
                    ? q.mag_fit_residual : q.acc_fit_residual;

    // Start at Excellent and downgrade
    int g = 5;
    for (int lvl = 0; lvl < 5; ++lvl) {
        if (q.inclination_sigma_deg <= incl_thresh[lvl] &&
            worst_fit               <= fit_thresh[lvl]  &&
            q.laser_plane_spread_deg <= laser_thresh[lvl]) {
            g = 5 - lvl;   // 5 = Excellent, 4 = VeryGood, ...
            break;
        }
        if (lvl == 4) {
            // Didn't meet any threshold
            g = 0;
        }
    }

    q.grade = static_cast<Grade>(g);
    return q;
}

}
