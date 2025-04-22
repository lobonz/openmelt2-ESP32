#include "src/SparkFun_LIS331/src/SparkFun_LIS331.h"

//Set high enough to allow for G forces at top RPM
//LOW_RANGE - +/-100g for the H3LIS331DH
//MED_RANGE - +/-200g for the H3LIS331DH
//HIGH_RANGE - +/-400g for the H3LIS331DH
#define ACCEL_RANGE LIS331::LOW_RANGE   

//REF
//Sensor Mounted @10cm from center of robot = 40.25gForce @ 600rpm **Hammertime V1**
//Sensor Mounted @5cm from center of robot = 20.12gForce @ 600rpm
//Sensor Mounted @3.9cm from center of robot = 15.7gForce @ 600rpm
//Sensor Mounted @10cm from center of robot = 161gForce @ 1200rpm
//Sensor Mounted @5cm from center of robot = 80.5gForce @ 1200rpm
//Sensor Mounted @3.9cm from center of robot = 62.79gForce @ 1200rpm

//Set to correspond to ACCEL_RANGE
#define ACCEL_MAX_SCALE 100

//Change as needed as needed
//(Adafuit breakout default is 0x18, Sparkfun default is 0x19)
#define ACCEL_I2C_ADDRESS 0x19

void init_accel();

float get_accel_force_g();

