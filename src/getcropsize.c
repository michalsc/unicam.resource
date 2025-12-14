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

ULONG L_UnicamGetCropSize(REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    ULONG size = UnicamBase->u_Size.width;
    size = (size << 16) | UnicamBase->u_Size.height;
    return size;
}

ULONG L_UnicamGetCropOffset(REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    ULONG offset = UnicamBase->u_Offset.x;
    offset = (offset << 16) | UnicamBase->u_Offset.y;
    return offset;
}


void L_UnicamSetCropSize(REGARG(UWORD width, "d0"), REGARG(UWORD height, "d1"), REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    if (width <= UnicamBase->u_FullSize.width && height <= UnicamBase->u_FullSize.height) {
        UnicamBase->u_Size.width = width;
        UnicamBase->u_Size.height = height;
    }
}

void L_UnicamSetCropOffset(REGARG(ULONG x, "d0"), REGARG(ULONG y, "d1"), REGARG(struct UnicamBase * UnicamBase, "a6"))
{
    UnicamBase->u_Offset.x = x;
    UnicamBase->u_Offset.y = y;
}
