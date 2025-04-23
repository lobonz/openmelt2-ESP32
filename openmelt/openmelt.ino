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
#include <WiFi.h>
#include <WebServer.h>

// Define to enable web server without requiring RC signal
#define ENABLE_WEBSERVER_STANDALONE

#ifdef ENABLE_WATCHDOG
#include <Adafruit_SleepyDog.h>
#endif

// WiFi credentials for the access point
const char* ssid = "Hammertime_AP";
const char* password = "hammertime123";

// Web server on port 80
WebServer server(80);

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
  Serial.println("\n\n*** OpenMelt starting up... ***");

  //get motor drivers setup (and off!) first thing
  init_motors();
  init_led();

#ifdef ENABLE_WATCHDOG
  //returns actual watchdog timeout MS
  Serial.println("Enabling watchdog with increased timeout");
  // Use a longer timeout for initial setup
  int watchdog_ms = Watchdog.enable(5000); // Increased from 2000ms
  Serial.print("Watchdog timeout set to: ");
  Serial.print(watchdog_ms);
  Serial.println(" ms");
#endif

  init_rc();
  service_watchdog(); // Reset watchdog
  
  init_accel();   //accelerometer uses i2c - which can fail blocking (so only initializing it -after- the watchdog is running)
  service_watchdog(); // Reset watchdog
  
//load settings on boot
#ifdef ENABLE_EEPROM_STORAGE  
  load_melty_config_settings();
  service_watchdog(); // Reset watchdog
#endif

  // Setup WiFi AP BEFORE RC checks so web server works even without RC controller
  Serial.println("*** Setting up WiFi Access Point... ***");
  
  // Complete WiFi reset
  WiFi.disconnect(true);
  service_watchdog(); // Reset watchdog
  
  WiFi.softAPdisconnect(true);
  service_watchdog(); // Reset watchdog
  
  delay(500);
  service_watchdog(); // Reset watchdog
  
  // Set WiFi mode explicitly
  Serial.println("Setting WiFi mode to AP");
  WiFi.mode(WIFI_AP);
  service_watchdog(); // Reset watchdog
  
  delay(500);
  service_watchdog(); // Reset watchdog
  
  // Create the access point
  Serial.print("Creating access point with SSID: ");
  Serial.println(ssid);
  bool apStarted = WiFi.softAP(ssid, password);
  service_watchdog(); // Reset watchdog
  
  if (apStarted) {
    Serial.println("*** Access point created successfully ***");
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
  } else {
    Serial.println("*** Failed to create access point! ***");
  }
  service_watchdog(); // Reset watchdog
  
  // Initialize web server
  Serial.println("Starting web server task...");
  init_web_server();
  service_watchdog(); // Reset watchdog
  
  Serial.println("*** Setup complete! ***");

//if JUST_DO_DIAGNOSTIC_LOOP - then we just loop and display debug info via USB (good for testing)
#ifdef JUST_DO_DIAGNOSTIC_LOOP
  while (1) {
    service_watchdog();
    echo_diagnostics();
    delay(250);   //delay prevents serial from getting flooded (can cause issues programming)
  }
#endif

// Only check for RC signal if we're not running the web server standalone
#if defined(VERIFY_RC_THROTTLE_ZERO_AT_BOOT) && !defined(ENABLE_WEBSERVER_STANDALONE)
  wait_for_rc_good_and_zero_throttle();     //Wait for good RC signal at zero throttle
  delay(MAX_MS_BETWEEN_RC_UPDATES + 1);     //Wait for first RC signal to have expired
  wait_for_rc_good_and_zero_throttle();     //Verify RC signal is still good / zero throttle
#endif
}

//dumps out diagnostics info
static void echo_diagnostics() {
  String diagnosticData = "";
  
  diagnosticData += "Raw Accel G: " + String(get_accel_force_g());
  diagnosticData += "  RC Health: " + String(rc_signal_is_healthy());
  diagnosticData += "  RC Throttle: " + String(rc_get_throttle_percent());
  diagnosticData += "  RC L/R: " + String(rc_get_leftright());
  diagnosticData += "  RC F/B: " + String(rc_get_forback());

#ifdef BATTERY_ALERT_ENABLED
  diagnosticData += "  Battery Voltage: " + String(get_battery_voltage());
#endif 
  
#ifdef ENABLE_EEPROM_STORAGE  
  diagnosticData += "  Accel Radius: " + String(load_accel_mount_radius());
  diagnosticData += "  Heading Offset: " + String(load_heading_led_offset());
  diagnosticData += "  Zero G Offset: " + String(load_accel_zero_g_offset());
#endif  
  diagnosticData += "\n";

  Serial.print(diagnosticData);
  
  // Update the web server with the latest data
  update_web_data(diagnosticData);
}

//Used to flash out max recorded RPM 100's of RPMs
static void display_rpm_if_requested() {
  //triggered by user pushing throttle up while bot is at idle for 750ms
  if (rc_get_forback() == RC_FORBACK_FORWARD) {
    delay(750);
     //verify throttle at zero to prevent accidental entry into RPM flash
    if (rc_get_forback() == RC_FORBACK_FORWARD && rc_get_throttle_percent() == 0) {
       
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
  if (rc_get_forback() == RC_FORBACK_BACKWARD) {
    delay(750);
    if (rc_get_forback() == RC_FORBACK_BACKWARD) {
      toggle_config_mode(); 
      if (get_config_mode() == false) save_melty_config_settings();    //save melty settings on config mode exit
      
      //wait for user to release stick - so we don't re-toggle modes
      while (rc_get_forback() == RC_FORBACK_BACKWARD) {
        service_watchdog();
      }
    }
  }    
}

//handles the bot when not spinning (with RC good)
static void handle_bot_idle() {

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

#ifndef ENABLE_WEBSERVER_STANDALONE
  //if the rc signal isn't good - assure motors off - and "slow flash" LED
  //this will interrupt a spun-up bot if the signal becomes bad
  while (rc_signal_is_healthy() == false) {
    motors_off();
    
    heading_led_on(0); delay(30);
    heading_led_off(); delay(600);
    
    //services watchdog and echo diagnostics while we are waiting for RC signal
    service_watchdog();
    echo_diagnostics();
  }

  //if RC is good - and throtte is above 0 - spin a single rotation
  if (rc_get_throttle_percent() > 0) {
    //this is where all the motor control happens!  (see spin_control.cpp)
    spin_one_rotation();  
  } else {    
    handle_bot_idle();
  }
#else
  // In standalone web server mode, just handle idle state and echo diagnostics
  motors_off();
  heading_led_on(0); delay(30);
  heading_led_off(); delay(120);
  echo_diagnostics();
  delay(250); // Slow down the diagnostics output a bit
#endif

}
