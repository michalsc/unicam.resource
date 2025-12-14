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

#include <libraries/configregs.h>
#include <libraries/configvars.h>

#include <stdint.h>

#include "unicam.h"
#include "videocore.h"
#include "smoothing.h"

/* Unicam DisplayList */
void VC4_ConstructUnicamDL(struct UnicamBase *UnicamBase, ULONG kernel)
{
    int unity = 0;
    ULONG scale_x = 0;
    ULONG scale_y = 0;
    ULONG scale = 0;
    ULONG recip_x = 0;
    ULONG recip_y = 0;
    ULONG calc_width = 0;
    ULONG calc_height = 0;
    ULONG offset_x = 0;
    ULONG offset_y = 0;

    ULONG cnt = 0x300; // Initial pointer to UnicamDL

    volatile ULONG *displist = (ULONG *)((ULONG)UnicamBase->u_PeriphBase + 0x00402000);

    if (UnicamBase->u_Size.width == UnicamBase->u_DisplaySize.width &&
        UnicamBase->u_Size.height == UnicamBase->u_DisplaySize.height && UnicamBase->u_Aspect == 1000)
    {
        unity = 1;
    }
    else
    {
        scale_x = 0x10000 * ((UnicamBase->u_Size.width * UnicamBase->u_Aspect) / 1000) / UnicamBase->u_DisplaySize.width;
        scale_y = 0x10000 * UnicamBase->u_Size.height / UnicamBase->u_DisplaySize.height;

        recip_x = 0xffffffff / scale_x;
        recip_y = 0xffffffff / scale_y;

        // Select larger scaling factor from X and Y, but it need to fit
        if (((0x10000 * UnicamBase->u_Size.height) / scale_x) > UnicamBase->u_DisplaySize.height) {
            scale = scale_y;
        }
        else {
            scale = scale_x;
        }

        if (UnicamBase->u_Integer)
        {
            scale = 0x10000 / (ULONG)(0x10000 / scale);
        }

        scale_x = scale * 1000 / UnicamBase->u_Aspect;
        scale_y = scale;

        calc_width = (0x10000 * UnicamBase->u_Size.width) / scale_x;
        calc_height = (0x10000 * UnicamBase->u_Size.height) / scale_y;

        offset_x = (UnicamBase->u_DisplaySize.width - calc_width) >> 1;
        offset_y = (UnicamBase->u_DisplaySize.height - calc_height) >> 1;
    }

    ULONG startAddress = (ULONG)UnicamBase->u_ReceiveBuffer;
    startAddress += UnicamBase->u_Offset.x * (UnicamBase->u_BPP / 8);
    startAddress += UnicamBase->u_Offset.y * UnicamBase->u_FullSize.width * (UnicamBase->u_BPP / 8);

    if (unity)
    {
        /* Unity scaling is simple, reserve less space for display list */
        cnt -= 16;

        UnicamBase->u_UnicamDL = cnt;

        /* Set control reg */
        ULONG control = 
            CONTROL_VALID
            | CONTROL_WORDS(7)
            | CONTROL_UNITY;
        
        if (UnicamBase->u_PixelOrder == 0)
            control |= CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XRGB);
        else
            control |= CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XBGR);

        if (UnicamBase->u_BPP == 16)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565);
        else if (UnicamBase->u_BPP == 24)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB888);

        wr32le(&displist[cnt++], control);

        /* Center it on the screen */
        wr32le(&displist[cnt++], POS0_X(offset_x) | POS0_Y(offset_y) | POS0_ALPHA(0xff));
        wr32le(&displist[cnt++], POS2_H(UnicamBase->u_Size.height) | POS2_W(UnicamBase->u_Size.width) | (1 << 30));
        wr32le(&displist[cnt++], 0xdeadbeef);

        /* Set address */
        wr32le(&displist[cnt++], 0xc0000000 | (ULONG)startAddress);
        wr32le(&displist[cnt++], 0xdeadbeef);

        /* Pitch is full width, always */
        wr32le(&displist[cnt++], UnicamBase->u_FullSize.width * (UnicamBase->u_BPP / 8));

        /* Done */
        wr32le(&displist[cnt++], 0x80000000);
    }
    else
    {
        cnt -= 32;
        
        UnicamBase->u_UnicamDL = cnt;

        /* Set control reg */
        ULONG control = 
            CONTROL_VALID
            | CONTROL_WORDS(16)
            | 0x01800;
        
        if (UnicamBase->u_PixelOrder == 0)
            control |= CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XRGB);
        else
            control |= CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XBGR);

        if (UnicamBase->u_BPP == 16)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565);
        else if (UnicamBase->u_BPP == 24)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB888);

        wr32le(&displist[cnt++], control);

        /* Center plane on the screen */
        wr32le(&displist[cnt++], POS0_X(offset_x) | POS0_Y(offset_y) | POS0_ALPHA(0xff));
        wr32le(&displist[cnt++], POS1_H(calc_height) | POS1_W(calc_width));
        wr32le(&displist[cnt++], POS2_H(UnicamBase->u_Size.height) | POS2_W(UnicamBase->u_Size.width) |
                               (SCALER_POS2_ALPHA_MODE_FIXED << SCALER_POS2_ALPHA_MODE_SHIFT));
        wr32le(&displist[cnt++], 0xdeadbeef); // Scratch written by HVS

        /* Set address and pitch */
        wr32le(&displist[cnt++], 0xc0000000 | startAddress);
        wr32le(&displist[cnt++], 0xdeadbeef);

        /* Pitch is full width, always */
        wr32le(&displist[cnt++], UnicamBase->u_FullSize.width * (UnicamBase->u_BPP / 8));

        /* LMB address */
        wr32le(&displist[cnt++], 0);

        /* Set PPF Scaler */
        wr32le(&displist[cnt++], (scale_x << 8) | (UnicamBase->u_Scaler << 30) | UnicamBase->u_Phase);
        wr32le(&displist[cnt++], (scale_y << 8) | (UnicamBase->u_Scaler << 30) | UnicamBase->u_Phase);
        wr32le(&displist[cnt++], 0); // Scratch written by HVS

        wr32le(&displist[cnt++], kernel);
        wr32le(&displist[cnt++], kernel);
        wr32le(&displist[cnt++], kernel);
        wr32le(&displist[cnt++], kernel);

        /* Done */
        wr32le(&displist[cnt++], 0x80000000);
    }
}


void VC6_ConstructUnicamDL(struct UnicamBase *UnicamBase, ULONG kernel)
{
    int unity = 0;
    ULONG scale_x = 0;
    ULONG scale_y = 0;
    ULONG scale = 0;
    ULONG recip_x = 0;
    ULONG recip_y = 0;
    ULONG calc_width = 0;
    ULONG calc_height = 0;
    ULONG offset_x = 0;
    ULONG offset_y = 0;

    ULONG cnt = 0x300; // Initial pointer to UnicamDL

    volatile ULONG *displist = (ULONG *)((ULONG)UnicamBase->u_PeriphBase + 0x00404000);

    if (UnicamBase->u_Size.width == UnicamBase->u_DisplaySize.width &&
        UnicamBase->u_Size.height == UnicamBase->u_DisplaySize.height && UnicamBase->u_Aspect == 1000)
    {
        unity = 1;
    }
    else
    {
        scale_x = 0x10000 * ((UnicamBase->u_Size.width * UnicamBase->u_Aspect) / 1000) / UnicamBase->u_DisplaySize.width;
        scale_y = 0x10000 * UnicamBase->u_Size.height / UnicamBase->u_DisplaySize.height;

        recip_x = 0xffffffff / scale_x;
        recip_y = 0xffffffff / scale_y;

        // Select larger scaling factor from X and Y, but it need to fit
        if (((0x10000 * UnicamBase->u_Size.height) / scale_x) > UnicamBase->u_DisplaySize.height) {
            scale = scale_y;
        }
        else {
            scale = scale_x;
        }

        if (UnicamBase->u_Integer)
        {
            scale = 0x10000 / (ULONG)(0x10000 / scale);
        }

        scale_x = scale * 1000 / UnicamBase->u_Aspect;
        scale_y = scale;

        calc_width = (0x10000 * UnicamBase->u_Size.width) / scale_x;
        calc_height = (0x10000 * UnicamBase->u_Size.height) / scale_y;

        offset_x = (UnicamBase->u_DisplaySize.width - calc_width) >> 1;
        offset_y = (UnicamBase->u_DisplaySize.height - calc_height) >> 1;
    }

    ULONG startAddress = (ULONG)UnicamBase->u_ReceiveBuffer;
    startAddress += UnicamBase->u_Offset.x * (UnicamBase->u_BPP / 8);
    startAddress += UnicamBase->u_Offset.y * UnicamBase->u_FullSize.width * (UnicamBase->u_BPP / 8);

    if (unity)
    {
        /* Unity scaling is simple, reserve less space for display list */
        cnt -= 16;

        UnicamBase->u_UnicamDL = cnt;

        /* Set control reg */
        ULONG control = 
            VC6_CONTROL_VALID
            | VC6_CONTROL_WORDS(8)
            | VC6_CONTROL_UNITY
            | VC6_CONTROL_ALPHA_EXPAND
            | VC6_CONTROL_RGB_EXPAND;
        
        if (UnicamBase->u_PixelOrder == 0)
            control |= VC6_CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XRGB);
        else
            control |= VC6_CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XBGR);

        if (UnicamBase->u_BPP == 16)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565);
        else if (UnicamBase->u_BPP == 24)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB888);

        wr32le(&displist[cnt++], control);

        /* Center it on the screen */
        wr32le(&displist[cnt++], VC6_POS0_X(offset_x) | VC6_POS0_Y(offset_y));
        wr32le(&displist[cnt++], (VC6_SCALER_POS2_ALPHA_MODE_FIXED << VC6_SCALER_POS2_ALPHA_MODE_SHIFT) | VC6_SCALER_POS2_ALPHA(0xfff));
        wr32le(&displist[cnt++], VC6_POS2_H(UnicamBase->u_Size.height) | VC6_POS2_W(UnicamBase->u_Size.width));
        wr32le(&displist[cnt++], 0xdeadbeef);

        /* Set address */
        wr32le(&displist[cnt++], 0xc0000000 | startAddress);
        wr32le(&displist[cnt++], 0xdeadbeef);

        /* Pitch is full width, always */
        wr32le(&displist[cnt++], UnicamBase->u_FullSize.width * (UnicamBase->u_BPP / 8));

        /* Done */
        wr32le(&displist[cnt++], 0x80000000);
    }
    else
    {
        cnt -= 24;
        
        UnicamBase->u_UnicamDL = cnt;

        /* Set control reg */
        ULONG control = 
            VC6_CONTROL_VALID
            | VC6_CONTROL_WORDS(17)
            | VC6_CONTROL_ALPHA_EXPAND
            | VC6_CONTROL_RGB_EXPAND;
        
        if (UnicamBase->u_PixelOrder == 0)
            control |= VC6_CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XRGB);
        else
            control |= VC6_CONTROL_PIXEL_ORDER(HVS_PIXEL_ORDER_XBGR);

        if (UnicamBase->u_BPP == 16)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB565);
        else if (UnicamBase->u_BPP == 24)
            control |= CONTROL_FORMAT(HVS_PIXEL_FORMAT_RGB888);

        wr32le(&displist[cnt++], control);

        /* Center plane on the screen */
        wr32le(&displist[cnt++], VC6_POS0_X(offset_x) | VC6_POS0_Y(offset_y));
        wr32le(&displist[cnt++], (VC6_SCALER_POS2_ALPHA_MODE_FIXED << VC6_SCALER_POS2_ALPHA_MODE_SHIFT) | VC6_SCALER_POS2_ALPHA(0xfff));
        wr32le(&displist[cnt++], VC6_POS1_H(calc_height) | VC6_POS1_W(calc_width));
        wr32le(&displist[cnt++], VC6_POS2_H(UnicamBase->u_Size.height) | VC6_POS2_W(UnicamBase->u_Size.width));
        wr32le(&displist[cnt++], 0xdeadbeef); // Scratch written by HVS

        /* Set address and pitch */
        wr32le(&displist[cnt++], 0xc0000000 | startAddress);
        wr32le(&displist[cnt++], 0xdeadbeef);

        /* Pitch is full width, always */
        wr32le(&displist[cnt++], UnicamBase->u_FullSize.width * (UnicamBase->u_BPP / 8));

        /* LMB address */
        wr32le(&displist[cnt++], 0);

        /* Set PPF Scaler */
        wr32le(&displist[cnt++], (scale_x << 8) | ((ULONG)UnicamBase->u_Scaler << 30) | UnicamBase->u_Phase);
        wr32le(&displist[cnt++], (scale_y << 8) | ((ULONG)UnicamBase->u_Scaler << 30) | UnicamBase->u_Phase);
        wr32le(&displist[cnt++], 0); // Scratch written by HVS

        wr32le(&displist[cnt++], kernel);
        wr32le(&displist[cnt++], kernel);
        wr32le(&displist[cnt++], kernel);
        wr32le(&displist[cnt++], kernel);

        /* Done */
        wr32le(&displist[cnt++], 0x80000000);
    }
}
