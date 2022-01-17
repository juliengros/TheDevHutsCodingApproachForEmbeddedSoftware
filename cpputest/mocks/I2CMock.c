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
#include "CppUTestExt/MockSupport_c.h"
#include "I2CMock.h"

I2CReturnCode I2C_Create(I2CRegisters* i2c_dev)
{
    mock_c()->actualCall("I2C_Create")
    ->withPointerParameters("i2c_dev", i2c_dev);

    return (I2CReturnCode) mock_c()->returnValue().value.intValue;
}

I2CReturnCode I2C_SetupController(I2CRegisters* i2c_dev,
                                  I2CSetupInfo* setup_info)
{
    mock_c()->actualCall("I2C_SetupController")
    ->withPointerParameters("i2c_dev", i2c_dev)
    ->withParameterOfType("I2CSetupInfo*", "setup_info", setup_info);
    return (I2CReturnCode) mock_c()->returnValue().value.intValue;
}

I2CReturnCode I2C_ShutdownController(I2CRegisters* i2c_dev)
{
    mock_c()->actualCall("I2C_ShutdownController")
    ->withPointerParameters("i2c_dev", i2c_dev);
    return (I2CReturnCode) mock_c()->returnValue().value.intValue;
}

I2CReturnCode I2C_LaunchTransaction(I2CRegisters*             i2c_dev,
                                    I2CTransactionDescriptor* descriptor)
{
    mock_c()->actualCall("I2C_LaunchTransaction")
    ->withPointerParameters("i2c_dev", i2c_dev)
    ->withParameterOfType("I2CTransactionDescriptor*", "descriptor", descriptor);
    return (I2CReturnCode) mock_c()->returnValue().value.intValue;
}
