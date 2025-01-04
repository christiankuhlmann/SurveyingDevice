#include "LDK_2M.h"
#define DEBUG_LDK2M

// Utility functions
void LDK_2M::flushSerial1()
{
    while (Serial1.read() > 0) {delay(10);}
}

void LDK_2M::disable()
{
    digitalWrite(PIN_LASER_ENA,HIGH);
}

void LDK_2M::enable()
{
    digitalWrite(PIN_LASER_ENA,LOW);
}

void LDK_2M::eraseBuffer()
{
    int i;
    for (i=0; i<LIDAR_BUFFER_SIZE;++i)
    {
        buffer[i] = 0x00;
    }
}

float LDK_2M::toDistance(char* data)
{
    static float d;
    if (!strcmp(data,"ERR204") | !strcmp(data,"ERR255") | !strcmp(data,"ERR256"))
    {
        Serial.print("Measurement error: ");
        Serial.print(data);
        Serial.print("\n");
        err_msg = data;
        return -1;
    }

    #ifdef DEBUG_LDK2M
    Serial.print("LDK2M:    Laser data: ");
    Serial.println(data);
    #endif
    
    d = std::strtof(data,nullptr);
    d = d/1000.0;

    return d;
}

// Main functions
LDK_2M::LDK_2M()
{   

}

void LDK_2M::beep()
{
    char generated_command[LIDAR_SEND_COMMAND_SIZE];
    generateCommand(LIDAR_DISABLE_BEEPER,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);
    delay(100);
    generateCommand(LIDAR_ENABLE_BEEPER,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);
    delay(100);
    generateCommand(LIDAR_DISABLE_BEEPER,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);
}

void LDK_2M::init()
{
    // Using UART1
    Serial1.setTimeout(SERIAL1_TIMEOUT_MS);
    Serial1.begin(9600);
    single_char_buffer = { 0x00 };
    *buffer = { 0x00 };
    laser_on = false;

    char generated_command[LIDAR_SEND_COMMAND_SIZE];
    lidar_received_msg received_msg;

    pinMode(PIN_LASER_ENA, OUTPUT);
    digitalWrite(PIN_LASER_ENA,HIGH);

    Serial1.flush();

    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Init 1/3) Get software version");
    #endif

    flushSerial1();
    eraseBuffer();

    // toggleLaser(true);
    delay(100);
    toggleLaser(false);
    delay(100);

    generateCommand(LIDAR_DISABLE_BEEPER,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);
    delay(100);
    generateCommand(LIDAR_ENABLE_BEEPER,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);
    delay(100);
    generateCommand(LIDAR_DISABLE_BEEPER,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);

    flushSerial1();
    eraseBuffer();

    generateCommand(LIDAR_READ_SOFTWARE_VERSION,generated_command);
    Serial1.write(generated_command,LIDAR_SEND_COMMAND_SIZE);

    readMsgFromUart(buffer);
    parseResponse(buffer, &received_msg);
    Serial.printf("Version: %s\n", received_msg.data);

    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Init 2/3) Disable beeper");
    #endif
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Init 3/3) Finished INIT");
    #endif

    flushSerial1();
    eraseBuffer();
};

bool LDK_2M::readMsgFromUart(char* buffer)
{
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Read from UART 1/3) Starting timer");
    #endif
    // while (Serial1.readBytes(&single_char_buffer,1))
    // {
    //     Serial.printf("%X\n",single_char_buffer);
    // }

    // Read buffer until start-byte found
    single_char_buffer = LIDAR_START_BYTE;
    Serial1.readBytesUntil(LIDAR_START_BYTE,&single_char_buffer,1);
    while (single_char_buffer == LIDAR_START_BYTE)
    {
        Serial1.readBytes(&single_char_buffer,1);
    }
    msg_start = single_char_buffer;
    // Erase buffer to remove the start byte and any bad ata
    eraseBuffer();

    // Reads bytes until terminator into buffer (not including terminator)
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Read from UART 2/3) Reading data");
    #endif
    
    // TODO: check 99 length, it this necessary?
    // Read bytes until end byte is found
    msg_len = Serial1.readBytesUntil(LIDAR_END_BYTE,buffer,99);
    if (msg_len == 0)
    {
        #ifdef DEBUG_LDK2M
        Serial.println("LDK2M:    (Read from UART 2/3) timer expired, read failed");
        #endif
        return 0;
    }

    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Read from UART 3/3) Succesfully read data");
    #endif

    return 1;
}

void LDK_2M::generateCommand(int type, char command_packet[LIDAR_SEND_COMMAND_SIZE])
{
    static char address;
    static char command;
    static char data;
    static char checksum;

    address = 0x01;
    command = 0;
    data = 0xFF;

    
    switch (type){
        case LIDAR_READ_SOFTWARE_VERSION:
            command = LIDAR_READ_SOFTWARE_VERSION;
            address = 0x00;
            checksum = 0x01;
            break;
        case LIDAR_READ_DEVICE_TYPE:
            command = LIDAR_READ_DEVICE_TYPE;
            checksum = 0x03;
            break;
        case LIDAR_READ_SLAVE_ADDR:
            command = LIDAR_READ_SLAVE_ADDR;
            address = 0x00;
            checksum = 0x04;
            break;
        case LIDAR_SET_SLAVE_ADDR:
            command = LIDAR_SET_SLAVE_ADDR;
            address = 0x00;
            data = 0x01;
            checksum = 0x43;
            break;
        case LIDAR_READ_DEVICE_ERROR_CODE:
            command = LIDAR_READ_DEVICE_ERROR_CODE;
            checksum = 0x09;
            break;
        case LIDAR_LASER_ON:
            command = LIDAR_LASER_ON;
            checksum = 0x43;
            break;
        case LIDAR_LASER_OFF:
            command = LIDAR_LASER_OFF;
            checksum = 0x44;
            break;
        case LIDAR_SINGLE_MEAS:
            command = LIDAR_SINGLE_MEAS;
            checksum = 0x45;
            break;
        case LIDAR_CONT_MEAS:
            command = LIDAR_CONT_MEAS;
            checksum = 0x46;
            break;
        case LIDAR_STOP_CONT_MEAS:
            command = LIDAR_STOP_CONT_MEAS;
            checksum = 0x49;
            break;
        case LIDAR_DISABLE_BEEPER:
            command = LIDAR_DISABLE_BEEPER;
            data = 0x00;
            checksum = 0x48;
            break;
        case LIDAR_ENABLE_BEEPER:
            command = LIDAR_DISABLE_BEEPER;
            data = 0x01;
            checksum = 0x49;
            break;
    }
    
    command_packet[0] = LIDAR_START_BYTE;
    command_packet[1] = address;
    command_packet[2] = command;

    if ((int)data != (int)0xFF)
    {
        command_packet[3] = data;
        command_packet[4] = checksum;
        command_packet[5] = LIDAR_END_BYTE;
    }
    else
    {
        command_packet[3] = checksum;
        command_packet[4] = LIDAR_END_BYTE;
        command_packet[5] = 0x00;
    }
    
    #ifdef DEBUG_LDK2M
    Serial.printf("LDK2M:    (Generate command 1/1) Generated command: %X %X %X %X %X %X\n", command_packet[0],command_packet[1],command_packet[2],command_packet[3],command_packet[4],command_packet[5]);
    #endif
};

int LDK_2M::parseResponse(char raw_message[], lidar_received_msg* msg)
{
    static int i;

    static int data_size; // Size of data in message (i.e. not address, command, or checksum)
    static int start; // Loop start position
    static char checksum; // Received checksum
    static unsigned int calculated_checksum; // Calculated checksum

    // Assign address and command form raw message
    msg->address = msg_start;
    msg->command = raw_message[0];
    
    // Size of received message => command, data[MAX 12], checksum
    data_size = msg_len - 2;

    // Start is 2 instead of 0 due to command
    start = 1;

    // Loop to populate the data array
    // Serial.printf("Msg address: %X\n",msg->address);
    // Serial.printf("Msg command: %X\n",msg->command);
    // Serial.print("Message: ");
    for (i=start;i<start+LIDAR_RECEIVE_DATA_MAX_SIZE;++i)
    {
        if (i<start+data_size)
        {
            // Serial.printf("%X ", raw_message[i]);
            msg->data[i-start] = raw_message[i];
        } else {
            msg->data[i-start] = 0;
        }
    }
    // Serial.print("\n");

    
    
    // Populate checksum from raw_message
    checksum = raw_message[start+data_size];
    // Serial.printf("Checksum: %X\n", checksum);

    // Calculate checksum
    calculated_checksum = 0x00;
    calculated_checksum += (unsigned int)msg->address;
    calculated_checksum += (unsigned int)msg->command;
    
    for (i=0;i<data_size;++i)
    {
        calculated_checksum += (unsigned int)msg->data[i];
    }
    calculated_checksum = calculated_checksum & (unsigned int)0x7F;

    // Serial.printf("Calculated checksum: %X\n", calculated_checksum);

    // validate checksum
    if ((char)calculated_checksum != checksum)
    {
        #ifdef DEBUG_LDK2M
        Serial.printf("LDK2M:    Checksum Invaid! %X != %X\n",(char) calculated_checksum, checksum);
        #endif

        Serial.println("LDK2M:    Checksum invalid!");
        return 1;
    }
    return 0;
}

float LDK_2M::getMeasurement()
{
    
    static float distance; // Distance returned by LIDAR
    static char generated_command[LIDAR_SEND_COMMAND_SIZE]; // Command to send to LIDAR
    lidar_received_msg received_msg;

    distance = -1.0;

    // Generate lidar single measurement command and send
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Get measurement 1/4) Request single measure");
    #endif

    generateCommand(LIDAR_SINGLE_MEAS,generated_command);

    // Flush the hardware UART buffer to remove old messages
    flushSerial1();
    // Flush the software message buffer to prevent erroneous reading
    eraseBuffer();

    // Send command to LIDAR
    Serial1.write(generated_command);

    // Parse resonse
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Get measurement 2/4) Read response");
    #endif

    delay(10);
    if (!readMsgFromUart(buffer))
    {
        return 0;
    }

    // Attempt to parse the message received over UART
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Get measurement 3/4) Parse read response");
    #endif

    
    if (!parseResponse(buffer,&received_msg))
    {
        distance = toDistance(received_msg.data);
    } else {
        Serial.println("Parsing failed!");
        flushSerial1();
        eraseBuffer();
        return -1;
    }
    

    // Return result
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Get measurement 4/4) Return response");
    #endif


    flushSerial1();
    eraseBuffer();
    
    return distance;
}

void LDK_2M::toggleLaser()
{
    char generated_command[LIDAR_SEND_COMMAND_SIZE];

    // Generate lidar LASER ON command and send
    // TODO: check whether receive response before changing laser status
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Toggle laser 1/1) Toggle laser");
    #endif
    
    if (laser_on)
    {
        generateCommand(LIDAR_LASER_OFF,generated_command);
        laser_on = false;
    } else {
        generateCommand(LIDAR_LASER_ON,generated_command);
        laser_on = true;
    }
    Serial1.write(generated_command);

    flushSerial1();
    eraseBuffer();
}

void LDK_2M::toggleLaser(bool mode)
{
    char generated_command[LIDAR_SEND_COMMAND_SIZE];

    // Generate lidar LASER ON command and send
    // TODO: check whether receive response before changing laser status
    #ifdef DEBUG_LDK2M
    Serial.println("LDK2M:    (Toggle laser 1/3) Toggle laser");
    #endif
    
    if (mode)
    {
        Serial.println("LDK2M:    (Toggle laser 2/3) Toggle ON");
        generateCommand(LIDAR_LASER_ON,generated_command);
        laser_on = true;
    } else {
        Serial.println("LDK2M:    (Toggle laser 2/3) Toggle OFF");
        generateCommand(LIDAR_LASER_OFF,generated_command);
        laser_on = false;
    }
    Serial1.write(generated_command);
    Serial1.write(generated_command);
    Serial.println("LDK2M:    (Toggle laser 3/3) Send command");

    flushSerial1();
    eraseBuffer();
}