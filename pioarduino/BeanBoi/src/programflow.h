#ifndef H_PROGRAMFLOW
#define H_PROGRAMFLOW

#define VBATPIN A6

#include <SensorHandler.h>
#include "RM3100SensorConnection.h"
#include "SCA3300SensorConnection.h"
#include "LDK2MSensorConnection.h"
#include "config.h"
#include "display_funcs.h"

// External declarations for global objects
extern RM3100 rm3100;
extern SCA3300 sca3300;
extern LDK_2M ldk2m;

extern SCA3300SensorConnection sc_accelerometer;
extern RM3100SensorConnection sc_magnetometer;
extern LDK2MSensorConnection sc_laser;

extern SensorHandler sh;
extern OLED::DisplayHandler dh;
extern bool y_n_selector;
extern int history_scroll_index;
extern unsigned int current_file_id;

enum LoadingEnum
{
    collecting_data,
    calib_stabilising,
    calculating,
    calibrating,
    aligning
};

/************************************************************************************************
 *                                      Function prototypes
 ************************************************************************************************/
int getBatteryVoltage();

void laserOn();
void laserOff();
void laserBeep();

int takeShot();
int getCalib();

void saveCalib();
void removePreviosCalib();
void clearCalibration();
void loadCalibration();

void testFunc();

void displayIdle();
void displayHistory();
void displayBatteryStatus();
void displayMode();

void displayCalibSaveYN();
void displayCalibRemYN();
void displayCalibExitYN();
void displayCalibrationQuality();

void displayStaticCalib(int n_calib);
void displayLaserCalib(int n_calib);
void displayLoading(LoadingEnum loading_type);

void initDisplayHandler();
void displayMenu(OLED::MenuEnum state);

void executeMenuAction(OLED::MenuEnum state);
void runCalibration();

#endif