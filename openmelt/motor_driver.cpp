//this module handles interfacing to the motors

#include "Arduino.h"
#include "melty_config.h"
#include "motor_driver.h"
#include "debug_handler.h"
#include "rc_handler.h"  // Include for rc_get_translation_percent()
#include <ESP32Servo.h>  // Using ESP32-specific servo library

// Servo objects for ESC control when using SERVO_PWM_THROTTLE
Servo motor1_servo;
Servo motor2_servo;

// Variables to store current PWM values
int current_motor1_pulse_width = 1500;
int current_motor2_pulse_width = 1500;

// Flag to enable direct ESC control (bypasses translational drift)
bool direct_esc_control = false;

// Getter functions for current PWM values
int get_motor1_pulse_width() {
  return current_motor1_pulse_width;
}

int get_motor2_pulse_width() {
  return current_motor2_pulse_width;
}

// Function to enable/disable direct ESC control
void set_direct_esc_control(bool enable) {
  direct_esc_control = enable;
  if (enable) {
    debug_print_level(DEBUG_WARNING, "MOTOR", "DIRECT ESC CONTROL ENABLED");
    debug_print("MOTOR", "Bypassing translational drift for direct throttle testing");
  } else {
    debug_print("MOTOR", "DIRECT ESC CONTROL DISABLED");
    debug_print("MOTOR", "Returning to normal translational drift control");
  }
}

// Function to handle normal driving mode when throttle is zero
// steering_x and steering_y should be normalized from -1.0 to 1.0
// where (0,1)=forward, (0,-1)=backward, (1,0)=right, (-1,0)=left
void normal_driving_mode(float steering_x, float steering_y) {
  if (THROTTLE_TYPE != SERVO_PWM_THROTTLE) return;

  // Apply deadzone from config
  if (fabs(steering_x) <= NORMAL_DRIVING_MODE_STEERING_DEADZONE) steering_x = 0.0;
  if (fabs(steering_y) <= NORMAL_DRIVING_MODE_STEERING_DEADZONE) steering_y = 0.0;

  // Calculate motor values based on steering inputs
  // Left motor (motor1) and right motor (motor2)
  float left_motor = steering_y + steering_x;   // Forward+Right -> Left motor increases
  float right_motor = steering_y - steering_x;  // Forward+Left -> Right motor increases

  // Apply motor direction reversing if configured
  if (NORMAL_DRIVING_MOTOR_1_REVERSE) left_motor = -left_motor;
  if (NORMAL_DRIVING_MOTOR_2_REVERSE) right_motor = -right_motor;

  // Limit values to range -1.0 to 1.0
  left_motor = constrain(left_motor, -1.0, 1.0);
  right_motor = constrain(right_motor, -1.0, 1.0);

  // Map from -1.0,1.0 to pulse width (1000-2000μs)
  // 1500μs is neutral, 2000μs is full forward, 1000μs is full reverse
  int left_pulse = 1500 + (left_motor * 500);
  int right_pulse = 1500 + (right_motor * 500);

  // Send values to motors
  current_motor1_pulse_width = left_pulse;
  current_motor2_pulse_width = right_pulse;
  motor1_servo.writeMicroseconds(left_pulse);
  motor2_servo.writeMicroseconds(right_pulse);

  // Debug output
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 500) {
    debug_printf("MOTOR", "Normal driving mode - Steering X: %.2f, Y: %.2f, Left PWM: %d, Right PWM: %d",
                steering_x, steering_y, left_pulse, right_pulse);
    last_debug = millis();
  }
}

// Function to directly set ESC throttle for testing
// throttle_percent should be 0.0-1.0
void set_esc_throttle(float throttle_percent) {
  if (!direct_esc_control) return;

  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    // Map throttle_percent (0-1.0) to pulse width (1500-2000μs)
    int pulse_width = 1500;
    if (throttle_percent > 0) {
      pulse_width = 1500 + (throttle_percent * 500);
    }

    // Set both ESCs to the same throttle
    current_motor1_pulse_width = pulse_width;
    current_motor2_pulse_width = pulse_width;
    motor1_servo.writeMicroseconds(pulse_width);
    motor2_servo.writeMicroseconds(pulse_width);

    debug_printf("MOTOR", "Direct ESC Control - Throttle: %.1f%%, PWM: %d", throttle_percent * 100, pulse_width);
  }
}

// Function to update PWM values for diagnostics
void update_pwm_values(int motor1_pwm, int motor2_pwm) {
  current_motor1_pulse_width = motor1_pwm;
  current_motor2_pulse_width = motor2_pwm;
}

// Function to directly set servo PWM values
void set_servo_pwm(int motor_pin, int pulse_width) {
  if (motor_pin == MOTOR_PIN1) {
    current_motor1_pulse_width = pulse_width;
    motor1_servo.writeMicroseconds(pulse_width);
  } else if (motor_pin == MOTOR_PIN2) {
    current_motor2_pulse_width = pulse_width;
    motor2_servo.writeMicroseconds(pulse_width);
  }
}

// Function to arm or calibrate ESCs
// For normal operation, just call init_motors()
// For calibration, call this function with calibrate=true
void arm_calibrate_escs(bool calibrate) {
  if (THROTTLE_TYPE != SERVO_PWM_THROTTLE) return;

  // Attach servos to pins
  motor1_servo.attach(MOTOR_PIN1);
  motor2_servo.attach(MOTOR_PIN2);

  if (calibrate) {
    // ESC Calibration sequence
    // 1. Send max signal (this usually enters programming mode)
    debug_print("MOTOR", "Calibration: Set throttle to maximum");
    motor1_servo.writeMicroseconds(2000);
    motor2_servo.writeMicroseconds(2000);
    current_motor1_pulse_width = 2000;
    current_motor2_pulse_width = 2000;
    delay(5000);

    // 2. Send neutral signal
    debug_print("MOTOR", "Calibration: Set throttle to neutral");
    motor1_servo.writeMicroseconds(1500);
    motor2_servo.writeMicroseconds(1500);
    current_motor1_pulse_width = 1500;
    current_motor2_pulse_width = 1500;
    delay(5000);

    debug_print("MOTOR", "Calibration complete - ESCs should now be calibrated");
  } else {
    // Normal arming sequence
    // Start with neutral signal
    motor1_servo.writeMicroseconds(1500);
    motor2_servo.writeMicroseconds(1500);
    current_motor1_pulse_width = 1500;
    current_motor2_pulse_width = 1500;
    delay(1000);  // Give ESCs time to initialize
  }
}

//motor_X_on functions are used for the powered phase of each rotation
//motor_X_coast functions are used for the unpowered phase of each rotation
//motor_X_off functions are used for when the robot is spun-down

void motor_on(float throttle_percent, int motor_pin, bool is_translating) {

  // Debug output every 500ms
  static unsigned long last_debug = 0;
  if (millis() - last_debug > 500) {
    debug_printf("MOTOR", "Motor_on called - Throttle percent: %.2f%%, Motor pin: %d, Translating: %d",
               throttle_percent * 100, motor_pin, is_translating);
    last_debug = millis();
  }

  if (THROTTLE_TYPE == BINARY_THROTTLE) {
    digitalWrite(motor_pin, HIGH);
  }

  if (THROTTLE_TYPE == FIXED_PWM_THROTTLE) {
    analogWrite(motor_pin, PWM_MOTOR_ON);
  }

  //If DYNAMIC_PWM_THROTTLE - PWM is scaled between PWM_MOTOR_COAST and PWM_MOTOR_ON
  //Applies over range defined by DYNAMIC_PWM_THROTTLE_PERCENT_MAX - maxed at PWM_MOTOR_ON above this
  if (THROTTLE_TYPE == DYNAMIC_PWM_THROTTLE) {
    float throttle_pwm = PWM_MOTOR_COAST + ((throttle_percent / DYNAMIC_PWM_THROTTLE_PERCENT_MAX) * (PWM_MOTOR_ON - PWM_MOTOR_COAST));
    if (throttle_pwm > PWM_MOTOR_ON) throttle_pwm = PWM_MOTOR_ON;
    analogWrite(motor_pin, throttle_pwm);
  }

  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    // For standard RC servo PWM with bi-directional ESCs
    int pulse_width = 1500;

    // Only proceed if throttle is actually above 0
    if (throttle_percent > 0) {
      if (direct_esc_control) {
        // Direct ESC control mode - just use throttle directly
        pulse_width = 1500 + (throttle_percent * 500);
      }
      else if (is_translating) {
        // Translational movement - scale by steering stick position
        float translation_scale = rc_get_translation_percent();
        // Only scale the portion above 1.0 since 1.0 is neutral
        float scaled_translate_percent = 1.0 + ((SERVO_PWM_TRANSLATE_PERCENT - 1.0) * translation_scale);
        pulse_width = 1500 + (scaled_translate_percent * 500);
      }
      else {
        // Normal spinning (no translation) - use throttle directly
      pulse_width = 1500 + (throttle_percent * 500);
      }
    }

    // Debug pulse width calculation
    if (millis() - last_debug > 500) {
      if (is_translating) {
        float translation_scale = rc_get_translation_percent();
        // Only scale the portion above 1.0 since 1.0 is neutral
        float scaled_translate_percent = 1.0 + ((SERVO_PWM_TRANSLATE_PERCENT - 1.0) * translation_scale);
        debug_printf("MOTOR", "Translation mode - Input throttle: %.2f%%, Scale: %.2f, SERVO_PWM_TRANSLATE_PERCENT: %.2f, Scaled: %.2f, Output PWM: %d μs",
                    throttle_percent * 100, translation_scale, SERVO_PWM_TRANSLATE_PERCENT,
                    scaled_translate_percent, pulse_width);
      } else {
        debug_printf("MOTOR", "Spin mode - Input throttle: %.2f%%, Output PWM: %d μs",
                    throttle_percent * 100, pulse_width);
      }
    }

    if (motor_pin == MOTOR_PIN1) {
      current_motor1_pulse_width = pulse_width;
      motor1_servo.writeMicroseconds(pulse_width);
    } else if (motor_pin == MOTOR_PIN2) {
      current_motor2_pulse_width = pulse_width;
      motor2_servo.writeMicroseconds(pulse_width);
    }
  }
}

void motor_1_on(float throttle_percent, bool is_translating) {
  motor_on(throttle_percent, MOTOR_PIN1, is_translating);
}

void motor_2_on(float throttle_percent, bool is_translating) {
  motor_on(throttle_percent, MOTOR_PIN2, is_translating);
}

void motor_coast(int motor_pin) {
  if (THROTTLE_TYPE == FIXED_PWM_THROTTLE || THROTTLE_TYPE == DYNAMIC_PWM_THROTTLE) {
    analogWrite(motor_pin, PWM_MOTOR_COAST);
  }
  if (THROTTLE_TYPE == BINARY_THROTTLE) {
    digitalWrite(motor_pin, LOW);  //same as "off" for brushed motors
  }
  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    // Get the translation percentage scaling factor
    float translation_scale = rc_get_translation_percent();

    // For bi-directional ESCs, handle coast mode based on SERVO_PWM_COAST_PERCENT
    if (motor_pin == MOTOR_PIN1) {
      if (SERVO_PWM_COAST_PERCENT <= 0.0f) {
        // Use neutral (1500μs) if coast percent is zero
        current_motor1_pulse_width = 1500;
        motor1_servo.writeMicroseconds(1500);
      } else {
        // Scale the coast percentage based on translation
        // At translation_scale = 0: Use 1.0 (no coasting)
        // At translation_scale = 1: Use SERVO_PWM_COAST_PERCENT (max coasting)
        float scaled_coast_percent = 1.0 * (1.0 - translation_scale) + (SERVO_PWM_COAST_PERCENT * translation_scale);

        // Calculate pulse width as a percentage of the current throttle
        int pulse_width = 1500;
        int throttle_range = current_motor1_pulse_width - 1500;
        pulse_width = 1500 + (throttle_range * scaled_coast_percent);

        // Debug output every 500ms
        static unsigned long last_debug = 0;
        if (millis() - last_debug > 500) {
          debug_printf("MOTOR", "Coast mode - Scale: %.2f, Original Coast: %.2f, Scaled Coast: %.2f, PWM: %d μs",
                     translation_scale, SERVO_PWM_COAST_PERCENT, scaled_coast_percent, pulse_width);
          last_debug = millis();
        }

        current_motor1_pulse_width = pulse_width;
        motor1_servo.writeMicroseconds(pulse_width);
      }
    } else if (motor_pin == MOTOR_PIN2) {
      if (SERVO_PWM_COAST_PERCENT <= 0.0f) {
        // Use neutral (1500μs) if coast percent is zero
        current_motor2_pulse_width = 1500;
        motor2_servo.writeMicroseconds(1500);
      } else {
        // Scale the coast percentage based on translation
        // At translation_scale = 0: Use 1.0 (no coasting)
        // At translation_scale = 1: Use SERVO_PWM_COAST_PERCENT (max coasting)
        float scaled_coast_percent = 1.0 * (1.0 - translation_scale) + (SERVO_PWM_COAST_PERCENT * translation_scale);

        // Calculate pulse width as a percentage of the current throttle
        int pulse_width = 1500;
        int throttle_range = current_motor2_pulse_width - 1500;
        pulse_width = 1500 + (throttle_range * scaled_coast_percent);
        current_motor2_pulse_width = pulse_width;
        motor2_servo.writeMicroseconds(pulse_width);
      }
    }
  }
}

void motor_1_coast() {
  motor_coast(MOTOR_PIN1);
}

void motor_2_coast() {
  motor_coast(MOTOR_PIN2);
}

void motor_off(int motor_pin) {
  if (THROTTLE_TYPE == FIXED_PWM_THROTTLE || THROTTLE_TYPE == DYNAMIC_PWM_THROTTLE) {
    analogWrite(motor_pin, PWM_MOTOR_OFF);
  }
  if (THROTTLE_TYPE == BINARY_THROTTLE) {
    digitalWrite(motor_pin, LOW);  //same as "off" for brushed motors
  }
  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    // For bi-directional ESCs, send neutral pulse width (1500μs = 0% throttle)
    if (motor_pin == MOTOR_PIN1) {
      current_motor1_pulse_width = 1500;
      motor1_servo.writeMicroseconds(1500);
    } else if (motor_pin == MOTOR_PIN2) {
      current_motor2_pulse_width = 1500;
      motor2_servo.writeMicroseconds(1500);
    }
  }
}

void motor_1_off() {
  motor_off(MOTOR_PIN1);
}

void motor_2_off() {
  motor_off(MOTOR_PIN2);
}

void motors_off() {
  motor_1_off();
  motor_2_off();
}

void init_motors() {
  debug_print("MOTOR", "Initializing motor drivers...");

  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    debug_print("MOTOR", "Using SERVO_PWM_THROTTLE mode");

    // First explicitly set pins as outputs for safety
    pinMode(MOTOR_PIN1, OUTPUT);
    pinMode(MOTOR_PIN2, OUTPUT);

    // Use the arming function (without calibration)
    arm_calibrate_escs(false);

    // Double-check neutral values are set
    debug_print("MOTOR", "Setting motors to neutral position");
    motor1_servo.writeMicroseconds(1500);
    motor2_servo.writeMicroseconds(1500);
    current_motor1_pulse_width = 1500;
    current_motor2_pulse_width = 1500;

    debug_printf("MOTOR", "Motors initialized - PWM1: %d, PWM2: %d",
                current_motor1_pulse_width, current_motor2_pulse_width);
  } else {
    // For non-servo throttle types
    debug_print("MOTOR", "Using non-servo throttle mode");
    pinMode(MOTOR_PIN1, OUTPUT);
    pinMode(MOTOR_PIN2, OUTPUT);
  }
  // Ensure motors are off
  motors_off();
  debug_print("MOTOR", "Motors set to off state");
}
