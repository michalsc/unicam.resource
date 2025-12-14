/*
    Copyright © 2024 Michal Schulz <michal.schulz@gmx.de>
    https://github.com/michalsc

    This Source Code Form is subject to the terms of the
    Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <exec/types.h>
#include <common/compiler.h>

#include "unicam.h"
#include "mbox.h"

void L_UnicamSetConfig(REGARG(ULONG cfg, "d0"), REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    UnicamBase->u_Integer = (cfg | UNICAMF_INTEGER) != 0;
    UnicamBase->u_Smooth = (cfg | UNICAMF_SMOOTHING) != 0;
    UnicamBase->u_Scaler = (cfg & UNICAMF_SCALER) >> UNICAMB_SCALER;
    UnicamBase->u_Phase = (cfg & UNICAMF_PHASE) >> UNICAMB_PHASE;
}
