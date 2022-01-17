/*
 * Copyright (c) TheDevHuts, 2022.
 * All rights reserved. Permission to use, copy, modify, distribute in any
 * form or by any means or store in any database or retrieval system any
 * parts of this copyrighted work is forbidden.
 * Contact TheDevHuts (contact@thedevhuts.ai) for licensing agreement
 * opportunities.
 *
 * Contributor: Florent Remis
 *
 */

#ifndef __DMAC_MOCK_H
#define __DMAC_MOCK_H

#include "DMAC.h"

#ifdef __cplusplus
extern "C" {
#endif

DMACReturnCode DMACMock_SetupChannel(DMACRegisters*      dmac_dev,
                                     DMACChannelConfig*  channel_config,
                                     bool                disable_abort_int,
                                     bool                disable_error_int,
                                     bool                disable_terminal_count_int,
                                     DMACChannelCallback callback);
DMACReturnCode DMACMock_SetupTransfer(DMACRegisters*      dmac_dev,
                                      DMACTransferConfig* transfer_config);
DMACReturnCode DMACMock_EnableChannel(DMACRegisters* dmac_dev,
                                      DMACChannel    channel);
DMACReturnCode DMACMock_EnableInterrupt(DMACRegisters* dmac_dev,
                                        uint32_t       priority);
DMACReturnCode DMACMock_DisableInterrupt(DMACRegisters* dmac_dev);

#ifdef __cplusplus
}
#endif

#endif // __DMAC_MOCK_H
