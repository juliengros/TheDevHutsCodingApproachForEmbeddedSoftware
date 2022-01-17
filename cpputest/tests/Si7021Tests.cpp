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

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include "I2CMock.h"
#include <string.h>

class Si7021_I2CSetupInfo_Comparator : public MockNamedValueComparator
{
public:
bool isEqual(const void* object1,
             const void* object2)
{
    const auto* I2CSetupInfo1 = (const I2CSetupInfo*) object1;
    const auto* I2CSetupInfo2 = (const I2CSetupInfo*) object2;
    bool ret                  = true;

    CHECK(ret &= (I2CSetupInfo1->mode == I2CSetupInfo2->mode));
    CHECK(ret &= (I2CSetupInfo1->role == I2CSetupInfo2->role));

    return ret;
}

SimpleString valueToString(const void* object)
{
    return StringFrom(object);
}
};

class Si7021_TransactionDescriptor_Comparator : public MockNamedValueComparator
{
public:
bool isEqual(const void* object1,
             const void* object2)
{
    const auto* I2CTransactionDesc1 = (const I2CTransactionDescriptor*) object1;
    const auto* I2CTransactionDesc2 = (const I2CTransactionDescriptor*) object2;
    bool ret                        = true;

    ret &= (I2CTransactionDesc1->addressing_mode == I2CTransactionDesc2->addressing_mode);
    ret &= (I2CTransactionDesc1->address == I2CTransactionDesc2->address);
    ret &= (I2CTransactionDesc1->direction == I2CTransactionDesc2->direction);
    ret &= (I2CTransactionDesc1->data_path == I2CTransactionDesc2->data_path);
    ret &= (I2CTransactionDesc1->data_count == I2CTransactionDesc2->data_count);
    if (I2CTransactionDesc1->direction == I2C_TX) {
        ret &= (0 == memcmp(I2CTransactionDesc1->data,
                            I2CTransactionDesc2->data,
                            I2CTransactionDesc1->data_count));
    } else {
        memcpy(I2CTransactionDesc2->data,
               I2CTransactionDesc1->data,
               I2CTransactionDesc1->data_count);
    }

    return ret;
}

SimpleString valueToString(const void* object)
{
    return StringFrom(object);
}
};

Si7021_I2CSetupInfo_Comparator si7021_setup_info_comparator;
Si7021_TransactionDescriptor_Comparator si7021_transaction_descriptor_comparator;

extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "FreeRTOSMock.h"
#include "I2CWrapperMock.h"
#include "Si7021.h"
#include "TestHelpers.h"
}
#define DEFAULT_SLAVE_ADDR 0x40

typedef uint8_t MockSi7021Revision[2];
typedef uint8_t MockSi7021Measurement[3];

static SemaphoreHandle_t mock_Si7021_mutex_handle = NEW_MUTEX(0);
static TaskHandle_t mock_Si7021_task_handle       = NEW_TASK(0);

static I2CSetupInfo expected_setup_info;
static I2CTransactionDescriptor expected_write_transaction_descriptor;
static I2CTransactionDescriptor expected_read_transaction_descriptor;

static uint8_t si7021_cmd_buffer[SI7021_MAX_CMD_LENGTH];

static MockSi7021Revision mock_si7021_fw_revision_1;
static MockSi7021Revision mock_si7021_fw_revision_2;
static MockSi7021Revision mock_si7021_fw_revision_unknown;

static MockSi7021Measurement mock_si7021_valid_temp_measurement;
static MockSi7021Measurement mock_si7021_invalid_temp_measurement;

static MockSi7021Measurement mock_si7021_valid_rh_measurement;
static MockSi7021Measurement mock_si7021_invalid_rh_measurement;

Si7021FirmwareRevision fw_revision;
static float temperature;
static float humidity;

static void ResetStaticVariables(void)
{
    for (uint16_t i = 0; i < sizeof(si7021_cmd_buffer); i++) {
        si7021_cmd_buffer[i] = 0x00;
    }

    mock_si7021_fw_revision_1[0] = 0xFF;
    mock_si7021_fw_revision_1[1] = 0x00;

    mock_si7021_fw_revision_2[0] = 0x20;
    mock_si7021_fw_revision_2[1] = 0x00;

    mock_si7021_fw_revision_unknown[0] = 0x30;
    mock_si7021_fw_revision_unknown[1] = 0x00;

    fw_revision = SI7021_REV_UNKNOWN;

    mock_si7021_valid_temp_measurement[0] = 0x65;
    mock_si7021_valid_temp_measurement[1] = 0xCC;
    mock_si7021_valid_temp_measurement[2] = 0x18;

    mock_si7021_invalid_temp_measurement[0] = 0x65;
    mock_si7021_invalid_temp_measurement[1] = 0xCC;
    mock_si7021_invalid_temp_measurement[2] = 0x17;

    temperature = 0;

    mock_si7021_valid_rh_measurement[0] = 0x7D;
    mock_si7021_valid_rh_measurement[1] = 0x52;
    mock_si7021_valid_rh_measurement[2] = 0x67;

    mock_si7021_invalid_rh_measurement[0] = 0x7D;
    mock_si7021_invalid_rh_measurement[1] = 0x52;
    mock_si7021_invalid_rh_measurement[2] = 0x68;

    humidity = 0;

    expected_setup_info.role = I2C_MASTER;
    expected_setup_info.mode = I2C_STANDARD_MODE;

    expected_write_transaction_descriptor.direction       = I2C_TX;
    expected_write_transaction_descriptor.addressing_mode = I2C_ADDRESSING_MODE_7_BIT;
    expected_write_transaction_descriptor.address         = DEFAULT_SLAVE_ADDR;
    expected_write_transaction_descriptor.data_path       = I2C_USE_FIFO;
    expected_write_transaction_descriptor.data            = si7021_cmd_buffer;
    expected_write_transaction_descriptor.data_count      = 0;
    expected_write_transaction_descriptor.callback        = NULL;

    expected_read_transaction_descriptor.direction       = I2C_RX;
    expected_read_transaction_descriptor.addressing_mode = I2C_ADDRESSING_MODE_7_BIT;
    expected_read_transaction_descriptor.address         = DEFAULT_SLAVE_ADDR;
    expected_read_transaction_descriptor.data_path       = I2C_USE_FIFO;
    expected_read_transaction_descriptor.data            = NULL;
    expected_read_transaction_descriptor.data_count      = 0;
    expected_read_transaction_descriptor.callback        = NULL;
}

static void I2CTransactionReturns(I2CSetupInfo*             setup_info,
                                  I2CTransactionDescriptor* transaction_descriptor,
                                  I2CWrapperReturnCode      return_code)
{
    mock().expectOneCall("I2CWrapper_LaunchI2CTransaction")
    .withParameterOfType("I2CSetupInfo*", "setup_info", setup_info)
    .withParameterOfType("I2CTransactionDescriptor*",
                         "transaction_descriptor",
                         transaction_descriptor)
    .andReturnValue(return_code);
}

static void ExpectResetCmdTransactionAndReturn(I2CWrapperReturnCode return_code)
{
    si7021_cmd_buffer[0]                             = SI7021_RESET_CMD;
    expected_write_transaction_descriptor.data_count = 1;
    I2CTransactionReturns(&expected_setup_info,
                          &expected_write_transaction_descriptor,
                          return_code);
}

static void ExpectRevisionCmdTransactionAndReturn(I2CWrapperReturnCode return_code)
{
    si7021_cmd_buffer[0]                             = SI7021_REVISION_CMD >> 8;
    si7021_cmd_buffer[1]                             = SI7021_REVISION_CMD & 0xFF;
    expected_write_transaction_descriptor.data_count = 2;
    I2CTransactionReturns(&expected_setup_info,
                          &expected_write_transaction_descriptor,
                          return_code);
}

static void ExpectMeasCmdTransactionAndReturn(uint8_t              cmd_id,
                                              I2CWrapperReturnCode return_code)
{
    si7021_cmd_buffer[0]                             = cmd_id;
    expected_write_transaction_descriptor.data_count = 1;
    I2CTransactionReturns(&expected_setup_info,
                          &expected_write_transaction_descriptor,
                          return_code);
}

static void ExpectRevisionReadTransactionAndReturn(I2CWrapperReturnCode   return_code,
                                                   Si7021FirmwareRevision mock_revision)
{
    switch (mock_revision) {
        case SI7021_REV_1:
            expected_read_transaction_descriptor.data = mock_si7021_fw_revision_1;
            break;

        case SI7021_REV_2:
            expected_read_transaction_descriptor.data = mock_si7021_fw_revision_2;
            break;

        default:
            expected_read_transaction_descriptor.data = mock_si7021_fw_revision_unknown;
            break;
    }
    expected_read_transaction_descriptor.data_count = sizeof(MockSi7021Revision);
    I2CTransactionReturns(&expected_setup_info,
                          &expected_read_transaction_descriptor,
                          return_code);
}

static void ExpectMeasReadTransactionAndReturn(I2CWrapperReturnCode  return_code,
                                               MockSi7021Measurement mock_measurement)
{
    expected_read_transaction_descriptor.data       = mock_measurement;
    expected_read_transaction_descriptor.data_count = sizeof(MockSi7021Measurement);

    I2CTransactionReturns(&expected_setup_info,
                          &expected_read_transaction_descriptor,
                          return_code);
}

static void StandardSetup(void)
{
    mock().strictOrder();
    UT_PTR_SET(I2CWrapper_LaunchI2CTransaction, I2CWrapperMock_LaunchI2CTransfer);
    mock().installComparator("I2CSetupInfo*", si7021_setup_info_comparator);
    mock().installComparator("I2CTransactionDescriptor*",
                             si7021_transaction_descriptor_comparator);
    ResetStaticVariables();
    ExpectMutexCreation(mock_Si7021_mutex_handle);
    ExpectTaskCreation(mock_Si7021_task_handle);
    LONGS_EQUAL(SI7021_OK, Si7021_Create());

    ExpectSemaphoreTakeBeforeTimeout(mock_Si7021_mutex_handle, IMMEDIATE_TIMEOUT);
    ExpectResetCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_RESET_DELAY));
    LONGS_EQUAL(SI7021_OK, Si7021_Acquire(DEFAULT_SLAVE_ADDR));
}

static void StandardTeardown(void)
{
    ExpectSemaphoreGive(mock_Si7021_mutex_handle);
    Si7021_Release();
    ExpectTaskDeletion(mock_Si7021_task_handle);
    ExpectSemaphoreDeletion(mock_Si7021_mutex_handle);
    Si7021_Destroy();
    mock().checkExpectations();
    mock().clear();
    mock().removeAllComparatorsAndCopiers();
}

TEST_GROUP(Si7021CreateDestroy)
{
    void setup()
    {
        mock().strictOrder();
    }

    void teardown()
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(Si7021CreateDestroy, MutexCreationFails)
{
    RefuseMutexCreation();
    LONGS_EQUAL(SI7021_MUTEX_NOT_CREATED, Si7021_Create());
}

TEST(Si7021CreateDestroy, TaskCreationFails)
{
    ExpectMutexCreation(mock_Si7021_mutex_handle);
    RefuseTaskCreation();
    ExpectSemaphoreDeletion(mock_Si7021_mutex_handle);
    LONGS_EQUAL(SI7021_TASK_NOT_CREATED, Si7021_Create());
}

TEST(Si7021CreateDestroy, DestroyDeletesMutexAndTask)
{
    ExpectMutexCreation(mock_Si7021_mutex_handle);
    ExpectTaskCreation(mock_Si7021_task_handle);
    LONGS_EQUAL(SI7021_OK, Si7021_Create());
    ExpectTaskDeletion(mock_Si7021_task_handle);
    ExpectSemaphoreDeletion(mock_Si7021_mutex_handle);
    Si7021_Destroy();
}

TEST_GROUP(Si7021AcquireRelease)
{
    void setup()
    {
        mock().strictOrder();
        UT_PTR_SET(I2CWrapper_LaunchI2CTransaction, I2CWrapperMock_LaunchI2CTransfer);
        mock().installComparator("I2CSetupInfo*", si7021_setup_info_comparator);
        mock().installComparator("I2CTransactionDescriptor*",
                                 si7021_transaction_descriptor_comparator);
        ResetStaticVariables();
        ExpectMutexCreation(mock_Si7021_mutex_handle);
        ExpectTaskCreation(mock_Si7021_task_handle);
        LONGS_EQUAL(SI7021_OK, Si7021_Create());
    }

    void teardown()
    {
        ExpectTaskDeletion(mock_Si7021_task_handle);
        ExpectSemaphoreDeletion(mock_Si7021_mutex_handle);
        Si7021_Destroy();
        mock().checkExpectations();
        mock().clear();
        mock().removeAllComparatorsAndCopiers();
    }
};

TEST(Si7021AcquireRelease, OutofBoundAddressReturnsInvalidInputData)
{
    LONGS_EQUAL(SI7021_INVALID_INPUT_DATA, Si7021_Acquire(0x80));
}

TEST(Si7021AcquireRelease, MutexUnavailable)
{
    RefuseSemaphoreTakeBeforeTimeout(mock_Si7021_mutex_handle, IMMEDIATE_TIMEOUT);
    LONGS_EQUAL(SI7021_MUTEX_UNAVAILABLE, Si7021_Acquire(DEFAULT_SLAVE_ADDR));
}

TEST(Si7021AcquireRelease, I2CError)
{
    ExpectSemaphoreTakeBeforeTimeout(mock_Si7021_mutex_handle, IMMEDIATE_TIMEOUT);
    ExpectResetCmdTransactionAndReturn(I2C_WRAPPER_I2C_ERROR);
    ExpectSemaphoreGive(mock_Si7021_mutex_handle);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_Acquire(DEFAULT_SLAVE_ADDR));
}

TEST(Si7021AcquireRelease, Succeeds)
{
    ExpectSemaphoreTakeBeforeTimeout(mock_Si7021_mutex_handle, IMMEDIATE_TIMEOUT);
    ExpectResetCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_RESET_DELAY));
    LONGS_EQUAL(SI7021_OK, Si7021_Acquire(DEFAULT_SLAVE_ADDR));
}

TEST(Si7021AcquireRelease, ReleaseGivesMutex)
{
    ExpectSemaphoreTakeBeforeTimeout(mock_Si7021_mutex_handle, IMMEDIATE_TIMEOUT);
    ExpectResetCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_RESET_DELAY));
    LONGS_EQUAL(SI7021_OK, Si7021_Acquire(DEFAULT_SLAVE_ADDR));
    ExpectSemaphoreGive(mock_Si7021_mutex_handle);
    Si7021_Release();
}

TEST_GROUP(Si7021ReadRevision)
{
    void setup()
    {
        StandardSetup();
    }

    void teardown()
    {
        StandardTeardown();
    }
};

TEST(Si7021ReadRevision, NullFirmwareRevisionReturnsInvalidData)
{
    LONGS_EQUAL(SI7021_INVALID_INPUT_DATA, Si7021_ReadRevision(NULL));
}

TEST(Si7021ReadRevision, I2CErrorSendingCommand)
{
    ExpectRevisionCmdTransactionAndReturn(I2C_WRAPPER_I2C_ERROR);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_ReadRevision(&fw_revision));
}

TEST(Si7021ReadRevision, I2CErrorReadingValue)
{
    ExpectRevisionCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectRevisionReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR, SI7021_REV_2);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_ReadRevision(&fw_revision));
}

TEST(Si7021ReadRevision, Revision1)
{
    ExpectRevisionCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectRevisionReadTransactionAndReturn(I2C_WRAPPER_OK, SI7021_REV_1);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadRevision(&fw_revision));
    LONGS_EQUAL(SI7021_REV_1, fw_revision);
}

TEST(Si7021ReadRevision, Revision2)
{
    ExpectRevisionCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectRevisionReadTransactionAndReturn(I2C_WRAPPER_OK, SI7021_REV_2);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadRevision(&fw_revision));
    LONGS_EQUAL(SI7021_REV_2, fw_revision);
}

TEST(Si7021ReadRevision, RevisionUnknown)
{
    ExpectRevisionCmdTransactionAndReturn(I2C_WRAPPER_OK);
    ExpectRevisionReadTransactionAndReturn(I2C_WRAPPER_OK, SI7021_REV_UNKNOWN);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadRevision(&fw_revision));
    LONGS_EQUAL(SI7021_REV_UNKNOWN, fw_revision);
}

TEST_GROUP(Si7021ReadTemperature)
{
    void setup()
    {
        StandardSetup();
    }

    void teardown()
    {
        StandardTeardown();
    }
};

TEST(Si7021ReadTemperature, NullTemperatureReturnsInvalidData)
{
    LONGS_EQUAL(SI7021_INVALID_INPUT_DATA, Si7021_ReadTemperature(NULL));
}

TEST(Si7021ReadTemperature, I2CErrorSendingCommand)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASTEMP_NOHOLD_CMD, I2C_WRAPPER_I2C_ERROR);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_ReadTemperature(&temperature));
}

TEST(Si7021ReadTemperature, I2CErrorReadingValue)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASTEMP_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASTEMP_DELAY));
    for (uint8_t i = 0; i < SI7021_MAX_READ_VAL_ATTEMPTS - 1; i++) {
        ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR,
                                           mock_si7021_valid_temp_measurement);
    }
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR, mock_si7021_valid_temp_measurement);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_ReadTemperature(&temperature));
}

TEST(Si7021ReadTemperature, ChecksumError)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASTEMP_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASTEMP_DELAY));
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_OK, mock_si7021_invalid_temp_measurement);
    LONGS_EQUAL(SI7021_CHECKSUM_ERROR, Si7021_ReadTemperature(&temperature));
}

TEST(Si7021ReadTemperature, SucceedsOnFirstReadAttempt)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASTEMP_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASTEMP_DELAY));
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_OK, mock_si7021_valid_temp_measurement);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadTemperature(&temperature));
}

TEST(Si7021ReadTemperature, SucceedsOnLastReadAttempt)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASTEMP_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASTEMP_DELAY));
    for (uint8_t i = 0; i < SI7021_MAX_READ_VAL_ATTEMPTS - 1; i++) {
        ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR,
                                           mock_si7021_valid_temp_measurement);
    }
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_OK, mock_si7021_valid_temp_measurement);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadTemperature(&temperature));
}

TEST_GROUP(Si7021ReadHumidity)
{
    void setup()
    {
        StandardSetup();
    }

    void teardown()
    {
        StandardTeardown();
    }
};

TEST(Si7021ReadHumidity, NullHumidityReturnsInvalidData)
{
    LONGS_EQUAL(SI7021_INVALID_INPUT_DATA, Si7021_ReadHumidity(NULL));
}

TEST(Si7021ReadHumidity, I2CErrorSendingCommand)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASRH_NOHOLD_CMD, I2C_WRAPPER_I2C_ERROR);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_ReadHumidity(&humidity));
}

TEST(Si7021ReadHumidity, I2CErrorReadingValue)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASRH_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASRH_DELAY));
    for (uint8_t i = 0; i < SI7021_MAX_READ_VAL_ATTEMPTS - 1; i++) {
        ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR,
                                           mock_si7021_valid_rh_measurement);
    }
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR, mock_si7021_valid_rh_measurement);
    LONGS_EQUAL(SI7021_I2C_ERROR, Si7021_ReadHumidity(&humidity));
}

TEST(Si7021ReadHumidity, ChecksumError)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASRH_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASRH_DELAY));
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_OK, mock_si7021_invalid_rh_measurement);
    LONGS_EQUAL(SI7021_CHECKSUM_ERROR, Si7021_ReadHumidity(&humidity));
}

TEST(Si7021ReadHumidity, SucceedsOnFirstReadAttempt)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASRH_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASRH_DELAY));
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_OK, mock_si7021_valid_rh_measurement);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadHumidity(&humidity));
}

TEST(Si7021ReadHumidity, SucceedsOnLastReadAttempt)
{
    ExpectMeasCmdTransactionAndReturn(SI7021_MEASRH_NOHOLD_CMD, I2C_WRAPPER_OK);
    ExpectTaskDelay(pdMS_TO_TICKS(SI7021_MEASRH_DELAY));
    for (uint8_t i = 0; i < SI7021_MAX_READ_VAL_ATTEMPTS - 1; i++) {
        ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_I2C_ERROR,
                                           mock_si7021_valid_rh_measurement);
    }
    ExpectMeasReadTransactionAndReturn(I2C_WRAPPER_OK, mock_si7021_valid_rh_measurement);
    LONGS_EQUAL(SI7021_OK, Si7021_ReadHumidity(&humidity));
}
