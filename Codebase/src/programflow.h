#include <waveshareoled.h>
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

enum LoadingEnum
{
    collecting_data,
    calculating,
    calibrating,
    aligning
};

void  laserOn()
{
    sc_laser.toggleLaser(true);
}

void  laserOff()
{
    sc_laser.toggleLaser(false);
}

void  takeShot()
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

void clearCalibration()
{
    sh.resetCalibration();
}

void testFunc()
{
    sh.update();
    dh.clearDisplay();
    dh.drawHeading(10.5);
    dh.drawInclination(-168.2);
    dh.drawDistance(8.3);
    dh.update();

    Driver_Delay_ms(5000); 

    dh.clearDisplay();
    dh.displayStaticCalib(OLED::CompassDirection::NORTH, OLED::CompassDirection::UP,"1/12");
    dh.update();

    Driver_Delay_ms(5000); 

    dh.clearDisplay();
    dh.displayLaserCalib(120, "6/8");
    dh.update();

    Driver_Delay_ms(5000); 

    for (int i=0; i<20; i++)
    {
        dh.clearDisplay();
        dh.displayLoading("Gathering", "Data", i);
        dh.update();
        Driver_Delay_ms(500);
    }
    

    Driver_Delay_ms(5000); 

    dh.clearDisplay();
    dh.drawBattery(5);
    dh.update();

    Driver_Delay_ms(2000); 
    dh.clearDisplay();
    dh.drawBattery(15);
    dh.update();

    Driver_Delay_ms(2000); 
    dh.clearDisplay();
    dh.drawBattery(45);
    dh.update();

    Driver_Delay_ms(2000); 
    dh.clearDisplay();
    dh.drawBattery(65);
    dh.update();

    Driver_Delay_ms(2000); 
    dh.clearDisplay();
    dh.drawBattery(85);
    dh.update();
    Driver_Delay_ms(2000); 
    
}


void displayIdle()
{
    // sh.update();
    // dh.clearDisplay();
    // dh.drawHeading(sh.getShotData().HIR(0));
    // dh.drawInclination(sh.getShotData().HIR(1));

    testFunc();


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

}

void displayCalibSaveYN()
{

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
        dh.displayLoading("Collecting","data...",count);
        break;
    }
    dh.update();
}

void initDisplayHandler()
{
    dh.init();
    dh.clearDisplay();
    dh.update();
    
}