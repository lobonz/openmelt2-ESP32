Recommend implementing one of these hardware solutions:
Add a capacitor across the signal line:
Connect a 0.01μF or 0.1μF ceramic capacitor between the throttle pin and ground
This forms a simple low-pass filter that will absorb high-frequency noise
Check your wiring and shielding:
Ensure signal wires are separated from power wires, especially those going to motors/ESCs
Twist signal wires with their corresponding ground wires to reduce EMI pickup
Improve ground connections:
Make sure you have solid ground connections between your receiver and ESP32
Sometimes adding an additional ground wire can reduce noise
EMI shielding:
Wrap receiver or signal wires in aluminum foil connected to ground
Use shielded cables for the RC signal lines
Power supply filtering:
Add additional capacitors on your power supply (10-100μF on the receiver's power)
Try powering the receiver from a separate, clean power source
The simplest solution to try first is adding a small ceramic capacitor between the throttle signal pin and ground - this can often dramatically reduce noise with minimal effort.
