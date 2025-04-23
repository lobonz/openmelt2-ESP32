//this module handles interfacing to the motors

#include "arduino.h"
#include "melty_config.h"
#include "motor_driver.h"
#include <ESP32Servo.h>  // Using ESP32-specific servo library

// Servo objects for ESC control when using SERVO_PWM_THROTTLE
Servo motor1_servo;
Servo motor2_servo;

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
    Serial.println("Calibration: Set throttle to maximum");
    motor1_servo.writeMicroseconds(2000);
    motor2_servo.writeMicroseconds(2000);
    delay(5000);
    
    // 2. Send neutral signal
    Serial.println("Calibration: Set throttle to neutral");
    motor1_servo.writeMicroseconds(1500);
    motor2_servo.writeMicroseconds(1500);
    delay(5000);
    
    Serial.println("Calibration complete - ESCs should now be calibrated");
  } else {
    // Normal arming sequence
    // Start with neutral signal
    motor1_servo.writeMicroseconds(1500);
    motor2_servo.writeMicroseconds(1500);
    delay(1000);  // Give ESCs time to initialize
  }
}

//motor_X_on functions are used for the powered phase of each rotation
//motor_X_coast functions are used for the unpowered phase of each rotation
//motor_X_off functions are used for when the robot is spun-down

void motor_on(float throttle_percent, int motor_pin) {

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
    // Map throttle_percent (0-1.0) to pulse width (1500-2000μs)
    // 1500μs = 0% throttle (neutral), 2000μs = 100% throttle (forward)
    int pulse_width = 1500 + (throttle_percent * 500);
    if (motor_pin == MOTOR_PIN1) {
      motor1_servo.writeMicroseconds(pulse_width);
    } else if (motor_pin == MOTOR_PIN2) {
      motor2_servo.writeMicroseconds(pulse_width);
    }
  }
}

void motor_1_on(float throttle_percent) {
  motor_on(throttle_percent, MOTOR_PIN1);
}

void motor_2_on(float throttle_percent) {
  motor_on(throttle_percent, MOTOR_PIN2);
}

void motor_coast(int motor_pin) {
  if (THROTTLE_TYPE == FIXED_PWM_THROTTLE || THROTTLE_TYPE == DYNAMIC_PWM_THROTTLE) {
    analogWrite(motor_pin, PWM_MOTOR_COAST);
  }
  if (THROTTLE_TYPE == BINARY_THROTTLE) {
    digitalWrite(motor_pin, LOW);  //same as "off" for brushed motors
  }
  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    // For bi-directional ESCs, send neutral pulse width (1500μs = 0% throttle)
    if (motor_pin == MOTOR_PIN1) {
      motor1_servo.writeMicroseconds(1500);
    } else if (motor_pin == MOTOR_PIN2) {
      motor2_servo.writeMicroseconds(1500);
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
      motor1_servo.writeMicroseconds(1500);
    } else if (motor_pin == MOTOR_PIN2) {
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
  if (THROTTLE_TYPE == SERVO_PWM_THROTTLE) {
    // Use the arming function (without calibration)
    arm_calibrate_escs(false);
  } else {
    pinMode(MOTOR_PIN1, OUTPUT);
    pinMode(MOTOR_PIN2, OUTPUT);
  }
  motors_off();
}
