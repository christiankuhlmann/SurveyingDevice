#include "FreeRTOS2.h"


void setup()
{
    Serial.begin(115200);

    sh.init();
    // rm3100.begin();
    rm3100.update();
    Serial.printf("Mag data: %f %f %f\n", rm3100.getX(),rm3100.getY(),rm3100.getZ());

    sc_accelerometer.getMeasurement();
    Serial.printf("Acc data: %lf %lf %lf\n", sca3300.getCalculatedAccelerometerX(),sca3300.getCalculatedAccelerometerY(),sca3300.getCalculatedAccelerometerZ());

    ldk2m.toggleLaser(true);
    ldk2m.getMeasurement();
    
    // setupOLED();
    initDisplayHandler();
  
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
        50000,  /* Stack size in words */
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

    initInterrupts();
    startDisplayTimer();
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
