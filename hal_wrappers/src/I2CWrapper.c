/*
 * Copyright (c) TheDevHuts, 2022.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact TheDevHuts (contact@thedevhuts.ai) for licensing agreement
 * opportunities.
 *
 * Contributors: Florent Remis / Julien Gros
 *
 */

#include "FreeRTOS.h"
#include "I2C.h"
#include "I2CWrapper.h"
#include "Printer.h"
#include "semphr.h"

#define I2C_WRAPPER_I2C_TIMEOUT_MS 25

static SemaphoreHandle_t i2c_mutex;
static StaticSemaphore_t i2c_mutex_buffer;
static TaskHandle_t current_task_handle;

static I2CWrapperReturnCode I2CWrapper_LaunchI2CTransfer_Implementation(
    I2CSetupInfo*             setup_info,
    I2CTransactionDescriptor* transaction_descriptor);

static I2CWrapperReturnCode I2CWrapper_LaunchI2CTransfer_Implementation(
    I2CSetupInfo*             setup_info,
    I2CTransactionDescriptor* transaction_descriptor)
{
    if ((setup_info == NULL) || (transaction_descriptor == NULL)) {
        return I2C_WRAPPER_INVALID_INPUT_DATA;
    }

    if (xSemaphoreTake(i2c_mutex, IMMEDIATE_TIMEOUT) != pdPASS) {
        return I2C_WRAPPER_I2C_MUTEX_UNAVAILABLE;
    }

    I2CWrapperReturnCode return_code = I2C_WRAPPER_OK;
    I2CReturnCode ret;

    if ((ret = I2C_SetupController(HAL_I2C, setup_info)) != I2C_OK) {
        Printer_Printf(INFINITE_TIMEOUT, "Error %d in I2C_SetupController\n", ret);
        return_code = I2C_WRAPPER_I2C_ERROR;
        goto give_mutex_and_return;
    }

    current_task_handle              = xTaskGetCurrentTaskHandle();
    transaction_descriptor->callback = I2CWrapper_I2CCallback;

    if ((ret = I2C_LaunchTransaction(HAL_I2C, transaction_descriptor)) != I2C_OK) {
        Printer_Printf(INFINITE_TIMEOUT, "Error %d in I2C_LaunchTransaction\n", ret);
        return_code = I2C_WRAPPER_I2C_ERROR;
        goto give_mutex_and_return;
    }

    uint32_t notification_value;

    if (xTaskNotifyWait(0x00,
                        0xffffffff,
                        &notification_value,
                        pdMS_TO_TICKS(I2C_WRAPPER_I2C_TIMEOUT_MS)) != pdTRUE) {
        return_code = I2C_WRAPPER_I2C_ERROR;
        goto give_mutex_and_return;
    }
    if (notification_value != I2C_OK) {
        return_code = I2C_WRAPPER_I2C_ERROR;
        goto give_mutex_and_return;
    }

give_mutex_and_return:
    xSemaphoreGive(i2c_mutex);
    return return_code;
}

I2CWrapperReturnCode I2CWrapper_Create(void)
{
    i2c_mutex = xSemaphoreCreateMutexStatic(&i2c_mutex_buffer);
    if (i2c_mutex == NULL) {
        return I2C_WRAPPER_I2C_MUTEX_NOT_CREATED;
    }
    return I2C_WRAPPER_OK;
}

void I2CWrapper_Destroy(void)
{
    vSemaphoreDelete(i2c_mutex);
}

I2CWrapperReturnCode (* I2CWrapper_LaunchI2CTransaction) (I2CSetupInfo* setup_info,
                                                          I2CTransactionDescriptor*
                                                          transaction_descriptor) =
    I2CWrapper_LaunchI2CTransfer_Implementation;

void I2CWrapper_I2CCallback(I2CReturnCode return_code)
{
    if (current_task_handle == NULL) {
        return;
    }

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xTaskNotifyFromISR(current_task_handle,
                       return_code,
                       eSetValueWithOverwrite,
                       &xHigherPriorityTaskWoken);

    current_task_handle = NULL;

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
