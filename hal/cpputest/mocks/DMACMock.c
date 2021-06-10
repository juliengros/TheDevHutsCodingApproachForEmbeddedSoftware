/*
 * Copyright (c) ANOTHERBRAIN, 2018.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact ANOTHERBRAIN (contact@anotherbrain.ai) for licensing agreement
 * opportunities.
 *
 * Contributor: Florent Remis
 *
 */

#include "CppUTestExt/MockSupport_c.h"
#include "DMACMock.h"

DMACReturnCode DMACMock_EnableInterrupt(DMACRegisters* dmac_dev,
                                        uint32_t       priority)
{
    mock_c()->actualCall("DMAC_EnableInterrupt")
    ->withPointerParameters("dmac_dev", dmac_dev)
    ->withUnsignedIntParameters("priority", priority);
    return (DMACReturnCode) mock_c()->returnValue().value.intValue;
}

DMACReturnCode DMACMock_DisableInterrupt(DMACRegisters* dmac_dev)
{
    mock_c()->actualCall("DMAC_DisableInterrupt")
    ->withPointerParameters("dmac_dev", dmac_dev);
    return (DMACReturnCode) mock_c()->returnValue().value.intValue;
}

DMACReturnCode DMACMock_SetupChannel(DMACRegisters*      dmac_dev,
                                     DMACChannelConfig*  channel_config,
                                     bool                disable_abort_int,
                                     bool                disable_error_int,
                                     bool                disable_terminal_count_int,
                                     DMACChannelCallback callback)
{
    mock_c()->actualCall("DMAC_SetupChannel")
    ->withPointerParameters("dmac_dev", dmac_dev)
    ->withParameterOfType("DMACChannelConfig*", "channel_config", channel_config)
    ->withBoolParameters("disable_abort_int", disable_abort_int)
    ->withBoolParameters("disable_error_int", disable_error_int)
    ->withBoolParameters("disable_terminal_count_int", disable_terminal_count_int)
    ->withPointerParameters("callback", callback);
    return (DMACReturnCode) mock_c()->returnValue().value.intValue;
}

DMACReturnCode DMACMock_SetupTransfer(DMACRegisters*      dmac_dev,
                                      DMACTransferConfig* transfer_config)
{
    mock_c()->actualCall("DMAC_SetupTransfer")
    ->withPointerParameters("dmac_dev", dmac_dev)
    ->withParameterOfType("DMACTransferConfig*", "transfer_config", transfer_config);
    return (DMACReturnCode) mock_c()->returnValue().value.intValue;
}

DMACReturnCode DMACMock_EnableChannel(DMACRegisters* dmac_dev,
                                      DMACChannel    channel)
{
    mock_c()->actualCall("DMAC_EnableChannel")
    ->withPointerParameters("dmac_dev", dmac_dev)
    ->withUnsignedIntParameters("channel", channel);
    return (DMACReturnCode) mock_c()->returnValue().value.intValue;
}
