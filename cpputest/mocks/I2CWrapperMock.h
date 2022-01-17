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

#ifndef __I2C_WRAPPER_MOCK_H
#define __I2C_WRAPPER_MOCK_H

#include "I2CWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

I2CWrapperReturnCode I2CWrapperMock_LaunchI2CTransfer(I2CSetupInfo*             setup_info,
                                                      I2CTransactionDescriptor* transacton_descriptor);

#ifdef __cplusplus
}
#endif

#endif // __I2C_WRAPPER_MOCK_H
