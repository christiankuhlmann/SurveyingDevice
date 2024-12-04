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


void displayIdle()
{
    sh.update();

    dh.clearDisplay();
    dh.drawHeading(sh.getShotData().HIR(0));
    dh.drawInclination(sh.getShotData().HIR(1));
    dh.update();
}

void initDisplayHandler()
{
    dh.init();
    dh.clearDisplay();
    dh.drawHeading(10.54);
    dh.drawInclination(-118.2);
    dh.update();
}