#include "FreeRTOS2.h"


void setup()
{

    Serial.begin(115200);
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Begin beaning...");

    delay(1000);
    xTaskCreatePinnedToCore(
        inputhandler, /* Function to implement the task */
        "inputhandler", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        2 ,  /* Priority of the task */
        &inputhandler_task,  /* Task handle. */
        0); /* Core where the task should run */
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Inputhandler started sucessfully");

    delay(500);
    xTaskCreatePinnedToCore(
        computehandler, /* Function to implement the task */
        "computehandler", /* Name of the task */
        100000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        1 ,  /* Priority of the task */
        &computefunc_task,  /* Task handle. */
        0); /* Core where the task should run */
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "Computehandler started sucessfully");

    delay(500);
    xTaskCreatePinnedToCore(
        displayhandler, /* Function to implement the task */
        "displayhandler", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        3 ,  /* Priority of the task */
        &displayhandler_task,  /* Task handle. */
        0); /* Core where the task should run */
    Debug_csd::debug(Debug_csd::DEBUG_ALWAYS, "displayhandler started sucessfully");

    // delay(250);
    // xTaskCreatePinnedToCore(
    //     inithandler, /* Function to implement the task */
    //     "init", /* Name of the task */
    //     10000,  /* Stack size in words */
    //     NULL,  /* Task input parameter */
    //     tskIDLE_PRIORITY ,  /* Priority of the task */
    //     &init_task,  /* Task handle. */
    //     0); /* Core where the task should run */
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
