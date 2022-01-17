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

#ifndef __I2C_WRAPPER_H
#define __I2C_WRAPPER_H

#include "I2C.h"

typedef enum {
    I2C_WRAPPER_OK,
    I2C_WRAPPER_INVALID_INPUT_DATA,
    I2C_WRAPPER_I2C_MUTEX_NOT_CREATED,
    I2C_WRAPPER_I2C_MUTEX_UNAVAILABLE,
    I2C_WRAPPER_I2C_ERROR,
    I2C_WRAPPER_NB_OF_RETURN_CODES
} I2CWrapperReturnCode;

#ifdef __cplusplus
extern "C" {
#endif

I2CWrapperReturnCode I2CWrapper_Create(void);
void I2CWrapper_Destroy(void);
void I2CWrapper_SpiCallback(I2CReturnCode return_code);

void I2CWrapper_I2CCallback(I2CReturnCode return_code);

extern I2CWrapperReturnCode (* I2CWrapper_LaunchI2CTransaction) (I2CSetupInfo* setup_info,
                                                                 I2CTransactionDescriptor*
                                                                 transaction_descriptor);

#ifdef __cplusplus
}
#endif

#endif // __I2C_WRAPPER_H
