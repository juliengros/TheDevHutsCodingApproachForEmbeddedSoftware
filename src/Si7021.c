/*
 * Copyright (c) TheDevHuts, 2022.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact TheDevHuts (contact@thedevhuts.ai) for licensing agreement
 * opportunities.
 *
 * Contributor: Julien Gros
 *
 */

#include "FreeRTOS.h"
#include "I2C.h"
#include "I2CWrapper.h"
#include "Printer.h"
#include "semphr.h"
#include "Si7021.h"

#define SI7021_DEBUG(f_, ...) \
    ; \
    Printer_Printf(INFINITE_TIMEOUT, "\n%s(): ", __func__); \
    Printer_Printf(INFINITE_TIMEOUT, (f_), ## __VA_ARGS__);
#define SI7021_INFO(f_, ...) \
    ; \
    Printer_Printf(INFINITE_TIMEOUT, "\n%s(): ", __func__); \
    Printer_Printf(INFINITE_TIMEOUT, (f_), ## __VA_ARGS__);
#define SI7021_ERROR(f_, ...) \
    ; \
    Printer_Printf(INFINITE_TIMEOUT, "\nERROR: %s(): %s:%d: ", __func__, __FILE__, __LINE__); \
    Printer_Printf(INFINITE_TIMEOUT, (f_), ## __VA_ARGS__);

typedef struct {
    SemaphoreHandle_t      mutex;
    StaticSemaphore_t      mutex_buffer;
    StaticTask_t           task;
    StackType_t            task_stack[2 * configMINIMAL_STACK_SIZE];
    TaskHandle_t           task_handle;
    uint16_t               addr;
    Si7021FirmwareRevision fw_revision;
    uint8_t                cmd_buffer[SI7021_MAX_CMD_LENGTH];
    uint8_t                rsp_buffer[SI7021_MAX_RSP_LENGTH];
} Si7021Info;

static Si7021Info   _Si7021;
static I2CSetupInfo setup_info = {
    .role = I2C_MASTER,
    .mode = I2C_STANDARD_MODE,
};

static I2CTransactionDescriptor transaction_descriptor = {
    .direction       = I2C_RX,
    .addressing_mode = I2C_ADDRESSING_MODE_7_BIT,
    .address         = 0x80,
    .data_path       = I2C_USE_FIFO,
    .data            = NULL,
    .data_count      = 0,
    .callback        = NULL
};

static uint8_t Si7021_ComputeCRC8(uint16_t data)
{
    for (uint8_t bit = 0; bit < 16; bit++) {
        if (data & 0x8000) {
            data = (data << 1) ^ SI7021_CRC8_POLY;
        } else {
            data <<= 1;
        }
    }
    return data >>= 8;
}

static float Si7021_ConvertTemp(uint16_t temp_code)
{
    float temperature = temp_code;

    temperature *= 175.72;
    temperature /= 65536;
    temperature -= 46.85;
    return temperature;
}

static float Si7021_ConvertHumidity(uint16_t rh_code)
{
    float humidity = rh_code;

    humidity *= 125;
    humidity /= 65536;
    humidity -= 6;

    return (humidity > 100.0) ? 100.0 : humidity;
}

static Si7021ReturnCode Si7021_Write(uint8_t* data,
                                     uint16_t data_length)
{
    transaction_descriptor.direction  = I2C_TX;
    transaction_descriptor.address    = _Si7021.addr;
    transaction_descriptor.data       = data;
    transaction_descriptor.data_count = data_length;

    if (I2CWrapper_LaunchI2CTransaction(&setup_info, &transaction_descriptor) != I2C_WRAPPER_OK) {
        return SI7021_I2C_ERROR;
    }
    return SI7021_OK;
}

static Si7021ReturnCode Si7021_Read(uint8_t* data,
                                    uint16_t data_length)
{
    transaction_descriptor.direction  = I2C_RX;
    transaction_descriptor.address    = _Si7021.addr;
    transaction_descriptor.data       = data;
    transaction_descriptor.data_count = data_length;

    if (I2CWrapper_LaunchI2CTransaction(&setup_info, &transaction_descriptor) != I2C_WRAPPER_OK) {
        return SI7021_I2C_ERROR;
    }
    return SI7021_OK;
}

static Si7021ReturnCode Si7021_Reset(void)
{
    _Si7021.cmd_buffer[0] = SI7021_RESET_CMD;
    Si7021ReturnCode ret = Si7021_Write(_Si7021.cmd_buffer, 1);
    if (ret == SI7021_OK) {
        vTaskDelay(pdMS_TO_TICKS(SI7021_RESET_DELAY));
    } else {
        SI7021_ERROR("Reset Failed: %d\n", ret);
    }
    return ret;
}

static Si7021ReturnCode Si7021_PerformMeasurement(uint8_t  cmd_id,
                                                  uint8_t  rsp_len,
                                                  uint32_t delay)
{
    Si7021ReturnCode return_code = SI7021_OK;

    _Si7021.cmd_buffer[0] = cmd_id;
    if ((return_code =
             Si7021_Write(_Si7021.cmd_buffer, 1)) != SI7021_OK) {
        SI7021_ERROR("Write() - Command %d Failed/n", cmd_id);
        return return_code;
    }

    vTaskDelay(pdMS_TO_TICKS(delay));
    for (uint8_t i = 0; i < rsp_len; i++) {
        _Si7021.rsp_buffer[i] = 0x00;
    }
    uint8_t read_attempts = 0;
    do {
        return_code =
            Si7021_Read(_Si7021.rsp_buffer, rsp_len);
    } while ((return_code != SI7021_OK) && (read_attempts++ < SI7021_MAX_READ_VAL_ATTEMPTS - 1));

    return return_code;
}

static void Si7021Task(void* pvParameters)
{
    UNUSED(pvParameters);
    vTaskDelay(pdMS_TO_TICKS(1000));

    SI7021_INFO("Starting\n");
    float temperature, humidity = 0;
    bool  init = true;

    while (1) {
        if (Si7021_Acquire(SI7021_DEFAULT_ADDR) != SI7021_OK) {
            SI7021_ERROR("Aquire() failed\n");
        }

        if (init) {
            if (Si7021_ReadRevision(&_Si7021.fw_revision) != SI7021_OK) {
                SI7021_ERROR("Si7021_ReadRevision() failed\n");
            }
            init = false;
        }
        if (Si7021_ReadTemperature(&temperature) != SI7021_OK) {
            SI7021_ERROR("ReadTemperature() failed\n");
        } else {
            SI7021_INFO("Temperature: %d\n", (int32_t) temperature);
        }

        if (Si7021_ReadHumidity(&humidity) != SI7021_OK) {
            SI7021_ERROR("ReadHumidity() failed\n");
        } else {
            SI7021_INFO("Humidity: %d%%\n", (int32_t) humidity);
        }
        Si7021_Release();
        vTaskDelay(pdMS_TO_TICKS(20000));
    }
    SI7021_INFO("Exiting\n");
    vTaskDelete(NULL);
}

Si7021ReturnCode Si7021_Create(void)
{
    _Si7021.mutex = xSemaphoreCreateMutexStatic(&(_Si7021.mutex_buffer));
    if (_Si7021.mutex == NULL) {
        return SI7021_MUTEX_NOT_CREATED;
    }

    _Si7021.task_handle = xTaskCreateStatic(Si7021Task,
                                            "Si7021_task",
                                            2 * configMINIMAL_STACK_SIZE,
                                            NULL,
                                            SI7021_TASK_PRIORITY,
                                            _Si7021.task_stack,
                                            &(_Si7021.task));

    if (_Si7021.task_handle == NULL) {
        vSemaphoreDelete(_Si7021.mutex);
        return SI7021_TASK_NOT_CREATED;
    }
    return SI7021_OK;
}

void Si7021_Destroy(void)
{
    vTaskDelete(_Si7021.task_handle);
    vSemaphoreDelete(_Si7021.mutex);
}

Si7021ReturnCode Si7021_Acquire(uint16_t i2c_addr)
{
    if (i2c_addr > I2C_ADDR_MAX_7BIT) {
        return SI7021_INVALID_INPUT_DATA;
    }

    if (xSemaphoreTake(_Si7021.mutex, IMMEDIATE_TIMEOUT) != pdPASS) {
        return SI7021_MUTEX_UNAVAILABLE;
    }

    Si7021ReturnCode return_code = SI7021_OK;

    _Si7021.addr = i2c_addr;

    if ((return_code = Si7021_Reset()) != SI7021_OK) {
        xSemaphoreGive(_Si7021.mutex);
    }

    return return_code;
}

void Si7021_Release(void)
{
    xSemaphoreGive(_Si7021.mutex);
}

Si7021ReturnCode Si7021_ReadRevision(Si7021FirmwareRevision* fw_revision)
{
    if (fw_revision == NULL) {
        return SI7021_INVALID_INPUT_DATA;
    }

    Si7021ReturnCode return_code = SI7021_OK;

    for (uint8_t i = 0; i < SI7021_REVISION_RSP_LEN; i++) {
        _Si7021.rsp_buffer[i] = 0x00;
    }

    _Si7021.cmd_buffer[0] = SI7021_REVISION_CMD >> 8;
    _Si7021.cmd_buffer[1] = SI7021_REVISION_CMD & 0xFF;

    if ((return_code =
             Si7021_Write(_Si7021.cmd_buffer, 2)) != SI7021_OK) {
        SI7021_ERROR("Write() - SI7021_REVISION_CMD Failed/n");
        return return_code;
    }

    return_code =
        Si7021_Read(_Si7021.rsp_buffer, SI7021_REVISION_RSP_LEN);

    if (return_code == SI7021_OK) {
        uint8_t revision = _Si7021.rsp_buffer[0];
        if (revision == 0x20) {
            *fw_revision = SI7021_REV_2;
            SI7021_INFO("Firmware Revision: 2");
        } else if (revision == 0xFF) {
            *fw_revision = SI7021_REV_1;
            SI7021_INFO("Firmware Revision: 1")
        } else {
            *fw_revision = SI7021_REV_UNKNOWN;
            SI7021_INFO("Firmware Revision: Unknown")
        }
    }
    return return_code;
}

Si7021ReturnCode Si7021_ReadTemperature(float* temperature)
{
    if (temperature == NULL) {
        return SI7021_INVALID_INPUT_DATA;
    }

    Si7021ReturnCode return_code = Si7021_PerformMeasurement(SI7021_MEASTEMP_NOHOLD_CMD,
                                                             SI7021_MEASTEMP_NOHOLD_RSP_LEN,
                                                             SI7021_MEASTEMP_DELAY);

    if (return_code == SI7021_OK) {
        SI7021_DEBUG("(MSB): %x, (LSB): %x, (CHXSUM): %x",
                     _Si7021.rsp_buffer[0],
                     _Si7021.rsp_buffer[1],
                     _Si7021.rsp_buffer[2]);

        uint16_t temp_code = (_Si7021.rsp_buffer[0] << 8) | _Si7021.rsp_buffer[1];
        if (Si7021_ComputeCRC8(temp_code) != _Si7021.rsp_buffer[2]) {
            SI7021_ERROR("CRC Check Failed");
            return_code = SI7021_CHECKSUM_ERROR;
        } else {
            SI7021_DEBUG("temp_code: %d", temp_code);
            *temperature = Si7021_ConvertTemp(temp_code);
        }
    }

    return return_code;
}

Si7021ReturnCode Si7021_ReadHumidity(float* humidity)
{
    if (humidity == NULL) {
        return SI7021_INVALID_INPUT_DATA;
    }

    Si7021ReturnCode return_code = Si7021_PerformMeasurement(SI7021_MEASRH_NOHOLD_CMD,
                                                             SI7021_MEASRH_NOHOLD_RSP_LEN,
                                                             SI7021_MEASRH_DELAY);

    if (return_code == SI7021_OK) {
        SI7021_DEBUG("(MSB): %x, (LSB): %x, (CHXSUM): %x",
                     _Si7021.rsp_buffer[0],
                     _Si7021.rsp_buffer[1],
                     _Si7021.rsp_buffer[2]);

        uint16_t rh_code = (_Si7021.rsp_buffer[0] << 8) | _Si7021.rsp_buffer[1];
        if (Si7021_ComputeCRC8(rh_code) != _Si7021.rsp_buffer[2]) {
            SI7021_ERROR("CRC Check Failed");
            return_code = SI7021_CHECKSUM_ERROR;
        } else {
            SI7021_DEBUG("rh_code: %d", rh_code);
            *humidity = Si7021_ConvertHumidity(rh_code);
        }
    }

    return return_code;
}
