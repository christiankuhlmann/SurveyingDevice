#pragma once
#ifndef HEADER_DEBUG_CSD
#define HEADER_DEBUG_CSD

#include <Arduino.h>

namespace Debug_csd {

// --- Log levels (lower = more critical) ---
enum LogLevel : uint8_t {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3,
    LOG_TRACE = 4
};

// Compile-time log level threshold (override via -DLOG_LEVEL=n in platformio.ini)
#ifndef LOG_LEVEL
#define LOG_LEVEL 3  // Default: show ERROR, WARN, INFO, DEBUG; hide TRACE
#endif

// --- ANSI color codes ---
constexpr const char* CLR_RESET = "\033[0m";
constexpr const char* CLR_RED   = "\033[31m";
constexpr const char* CLR_YEL   = "\033[33m";
constexpr const char* CLR_WHT   = "\033[37m";
constexpr const char* CLR_CYN   = "\033[36m";
constexpr const char* CLR_DIM   = "\033[2m";

// --- Category enable flags ---
const static bool DEBUG_FILE_ENA = true;
const static bool DEBUG_LIDAR_ENA = false;
const static bool DEBUG_OLED_ENA = true;
const static bool DEBUG_MAG_ENA = false;
const static bool DEBUG_ACCEL_ENA = false;
const static bool DEBUG_MAIN_ENA = true;
const static bool DEBUG_LIDAR_EXTENDED_ENA = false;
const static bool DEBUG_SENSOR_ENA = false;
const static bool DEBUG_BLE_ENA = true;
const static bool DEBUG_HEAP_ENA = true;

// --- Category IDs ---
const static unsigned int DEBUG_ALWAYS = 0;
const static unsigned int DEBUG_FILE = 1;
const static unsigned int DEBUG_LIDAR = 2;
const static unsigned int DEBUG_OLED = 3;
const static unsigned int DEBUG_MAG = 4;
const static unsigned int DEBUG_ACCEL = 5;
const static unsigned int DEBUG_MAIN = 6;
const static unsigned int DEBUG_LIDAR_EXTENDED = 7;
const static unsigned int DEBUG_SENSOR = 8;
const static unsigned int DEBUG_BLE = 9;
const static unsigned int DEBUG_HEAP = 10;

const static unsigned int NUM_CATEGORIES = 11;

const static bool DEBUG_BOOL_ARR[NUM_CATEGORIES] = {
    true, DEBUG_FILE_ENA, DEBUG_LIDAR_ENA, DEBUG_OLED_ENA, DEBUG_MAG_ENA,
    DEBUG_ACCEL_ENA, DEBUG_MAIN_ENA, DEBUG_LIDAR_EXTENDED_ENA, DEBUG_SENSOR_ENA,
    DEBUG_BLE_ENA, DEBUG_HEAP_ENA
};
const static char DEBUG_STR_ARR[NUM_CATEGORIES][6] = {
    "SYS  ", "FILE ", "LIDAR", "OLED ", "MAG  ", "ACCEL",
    "MAIN ", "LIDEX", "SENSR", "BLE  ", "HEAP "
};

// --- New log API ---
void log(LogLevel level, unsigned int category, const char* str);
void logf(LogLevel level, unsigned int category, const char* format, ...);

// --- Backward-compatible wrappers (map to INFO level) ---
void debug(unsigned int mode, const char* str);
void debugf(unsigned int mode, const char *format, ...);

}

#endif