/*
 * Copyright (c) ANOTHERBRAIN, 2018.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact ANOTHERBRAIN (contact@anotherbrain.ai) for licensing agreement
 * opportunities.
 *
 * Contributors: Florent Remis / Julien Gros
 *
 */

#include "CppUTestExt/MockSupport_c.h"
#include "I2CWrapperMock.h"

I2CWrapperReturnCode I2CWrapperMock_LaunchI2CTransfer(I2CSetupInfo*             setup_info,
                                                      I2CTransactionDescriptor* transacton_descriptor)
{
    mock_c()->actualCall("I2CWrapper_LaunchI2CTransaction")
    ->withParameterOfType("I2CSetupInfo*", "setup_info", setup_info)
    ->withParameterOfType("I2CTransactionDescriptor*",
                          "transaction_descriptor",
                          transacton_descriptor);
    return (I2CReturnCode) mock_c()->returnValue().value.intValue;
}
