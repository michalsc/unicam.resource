/*
    Copyright © 2023-2024 Michal Schulz <michal.schulz@gmx.de>
    https://github.com/michalsc

    This Source Code Form is subject to the terms of the
    Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <exec/types.h>
#include <exec/resident.h>
#include <exec/io.h>
#include <exec/devices.h>
#include <exec/errors.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <libraries/configregs.h>
#include <libraries/configvars.h>
#include <common/compiler.h>

#include <proto/devicetree.h>
#include <proto/exec.h>
#include <stdint.h>

#include "unicam.h"

extern UBYTE rom_end;
extern UBYTE rom_start;
extern const char deviceName[];
extern const char deviceIdString[];
void Init();

const struct Resident RomTag __attribute__((used)) = {
    RTC_MATCHWORD,
    (struct Resident *)&RomTag,
    (APTR)&rom_end,
    RTF_COLDSTART,
    UNICAM_VERSION,
    NT_RESOURCE,
    UNICAM_PRIORITY,
    (char *)((intptr_t)&deviceName),
    (char *)((intptr_t)&deviceIdString),
    Init,
};

const char deviceName[] = "unicam.resource";
const char deviceIdString[] = VERSION_STRING;
