# G-Force Calculation in OpenMelt

The G-force calculation is implemented in the `accel_handler.cpp` file. At rest, it only experiences 1G from gravity.

The system works based on centripetal acceleration. When a body moves in a circular path, it experiences an acceleration toward the center of the circle. The physics equation relating this is:

```
G = 0.00001118 * r * RPM²
```

Where:
- G is the acceleration in G-forces
- r is the radius in cm
- RPM is rotations per minute

Looking at the code in `spin_control.cpp`:

```cpp
// calculate RPM from g's - derived from "G = 0.00001118 * r * RPM^2"
rpm = fabs(get_accel_force_g() - accel_zero_g_offset) * 89445.0f;
rpm = rpm / effective_radius_in_cm;
rpm = sqrt(rpm);
```

The code is actually inverting this formula to calculate RPM from measured G-force. The constant 89445.0f is approximately 1/(0.00001118), used to solve for RPM:

```
RPM = sqrt(G / (0.00001118 * r))
```

For the accelerometer configuration:
- X-axis points toward the center of rotation (radial direction)
- Y-axis is tangential to the circular motion
- Z-axis is vertical

When spinning:
1. The accelerometer measures centripetal acceleration primarily on the X-axis
2. This reading is corrected by subtracting `accel_zero_g_offset` (calibrated when stationary)
3. The absolute value is used so orientation doesn't matter
4. The formula then calculates RPM based on this measured acceleration and the configured radius

When the robot is stationary on your desk, the only acceleration is gravity (1G) on the Z-axis, which doesn't contribute to the RPM calculation since it doesn't generate a reading on the X-axis that points toward the center of rotation.

This is why when sitting still, the system doesn't detect rotation - it's looking for centripetal acceleration on the X-axis, which only appears during actual spinning motion. 
