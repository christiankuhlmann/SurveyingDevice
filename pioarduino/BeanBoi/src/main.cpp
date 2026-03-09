#include "FreeRTOS2.h"

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    Debug_csd::logf(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_MAIN, "STACK OVERFLOW in task: %s", pcTaskName);
    esp_restart();
}

void setup()
{

    Serial.begin(115200);
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Begin beaning...");

    initialise_device();

    delay(500);
    xTaskCreatePinnedToCore(
        computehandler,
        "computehandler",
        TaskStackSizes::COMPUTE,
        NULL,
        TaskPriorities::COMPUTE,
        &computefunc_task,
        0); // Core 0: Sensor processing
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Computehandler started successfully");

    delay(500);
    xTaskCreatePinnedToCore(
        inputhandler,
        "inputhandler",
        TaskStackSizes::INPUT_HANDLER,
        NULL,
        TaskPriorities::INPUT_HANDLER,
        &inputhandler_task,
        0); // Core 0: Button handling
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Inputhandler started successfully");

    delay(500);
    xTaskCreatePinnedToCore(
        displayhandler,
        "displayhandler",
        TaskStackSizes::DISPLAY_HANDLER,
        NULL,
        TaskPriorities::DISPLAY_HANDLER,
        &displayhandler_task,
        0); // Core 0: OLED refresh
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "displayhandler started successfully");

    startBLETask();

    initialise_interrupts();
}

// extern "C" void app_main()
// {
//     // initialize arduino library before we start the tasks
//     setup();

//     // xTaskCreate(&blink_task, "blink_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
//     // xTaskCreate(&arduinoTask, "arduino_task", configMINIMAL_STACK_SIZE, NULL, 5, NULL);
// }
void loop(){
    delay(1);
}
