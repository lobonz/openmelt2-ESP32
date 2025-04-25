#include "debug_handler.h"
#include "rc_handler.h"
#include "accel_handler.h"
#include "motor_driver.h"
#include "spin_control.h"
#include "config_storage.h"
#include "battery_monitor.h"
#include "web_server.h"
#include <stdarg.h>

// Time-based buffer to avoid flooding
#define MIN_MS_BETWEEN_ENTRIES 50
static unsigned long last_debug_entry_time = 0;

// Circular buffer for debug entries
typedef struct {
  char message[DEBUG_ENTRY_LENGTH];
  DebugLevel level;
  unsigned long timestamp;
  bool used;
} DebugEntry;

static DebugEntry debug_entries[DEBUG_MAX_ENTRIES];
static int debug_write_index = 0;

// Standard telemetry string
static char telemetry_data[512] = "";
// Store the latest accelerometer values
static float latest_accel_x = 0.0f;
static float latest_accel_y = 0.0f;
static float latest_accel_z = 0.0f;
static float latest_accel_used = 0.0f;

// Mutex for protecting access to the debug data
portMUX_TYPE debugMux = portMUX_INITIALIZER_UNLOCKED;

// Flag to indicate debug is initialized
volatile bool debug_initialized = false;

// Flag to track if the web update is needed
static bool web_update_needed = false;
static unsigned long last_web_update = 0;

void init_debug_handler() {
  // Initialize the debug entries
  portENTER_CRITICAL(&debugMux);
  for (int i = 0; i < DEBUG_MAX_ENTRIES; i++) {
    debug_entries[i].used = false;
    debug_entries[i].message[0] = '\0';
  }
  telemetry_data[0] = '\0';
  debug_initialized = true;
  portEXIT_CRITICAL(&debugMux);
  
  Serial.println("Debug handler initialized");
}

// Update the latest accelerometer values
void update_accel_values(float x, float y, float z, float used) {
  portENTER_CRITICAL(&debugMux);
  latest_accel_x = x;
  latest_accel_y = y;
  latest_accel_z = z;
  latest_accel_used = used;
  portEXIT_CRITICAL(&debugMux);
  
  // Mark that we need a web update with the new accel data
  web_update_needed = true;
}

// Internal helper to add a message to the buffer
static void add_debug_entry(DebugLevel level, const char* module, const char* message) {
  // Check if debug system is initialized
  if (!debug_initialized) {
    Serial.print("[");
    Serial.print(module);
    Serial.print("] ");
    Serial.println(message);
    return;
  }

  // Rate limit debug entries
  unsigned long current_time = millis();
  if (current_time - last_debug_entry_time < MIN_MS_BETWEEN_ENTRIES) {
    return;
  }
  last_debug_entry_time = current_time;
  
  // Check for accelerometer raw data in the message
  if (strcmp(module, "ACCEL") == 0) {
    // Check if this is a raw accelerometer value message
    if (strstr(message, "Raw Accel - X:") != NULL) {
      // Parse accelerometer values
      float x = 0.0f, y = 0.0f, z = 0.0f, used = 0.0f;
      
      // Very simple parsing - just extract the values
      const char* x_pos = strstr(message, "X: ");
      const char* y_pos = strstr(message, "Y: ");
      const char* z_pos = strstr(message, "Z: ");
      const char* used_pos = strstr(message, "Used value: ");
      
      if (x_pos && y_pos && z_pos && used_pos) {
        x = atof(x_pos + 3);
        y = atof(y_pos + 3);
        z = atof(z_pos + 3);
        used = atof(used_pos + 12);
        
        // Update the accelerometer values
        update_accel_values(x, y, z, used);
      }
    }
  }
  
  // Format the message with module name prefix
  char formatted_message[DEBUG_ENTRY_LENGTH];
  snprintf(formatted_message, DEBUG_ENTRY_LENGTH, "[%s] %s", module, message);
  
  // Output to serial immediately (doesn't need mutex protection)
  Serial.println(formatted_message);
  
  // Store in circular buffer with critical section protection
  portENTER_CRITICAL(&debugMux);
  debug_entries[debug_write_index].level = level;
  debug_entries[debug_write_index].timestamp = current_time;
  debug_entries[debug_write_index].used = true;
  strncpy(debug_entries[debug_write_index].message, formatted_message, DEBUG_ENTRY_LENGTH - 1);
  debug_entries[debug_write_index].message[DEBUG_ENTRY_LENGTH - 1] = '\0';
  
  // Move to next slot in circular buffer
  debug_write_index = (debug_write_index + 1) % DEBUG_MAX_ENTRIES;
  
  // Mark that web data needs updating, but don't do it here
  web_update_needed = true;
  
  portEXIT_CRITICAL(&debugMux);
}

void debug_print(const char* module, const char* message) {
  debug_print_level(DEBUG_INFO, module, message);
}

void debug_printf(const char* module, const char* format, ...) {
  char buffer[DEBUG_ENTRY_LENGTH];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, DEBUG_ENTRY_LENGTH, format, args);
  va_end(args);
  
  debug_print(module, buffer);
}

void debug_print_level(DebugLevel level, const char* module, const char* message) {
  add_debug_entry(level, module, message);
}

void debug_printf_level(DebugLevel level, const char* module, const char* format, ...) {
  char buffer[DEBUG_ENTRY_LENGTH];
  va_list args;
  va_start(args, format);
  vsnprintf(buffer, DEBUG_ENTRY_LENGTH, format, args);
  va_end(args);
  
  debug_print_level(level, module, buffer);
}

String get_debug_data() {
  String result;
  
  // Make sure debug system is initialized
  if (!debug_initialized) {
    return "Debug system not initialized";
  }
  
  // Get telemetry data with mutex protection
  portENTER_CRITICAL(&debugMux);
  result = telemetry_data;
  
  // Add accelerometer values explicitly to help parsing
  if (latest_accel_x != 0.0f || latest_accel_y != 0.0f || latest_accel_z != 0.0f) {
    char accelBuffer[128];
    snprintf(accelBuffer, sizeof(accelBuffer), " X: %.2fg, Y: %.2fg, Z: %.2fg, Used value: %.2fg", 
             latest_accel_x, latest_accel_y, latest_accel_z, latest_accel_used);
    result += accelBuffer;
  }
  
  result += "\n\n";
  
  // Add the most recent entries
  for (int i = 0; i < DEBUG_MAX_ENTRIES; i++) {
    int idx = (debug_write_index - 1 - i + DEBUG_MAX_ENTRIES) % DEBUG_MAX_ENTRIES;
    if (debug_entries[idx].used) {
      // Add level indicator
      const char* level_str = "";
      switch (debug_entries[idx].level) {
        case DEBUG_WARNING: level_str = "⚠️ "; break;
        case DEBUG_ERROR: level_str = "🛑 "; break;
        default: level_str = ""; break;
      }
      
      result += level_str;
      result += debug_entries[idx].message;
      result += "\n";
    }
  }
  portEXIT_CRITICAL(&debugMux);
  
  return result;
}

void clear_debug_data() {
  // Make sure debug system is initialized
  if (!debug_initialized) return;
  
  portENTER_CRITICAL(&debugMux);
  for (int i = 0; i < DEBUG_MAX_ENTRIES; i++) {
    debug_entries[i].used = false;
  }
  telemetry_data[0] = '\0';
  portEXIT_CRITICAL(&debugMux);
}

void update_standard_diagnostics() {
  // Check if debug system is initialized
  if (!debug_initialized) return;
  
  // Don't update the web too frequently (max once every 250ms)
  unsigned long current_time = millis();
  if (current_time - last_web_update < 250 && !web_update_needed) {
    return;
  }
  
  last_web_update = current_time;
  web_update_needed = false;
  
  // Build new telemetry string outside critical section
  char newTelemetry[512] = "";
  char buffer[64];
  
  // Add telemetry data with safer string handling
  snprintf(buffer, sizeof(buffer), "Raw Accel G: %.2f  ", get_accel_force_g());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  snprintf(buffer, sizeof(buffer), "RC Health: %d  ", rc_signal_is_healthy());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  snprintf(buffer, sizeof(buffer), "RC Throttle: %d  ", rc_get_throttle_percent());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  snprintf(buffer, sizeof(buffer), "RC L/R: %d  ", rc_get_leftright());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  snprintf(buffer, sizeof(buffer), "RC F/B: %d  ", rc_get_forback());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  // Add motor PWM values if using servo PWM throttle
  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    snprintf(buffer, sizeof(buffer), "Motor1 PWM: %dus  ", get_motor1_pulse_width());
    strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
    
    snprintf(buffer, sizeof(buffer), "Motor2 PWM: %dus  ", get_motor2_pulse_width());
    strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  }
  
#ifdef BATTERY_ALERT_ENABLED
  snprintf(buffer, sizeof(buffer), "Battery: %.2fV  ", get_battery_voltage());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
#endif 
  
#ifdef ENABLE_EEPROM_STORAGE  
  snprintf(buffer, sizeof(buffer), "Radius: %.2f  ", load_accel_mount_radius());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  snprintf(buffer, sizeof(buffer), "Heading: %d  ", load_heading_led_offset());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
  
  snprintf(buffer, sizeof(buffer), "Zero G: %.2f  ", load_accel_zero_g_offset());
  strncat(newTelemetry, buffer, sizeof(newTelemetry) - strlen(newTelemetry) - 1);
#endif

  // Update telemetry in critical section
  portENTER_CRITICAL(&debugMux);
  strncpy(telemetry_data, newTelemetry, sizeof(telemetry_data) - 1);
  telemetry_data[sizeof(telemetry_data) - 1] = '\0';
  portEXIT_CRITICAL(&debugMux);

  // Now update the web data with all current data
  String completeData = get_debug_data();
  int separatorIndex = completeData.indexOf("\n\n");
  if (separatorIndex != -1) {
    String telemetry = completeData.substring(0, separatorIndex);
    String logs = completeData.substring(separatorIndex + 2);
    update_web_data(telemetry, logs);
  } else {
    update_web_data(completeData, "");
  }
}

// These helper functions are no longer used, but kept for API compatibility
void add_telemetry_data(const char* label, float value) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s: %.2f", label, value);
  
  portENTER_CRITICAL(&debugMux);
  strncat(telemetry_data, buffer, sizeof(telemetry_data) - strlen(telemetry_data) - 1);
  strncat(telemetry_data, "  ", sizeof(telemetry_data) - strlen(telemetry_data) - 1);
  portEXIT_CRITICAL(&debugMux);
}

void add_telemetry_data(const char* label, int value) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s: %d", label, value);
  
  portENTER_CRITICAL(&debugMux);
  strncat(telemetry_data, buffer, sizeof(telemetry_data) - strlen(telemetry_data) - 1);
  strncat(telemetry_data, "  ", sizeof(telemetry_data) - strlen(telemetry_data) - 1);
  portEXIT_CRITICAL(&debugMux);
}

void add_telemetry_data(const char* label, const String& value) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "%s: %s", label, value.c_str());
  
  portENTER_CRITICAL(&debugMux);
  strncat(telemetry_data, buffer, sizeof(telemetry_data) - strlen(telemetry_data) - 1);
  strncat(telemetry_data, "  ", sizeof(telemetry_data) - strlen(telemetry_data) - 1);
  portEXIT_CRITICAL(&debugMux);
} 
