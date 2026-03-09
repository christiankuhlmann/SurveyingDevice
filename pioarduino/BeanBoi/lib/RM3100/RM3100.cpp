#include "RM3100.h"
#include "debug_csd.h"

uint8_t RM3100::readReg(uint8_t addr){
  uint8_t data = 0;
  
  // Enable transmission to specific register to read from
  Wire.beginTransmission(RM3100Address);
  Wire.write(addr); //request from the REVID register
  Wire.endTransmission();

  vTaskDelay(pdMS_TO_TICKS(100));

  // Request 1 byte from the register specified earlier
  Wire.requestFrom(RM3100Address, 1);
  if(Wire.available() == 1) {
    data = Wire.read();
  }
  return data;
}

//addr is the 7 bit (No r/w bit) value of the internal register's address, data is 8 bit data being written
void RM3100::writeReg(uint8_t addr, uint8_t data){
  Wire.beginTransmission(RM3100Address);
  Wire.write(addr);
  Wire.write(data);
  Wire.endTransmission();
}

//newCC is the new cycle count value (16 bits) to change the data acquisition
void RM3100::changeCycleCount(uint16_t newCC){
  uint8_t CCMSB = (newCC & 0xFF00) >> 8; //get the most significant byte
  uint8_t CCLSB = newCC & 0xFF; //get the least significant byte
  
  Wire.beginTransmission(RM3100Address);
  Wire.write(RM3100_CCX1_REG);
  Wire.write(CCMSB);  //write new cycle count to ccx1
  Wire.write(CCLSB);  //write new cycle count to ccx0
  Wire.write(CCMSB);  //write new cycle count to ccy1
  Wire.write(CCLSB);  //write new cycle count to ccy0
  Wire.write(CCMSB);  //write new cycle count to ccz1
  Wire.write(CCLSB);  //write new cycle count to ccz0     
  Wire.endTransmission();  
}


void RM3100::begin(){
  Wire.begin(23,22);
  pinMode(pin_drdy, INPUT);
  revid = readReg(RM3100_REVID_REG);
  
  changeCycleCount(initialCC); //change the cycle count; default = 200 (lower cycle count = higher data rates but lower resolution)
  
  
  cycleCount = readReg(RM3100_CCX1_REG);
  cycleCount = (cycleCount << 8) | readReg(RM3100_CCX0_REG);

  gain = (0.3671 * (float)cycleCount) + 1.5; //linear equation to calculate the gain from cycle count

  if (singleMode){
    //set up single measurement mode
    writeReg(RM3100_CMM_REG, 0);
    writeReg(RM3100_POLL_REG, 0x70);
  }
  else{
    // Enable transmission to take continuous measurement with Alarm functions off
    writeReg(RM3100_CMM_REG, 0x79);
  }
}

void RM3100::begin(uint8_t pin)
{
  this->pin_drdy = pin;
  this->begin();
}

void RM3100::begin(bool usedrdy)
{
  this->useDRDYPin = usedrdy;
  this->begin();
}


bool RM3100::update() {
  long x = 0;
  long y = 0;
  long z = 0;
  uint8_t x2,x1,x0,y2,y1,y0,z2,z1,z0;

  //wait until data is ready using 1 of two methods (chosen in options at top of code)
  if(useDRDYPin){ 
    unsigned long timeout = millis() + 1000; // 1 second timeout
    while(digitalRead(pin_drdy) == LOW && millis() < timeout); //check RDRY pin
    if(millis() >= timeout) {
      Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_MAG, "DRDY pin timeout, data may be stale");
    }
  }
  else{
    unsigned long timeout = millis() + 1000; // 1 second timeout
    while((readReg(RM3100_STATUS_REG) & 0x80) != 0x80 && millis() < timeout); //read internal status register
    if(millis() >= timeout) {
      Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_MAG, "Status register timeout, data may be stale");
    }
  }

  Wire.beginTransmission(RM3100Address);
  Wire.write(0x24); //request from the first measurement results register
  if (Wire.endTransmission() != 0) {
    Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_MAG, "I2C transmission error");
    return false;
  }

  // Request 9 bytes from the measurement results registers
  Wire.requestFrom(RM3100Address, 9);
  if(Wire.available() == 9) {
    x2 = Wire.read();
    x1 = Wire.read();
    x0 = Wire.read();
    
    y2 = Wire.read();
    y1 = Wire.read();
    y0 = Wire.read();
    
    z2 = Wire.read();
    z1 = Wire.read();
    z0 = Wire.read();
  } else {
    Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_MAG, "I2C read failed, expected 9 bytes");
    return false;
  }

  // Sign-extend 24-bit values into 32-bit signed integers
  x = ((int32_t)(int8_t)x2 << 16) | ((uint32_t)x1 << 8) | x0;
  y = ((int32_t)(int8_t)y2 << 16) | ((uint32_t)y1 << 8) | y0;
  z = ((int32_t)(int8_t)z2 << 16) | ((uint32_t)z1 << 8) | z0;

  this->mag_data.x_counts = x;
  this->mag_data.y_counts = y;
  this->mag_data.z_counts = z;

  // Use 45uT as basis
  this->mag_data.x_ut = ((float)(x)/gain);// / 45.;
  this->mag_data.y_ut = ((float)(y)/gain);// / 45.;
  this->mag_data.z_ut = ((float)(z)/gain);// / 45.;

  return true;
}

RM3100::RM3100(){}

float RM3100::getX()
{
  return this->mag_data.x_ut;
}
float RM3100::getY()
{
  return this->mag_data.y_ut;
}
float RM3100::getZ()
{
  return this->mag_data.z_ut;
}