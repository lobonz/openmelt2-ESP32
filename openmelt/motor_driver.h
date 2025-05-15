//intitialize motors
void init_motors();

// Flag to enable direct ESC control (bypasses translational drift)
extern bool direct_esc_control;

// Function to arm or calibrate ESCs
// For normal operation, just call init_motors()
// For calibration, set calibrate=true
void arm_calibrate_escs(bool calibrate = false);

// Functions to get current PWM values
int get_motor1_pulse_width();
int get_motor2_pulse_width();

// Function to update PWM values for diagnostics
void update_pwm_values(int motor1_pwm, int motor2_pwm);

// Function to directly set servo PWM values
void set_servo_pwm(int motor_pin, int pulse_width);

// Direct ESC control functions (for testing)
void set_direct_esc_control(bool enable);
void set_esc_throttle(float throttle_percent);

// Normal driving mode when throttle is zero
void normal_driving_mode(float steering_x, float steering_y);

//turn motor_X_on (throttle_percent only used for dynamic PWM throttle mode)
//is_translating flag indicates if this is part of a translational movement
void motor_on(float throttle_percent, int motor_pin, bool is_translating = false);
void motor_1_on(float throttle_percent, bool is_translating = false);
void motor_2_on(float throttle_percent, bool is_translating = false);

//motors shut-down (robot not translating)
void motor_1_off();
void motor_2_off();
void motors_off();

//motors coasting (unpowered part of rotation when translating)
void motor_1_coast();
void motor_2_coast();

