/*
    Copyright © 2021 Michal Schulz <michal.schulz@gmx.de>
    https://github.com/michalsc

    This Source Code Form is subject to the terms of the
    Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <exec/types.h>
#include <exec/execbase.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/devicetree.h>
#include <proto/mailbox.h>

#include <libraries/configregs.h>
#include <libraries/configvars.h>

#include <stdint.h>

#include "unicam.h"

/* status register flags */

#define MBOX_TX_FULL (1UL << 31)
#define MBOX_RX_EMPTY (1UL << 30)
#define MBOX_CHANMASK 0xF

/* VideoCore tags used. */

#define VCTAG_GET_ARM_MEMORY     0x00010005
#define VCTAG_GET_CLOCK_RATE     0x00030002

ULONG enable_unicam_domain(struct UnicamBase *UnicamBase)
{
    ULONG FBReq[8];
    APTR MailboxBase = UnicamBase->u_MailboxBase;

    FBReq[0] = 4 * 8;       // Length
    FBReq[1] = 0;           // Request
    FBReq[2] = 0x00038030;  // SetClockRate
    FBReq[3] = 8;
    FBReq[4] = 0;
    FBReq[5] = 14;          // unicam1
    FBReq[6] = 1;
    FBReq[7] = 0;

    MB_RawCommand(FBReq);

    return FBReq[6];
}

struct Size get_display_size(struct UnicamBase *UnicamBase)
{
    ULONG FBReq[8];
    APTR MailboxBase = UnicamBase->u_MailboxBase;
    struct Size dimension;

    FBReq[0] = 4 * 8;
    FBReq[1] = 0;
    FBReq[2] = 0x40003;
    FBReq[3] = 8;
    FBReq[4] = 0;
    FBReq[5] = 0;
    FBReq[6] = 0;
    FBReq[7] = 0;

    MB_RawCommand(FBReq);

    dimension.width = FBReq[5];
    dimension.height = FBReq[6];

    return dimension;
}
