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

ULONG L_UnicamGetKernel(REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    ULONG kernel = UnicamBase->u_KernelB;
    kernel = (kernel << 16) | UnicamBase->u_KernelC;
    
    return kernel;
}

void L_UnicamSetKernel(REGARG(UWORD b, "d0"), REGARG(UWORD c, "d1"), REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    UnicamBase->u_KernelB = b;
    UnicamBase->u_KernelC = c;
}

void L_UnicamSetAspect(REGARG(UWORD aspect, "d0"), REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    UnicamBase->u_Aspect = aspect;
}

UWORD L_UnicamGetAspect(REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    return UnicamBase->u_Aspect;
}
