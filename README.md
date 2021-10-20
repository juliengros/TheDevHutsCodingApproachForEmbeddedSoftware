# MyEmbeddedExperience

Temperature and Humidity demo application with the following architecture
- HAL / I2C Driver (Andes RISCV platform)
- HAL wrapper for I2C (adapter layer that deals with I2C concurrent accesses)
- Si7021 module implementing an API as well as a FreeRTOS task polling periodically temperature and humidity.

All modules come with CPPUTEST files (hal wrapper tests file not included)
