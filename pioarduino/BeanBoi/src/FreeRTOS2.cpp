#include "FreeRTOS2.h"
#include <esp_task_wdt.h>

// Watchdog timeout for the compute task (seconds)
static constexpr uint32_t WDT_TIMEOUT_S = 10;

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

std::atomic<OLED::MenuEnum> menu_state{static_cast<OLED::MenuEnum>(0)};

std::atomic<DeviceStateEnum> current_mode{MODE_IDLE};
std::atomic<DeviceStateEnum> next_mode{MODE_IDLE};
std::atomic<DisplayModeEnum> display_mode{DISP_IDLE};
std::atomic<int> calib_progress{0};

/****************************************************************
 * Interrupt handlers implementations
 ****************************************************************/
static constexpr uint32_t DEBOUNCE_MS = 50;  // Ignore edges within 50ms

void IRAM_ATTR displayTimerISR()
{
    xTaskNotifyFromISR(displayhandler_task,(uint32_t)0x00,eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B1Interrupt()
{
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if (now - last_time < DEBOUNCE_MS) return;
    last_time = now;
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B1,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B1),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B2Interrupt()
{
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if (now - last_time < DEBOUNCE_MS) return;
    last_time = now;
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B2,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B2),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B3Interrupt()
{
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if (now - last_time < DEBOUNCE_MS) return;
    last_time = now;
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B3,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B3),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B4Interrupt()
{
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if (now - last_time < DEBOUNCE_MS) return;
    last_time = now;
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)ID_B4,eSetValueWithoutOverwrite,NULL);
    xTaskNotifyFromISR(inputhandler_task,(uint32_t)(0x10 | ID_B4),eSetValueWithOverwrite,NULL);
}

void IRAM_ATTR B5Interrupt()
{
    static uint32_t last_time = 0;
    uint32_t now = millis();
    if (now - last_time < DEBOUNCE_MS) return;
    last_time = now;
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
        case ACTION_ON_SHORT: Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "On short"); break;
        case ACTION_UP_SHORT: Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Up short"); break;
        case ACTION_DOWN_SHORT: Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Down short"); break;
        case ACTION_MODE_SHORT: Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Mode short"); break;
        case ACTION_OFF_SHORT: Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Off short"); break;
    }

    switch (current_mode.load())
    {
        /************************************************************************************************
         *                                      IDLE MODE
         ************************************************************************************************/
        case MODE_IDLE:
        if (action == ACTION_ON_SHORT)
        {
            next_mode = MODE_LASER_ON;
            display_mode = DISP_IDLE;
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Laser on");
            laserOn();
        } else if (action == ACTION_MODE_SHORT)
        {
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Switching mode to: MENU");
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
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Taking shot");
            if (takeShot() != 0) {
                // Shot failed — show error briefly then return to laser mode
                displayError("HOLD STEADY");
                vTaskDelay(pdMS_TO_TICKS(2000));
                next_mode = MODE_LASER_ON;
            }
            laserOff();
        } else if (action == ACTION_OFF_SHORT)
        {
            next_mode = MODE_IDLE;
            display_mode = DISP_IDLE;
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Laser off");
            laserOff();
        } else if (action == ACTION_MODE_SHORT)
        {
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Switching mode to: MENU");
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
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Switching mode to: CALIB");
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
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Getting calibration");
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
            Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Switching mode to: IDLE");
            if (calib_progress > 0)
            {
                next_mode = MODE_CALIB_EXIT;
                display_mode = DISP_CALIB_EXIT;
                y_n_selector = true;  // Reset to default YES
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
                    removePreviousCalib();  // calls removePrevCalib(true) since progress will be <= N_ORIENTATIONS
                    laserOff();
                    display_mode = DISP_STATIC_CALIB;
                } else {
                    removePreviousCalib();
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
                Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Undo calib sample, progress: %d", calib_progress.load());
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
                removePreviousCalib();
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
                y_n_selector = true;  // Reset to default YES
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
    current_mode.store(next_mode.load());
}

void updateDisplay()
{
    switch (display_mode.load())
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
    bool sensors_ok = sh.init();
    // rm3100.begin();
    rm3100.update();
    Debug_csd::logf(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAG, "Mag data: %f %f %f", rm3100.getX(),rm3100.getY(),rm3100.getZ());

    // current_mode = MODE_IDLE;
    current_mode = MODE_IDLE;
    next_mode = MODE_IDLE;
    display_mode = DISP_IDLE;
    menu_state = static_cast<OLED::MenuEnum>(0);
    
    // loadCalibration(); // Load calibration from filesystem
    initDisplayHandler();

    if (!sensors_ok) {
        // Show sensor error on display for 3 seconds
        displayError("SENSOR ERR");
        delay(3000);
    }
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
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Start displayhandler");
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
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Start inputhandler");
    while(true)
    {
        Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Eventhandler: Waiting for notify...");
        enableRisingInterrupts();
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &buttonNumber, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */
        enableFallingInterrupts();

        Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Received interrupt...");
        inputhandlerNotifiedValue = 0;  // Prevent stale value on timeout
        xTaskNotifyWait(        0x00,      /* Don't clear any notification bits on entry. */
                                ULONG_MAX, /* Reset the notification value to 0 on exit. */
                                &inputhandlerNotifiedValue, /* Notified value pass out. */
                                pdMS_TO_TICKS(BTN_LONG_PRESS_MS)); /* Block for 2s. */

        // Check if we got the button release notification (0x10 bit set)
        // or if we timed out (long press)
        if (inputhandlerNotifiedValue & 0x10) {
            Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "SHORT PRESS");
            inputhandlerNotifiedValue = (inputhandlerNotifiedValue & 0x0F) + 0x10; // Keep button ID, add short press flag
        } else {
            Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "LONG PRESS");
            inputhandlerNotifiedValue = buttonNumber; // Use the button that triggered, no flag = long press
        } 
        xTaskNotify(computefunc_task, inputhandlerNotifiedValue, eSetValueWithOverwrite);

        clearInputHandlerEvents();
    }
}

/****************************************************************
 * Task to handle computation - should have lowest priority
 * Protected by hardware watchdog (WDT_TIMEOUT_S seconds)
 ****************************************************************/
void computehandler(void* parameter)
{
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Start computehandler");
    Debug_csd::log(Debug_csd::LOG_INFO, Debug_csd::DEBUG_MAIN, "Initialising device...");

    // Register this task with the ESP32 Task Watchdog Timer
    const esp_task_wdt_config_t wdt_config = {
        .timeout_ms = WDT_TIMEOUT_S * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_init(&wdt_config);
    esp_task_wdt_add(NULL);  // NULL = current task

    sc_accelerometer.getMeasurement();
    while(true)
    {
        // Remove from WDT while blocking (legitimate idle wait)
        esp_task_wdt_delete(NULL);
        Debug_csd::log(Debug_csd::LOG_DEBUG, Debug_csd::DEBUG_MAIN, "Computehandler: Waiting for notify...");
        xTaskNotifyWait(    0x00,      /* Don't clear any notification bits on entry. */
                            ULONG_MAX, /* Reset the notification value to 0 on exit. */
                            &computefuncNotifiedValue, /* Notified value pass out. */
                            portMAX_DELAY );  /* Block indefinitely. */
        // Re-register with WDT while actively processing
        esp_task_wdt_add(NULL);
        executeAction(computefuncNotifiedValue);
        esp_task_wdt_reset();  // Feed after completing action
    }
}
