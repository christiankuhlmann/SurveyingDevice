#include "programflow.h"
#include "FreeRTOS2.h"
#include <ble_manager.h>
#include <esp_task_wdt.h>

// Global variable definitions
RM3100 rm3100;
SCA3300 sca3300;
LDK_2M ldk2m;

SCA3300SensorConnection sc_accelerometer(sca3300);
RM3100SensorConnection sc_magnetometer(rm3100);
LDK2MSensorConnection sc_laser(ldk2m);

SensorHandler sh(sc_accelerometer, sc_magnetometer, sc_laser);
OLED::DisplayHandler dh;
std::atomic<bool> y_n_selector{true};
std::atomic<int> history_scroll_index{0};
std::atomic<unsigned int> current_file_id{0};

// Function implementations
int getBatteryVoltage()
{
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    
    if (measuredvbat >= 3.98) {
        return 100;
    } else if (measuredvbat >= 3.84) {
        return 70;
    } else if (measuredvbat >= 3.75) {
        return 40;
    } else if (measuredvbat >= 3.69) {
        return 20;
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

int takeShot()
{
    // Block shot-taking if sensors are not initialised
    if (!sh.isSensorsReady()) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "Cannot take shot: sensors not ready");
        return 1;
    }
    // Only beep and persist if the shot was taken successfully
    sh.lock();
    int result = sh.takeShot();
    sh.unlock();
    if (result != 1) {
        sc_laser.beep();
        if (result == 2) {
            Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_SENSOR, "Shot taken with low quality (outlier fallback)");
        }
        // Persist to NVS and send over BLE
        MeasurementRecord rec = sh.getShotData(true);
        if (!saveShotData(rec, current_file_id)) {
            Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_FILE, "Failed to save shot data to NVS");
        }
        sendBLEData(rec);
        return 0;
    } else {
        return 1;
    }
}

int getCalib()
{
    if (!sh.isSensorsReady()) {
        Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_SENSOR, "Cannot calibrate: sensors not ready");
        return 0;
    }
    sh.lock();
    if (sh.getCalibProgress() < N_ORIENTATIONS)
    {
        sh.collectStaticCalibData();
    } else if (sh.getCalibProgress() < (N_ORIENTATIONS + N_LASER_CAL))
    {
        sh.collectLaserCalibData();
    }
    int progress = sh.getCalibProgress();
    sh.unlock();
    return progress;
}

void saveCalib()
{
    sh.saveCalibration();
}

void removePreviousCalib()
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
    y_n_selector = false;
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
    // Non-blocking try-lock: skip sensor update if compute task holds the mutex
    // (e.g. during calibration or shot-taking). Display renders cached data for that frame.
    static MeasurementRecord cached_data;
    if (sh.tryLock()) {
        sh.update();
        cached_data = sh.getShotData();
        sh.unlock();
    }
    // If lock not acquired, cached_data retains the last consistent snapshot
    dh.clearDisplay();
    dh.drawHeading(cached_data.heading);
    dh.drawInclination(cached_data.inclination);
    dh.drawRoll(cached_data.roll);
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

    int shot_count = sh.getShotCount(current_file_id);
    if (shot_count <= 0) {
        dh.drawCentered("No shots", SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 30, &Font12);
        dh.update();
        return;
    }

    // Clamp scroll index
    if (history_scroll_index >= shot_count) history_scroll_index = shot_count - 1;
    if (history_scroll_index < 0) history_scroll_index = 0;

    // Display up to 5 shots per page using Font8 (8px height + 2px gap = 10px per row)
    // Available vertical space: 128 - 16 (top bar) - 12 (title) = 100px → 10 rows at 10px each
    const int ROWS_PER_PAGE = 5;
    const int ROW_HEIGHT = 10;
    int page_start = (history_scroll_index / ROWS_PER_PAGE) * ROWS_PER_PAGE;

    // Title
    char title[16];
    snprintf(title, sizeof(title), "Shots (%d)", shot_count);
    dh.drawCentered(String(title), SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 4, &Font8);

    // Draw shot rows
    MeasurementRecord rec;
    for (int i = 0; i < ROWS_PER_PAGE; i++) {
        int shot_idx = page_start + i;
        if (shot_idx >= shot_count) break;

        // Shot IDs in NVS are 1-indexed
        int y = TOP_BAR_HEIGHT + 14 + i * ROW_HEIGHT;
        bool selected = (shot_idx == history_scroll_index);

        if (selected) {
            Paint_DrawRectangle(0, y - 1, 63, y + 8, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        }

        if (sh.readShotByIndex(rec, current_file_id, shot_idx + 1)) {
            char line[28];
            snprintf(line, sizeof(line), "%03d %5.1f %5.1f %4.1f",
                     shot_idx + 1, rec.heading, rec.inclination, rec.distance);
            if (selected) {
                dh.drawLeftBlack(String(line), 1, y, &Font8);
            } else {
                dh.drawLeft(String(line), 1, y, &Font8);
            }
        }
    }

    // Scroll indicators
    if (page_start > 0) {
        dh.drawCentered("^", SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 12, &Font8);
    }
    if (page_start + ROWS_PER_PAGE < shot_count) {
        dh.drawCentered("v", SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 14 + ROWS_PER_PAGE * ROW_HEIGHT, &Font8);
    }

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

void displayCalibrationQuality()
{
    const auto &q = sh.getCalibParms().quality;
    dh.clearDisplay();
    displayBatteryStatus();

    // Title: grade name
    const char* grade_str = NumericalMethods::gradeToString(q.grade);
    dh.drawCentered(String(grade_str), SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 5, &Font12);

    // Metrics using Font8 (compact)
    char line[26];
    snprintf(line, sizeof(line), "Inc s: %.2f dg", q.inclination_sigma_deg);
    dh.drawLeft(String(line), 1, TOP_BAR_HEIGHT + 25, &Font8);

    snprintf(line, sizeof(line), "Mag r: %.4f", q.mag_fit_residual);
    dh.drawLeft(String(line), 1, TOP_BAR_HEIGHT + 37, &Font8);

    snprintf(line, sizeof(line), "Acc r: %.4f", q.acc_fit_residual);
    dh.drawLeft(String(line), 1, TOP_BAR_HEIGHT + 49, &Font8);

    snprintf(line, sizeof(line), "Las s: %.2f dg", q.laser_plane_spread_deg);
    dh.drawLeft(String(line), 1, TOP_BAR_HEIGHT + 61, &Font8);

    // Prompt
    dh.drawCentered("ON: continue", SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 80, &Font8);

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
        dh.displayLaserCalib(180, "5/8");
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

void displayError(const char* msg)
{
    dh.clearDisplay();
    dh.drawCentered(String(msg), SCREEN_WIDTH/2, TOP_BAR_HEIGHT + 40, &Font12);
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
        dh.displayLoading("Waiting","...",count);
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

    case OLED::MenuEnum::MENU_HISTORY:
        history_scroll_index = 0;
        next_mode = MODE_HISTORY;
        display_mode = DISP_HISTORY;
    break;
    
    case OLED::MenuEnum::MENU_FORCE_CAL:
        Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "FORCE_CAL start - Free heap: %u, Largest block: %u", 
                         ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        
        // Add a small delay to allow any pending operations to complete
        vTaskDelay(pdMS_TO_TICKS(100));
        Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "After delay - Free heap: %u, Largest block: %u", 
                         ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        
        sh.loadRawCalibrationData();  // Only load raw data, not computed parameters
        Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "After loadRawCalibrationData - Free heap: %u, Largest block: %u", 
                         ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        
        sh.calibrate();
        Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "After calibrate - Free heap: %u, Largest block: %u", 
                         ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        
        sh.align();
        Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "After align - Free heap: %u, Largest block: %u", 
                         ESP.getFreeHeap(), ESP.getMaxAllocHeap());
        
        sh.saveCalibration();
        Debug_csd::logf(Debug_csd::LOG_TRACE, Debug_csd::DEBUG_HEAP, "FORCE_CAL complete - Free heap: %u, Largest block: %u", 
                         ESP.getFreeHeap(), ESP.getMaxAllocHeap());
    break;

    default:
        break;
    }
}

void runCalibration(){
    // Extend WDT timeout for heavy Eigen calibration operations
    const esp_task_wdt_config_t calib_wdt = {
        .timeout_ms = 30000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&calib_wdt);

    sh.lock();
    sh.calibrate();
    sh.align();
    sh.unlock();

    // Restore normal WDT timeout
    const esp_task_wdt_config_t normal_wdt = {
        .timeout_ms = 10000,
        .idle_core_mask = 0,
        .trigger_panic = true
    };
    esp_task_wdt_reconfigure(&normal_wdt);
}