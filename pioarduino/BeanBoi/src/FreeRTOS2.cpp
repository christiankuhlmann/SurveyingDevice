#include "FreeRTOS2.h"

/****************************************************************
 * Global variable definitions
 ****************************************************************/
const int BTN_LONG_PRESS_MS = 2000;
const int DISPLAY_HZ = 5;
const float DISPLAY_PERIOD = 1.0/DISPLAY_HZ;
const int DISPLAY_PRESCALAR = 80;
const int DISPLAY_TMR_HZ = APB_CLK_FREQ/DISPLAY_PRESCALAR;
const uint64_t DISPLAY_TICKS = DISPLAY_PERIOD * APB_CLK_FREQ/DISPLAY_PRESCALAR;

hw_timer_t *displayTimer_cfg = NULL;

TaskHandle_t inputhandler_task;
TaskHandle_t computefunc_task;
TaskHandle_t displayhandler_task;
TaskHandle_t init_task;

uint32_t buttonNumber = 0x00;
uint32_t inputhandlerNotifiedValue = 0x00;
uint32_t computefuncNotifiedValue = 0x00;
uint32_t displayhandlerNotifiedValue = 0x00;

DeviceStateEnum current_mode;
DeviceStateEnum next_mode;
DisplayModeEnum display_mode;
OLED::MenuEnum menu_state;
int calib_progress = 0;

/****************************************************************
 * Interrupt handlers implementations
 ****************************************************************/
void IRAM_ATTR displayTimerISR()
{
    xTaskNotifyFromISR(displayhandler_task,(uint32_t)0x00,eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B1Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B1_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B1,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B1),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B2Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B2_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B2,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B2),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B3Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B3_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B3,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B3),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B4Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B4_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B4,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B4),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B5Interrupt()
{
    // Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"B5_INTERRUPT");
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B5,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B5),eSetValueWithOverwrite,NULL);
}

/****************************************************************
 * Hardware timers
 ****************************************************************/
void startDisplayTimer()
{
    // Using arduino version >= 3
    displayTimer_cfg = timerBegin(DISPLAY_TMR_HZ);
    timerAttachInterrupt(displayTimer_cfg,&displayTimerISR);
    timerAlarm(displayTimer_cfg,DISPLAY_TICKS,true,UINT64_MAX);
}

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
    // Only use the default notification index (0) - no indexed clearing needed
}

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
        /************************************************************************************************
         *                                      IDLE MODE
         ************************************************************************************************/
        case MODE_IDLE:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_LASER_ON;
            display_mode = DISP_IDLE;
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Laser on");
            laserOn();
        } else if (action == ACTION_MODE_SHORT)
        {
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: MENU");
            next_mode = MODE_MENU;
            display_mode = DISP_MENU;
        }
        break;

        /************************************************************************************************
         *                                      LASER ON MODE
         ************************************************************************************************/
        case MODE_LASER_ON:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Taking shot");
            takeShot();
            laserOff();
        } else if (action == ACTION_OFF_SHORT)
        {
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Laser off");
            laserOff();
        } else if (action == ACTION_MODE_SHORT)
        {
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: MENU");
            next_mode = MODE_MENU;
            display_mode = DISP_MENU;
        }
        break;

        /************************************************************************************************
         *                                      MENU MODE
         ************************************************************************************************/
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
                menu_state = static_cast<OLED::MenuEnum>(MENU_SIZE - 1);
            } else {
                menu_state = static_cast<OLED::MenuEnum>(static_cast<int>(menu_state) - 1);
            }

        } else if (action == ACTION_DOWN_SHORT){
            if (static_cast<int>(menu_state) >= MENU_SIZE - 1) {
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
            display_mode = DISP_CALIB_STABILISE;
            delay(1000);
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Getting calibration");
            if (calib_progress == 2 || calib_progress == 3)
            {
                display_mode = DISP_CALIB_STABILISE;
                delay(4000); // If facing down delay 3s before taking measurement
            }

            display_mode = DISP_CALIB_LOADING;
            calib_progress = getCalib();
            laserBeep();

            if (calib_progress >= (N_ORIENTATIONS + N_LASER_CAL))
            {
                runCalibration();
                next_mode = MODE_CALIB_SAVE_YN;
                display_mode = DISP_CALIB_QUALITY;
            } else if (calib_progress >= N_ORIENTATIONS) {
                next_mode = MODE_CALIB;
                display_mode = DISP_LASER_CALIB;
                laserOn();
                laserOn();
            } else {
                next_mode = MODE_CALIB;
                display_mode = DISP_STATIC_CALIB; 
                laserOff();
                laserOff();
            }

        } else if (action == ACTION_MODE_SHORT){
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Switching mode to: IDLE");
            if (calib_progress > 0)
            {
                next_mode = MODE_CALIB_EXIT;
                display_mode = DISP_CALIB_EXIT;
            } else {
                loadCalibration();
                next_mode = MODE_IDLE;
                display_mode = DISP_IDLE;
            }
        } else if (action == ACTION_DOWN_SHORT){
            // Undo last calibration sample
            if (calib_progress > 0)
            {
                // Cross-phase check: if in laser phase with 0 laser samples,
                // step back into static phase
                bool in_laser_phase = (calib_progress >= N_ORIENTATIONS);
                bool laser_at_zero = (sh.getCalibProgress(false) == 0);

                if (in_laser_phase && laser_at_zero) {
                    // Roll back into static phase
                    removePreviosCalib();  // calls removePrevCalib(true) since progress will be <= N_ORIENTATIONS
                    laserOff();
                    display_mode = DISP_STATIC_CALIB;
                } else {
                    removePreviosCalib();
                    // Stay in current phase
                    if (sh.getCalibProgress() >= N_ORIENTATIONS) {
                        display_mode = DISP_LASER_CALIB;
                    } else {
                        display_mode = DISP_STATIC_CALIB;
                        laserOff();
                    }
                }
                calib_progress = sh.getCalibProgress();
                laserBeep();
                Debug_csd::debugf(Debug_csd::DEBUG_ALWAYS, "Undo calib sample, progress: %d", calib_progress);
            }
        }
        break;

        /************************************************************************************************
         *                                REMOVE LATEST CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB_REM_YN:
        if (action == ACTION_ON_SHORT)
        {
            if(y_n_selector) {
                removePreviosCalib();
                calib_progress = sh.getCalibProgress();
            }
            // Return to calibration mode regardless of choice
            if (calib_progress >= N_ORIENTATIONS) {
                next_mode = MODE_CALIB;
                display_mode = DISP_LASER_CALIB;
            } else {
                next_mode = MODE_CALIB;
                display_mode = DISP_STATIC_CALIB;
            }
        } else if (action == ACTION_UP_SHORT){
            y_n_selector = true;
        } else if (action == ACTION_DOWN_SHORT){
            y_n_selector = false;
        }
        break;

        /************************************************************************************************
         *                                      SAVE CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB_SAVE_YN:
        if (action == ACTION_ON_SHORT)
        {
            if (display_mode == DISP_CALIB_QUALITY) {
                // User has seen quality — advance to save prompt
                display_mode = DISP_CALIB_SAVE;
                y_n_selector = true;
            } else {
                // Save or discard, then load calibration from NVS
                if(y_n_selector) saveCalib();
                loadCalibration();
                next_mode = MODE_IDLE;
                display_mode = DISP_IDLE;
            }
        } else if (action == ACTION_UP_SHORT){
            if (display_mode == DISP_CALIB_SAVE) y_n_selector = true;
        } else if (action == ACTION_DOWN_SHORT){
            if (display_mode == DISP_CALIB_SAVE) y_n_selector = false;
        }
        break;

        /************************************************************************************************
         *                                      EXIT CALIBRATION MODE
         ************************************************************************************************/
        case MODE_CALIB_EXIT:
        if (action == ACTION_ON_SHORT)
        {
            if(y_n_selector){
                loadCalibration();
                next_mode = MODE_IDLE;
                display_mode = DISP_IDLE;
            } else {
                next_mode = MODE_CALIB;
                display_mode = DISP_STATIC_CALIB;
            }
        } else if (action == ACTION_UP_SHORT){
            y_n_selector = true;
        } else if (action == ACTION_DOWN_SHORT){
            y_n_selector = false;
        }
        break;

        case MODE_BLUETOOTH:
        break;

        /************************************************************************************************
         *                                      SHOT HISTORY MODE
         ************************************************************************************************/
        case MODE_HISTORY:
        if (action == ACTION_UP_SHORT) {
            history_scroll_index--;
            if (history_scroll_index < 0) history_scroll_index = 0;
        } else if (action == ACTION_DOWN_SHORT) {
            int shot_count = sh.getShotCount(current_file_id);
            history_scroll_index++;
            if (history_scroll_index >= shot_count) history_scroll_index = shot_count - 1;
            if (history_scroll_index < 0) history_scroll_index = 0;
        } else if (action == ACTION_MODE_SHORT) {
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
        }
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

    case DISP_CALIB_QUALITY:
        displayCalibrationQuality();
        break;

    case DISP_CALIB_REM:
        displayCalibRemYN();
        break;

    case DISP_CALIB_EXIT:
        displayCalibExitYN();
        break;

    case DISP_CALIB_STABILISE:
        displayLoading(calib_stabilising);
        break;

    case DISP_CALIB_LOADING:
        displayLoading(collecting_data);
        break;

    default:
        break;
    }
}

void initialise_device()
{
    sh.init();
    // rm3100.begin();
    rm3100.update();
    Serial.printf("Mag data: %f %f %f\n", rm3100.getX(),rm3100.getY(),rm3100.getZ());

    // current_mode = MODE_IDLE;
    current_mode = MODE_IDLE;
    next_mode = MODE_IDLE;
    display_mode = DISP_IDLE;
    menu_state = static_cast<OLED::MenuEnum>(0);
    
    // loadCalibration(); // Load calibration from filesystem
    initDisplayHandler();
}

void initialise_interrupts()
{
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
        inputhandlerNotifiedValue = 0;  // Prevent stale value on timeout
        xTaskNotifyWait(        0x00,      /* Don't clear any notification bits on entry. */
                                ULONG_MAX, /* Reset the notification value to 0 on exit. */
                                &inputhandlerNotifiedValue, /* Notified value pass out. */
                                pdMS_TO_TICKS(BTN_LONG_PRESS_MS)); /* Block for 2s. */

        // Check if we got the button release notification (0x10 bit set)
        // or if we timed out (long press)
        if (inputhandlerNotifiedValue & 0x10) {
            Debug_csd::debugf(Debug_csd::DEBUG_ALWAYS,"SHORT PRESS");
            inputhandlerNotifiedValue = (inputhandlerNotifiedValue & 0x0F) + 0x10; // Keep button ID, add short press flag
        } else {
            Debug_csd::debug(Debug_csd::DEBUG_ALWAYS,"LONG PRESS");
            inputhandlerNotifiedValue = buttonNumber; // Use the button that triggered, no flag = long press
        } 
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
