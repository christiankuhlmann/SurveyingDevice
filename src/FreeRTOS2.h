#ifndef HEADER_FREERTOS2
#define HEADER_FREERTOS2

#include <Arduino.h>
#include "soc/rtc_wdt.h"
#include "utils.h"
#include <debug.h>
#include <inttypes.h>

using namespace Debug;

/****************************************************************
 * Define Button values
 ****************************************************************/
#define ID_B1 0x11
#define ID_B2 0x12
#define ID_B3 0x13
#define ID_B4 0x14
#define ID_B5 0x15

#define ACTION_ON_LONG      ID_B1
#define ACTION_OFF_LONG     ID_B2
#define ACTION_UP_LONG      ID_B3
#define ACTION_DOWN_LONG    ID_B4
#define ACTION_MODE_LONG    ID_B5

#define ACTION_ON_SHORT     0x10 + ACTION_ON_LONG
#define ACTION_OFF_SHORT    0x10 + ACTION_OFF_LONG
#define ACTION_UP_SHORT     0x10 + ACTION_UP_LONG
#define ACTION_DOWN_SHORT   0x10 + ACTION_DOWN_LONG
#define ACTION_MODE_SHORT   0x10 + ACTION_MODE_LONG

/****************************************************************
 * Config for timing
 ****************************************************************/
const static int BTN_LONG_PRESS_MS = 2000;
const static int DISPLAY_HZ = 10;
const static float DISPLAY_PERIOD = 1.0/DISPLAY_HZ;
const static int DISPLAY_PRESCALAR = 80;
const static uint64_t DISPLAY_TICKS = DISPLAY_PERIOD *  APB_CLK_FREQ/DISPLAY_PRESCALAR;
static hw_timer_t *displayTimer_cfg = NULL;

/****************************************************************
 * FreeRTOS Task definitions
 ****************************************************************/
TaskHandle_t inputhandler_task;
TaskHandle_t computefunc_task;
TaskHandle_t displayhandler_task;

static uint32_t buttonNumber = 0x00;
static uint32_t inputhandlerNotifiedValue = 0x00;
static uint32_t computefuncNotifiedValue = 0x00;
static uint32_t displayhandlerNotifiedValue = 0x00;

/****************************************************************
 * Program flow enums
 ****************************************************************/
enum DisplayModeEnum
{
    DISP_IDLE,
    DISP_SHOT_TAKEN,
    DISP_HISTORY,
    DISP_MENU,

    DISP_CALIBRATION,
    DISP_LASER_CALIB,
    DISP_STATIC_CALIB,

    DISP_CALIB_SAVE,
    DISP_CALIB_REM
};

enum DeviceStateEnum
{
    MODE_IDLE,
    MODE_LASER_ON,
    MODE_SHOT_TAKEN,
    MODE_HISTORY,
    MODE_MENU,

    MODE_CALIB,
    MODE_CALIB_REM_YN,
    MODE_CALIB_SAVE_YN,

    MODE_BLUETOOTH,
    MODE_FILES,
    MODE_CONFIG
};

static DeviceStateEnum current_mode;
static DeviceStateEnum next_mode;
static DisplayModeEnum display_mode;


/****************************************************************
 * Interrupt handlers
 ****************************************************************/
void IRAM_ATTR displayTimerISR()
{
    xTaskNotify(displayhandler_task,0x00,eSetValueWithOverwrite);
}
void IRAM_ATTR B1Interrupt()
{
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B1,0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B2Interrupt()
{
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B2,0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B3Interrupt()
{
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B3,0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B4Interrupt()
{
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B4,0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B5Interrupt()
{
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B5,0x10,eSetValueWithOverwrite,NULL);
}


/****************************************************************
 * Extern function definitions
 ****************************************************************/
void extern laserOn();
void extern laserOff();
void extern takeShot();


/****************************************************************
 * Program flow FSM
 ****************************************************************/
void executeAction(const uint32_t action)
{
    switch (current_mode)
    {
        case MODE_IDLE:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_LASER_ON;
            display_mode = DISP_IDLE;
            // laserOn();
        } else if (action == ACTION_MODE_SHORT)
        {
            debug(DEBUG_ALWAYS, "Switching mode to: HISTORY");
            next_mode = MODE_HISTORY;
            display_mode = DISP_HISTORY;
        }
        break;

        case MODE_LASER_ON:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_LASER_ON;
            display_mode = DISP_SHOT_TAKEN;
            // takeShot();
        } else if (action == ACTION_OFF_SHORT) {
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
            // laserOff();
        } else if (action == ACTION_MODE_SHORT)
        {
            debug(DEBUG_ALWAYS, "Switching mode to: HISTORY");
            next_mode = MODE_HISTORY;
            display_mode = DISP_HISTORY;
        }
        break;

        case MODE_HISTORY:
        if (action == ACTION_MODE_SHORT){
            debug(DEBUG_ALWAYS, "Switching mode to: MENU");
            next_mode = MODE_MENU;
            display_mode = DISP_MENU;
        }
        break;

        case MODE_MENU:
        if (action == ACTION_MODE_SHORT){
            debug(DEBUG_ALWAYS, "Switching mode to: CALIB");
            next_mode = MODE_CALIB;
            display_mode = DISP_CALIBRATION;
        }
        break;

        case MODE_CALIB:
        if (action == ACTION_MODE_SHORT){
            debug(DEBUG_ALWAYS, "Switching mode to: IDLE");
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
        }
        break;

        case MODE_CALIB_REM_YN:
        break;

        case MODE_CALIB_SAVE_YN:
        break;
    }
    current_mode = next_mode;
}



void updateDisplay()
{
    xTaskNotifyFromISR( displayhandler_task,
                        0x00,
                        eSetValueWithOverwrite, 
                        NULL);
}

void startDisplayTimer()
{
    displayTimer_cfg = timerBegin(0, DISPLAY_PRESCALAR, true);
    timerAttachInterrupt(displayTimer_cfg, &displayTimerISR, true);
    timerAlarmWrite(displayTimer_cfg, DISPLAY_TICKS, true);
    timerAlarmEnable(displayTimer_cfg);
}

void displayhandler(void* parameter)
{
    debug(DEBUG_ALWAYS,"Start displayhandler");
    while(true)
    {
        debug(DEBUG_OLED,"Displayhandler: Waiting for notify...\n");
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &displayhandlerNotifiedValue, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */
        updateDisplay();
    }
}

void inputhandler(void* parameter)
{
    debug(DEBUG_ALWAYS,"Start inputhandler");
    while(true)
    {

        debug(DEBUG_ALWAYS,"Eventhandler: Waiting for notify...\n");
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &buttonNumber, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */

        debug(DEBUG_ALWAYS,"Received interrupt...");
        xTaskNotifyWaitIndexed( buttonNumber, /* Index to wait on. */
                                0x00, /* Don't clear any notification bits on entry. */
                                ULONG_MAX, /* Don't clear any notification bits on exit. */
                                &inputhandlerNotifiedValue, /* Notified value pass out. */
                                pdMS_TO_TICKS(BTN_LONG_PRESS_MS)); /* Block for 2s. */

        // Set to 16 if released or set to 0 otherwise
        // Adding 16 to a button number will produce the long or short press where +16 = SHORT
        inputhandlerNotifiedValue += buttonNumber; 
        debugf(DEBUG_ALWAYS,"Interrupt is: %"PRIu32"", inputhandlerNotifiedValue);
        xTaskNotify(computefunc_task, inputhandlerNotifiedValue, eSetValueWithOverwrite);
    }
}

void computehandler(void* parameter)
{
    debug(DEBUG_ALWAYS,"Start computehandler");
    while(true)
    {
        debug(DEBUG_ALWAYS,"Computehandler: Waiting for notify...\n");
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &computefuncNotifiedValue, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */

        debugf(DEBUG_ALWAYS,"Notified value is: %"PRIu32"", computefuncNotifiedValue);

        executeAction(computefuncNotifiedValue);
    }
}

#endif