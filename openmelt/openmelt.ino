//See melty_config.h for configuration parameters

#include "rc_handler.h"
#include "melty_config.h"
#include "motor_driver.h"
#include "accel_handler.h"
#include "spin_control.h"
#include "config_storage.h"
#include "led_driver.h"
#include "battery_monitor.h"
#include "web_server.h"
#include "debug_handler.h"
#include <WiFi.h>
#include <WebServer.h>
#include <math.h>  // For fabs() function used in normal driving mode

// Web server functionality is controlled by ENABLE_WIFI and ENABLE_WEBSERVER in melty_config.h

#ifdef ENABLE_WATCHDOG
#include <Adafruit_SleepyDog.h>
#endif

// WiFi credentials for the access point
const char* ssid = "Hammertime_AP";
const char* password = "hammertime123";

// Web server on port 80
WebServer server(80);

// Counter to reduce diagnostic update frequency
unsigned long last_diagnostic_update = 0;

void service_watchdog() {
#ifdef ENABLE_WATCHDOG
    Watchdog.reset();
#endif
}

//loops until a good RC signal is detected and throttle is zero (assures safe start)
static void wait_for_rc_good_and_zero_throttle() {
    while (rc_signal_is_healthy() == false || rc_get_throttle_percent() > 0) {
      
      //"slow on/off" for LED while waiting for signal
      heading_led_on(0); delay(250);
      heading_led_off(); delay(250);
      
      //services watchdog and echo diagnostics while we are waiting for RC signal
      service_watchdog();
      echo_diagnostics();
  }
}
  

//Arduino initial setup function
void setup() {
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting OpenMelt...");
  
  // Initialize debug handler first before any debug calls
  init_debug_handler();
  
  debug_print("SYSTEM", "*** OpenMelt starting up... ***");

  //get motor drivers setup (and off!) first thing
  init_motors();
  init_led();

#ifdef ENABLE_WATCHDOG
  //returns actual watchdog timeout MS
  debug_print("SYSTEM", "Enabling watchdog with increased timeout");
  // Use a longer timeout for initial setup
  int watchdog_ms = Watchdog.enable(WATCH_DOG_TIMEOUT_MS); // Increased from 2000ms
  debug_printf("SYSTEM", "Watchdog timeout set to: %d ms", watchdog_ms);
#endif

  init_rc();
  service_watchdog(); // Reset watchdog
  
  init_accel();   //accelerometer uses i2c - which can fail blocking (so only initializing it -after- the watchdog is running)
  service_watchdog(); // Reset watchdog
  
//load settings on boot
#ifdef ENABLE_EEPROM_STORAGE
  debug_print("SYSTEM", "Initializing EEPROM for persistent storage");
  // Initialize EEPROM first before loading settings
  init_eeprom();
  service_watchdog(); // Reset watchdog
  
  debug_print("SYSTEM", "Loading stored configuration");  
  load_melty_config_settings();
  service_watchdog(); // Reset watchdog
#endif

#ifdef ENABLE_WIFI
  // Setup WiFi - using longer delays to ensure stability
  debug_print("WIFI", "Setting up WiFi Access Point...");
  
  // Complete WiFi reset
  WiFi.disconnect(true);
  service_watchdog(); // Reset watchdog
  delay(200);
  
  WiFi.softAPdisconnect(true);
  service_watchdog(); // Reset watchdog
  delay(200);
  
  WiFi.mode(WIFI_OFF);
  service_watchdog(); // Reset watchdog
  delay(500);
  
  // Set WiFi mode explicitly
  debug_print("WIFI", "Setting WiFi mode to AP");
  WiFi.mode(WIFI_AP);
  service_watchdog(); // Reset watchdog
  delay(500);

#ifdef DISABLE_WIFI_POWER_SAVE
  // Disable power saving mode to prevent random GPIO signals
  WiFi.setSleep(false);
  debug_print("WIFI", "WiFi power saving mode disabled");
  service_watchdog(); // Reset watchdog
  delay(100);
#endif
  
  // Create the access point with configured power level
  debug_printf("WIFI", "Creating access point with SSID: %s", ssid);
  WiFi.setTxPower(WIFI_POWER_LEVEL); // Use configured power level
  bool apStarted = WiFi.softAP(ssid, password);
  service_watchdog(); // Reset watchdog
  delay(500); // More time for AP to stabilize
  
  if (apStarted) {
    debug_print("WIFI", "Access point created successfully");
    IPAddress IP = WiFi.softAPIP();
    debug_printf("WIFI", "AP IP address: %s", IP.toString().c_str());
  } else {
    debug_print_level(DEBUG_ERROR, "WIFI", "Failed to create access point!");
  }
  service_watchdog(); // Reset watchdog
  
  // Initialize web server
  debug_print("SYSTEM", "Starting web server task...");
  init_web_server();
  service_watchdog(); // Reset watchdog
#else
  debug_print("WIFI", "WiFi disabled by configuration");
#endif // ENABLE_WIFI
  
  // Give system time to stabilize before continuing
  delay(500);
  
  debug_print("SYSTEM", "Setup complete!");

//if JUST_DO_DIAGNOSTIC_LOOP - then we just loop and display debug info via USB (good for testing)
#ifdef JUST_DO_DIAGNOSTIC_LOOP
  while (1) {
    service_watchdog();
    echo_diagnostics();
    delay(250);   //delay prevents serial from getting flooded (can cause issues programming)
  }
#endif

#if defined(VERIFY_RC_THROTTLE_ZERO_AT_BOOT)
  wait_for_rc_good_and_zero_throttle();     //Wait for good RC signal at zero throttle
  delay(MAX_MS_BETWEEN_RC_UPDATES + 1);     //Wait for first RC signal to have expired
  wait_for_rc_good_and_zero_throttle();     //Verify RC signal is still good / zero throttle
#endif

}

//dumps out diagnostics info
static void echo_diagnostics() {
  // Don't update diagnostics too frequently - at most once every 100ms
  unsigned long current_time = millis();
  if (current_time - last_diagnostic_update < 100) {
    return;
  }
  last_diagnostic_update = current_time;
  
  // Use the new debug handler to update and get diagnostics
  update_standard_diagnostics();
}

//Used to flash out max recorded RPM 100's of RPMs
static void display_rpm_if_requested() {
  //triggered by user pushing throttle up while bot is at idle for 750ms
  if (rc_get_forback_enum() == RC_FORBACK_FORWARD) {
    delay(750);
     //verify throttle at zero to prevent accidental entry into RPM flash
    if (rc_get_forback_enum() == RC_FORBACK_FORWARD && rc_get_throttle_percent() == 0) {
       
      //throttle up cancels RPM count
      for (int x = 0; x < get_max_rpm() && rc_get_throttle_percent() == 0; x = x + 100) {
        service_watchdog();   //flashing out RPM can take a while - need to assure watchdog doesn't trigger
        delay(600); heading_led_on(0);
        delay(20); heading_led_off();
      }
      delay(1500);  //flash-out punctuated with delay to make clear RPM count has completed
    }
  }
}

//checks if user has requested to enter / exit config mode
static void check_config_mode() {
  //if user pulls control stick back for 750ms - enters (or exits) interactive configuration mode
  if (rc_get_forback_enum() == RC_FORBACK_BACKWARD) {
    delay(750);
    if (rc_get_forback_enum() == RC_FORBACK_BACKWARD) {
      toggle_config_mode(); 
      if (get_config_mode() == false) save_melty_config_settings();    //save melty settings on config mode exit
      
      //wait for user to release stick - so we don't re-toggle modes
      while (rc_get_forback_enum() == RC_FORBACK_BACKWARD) {
        service_watchdog();
      }
    }
  }    
}

//handles the bot when not spinning (with RC good)
static void handle_bot_idle() {
    // Original idle behavior
    motors_off();               //assure motors are off
    
    //normal LED "fast flash" - indicates RC signal is good while sitting idle
    heading_led_on(0); delay(30);
    heading_led_off(); delay(120);

    //if in config mode blip LED again to show "double-flash" 
    if (get_config_mode() == true) {
        heading_led_off(); delay(400);
        heading_led_on(0); delay(30);
        heading_led_off(); delay(140);
    }

    check_config_mode();          //check if user requests we enter / exit config mode
    display_rpm_if_requested();   //flashed out RPM if user has requested

    echo_diagnostics();           //echo diagnostics if bot is idle
}

//main control loop
void loop() {

  service_watchdog();             //keep the watchdog happy

  // Static variables for mode switching logic
  static bool in_normal_driving_mode = false;
  static unsigned long last_steering_active_time = 0;
  static unsigned long last_throttle_active_time = 0;

  // Check if RC is healthy before reading values
  if (!rc_signal_is_healthy()) {
    // If RC signal is not healthy, ensure motors are off
    motors_off();
    in_normal_driving_mode = false;
    
    // Show slow flash for no signal
    heading_led_on(0); delay(30);
    heading_led_off(); delay(600);
    
    //services watchdog and echo diagnostics while we are waiting for RC signal
    service_watchdog();
    echo_diagnostics();
    return; // Skip the rest of the loop until RC is healthy
  }

  // RC signal is healthy, proceed with normal operation
  // Get throttle with deadzone
  int throttle_percent = rc_get_throttle_percent();
  bool throttle_is_zero = (throttle_percent <= THROTTLE_DEADZONE_PERCENT);
  
  // If throttle has been activated, record the time
  if (!throttle_is_zero) {
    last_throttle_active_time = millis();
  }

  // Check if we should be in melty mode (throttle above deadzone)
  if (!throttle_is_zero) {
    // Switch to melty mode if throttle is active
    in_normal_driving_mode = false;
    //this is where all the motor control happens!  (see spin_control.cpp)
    spin_one_rotation();  
  } else {
    // Throttle is zero, determine if we should be in normal driving mode or idle
    
    // Get steering stick values
    float steering_x = rc_get_leftright() / 450.0;  // Normalize to -1.0 to 1.0 range
    float steering_y = rc_get_forback() / 450.0;    // Normalize to -1.0 to 1.0 range
    
    // Apply deadzone
    bool steering_x_active = (fabs(steering_x) > NORMAL_DRIVING_MODE_STEERING_DEADZONE);
    bool steering_y_active = (fabs(steering_y) > NORMAL_DRIVING_MODE_STEERING_DEADZONE);
    
    // If steering is active, record the time
    if (steering_x_active || steering_y_active) {
      last_steering_active_time = millis();
    }
    
    // Check if we are in or should switch to normal driving mode
    unsigned long current_time = millis();
    unsigned long steering_inactive_time = current_time - last_steering_active_time;
    unsigned long throttle_inactive_time = current_time - last_throttle_active_time;
    
    // If currently in driving mode, stay in it if steering is active
    if (in_normal_driving_mode) {
      if (steering_x_active || steering_y_active) {
        // Continue normal driving mode
        if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
          // Zero out steering values inside deadzone for smoother control
          if (fabs(steering_x) <= NORMAL_DRIVING_MODE_STEERING_DEADZONE) steering_x = 0.0;
          if (fabs(steering_y) <= NORMAL_DRIVING_MODE_STEERING_DEADZONE) steering_y = 0.0;
          
          normal_driving_mode(steering_x, steering_y);
          
          // LED pattern - quick double blink for driving mode
          heading_led_on(0); delay(20);
          heading_led_off(); delay(80);
          heading_led_on(0); delay(20);
          heading_led_off(); delay(120);
        } else {
          // If not using servo PWM, fall back to idle mode
          in_normal_driving_mode = false;
          handle_bot_idle();
        }
      } 
      else if (steering_inactive_time > MODE_SWITCH_TIMEOUT_MS && 
               throttle_inactive_time > MODE_SWITCH_TIMEOUT_MS) {
        // If steering has been inactive for the timeout period, exit driving mode
        in_normal_driving_mode = false;
        handle_bot_idle();
      }
      else {
        // Steering is inactive but timeout hasn't elapsed, stay in driving mode
        if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
          normal_driving_mode(0.0, 0.0);  // Zero inputs to stop movement
          
          // LED pattern - quick double blink for driving mode
          heading_led_on(0); delay(20);
          heading_led_off(); delay(80);
          heading_led_on(0); delay(20);
          heading_led_off(); delay(120);
        } else {
          // If not using servo PWM, fall back to idle mode
          in_normal_driving_mode = false;
          handle_bot_idle();
        }
      }
    } 
    else {
      // Not in driving mode yet
      if (steering_x_active || steering_y_active) {
        // Steering is active, switch to driving mode
        in_normal_driving_mode = true;
        
        if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
          // Zero out steering values inside deadzone for smoother control
          if (fabs(steering_x) <= NORMAL_DRIVING_MODE_STEERING_DEADZONE) steering_x = 0.0;
          if (fabs(steering_y) <= NORMAL_DRIVING_MODE_STEERING_DEADZONE) steering_y = 0.0;
          
          normal_driving_mode(steering_x, steering_y);
          
          // LED pattern - quick double blink for driving mode
          heading_led_on(0); delay(20);
          heading_led_off(); delay(80);
          heading_led_on(0); delay(20);
          heading_led_off(); delay(120);
        } else {
          // If not using servo PWM, fall back to idle mode
          in_normal_driving_mode = false;
          handle_bot_idle();
        }
      } else {
        // Remain in idle mode
        handle_bot_idle();
      }
    }
  }
}
