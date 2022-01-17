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
#include "DMACMock.h"
#include "ExternalInterruptsMock.h"

class I2C_DMACChannelConfig_Comparator : public MockNamedValueComparator
{
public:
bool isEqual(const void* object1,
             const void* object2)
{
    const auto* DMACChannelConfig1 = (const DMACChannelConfig*) object1;
    const auto* DMACChannelConfig2 = (const DMACChannelConfig*) object2;
    bool ret                       = true;

    CHECK(ret &= (DMACChannelConfig1->channel == DMACChannelConfig2->channel));
    CHECK(ret &= (DMACChannelConfig1->src_bus_index == DMACChannelConfig2->src_bus_index));
    CHECK(ret &= (DMACChannelConfig1->dst_bus_index == DMACChannelConfig2->dst_bus_index));
    CHECK(ret &= (DMACChannelConfig1->channel_priority == DMACChannelConfig2->channel_priority));
    CHECK(ret &= (DMACChannelConfig1->src_burst_size == DMACChannelConfig2->src_burst_size));
    CHECK(ret &=
              (DMACChannelConfig1->src_transfer_width == DMACChannelConfig2->src_transfer_width));
    CHECK(ret &=
              (DMACChannelConfig1->dst_transfer_width == DMACChannelConfig2->dst_transfer_width));
    CHECK(ret &=
              (DMACChannelConfig1->src_handshake_mode == DMACChannelConfig2->src_handshake_mode));
    CHECK(ret &=
              (DMACChannelConfig1->dst_handshake_mode == DMACChannelConfig2->dst_handshake_mode));
    CHECK(ret &= (DMACChannelConfig1->src_addr_ctrl == DMACChannelConfig2->src_addr_ctrl));
    CHECK(ret &= (DMACChannelConfig1->dst_addr_ctrl == DMACChannelConfig2->dst_addr_ctrl));
    CHECK(ret &= (DMACChannelConfig1->src_pair == DMACChannelConfig2->src_pair));
    CHECK(ret &= (DMACChannelConfig1->dst_pair == DMACChannelConfig2->dst_pair));

    return ret;
}

SimpleString valueToString(const void* object)
{
    return StringFrom(object);
}
};

class I2C_DMACTransferConfig_Comparator : public MockNamedValueComparator
{
public:
bool isEqual(const void* object1,
             const void* object2)
{
    const auto* DMACTransferConfig1 = (const DMACTransferConfig*) object1;
    const auto* DMACTransferConfig2 = (const DMACTransferConfig*) object2;
    bool ret                        = true;

    CHECK(ret &= (DMACTransferConfig1->channel == DMACTransferConfig2->channel));
    CHECK(ret &= (DMACTransferConfig1->transfer_size == DMACTransferConfig2->transfer_size));
    CHECK(ret &= (DMACTransferConfig1->src_address == DMACTransferConfig2->src_address));
    CHECK(ret &= (DMACTransferConfig1->dst_address == DMACTransferConfig2->dst_address));

    return ret;
}

SimpleString valueToString(const void* object)
{
    return StringFrom(object);
}
};

I2C_DMACChannelConfig_Comparator  channel_config_comparator;
I2C_DMACTransferConfig_Comparator transfer_config_comparator;

extern "C" {
#include "CppUTestExt/MockSupport_c.h"
#include "I2C.h"

#define EXPECTED_I2C_EXTERNAL_INTERRUPT_PRIORITY 1
typedef struct {
    __IO uint32_t IdRev;
    __IO uint32_t Reserved0[3];
    __IO uint32_t Cfg;
    __IO uint32_t IntEn;
    __IO uint32_t Status;
    __IO uint32_t Addr;
    __IO uint32_t Data;
    __IO uint32_t Ctrl;
    __IO uint32_t Cmd;
    __IO uint32_t Setup;
    __IO uint32_t TPM;
} MockI2CRegisters;

MockI2CRegisters MOCK_HAL_I2C;

#define T_PCLK      20    // ns
#define HAL_TPM     ((MOCK_HAL_I2C.TPM & I2C_TPM_TPM_MASK) >> I2C_TPM_TPM_OFFSET)
#define HAL_T_SP    ((MOCK_HAL_I2C.Setup & I2C_SETUP_T_SP_MASK) >> I2C_SETUP_T_SP_OFFSET)
#define HAL_T_SUDAT ((MOCK_HAL_I2C.Setup & I2C_SETUP_T_SUDAT_MASK) >> I2C_SETUP_T_SUDAT_OFFSET)
#define HAL_T_HDDAT ((MOCK_HAL_I2C.Setup & I2C_SETUP_T_HDDAT_MASK) >> I2C_SETUP_T_HDDAT_OFFSET)
#define HAL_T_SCLHI ((MOCK_HAL_I2C.Setup & I2C_SETUP_T_SCLHI_MASK) >> I2C_SETUP_T_SCLHI_OFFSET)
#define HAL_T_SCLRATIO \
    ((MOCK_HAL_I2C.Setup & I2C_SETUP_T_SCLRATIO_MASK) >> \
     I2C_SETUP_T_SCLRATIO_OFFSET)

static void MockTransactionCompleteCallback(I2CReturnCode status)
{
    mock().actualCall("MockTransactionCompleteCallback").withParameter("status", status);
}

static uint8_t default_data_buffer[32];
static I2CSetupInfo default_setup;
static I2CTransactionDescriptor default_transaction;
static DMACChannelConfig  expected_dmac_channel_config[2];
static DMACTransferConfig expected_dmac_transfer_config[2];

static void ResetStaticVariables(void)
{
    expected_dmac_channel_config[I2C_TX].channel            = DMAC_CHANNEL_I2C;
    expected_dmac_channel_config[I2C_TX].src_bus_index      = 0;
    expected_dmac_channel_config[I2C_TX].dst_bus_index      = 0;
    expected_dmac_channel_config[I2C_TX].channel_priority   = 1;
    expected_dmac_channel_config[I2C_TX].src_burst_size     = DMAC_BURST_SIZE_1;
    expected_dmac_channel_config[I2C_TX].src_transfer_width = DMAC_TRANSFER_WIDTH_BYTE;
    expected_dmac_channel_config[I2C_TX].dst_transfer_width = DMAC_TRANSFER_WIDTH_BYTE;
    expected_dmac_channel_config[I2C_TX].src_handshake_mode = false;
    expected_dmac_channel_config[I2C_TX].dst_handshake_mode = true;
    expected_dmac_channel_config[I2C_TX].src_addr_ctrl      = DMAC_ADDR_CTRL_INCREMENT;
    expected_dmac_channel_config[I2C_TX].dst_addr_ctrl      = DMAC_ADDR_CTRL_FIXED;
    expected_dmac_channel_config[I2C_TX].src_pair           = 0;
    expected_dmac_channel_config[I2C_TX].dst_pair           = DMAC_CHANNEL_I2C;

    expected_dmac_channel_config[I2C_RX].channel            = DMAC_CHANNEL_I2C;
    expected_dmac_channel_config[I2C_RX].src_bus_index      = 0;
    expected_dmac_channel_config[I2C_RX].dst_bus_index      = 0;
    expected_dmac_channel_config[I2C_RX].channel_priority   = 1;
    expected_dmac_channel_config[I2C_RX].src_burst_size     = DMAC_BURST_SIZE_1;
    expected_dmac_channel_config[I2C_RX].src_transfer_width = DMAC_TRANSFER_WIDTH_BYTE;
    expected_dmac_channel_config[I2C_RX].dst_transfer_width = DMAC_TRANSFER_WIDTH_BYTE;
    expected_dmac_channel_config[I2C_RX].src_handshake_mode = true;
    expected_dmac_channel_config[I2C_RX].dst_handshake_mode = false;
    expected_dmac_channel_config[I2C_RX].src_addr_ctrl      = DMAC_ADDR_CTRL_FIXED;
    expected_dmac_channel_config[I2C_RX].dst_addr_ctrl      = DMAC_ADDR_CTRL_INCREMENT;
    expected_dmac_channel_config[I2C_RX].src_pair           = DMAC_CHANNEL_I2C;
    expected_dmac_channel_config[I2C_RX].dst_pair           = 0;

    expected_dmac_transfer_config[I2C_TX].channel       = DMAC_CHANNEL_I2C;
    expected_dmac_transfer_config[I2C_TX].transfer_size = sizeof(default_data_buffer);
    expected_dmac_transfer_config[I2C_TX].src_address   = (uint32_t) default_data_buffer;
    expected_dmac_transfer_config[I2C_TX].dst_address   = (uint32_t) &(MOCK_HAL_I2C.Data);

    expected_dmac_transfer_config[I2C_RX].channel       = DMAC_CHANNEL_I2C;
    expected_dmac_transfer_config[I2C_RX].transfer_size = sizeof(default_data_buffer);
    expected_dmac_transfer_config[I2C_RX].src_address   = (uint32_t) &(MOCK_HAL_I2C.Data);
    expected_dmac_transfer_config[I2C_RX].dst_address   = (uint32_t) default_data_buffer;

    for (uint16_t i = 0; i < sizeof(default_data_buffer); i++) {
        default_data_buffer[i] = 0;
    }

    default_setup.role = I2C_MASTER;
    default_setup.mode = I2C_STANDARD_MODE;

    default_transaction.addressing_mode = I2C_ADDRESSING_MODE_7_BIT;
    default_transaction.direction       = I2C_TX;
    default_transaction.data_path       = I2C_USE_FIFO;
    default_transaction.address         = I2C_ADDR_MAX_7BIT;
    default_transaction.data            = default_data_buffer;
    default_transaction.data_count      = sizeof(default_data_buffer);
    default_transaction.callback        = &(MockTransactionCompleteCallback);
}

static void ExpectExternalInterruptEnabled(void)
{
    mock().expectOneCall("ExternalInterrupts_EnableInterrupt")
    .withParameter("source", EXTERNAL_IRQ_I2C_SOURCE)
    .withParameter("priority", EXPECTED_I2C_EXTERNAL_INTERRUPT_PRIORITY);
}

static void ExpectExternalInterruptDisabled(void)
{
    mock().expectOneCall("ExternalInterrupts_DisableInterrupt")
    .withParameter("source", EXTERNAL_IRQ_I2C_SOURCE);
}

static void ExpectDMACChannelSetup(I2CDirection dir)
{
    mock().expectOneCall("DMAC_SetupChannel")
    .withPointerParameter("dmac_dev", HAL_DMAC)
    .withParameterOfType("DMACChannelConfig*",
                         "channel_config",
                         &(expected_dmac_channel_config[dir]))
    .withParameter("disable_abort_int", false)
    .withParameter("disable_error_int", false)
    .withParameter("disable_terminal_count_int", false)
    .withPointerParameter("callback", (void*) I2C_DMACCallback)
    .andReturnValue(DMAC_OK);
}

static void ExpectDMACTransferSetup(I2CDirection dir)
{
    mock().expectOneCall("DMAC_SetupTransfer")
    .withPointerParameter("dmac_dev", HAL_DMAC)
    .withParameterOfType("DMACTransferConfig*",
                         "transfer_config",
                         &(expected_dmac_transfer_config[dir]))
    .andReturnValue(DMAC_OK);
}

static void ExpectDMACChannelEnabled(void)
{
    mock().expectOneCall("DMAC_EnableChannel")
    .withPointerParameter("dmac_dev", HAL_DMAC)
    .withParameter("channel", DMAC_CHANNEL_I2C)
    .andReturnValue(DMAC_OK);
}

static void ExpectTransactionComplete(I2CReturnCode status)
{
    mock().expectOneCall("MockTransactionCompleteCallback")
    .withParameter("status", status);
}

static void InstallMockFunctions(void)
{
    UT_PTR_SET(DMAC_SetupChannel, DMACMock_SetupChannel);
    UT_PTR_SET(DMAC_SetupTransfer, DMACMock_SetupTransfer);
    UT_PTR_SET(DMAC_EnableChannel, DMACMock_EnableChannel);
    UT_PTR_SET(DMAC_EnableInterrupt, DMACMock_EnableInterrupt);
    UT_PTR_SET(DMAC_DisableInterrupt, DMACMock_DisableInterrupt);
}

static void ResetControllerRegisters(void)
{
    MOCK_HAL_I2C.IdRev  = 0x020210AB; // Major: 0xA, Minor: 0xB
    MOCK_HAL_I2C.Cfg    = 0x00000003; // 16 bytes FIFO
    MOCK_HAL_I2C.IntEn  = 0x00000000;
    MOCK_HAL_I2C.Status = 0x00000001;
    MOCK_HAL_I2C.Addr   = 0x00000000;
    MOCK_HAL_I2C.Data   = 0x00000000;
    MOCK_HAL_I2C.Ctrl   = 0x00001E00;
    MOCK_HAL_I2C.Cmd    = 0x00000000;
    MOCK_HAL_I2C.Setup  = 0x05252100;
    MOCK_HAL_I2C.IdRev  = 0x020210AB;
}

static void CheckTimingParams(const I2CMode mode)
{
    //
    // Expected timing parameters ranges depending on selected mode
    // for Timin Parameter Multiplier (tpm), Spike Suppression Width (sp),
    // Data Setup Time (sudat) and Data Hold Time (hddat)
    // -1 means no specified value
    //
    // value, Standard Mode, Fast-mode, Fast-Mode+
    int16_t exp_tpm[]   = {0, 0, 0};
    int16_t min_sp[]    = {-1, 0, 0};
    int16_t max_sp[]    = {-1, 50, 50};
    int16_t min_sudat[] = {250, 100, 50};
    int16_t max_sudat[] = {-1, -1, -1};
    int16_t min_hddat[] = {300, 300, 0};
    int16_t max_hddat[] = {-1, -1, -1};
    int16_t min_sclhi[] = {4000, 600, 260};
    int16_t max_sclhi[] = {-1, -1, -1};
    int16_t min_scllo[] = {4700, 1300, 500};
    int16_t max_scllo[] = {-1, -1, -1};

    //
    // Programmed values in controller
    //
    uint16_t pg_tpm                     = HAL_TPM;
    uint16_t pg_spike_suppression_width = HAL_T_SP * (T_PCLK * (HAL_TPM + 1));
    uint16_t pg_setup_time              = ((2 * T_PCLK) +
                                           (((2 + HAL_T_SP + HAL_T_SUDAT) *
                                             T_PCLK) * (HAL_TPM + 1)));
    uint16_t pg_hold_time = ((2 * T_PCLK) +
                             (((2 + HAL_T_SP + HAL_T_HDDAT) * T_PCLK) *
                              (HAL_TPM + 1)));

    uint16_t pg_sclhi_period = ((2 * T_PCLK) +
                                (((2 + HAL_T_SP + HAL_T_SCLHI) * T_PCLK) *
                                 (HAL_TPM + 1)));

    uint16_t pg_scllo_period = ((2 * T_PCLK) +
                                (((2 + HAL_T_SP +
                                   (HAL_T_SCLHI * (1 + HAL_T_SCLRATIO))) *
                                  T_PCLK) *
                                 (HAL_TPM + 1)));

    //
    // Check programmed values against valid range
    //
    CHECK((exp_tpm[mode] == -1) || (exp_tpm[mode] == pg_tpm));

    CHECK((min_sp[mode] == -1) || (min_sp[mode] <= pg_spike_suppression_width));
    CHECK((max_sp[mode] == -1) || (max_sp[mode] >= pg_spike_suppression_width));

    CHECK((min_sudat[mode] == -1) || (min_sudat[mode] <= pg_setup_time));
    CHECK((max_sudat[mode] == -1) || (max_sudat[mode] >= pg_setup_time));

    CHECK((min_hddat[mode] == -1) || (min_hddat[mode] <= pg_hold_time));
    CHECK((max_hddat[mode] == -1) || (max_hddat[mode] >= pg_hold_time));

    CHECK((min_sclhi[mode] == -1) || (min_sclhi[mode] <= pg_sclhi_period));
    CHECK((max_sclhi[mode] == -1) || (max_sclhi[mode] >= pg_sclhi_period));

    CHECK((min_scllo[mode] == -1) || (min_scllo[mode] <= pg_scllo_period));
    CHECK((max_scllo[mode] == -1) || (max_scllo[mode] >= pg_scllo_period));
}

static void SetupController(I2CRole role,
                            I2CMode mode)
{
    default_setup.role = role;
    default_setup.mode = mode;

    LONGS_EQUAL(I2C_OK,
                I2C_SetupController((I2CRegisters*) &MOCK_HAL_I2C, &default_setup));
}

static void LaunchDefaultTransaction(void)
{
    if ((default_transaction.data_path == I2C_USE_DMA) && (default_transaction.data_count != 0)) {
        ExpectDMACChannelSetup(default_transaction.direction);
        ExpectDMACTransferSetup(default_transaction.direction);
        ExpectDMACChannelEnabled();
    }
    ExpectExternalInterruptEnabled();
    LONGS_EQUAL(I2C_OK, I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

static void LaunchTransaction(I2CDirection dir,
                              I2CDataPath  data_path)
{
    default_transaction.data_path = data_path;
    default_transaction.direction = dir;

    LaunchDefaultTransaction();
}

static void CheckSetupShutdownController(I2CRole role,
                                         I2CMode mode)
{
    SetupController(role, mode);
    CheckTimingParams(mode);
    CHECK_EQUAL(role, (MOCK_HAL_I2C.Setup & I2C_SETUP_MASTER_MASK) >> I2C_SETUP_MASTER_OFFSET);
    CHECK((MOCK_HAL_I2C.Setup & I2C_SETUP_IICEN_MASK) >> I2C_SETUP_IICEN_OFFSET);
    ExpectExternalInterruptDisabled();
    LONGS_EQUAL(I2C_OK, I2C_ShutdownController((I2CRegisters*) &MOCK_HAL_I2C));
    CHECK_FALSE((MOCK_HAL_I2C.Setup & I2C_SETUP_IICEN_MASK) >> I2C_SETUP_IICEN_OFFSET);
}
}

TEST_GROUP(I2C_DeviceIrqHandler)
{
    void setup(void)
    {
        mock().strictOrder();
        mock().installComparator("DMACChannelConfig*", channel_config_comparator);
        mock().installComparator("DMACTransferConfig*", transfer_config_comparator);
        InstallMockFunctions();
        ResetControllerRegisters();
        ResetStaticVariables();
        LONGS_EQUAL(I2C_OK, I2C_Create((I2CRegisters*) &MOCK_HAL_I2C));
        SetupController(I2C_MASTER, I2C_STANDARD_MODE);
    }

    void teardown(void)
    {
        mock().checkExpectations();
        mock().clear();
        mock().removeAllComparatorsAndCopiers();
    }
};

TEST(I2C_DeviceIrqHandler, NullDeviceReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA, I2C_DeviceIrqHandler(NULL));
}

TEST(I2C_DeviceIrqHandler, TxMasterFifoCallbackCalledWithAddrHit)
{
    LaunchTransaction(I2C_TX, I2C_USE_FIFO);
    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_ADDRHIT_MASK;
    ExpectTransactionComplete(I2C_OK);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, TxMasterFifoCallbackCalledWithAddrHitError)
{
    LaunchTransaction(I2C_TX, I2C_USE_FIFO);
    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_FIFOEMPTY_MASK;
    ExpectTransactionComplete(I2C_ADDR_HIT_ERROR);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, TxMasterFifoCallbackNotCalledWithFifoEmpty)
{
    LaunchTransaction(I2C_TX, I2C_USE_FIFO);
    MOCK_HAL_I2C.Status = I2C_STATUS_FIFOEMPTY_MASK;
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, RxMasterFifoCallbackCalledWithAddrHit)
{
    LaunchTransaction(I2C_RX, I2C_USE_FIFO);
    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_ADDRHIT_MASK;
    ExpectTransactionComplete(I2C_OK);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, RxMasterFifoCallbackCalledWithAddrHitError)
{
    LaunchTransaction(I2C_RX, I2C_USE_FIFO);
    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_FIFOFULL_MASK;
    ExpectTransactionComplete(I2C_ADDR_HIT_ERROR);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, RxMasterFifoCallbackNotCalledWithFifoFull)
{
    LaunchTransaction(I2C_RX, I2C_USE_FIFO);
    MOCK_HAL_I2C.Status = I2C_STATUS_FIFOFULL_MASK;
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, TxMasterDmaCallbackCalledWithAddrHit)
{
    LaunchTransaction(I2C_TX, I2C_USE_DMA);

    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_ADDRHIT_MASK;
    ExpectTransactionComplete(I2C_OK);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, TxMasterDmaCallbackCalledWithAddrHitError)
{
    LaunchTransaction(I2C_TX, I2C_USE_DMA);

    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK;
    ExpectTransactionComplete(I2C_ADDR_HIT_ERROR);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, RxMasterDmaCallbackCalledWithAddrHit)
{
    LaunchTransaction(I2C_RX, I2C_USE_DMA);

    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_ADDRHIT_MASK;
    ExpectTransactionComplete(I2C_OK);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, RxMasterDmaCallbackCalledWithAddrHitError)
{
    LaunchTransaction(I2C_RX, I2C_USE_DMA);

    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK;
    ExpectTransactionComplete(I2C_ADDR_HIT_ERROR);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST(I2C_DeviceIrqHandler, NullCallbackDoesNotSnag)
{
    default_transaction.callback = NULL;
    LaunchDefaultTransaction();

    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_ADDRHIT_MASK;
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));

    MOCK_HAL_I2C.Cmd             = I2C_CMD_NO_ACTION;
    default_transaction.callback = &(MockTransactionCompleteCallback);
    LaunchDefaultTransaction();

    MOCK_HAL_I2C.Status = I2C_STATUS_CMPL_MASK | I2C_STATUS_ADDRHIT_MASK;
    ExpectTransactionComplete(I2C_OK);
    LONGS_EQUAL(I2C_OK, I2C_DeviceIrqHandler((I2CRegisters*) &MOCK_HAL_I2C));
}

TEST_GROUP(I2C_LaunchTransaction)
{
    void setup(void)
    {
        mock().strictOrder();
        mock().installComparator("DMACChannelConfig*", channel_config_comparator);
        mock().installComparator("DMACTransferConfig*", transfer_config_comparator);
        InstallMockFunctions();
        ResetControllerRegisters();
        ResetStaticVariables();
        LONGS_EQUAL(I2C_OK, I2C_Create((I2CRegisters*) &MOCK_HAL_I2C));
        LONGS_EQUAL(I2C_OK, I2C_SetupController((I2CRegisters*) &MOCK_HAL_I2C, &default_setup));
    }

    void teardown(void)
    {
        mock().checkExpectations();
        mock().clear();
        mock().removeAllComparatorsAndCopiers();
    }
};

TEST(I2C_LaunchTransaction, NullDeviceReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction(NULL, &default_transaction));
}

TEST(I2C_LaunchTransaction, NullDescriptorReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, NULL));
}

TEST(I2C_LaunchTransaction, NullDataBufferWithNonZeroLengthReturnsInvalidInputData)
{
    default_transaction.data = NULL;
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, NonNullDataBufferWithZeroLengthReturnsInvalidInputData)
{
    default_transaction.data_count = 0;
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, RxMasterWithNullDataBufferAndZeroDataLengthReturnsInvalidInputData)
{
    default_transaction.data       = NULL;
    default_transaction.data_count = 0;
    default_transaction.direction  = I2C_RX;
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, MoreThan256bytesReturnsInvalidInputData)
{
    default_transaction.data_count = 257;
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, OutOfBound10bitAddressReturnsInvalidInputData)
{
    default_transaction.addressing_mode = I2C_ADDRESSING_MODE_10_BIT;
    default_transaction.address         = 0x3FF + 1;
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, OutOfBound7bitAddressReturnsInvalidInputData)
{
    default_transaction.addressing_mode = I2C_ADDRESSING_MODE_7_BIT;
    default_transaction.address         = 0x7F + 1;
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, NotEnabledReturnsControllerNotEnabled)
{
    MOCK_HAL_I2C.Setup &= ~I2C_SETUP_IICEN_MASK;
    LONGS_EQUAL(I2C_CONTROLLER_NOT_ENABLED,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, CommandPendingReturnsCommandPending)
{
    MOCK_HAL_I2C.Cmd = (0x01 << I2C_CMD_CMD_OFFSET) & I2C_CMD_CMD_MASK;
    LONGS_EQUAL(I2C_CMD_PENDING,
                I2C_LaunchTransaction((I2CRegisters*) &MOCK_HAL_I2C, &default_transaction));
}

TEST(I2C_LaunchTransaction, MasterFifoSetsDataCountToZeroWhenSending256bytes)
{
    MOCK_HAL_I2C.Ctrl             |= I2C_CTRL_DATACNT_MASK;
    default_transaction.data_count = 256;

    for (I2CDirection dir = I2C_TX; dir <= I2C_RX; dir++) {
        LaunchTransaction(dir, I2C_USE_FIFO);

        CHECK_EQUAL(0, ((MOCK_HAL_I2C.Ctrl & I2C_CTRL_DATACNT_MASK) >> I2C_CTRL_DATACNT_OFFSET));
        MOCK_HAL_I2C.Cmd = I2C_CMD_NO_ACTION;
    }
}

TEST(I2C_LaunchTransaction, MasterDmaSetsDataCountToZeroWhenSending256bytes)
{
    MOCK_HAL_I2C.Ctrl             |= I2C_CTRL_DATACNT_MASK;
    default_transaction.data_count = 256;

    for (I2CDirection dir = I2C_TX; dir <= I2C_RX; dir++) {
        expected_dmac_transfer_config[dir].transfer_size = 256;
        LaunchTransaction(dir, I2C_USE_DMA);

        CHECK_EQUAL(0, ((MOCK_HAL_I2C.Ctrl & I2C_CTRL_DATACNT_MASK) >> I2C_CTRL_DATACNT_OFFSET));
        MOCK_HAL_I2C.Cmd = I2C_CMD_NO_ACTION;
    }
}

TEST(I2C_LaunchTransaction, Master10bitsAddressingMode)
{
    default_transaction.addressing_mode = I2C_ADDRESSING_MODE_10_BIT;

    for (I2CDirection dir = I2C_TX; dir <= I2C_RX; dir++) {
        for (I2CDataPath path = I2C_USE_FIFO; path <= I2C_USE_DMA; path++) {
            LaunchTransaction(dir, path);
            CHECK_EQUAL(default_transaction.addressing_mode,
                        (MOCK_HAL_I2C.Setup & I2C_SETUP_ADDRESSING_MASK) >>
                        I2C_SETUP_ADDRESSING_OFFSET);
            MOCK_HAL_I2C.Cmd = I2C_CMD_NO_ACTION;
        }
    }
}

TEST(I2C_LaunchTransaction, TxMasterWithNullDataBufferAndZeroDataLengthLaunchesScan)
{
    default_transaction.data       = NULL;
    default_transaction.data_count = 0;

    for (I2CDataPath path = I2C_USE_FIFO; path <= I2C_USE_DMA; path++) {
        LaunchTransaction(I2C_TX, path);

        // Check Phase control
        CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_START_MASK) >> I2C_CTRL_PHASE_START_OFFSET);
        CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_ADDR_MASK) >> I2C_CTRL_PHASE_ADDR_OFFSET);
        CHECK_EQUAL(0,
                    (MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_DATA_MASK) >> I2C_CTRL_PHASE_DATA_OFFSET);
        CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_STOP_MASK) >> I2C_CTRL_PHASE_STOP_OFFSET);

        MOCK_HAL_I2C.Ctrl = 0x00001E00;
        MOCK_HAL_I2C.Cmd  = 0x00000000;
    }
}

TEST(I2C_LaunchTransaction, TxMasterFifoWorksAsExpected)
{
    // Set Fifo Full status to ensure LaunchTransaction also enables Fifo Empty interrupt
    MOCK_HAL_I2C.Status |= I2C_STATUS_FIFOFULL_MASK;

    LaunchTransaction(I2C_TX, I2C_USE_FIFO);

    // Check DMA is not enabled
    CHECK_EQUAL(0, (MOCK_HAL_I2C.Setup & I2C_SETUP_DMAEN_MASK) >> I2C_SETUP_DMAEN_OFFSET);

    // Check Phase control
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_START_MASK) >> I2C_CTRL_PHASE_START_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_ADDR_MASK) >> I2C_CTRL_PHASE_ADDR_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_DATA_MASK) >> I2C_CTRL_PHASE_DATA_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_STOP_MASK) >> I2C_CTRL_PHASE_STOP_OFFSET);

    // Check direction
    CHECK_EQUAL(0, (MOCK_HAL_I2C.Ctrl & I2C_CTRL_DIR_MASK) >> I2C_CTRL_DIR_OFFSET);

    // Check Address (target slave)
    CHECK_EQUAL(default_transaction.address,
                (MOCK_HAL_I2C.Addr & I2C_ADDR_ADDR_MASK) >> I2C_ADDR_ADDR_OFFSET);
    CHECK_EQUAL(default_transaction.addressing_mode,
                (MOCK_HAL_I2C.Setup & I2C_SETUP_ADDRESSING_MASK) >> I2C_SETUP_ADDRESSING_OFFSET);

    // Check number of bytes to be transmitted
    CHECK_EQUAL(default_transaction.data_count,
                ((MOCK_HAL_I2C.Ctrl & I2C_CTRL_DATACNT_MASK) >> I2C_CTRL_DATACNT_OFFSET));

    // Check Interrupts are enabled
    CHECK((MOCK_HAL_I2C.IntEn & I2C_INTEN_CMPL_MASK) >> I2C_INTEN_CMPL_OFFSET);
    CHECK((MOCK_HAL_I2C.IntEn & I2C_INTEN_FIFOEMPTY_MASK) >> I2C_INTEN_FIFOEMPTY_OFFSET);

    // Check Command Register equals "Issue Transaction"
    CHECK_EQUAL(0x01u, (MOCK_HAL_I2C.Cmd & I2C_CMD_CMD_MASK) >> I2C_CMD_CMD_OFFSET);
}

TEST(I2C_LaunchTransaction, RxMasterFifoWorksAsExpected)
{
    LaunchTransaction(I2C_RX, I2C_USE_FIFO);

    // Check DMA is not enabled
    CHECK_EQUAL(0, (MOCK_HAL_I2C.Setup & I2C_SETUP_DMAEN_MASK) >> I2C_SETUP_DMAEN_OFFSET);

    // Check Phase control
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_START_MASK) >> I2C_CTRL_PHASE_START_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_ADDR_MASK) >> I2C_CTRL_PHASE_ADDR_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_DATA_MASK) >> I2C_CTRL_PHASE_DATA_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_STOP_MASK) >> I2C_CTRL_PHASE_STOP_OFFSET);

    // Check direction
    CHECK_EQUAL(1, (MOCK_HAL_I2C.Ctrl & I2C_CTRL_DIR_MASK) >> I2C_CTRL_DIR_OFFSET);

    // Check Address (target slave)
    CHECK_EQUAL(default_transaction.address,
                (MOCK_HAL_I2C.Addr & I2C_ADDR_ADDR_MASK) >> I2C_ADDR_ADDR_OFFSET);
    CHECK_EQUAL(default_transaction.addressing_mode,
                (MOCK_HAL_I2C.Setup & I2C_SETUP_ADDRESSING_MASK) >> I2C_SETUP_ADDRESSING_OFFSET);

    // Check number of bytes to be transmitted
    CHECK_EQUAL(default_transaction.data_count,
                ((MOCK_HAL_I2C.Ctrl & I2C_CTRL_DATACNT_MASK) >> I2C_CTRL_DATACNT_OFFSET));

    // Check Interrupts are enabled
    CHECK((MOCK_HAL_I2C.IntEn & I2C_INTEN_CMPL_MASK) >> I2C_INTEN_CMPL_OFFSET);
    CHECK((MOCK_HAL_I2C.IntEn & I2C_INTEN_FIFOFULL_MASK) >> I2C_INTEN_FIFOFULL_OFFSET);

    // Check Command Register equals "Issue Transaction"
    CHECK_EQUAL(0x01u, (MOCK_HAL_I2C.Cmd & I2C_CMD_CMD_MASK) >> I2C_CMD_CMD_OFFSET);
}

TEST(I2C_LaunchTransaction, TxMasterDmaWorksAsExpected)
{
    LaunchTransaction(I2C_TX, I2C_USE_DMA);

    // Check DMA is enabled
    CHECK_EQUAL(1, (MOCK_HAL_I2C.Setup & I2C_SETUP_DMAEN_MASK) >> I2C_SETUP_DMAEN_OFFSET);

    // Check Phase control
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_START_MASK) >> I2C_CTRL_PHASE_START_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_ADDR_MASK) >> I2C_CTRL_PHASE_ADDR_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_DATA_MASK) >> I2C_CTRL_PHASE_DATA_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_STOP_MASK) >> I2C_CTRL_PHASE_STOP_OFFSET);

    // Check direction
    CHECK_EQUAL(0, (MOCK_HAL_I2C.Ctrl & I2C_CTRL_DIR_MASK) >> I2C_CTRL_DIR_OFFSET);

    // Check Address (target slave)
    CHECK_EQUAL(default_transaction.address,
                (MOCK_HAL_I2C.Addr & I2C_ADDR_ADDR_MASK) >> I2C_ADDR_ADDR_OFFSET);
    CHECK_EQUAL(default_transaction.addressing_mode,
                (MOCK_HAL_I2C.Setup & I2C_SETUP_ADDRESSING_MASK) >> I2C_SETUP_ADDRESSING_OFFSET);

    // Check number of bytes to be transmitted
    CHECK_EQUAL(default_transaction.data_count,
                ((MOCK_HAL_I2C.Ctrl & I2C_CTRL_DATACNT_MASK) >> I2C_CTRL_DATACNT_OFFSET));

    // Check Interrupts
    CHECK((MOCK_HAL_I2C.IntEn & I2C_INTEN_CMPL_MASK) >> I2C_INTEN_CMPL_OFFSET);
    CHECK_EQUAL(0, (MOCK_HAL_I2C.IntEn & I2C_INTEN_FIFOEMPTY_MASK) >> I2C_INTEN_FIFOEMPTY_OFFSET);
    CHECK_EQUAL(0, (MOCK_HAL_I2C.IntEn & I2C_INTEN_FIFOFULL_MASK) >> I2C_INTEN_FIFOFULL_OFFSET);

    // Check Command Register equals "Issue Transaction"
    CHECK_EQUAL(0x01u, (MOCK_HAL_I2C.Cmd & I2C_CMD_CMD_MASK) >> I2C_CMD_CMD_OFFSET);
}

TEST(I2C_LaunchTransaction, RxMasterDmaWorksAsExpected)
{
    LaunchTransaction(I2C_RX, I2C_USE_DMA);

    // Check DMA is enabled
    CHECK_EQUAL(1, (MOCK_HAL_I2C.Setup & I2C_SETUP_DMAEN_MASK) >> I2C_SETUP_DMAEN_OFFSET);

    // Check Phase control
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_START_MASK) >> I2C_CTRL_PHASE_START_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_ADDR_MASK) >> I2C_CTRL_PHASE_ADDR_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_DATA_MASK) >> I2C_CTRL_PHASE_DATA_OFFSET);
    CHECK((MOCK_HAL_I2C.Ctrl & I2C_CTRL_PHASE_STOP_MASK) >> I2C_CTRL_PHASE_STOP_OFFSET);

    // Check direction
    CHECK_EQUAL(1, (MOCK_HAL_I2C.Ctrl & I2C_CTRL_DIR_MASK) >> I2C_CTRL_DIR_OFFSET);

    // Check Address (target slave)
    CHECK_EQUAL(default_transaction.address,
                (MOCK_HAL_I2C.Addr & I2C_ADDR_ADDR_MASK) >> I2C_ADDR_ADDR_OFFSET);
    CHECK_EQUAL(default_transaction.addressing_mode,
                (MOCK_HAL_I2C.Setup & I2C_SETUP_ADDRESSING_MASK) >> I2C_SETUP_ADDRESSING_OFFSET);

    // Check number of bytes to be transmitted
    CHECK_EQUAL(default_transaction.data_count,
                ((MOCK_HAL_I2C.Ctrl & I2C_CTRL_DATACNT_MASK) >> I2C_CTRL_DATACNT_OFFSET));

    // Check Interrupts
    CHECK((MOCK_HAL_I2C.IntEn & I2C_INTEN_CMPL_MASK) >> I2C_INTEN_CMPL_OFFSET);
    CHECK_EQUAL(0, (MOCK_HAL_I2C.IntEn & I2C_INTEN_FIFOEMPTY_MASK) >> I2C_INTEN_FIFOEMPTY_OFFSET);
    CHECK_EQUAL(0, (MOCK_HAL_I2C.IntEn & I2C_INTEN_FIFOFULL_MASK) >> I2C_INTEN_FIFOFULL_OFFSET);

    // Check Command Register equals "Issue Transaction"
    CHECK_EQUAL(0x01u, (MOCK_HAL_I2C.Cmd & I2C_CMD_CMD_MASK) >> I2C_CMD_CMD_OFFSET);
}

TEST_GROUP(I2C_ShutdownController)
{
    void setup(void)
    {
        mock().strictOrder();
        ResetControllerRegisters();
        LONGS_EQUAL(I2C_OK, I2C_Create((I2CRegisters*) &MOCK_HAL_I2C));
    }

    void teardown(void)
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(I2C_ShutdownController, NullDeviceReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA, I2C_ShutdownController(NULL));
}

TEST_GROUP(I2C_SetupController)
{
    void setup(void)
    {
        mock().strictOrder();
        ResetControllerRegisters();
        LONGS_EQUAL(I2C_OK, I2C_Create((I2CRegisters*) &MOCK_HAL_I2C));
    }

    void teardown(void)
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(I2C_SetupController, NullDeviceReturnsInvalidInputData)
{
    I2CSetupInfo setup_info = {I2C_MASTER, I2C_STANDARD_MODE};

    LONGS_EQUAL(I2C_INVALID_INPUT_DATA, I2C_SetupController(NULL, &setup_info));
}

TEST(I2C_SetupController, NullSetupInfoReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_SetupController((I2CRegisters*) &MOCK_HAL_I2C, NULL));
}

TEST(I2C_SetupController, UnsupportedModeReturnsInvalidInputData)
{
    I2CSetupInfo setup_info = {I2C_MASTER, I2C_UNSUPPORTED_MODE};

    LONGS_EQUAL(I2C_INVALID_INPUT_DATA,
                I2C_SetupController((I2CRegisters*) &MOCK_HAL_I2C, &setup_info));
}

TEST(I2C_SetupController, SetupShutdownWorksAsExpected)
{
    CheckSetupShutdownController(I2C_MASTER, I2C_STANDARD_MODE);
    CheckSetupShutdownController(I2C_SLAVE, I2C_STANDARD_MODE);
    CheckSetupShutdownController(I2C_MASTER, I2C_FAST_MODE);
    CheckSetupShutdownController(I2C_SLAVE, I2C_FAST_MODE);
    CheckSetupShutdownController(I2C_MASTER, I2C_FAST_MODE_PLUS);
    CheckSetupShutdownController(I2C_SLAVE, I2C_FAST_MODE_PLUS);
}

TEST_GROUP(I2C_Create)
{
    void setup(void)
    {
        mock().strictOrder();
    }

    void teardown(void)
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(I2C_Create, NullDeviceReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA, I2C_Create(NULL));
}

TEST_GROUP(I2C_GetConfig)
{
    void setup(void)
    {
        mock().strictOrder();
        ResetStaticVariables();
        ResetControllerRegisters();
        LONGS_EQUAL(I2C_OK, I2C_Create((I2CRegisters*) &MOCK_HAL_I2C));
    }

    void teardown(void)
    {
        mock().checkExpectations();
        mock().clear();
    }
};

TEST(I2C_GetConfig, NullDeviceReturnsInvalidInputData)
{
    I2CConfig* return_value;

    LONGS_EQUAL(I2C_INVALID_INPUT_DATA, I2C_GetConfig(NULL, &return_value));
}

TEST(I2C_GetConfig, NullReturnValueReturnsInvalidInputData)
{
    LONGS_EQUAL(I2C_INVALID_INPUT_DATA, I2C_GetConfig((I2CRegisters*) &MOCK_HAL_I2C, NULL));
}

TEST(I2C_GetConfig, ReturnsConfig)
{
    I2CConfig* i2c_config;

    LONGS_EQUAL(I2C_OK, I2C_GetConfig((I2CRegisters*) &MOCK_HAL_I2C, &i2c_config));

    LONGS_EQUAL(16, i2c_config->fifo_size);
    LONGS_EQUAL(0x0A, i2c_config->id_rev.major);
    LONGS_EQUAL(0x0B, i2c_config->id_rev.minor);
}
