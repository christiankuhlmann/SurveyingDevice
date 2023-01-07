#include "sensorhandler.h"

void SensorHandler::update()
{
    Matrix<double,3,SAMPLING_SIZE> accel_samples;
    Matrix<double,3,SAMPLING_SIZE> mag_samples;
    int sample_num;

    // get an average of SAMPLING_SIZE samples for accuracy
    for (sample_num=0;sample_num<SAMPLING_SIZE;sample_num++)
    {
        accel_sensor->update();
        mag_sensor->update();
        accel_samples.col(sample_num) = accel_sensor->get_gravity_unit_vec();
        mag_samples.col(sample_num) = mag_sensor->get_mag_unit_vec();
    }
    mag_data = mag_samples.rowwise().mean();
    grav_data= accel_samples.rowwise().mean();

    // get lidar distance measurement
    distance = lidar_sensor->get_measurement();
    // Convert data to orientation
    get_orientation();
}

// Returns heading and inclination
// https://arduino.stackexchange.com/a/88707
void SensorHandler::get_orientation()
{
    Vector3d z_axis;
    double dot_prod;
    double scaling;

    Vector3d grav_data = accel_sensor->get_gravity_unit_vec();
    Vector3d mag_data = mag_sensor->get_mag_unit_vec();

    z_axis << 0,0,1;
    dot_prod = grav_data.dot(z_axis);
    scaling = z_axis.norm() * grav_data.norm();
	inclination =  RAD_TO_DEG * acos(dot_prod/scaling) + inclination_correction;

 
    Vector3d vector_north = mag_data - ((mag_data.dot(grav_data) / grav_data.dot(grav_data)) * grav_data);
    heading =  RAD_TO_DEG * atan2(vector_north(1), vector_north(0)) + heading_correction;

}

SensorHandler::SensorHandler(Accelerometer* accel, Magnetometer* mag, Lidar* lidar)
{
    accel_sensor = accel;
    mag_sensor = mag;
    lidar_sensor = lidar;
    inclination_correction = 0;
    inclination_correction = 0;
    calibration_num = 0;

    // Add code to load/save sensor calibration data from file
}

bool SensorHandler::calibrate()
{
    if (calibration_num < 8)
    {
        Serial.printf("Got calibration %i!\n",calibration_num);
        calibration_num++;
        this->update();

        calibraion_tilt_vecs.col(calibration_num) = grav_data;
        calibration_distances[calibration_num] = distance;

        return false;
    } else {
        Vector3d norm = calc_normal_vec(calibraion_tilt_vecs);
        Vector3d true_vec = calc_true_vec(norm,calibration_distances);
        Vector2d head_inc_corr = get_inclination_heading(true_vec);
        heading_correction = head_inc_corr[0];
        inclination_correction = head_inc_corr[1];
        return true;
    }
}

double SensorHandler::get_inclination()
{
    return inclination;
}
double SensorHandler::get_heading()
{
    return heading;
}
double SensorHandler::get_distance()
{
    return distance;
}