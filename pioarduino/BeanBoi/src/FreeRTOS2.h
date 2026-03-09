#ifndef HEADER_FREERTOS2
#define HEADER_FREERTOS2

#include <Arduino.h>
#include "utils.h"
#include <inttypes.h>
#include <atomic>
#include <freertos/task.h>
#include <debug_csd.h>
#include "config.h"
#include "display_funcs.h"
#include "programflow.h"
#include <ble_manager.h>

/****************************************************************
 * Define Button values
 ****************************************************************/
#define PIN_BUTTON1 32 // Centre
#define PIN_BUTTON2 15 // Left
#define PIN_BUTTON3 33 // Right
#define PIN_BUTTON4 27 // Up
#define PIN_BUTTON5 12 // Down

/****************************************************************
 * Define Button ID values
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
 * FreeRTOS Task Priorities
 * 
 * Priority scheme (higher number = higher priority):
 * - DISPLAY (3): Highest priority for 5Hz UI refresh, ensures 
 *                responsive user feedback
 * - INPUT (2):   Medium priority for button handling, ensures
 *                timely response to user interactions
 * - COMPUTE (1): Lowest priority for sensor processing and 
 *                calibration calculations, runs when higher 
 *                priority tasks are idle
 * 
 * BLE task runs on Core 1 with priority 1 (see ble_manager.cpp)
 ****************************************************************/
namespace TaskPriorities {
    constexpr UBaseType_t COMPUTE = 1;   // Background processing
    constexpr UBaseType_t INPUT_HANDLER = 2;     // Button interrupt handling
    constexpr UBaseType_t DISPLAY_HANDLER = 3;   // User-facing 5Hz refresh
}

/****************************************************************
 * FreeRTOS Task Stack Sizes (in words, 1 word = 4 bytes)
 * 
 * Stack allocations:
 * - COMPUTE: 100,000 words (400KB) - Large due to Eigen matrix
 *            operations during calibration (fitEllipsoid uses
 *            ~30KB static buffers)
 * - INPUT_HANDLER: 2,500 words (10KB) - Moderate for state machine
 * - DISPLAY_HANDLER: 2,500 words (10KB) - Moderate for OLED operations
 ****************************************************************/
namespace TaskStackSizes {
    constexpr uint32_t COMPUTE = 25000;   // 100KB for Eigen operations
    constexpr uint32_t INPUT_HANDLER = 2500;      // 10KB for state machine
    constexpr uint32_t DISPLAY_HANDLER = 2500;    // 10KB for OLED rendering
}

/****************************************************************
 * Config for timing - external declarations
 ****************************************************************/
extern const int BTN_LONG_PRESS_MS;
extern const int DISPLAY_HZ;
extern const float DISPLAY_PERIOD;
extern const int DISPLAY_PRESCALAR;
extern const int DISPLAY_TMR_HZ;
extern const uint64_t DISPLAY_TICKS;

/****************************************************************
 * FreeRTOS Task definitions - external declarations
 ****************************************************************/
extern TaskHandle_t inputhandler_task;
extern TaskHandle_t computefunc_task;
extern TaskHandle_t displayhandler_task;
extern TaskHandle_t init_task;

extern uint32_t buttonNumber;
extern uint32_t inputhandlerNotifiedValue;
extern uint32_t computefuncNotifiedValue;
extern uint32_t displayhandlerNotifiedValue;

extern hw_timer_t *displayTimer_cfg;

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
    DISP_CALIB_QUALITY,
    DISP_CALIB_REM,
    DISP_CALIB_EXIT,

    DISP_CALIB_LOADING
};

enum DeviceStateEnum
{
    MODE_IDLE,
    MODE_LASER_ON,
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

/****************************************************************
 * External global variables (atomic for cross-task safety)
 ****************************************************************/
extern std::atomic<DeviceStateEnum> current_mode;
extern std::atomic<DeviceStateEnum> next_mode;
extern std::atomic<DisplayModeEnum> display_mode;
extern std::atomic<OLED::MenuEnum> menu_state;
extern std::atomic<int> calib_progress;

/****************************************************************
 * Interrupt handlers
 ****************************************************************/
void IRAM_ATTR displayTimerISR();
void IRAM_ATTR B1Interrupt();
void IRAM_ATTR B2Interrupt();
void IRAM_ATTR B3Interrupt();
void IRAM_ATTR B4Interrupt();
void IRAM_ATTR B5Interrupt();

/****************************************************************
 * Function declarations
 ****************************************************************/
void startDisplayTimer();
void enableRisingInterrupts();
void enableFallingInterrupts();
void initInterrupts();
void clearInputHandlerEvents();

void executeAction(const uint32_t action);
void updateDisplay();

void initialise_device();
void initialise_interrupts();

// Task functions
void displayhandler(void* parameter);
void inputhandler(void* parameter);
void computehandler(void* parameter);

#endif
