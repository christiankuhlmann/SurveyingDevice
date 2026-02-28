#include "measurements.h"
#include "json.hpp"
#include <fstream>
#include <iostream>

using json = nlohmann::json;


// -------------------- True data variables --------------------
// True static Calibration data (12 orientations x 20 samples = 240)
Matrix<float,3,N_ALIGN_MAG_ACC> true_static_body_data_x;
Matrix<float,3,N_ALIGN_MAG_ACC> true_static_body_data_y;
Matrix<float,3,N_ALIGN_MAG_ACC> true_static_body_data_z;
Matrix<float,3,N_ALIGN_MAG_ACC> true_static_mag_data;
Matrix<float,3,N_ALIGN_MAG_ACC> true_static_acc_data;


// True Laser Calibration data
Matrix<float,3,8> true_laser_body_data_x;
Matrix<float,3,8> true_laser_body_data_y;
Matrix<float,3,8> true_laser_body_data_z;
Matrix<float,3,8> true_laser_mag_data;
Matrix<float,3,8> true_laser_acc_data;
Matrix<float,3,8> true_laser_data;

// -------------------- Measurement variables --------------------
Matrix<float,3,N_ALIGN_MAG_ACC> measured_static_mag_data, measured_static_acc_data;
Matrix<float,3,8> measured_laser_mag_data, measured_laser_acc_data;

Matrix3f mag_Tsi;
Matrix3f mag_R_mount;  // Physical mounting misalignment between mag and acc ICs
Matrix3f mag_Tcc, acc_Tcc;
Matrix3f mag_Tsf, acc_Tsf;
Vector3f mag_h_hi, acc_h_a;
Vector3f mag_h_hb;

// -------------------- Calibration variables --------------------
Matrix<float,3,N_ALIGN_MAG_ACC> calibrated_static_mag_data, calibrated_static_acc_data;
Matrix<float,3,8> calibrated_laser_mag_data, calibrated_laser_acc_data;

Matrix3f Rm_calib, Ra_calib;
Vector3f bm_calib, ba_calib;

// -------------------- Laser Alignment variables --------------------
Matrix<float,3,N_ALIGN_MAG_ACC> laser_aligned_static_mag_data, laser_aligned_static_acc_data;
Matrix<float,3,8> laser_aligned_laser_mag_data, laser_aligned_laser_acc_data;

Matrix3f Rm_laser, Ra_laser;

// -------------------- Mag-Acc Alignment variables --------------------
Matrix<float,3,N_ALIGN_MAG_ACC> mag_acc_aligned_static_mag_data, mag_acc_aligned_static_acc_data;
Matrix<float,3,8> mag_acc_aligned_laser_mag_data, mag_acc_aligned_laser_acc_data;

Matrix3f Rm_align;
float inclination_angle;
float inclination_variance;


void runGeneratedTest()
{
    srand(42);  // Fixed seed for reproducible results

    std::cout << "Starting generated test..." << std::endl;
    // 1. Generate data 
    // 1.1 Generate static data
    generateTrueStaticData(
        true_static_body_data_x,
        true_static_body_data_y,
        true_static_body_data_z,
        true_static_mag_data,
        true_static_acc_data);

    // 1.2 Generate laser data
    generateTrueLaserData(
        true_laser_body_data_x,
        true_laser_body_data_y,
        true_laser_body_data_z,
        true_laser_mag_data,
        true_laser_acc_data,
        true_laser_data
    );

   std::cout << "Generated true data." << std::endl;

    // 1.3 Generate sensor errors
    generateCrossCouplingMatrix(mag_Tcc, MAG_CROSS_COUPLING_AMPLITUDE);
    generateScaleFactorMatrix(mag_Tsf, MAG_SCALEFACTOR_AMPLITUDE);
    generateSoftIronMatrix(mag_Tsi);
    generateMountingMisalignment(mag_R_mount, MAG_MOUNTING_MISALIGNMENT_DEG);
    generateBiasVector(mag_h_hi, MAG_HARDIRON_AMPLITUDE);
    generateBiasVector(mag_h_hb, MAG_BIAS_ERROR_AMPLITUDE);

    generateCrossCouplingMatrix(acc_Tcc, ACC_CROSS_COUPLING_AMPLITUDE);
    generateScaleFactorMatrix(acc_Tsf, ACC_SCALEFACTOR_AMPLITUDE);
    generateBiasVector(acc_h_a, ACC_BIAS_AMPLITUDE);

    std::cout << "Generated sensor error models." << std::endl;

    // 1.3 Apply sensor errors
    applyMagErrors(true_static_mag_data, measured_static_mag_data, mag_Tcc, mag_Tsf, mag_Tsi, mag_R_mount, mag_h_hi, mag_h_hb);
    applyAccErrors(true_static_acc_data, measured_static_acc_data, acc_Tcc, acc_Tsf, acc_h_a);

    applyMagErrors(true_laser_mag_data, measured_laser_mag_data, mag_Tcc, mag_Tsf, mag_Tsi, mag_R_mount, mag_h_hi, mag_h_hb);
    applyAccErrors(true_laser_acc_data, measured_laser_acc_data, acc_Tcc, acc_Tsf, acc_h_a);

    std::cout << "Applied sensor errors to static and laser data." << std::endl;
    // 2. Calibrate accelerometer and magnetometer

    std::cout << "Starting calibration..." << std::endl;
    // Perform calibration
    NumericalMethods::calibrateEllipsoid(measured_static_mag_data, Rm_calib, bm_calib);
    NumericalMethods::calibrateEllipsoid(measured_static_acc_data, Ra_calib, ba_calib);
    
    std::cout << "Calibration complete." << std::endl;
    // Apply calibrations
    calibrated_static_mag_data = Rm_calib * (measured_static_mag_data.colwise() - bm_calib);
    calibrated_static_acc_data = Ra_calib * (measured_static_acc_data.colwise() - ba_calib);
    calibrated_laser_mag_data = Rm_calib * (measured_laser_mag_data.colwise() - bm_calib);
    calibrated_laser_acc_data = Ra_calib * (measured_laser_acc_data.colwise() - ba_calib);

    std::cout << "Applied calibrations to measured data." << std::endl;
    // Save pre-normalisation calibrated data for magnitude analysis
    Matrix<float,3,N_ALIGN_MAG_ACC> calibrated_static_mag_unnorm = calibrated_static_mag_data;
    Matrix<float,3,N_ALIGN_MAG_ACC> calibrated_static_acc_unnorm = calibrated_static_acc_data;

    // Normalize calibrated data
    calibrated_static_acc_data.colwise().normalize();
    calibrated_static_mag_data.colwise().normalize();
    calibrated_laser_acc_data.colwise().normalize();
    calibrated_laser_mag_data.colwise().normalize();

    std::cout << "Normalized calibrated data." << std::endl;
    // 3. Align sensors with laser direction

    std::cout << "Starting sensor-laser alignment..." << std::endl;
    // Perform alignment
    NumericalMethods::alignLaser(calibrated_laser_acc_data, calibrated_laser_mag_data, Ra_laser, Rm_laser);


    std::cout << "Sensor-laser alignment complete." << std::endl;
    // Apply alignments
    laser_aligned_static_mag_data = Rm_laser * calibrated_static_mag_data;
    laser_aligned_static_acc_data = Ra_laser * calibrated_static_acc_data;
    laser_aligned_laser_mag_data = Rm_laser * calibrated_laser_mag_data;
    laser_aligned_laser_acc_data = Ra_laser * calibrated_laser_acc_data;


    std::cout << "Applied sensor-laser alignment to calibrated data." << std::endl;
    // 4. Solve constrained Wahba problem (enforce rotation about x-axis)

    std::cout << "Starting magnetometer-accelerometer alignment..." << std::endl;
    // Perform alignment
    NumericalMethods::alignMagAcc_CWB(laser_aligned_static_acc_data, laser_aligned_static_mag_data, Rm_align, inclination_angle, &inclination_variance);

    std::cout << "Magnetometer-accelerometer alignment complete." << std::endl;
    // Apply alignments
    mag_acc_aligned_static_mag_data = Rm_align * laser_aligned_static_mag_data;
    mag_acc_aligned_laser_mag_data = Rm_align * laser_aligned_laser_mag_data;

    // Acc is unchanged by CWB (rotation is about shared x-axis, applied to mag only)
    mag_acc_aligned_static_acc_data = laser_aligned_static_acc_data;
    mag_acc_aligned_laser_acc_data = laser_aligned_laser_acc_data;

    std::cout << "Applied magnetometer-accelerometer alignment to laser-aligned data." << std::endl;

    // 5. Compute calibration quality metric
    NumericalMethods::CalibrationQuality quality = NumericalMethods::computeCalibrationQuality(
        calibrated_static_mag_unnorm,
        calibrated_static_acc_unnorm,
        calibrated_laser_mag_data,
        calibrated_laser_acc_data,
        inclination_variance);

    std::cout << "\n  Calibration Quality Grade: " << NumericalMethods::gradeToString(quality.grade) << std::endl;
    std::cout << "    Inclination σ:       " << quality.inclination_sigma_deg << "°" << std::endl;
    std::cout << "    Mag fit residual:    " << quality.mag_fit_residual << std::endl;
    std::cout << "    Acc fit residual:    " << quality.acc_fit_residual << std::endl;
    std::cout << "    Laser plane spread:  " << quality.laser_plane_spread_deg << "°\n" << std::endl;

    // 6. Save calibration to JSON file for visualisation
    json j;
    
    // Helper lambda to convert Eigen matrix to json array
    auto matrixToJson = [](const auto& mat) {
        json result = json::array();
        for (int i = 0; i < mat.rows(); ++i) {
            json row = json::array();
            for (int j = 0; j < mat.cols(); ++j) {
                row.push_back(mat(i, j));
            }
            result.push_back(row);
        }
        return result;
    };
    
    auto vectorToJson = [](const auto& vec) {
        json result = json::array();
        for (int i = 0; i < vec.size(); ++i) {
            result.push_back(vec(i));
        }
        return result;
    };
    
    // True static data
    j["true_static_mag_data"] = matrixToJson(true_static_mag_data);
    j["true_static_acc_data"] = matrixToJson(true_static_acc_data);
    j["true_laser_mag_data"] = matrixToJson(true_laser_mag_data);
    j["true_laser_acc_data"] = matrixToJson(true_laser_acc_data);
    j["true_laser_data"] = matrixToJson(true_laser_data);
    
    // Measured data
    j["measured_static_mag_data"] = matrixToJson(measured_static_mag_data);
    j["measured_static_acc_data"] = matrixToJson(measured_static_acc_data);
    j["measured_laser_mag_data"] = matrixToJson(measured_laser_mag_data);
    j["measured_laser_acc_data"] = matrixToJson(measured_laser_acc_data);
    
    // Sensor error matrices
    j["mag_Tcc"] = matrixToJson(mag_Tcc);
    j["mag_Tsf"] = matrixToJson(mag_Tsf);
    j["mag_Tsi"] = matrixToJson(mag_Tsi);
    j["mag_R_mount"] = matrixToJson(mag_R_mount);
    j["mag_h_hi"] = vectorToJson(mag_h_hi);
    j["mag_h_hb"] = vectorToJson(mag_h_hb);
    j["acc_Tcc"] = matrixToJson(acc_Tcc);
    j["acc_Tsf"] = matrixToJson(acc_Tsf);
    j["acc_h_a"] = vectorToJson(acc_h_a);
    
    // Calibration matrices and vectors
    j["Rm_calib"] = matrixToJson(Rm_calib);
    j["bm_calib"] = vectorToJson(bm_calib);
    j["Ra_calib"] = matrixToJson(Ra_calib);
    j["ba_calib"] = vectorToJson(ba_calib);
    
    // Calibrated data (normalised)
    j["calibrated_static_mag_data"] = matrixToJson(calibrated_static_mag_data);
    j["calibrated_static_acc_data"] = matrixToJson(calibrated_static_acc_data);
    j["calibrated_laser_mag_data"] = matrixToJson(calibrated_laser_mag_data);
    j["calibrated_laser_acc_data"] = matrixToJson(calibrated_laser_acc_data);
    
    // Calibrated data (pre-normalisation — for magnitude analysis)
    j["calibrated_mag_unnorm"] = matrixToJson(calibrated_static_mag_unnorm);
    j["calibrated_acc_unnorm"] = matrixToJson(calibrated_static_acc_unnorm);
    
    // Laser alignment matrices
    j["Rm_laser"] = matrixToJson(Rm_laser);
    j["Ra_laser"] = matrixToJson(Ra_laser);
    
    // Laser aligned data
    j["laser_aligned_static_mag_data"] = matrixToJson(laser_aligned_static_mag_data);
    j["laser_aligned_static_acc_data"] = matrixToJson(laser_aligned_static_acc_data);
    j["laser_aligned_laser_mag_data"] = matrixToJson(laser_aligned_laser_mag_data);
    j["laser_aligned_laser_acc_data"] = matrixToJson(laser_aligned_laser_acc_data);
    
    // Mag-Acc alignment matrix and inclination
    j["Rm_align"] = matrixToJson(Rm_align);
    j["inclination_angle"] = inclination_angle;
    j["inclination_variance"] = inclination_variance;
    
    // Calibration quality metrics
    j["quality_grade"] = NumericalMethods::gradeToString(quality.grade);
    j["quality_grade_int"] = static_cast<int>(quality.grade);
    j["quality_inclination_sigma_deg"] = quality.inclination_sigma_deg;
    j["quality_mag_fit_residual"] = quality.mag_fit_residual;
    j["quality_acc_fit_residual"] = quality.acc_fit_residual;
    j["quality_laser_plane_spread_deg"] = quality.laser_plane_spread_deg;
    
    // Final aligned data
    j["mag_acc_aligned_static_mag_data"] = matrixToJson(mag_acc_aligned_static_mag_data);
    j["mag_acc_aligned_static_acc_data"] = matrixToJson(mag_acc_aligned_static_acc_data);
    j["mag_acc_aligned_laser_mag_data"] = matrixToJson(mag_acc_aligned_laser_mag_data);
    j["mag_acc_aligned_laser_acc_data"] = matrixToJson(mag_acc_aligned_laser_acc_data);
    
    // Write to file
    std::ofstream outputFile("calibration_results.json");
    outputFile << std::setw(2) << j << std::endl;
    outputFile.close();
    
}

void runRealDataTest()
{

}


int main() {

    runGeneratedTest();

    return 0;
}