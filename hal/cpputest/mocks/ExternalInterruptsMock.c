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
#include "ExternalInterruptsMock.h"

void ExternalInterrupts_EnableInterrupt(ExternalIRQSource source,
                                        uint32_t          priority)
{
    mock_c()->actualCall("ExternalInterrupts_EnableInterrupt")
    ->withUnsignedIntParameters("source", source)
    ->withUnsignedIntParameters("priority", priority);
}

void ExternalInterrupts_DisableInterrupt(ExternalIRQSource source)
{
    mock_c()->actualCall("ExternalInterrupts_DisableInterrupt")
    ->withUnsignedIntParameters("source", source);
}
