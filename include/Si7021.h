/*
 * Copyright (c) TheDevHuts, 2022.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact TheDevHuts (contact@thedevhuts.com) for licensing agreement
 * opportunities.
 *
 * Contributor: Julien Gros
 *
 */

#ifndef __SI7021_H
#define __SI7021_H

#include "Common.h"

#define SI7021_DEFAULT_ADDR   0X40
#define SI7021_MAX_CMD_LENGTH 2
#define SI7021_MAX_RSP_LENGTH 3 // (MSB/LSB/CHECKSUM)

#define SI7021_RESET_CMD   0xFE
#define SI7021_RESET_DELAY 15 // ms

#define SI7021_REVISION_CMD     0x84B8
#define SI7021_REVISION_RSP_LEN 2

#define SI7021_MEASTEMP_NOHOLD_CMD     0xF3
#define SI7021_MEASTEMP_NOHOLD_RSP_LEN 3
#define SI7021_MEASTEMP_DELAY          20 // ms

#define SI7021_MEASRH_NOHOLD_CMD     0xF5
#define SI7021_MEASRH_NOHOLD_RSP_LEN 3
#define SI7021_MEASRH_DELAY          20 // ms

#define SI7021_CRC8_POLY             0x13100 // CRC8 (16bits) -> x^8 + x^5 + x^4 + 1
#define SI7021_MAX_READ_VAL_ATTEMPTS 4

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SI7021_OK,
    SI7021_INVALID_INPUT_DATA,
    SI7021_MUTEX_NOT_CREATED,
    SI7021_TASK_NOT_CREATED,
    SI7021_MUTEX_UNAVAILABLE,
    SI7021_I2C_ERROR,
    SI7021_CHECKSUM_ERROR,
    SI7021_NB_OF_RETURN_CODES
} Si7021ReturnCode;

typedef enum {
    SI7021_REV_1 = 1,
    SI7021_REV_2 = 2,
    SI7021_REV_UNKNOWN
} Si7021FirmwareRevision;

Si7021ReturnCode Si7021_Create(void);
void Si7021_Destroy(void);

Si7021ReturnCode Si7021_Acquire(uint16_t i2c_addr);
void Si7021_Release(void);

Si7021ReturnCode Si7021_ReadRevision(Si7021FirmwareRevision* fw_revision);
Si7021ReturnCode Si7021_ReadTemperature(float* temperature);
Si7021ReturnCode Si7021_ReadHumidity(float* humidity);

#ifdef __cplusplus
}
#endif

#endif // __SI7021_H
