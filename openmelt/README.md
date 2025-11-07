# OpenMelt

OpenMelt is an ESP32-based controller for melty brain style combat robots, featuring translational drift control and web-based diagnostics.

## Features

- Support for standard RC receivers
- Melty brain translational drift via accelerometer-based RPM sensing
- Web interface for diagnostics and configuration
- Support for ESCs using standard RC PWM signals
- Real-time diagnostics and logging
- User-configurable settings with persistent storage
- Normal two-wheel driving mode when throttle is at zero

## Debug System

The system includes a central debug handler that:

1. Collects debug messages from all modules
2. Formats and displays messages both via Serial and on the web interface
3. Includes telemetry data from system components
4. Supports different message priority levels (info, warning, error)

### Debug API

To add debug messages in your code:

```cpp
// Standard debug message (INFO level)
debug_print("MODULE", "Your message here");

// Formatted debug message (INFO level)
debug_printf("MODULE", "Value: %d, Status: %s", value, status);

// Debug with level specification
debug_print_level(DEBUG_WARNING, "MODULE", "Warning message");
debug_print_level(DEBUG_ERROR, "MODULE", "Error message");

// String-safe debug functions (no need to call .c_str())
debug_print_safe("MODULE", "Message with " + String(value) + " embedded");
debug_print_safe(String("MODULE"), String("Both module and message as String objects"));
debug_print_safe(String("MODULE"), "String module with char* message");

// Access all current debug data as a string
String debugData = get_debug_data();
```

## Configuration

Primary settings are in `melty_config.h`:

- RC channel pins and pulse thresholds
- Accelerometer configuration
- Motor driver settings
- Default values for heading LED, accelerometer radius and zero offset

## Web Interface

Connect to the "Hammertime_AP" WiFi network (password: hammertime123) to access the web interface at the AP's IP address.

## Hardware

Tested with:
- ESP32 (M5Stack Stamp S3)
- H3LIS331DL accelerometer (±100g/±200g/±400g range options)
- Standard RC receivers
- Standard RC ESCs using PWM signaling


OpenMelt Config Process
- Spin up to check heading LED (will not be tracking right)
- Spin down fully
- Pull control stick (forward/backward) back + HOLD for 1 second (This enters config mode)
- Spin up to approx 50% Throttle
- Move the control stick left or right until tracking is correct
  The LED should shimmer in a strip rather than separate dashes
- Move the control stick back and forward to check the heading
- Move the control stick to the bottom diagonal left or right to adjust heading, retest forward backward heading and repeat until correct heading.
- Test forward backward left right - when happy its correct - spin down.
- Pull the control stick back and hold for 1 second to exit config mode and save your settings

## Normal Driving Mode

When the throttle is at zero (not spinning), the bot can function as a standard two-wheeled robot:

- Push the steering stick forward to drive both motors forward
- Pull the steering stick back to drive both motors backward
- Push the steering stick left to turn left (left motor backward, right motor forward)
- Push the steering stick right to turn right (left motor forward, right motor backward)

The system implements smart mode switching with these configurable parameters:
- `NORMAL_DRIVING_MODE_STEERING_DEADZONE`: Prevents drift from small steering inputs (default: 0.1)
- `THROTTLE_DEADZONE_PERCENT`: Small throttle values that are treated as zero (default: 3%)
- `MODE_SWITCH_TIMEOUT_MS`: Timeout before switching modes when inputs are inactive (default: 500ms)
- `NORMAL_DRIVING_MOTOR_1_REVERSE`: Set to true to reverse motor 1 direction (default: false)
- `NORMAL_DRIVING_MOTOR_2_REVERSE`: Set to true to reverse motor 2 direction (default: false)

This feature makes the robot multi-purpose, allowing it to drive normally when not in melty brain mode and automatically switching back to melty brain behavior when throttle is applied.
