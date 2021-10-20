# TheDevHuts coding approach for embedded software

This code snippets will give you an overview of how we@TheDevHuts approach embedded software development.
## Coding, Testing and Documentation tight together

The files included in this repo implement a Temperature and Humidity demo application.
- HAL / I2C Driver (Andes RISCV platform)
- HAL wrapper for I2C (adapter layer that deals with I2C concurrent accesses)
- Si7021 module implementing a set of temperature / humidity measurements APIs as well as a FreeRTOS task polling periodically temperature and humidity.

All modules come with CPPUTEST files (hal wrapper tests file not included in that repo)
