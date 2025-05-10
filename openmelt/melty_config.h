//This file is intended to include all commonly modified settings for Open Melt

#ifndef MELTY_CONFIG_GUARD  //header guard
#define MELTY_CONFIG_GUARD

//----------AVR SPECIFIC FUNCTIONALITY----------
//This code has not been tested on ARM / non-AVR Arduinos (but may work)
//Doesn't currently support persistent config storage for non-AVR Arduinos (see config_storage.cpp for details)
//490Hz PWM-throttle behavior is specific to Atmega32u4 (see below)

//----------DIAGNOSTICS----------
// #define JUST_DO_DIAGNOSTIC_LOOP                 //Disables the robot / just displays config / battery voltage / RC info via serial

//----------EEPROM----------
#define ENABLE_EEPROM_STORAGE                     //Comment out this to disable EEPROM (for ARM)
#define EEPROM_WRITTEN_SENTINEL_VALUE 42          //Changing this value will cause existing EEPROM values to be invalidated (revert to defaults)

//----------TRANSLATIONAL DRIFT SETTINGS----------
//"DEFAULT" values are overriden by interactive config / stored in EEPROM (interactive config will be easier if they are about correct)
//To force these values to take effect after interactive config - increment EEPROM_WRITTEN_SENTINEL_VALUE
#define DEFAULT_ACCEL_MOUNT_RADIUS_CM 10.0         //Radius of accelerometer from center of robot
#define DEFAULT_LED_OFFSET_PERCENT 0              //Adjust to make heading LED line up with direction robot travels 0-99 (increasing moves beacon clockwise)
                                                   
#define DEFAULT_ACCEL_ZERO_G_OFFSET 1.5f          //Value accelerometer returns with robot at rest (in G) - adjusts for any offset
                                                  //H3LIS331 claims +/-1g DC offset - typical - but +/-2.5 has been observed at +/-400g setting (enough to cause tracking error)
                                                  //Just enterring and exiting config mode will automatically set this value / save to EEPROM (based on current accel reading reflecting 0g)
                                                  //For small-radius bots - try changing to H3LIS331 to +/-200g range for improved accuracy (accel_handler.h)

#define LEFT_RIGHT_HEADING_CONTROL_DIVISOR 1.5f   //How quick steering is (larger values = slower)

#define MIN_TRANSLATION_RPM 250                   //full power spin in below this number (increasing can reduce spin-up time)

//----------RGB LED CONFIGURATION----------
#define USE_RGB_LED true                         // Set to true to use RGB LED, false for standard LED
#define NUM_RGB_LEDS 4                            // Number of RGB LEDs in the strip/chain
#define RGB_LED_TYPE WS2812B                      // Type of RGB LED being used

// RGB LED color options
enum led_color_t {
  RED,
  BLUE,
  YELLOW,
  GREEN,
  PURPLE,
  MAGENTA,
  ORANGE,
  CONFIG  // New color for config mode
};

#define RGB_LED_COLOR YELLOW                       // Default color for RGB LED
#define CONFIG_LED_COLOR MAGENTA                       // Color for config mode indicator

//----------PIN MAPPINGS----------
//RC pins must be Arduino interrupt pins
//we need 3 interrupt pins - which requires an Arduino with Atmega32u4 or better (Atmega328 only support 2 interrupts)
//Common RC receiver setup LEFTRIGHT = CH1, FORBACK = CH2, THROTTLE = CH3
//Note: Accelerometer is connected with default Arduino SDA / SCL pins

//I2C pins for M5 Stamp S3
#define I2C_SDA_PIN 13                          // SDA pin for I2C communication (M5 Stamp S3 uses pin 13, Arduino uses pin A4)
#define I2C_SCL_PIN 15                          // SCL pin for I2C communication (M5 Stamp S3 uses pin 15, Arduino uses pin A5)

#define LEFTRIGHT_RC_CHANNEL_PIN 2                // To Left / Right on RC receiver
#define FORBACK_RC_CHANNEL_PIN 1                  // To Forward / Back on RC receiver (Pin 1 on Arduino Micro labelled as "TX" - https://docs.arduino.cc/hacking/hardware/PinMapping32u4)
#define THROTTLE_RC_CHANNEL_PIN 3                 // To Throttle on RC receiver (Pin 0 on Arduino Micro labelled as "RX" - https://docs.arduino.cc/hacking/hardware/PinMapping32u4)
#define EMERGENCY_OFF 4                           // To cut power to both ESCs

#define HEADING_LED_PIN	7                        //To heading LED (pin 21 is on-board M5StampS3 RGB LED)

//no configuration changes are needed if only 1 motor is used!
#define MOTOR_PIN1 9                              //Pin for Motor 1 driver
#define MOTOR_PIN2 10                             //Pin for Motor 2 driver

#define BATTERY_ADC_PIN 14                        //Pin for battery monitor (if enabled) changed from A0 for Arduino to 14 for M5StampS3-1.27


//----------THROTTLE CONFIGURATION----------
//THROTTLE_TYPE / High-speed PWM motor driver support:
//Setting THROTTLE_TYPE to FIXED_PWM_THROTTLE or DYNAMIC_PWM_THROTTLE pulses 490Hz PWM signal on motor drive pins at specified duty cycle (0-255)
//Can be used for 2 possible purposes:
//  1. Used as control signal for a brushless ESC supporting high speed (490Hz) PWM (tested with Hobbypower 30A / "Simonk" firmware)
//          Assumes Arduino PWM output is 490Hz (such as Arduino Micro pins 9 and 10) - should be expected NOT to work with non-AVR Arduino's without changes
//  2. 490hz signal maybe fed into a MOSFET or other on / off motor driver to control drive strength (relatively low frequency)
//Thanks to Richard Wong for his research in implementing brushless ESC support

enum throttle_modes {
  BINARY_THROTTLE,      //Motors pins are fully on/off - throttle controlled by portion of each rotation motor is on (no PWM)

  FIXED_PWM_THROTTLE,   //Motors pins are PWM at PWM_MOTOR_ON, PWM_MOTOR_COAST or PWM_MOTOR_OFF
                        //throttle controlled by portion of each rotation motor is on

  DYNAMIC_PWM_THROTTLE,  //Scales PWM throttling between PWM_MOTOR_COAST and PWM_MOTOR_ON
                        //Range of throttle scaled over is determined by DYNAMIC_PWM_THROTTLE_PERCENT_MAX
                        //PWM is locked at PWM_MOTOR_ON for throttle positions higher than DYNAMIC_PWM_THROTTLE_PERCENT_MAX
                        //Robot speed is additionally controlled by portion of each rotation motor is on (unless DYNAMIC_PWM_MOTOR_ON_PORTION is defined)
                        //This mode reduces current levels during spin up at part throttle

  SERVO_PWM_THROTTLE    //Standard RC Servo PWM signal (50Hz, 1000-2000μs pulse width)
                        //Compatible with standard RC ESCs like BLHeli
};

#define DYNAMIC_PWM_THROTTLE_PERCENT_MAX 1.0f   //Range of RC throttle DYNAMIC_PWM_THROTTLE is applied to 
                                                //0.5f for 0-50% throttle (full PWM_MOTOR_ON used for >50% throttle)
                                                //1.0f for 0-100% throttle

#define THROTTLE_TYPE SERVO_PWM_THROTTLE         //<---Using standard RC servo PWM for BLHeli ESCs

//----------ESC SETTINGS----------
// Standard RC servo PWM signal for bi-directional BLHeli ESCs:
// - 50Hz frequency (20ms period)
// - 1000-2000μs pulse width (1-2ms) where:
//   * 1000μs = -100% throttle (full reverse)
//   * 1500μs = 0% throttle (neutral/stop)
//   * 2000μs = +100% throttle (full forward)
// - For this application we only use 1500-2000μs range (neutral to forward)
// - 3.3V logic level from M5 Stamp S3 should be sufficient for most modern ESCs
// - If ESC doesn't respond to 3.3V signal, a level shifter to 5V may be needed
// 
// Normal ESC initialization sequence:
// 1. Power up the ESC with the controller sending a neutral signal (1500μs)
// 2. ESC should emit initialization tones (usually 1-3 tones)
// 3. After initialization, the ESC is ready to accept throttle commands

#define SET_SERVO_PWM_COAST_PERCENT 0.9f         // Set to a value between 0.0-1.0 to make coast use a % of current throttle
                                                 // 0.0 = neutral position (1500μs), 0.5 = 50% of current throttle
                                                 // Helps prevent voltage spikes when using damped mode in ESCs

#define SERVO_PWM_TRANSLATE_PERCENT 1.0f         // Sets "on" portion of rotation to this % of max throttle (2000μs)
                                                 // 1.0 = 100% (2000μs), 0.5 = 50% (1750μs)
                                                 // Overrides user throttle input during powered phase

#define DYNAMIC_PWM_MOTOR_ON_PORTION 0.5f       //if defined (and DYNAMIC_PWM_THROTTLE is set) portion of each rotation motor is on is fixed at this value

//----------PWM MOTOR SETTINGS---------- 
//(only used if a PWM throttle mode is chosen)
//PWM values are 0-255 duty cycle
#define PWM_MOTOR_ON 230                          //Motor PWM ON duty cycle (Simonk: 140 seems barely on / 230 seems a good near-full-throttle value)
#define PWM_MOTOR_COAST 100                       //Motor PWM COAST duty cycle - set to same as PWM_ESC_MOTOR_OFF for fully unpowered (best translation?)
#define PWM_MOTOR_OFF 100                         //Motor PWM OFF duty cycle (Simonk: 100 worked well in testing - if this is too low - ESC may not init)


//----------BATTERY MONITOR----------
//#define BATTERY_ALERT_ENABLED                     //if enabled - heading LED will flicker when battery voltage is low
#define VOLTAGE_DIVIDER 11                        //(~10:1 works well - 10kohm to GND, 100kohm to Bat+).  Resistors have tolerances!  Adjust as needed...
#define BATTERY_ADC_WARN_VOLTAGE_THRESHOLD 7.0f  //If voltage drops below this value - then alert is triggered
#define ARDUINIO_VOLTAGE 5.0f                     //Needed for ADC maths for battery monitor
#define LOW_BAT_REPEAT_READS_BEFORE_ALARM 20      //Requires this many ADC reads below threshold before alarming


//----------SAFETY----------
#define ENABLE_WATCHDOG                           //Uses Adafruit's sleepdog to enable watchdog / reset (tested on AVR - should work for ARM https://github.com/adafruit/Adafruit_SleepyDog)
#define WATCH_DOG_TIMEOUT_MS 5000                 //Timeout value for watchdog (increased from 2000ms for WiFi operations)
#define VERIFY_RC_THROTTLE_ZERO_AT_BOOT           //Requires RC throttle be 0% at boot to allow spin-up for duration of MAX_MS_BETWEEN_RC_UPDATES (about 1 second)
                                                  //Intended as safety feature to prevent bot from spinning up at power-on if RC was inadvertently left on.
                                                  //Downside is if unexpected reboot occurs during a fight - driver will need to set throttle to zero before power 

#endif
