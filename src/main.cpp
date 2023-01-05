#include <Arduino.h>
// #include <Adafruit_BNO055.h>

#include <iostream>
#include <math.h>
#include <nvs_flash.h>
#include <string>

#include <Adafruit_BNO055.h>
#include <Adafruit_Sensor.h>
#include <BNO055_support.h>
#include <Wire.h>

#include "filefuncs.h"
#include "lasercalibration.h"
#include "magnetometer.h"
#include "accelerometer.h"
#include "OLED.h"
#include "config.h"
#include "lidar.h"
#include "unified.h"
#include "interrupts.h"
#include "BLE.h"


static Adafruit_SSD1306 display;

// Define sensor structs
static struct bno055_t myBNO;
static struct bno055_gravity myGravityData;
static struct bno055_mag myMagData;

// Static global objects
static Magnetometer magnetometer(&myMagData);
static Accelerometer accelerometer(&myGravityData);
static Lidar lidar;
static BLEHandler blehandler;

// Mainloop flow state enum
enum shot_status {IDLE, WAITING, SPLAY, BASE};
// Holds current state to act on in after interrupt triggered
static shot_status current_state = IDLE;
// Holds next state to act on in after interrupt triggered
static shot_status next_state = IDLE;

// Distance measurement from lidar
static double distance = 0;
// Current shot ID
static int shot_ID = 0;


// File used to store all survey data in
static char current_file_name[] = "test.survey";



/****************************************************
*   All global variables are now declared. Following
*   declerations are for general mainloop functions.
*****************************************************/

// Initialises BNO sensor
void init_bno(){
  //Initialize I2C communication
  Wire.begin();
 
  //Initialization of the BNO055
  BNO_Init(&myBNO); //Assigning the structure to hold information about the device
 
  //Configuration to NDoF mode
  bno055_set_operation_mode(OPERATION_MODE_NDOF);
 
  delay(1);
}



void save_splay(double distance,bool base=false)
{
  // Node struct to hold data to be saved
  node *n = (struct node*)malloc(sizeof(node));

  char str_id[4];
  double heading;
  double inclination;

  debug(DEBUG_MAIN, "Updating sensors");
  magnetometer.update();
  heading = magnetometer.get_heading();
  accelerometer.update();
  inclination = accelerometer.get_inclination();

  // Populate node object
  debug(DEBUG_MAIN, "Assigning values to node object");
  n->id = shot_ID;
  n->heading = heading;
  n->inclination = inclination;

  // Populate str_id with string version of int id
  debug(DEBUG_MAIN, "Writing to file");
  sprintf(str_id,"%d",shot_ID);
  write_to_file(current_file_name,str_id,n);
  
  debug(DEBUG_MAIN, "Finished saving data, returning...");
}

void idle_state()
{
  if (interrupt_button_pressed)
  {
    interrupt_button_pressed = false;
    next_state = WAITING;
    start_shot_interrupt_timer();
  }
}

void waiting_state()
{
  if (interrupt_button_released)
  {
    stop_shot_interrupt_timer();
    interrupt_button_pressed = false;
    interrupt_button_released = false;
    Serial.println("Toggle laser");
    next_state = IDLE;
    lidar.toggle_laser();
  } else if (interrupt_get_shot)
  {
    stop_shot_interrupt_timer();
    interrupt_button_pressed = false;
    interrupt_button_released = false;
    Serial.println("Get shot...");
    next_state = SPLAY;
    debug(DEBUG_MAIN, "Received shot interrupt");
    try
    {
      distance = lidar.get_measurement();
      debug(DEBUG_MAIN, "Succesfully got measurement");
      interrupt_get_shot = false;
    }
    catch(const char* e)
    {
      debug(DEBUG_MAIN, "Failed to got measurement");
      laser_on = false;
      Serial.println(e);
      next_state = IDLE;
    }
  } 
}

void splay_state()
{
  disable_shot_interrupt();
  next_state = IDLE;
  save_splay(distance);

  debug(DEBUG_MAIN,"Reading splay and sending to BLE...");
  node n1;
  char str_id[4];
  snprintf(str_id,sizeof(str_id),"%d",shot_ID);
  read_from_file(current_file_name,str_id,&n1);
  shared_bledata.write(&n1);
  blehandler.update();

  shot_ID += 1;
  enable_shot_interrupt();
}

void interrupt_loop()
{
  current_state = next_state;
  switch (current_state){
    case IDLE:
      idle_state();
      break;

    case WAITING:
      waiting_state();
      break;

    case SPLAY:
      splay_state();
      break;
  }
}

// Runs on power on
void setup(){
  // Initialise serial and UARTs
  Serial.begin(9600);
  Serial1.begin(9600);

  // Set GPIO_14 as LIDAR enable
  pinMode(GPIO_NUM_14, OUTPUT);
  digitalWrite(GPIO_NUM_14,LOW);

  // Initialise OLED displays
  display = init_OLED(0x3C);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.setCursor(0,0);
  display.display();

  // Initialise data objects and sensors
  debug(DEBUG_MAIN,"Initialising sensor objects");
  init_bno();
  magnetometer.init();

  init_interrupts();
  try
  {
    lidar.init();
  }
  catch(const char* e)
  {
    Serial.println(e);
  }

  blehandler.start();
  Serial.println("finished init");

}


// Runs aafter setup
void loop(){
  // TODO: Execute BLE commands which should have a struct made for them.
  // After command executed ack should be sent back such that commands aren't spammed during operation.

  // TODO: Move BLE onto CPU2 and operate all main HW stuff on CPU1. Make sure to init all interrupts on CPU1

  interrupt_loop();
  delay(500);

  // Magnetometer update code
  // magnetometer.update();
  // accelerometer.update();
  // magnetometer.add_calibration_data();
  // Serial.println(magnetometer.check_calibration_progress());
  // delay(50);

}