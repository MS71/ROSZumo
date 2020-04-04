# ROSZumo
"second" Small robot based on ESP32 + ROS + ZUMO 32u4 base


Components:
* ZUMO32U4 Base (I2C Slave, Bat ADC, Motor Encoder, PWM for Lidar Stepper, l3gd20h, lsm303d, Line Sonsor, ...)
* ESP32CAM
* ESP32 WRoverwith CAM and SDCard
* driver and connector for rotating LIDAR
* 12x STM VL53L1X TOF sensor
* optional rotating Lidar with PCF8475+ULN2003 Stepper motor
* BNO055 Orientation Sensor
* STM32G081 uC
* connector for orange PI Zero H5
* connector for RPI SPI Display
* NIMH charger with front contacts
* SPI flash
* I2C level shifter
* I2C bus connector
* SPI bus connector
* low power design (shutdown and wakeup)
* optional Stereo/Quad SPI Camera

![alt text](images/zumoros_01.jpg)

![alt text](images/zumoros_02.jpg)

![alt text](images/zumoros_03.jpg)

![alt text](images/zumoros_04.jpg)

My Current Setup:
* Zumo AVR 32U4 
  * I2C slave
  * motor driver
  * bridge to all Zumo sensors (line sensor, beeper, IMU, ...)
  * battery pack 4xAA
  
* STM32G081
  * I2C slave
  * GPIO extender (e.g. for VL53L1X resets)
  * RPI LCD display driver
  * Battery changer
  * RTC
  * low power managment
  
* front VL53L1X as I2C slave

* ESP32Cam
  * I2C master
  * Camera
  * ROS2 Node
  * OTA firmware update
  
* external Linux PC or RPI3/4 as ROS2 master

  
