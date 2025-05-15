#ifndef DEBUG_HANDLER_H
#define DEBUG_HANDLER_H

#include <Arduino.h>

// Maximum number of log entries to keep
#define DEBUG_MAX_ENTRIES 20
// Maximum length of each debug entry
#define DEBUG_ENTRY_LENGTH 100

// Debug levels
enum DebugLevel {
  DEBUG_INFO = 0,
  DEBUG_WARNING = 1,
  DEBUG_ERROR = 2
};

// Initialize the debug handler
void init_debug_handler();

// Add a debug message to the global buffer
void debug_print(const char* module, const char* message);
void debug_printf(const char* module, const char* format, ...);

// String-safe versions that accept Arduino String objects
void debug_print_safe(const char* module, const String& message);
void debug_print_safe(const String& module, const String& message);
void debug_print_safe(const String& module, const char* message);
void debug_print_safe(const char* module, const char* message);

// Add a debug message with a level
void debug_print_level(DebugLevel level, const char* module, const char* message);
void debug_printf_level(DebugLevel level, const char* module, const char* format, ...);

// Get the current debug data as a string
String get_debug_data();

// Clear all debug entries
void clear_debug_data();

// Update the standard diagnostic data
void update_standard_diagnostics();

// Format telemetry data for diagnostics
void add_telemetry_data(const char* label, float value);
void add_telemetry_data(const char* label, int value);
void add_telemetry_data(const char* label, const String& value);

#endif // DEBUG_HANDLER_H 
