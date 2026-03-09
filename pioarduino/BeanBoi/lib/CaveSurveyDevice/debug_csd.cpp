#include "debug_csd.h"
#include <stdarg.h>

namespace Debug_csd
{

// Level tags (3 chars each)
static const char* LEVEL_TAGS[] = { "ERR", "WRN", "INF", "DBG", "TRC" };

// ANSI color per level
static const char* LEVEL_COLORS[] = { CLR_RED, CLR_YEL, CLR_WHT, CLR_CYN, CLR_DIM };

// Returns true if a message at `level` for `category` should be emitted
static inline bool shouldLog(LogLevel level, unsigned int category) {
    // Compile-time level filter
    if (level > LOG_LEVEL) return false;
    // ERROR always shown regardless of category
    if (level == LOG_ERROR) return true;
    // Category filter
    if (category < NUM_CATEGORIES) return DEBUG_BOOL_ARR[category];
    return false;
}

void log(LogLevel level, unsigned int category, const char* str)
{
    if (!shouldLog(level, category)) return;

    const char* tag = (category < NUM_CATEGORIES) ? DEBUG_STR_ARR[category] : "???? ";
    Serial.printf("%s[%7lums] [%s] %s: %s%s\n",
        LEVEL_COLORS[level], millis(), LEVEL_TAGS[level], tag, str, CLR_RESET);
}

void logf(LogLevel level, unsigned int category, const char* format, ...)
{
    if (!shouldLog(level, category)) return;

    const char* tag = (category < NUM_CATEGORIES) ? DEBUG_STR_ARR[category] : "???? ";
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("%s[%7lums] [%s] %s: %s%s\n",
        LEVEL_COLORS[level], millis(), LEVEL_TAGS[level], tag, buffer, CLR_RESET);
}

// Backward-compatible wrappers — map to INFO level
void debug(unsigned int mode, const char* str)
{
    log(LOG_INFO, mode, str);
}

void debugf(unsigned int mode, const char *format, ...)
{
    if (!shouldLog(LOG_INFO, mode)) return;

    const char* tag = (mode < NUM_CATEGORIES) ? DEBUG_STR_ARR[mode] : "???? ";
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.printf("%s[%7lums] [%s] %s: %s%s\n",
        LEVEL_COLORS[LOG_INFO], millis(), LEVEL_TAGS[LOG_INFO], tag, buffer, CLR_RESET);
}

}