#ifndef HEADER_FREERTOS2
#define HEADER_FREERTOS2

#include <Arduino.h>
// #include "soc/rtc_wdt.h"
#include "utils.h"
#include <inttypes.h>
#include <freertos/task.h>
#include <debug_csd.h>
#include "config.h"
#include "display_funcs.h"
#include "programflow.h"

// #define configTASK_NOTIFICATION_ARRAY_ENTRIES 5

/****************************************************************
 * Define Button values
 ****************************************************************/
#define PIN_BUTTON1 32 // Centre
#define PIN_BUTTON2 15 // Left
#define PIN_BUTTON3 33 // Right
#define PIN_BUTTON4 27 // Up
#define PIN_BUTTON5 12 // Down

/****************************************************************
 * Define Button values
 ****************************************************************/
#define ID_B1 0x1
#define ID_B2 0x2
#define ID_B3 0x3
#define ID_B4 0x4
#define ID_B5 0x5

#define ACTION_ON_LONG      ID_B5 // ID_B1
#define ACTION_OFF_LONG     ID_B2 // B2 not working
#define ACTION_UP_LONG      ID_B3
#define ACTION_DOWN_LONG    ID_B4
#define ACTION_MODE_LONG    ID_B1

#define ACTION_ON_SHORT     0x10 + ACTION_ON_LONG
#define ACTION_OFF_SHORT    0x10 + ACTION_OFF_LONG
#define ACTION_UP_SHORT     0x10 + ACTION_UP_LONG
#define ACTION_DOWN_SHORT   0x10 + ACTION_DOWN_LONG
#define ACTION_MODE_SHORT   0x10 + ACTION_MODE_LONG

/****************************************************************
 * Config for timing
 ****************************************************************/
const static int BTN_LONG_PRESS_MS = 2000;
const static int DISPLAY_HZ = 3;
const static float DISPLAY_PERIOD = 1.0/DISPLAY_HZ;
const static int DISPLAY_PRESCALAR = 80;
const static int DISPLAY_TMR_HZ = APB_CLK_FREQ/DISPLAY_PRESCALAR;
const static uint64_t DISPLAY_TICKS = DISPLAY_PERIOD *  APB_CLK_FREQ/DISPLAY_PRESCALAR;
static hw_timer_t *displayTimer_cfg = NULL;

/****************************************************************
 * FreeRTOS Task definitions
 ****************************************************************/
TaskHandle_t inputhandler_task;
TaskHandle_t computefunc_task;
TaskHandle_t displayhandler_task;
TaskHandle_t init_task;

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
    DISP_CALIB_STABILISE,

    DISP_CALIB_SAVE,
    DISP_CALIB_REM,
    DISP_CALIB_EXIT,

    DISP_CALIB_LOADING
};

enum DeviceStateEnum
{
    MODE_IDLE,
    MODE_LASER_ON,
    // MODE_SHOT_TAKEN,
    MODE_HISTORY,
    MODE_MENU,

    MODE_CALIB,
    MODE_CALIB_REM_YN,
    MODE_CALIB_SAVE_YN,
    MODE_CALIB_EXIT,

    MODE_BLUETOOTH,
    MODE_FILES,
    MODE_CONFIG
};

static DeviceStateEnum current_mode;
static DeviceStateEnum next_mode;
static DisplayModeEnum display_mode;
static OLED::MenuEnum menu_state;
/****************************************************************
 * Globals
 ****************************************************************/
static int calib_progress = 0;

/****************************************************************
 * Interrupt handlers
 ****************************************************************/
void IRAM_ATTR displayTimerISR()
{
    xTaskNotifyFromISR(displayhandler_task,(uint32_t)0x00,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B1Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B1_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B1,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B1,(uint32_t)0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B2Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B2_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B2,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B2,(uint32_t)0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B3Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B3_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B3,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B3,(uint32_t)0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B4Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B4_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B4,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B4,(uint32_t)0x10,eSetValueWithOverwrite,NULL);
}
void IRAM_ATTR B5Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B5_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B5,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyIndexedFromISR(inputhandler_task,ID_B5,(uint32_t)0x10,eSetValueWithOverwrite,NULL);
}
//  configTASK_NOTIFICATION_ARRAY_ENTRIES
/****************************************************************
 * Button Interrupts
 ****************************************************************/
void enableRisingInterrupts()
{
    detachInterrupt(PIN_BUTTON1);
    detachInterrupt(PIN_BUTTON2);
    detachInterrupt(PIN_BUTTON3);
    detachInterrupt(PIN_BUTTON4);
    detachInterrupt(PIN_BUTTON5);
    attachInterrupt(PIN_BUTTON1,B1Interrupt,RISING);
    attachInterrupt(PIN_BUTTON2,B2Interrupt,RISING);
    attachInterrupt(PIN_BUTTON3,B3Interrupt,RISING);
    attachInterrupt(PIN_BUTTON4,B4Interrupt,RISING);
    attachInterrupt(PIN_BUTTON5,B5Interrupt,RISING);
}
void enableFallingInterrupts()
{
    {
    detachInterrupt(PIN_BUTTON1);
    detachInterrupt(PIN_BUTTON2);
    detachInterrupt(PIN_BUTTON3);
    detachInterrupt(PIN_BUTTON4);
    detachInterrupt(PIN_BUTTON5);
    attachInterrupt(PIN_BUTTON1,B1Interrupt,FALLING);
    attachInterrupt(PIN_BUTTON2,B2Interrupt,FALLING);
    attachInterrupt(PIN_BUTTON3,B3Interrupt,FALLING);
    attachInterrupt(PIN_BUTTON4,B4Interrupt,FALLING);
    attachInterrupt(PIN_BUTTON5,B5Interrupt,FALLING);
}
}
void initInterrupts()
{
    pinMode(PIN_BUTTON1,INPUT_PULLUP);
    pinMode(PIN_BUTTON2,INPUT_PULLUP);
    pinMode(PIN_BUTTON3,INPUT_PULLUP);
    pinMode(PIN_BUTTON4,INPUT_PULLUP);
    pinMode(PIN_BUTTON5,INPUT_PULLUP);
    attachInterrupt(PIN_BUTTON1,B1Interrupt,RISING);
    attachInterrupt(PIN_BUTTON2,B2Interrupt,RISING);
    attachInterrupt(PIN_BUTTON3,B3Interrupt,RISING);
    attachInterrupt(PIN_BUTTON4,B4Interrupt,RISING);
    attachInterrupt(PIN_BUTTON5,B5Interrupt,RISING);
}
void clearInputHandlerEvents(){
    xTaskNotifyStateClear(inputhandler_task);
    xTaskNotifyStateClearIndexed(inputhandler_task,1);
    xTaskNotifyStateClearIndexed(inputhandler_task,2);
    xTaskNotifyStateClearIndexed(inputhandler_task,3);
    xTaskNotifyStateClearIndexed(inputhandler_task,4);
    xTaskNotifyStateClearIndexed(inputhandler_task,5);
}

/****************************************************************
 * Extern function definitions
 ****************************************************************/
void extern laserOn();
void extern laserOff();
void extern takeShot();
int extern getCalib();
void extern saveCalib();
void extern clearCalibration();
void extern loadCalibration();

/****************************************************************
 * Program flow FSM
 ****************************************************************/
void executeAction(const uint32_t action)
{
    switch (action)
    {
        case ACTION_ON_SHORT: Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "On short"); break;
        case ACTION_UP_SHORT: Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Up short"); break;
        case ACTION_DOWN_SHORT: Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Down short"); break;
        case ACTION_MODE_SHORT: Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Mode short"); break;
        case ACTION_OFF_SHORT: Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Off short"); break;

    }

    switch (current_mode)
    {
        case MODE_IDLE:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_LASER_ON;
            display_mode = DISP_IDLE;
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Laser on");
            laserOn();
        } else if (action == ACTION_MODE_SHORT)
        {
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: HISTORY");
            next_mode = MODE_HISTORY;
            display_mode = DISP_HISTORY;
        }
        break;

        case MODE_LASER_ON:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_LASER_ON;
            display_mode = DISP_SHOT_TAKEN;
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Taking shot");
            takeShot();
        } else if (action == ACTION_OFF_SHORT) {
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Laser off");
            laserOff();
        } else if (action == ACTION_MODE_SHORT)
        {
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: HISTORY");
            next_mode = MODE_HISTORY;
            display_mode = DISP_HISTORY;
        }
        break;

        case MODE_HISTORY:
        if (action == ACTION_MODE_SHORT){
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: MENU");
            next_mode = MODE_MENU;
            display_mode = DISP_MENU;
        }
        break;

        case MODE_MENU:
        if (action == ACTION_ON_SHORT){
            executeMenuAction(menu_state);

        } else if (action == ACTION_MODE_SHORT){
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: CALIB");
            clearCalibration(); // Clear calibration when entering calibration mode
            next_mode = MODE_CALIB;
            display_mode = DISP_STATIC_CALIB;

        } else if (action == ACTION_UP_SHORT){
            if (static_cast<int>(menu_state) <= 0) {
                menu_state = static_cast<OLED::MenuEnum>(3);
            } else {
                menu_state = static_cast<OLED::MenuEnum>(static_cast<int>(menu_state) - 1);
            }

        } else if (action == ACTION_DOWN_SHORT){
            if (static_cast<int>(menu_state) >= 3) {
                menu_state = static_cast<OLED::MenuEnum>(0);
            } else {
                menu_state = static_cast<OLED::MenuEnum>(static_cast<int>(menu_state) + 1);
            }
        }
        break;


        /************************************************************************************************
         *                                      CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB:
        if (action == ACTION_ON_SHORT)
        {
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Getting calibration");
            if (calib_progress == 2 || calib_progress == 3)
            {
                display_mode = DISP_CALIB_STABILISE;
                delay(3000); // If facing down delay 3s before taking measurement
            }

            display_mode = DISP_CALIB_LOADING;
            calib_progress = getCalib();
            laserBeep();
            if (calib_progress >= (N_ORIENTATIONS + N_LASER_CAL))
            {
                next_mode = MODE_CALIB_SAVE_YN;
                display_mode = DISP_CALIB_SAVE;
            } else if (calib_progress >= N_ORIENTATIONS) {
                next_mode = MODE_CALIB;
                display_mode = DISP_LASER_CALIB;
                laserOn();
            } else {
                next_mode = MODE_CALIB;
                display_mode = DISP_STATIC_CALIB; 
            }
        } else if (action == ACTION_MODE_SHORT){
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: IDLE");
            // clearCalibration();
            // loadCalibration();
            // calib_progress = 0;
            // next_mode = MODE_IDLE;
            // display_mode = DISP_IDLE;
            if (calib_progress > 0)
            {
                next_mode = MODE_CALIB_EXIT;
                display_mode = DISP_CALIB_EXIT;
            } else {
                next_mode = MODE_IDLE;
                display_mode = DISP_IDLE;
            }

        }
        break;


        /************************************************************************************************
         *                                REMOVE LATEST CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB_REM_YN:
        if (action == ACTION_ON_SHORT)
        {
            if(y_n_selector) removePreviosCalib();
        } else if (action == ACTION_UP_SHORT){
            y_n_selector = true;
        } else if (action == ACTION_DOWN_SHORT){
            y_n_selector = false;
        }
        break;


        /************************************************************************************************
         *                                    SAVE CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB_SAVE_YN:
        if (action == ACTION_ON_SHORT)
        {
            if(y_n_selector) saveCalib();
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
        } else if (action == ACTION_UP_SHORT){
            y_n_selector = true;
        } else if (action == ACTION_DOWN_SHORT){
            y_n_selector = false;
        }
        break;


        /************************************************************************************************
         *                                    EXIT CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB_EXIT:
        if (action == ACTION_ON_SHORT)
        {
            if(y_n_selector) {
                calib_progress = 0;
                clearCalibration();
                loadCalibration();
                display_mode = DISP_IDLE;
                next_mode = MODE_IDLE;
            } else {
                if (calib_progress >= N_ORIENTATIONS) {
                    next_mode = MODE_CALIB;
                    display_mode = DISP_LASER_CALIB;
                } else {
                    next_mode = MODE_CALIB;
                    display_mode = DISP_STATIC_CALIB; 
                }
            }
        } else if (action == ACTION_UP_SHORT){
            y_n_selector = true;
        } else if (action == ACTION_DOWN_SHORT){
            y_n_selector = false;
        }
        break;

        case MODE_BLUETOOTH:
        break;

        case MODE_FILES:
        break;

        case MODE_CONFIG:
        break;
    }
    current_mode = next_mode;
}

void updateDisplay()
{
    switch (display_mode)
    {
    case DISP_IDLE:
        // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Displaying idle...");
        displayIdle();
        break;

    case DISP_HISTORY:
        // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Displaying history...");
        displayHistory();
        break;

    case DISP_MENU:
        // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Displaying menu...");
        displayMenu(menu_state);
        break;

    case DISP_STATIC_CALIB:
        // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Displaying static calib...");
        displayStaticCalib(sh.getCalibProgress(true));
        break;

    case DISP_LASER_CALIB:
        // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Displaying laser calib...");
        // Serial.println(sh.getCalibProgress(false));
        displayLaserCalib(sh.getCalibProgress(false));
        break;

    case DISP_CALIB_SAVE:
        displayCalibSaveYN();
        break;

    case DISP_CALIB_REM:
        displayCalibRemYN();
        break;

    case DISP_CALIB_EXIT:
        displayCalibExitYN();
        break;

    case DISP_CALIB_LOADING:
        displayLoading(LoadingEnum::collecting_data);
        break;

    case DISP_CALIB_STABILISE:
        displayLoading(LoadingEnum::calib_stabilising);
        break;
    
    default:
        break;
    }
    // xTaskNotifyFromISR( displayhandler_task,
    //                     0x00,
    //                     eSetValueWithOverwrite, 
    //                     NULL);

}

void startDisplayTimer()
{
    // displayTimer_cfg = timerBegin(0, DISPLAY_PRESCALAR, true);
    // timerAttachInterrupt(displayTimer_cfg, &displayTimerISR, true);
    // timerAlarmWrite(displayTimer_cfg, DISPLAY_TICKS, true);
    // timerAlarmEnable(displayTimer_cfg);
    displayTimer_cfg = timerBegin(DISPLAY_TMR_HZ);
    timerAttachInterrupt(displayTimer_cfg,&displayTimerISR);
    timerAlarm(displayTimer_cfg,DISPLAY_TICKS,true,UINT64_MAX);
    // configTASK_NOTIFICATION_ARRAY_ENTRIES
}

void initialise_device()
{
    sh.init();
    // rm3100.begin();
    rm3100.update();
    Serial.printf("Mag data: %f %f %f\n", rm3100.getX(),rm3100.getY(),rm3100.getZ());

    sc_accelerometer.getMeasurement();
    Serial.printf("Acc data: %lf %lf %lf\n", sca3300.getCalculatedAccelerometerX(),sca3300.getCalculatedAccelerometerY(),sca3300.getCalculatedAccelerometerZ());

    ldk2m.toggleLaser(true);
    ldk2m.getMeasurement();

    Serial.println("sh.update...");
    sh.update();

    initDisplayHandler();
    initInterrupts();
    startDisplayTimer();
}

/****************************************************************
 * Task to handle display - should have highest priority
 ****************************************************************/
void displayhandler(void* parameter)
{
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Start displayhandler");
    while(true)
    {
        // Debug_csd::debug(Debug_csd::DEBUG_OLED,"Displayhandler: Waiting for notify...\n");
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &displayhandlerNotifiedValue, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */
        updateDisplay();

    }
}

/****************************************************************
 * Task to handle inputs - should have second highest priority
 ****************************************************************/
void inputhandler(void* parameter)
{
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Start inputhandler");
    while(true)
    {

        Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Eventhandler: Waiting for notify...\n");
        enableRisingInterrupts();
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &buttonNumber, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */
        enableFallingInterrupts();

        Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Received interrupt...");
        clearInputHandlerEvents();
        xTaskNotifyWaitIndexed( buttonNumber, /* Index to wait on. */
                                ULONG_MAX, /* Reset the notification value to 0 on entry. */
                                ULONG_MAX, /* Reset the notification value to 0 on exit. */
                                &inputhandlerNotifiedValue, /* Notified value pass out. */
                                pdMS_TO_TICKS(BTN_LONG_PRESS_MS)); /* Block for 2s. */

        // Set to 16 if released or set to 0 otherwise
        // Adding 16 to a button number will produce the long or short press where +16 = SHORT
        if (inputhandlerNotifiedValue == 0x10) {Debug_csd::debugf(Debug_csd::DEBUG_ALWAYS,"SHORT PRESS");}
        else {Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"LONG PRESS");}
        inputhandlerNotifiedValue += buttonNumber; 
        xTaskNotify(computefunc_task, inputhandlerNotifiedValue, eSetValueWithOverwrite);

        clearInputHandlerEvents();
    }
}

/****************************************************************
 * Task to handle computation - should have lowest priority
 ****************************************************************/
void computehandler(void* parameter)
{
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Start computehandler");
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Initialising device...");
    initialise_device();
    sc_accelerometer.getMeasurement();
    while(true)
    {
        Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"Computehandler: Waiting for notify...\n");
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &computefuncNotifiedValue, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */
        executeAction(computefuncNotifiedValue);
    }
}

/****************************************************************
 * Task to handle initialisation - should have highest priority
 ****************************************************************/
// void inithandler(void* parameter)
// {
//     initialise_device();
//     while(true) {delay(1);}
// }

#endif