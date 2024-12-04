#ifndef HEADER_LDK_2M
#define HEADER_LDK_2M

#include <Arduino.h>
#include <HardwareSerial.h>
// Const vars for lidar

const static int LIDAR_READ_SOFTWARE_VERSION = 0x01;
const static int LIDAR_READ_DEVICE_TYPE = 0x02;
const static int LIDAR_READ_SLAVE_ADDR = 0x04;
const static int LIDAR_SET_SLAVE_ADDR = 0x41;
const static int LIDAR_READ_DEVICE_ERROR_CODE = 0x08;
const static int LIDAR_LASER_ON = 0x42;
const static int LIDAR_LASER_OFF = 0x43;
const static int LIDAR_SINGLE_MEAS = 0x44;
const static int LIDAR_CONT_MEAS = 0x45;
const static int LIDAR_STOP_CONT_MEAS = 0x46;
const static int LIDAR_DISABLE_BEEPER = 0x47;
const static int LIDAR_ENABLE_BEEPER = 0xF0;
const static int LIDAR_START_BYTE = 0xAA;
const static int LIDAR_END_BYTE = 0xA8;

const static int LIDAR_SEND_COMMAND_SIZE = 6;
const static int LIDAR_RECEIVE_DATA_MAX_SIZE = 12;

const static int LIDAR_BUFFER_SIZE = 100;
const static int LIDAR_MEAS_LEN = 6;
const static uint8_t PIN_LASER_ENA = 26;

static bool laser_on = true;
static bool uart_timeout = false;
static unsigned long SERIAL1_TIMEOUT_MS = 1000;

// LIDAR message struct
struct lidar_received_msg {
    char address;
    char command;
    char data[LIDAR_RECEIVE_DATA_MAX_SIZE];
};


// Class to deal with all things lidar
class LDK_2M {
    public:
        // Default constructor
        LDK_2M();

        // Initialise lidar module
        void init();

        // Get lidar mesaurement
        float getMeasurement();

        // Toggle laser
        void toggleLaser();

        // Set specific laser mode
        void toggleLaser(bool mode);

        void beep();

    private:
        // Holds a single character - used for reading single char from UART buffer until start bit received
        char single_char_buffer;

        // Larger char buffer to hold message received on UART buffer
        char buffer[LIDAR_BUFFER_SIZE];

        // Length of message received from uart buffer
        int msg_len;

        // First character of message
        char msg_start;

        // Error message
        char* err_msg;

        // Enable lidar via GPIO pin
        void enable();

        // Disable lidar via gpio pin
        void disable();

        // Erase message buffer (variable not actual UART buffer)
        void eraseBuffer();

        // Reads a message from the UART into buffer
        bool readMsgFromUart(char* buffer);

        // Converts a string containing the distance to a float
        float toDistance(char* data);

        // Flush rx
        void flushSerial1();

        // Generate lidar command 
        void generateCommand(int type, char command_arr[LIDAR_SEND_COMMAND_SIZE]);

        // Pack received lidar message into lidar msg struct
        int parseResponse(char raw_data[], lidar_received_msg* receivec_msg);

};

#endif