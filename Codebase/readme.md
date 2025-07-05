# Readme

This project covers the firmware required for the BeanBoi cave surveying device.
The current sensor list includes:
- SCA3300: Accelerometer
- LDK2M: 1D-LiDAR
- RM3100: Magnetometer

## Hardware
## Platformio
<!-- - The current version of platform IO is old and doesn't support using custom build flags for ESP i.e. sdkconfig.defaults. -->
- The FreeRTOS used in this project requires notification values which is not included in this version of platformIOs ESP build using Arduino
- Notifications must be enabled in sdkconfig.defaults using "CONFIG_FREERTOS_TASK_NOTIFICATION_ARRAY_ENTRIES = 6"
- **pioarduino must be used to build this project in order for the build to succeed**