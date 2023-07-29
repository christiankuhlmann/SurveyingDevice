#include "RM3100.h"

uint8_t RM3100::readReg(uint8_t addr){
  uint8_t data = 0;
  
  // Enable transmission to specific register to read from
  Wire.beginTransmission(RM3100Address);
  Wire.write(addr); //request from the REVID register
  Wire.endTransmission();

  delay(100);

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
  Wire.begin();
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


void RM3100::update() {
  long x = 0;
  long y = 0;
  long z = 0;
  uint8_t x2,x1,x0,y2,y1,y0,z2,z1,z0;

  //wait until data is ready using 1 of two methods (chosen in options at top of code)
  if(useDRDYPin){ 
    while(digitalRead(pin_drdy) == LOW); //check RDRY pin
  }
  else{
    while((readReg(RM3100_STATUS_REG) & 0x80) != 0x80); //read internal status register
  }

  Wire.beginTransmission(RM3100Address);
  Wire.write(0x24); //request from the first measurement results register
  Wire.endTransmission();

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
  }

  //special bit manipulation since there is not a 24 bit signed int data type
  if (x2 & 0x80){
      x = 0xFF;
  }
  if (y2 & 0x80){
      y = 0xFF;
  }
  if (z2 & 0x80){
      z = 0xFF;
  }

  //format results into single 32 bit signed value
  x = (x * 256 * 256 * 256) | (int32_t)(x2) * 256 * 256 | (uint16_t)(x1) * 256 | x0;
  y = (y * 256 * 256 * 256) | (int32_t)(y2) * 256 * 256 | (uint16_t)(y1) * 256 | y0;
  z = (z * 256 * 256 * 256) | (int32_t)(z2) * 256 * 256 | (uint16_t)(z1) * 256 | z0;

  //calculate magnitude of results
  float uT = sqrt(pow(((float)(x)/gain),2) + pow(((float)(y)/gain),2)+ pow(((float)(z)/gain),2));

  this->mag_data.x_counts = x;
  this->mag_data.y_counts = y;
  this->mag_data.z_counts = z;

  // Use 45uT as basis
  this->mag_data.x_ut = ((float)(x)/gain);// / 45.;
  this->mag_data.y_ut = ((float)(y)/gain);// / 45.;
  this->mag_data.z_ut = ((float)(z)/gain);// / 45.;

  // Serial.printf("RM3100 data [    %f    %f    %f    ]\n", mag_data.x_ut, mag_data.y_ut, mag_data.z_ut);

  // //display results
  // Serial.print("Data in counts:");
  // Serial.print("   X:");
  // Serial.print(x);
  // Serial.print("   Y:");
  // Serial.print(y);
  // Serial.print("   Z:");
  // Serial.println(z);

  // Serial.print("Data in microTesla(uT):");
  // Serial.print("   X:");
  // Serial.print((float)(x)/gain);
  // Serial.print("   Y:");
  // Serial.print((float)(y)/gain);
  // Serial.print("   Z:");
  // Serial.println((float)(z)/gain);

  // //Magnitude should be around 45 uT (+/- 15 uT)
  // Serial.print("Magnitude(uT):");
  // Serial.println(uT);
  // Serial.println();  

}

RM3100::RM3100()
{}

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