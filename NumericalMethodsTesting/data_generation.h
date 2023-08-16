//
// Created by chris on 13/04/2023.
//

#ifndef MAGNETOMETER_CALIBRATION_DATA_GENERATION_H
#define MAGNETOMETER_CALIBRATION_DATA_GENERATION_H

#include "Eigen/Dense"
#include <random>
#define _USE_MATH_DEFINES
#include <math.h>

#include "util.h"


// #define DISPERSED_GENERATION
#ifdef DISPERSED_GENERATION
#define N_SAMPLES 100
#define N_Y 10
#define N_Z N_SAMPLES/N_Y
#else
#define N_SAMPLES 12*5
#endif

#define INCLINATION_OFFSET 0.0872664626
#define HEADING_OFFSET -0.0436332313

using namespace Eigen;

MatrixXf generateTrueInertialAlignData(Vector3f true_vec)
{
#ifdef DISPERSED_GENERATION
    // ------------------------------------------
    int y;
    int z;
    int n = 0;

    MatrixXf samples = MatrixXf::Zero(3,N_SAMPLES);
    Matrix3f R;

    for(y=0;y<n_y;y++)
    {
        for(z=0;z<n_z;z++)
        {
            R = y_rotation(360.* y/N_Y) * z_rotation(360.* z/N_Z);
            samples.col(n) = R * true_vec;
            n++;
        }
    }

    return samples;
#else
    VectorXf x_rots(12);
    x_rots << 0, 0 , 180, 180, 270, 270, 270, 90 , 0  , 0  , 0  , 0;
    VectorXf y_rots(12);
    y_rots << 0, 0 , 90 , 90 , 0  , 0  , 180, 0  , 270, 270, 90 , 90;
    VectorXf z_rots(12);
    z_rots << 0, 90, 0  , 45 , 270, 0  , 0  , 225, 90 , 180, 180, 225;

    Matrix3f Rx, Ry, Rz, R;
    MatrixXf samples = MatrixXf::Zero(3,12);

    for (int n=0; n<12; n++)
    {
        Rx = x_rotation(x_rots(n));
        Ry = y_rotation(y_rots(n));
        Rz = z_rotation(z_rots(n));
        R = Rx * Ry * Rz;
        samples.col(n) = R * true_vec;
    }
    return samples;
#endif
}

MatrixXf generateInertialAlignData(MatrixXf true_data, Matrix3f Tm, Vector3f hm)
{
    int n = true_data.cols();
    MatrixXf samples = MatrixXf::Zero(3,N_SAMPLES);
    std::default_random_engine generator;
    std::normal_distribution<float> distribution(0,0.01);
    Vector3f noise;
    for(int i=0;i<n;i++)
    {
        for(int j=0;j<(int)(N_SAMPLES/12);j++)
        {
            noise << distribution(generator),distribution(generator),distribution(generator);
            //std::cout << "i: " << i << "\n" << true_data.col(i) << "\n\n";
            samples.col(i*N_SAMPLES/12+j) = Tm * true_data.col(i) + hm + noise;
        }
    }
    return samples;
}


//MatrixXf generateLaserAlignData()
//{
//    // Define n-rotations
//    const int n_rots = 8;
//
//    // Define characteristics of laser
//    const int true_distance = 10;
//    const float disto_len = 0.1;
//
//    // Rotate disto
//    Matrix<float,4,n_rots> out;
//
//    std::default_random_engine generator;
//    std::normal_distribution<float> theta_noise_dist(0,0.0001);
//    std::normal_distribution<float> distance_noise_dist(0,0.0001);
//
//    // Get g vector of the device
//    float theta = 0;
//    float alpha, beta, gamma, distance;
//
//    alpha = COMPOUND_OFFSET;
//    gamma = asin((disto_len * sin(alpha)/true_distance));
//    beta = M_PI - gamma;
//    distance = (true_distance * sin(beta))/sin(alpha);
//
//    for (int i=0;i<n_rots;i++) {
//        theta = 2*M_PI / n_rots * i + theta_noise_dist(generator);
//        out.col(i) <<  x_rotation(Rad2Deg(theta)) * y_rotation(Rad2Deg(COMPOUND_OFFSET)) * Vector3f{0, 0, 1}, distance * (1+distance_noise_dist(generator));
//    }
//    return out;
//}

MatrixXf generateLaserAlignData()
{
    // ---------------------------------------- Define needed parameters ---------------------------------------
    float initial_roll, combined_error;
    Vector3f laser_vec;

    const int n_rots = 8;
    const Vector3f target{1,1,1};
    const float disto_len = 0.1;

    const Vector3f x_ax = Vector3f(1,0,0);
    const Vector3f y_ax = Vector3f(0,1,0);
    const Vector3f z_ax = Vector3f(0,0,1);

    // ---------------------------------------- Find parameters of laser ---------------------------------------
    laser_vec = Vector3f(1,0,0);
    laser_vec = arbitrary_rotation(laser_vec, y_ax, INCLINATION_OFFSET);
    laser_vec = arbitrary_rotation(laser_vec, z_ax, HEADING_OFFSET);
    combined_error = acos(laser_vec.dot(x_ax));
    initial_roll = -M_PI_2 + atan2(laser_vec(2),laser_vec(1));

    cout << "Laser vec: " << laser_vec(0) << "  " << laser_vec(1) << "  " << laser_vec(2) << "\n";
    cout << "Combined error: " << combined_error << "\n";
    cout << "Initial roll: " <<  Rad2Deg(initial_roll) << "\n";


    // ------------------------------------ Find initial position of device ------------------------------------
    float  target_len, alpha, beta, gamma, theta;
    Vector3f initial_disto_tip;
    target_len = target.norm();
    theta = combined_error;
    alpha = M_PI - theta;
    beta = asin(disto_len * sin(alpha)/target_len);
    gamma = theta-beta;
    cout << "gamma: " << Rad2Deg(gamma) << "\n";

    initial_disto_tip = arbitrary_rotation(target/target.norm() * disto_len,target.cross(z_ax),gamma);


    // ------------------------------------ Rotate device about target axis ------------------------------------
    float phi, heading, inclination, roll;
    Vector3f disto_tip;
    Matrix<float,4,n_rots> out_hird;

    for (int i=0; i<n_rots; i++)
    {
        phi = i * 2*M_PI/n_rots;
        roll = initial_roll + phi;
        disto_tip = arbitrary_rotation(initial_disto_tip,target,phi);
        laser_vec = target - disto_tip;
        cout << "Disto tip: " << disto_tip(0) << "  " << disto_tip(1) << " " << disto_tip(2) << "\n";


        heading = atan2(disto_tip(1), disto_tip(0));
        inclination = atan2(pow( pow(disto_tip(0),2) + pow(disto_tip(1),2), 0.5),disto_tip(2));
//        out_hird.col(i) << disto_tip, 1;
         out_hird.col(i) << heading , inclination, roll, laser_vec.norm();
    }

    return out_hird;
}
#endif //MAGNETOMETER_CALIBRATION_DATA_GENERATION_H
