#ifndef H_PROGRAMFLOW
#define H_PROGRAMFLOW

#define VBATPIN A6

#include <SensorHandler.h>
#include "RM3100SensorConnection.h"
#include "SCA3300SensorConnection.h"
#include "LDK2MSensorConnection.h"
#include "config.h"
#include "display_funcs.h"

static RM3100 rm3100;
static SCA3300 sca3300;
static LDK_2M ldk2m;

static SCA3300SensorConnection sc_accelerometer(sca3300);
static RM3100SensorConnection sc_magnetometer(rm3100);
static LDK2MSensorConnection sc_laser(ldk2m);

static SensorHandler sh(sc_accelerometer, sc_magnetometer, sc_laser);

static OLED::DisplayHandler dh;

static bool y_n_selector = true;

enum LoadingEnum
{
    collecting_data,
    calib_stabilising,
    calculating,
    calibrating,
    aligning
};


int getBatteryVoltage();

void laserOn();

void laserOff();

void laserBeep();

void takeShot();

int getCalib();

void saveCalib();

void removePreviosCalib();

void clearCalibration();

void testFunc();

void displayIdle();

void displayHistory();

void displayCalibSaveYN();

void displayCalibExitYN();

void displayStaticCalib(int n_calib);

void displayLaserCalib(int n_calib);

void displayLoading(LoadingEnum loading_type);

void initDisplayHandler();

void executeMenuAction(OLED::MenuEnum state);

// #endif

int getBatteryVoltage()
{
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    
    if (measuredvbat >= 3.98) {
        return 100; // Between 75% and 100%
    } else if (3.98 > measuredvbat >= 3.84) {
        return 70; // Between 50% and 75%
    } else if (3.84 > measuredvbat >= 3.75) {
        return 40; // Between 25% and 50%
    } else if (3.75 > measuredvbat >= 3.69) {
        return 20; // Between 25% and 50%
    } else {
        return 5;
    }
}

void laserOn()
{
    sc_laser.toggleLaser(true);
}

void laserOff()
{
    sc_laser.toggleLaser(false);
}

void laserBeep()
{
    sc_laser.beep();
}

void takeShot()
{
    sc_laser.beep();
    sh.takeShot();
}

int getCalib()
{
    if (sh.getCalibProgress() < N_ORIENTATIONS)
    {
        sh.collectStaticCalibData();
    } else if (sh.getCalibProgress() < (N_ORIENTATIONS + N_LASER_CAL))
    {
        sh.collectLaserCalibData();
    } else
    {
        sh.calibrate();
        sh.align();
    }
    return sh.getCalibProgress();
}

void saveCalib()
{
    sh.saveCalibration();
}

void removePreviosCalib()
{
    sh.removePrevCalib((sh.getCalibProgress() <= N_ORIENTATIONS));
}

void clearCalibration()
{
    sh.resetCalibration();
}

void loadCalibration()
{
    sh.loadCalibration();
}

void testFunc()
{
    // Serial.println("Getting accel measurement...");
    // sc_accelerometer.getMeasurement();

    // Serial.println("Updating sensorhandler...");
    // sh.update();
    // dh.clearDisplay();
    // dh.drawHeading(10.5);
    // dh.drawInclination(-168.2);
    // dh.drawDistance(8.3);
    // dh.update();

    // Driver_Delay_ms(5000); 

    // dh.clearDisplay();
    // dh.displayStaticCalib(OLED::CompassDirection::NORTH, OLED::CompassDirection::UP,"1/12");
    // dh.update();

    // Driver_Delay_ms(5000); 

    // dh.clearDisplay();
    // dh.displayLaserCalib(120, "6/8");
    // dh.update();

    // Driver_Delay_ms(5000); 

    // for (int i=0; i<20; i++)
    // {
    //     dh.clearDisplay();
    //     dh.displayLoading("Gathering", "Data", i);
    //     dh.update();
    //     Driver_Delay_ms(500);
    // }
    

    // Driver_Delay_ms(5000); 

    // dh.clearDisplay();
    // dh.drawBattery(5);
    // dh.update();

    // Driver_Delay_ms(2000); 
    // dh.clearDisplay();
    // dh.drawBattery(15);
    // dh.update();

    // Driver_Delay_ms(2000); 
    // dh.clearDisplay();
    // dh.drawBattery(45);
    // dh.update();

    // Driver_Delay_ms(2000); 
    // dh.clearDisplay();
    // dh.drawBattery(65);
    // dh.update();

    // Driver_Delay_ms(2000); 
    // dh.clearDisplay();
    // dh.drawBattery(85);
    // dh.update();
    // Driver_Delay_ms(2000); 

    dh.clearDisplay();
    y_n_selector = true;
    displayCalibSaveYN();
    dh.update();
    Driver_Delay_ms(2000); 

    dh.clearDisplay();
    y_n_selector = false;
    displayCalibSaveYN();
    dh.update();
    Driver_Delay_ms(2000); 

    dh.clearDisplay();
    y_n_selector = true;
    displayCalibExitYN();
    dh.update();
    Driver_Delay_ms(2000); 

    dh.clearDisplay();
    y_n_selector = false;\
    displayCalibExitYN();
    dh.update();
    Driver_Delay_ms(2000); 
    
}

void displayBatteryStatus()
{
    dh.drawBattery(getBatteryVoltage());
}

void displayMode()
{

}

void displayIdle()
{
    sh.update();
    dh.clearDisplay();
    dh.drawHeading(RAD_TO_DEG * sh.getShotData().HIR(0));
    dh.drawInclination(RAD_TO_DEG * sh.getShotData().HIR(1));
    displayBatteryStatus();
    dh.update();

    // testFunc();


    // dummy_value++;
    // dh.drawHeading(sh.getDirection);
    // dh.drawInclination(dummy_value);
    // dh.update();
    // Driver_Delay_ms(500); 


    // dh.clearDisplay();
    // dummy_float += 45;
    // dummy_float = fmod(dummy_float,360);
    // dh.displayLaserCalib(DEG_TO_RAD*dummy_float, "3/12");
    // dh.update();
    // Driver_Delay_ms(1000); 

    // dh.clearDisplay();  
    // dh.displayStaticCalib(OLED::CompassDirection::EAST,OLED::CompassDirection::NORTH,"3/12");
    // dh.update();
    // Driver_Delay_ms(1000); 
}

void displayHistory()
{
    dh.clearDisplay();
    displayBatteryStatus();
    dh.update();
}

void displayCalibSaveYN()
{
    dh.clearDisplay();
    dh.displayYN("Save", "calib?", y_n_selector);
    displayBatteryStatus();
    dh.update();
}

void displayCalibRemYN()
{
    dh.clearDisplay();
    dh.displayYN("Remove", "recent?", y_n_selector);
    displayBatteryStatus();
    dh.update();
}

void displayCalibExitYN()
{
    dh.clearDisplay();
    dh.displayYN("Exit", "calib?", y_n_selector);
    displayBatteryStatus();
    dh.update();
}

void displayStaticCalib(int n_calib)
{
    dh.clearDisplay();
    switch(n_calib)
    {
        case 0:
        dh.displayStaticCalib(OLED::CompassDirection::SOUTH,OLED::CompassDirection::UP,"1/12");
        break;

        case 1:
        dh.displayStaticCalib(OLED::CompassDirection::WEST,OLED::CompassDirection::UP,"2/12");
        break;

        case 2:
        dh.displayStaticCalib(OLED::CompassDirection::WEST,OLED::CompassDirection::DOWN,"3/12");
        break;

        case 3:
        dh.displayStaticCalib(OLED::CompassDirection::NORTH_WEST,OLED::CompassDirection::DOWN,"4/12");
        break;

        case 4:
        dh.displayStaticCalib(OLED::CompassDirection::EAST,OLED::CompassDirection::SOUTH,"5/12");
        break;

        case 5:
        dh.displayStaticCalib(OLED::CompassDirection::SOUTH,OLED::CompassDirection::WEST,"6/12");
        break;

        case 6:
        dh.displayStaticCalib(OLED::CompassDirection::NORTH,OLED::CompassDirection::WEST,"7/12");
        break;

        case 7:
        dh.displayStaticCalib(OLED::CompassDirection::NORTH_EAST,OLED::CompassDirection::NORTH_WEST,"8/12");
        break;

        case 8:
        dh.displayStaticCalib(OLED::CompassDirection::UP,OLED::CompassDirection::EAST,"9/12");
        break;

        case 9:
        dh.displayStaticCalib(OLED::CompassDirection::UP,OLED::CompassDirection::SOUTH,"10/12");
        break;

        case 10:
        dh.displayStaticCalib(OLED::CompassDirection::DOWN,OLED::CompassDirection::NORTH,"11/12");
        break;

        case 11:
        dh.displayStaticCalib(OLED::CompassDirection::DOWN,OLED::CompassDirection::NORTH_EAST,"12/12");
        break;
    }
    displayBatteryStatus();
    dh.update();
}

void displayLaserCalib(int n_calib)
{
    dh.clearDisplay();
    switch (n_calib)
    {
        case 0:
        dh.displayLaserCalib(0, "1/8");
        break;
        
        case 1:
        dh.displayLaserCalib(45, "2/8");
        break;
                
        case 2:
        dh.displayLaserCalib(90, "3/8");
        break;
                
        case 3:
        dh.displayLaserCalib(135, "4/8");
        break;
                
        case 4:
        dh.displayLaserCalib(180, "4/8");
        break;
                
        case 5:
        dh.displayLaserCalib(225, "6/8");
        break;
                
        case 6:
        dh.displayLaserCalib(270, "7/8");
        break;
                        
        case 7:
        dh.displayLaserCalib(315 ,"8/8");
        break;
    }
    displayBatteryStatus();
    dh.update();
}

void displayLoading(LoadingEnum loading_type)
{
    static int count = 0;
    count += 1;
    dh.clearDisplay();
    switch (loading_type)
    {
    case collecting_data:
        dh.displayLoading("Gathering","data...",count);
        break;
    
    case calib_stabilising:
        dh.displayLoading("Waiting","3s...",count);
        break;
    }
    displayBatteryStatus();
    dh.update();
}

void initDisplayHandler()
{
    dh.init();
    dh.clearDisplay();
    dh.update();
    
}

void displayMenu(OLED::MenuEnum state)
{
    dh.clearDisplay();
    dh.displayMenu(state);
    displayBatteryStatus();
    dh.update();
}

void executeMenuAction(OLED::MenuEnum menu_action)
{
    switch (menu_action)
    {
    case OLED::MenuEnum::MENU_DUMP_DATA:
        sh.dumpCalibToSerial();
    break;
    
    default:
        break;
    }
}

#endif