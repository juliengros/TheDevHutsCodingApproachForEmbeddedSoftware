/*
 * Copyright (c) THEDEVHUTS, 2020.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact ANOTHERBRAIN (contact@anotherbrain.ai) for licensing agreement
 * opportunities.
 *
 * Contributor: Julien Gros
 *
 */

#ifndef __I2C_H
#define __I2C_H

#include "CommonDefs.h"
#include "DMAC.h"

#define HAL_I2C_BASE 0xF0A00000
#define HAL_I2C      ((I2CRegisters*) HAL_I2C_BASE)

#define I2C_IDREV_ID_MASK      0xffffff00
#define I2C_IDREV_ID_OFFSET    8
#define I2C_IDREV_MAJOR_MASK   0x000000f0
#define I2C_IDREV_MAJOR_OFFSET 4
#define I2C_IDREV_MINOR_MASK   0x0000000f
#define I2C_IDREV_MINOR_OFFSET 0

#define I2C_CFG_FIFOSIZE_MASK   0x00000003
#define I2C_CFG_FIFOSIZE_OFFSET 0

#define I2C_FIFO_SIZE_2  0x0u
#define I2C_FIFO_SIZE_4  0x1u
#define I2C_FIFO_SIZE_8  0x2u
#define I2C_FIFO_SIZE_16 0x3u

#define I2C_INTEN_CMPL_MASK        0x00000200
#define I2C_INTEN_CMPL_OFFSET      9
#define I2C_INTEN_BYTERECV_MASK    0x00000100
#define I2C_INTEN_BYTERECV_OFFSET  8
#define I2C_INTEN_BYTETRANS_MASK   0x00000080
#define I2C_INTEN_BYTETRANS_OFFSET 7
#define I2C_INTEN_START_MASK       0x00000040
#define I2C_INTEN_START_OFFSET     6
#define I2C_INTEN_STOP_MASK        0x00000020
#define I2C_INTEN_STOP_OFFSET      5
#define I2C_INTEN_ARBLOSE_MASK     0x00000010
#define I2C_INTEN_ARBLOSE_OFFSET   4
#define I2C_INTEN_ADDRHIT_MASK     0x00000008
#define I2C_INTEN_ADDRHIT_OFFSET   3
#define I2C_INTEN_FIFOHALF_MASK    0x00000004
#define I2C_INTEN_FIFOHALF_OFFSET  2
#define I2C_INTEN_FIFOFULL_MASK    0x00000002
#define I2C_INTEN_FIFOFULL_OFFSET  1
#define I2C_INTEN_FIFOEMPTY_MASK   0x00000001
#define I2C_INTEN_FIFOEMPTY_OFFSET 0

#define I2C_STATUS_LINESDA_MASK     0x00004000
#define I2C_STATUS_LINESDA_OFFSET   14
#define I2C_STATUS_LINESCL_MASK     0x00002000
#define I2C_STATUS_LINESCL_OFFSET   13
#define I2C_STATUS_GENCALL_MASK     0x00001000
#define I2C_STATUS_GENCALL_OFFSET   12
#define I2C_STATUS_BUSBUSY_MASK     0x00000800
#define I2C_STATUS_BUSBUSY_OFFSET   11
#define I2C_STATUS_ACK_MASK         0x00000400
#define I2C_STATUS_ACK_OFFSET       10
#define I2C_STATUS_CMPL_MASK        0x00000200
#define I2C_STATUS_CMPL_OFFSET      9
#define I2C_STATUS_BYTERECV_MASK    0x00000100
#define I2C_STATUS_BYTERECV_OFFSET  8
#define I2C_STATUS_BYTETRANS_MASK   0x00000080
#define I2C_STATUS_BYTETRANS_OFFSET 7
#define I2C_STATUS_START_MASK       0x00000040
#define I2C_STATUS_START_OFFSET     6
#define I2C_STATUS_STOP_MASK        0x00000020
#define I2C_STATUS_STOP_OFFSET      5
#define I2C_STATUS_ARBLOSE_MASK     0x00000010
#define I2C_STATUS_ARBLOSE_OFFSET   4
#define I2C_STATUS_ADDRHIT_MASK     0x00000008
#define I2C_STATUS_ADDRHIT_OFFSET   3
#define I2C_STATUS_FIFOHALF_MASK    0x00000004
#define I2C_STATUS_FIFOHALF_OFFSET  2
#define I2C_STATUS_FIFOFULL_MASK    0x00000002
#define I2C_STATUS_FIFOFULL_OFFSET  1
#define I2C_STATUS_FIFOEMPTY_MASK   0x00000001
#define I2C_STATUS_FIFOEMPTY_OFFSET 0

#define I2C_ADDR_ADDR_MASK   0x000003ff
#define I2C_ADDR_ADDR_OFFSET 0

#define I2C_ADDR_MAX_7BIT  0x7fu
#define I2C_ADDR_MAX_10BIT 0x3ffu

#define I2C_DATA_DATA_MASK   0x000000ff
#define I2C_DATA_DATA_OFFSET 0

#define I2C_CTRL_PHASE_START_MASK   0x00001000
#define I2C_CTRL_PHASE_START_OFFSET 12
#define I2C_CTRL_PHASE_ADDR_MASK    0x00000800
#define I2C_CTRL_PHASE_ADDR_OFFSET  11
#define I2C_CTRL_PHASE_DATA_MASK    0x00000400
#define I2C_CTRL_PHASE_DATA_OFFSET  10
#define I2C_CTRL_PHASE_STOP_MASK    0x00000200
#define I2C_CTRL_PHASE_STOP_OFFSET  9
#define I2C_CTRL_DIR_MASK           0x00000100
#define I2C_CTRL_DIR_OFFSET         8
#define I2C_CTRL_DATACNT_MASK       0x000000ff
#define I2C_CTRL_DATACNT_OFFSET     0

#define I2C_CMD_CMD_MASK   0x00000007
#define I2C_CMD_CMD_OFFSET 0

#define I2C_CMD_NO_ACTION         0x00u
#define I2C_CMD_ISSUE_TRANSACTION 0x01u // issue a data transaction (Master only)
#define I2C_CMD_ACK_RESP          0x02u // respond with an ACK to the received byte
#define I2C_CMD_NACK_RESP         0x03u // respond with a NACK to the received byte
#define I2C_CMD_CLEAR_FIFO        0x04u
#define I2C_CMD_RESET             0x05u // abort current transaction, set SDA and SCL lines to the
                                        // open-drain mode, reset the Status Register and the
                                        // Interrupt Enable Register, and empty the FIFO

#define I2C_SETUP_T_SUDAT_MASK      0x1f000000
#define I2C_SETUP_T_SUDAT_OFFSET    24
#define I2C_SETUP_T_SP_MASK         0x00e00000
#define I2C_SETUP_T_SP_OFFSET       21
#define I2C_SETUP_T_HDDAT_MASK      0x001f0000
#define I2C_SETUP_T_HDDAT_OFFSET    16
#define I2C_SETUP_T_SCLRATIO_MASK   0x00002000
#define I2C_SETUP_T_SCLRATIO_OFFSET 13
#define I2C_SETUP_T_SCLHI_MASK      0x00001ff0
#define I2C_SETUP_T_SCLHI_OFFSET    4
#define I2C_SETUP_DMAEN_MASK        0x00000008
#define I2C_SETUP_DMAEN_OFFSET      3
#define I2C_SETUP_MASTER_MASK       0x00000004
#define I2C_SETUP_MASTER_OFFSET     2
#define I2C_SETUP_ADDRESSING_MASK   0x00000002
#define I2C_SETUP_ADDRESSING_OFFSET 1
#define I2C_SETUP_IICEN_MASK        0x00000001
#define I2C_SETUP_IICEN_OFFSET      0

#define I2C_TPM_TPM_MASK   0x0000001f
#define I2C_TPM_TPM_OFFSET 0

typedef struct {
    __I uint32_t  IdRev;
    __I uint32_t  Reserved0[3];
    __I uint32_t  Cfg;
    __IO uint32_t IntEn;
    __IO uint32_t Status;
    __IO uint32_t Addr;
    __IO uint32_t Data;
    __IO uint32_t Ctrl;
    __IO uint32_t Cmd;
    __IO uint32_t Setup;
    __IO uint32_t TPM;
} I2CRegisters;

typedef struct {
    uint32_t id;
    uint8_t  major;
    uint8_t  minor;
} I2CIdRevRegister;

typedef struct {
    I2CIdRevRegister id_rev;
    uint8_t          fifo_size;
} I2CConfig;

typedef enum {
    I2C_OK,
    I2C_INVALID_INPUT_DATA,
    I2C_CONTROLLER_NOT_ENABLED,
    I2C_CMD_PENDING,
    I2C_ADDR_HIT_ERROR,
    I2C_DMAC_ERROR,
    I2C_NB_OF_RETURN_CODES
} I2CReturnCode;

typedef uint8_t I2CMode;
typedef enum {
    I2C_STANDARD_MODE,
    I2C_FAST_MODE,
    I2C_FAST_MODE_PLUS,
    I2C_UNSUPPORTED_MODE
} _I2CMode;

typedef uint8_t I2CDirection;
typedef enum {
    I2C_TX,
    I2C_RX
} _I2CDirection;

typedef uint8_t I2CRole;
typedef enum {
    I2C_SLAVE,
    I2C_MASTER
} _I2CRole;

typedef uint8_t I2CAddressingMode;
typedef enum {
    I2C_ADDRESSING_MODE_7_BIT,
    I2C_ADDRESSING_MODE_10_BIT
} _I2CAddressingMode;

typedef void (* I2CCallback)(I2CReturnCode);

typedef struct {
    I2CRole role;
    I2CMode mode;
} I2CSetupInfo;

typedef uint8_t I2CDataPath;
typedef enum {
    I2C_USE_FIFO,
    I2C_USE_DMA
} _I2CDataPath;

typedef struct {
    I2CDirection      direction;
    I2CAddressingMode addressing_mode;
    uint16_t          address;
    I2CDataPath       data_path;
    uint8_t*          data;
    uint16_t          data_count;
    I2CCallback       callback;
} I2CTransactionDescriptor;

#ifdef __cplusplus
extern "C" {
#endif

I2CReturnCode I2C_Create(I2CRegisters* i2c_dev);
I2CReturnCode I2C_GetConfig(I2CRegisters* i2c_dev,
                            I2CConfig**   return_value);
I2CReturnCode I2C_SetupController(I2CRegisters* i2c_dev,
                                  I2CSetupInfo* setup_info);
I2CReturnCode I2C_ShutdownController(I2CRegisters* i2c_dev);
I2CReturnCode I2C_LaunchTransaction(I2CRegisters*             i2c_dev,
                                    I2CTransactionDescriptor* descriptor);
I2CReturnCode I2C_DeviceIrqHandler(I2CRegisters* i2c_dev);
void I2C_DMACCallback(DMACReturnCode return_code);

#ifdef __cplusplus
}
#endif

#endif // __I2C_H
