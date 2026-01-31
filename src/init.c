/*
    Copyright © 2023-2024 Michal Schulz <michal.schulz@gmx.de>
    https://github.com/michalsc

    This Source Code Form is subject to the terms of the
    Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed
    with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <exec/types.h>
#include <exec/execbase.h>

#include <proto/exec.h>
#include <proto/expansion.h>
#include <proto/devicetree.h>
#include <common/compiler.h>

#include <inline/unicam.h>

#include "unicam.h"
#include "smoothing.h"
#include "mbox.h"
#include "videocore.h"

extern const char deviceName[];
extern const char deviceIdString[];

CONST_APTR GetPropValueRecursive(APTR key, CONST_STRPTR property, APTR DeviceTreeBase);
CONST_STRPTR FindToken(CONST_STRPTR string, CONST_STRPTR token);
int _strcmp(const char *s1, const char *s2);

APTR Init(REGARG(struct ExecBase *SysBase, "a6"))
{
    struct DeviceTreeBase *DeviceTreeBase = NULL;
    struct MailboxBase *MailboxBase = NULL;
    struct ExpansionBase *ExpansionBase = NULL;
    struct UnicamBase *UnicamBase = NULL;
    struct CurrentBinding binding;

    bug("[unicam] Init\n");

    DeviceTreeBase = OpenResource("devicetree.resource");
    MailboxBase = OpenResource("mailbox.resource");

    if (DeviceTreeBase != NULL && MailboxBase != NULL)
    {
        APTR base_pointer = NULL;
    
        ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library", 0);
        GetCurrentBinding(&binding, sizeof(binding));

        base_pointer = AllocMem(BASE_NEG_SIZE + BASE_POS_SIZE, MEMF_PUBLIC | MEMF_CLEAR);

        if (base_pointer != NULL)
        {
            BYTE start_on_boot = 0;
            APTR key;
            ULONG relFuncTable[UNICAM_FUNC_COUNT + 1];

            relFuncTable[0] = (ULONG)&L_UnicamStart;
            relFuncTable[1] = (ULONG)&L_UnicamStop;
            relFuncTable[2] = (ULONG)&L_UnicamGetFramebuffer;
            relFuncTable[3] = (ULONG)&L_UnicamGetFramebufferSize;
            relFuncTable[4] = (ULONG)&L_UnicamGetCropSize;
            relFuncTable[5] = (ULONG)&L_UnicamGetCropOffset;
            relFuncTable[6] = (ULONG)&L_UnicamGetKernel;
            relFuncTable[7] = (ULONG)&L_UnicamGetConfig;
            relFuncTable[8] = (ULONG)&L_UnicamGetSize;
            relFuncTable[9] = (ULONG)&L_UnicamGetMode;
            relFuncTable[10] = (ULONG)&L_UnicamConstructDL;
            relFuncTable[11] = (ULONG)&L_UnicamSetConfig;
            relFuncTable[12] = (ULONG)&L_UnicamSetCropSize;
            relFuncTable[13] = (ULONG)&L_UnicamSetCropOffset;
            relFuncTable[14] = (ULONG)&L_UnicamSetKernel;
            relFuncTable[15] = (ULONG)&L_UnicamGetAspect;
            relFuncTable[16] = (ULONG)&L_UnicamSetAspect;
            relFuncTable[17] = (ULONG)-1;

            UnicamBase = (struct UnicamBase *)((UBYTE *)base_pointer + BASE_NEG_SIZE);
            UnicamBase->u_SysBase = SysBase;
            UnicamBase->u_MailboxBase = MailboxBase;

            MakeFunctions(UnicamBase, relFuncTable, 0);

            UnicamBase->u_ConfigDev = binding.cb_ConfigDev;
            UnicamBase->u_ROMBase = binding.cb_ConfigDev->cd_BoardAddr;
            UnicamBase->u_Node.lib_Node.ln_Type = NT_RESOURCE;
            UnicamBase->u_Node.lib_Node.ln_Pri = UNICAM_PRIORITY;
            UnicamBase->u_Node.lib_Node.ln_Name = (STRPTR)deviceName;
            UnicamBase->u_Node.lib_NegSize = BASE_NEG_SIZE;
            UnicamBase->u_Node.lib_PosSize = BASE_POS_SIZE;
            UnicamBase->u_Node.lib_Version = UNICAM_VERSION;
            UnicamBase->u_Node.lib_Revision = UNICAM_REVISION;
            UnicamBase->u_Node.lib_IdString = (STRPTR)deviceIdString;

            UnicamBase->u_IsVC6 = 0;
            UnicamBase->u_Offset.x = 0;
            UnicamBase->u_Offset.y = 0;
            UnicamBase->u_FullSize.width = 0;
            UnicamBase->u_FullSize.height = 0;
            UnicamBase->u_Mode = 0;
            UnicamBase->u_BPP = 0;
            UnicamBase->u_Size = UnicamBase->u_FullSize;
            UnicamBase->u_Phase = 64;
            UnicamBase->u_Scaler = 3;
            UnicamBase->u_Smooth = 0;
            UnicamBase->u_Integer = 0;
            UnicamBase->u_KernelB = 250;
            UnicamBase->u_KernelC = 750;
            UnicamBase->u_Aspect = 1000;

            SumLibrary((struct Library*)UnicamBase);

            key = DT_OpenKey("/emu68/unicam");

            const char *cmdline = DT_GetPropValue(DT_FindProperty(DT_OpenKey("/chosen"), "bootargs"));
            const char *cmd;

            if (_strcmp(DT_GetPropValue(DT_FindProperty(key, "status")), "okay") == 0)
            {
                start_on_boot = 1;
                UnicamBase->u_StartOnBoot = TRUE;
                bug("[unicam] Starting HDMI passthrough on boot\n");
            }

            if (DT_FindProperty(key, "integer-scaling"))
            {
                UnicamBase->u_Integer = 1;
                bug("[unicam] Use integer scaling\n");
            }

            ULONG kernel = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "kernel"));
            UnicamBase->u_KernelB = kernel >> 16;
            UnicamBase->u_KernelC = kernel & 0xffff;
            UnicamBase->u_Aspect = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "aspect-ratio"));

            if (DT_FindProperty(key, "smoothing"))
            {
                UnicamBase->u_Smooth = 1;
                bug("[unicam] Enable smoothing kernel. B=%ld, C=%ld\n", UnicamBase->u_KernelB, UnicamBase->u_KernelC);
            }

            ULONG scaler = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "scaler"));
            UnicamBase->u_Scaler = scaler >> 16;
            UnicamBase->u_Phase = scaler & 0xffff;

            bug("[unicam] Scaler=%ld, Phase=%ld\n", UnicamBase->u_Scaler, UnicamBase->u_Phase);

            ULONG full_size = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "full-size"));
            UnicamBase->u_FullSize.width = full_size >> 16;
            UnicamBase->u_FullSize.height = full_size & 0xffff;

            ULONG size = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "size"));
            UnicamBase->u_Size.width = size >> 16;
            UnicamBase->u_Size.height = size & 0xffff;

            ULONG offset = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "offset"));
            UnicamBase->u_Offset.x = offset >> 16;
            UnicamBase->u_Offset.y = offset & 0xffff;

            UnicamBase->u_Mode = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "mode"));
            UnicamBase->u_BPP = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "bpp"));
            UnicamBase->u_Type = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "type"));
            UnicamBase->u_PixelOrder = *(ULONG *)DT_GetPropValue(DT_FindProperty(key, "pixel-order"));

            bug("[unicam] Displayed size: %ld x %ld\n", UnicamBase->u_Size.width, UnicamBase->u_Size.height);
            bug("[unicam] Display offset: %ld, %ld\n", UnicamBase->u_Offset.x, UnicamBase->u_Offset.y);
            bug("[unicam] Type: %s\n", (ULONG)(UnicamBase->u_Type == 0 ? "FrameThrower" : "C790"));

            key = DT_OpenKey("/gpu");
            if (key)
            {
                const char *comp = DT_GetPropValue(DT_FindProperty(key, "compatible"));

                if (comp != NULL)
                {
                    if (_strcmp("brcm,bcm2711-vc5", comp) == 0)
                    {
                        UnicamBase->u_IsVC6 = 1;
                        bug("[unicam] VC6 detected\n");
                    }
                }
            }

            /* Open /soc key and learn about VC4 to CPU mapping. Use it to adjust the addresses obtained above */
            key = DT_OpenKey("/soc");
            if (key)
            {
                int size_cells = 1;
                int address_cells = 1;
                int cpu_address_cells = 1;

                const ULONG * siz = GetPropValueRecursive(key, "#size-cells", DeviceTreeBase);
                const ULONG * addr = GetPropValueRecursive(key, "#address-cells", DeviceTreeBase);
                const ULONG * cpu_addr = DT_GetPropValue(DT_FindProperty(DT_OpenKey("/"), "#address-cells"));
            
                if (siz != NULL)
                    size_cells = *siz;
                
                if (addr != NULL)
                    address_cells = *addr;

                if (cpu_addr != NULL)
                    cpu_address_cells = *cpu_addr;

                const ULONG *reg = DT_GetPropValue(DT_FindProperty(key, "ranges"));

                ULONG phys_vc4 = reg[address_cells - 1];
                ULONG phys_cpu = reg[address_cells + cpu_address_cells - 1];

                UnicamBase->u_PeriphBase = (APTR)phys_cpu;

                DT_CloseKey(key);
            }

            bug("[unicam] Periph base: %08lx\n", (ULONG)UnicamBase->u_PeriphBase);

            UnicamBase->u_ReceiveBuffer = NULL;
            /* Get location of receive buffer. If it does not exist, reserve it now */
            key = DT_OpenKey("/emu68");
            if (key)
            {
                const ULONG *reg = DT_GetPropValue(DT_FindProperty(key, "unicam-mem"));

                if (reg != NULL)
                {
                    UnicamBase->u_ReceiveBuffer = (APTR)reg[0];
                    UnicamBase->u_ReceiveBufferSize = reg[1];
                }

                DT_CloseKey(key);
            }

            if (UnicamBase->u_ReceiveBuffer == NULL)
            {
                const ULONG size = sizeof(ULONG) * UnicamBase->u_FullSize.width * UnicamBase->u_FullSize.height + 512;
                
                UnicamBase->u_ReceiveBuffer = AllocMem(size, MEMF_FAST);
                UnicamBase->u_ReceiveBufferSize = size;

                UnicamBase->u_ReceiveBuffer = (APTR)(((ULONG)UnicamBase->u_ReceiveBuffer + 63) & ~63);
            }

            bug("[unicam] Receive buffer: %08lx, size: %08lx\n", (ULONG)UnicamBase->u_ReceiveBuffer, UnicamBase->u_ReceiveBufferSize);

            AddResource(UnicamBase);

            if (start_on_boot)
            {
                LONG kernel_b = (UnicamBase->u_KernelB * 256) / 1000;
                LONG kernel_c = (UnicamBase->u_KernelC * 256) / 1000;

                UnicamBase->u_UnicamKernel = 0xfc0;
                ULONG *dlistPtr = (ULONG *)((ULONG)UnicamBase->u_PeriphBase + 
                    (UnicamBase->u_IsVC6 ? 0x00404000 : 0x00402000));

                bug("[unicam] DisplayList at %08lx\n", (ULONG)dlistPtr);
                
                if (UnicamBase->u_Type == TYPE_C790) {
                    init_c790_ic(UnicamBase);
                }

                UnicamStart(UnicamBase->u_ReceiveBuffer, 1, 
                    UnicamBase->u_Mode, 
                    UnicamBase->u_FullSize.width, UnicamBase->u_FullSize.height,
                    UnicamBase->u_BPP);

                while (UnicamBase->u_DisplaySize.width == 0 || UnicamBase->u_DisplaySize.height == 0)
                {
                    UnicamBase->u_DisplaySize = get_display_size(UnicamBase);
                }

                if (UnicamBase->u_Smooth)
                {
                    compute_scaling_kernel(&dlistPtr[UnicamBase->u_UnicamKernel], kernel_b, kernel_c);
                }
                else
                {
                    compute_nearest_neighbour_kernel(&dlistPtr[UnicamBase->u_UnicamKernel]);
                }

                if (UnicamBase->u_IsVC6)
                {
                    VC6_ConstructUnicamDL(UnicamBase, UnicamBase->u_UnicamKernel);
                }
                else
                {
                    VC4_ConstructUnicamDL(UnicamBase, UnicamBase->u_UnicamKernel);
                }

                *(volatile uint32_t *)(UnicamBase->u_PeriphBase + 0x00400024) = LE32(UnicamBase->u_UnicamDL);
            }

            binding.cb_ConfigDev->cd_Flags &= ~CDF_CONFIGME;
            binding.cb_ConfigDev->cd_Driver = UnicamBase;
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return  UnicamBase;
}

static void putch(REGARG(UBYTE data, "d0"), REGARG(APTR ignore, "a3"))
{
    (void)ignore;
    *(UBYTE*)0xdeadbeef = data;
}

void kprintf(REGARG(const char * msg, "a0"), REGARG(void * args, "a1")) 
{
    struct ExecBase *SysBase = *(struct ExecBase **)4UL;
    RawDoFmt(msg, args, (APTR)putch, NULL);
}

/*
    Some properties, like e.g. #size-cells, are not always available in a key, but in that case the properties
    should be searched for in the parent. The process repeats recursively until either root key is found
    or the property is found, whichever occurs first
*/
CONST_APTR GetPropValueRecursive(APTR key, CONST_STRPTR property, APTR DeviceTreeBase)
{
    do {
        /* Find the property first */
        APTR prop = DT_FindProperty(key, property);

        if (prop)
        {
            /* If property is found, get its value and exit */
            return DT_GetPropValue(prop);
        }
        
        /* Property was not found, go to the parent and repeat */
        key = DT_GetParent(key);
    } while (key);

    return NULL;
}


CONST_STRPTR FindToken(CONST_STRPTR string, CONST_STRPTR token)
{
    CONST_STRPTR ret = NULL;

    if (string)
    {
        do {
            while (*string == ' ' || *string == '\t') {
                string++;
            }

            if (*string == 0)
                break;

            for (int i=0; token[i] != 0; i++)
            {
                if (string[i] != token[i])
                {
                    break;
                }

                if (token[i] == '=') {
                    ret = string;
                    break;
                }

                if (string[i+1] == 0 || string[i+1] == ' ' || string[i+1] == '\t') {
                    ret = string;
                    break;
                }
            }

            if (ret)
                break;

            while(*string != 0) {
                if (*string != ' ' && *string != '\t')
                    string++;
                else break;
            }

        } while(!ret && *string != 0);
    }
    return ret;
}

int _strcmp(const char *s1, const char *s2)
{
    while (*s1 == *s2++)
        if (*s1++ == '\0')
            return (0);
    return (*(const unsigned char *)s1 - *(const unsigned char *)(s2 - 1));
}
