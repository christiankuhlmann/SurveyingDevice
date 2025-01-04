#include "SensorHandler.h"
#include <queue>
#include <ArduinoJson.h>


typedef Matrix<float, Dynamic, Dynamic, RowMajor> RowMatrixXf;
static StaticJsonDocument<60480> root;
static float eig_arr[3][N_ALIGN_MAG_ACC];
static float las_arr[3][N_LASER_CAL];

static unsigned int counter;

void getFileName(const unsigned int fileID, char (&fname)[FNAME_LENGTH])
{
    sprintf(fname,"SD%03u\0", fileID);
}
void getVarName(const unsigned int counter, char (&varname)[VARNAME_LENGTH])
{
    sprintf(varname,"%03u\0", counter+1);
}

bool getCounter(const unsigned int fileID, unsigned int &counter)
{
    char fname[FNAME_LENGTH];
    getFileName(fileID, fname);
    // If file and varname exist ...
    if (FileFuncs::readFromFile(fname,"counter",counter)){
        return true;
    }
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Counter not found...");
    return false;
}
void setCounter(const unsigned int fileID, const unsigned int &counter)
{
    char fname[FNAME_LENGTH];
    getFileName(fileID, fname);
    FileFuncs::writeToFile(fname,"counter",counter);
}

void saveShotData(const ShotData &sd, const unsigned int fileID)
{
    char fname[FNAME_LENGTH];
    char varname[VARNAME_LENGTH];
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Saving shot data to file...");
    getFileName(fileID,fname);
    Serial.printf("Namespace to write to: %s\n", fname);


    // If file exists doesn't exist, create it with counter = 0
    if (!getCounter(fileID, counter))
    {
        Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"File doesn't exist, creating new one...");
        counter = 0;
        setCounter(fileID, counter);
    } // Get latest shot ID

    // Save shot to file with ID = counter
    Serial.printf("Counter to write to: %u\n", counter);
    getVarName(counter+1,varname);
    setCounter(fileID,counter+1);
    Serial.printf("Key to write to: %s\n", varname);

    FileFuncs::writeToFile(fname,varname,&sd,sizeof(ShotData)); // Save the shot
}
bool readShotData(ShotData &sd, unsigned int fileID, unsigned int shotID)
{
    char fname[FNAME_LENGTH];
    char varname[VARNAME_LENGTH];
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Reading shot data from file...");
    getFileName(fileID,fname);
    getVarName(shotID,varname);
    return FileFuncs::readFromFile(fname,varname,&sd,sizeof(ShotData));
}

bool readShotData(ShotData &sd, unsigned int fileID)
{
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Reading latest shot data from file...");
    getCounter(fileID, counter); // Get latest shot ID
    return readShotData(sd,fileID,counter);
}

// IMPORTANT: References are immutable and must be definied upon initialisation!
SensorHandler::SensorHandler(Accelerometer &a, Magnetometer &m, Laser &l):acc(a), mag(m), las(l){}

void SensorHandler::init()
{
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR, "Acc init...");
    acc.init();
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR, "Mag init...");
    mag.init();
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR, "Las init...");
    las.init();

    Debug_csd::debug(Debug_csd::DEBUG_SENSOR, "Loading calibration...");
    resetCalibration();
    loadCalibration();
}

void SensorHandler::resetCalibration()
{
    calib_parms.Ra_cal.setIdentity();
    calib_parms.Rm_cal.setIdentity();
    calib_parms.Ra_las.setIdentity();
    calib_parms.Rm_las.setIdentity();
    calib_parms.Rm_align.setIdentity();
    calib_parms.ba_cal.setZero();
    calib_parms.bm_cal.setZero();
    calib_parms.inclination_angle = 0;
    static_calib_progress = 0;
    las_calib_progress = 0;
}

void SensorHandler::update()
{
    mag_data << 0,0,0;
    acc_data << 0,0,0;
    for (int i=0; i<N_UPDATE_SAMPLES; i++)
    {
        mag_data += mag.getMeasurement();
        acc_data += acc.getMeasurement();
    }
    mag_data /= N_UPDATE_SAMPLES;
    acc_data /= N_UPDATE_SAMPLES;

    // Store corrected data in corrected_acc_data and corrected_mag_data
    correctData(corrected_shot_data.m, corrected_shot_data.g);


    Matrix3f ENU;
    ENU = NumericalMethods::inertialToENU(corrected_shot_data.m,corrected_shot_data.g);
    corrected_shot_data.HIR = NumericalMethods::inertialToCardan(corrected_shot_data.m,corrected_shot_data.g);
    corrected_shot_data.v = NumericalMethods::inertialToVector(corrected_shot_data.m,corrected_shot_data.g);

    Serial.println("");
    Serial.printf("Raw acc data: X %f   Y %f   Z %f   Norm: %f\n", acc_data(0), acc_data(1), acc_data(2), acc_data.norm());
    Serial.printf("Raw mag data: X %f   Y %f   Z %f   Norm: %f\n", mag_data(0), mag_data(1), mag_data(2), mag_data.norm());
    
    Serial.println("");
    Serial.printf("Corrected acc data: X %f   Y %f   Z %f   Norm: %f\n", corrected_shot_data.g(0), corrected_shot_data.g(1), corrected_shot_data.g(2), corrected_shot_data.g.norm());
    Serial.printf("Corrected mag data: X %f   Y %f   Z %f   Norm: %f\n", corrected_shot_data.m(0), corrected_shot_data.m(1), corrected_shot_data.m(2), corrected_shot_data.m.norm());
    
    Serial.println("");
    Serial.printf("ENU data: \nE: %f %f %f\nN: %f %f %f\nU: %f %f %f\n",
    ENU.col(0)(0), ENU.col(0)(1), ENU.col(0)(2),
    ENU.col(1)(0), ENU.col(1)(1), ENU.col(1)(2),
    ENU.col(2)(0), ENU.col(2)(1), ENU.col(2)(2));

    Serial.println("");
    Serial.printf("HIR data: H %f   I %f   R %f\n", corrected_shot_data.HIR(0), corrected_shot_data.HIR(1), corrected_shot_data.HIR(2));
    Serial.println("");
}

Vector3f SensorHandler::getCardan(bool corrected)
{
    if (corrected) 
    {
        return NumericalMethods::inertialToCardan(corrected_acc_data, corrected_mag_data);
    }   else    {
        return NumericalMethods::inertialToCardan(acc_data, mag_data);
    }
}
Vector3f SensorHandler::getCartesian(bool corrected)
{
    if (corrected) 
    {
        return NumericalMethods::inertialToVector(corrected_acc_data, corrected_mag_data);
    }   else    {
        return NumericalMethods::inertialToVector(acc_data, mag_data);
    }
}
ShotData SensorHandler::getShotData(bool corrected)
{
    if (corrected) 
    {
        return corrected_shot_data;
    }   else    {
        return shot_data;
    }
}

int SensorHandler::takeShot(const bool laser_reading, const bool use_stabilisation)
{
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Starting to take shot...");
    if (use_stabilisation)
    {
        // Wait until device is steady
        Vector<float,N_STABILISATION> norm_buffer;
        for (int i=0; i<N_STABILISATION; i++)
        {
            norm_buffer(i) = acc.getMeasurement().norm();
        }

        int i = 0;
        while (NumericalMethods::stDev(norm_buffer) > STDEV_LIMIT)
        {
            norm_buffer(i%N_STABILISATION) = acc.getMeasurement().norm();
            i++;

            if (i > 1000) { return 1; }
        }
    }

    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Stabilised...");
    // Take samples
    mag_data << 0,0,0;
    acc_data << 0,0,0;
    for (int i=0; i<N_SHOT_SMAPLES; i++)
    {
        mag_data += mag.getMeasurement();
        acc_data += acc.getMeasurement();
    }
    mag_data /= N_SHOT_SMAPLES;
    acc_data /= N_SHOT_SMAPLES;
    Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Data collected...");

    
    if (laser_reading) { las_data = las.getMeasurement(); }
    if (las_data <= 0) {return 1;}

    // Update stored shot data
    shot_data.m = mag_data;
    shot_data.g = acc_data;
    shot_data.d = las_data;
    shot_data.HIR = NumericalMethods::inertialToCardan(shot_data.m,shot_data.g);
    shot_data.v = NumericalMethods::inertialToVector(shot_data.m,shot_data.g);

    // Save corrected shot data in corrected_shot_data
    correctData(corrected_shot_data.m, corrected_shot_data.g);
    corrected_shot_data.HIR = NumericalMethods::inertialToCardan(corrected_shot_data.m,corrected_shot_data.g);
    corrected_shot_data.v = NumericalMethods::inertialToVector(corrected_shot_data.m,corrected_shot_data.g);
    corrected_shot_data.d = shot_data.d + DEVICE_LENGTH;



    
    Debug_csd::debugf(Debug_csd::DEBUG_SENSOR,"Laser measurement: %f ", las_data);

    las.toggleLaser(true);

    return 0;
}

void SensorHandler::correctData(Vector3f &m, Vector3f &g)
{
    // Serial.printf("Rm_align: %f %f %f\n", calib_parms.Rm_align(0), calib_parms.Rm_align(1), calib_parms.Rm_align(2));
    // Serial.printf("Rm_las: %f %f %f\n", calib_parms.Rm_las(0), calib_parms.Rm_las(1), calib_parms.Rm_las(2));
    // Serial.printf("Rm_cal: %f %f %f\n", calib_parms.Rm_cal(0), calib_parms.Rm_cal(1), calib_parms.Rm_cal(2));

    // Apply sensor calibration
    m = calib_parms.Rm_cal * (mag_data - calib_parms.bm_cal);
    g = calib_parms.Ra_cal * (acc_data - calib_parms.ba_cal);

    // Normalise data
    m.colwise().normalize();
    g.colwise().normalize();

    // Apply alignment
    m = calib_parms.Rm_align * calib_parms.Rm_las * m; 
    g = calib_parms.Ra_las * g;

    // Apply final normalisation
    m.colwise().normalize();
    g.colwise().normalize();
}

void SensorHandler::eraseFlash()
{
    FileFuncs::erase_flash();
}
void SensorHandler::getFlashStats()
{
    FileFuncs::getStatus();
}

int SensorHandler::collectStaticCalibData()
{
    if (static_calib_progress >= N_ORIENTATIONS) { return N_ORIENTATIONS; }

    int index = 0;
    int n_avg = 15;
    Vector3f g, m;
    for (int i=0; i<N_SAMPLES_PER_ORIENTATION;i++)
    {
        index = static_calib_progress*N_SAMPLES_PER_ORIENTATION+i;

        g.setZero();
        m.setZero();

        for (int j=0; j<n_avg;j++)
        {
            g += acc.getMeasurement();
            m += mag.getMeasurement();
            // Serial.println(j);
        }
        static_calib_data.acc_data.col(index) = g/n_avg;
        static_calib_data.mag_data.col(index) = m/n_avg;
    }

    // takeShot(false);
    // mag_align_data.col(mag_acc_align_progress) = getMagData();
    // acc_align_data.col(mag_acc_align_progress) = getAccData();
    static_calib_progress++;
    Debug_csd::debugf(Debug_csd::DEBUG_SENSOR,"Static calibration progress %i/%i", static_calib_progress, N_ORIENTATIONS);
    return static_calib_progress;

}
int SensorHandler::collectLaserCalibData()
{
    if (las_calib_progress >= N_LASER_CAL) { return N_LASER_CAL; }

    if (takeShot()) {
        Debug_csd::debug(Debug_csd::DEBUG_SENSOR,"Shot failed! Try again.");
        return las_calib_progress;
    }

    laser_calib_data.acc_data.col(las_calib_progress) = acc_data;
    laser_calib_data.mag_data.col(las_calib_progress) = mag_data;
    
    las_calib_progress++;
    Debug_csd::debugf(Debug_csd::DEBUG_SENSOR,"Laser calibration progress %i/%i", las_calib_progress, N_LASER_CAL);
    return las_calib_progress;
}

int SensorHandler::calibrate()
{
    Vector<float,10> Um = NumericalMethods::fitEllipsoid(static_calib_data.mag_data);
    NumericalMethods::calculateEllipsoidTransformation(Um, calib_parms.Rm_cal, calib_parms.bm_cal);

    Vector<float,10> Ua = NumericalMethods::fitEllipsoid(static_calib_data.acc_data);
    NumericalMethods::calculateEllipsoidTransformation(Ua, calib_parms.Ra_cal, calib_parms.ba_cal);
    
    return 0;
}
int SensorHandler::align()
{

    // ------------------------------ Run laser alignment ------------------------------
    // Using collected data, apply calibration corrections
    Matrix<float,3,N_LASER_CAL> temp_acc_data = calib_parms.Ra_cal * (laser_calib_data.acc_data.colwise() - calib_parms.ba_cal);
    Matrix<float,3,N_LASER_CAL> temp_mag_data = calib_parms.Rm_cal * (laser_calib_data.mag_data.colwise() - calib_parms.bm_cal);

    // Normalise data as calibration should lead to normalised data
    temp_acc_data.colwise().normalize();
    temp_mag_data.colwise().normalize();

    // Run laser alignment
    NumericalMethods::alignToNorm(temp_acc_data, calib_parms.Ra_las);
    NumericalMethods::alignToNorm(temp_mag_data, calib_parms.Rm_las);

    // ------------------------------ Run MAG.I.CAL alignment ------------------------------
    static Matrix<float,3,N_ALIGN_MAG_ACC> acc_magical_data, mag_magical_data;

    // Apply calibration correction
    acc_magical_data = calib_parms.Ra_cal * (static_calib_data.acc_data.colwise() - calib_parms.ba_cal);
    mag_magical_data = calib_parms.Rm_cal * (static_calib_data.mag_data.colwise() - calib_parms.bm_cal);

    // Normalise data
    acc_magical_data.colwise().normalize();
    mag_magical_data.colwise().normalize();

    // Apply laser alignment
    acc_magical_data = calib_parms.Ra_las * acc_magical_data;
    mag_magical_data = calib_parms.Rm_las * mag_magical_data;
    
    // Calculate MAG.I.CAL alignment
    NumericalMethods::alignMagAcc(
        acc_magical_data, 
        mag_magical_data, 
        calib_parms.Rm_align, calib_parms.inclination_angle);

    Vector3f temp1, temp2;
    // temp1 = NumericalMethods::normalVec(temp_acc_data);
    // temp2 = NumericalMethods::normalVec(calib_parms.Ra_las * temp_acc_data);
    // Serial.printf("Acc Matrix: \n %f %f %f \n %f %f %f \n %f %f %f \n\n", 
    // calib_parms.Ra_las(0,0), calib_parms.Ra_las(0,1), calib_parms.Ra_las(0,2),
    // calib_parms.Ra_las(1,0), calib_parms.Ra_las(1,1), calib_parms.Ra_las(1,2),
    // calib_parms.Ra_las(2,0), calib_parms.Ra_las(2,1), calib_parms.Ra_las(2,2));
    // Serial.printf("Original acc norm: %f %f %f \n", temp1(0), temp1(1), temp1(2));
    // Serial.printf("Corrected acc norm: %f %f %f \n", temp2(0), temp2(1), temp2(2));
    // Serial.println("");

    // temp1 = NumericalMethods::normalVec(temp_mag_data);
    // temp2 = NumericalMethods::normalVec(calib_parms.Rm_las * temp_mag_data);
    // Serial.printf("Mag Matrix: \n %f %f %f \n %f %f %f \n %f %f %f \n\n", 
    // calib_parms.Rm_las(0,0), calib_parms.Rm_las(0,1), calib_parms.Rm_las(0,2),
    // calib_parms.Rm_las(1,0), calib_parms.Rm_las(1,1), calib_parms.Rm_las(1,2),
    // calib_parms.Rm_las(2,0), calib_parms.Rm_las(2,1), calib_parms.Rm_las(2,2));
    // Serial.printf("Original mag norm: %f %f %f \n", temp1(0), temp1(1), temp1(2));
    // Serial.printf("Corrected mag norm: %f %f %f \n", temp2(0), temp2(1), temp2(2));
    // Serial.println("");

    temp1 = NumericalMethods::normalVec(temp_acc_data);
    temp2 = NumericalMethods::normalVec(temp_mag_data);
    Serial.printf("Corrected acc norm: %f %f %f \n", temp1(0), temp1(1), temp1(2));
    Serial.printf("Corrected mag norm: %f %f %f \n", temp2(0), temp2(1), temp2(2));

    temp1 = NumericalMethods::normalVec(calib_parms.Ra_las * temp_acc_data);
    temp2 = NumericalMethods::normalVec(calib_parms.Rm_align * calib_parms.Rm_las * temp_mag_data);

    if (temp2(0)<0) calib_parms.Rm_align = -1 * calib_parms.Rm_align;
    temp2 = NumericalMethods::normalVec(calib_parms.Rm_align * calib_parms.Rm_las * temp_mag_data);


    Serial.printf("Aligned acc norm: %f %f %f \n", temp1(0), temp1(1), temp1(2));
    Serial.printf("Aligned mag norm: %f %f %f \n", temp2(0), temp2(1), temp2(2));

    Serial.printf("Inclination angle: %f\n", calib_parms.inclination_angle);
    Serial.println("");

    Vector3f a, b;
    float temp;
    for (int i=0; i<N_ALIGN_MAG_ACC; i++)
    {
        a = calib_parms.Rm_align * mag_magical_data.col(i);
        b = acc_magical_data.col(i);
        temp = acos(a.dot(b) / (a.norm() * b.norm()));
        Serial.printf("Angle: %f\n", RAD_TO_DEG * temp);
    }

    // // ------------------------------ Second alignment ------------------------------
    // Matrix3f Ra_las2, Rm_las2, Rm_align2;
    // float inclination_angle2;

    // temp_acc_data = calib_parms.Ra_las * temp_acc_data;
    // temp_mag_data = calib_parms.Rm_align * calib_parms.Rm_las * temp_mag_data;
    // NumericalMethods::alignToNorm(temp_acc_data, Ra_las2);
    // NumericalMethods::alignToNorm(temp_mag_data, Rm_las2);

    // acc_magical_data = Ra_las2 * acc_magical_data;
    // mag_magical_data = Rm_las2 * calib_parms.Rm_align * mag_magical_data;
    // NumericalMethods::alignMagAcc(
    //     acc_magical_data, 
    //     mag_magical_data, 
    //     Rm_align2, inclination_angle2);

    // temp1 = NumericalMethods::normalVec(Ra_las2 * temp_acc_data);
    // temp2 = NumericalMethods::normalVec(Rm_align2 * Rm_las2 * temp_mag_data);
    // Serial.printf("Aligned acc norm2: %f %f %f \n", temp1(0), temp1(1), temp1(2));
    // Serial.printf("Aligned mag norm2: %f %f %f \n", temp2(0), temp2(1), temp2(2));
    // Serial.printf("Inclination angle2: %f\n", inclination_angle2);


    // for (int i=0; i<N_ALIGN_MAG_ACC; i++)
    // {
    //     a = Rm_align2 * mag_magical_data.col(i);
    //     b = acc_magical_data.col(i);
    //     temp = acos(a.dot(b) / (a.norm() * b.norm()));
    //     Serial.printf("Angle: %f\n", RAD_TO_DEG * temp);
    // }

    return 0;
}

void SensorHandler::saveCalibration()
{
    EigenFileFuncs::writeToFile("static_calib","acc_data", static_calib_data.acc_data);
    EigenFileFuncs::writeToFile("static_calib","mag_data", static_calib_data.mag_data);
    EigenFileFuncs::writeToFile("laser_calib","acc_data", laser_calib_data.acc_data);
    EigenFileFuncs::writeToFile("laser_calib","mag_data", laser_calib_data.mag_data);

    EigenFileFuncs::writeToFile("calib_parms","Ra_cal", calib_parms.Ra_cal);
    EigenFileFuncs::writeToFile("calib_parms","ba_cal", calib_parms.ba_cal);
    EigenFileFuncs::writeToFile("calib_parms","Rm_cal", calib_parms.Rm_cal);
    EigenFileFuncs::writeToFile("calib_parms","bm_cal", calib_parms.bm_cal);

    EigenFileFuncs::writeToFile("calib_parms","Ra_las", calib_parms.Ra_las);
    EigenFileFuncs::writeToFile("calib_parms","Rm_las", calib_parms.Rm_las);
    EigenFileFuncs::writeToFile("calib_parms","Rm_align", calib_parms.Rm_align);

}
void SensorHandler::loadCalibration()
{
    EigenFileFuncs::readFromFile("static_calib","acc_data", static_calib_data.acc_data);
    EigenFileFuncs::readFromFile("static_calib","mag_data", static_calib_data.mag_data);
    EigenFileFuncs::readFromFile("laser_calib","acc_data", laser_calib_data.acc_data);
    EigenFileFuncs::readFromFile("laser_calib","mag_data", laser_calib_data.mag_data);

    EigenFileFuncs::readFromFile("calib_parms","Ra_cal", calib_parms.Ra_cal);
    EigenFileFuncs::readFromFile("calib_parms","ba_cal", calib_parms.ba_cal);
    EigenFileFuncs::readFromFile("calib_parms","Rm_cal", calib_parms.Rm_cal);
    EigenFileFuncs::readFromFile("calib_parms","bm_cal", calib_parms.bm_cal);

    EigenFileFuncs::readFromFile("calib_parms","Ra_las", calib_parms.Ra_las);
    EigenFileFuncs::readFromFile("calib_parms","Rm_las", calib_parms.Rm_las);
    EigenFileFuncs::readFromFile("calib_parms","Rm_align", calib_parms.Rm_align);

}

void SensorHandler::removePrevCalib(bool static_calib)
{
    // To implement
}

int SensorHandler::getCalibProgress()
{
    return static_calib_progress + las_calib_progress;
}

int SensorHandler::getCalibProgress(bool static_calib)
{
    if (static_calib)
    {
        return static_calib_progress;
    } else {
        return las_calib_progress;
    }
    
}

Vector3f SensorHandler::getMagData()
{
    return mag_data;
}
Vector3f SensorHandler::getAccData()
{
    return acc_data;
}

const StaticCalibrationData& SensorHandler::getStaticCalibData()
{
    return static_calib_data;
}
const LaserCalibrationData& SensorHandler::getLaserCalibData()
{
    return laser_calib_data;
}
const DeviceCalibrationParameters& SensorHandler::getCalibParms()
{
    return calib_parms;
}

void SensorHandler::dumpCalibToSerial()
{
    root.clear();

    JsonArray jarr;
    MatrixXf static_acc_samples, static_mag_samples, laser_acc_samples, laser_mag_samples;

    JsonArray acc_data = root.createNestedArray("static_acc_samples");
    static_acc_samples = static_calib_data.acc_data;
    Map<RowMatrixXf>(&eig_arr[0][0], 3, N_ALIGN_MAG_ACC) = static_acc_samples; 
    copyArray(eig_arr,acc_data);
    Serial.print("static_acc_samples\n");
    // serializeJson(acc_data,Serial);

    JsonArray mag_data = root.createNestedArray("static_mag_samples");
    static_mag_samples = static_calib_data.mag_data;
    Map<RowMatrixXf>(&eig_arr[0][0], 3, N_ALIGN_MAG_ACC) = static_mag_samples; 
    copyArray(eig_arr,mag_data);
    Serial.print("static_mag_samples\n");
    // serializeJson(mag_data,Serial);

    JsonArray las_acc_data = root.createNestedArray("laser_acc_samples");
    laser_acc_samples = laser_calib_data.acc_data;
    Map<RowMatrixXf>(&las_arr[0][0], 3, N_LASER_CAL) = laser_acc_samples; 
    copyArray(las_arr,las_acc_data);
    Serial.print("laser_acc_samples\n");
    // serializeJson(las_acc_data,Serial);

    JsonArray las_mag_data = root.createNestedArray("laser_mag_samples");
    laser_mag_samples = laser_calib_data.mag_data;
    Map<RowMatrixXf>(&las_arr[0][0], 3, N_LASER_CAL) = laser_mag_samples; 
    copyArray(las_arr,las_mag_data);
    Serial.print("laser_mag_samples\n");
    serializeJson(las_mag_data,Serial);


    // JsonObject parms = root.createNestedObject("parms");
    JsonArray Ra_static = root.createNestedArray("Ra_static");
    JsonArray ba_static = root.createNestedArray("ba_static");
    JsonArray Rm_static = root.createNestedArray("Rm_static");
    JsonArray bm_static = root.createNestedArray("bm_static");
    JsonArray Ra_laser = root.createNestedArray("Ra_laser");
    JsonArray Rm_laser = root.createNestedArray("Rm_laser");
    JsonArray Rm_align = root.createNestedArray("Rm_align");


    float vec3fdata[3];
    float mat3fdata[3][3];

    Map<RowMatrixXf>(&mat3fdata[0][0], 3, 3) = calib_parms.Ra_cal; 
    copyArray(mat3fdata,Ra_static);
    Map<Vector3f>(&vec3fdata[0], 3) = calib_parms.ba_cal; 
    copyArray(vec3fdata,ba_static);


    Map<RowMatrixXf>(&mat3fdata[0][0], 3, 3) = calib_parms.Rm_cal; 
    copyArray(mat3fdata,Rm_static);
    Map<Vector3f>(&vec3fdata[0], 3) = calib_parms.bm_cal; 
    copyArray(vec3fdata,bm_static);


    Map<RowMatrixXf>(&mat3fdata[0][0], 3, 3) = calib_parms.Ra_las; 
    copyArray(mat3fdata,Ra_laser);

    Map<RowMatrixXf>(&mat3fdata[0][0], 3, 3) = calib_parms.Rm_las; 
    copyArray(mat3fdata,Rm_laser);


    Map<RowMatrixXf>(&mat3fdata[0][0], 3, 3) = calib_parms.Rm_align; 
    copyArray(mat3fdata,Rm_align);

    serializeJson(root,Serial);

}

