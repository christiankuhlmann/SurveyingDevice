

#include "FreeRTOS2.h"
#include <waveshareoled.h>
#include <SensorHandler.h>
#include "RM3100SensorConnection.h"
#include "SCA3300SensorConnection.h"
#include "LDK2MSensorConnection.h"

static RM3100 rm3100;
static SCA3300 sca3300;
static LDK_2M ldk2m;

static SCA3300SensorConnection sc_accelerometer(sca3300);
static RM3100SensorConnection sc_magnetometer(rm3100);
static LDK2MSensorConnection sc_laser(ldk2m);

static SensorHandler sh(sc_accelerometer, sc_magnetometer, sc_laser);


void setup()
{
    Serial.begin(115200);

    sh.init();
    // rm3100.begin();
    rm3100.update();
    Serial.printf("Mag data: %f %f %f\n", rm3100.getX(),rm3100.getY(),rm3100.getZ());

    ldk2m.toggleLaser(true);
    ldk2m.getMeasurement();
    
    System_Init();                                                                                                                                                                                                                                                                                   
    Serial.print(F("OLED_Init()...\r\n"));
    OLED_2IN42_Init();
    Driver_Delay_ms(500); 
    OLED_2IN42_Clear(); 

    //0.Create a new image cache
    UBYTE *BlackImage;
    UWORD Imagesize = ((OLED_2IN42_WIDTH%8==0)? (OLED_2IN42_WIDTH/8): (OLED_2IN42_WIDTH/8+1)) * OLED_2IN42_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) { 
        Serial.print("Failed to apply for black memory...\r\n");
        //return -1;
    }
    Serial.print("Paint_NewImage\r\n");
    Paint_NewImage(BlackImage, OLED_2IN42_WIDTH, OLED_2IN42_HEIGHT, 270, BLACK);  

    //1.Select Image
    Paint_SelectImage(BlackImage);
    Paint_Clear(BLACK);

    // 2.Drawing on the image   
    Serial.print("Drawing:page 1\r\n");
    Paint_DrawPoint(20, 10, WHITE, DOT_PIXEL_1X1, DOT_STYLE_DFT);
    Paint_DrawPoint(30, 10, WHITE, DOT_PIXEL_2X2, DOT_STYLE_DFT);
    Paint_DrawPoint(40, 10, WHITE, DOT_PIXEL_3X3, DOT_STYLE_DFT);
    Paint_DrawLine(10, 10, 10, 20, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(20, 20, 20, 30, WHITE, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
    Paint_DrawLine(30, 30, 30, 40, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawLine(40, 40, 40, 50, WHITE, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
    Paint_DrawCircle(60, 30, 15, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawCircle(100, 40, 20, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);      
    Paint_DrawRectangle(50, 30, 60, 40, WHITE, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
    Paint_DrawRectangle(90, 30, 110, 50, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);   
    // 3.Show image on page1
    OLED_2IN42_Display(BlackImage);
    Driver_Delay_ms(2000);      
    Paint_Clear(BLACK);
    
    // Drawing on the image
    Serial.print("Drawing:page 2\r\n");     
    Paint_DrawString_EN(10, 0, "waveshare", &Font16, WHITE, WHITE);
    Paint_DrawString_EN(10, 17, "hello world", &Font8, WHITE, WHITE);
    Paint_DrawNum(10, 30, "123.456789", &Font8, 4, WHITE, WHITE);
    Paint_DrawNum(10, 43, "987654", &Font12, 5, WHITE, WHITE);
    // Show image on page2
    OLED_2IN42_Display(BlackImage);
    Driver_Delay_ms(2000);  
    Paint_Clear(BLACK);   
    
    // Drawing on the image
    Serial.print("Drawing:page 3\r\n");
    Paint_DrawString_CN(10, 0,"你好Abc", &Font12CN, YELLOW, YELLOW);
    Paint_DrawString_CN(0, 20, "微雪电子", &Font24CN, WHITE, WHITE);
    // Show image on page3
    OLED_2IN42_Display(BlackImage);
    Driver_Delay_ms(2000);    
    Paint_Clear(BLACK); 

    // Drawing on the image
    Serial.print("Drawing:page 4\r\n");
    OLED_2IN42_Display_Array(gImage_1in3);
    Driver_Delay_ms(2000);
    Paint_Clear(BLACK); 

    OLED_2IN42_Clear();  

    
    initInterrupts();
    debug(DEBUG_ALWAYS, "Begin beaning...");

    delay(1000);
    xTaskCreatePinnedToCore(
        inputhandler, /* Function to implement the task */
        "inputhandler", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        2 ,  /* Priority of the task */
        &inputhandler_task,  /* Task handle. */
        0); /* Core where the task should run */
    debug(DEBUG_ALWAYS, "Inputhandler started sucessfully");

    delay(500);
    xTaskCreatePinnedToCore(
        computehandler, /* Function to implement the task */
        "computehandler", /* Name of the task */
        50000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        1 ,  /* Priority of the task */
        &computefunc_task,  /* Task handle. */
        0); /* Core where the task should run */
    debug(DEBUG_ALWAYS, "Computehandler started sucessfully");

    delay(500);
    xTaskCreatePinnedToCore(
        displayhandler, /* Function to implement the task */
        "displayhandler", /* Name of the task */
        10000,  /* Stack size in words */
        NULL,  /* Task input parameter */
        3 ,  /* Priority of the task */
        &displayhandler_task,  /* Task handle. */
        0); /* Core where the task should run */
    debug(DEBUG_ALWAYS, "displayhandler started sucessfully");
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
