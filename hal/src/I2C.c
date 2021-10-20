/*
 * Copyright (c) THEDEVHUTS, 2020.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact THEDEVHUTS (contact@thedevhuts.com) for licensing agreement
 * opportunities.
 *
 * Contributor: Julien Gros
 *
 */

#include "DMAC.h"
#include "ExternalInterrupts.h"
#include "I2C.h"

#define I2C_CLK                50 // APB clock in MHz
#define I2C_INTERRUPT_PRIORITY 1

#define I2C_TPM 0 // Timing Parameter Multiplier
#define T_SP    2 // Spike Suppression Width

// Data Setup Time
#define T_SUDAT_STD     7u
#define T_SUDAT_FM      0u
#define T_SUDAT_FM_PLUS 0u

// Data Hold Time
#define T_HDDAT_STD     9u
#define T_HDDAT_FM      9u
#define T_HDDAT_FM_PLUS 0u

// SCL Clock
#define T_SCLHI_STD     229u
#define T_SCLHI_FM      30u
#define T_SCLHI_FM_PLUS 10u

// SCL Ratio
#define T_SCLRATIO_STD     0u
#define T_SCLRATIO_FM      1u
#define T_SCLRATIO_FM_PLUS 1u

#define T_SUDAT(mode) \
    (((mode) == I2C_STANDARD_MODE) ? T_SUDAT_STD : \
     ((mode) == I2C_FAST_MODE) ? T_SUDAT_FM : T_SUDAT_FM_PLUS)

#define T_HDDAT(mode) \
    (((mode) == I2C_STANDARD_MODE) ? T_HDDAT_STD : \
     ((mode) == I2C_FAST_MODE) ? T_HDDAT_FM : T_HDDAT_FM_PLUS)

#define T_SCLHI(mode) \
    (((mode) == I2C_STANDARD_MODE) ? T_SCLHI_STD : \
     ((mode) == I2C_FAST_MODE) ? T_SCLHI_FM : T_SCLHI_FM_PLUS)

#define T_SCLRATIO(mode) \
    (((mode) == I2C_STANDARD_MODE) ? T_SCLRATIO_STD : \
     ((mode) == I2C_FAST_MODE) ? T_SCLRATIO_FM : T_SCLRATIO_FM_PLUS)

typedef struct {
    I2CRole           role;
    I2CMode           mode;
    I2CDirection      dir;
    I2CAddressingMode addr_mode;
    uint16_t          addr;   // Target Address if Master, Controller Address if Slave
    I2CDataPath       data_path;
    uint8_t*          data;
    uint16_t          remaining_data;
    I2CCallback       callback;
} I2CTransaction;

static I2CConfig i2c_config;
static volatile I2CTransaction current_transaction;

static void ReadHWConfig(I2CRegisters* i2c_dev);
static bool I2CCmdPending(I2CRegisters* i2c_dev);
static void I2CEnable(I2CRegisters* i2c_dev);
static void I2CDisable(I2CRegisters* i2c_dev);
static bool I2CEnabled(I2CRegisters* i2c_dev);
static void EnableInterrupt(I2CRegisters* i2c_dev,
                            uint32_t      priority);
static void DisableInterrupt(I2CRegisters* i2c_dev);
static void WriteAvailableData(I2CRegisters* i2c_dev);
static void ReadAvailableData(I2CRegisters* i2c_dev);
static I2CReturnCode SetupDataPath(I2CRegisters* i2c_dev);

static void ReadHWConfig(I2CRegisters* i2c_dev)
{
    i2c_config.id_rev.id =
        (uint32_t) ((i2c_dev->IdRev & I2C_IDREV_ID_MASK) >> I2C_IDREV_ID_OFFSET);
    i2c_config.id_rev.major =
        (uint8_t) ((i2c_dev->IdRev & I2C_IDREV_MAJOR_MASK) >> I2C_IDREV_MAJOR_OFFSET);
    i2c_config.id_rev.minor =
        (uint8_t) ((i2c_dev->IdRev & I2C_IDREV_MINOR_MASK) >> I2C_IDREV_MINOR_OFFSET);
    i2c_config.fifo_size =
        (uint8_t) (0x02 << ((i2c_dev->Cfg & I2C_CFG_FIFOSIZE_MASK) >> I2C_CFG_FIFOSIZE_OFFSET));
}

static bool I2CCmdPending(I2CRegisters* i2c_dev)
{
    return (i2c_dev->Cmd & I2C_CMD_CMD_MASK) != 0;
}

static void I2CEnable(I2CRegisters* i2c_dev)
{
    i2c_dev->Setup |= (0x01 << I2C_SETUP_IICEN_OFFSET) & I2C_SETUP_IICEN_MASK;
}

static void I2CDisable(I2CRegisters* i2c_dev)
{
    i2c_dev->Setup &= ~I2C_SETUP_IICEN_MASK;
}

static bool I2CEnabled(I2CRegisters* i2c_dev)
{
    return i2c_dev->Setup & I2C_SETUP_IICEN_MASK;
}

static bool FifoEmpty(I2CRegisters* i2c_dev)
{
    return i2c_dev->Status & I2C_STATUS_FIFOEMPTY_MASK;
}

static bool FifoFull(I2CRegisters* i2c_dev)
{
    return i2c_dev->Status & I2C_STATUS_FIFOFULL_MASK;
}

static void EnableInterrupt(I2CRegisters* i2c_dev,
                            uint32_t      priority)
{
    UNUSED(i2c_dev);
    ExternalInterrupts_EnableInterrupt(EXTERNAL_IRQ_I2C_SOURCE, priority);
}

static void DisableInterrupt(I2CRegisters* i2c_dev)
{
    UNUSED(i2c_dev);
    ExternalInterrupts_DisableInterrupt(EXTERNAL_IRQ_I2C_SOURCE);
}

static void WriteAvailableData(I2CRegisters* i2c_dev)
{
    while ((!FifoFull(i2c_dev)) &&
           (current_transaction.remaining_data > 0)) {
        i2c_dev->Data = (*current_transaction.data);
        current_transaction.remaining_data--;
        current_transaction.data++;
    }

    if (current_transaction.remaining_data == 0) {
        i2c_dev->IntEn &= ~I2C_INTEN_FIFOEMPTY_MASK;
    }
}

static void ReadAvailableData(I2CRegisters* i2c_dev)
{
    while ((!FifoEmpty(i2c_dev)) &&
           (current_transaction.remaining_data > 0)) {
        (*current_transaction.data) = i2c_dev->Data;
        current_transaction.remaining_data--;
        current_transaction.data++;
    }

    if (current_transaction.remaining_data == 0) {
        i2c_dev->IntEn &= ~I2C_INTEN_FIFOFULL_MASK;
    }
}

static I2CReturnCode SetupDataPath(I2CRegisters* i2c_dev)
{
    if (current_transaction.remaining_data == 0) {
        // Disable DMA
        i2c_dev->Setup &= ~I2C_SETUP_DMAEN_MASK;

        // Disable fifo interrupts
        i2c_dev->IntEn &= ~I2C_INTEN_FIFOEMPTY_MASK;
        i2c_dev->IntEn &= ~I2C_INTEN_FIFOFULL_MASK;
        return I2C_OK;
    }

    if (current_transaction.data_path == I2C_USE_FIFO) {
        // Disable DMA
        i2c_dev->Setup &= ~I2C_SETUP_DMAEN_MASK;

        // Setup FIFO
        if (current_transaction.dir == I2C_TX) {
            i2c_dev->IntEn |= I2C_INTEN_FIFOEMPTY_MASK;
            WriteAvailableData(i2c_dev);
        } else {
            i2c_dev->IntEn |= I2C_INTEN_FIFOFULL_MASK;
        }
    } else {
        // Setup DMA
        DMACChannelConfig dmac_channel_config;

        dmac_channel_config.channel            = DMAC_CHANNEL_I2C;
        dmac_channel_config.src_bus_index      = 0;
        dmac_channel_config.dst_bus_index      = 0;
        dmac_channel_config.channel_priority   = 1;
        dmac_channel_config.src_burst_size     = DMAC_BURST_SIZE_1;
        dmac_channel_config.src_transfer_width = DMAC_TRANSFER_WIDTH_BYTE;
        dmac_channel_config.dst_transfer_width = DMAC_TRANSFER_WIDTH_BYTE;
        dmac_channel_config.src_handshake_mode = (current_transaction.dir == I2C_RX);
        dmac_channel_config.dst_handshake_mode = (current_transaction.dir == I2C_TX);
        dmac_channel_config.src_addr_ctrl      =
            (current_transaction.dir == I2C_TX) ? DMAC_ADDR_CTRL_INCREMENT : DMAC_ADDR_CTRL_FIXED;
        dmac_channel_config.dst_addr_ctrl =
            (current_transaction.dir == I2C_TX) ? DMAC_ADDR_CTRL_FIXED : DMAC_ADDR_CTRL_INCREMENT;
        dmac_channel_config.src_pair =
            (current_transaction.dir == I2C_TX) ? 0 : DMAC_CHANNEL_I2C;
        dmac_channel_config.dst_pair =
            (current_transaction.dir == I2C_TX) ? DMAC_CHANNEL_I2C : 0;
        if (DMAC_SetupChannel(HAL_DMAC,
                              &dmac_channel_config,
                              false,
                              false,
                              false,
                              I2C_DMACCallback) != DMAC_OK) {
            return I2C_DMAC_ERROR;
        }

        DMACTransferConfig dmac_transfer_config;

        dmac_transfer_config.channel       = DMAC_CHANNEL_I2C;
        dmac_transfer_config.transfer_size = current_transaction.remaining_data;
        dmac_transfer_config.src_address   =
            (current_transaction.dir == I2C_TX) ?
            ((uint32_t) current_transaction.data) : (uint32_t) (&(i2c_dev->Data));
        dmac_transfer_config.dst_address =
            (current_transaction.dir == I2C_TX) ?
            (uint32_t) (&(i2c_dev->Data)) : ((uint32_t) current_transaction.data);
        if (DMAC_SetupTransfer(HAL_DMAC, &dmac_transfer_config) != DMAC_OK) {
            return I2C_DMAC_ERROR;
        }
        if (DMAC_EnableChannel(HAL_DMAC, DMAC_CHANNEL_I2C) != DMAC_OK) {
            return I2C_DMAC_ERROR;
        }
        // Disable fifo interrupts
        i2c_dev->IntEn &= ~I2C_INTEN_FIFOEMPTY_MASK;
        i2c_dev->IntEn &= ~I2C_INTEN_FIFOFULL_MASK;
        // Enable I2C DMA
        i2c_dev->Setup |= I2C_SETUP_DMAEN_MASK;
    }
    return I2C_OK;
}

void I2C_DMACCallback(DMACReturnCode return_code)
{
    if ((return_code != DMAC_TERMINAL_COUNT) &&
        (current_transaction.callback != NULL)) {
        current_transaction.callback(I2C_DMAC_ERROR);
    }
}

I2CReturnCode I2C_Create(I2CRegisters* i2c_dev)
{
    if (i2c_dev == NULL) {
        return I2C_INVALID_INPUT_DATA;
    }
    ReadHWConfig(i2c_dev);
    return I2C_OK;
}

I2CReturnCode I2C_GetConfig(I2CRegisters* i2c_dev,
                            I2CConfig**   return_value)
{
    if ((i2c_dev == NULL) ||
        (return_value == NULL)) {
        return I2C_INVALID_INPUT_DATA;
    }
    *return_value = &i2c_config;
    return I2C_OK;
}

I2CReturnCode I2C_SetupController(I2CRegisters* i2c_dev,
                                  I2CSetupInfo* setup_info)
{
    if ((i2c_dev == NULL) ||
        (setup_info == NULL) ||
        (setup_info->mode >= I2C_UNSUPPORTED_MODE)) {
        return I2C_INVALID_INPUT_DATA;
    }

    i2c_dev->Setup &= ~I2C_SETUP_MASTER_MASK;
    i2c_dev->Setup |= (setup_info->role << I2C_SETUP_MASTER_OFFSET) & I2C_SETUP_MASTER_MASK;

    // Setup Timing Parameter Multiplier
    i2c_dev->TPM |= (I2C_TPM << I2C_TPM_TPM_OFFSET) & I2C_TPM_TPM_MASK;

    // Setup Timing Parameters
    i2c_dev->Setup &= ~I2C_SETUP_T_SP_MASK;
    i2c_dev->Setup |= (T_SP << I2C_SETUP_T_SP_OFFSET) & I2C_SETUP_T_SP_MASK;

    I2CMode mode = setup_info->mode;

    i2c_dev->Setup &= ~I2C_SETUP_T_SUDAT_MASK;
    i2c_dev->Setup |= (T_SUDAT(mode) << I2C_SETUP_T_SUDAT_OFFSET) &
                      I2C_SETUP_T_SUDAT_MASK;

    i2c_dev->Setup &= ~I2C_SETUP_T_HDDAT_MASK;
    i2c_dev->Setup |= (T_HDDAT(mode) << I2C_SETUP_T_HDDAT_OFFSET) &
                      I2C_SETUP_T_HDDAT_MASK;

    i2c_dev->Setup &= ~I2C_SETUP_T_SCLHI_MASK;
    i2c_dev->Setup |= (T_SCLHI(mode) << I2C_SETUP_T_SCLHI_OFFSET) &
                      I2C_SETUP_T_SCLHI_MASK;

    i2c_dev->Setup &= ~I2C_SETUP_T_SCLRATIO_MASK;
    i2c_dev->Setup |= (T_SCLRATIO(mode) << I2C_SETUP_T_SCLRATIO_OFFSET) &
                      I2C_SETUP_T_SCLRATIO_MASK;

    I2CEnable(i2c_dev);

    return I2C_OK;
}

I2CReturnCode I2C_ShutdownController(I2CRegisters* i2c_dev)
{
    if (i2c_dev == NULL) {
        return I2C_INVALID_INPUT_DATA;
    }
    DisableInterrupt(i2c_dev);
    I2CDisable(i2c_dev);
    return I2C_OK;
}

I2CReturnCode I2C_LaunchTransaction(I2CRegisters*             i2c_dev,
                                    I2CTransactionDescriptor* descriptor)
{
    I2CReturnCode ret = I2C_OK;

    if ((i2c_dev == NULL) ||
        (descriptor == NULL) ||
        ((descriptor->data == NULL) && (descriptor->data_count != 0)) ||
        ((descriptor->data == NULL) && (descriptor->data_count == 0) &&
         (descriptor->direction != I2C_TX)) ||
        ((descriptor->data != NULL) && (descriptor->data_count == 0)) ||
        (descriptor->data_count > (I2C_CTRL_DATACNT_MASK + 1)) ||
        ((descriptor->addressing_mode == I2C_ADDRESSING_MODE_10_BIT) &&
         (descriptor->address > I2C_ADDR_MAX_10BIT)) ||
        ((descriptor->addressing_mode == I2C_ADDRESSING_MODE_7_BIT) &&
         (descriptor->address > I2C_ADDR_MAX_7BIT))) {
        return I2C_INVALID_INPUT_DATA;
    }

    if (!I2CEnabled(i2c_dev)) {
        return I2C_CONTROLLER_NOT_ENABLED;
    }

    if (I2CCmdPending(i2c_dev)) {
        return I2C_CMD_PENDING;
    }

    current_transaction.role =
        (bool) ((i2c_dev->Setup & I2C_SETUP_MASTER_MASK) >> I2C_SETUP_MASTER_OFFSET);
    current_transaction.addr           = descriptor->address;
    current_transaction.addr_mode      = descriptor->addressing_mode;
    current_transaction.dir            = descriptor->direction;
    current_transaction.remaining_data = descriptor->data_count;
    current_transaction.data           = descriptor->data;
    current_transaction.data_path      = descriptor->data_path;
    current_transaction.callback       = descriptor->callback;

    // Set address and addressing mode
    i2c_dev->Addr &= ~I2C_ADDR_ADDR_MASK;
    i2c_dev->Addr |= (descriptor->address << I2C_ADDR_ADDR_OFFSET) & I2C_ADDR_ADDR_MASK;

    i2c_dev->Setup &= ~I2C_SETUP_ADDRESSING_MASK;
    i2c_dev->Setup |= (descriptor->addressing_mode << I2C_SETUP_ADDRESSING_OFFSET) &
                      I2C_SETUP_ADDRESSING_MASK;

    // Set transaction Phases
    i2c_dev->Ctrl |= I2C_CTRL_PHASE_START_MASK;
    i2c_dev->Ctrl |= I2C_CTRL_PHASE_ADDR_MASK;
    if (current_transaction.data != NULL) {
        i2c_dev->Ctrl |= I2C_CTRL_PHASE_DATA_MASK;
    } else {
        i2c_dev->Ctrl &= ~I2C_CTRL_PHASE_DATA_MASK;
    }
    i2c_dev->Ctrl |= I2C_CTRL_PHASE_STOP_MASK;

    // Set Direction
    i2c_dev->Ctrl &= ~I2C_CTRL_DIR_MASK;
    i2c_dev->Ctrl |= (descriptor->direction << I2C_CTRL_DIR_OFFSET) & I2C_CTRL_DIR_MASK;

    // Set Data Count
    i2c_dev->Ctrl &= ~I2C_CTRL_DATACNT_MASK;
    i2c_dev->Ctrl |= (descriptor->data_count << I2C_CTRL_DATACNT_OFFSET) & I2C_CTRL_DATACNT_MASK;

    // Setup Data Path (DMA or FIFO)
    if ((ret = SetupDataPath(i2c_dev) != I2C_OK)) {
        return ret;
    }
    i2c_dev->IntEn |= I2C_INTEN_CMPL_MASK;

    // Issue Transaction
    i2c_dev->Cmd |= (I2C_CMD_ISSUE_TRANSACTION << I2C_CMD_CMD_OFFSET) & I2C_CMD_CMD_MASK;

    EnableInterrupt(i2c_dev, I2C_INTERRUPT_PRIORITY);

    return ret;
}

void ExternalInterrupts_I2cIrqHandler(void)
{
    I2C_DeviceIrqHandler(HAL_I2C);
}

I2CReturnCode I2C_DeviceIrqHandler(I2CRegisters* i2c_dev)
{
    if (i2c_dev == NULL) {
        return I2C_INVALID_INPUT_DATA;
    }

    if ((i2c_dev->Status & I2C_STATUS_CMPL_MASK)) {
        I2CReturnCode ret = (i2c_dev->Status &
                             I2C_STATUS_ADDRHIT_MASK) ? I2C_OK : I2C_ADDR_HIT_ERROR;
        if ((ret == I2C_OK) &&
            (current_transaction.dir == I2C_RX) &&
            (current_transaction.data_path != I2C_USE_DMA)) {
            ReadAvailableData(i2c_dev);
        }
        i2c_dev->Status |= I2C_STATUS_CMPL_MASK;
        if (current_transaction.callback != NULL) {
            current_transaction.callback(ret);
        }
        return I2C_OK;
    }

    if (((i2c_dev->Status & I2C_STATUS_FIFOEMPTY_MASK)) &&
        (current_transaction.dir == I2C_TX)) {
        WriteAvailableData(i2c_dev);
        return I2C_OK;
    }

    if (((i2c_dev->Status & I2C_STATUS_FIFOFULL_MASK)) &&
        (current_transaction.dir == I2C_RX)) {
        ReadAvailableData(i2c_dev);
        return I2C_OK;
    }
    return I2C_OK;
}
