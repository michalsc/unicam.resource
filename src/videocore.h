#ifndef _VIDEOCORE_H
#define _VIDEOCORE_H

#include "unicam.h"
#include <stdint.h>


#define CONTROL_FORMAT(n)       (n & 0xf)
#define CONTROL_END             (1<<31)
#define CONTROL_VALID           (1<<30)
#define CONTROL_WORDS(n)        (((n) & 0x3f) << 24)
#define CONTROL0_FIXED_ALPHA    (1<<19)
#define CONTROL0_HFLIP          (1<<16)
#define CONTROL0_VFLIP          (1<<15)
#define CONTROL_PIXEL_ORDER(n)  ((n & 3) << 13)
#define CONTROL_SCL1(scl)       ((scl) << 8)
#define CONTROL_SCL0(scl)       ((scl) << 5)
#define CONTROL_UNITY           (1<<4)

#define POS0_X(n) (n & 0xfff)
#define POS0_Y(n) ((n & 0xfff) << 12)
#define POS0_ALPHA(n) ((n & 0xff) << 24)

#define POS1_W(n) (n & 0xffff)
#define POS1_H(n) ((n & 0xffff) << 16)

#define POS2_W(n) (n & 0xffff)
#define POS2_H(n) ((n & 0xffff) << 16)

#define SCALER_POS2_ALPHA_MODE_MASK             0xc0000000
#define SCALER_POS2_ALPHA_MODE_SHIFT            30
#define SCALER_POS2_ALPHA_MODE_PIPELINE         0
#define SCALER_POS2_ALPHA_MODE_FIXED            1
#define SCALER_POS2_ALPHA_MODE_FIXED_NONZERO    2
#define SCALER_POS2_ALPHA_MODE_FIXED_OVER_0x07  3
#define SCALER_POS2_ALPHA_PREMULT               (1 << 29)
#define SCALER_POS2_ALPHA_MIX                   (1 << 28)

enum hvs_pixel_format {
    /* 8bpp */
    HVS_PIXEL_FORMAT_RGB332 = 0,
    /* 16bpp */
    HVS_PIXEL_FORMAT_RGBA4444 = 1,
    HVS_PIXEL_FORMAT_RGB555 = 2,
    HVS_PIXEL_FORMAT_RGBA5551 = 3,
    HVS_PIXEL_FORMAT_RGB565 = 4,
    /* 24bpp */
    HVS_PIXEL_FORMAT_RGB888 = 5,
    HVS_PIXEL_FORMAT_RGBA6666 = 6,
    /* 32bpp */
    HVS_PIXEL_FORMAT_RGBA8888 = 7,

    HVS_PIXEL_FORMAT_YCBCR_YUV420_3PLANE = 8,
    HVS_PIXEL_FORMAT_YCBCR_YUV420_2PLANE = 9,
    HVS_PIXEL_FORMAT_YCBCR_YUV422_3PLANE = 10,
    HVS_PIXEL_FORMAT_YCBCR_YUV422_2PLANE = 11,
    HVS_PIXEL_FORMAT_H264 = 12,
    HVS_PIXEL_FORMAT_PALETTE = 13,
    HVS_PIXEL_FORMAT_YUV444_RGB = 14,
    HVS_PIXEL_FORMAT_AYUV444_RGB = 15,
    HVS_PIXEL_FORMAT_RGBA1010102 = 16,
    HVS_PIXEL_FORMAT_YCBCR_10BIT = 17,
};

enum palette_type {
    PALETTE_NONE = 0, 
    PALETTE_1BPP = 1,
    PALETTE_2BPP = 2,
    PALETTE_4BPP = 3,
    PALETTE_8BPP = 4,
};

#define HVS_PIXEL_ORDER_RGBA                    0
#define HVS_PIXEL_ORDER_BGRA                    1
#define HVS_PIXEL_ORDER_ARGB                    2
#define HVS_PIXEL_ORDER_ABGR                    3

#define HVS_PIXEL_ORDER_XBRG                    0
#define HVS_PIXEL_ORDER_XRBG                    1
#define HVS_PIXEL_ORDER_XRGB                    2
#define HVS_PIXEL_ORDER_XBGR                    3

#define HVS_PIXEL_ORDER_XYCBCR			        0
#define HVS_PIXEL_ORDER_XYCRCB			        1
#define HVS_PIXEL_ORDER_YXCBCR			        2
#define HVS_PIXEL_ORDER_YXCRCB			        3

#define SCALER_CTL0_SCL_H_PPF_V_PPF             0
#define SCALER_CTL0_SCL_H_TPZ_V_PPF             1
#define SCALER_CTL0_SCL_H_PPF_V_TPZ             2
#define SCALER_CTL0_SCL_H_TPZ_V_TPZ             3
#define SCALER_CTL0_SCL_H_PPF_V_NONE            4
#define SCALER_CTL0_SCL_H_NONE_V_PPF            5
#define SCALER_CTL0_SCL_H_NONE_V_TPZ            6
#define SCALER_CTL0_SCL_H_TPZ_V_NONE            7



#define VC6_CONTROL_FORMAT(n)       (n & 0x1f)
#define VC6_CONTROL_END             (1<<31)
#define VC6_CONTROL_VALID           (1<<30)
#define VC6_CONTROL_WORDS(n)        (((n) & 0x3f) << 24)
#define VC6_CONTROL0_FIXED_ALPHA    (1<<19)
#define VC6_CONTROL0_HFLIP          (1<<31)
#define VC6_CONTROL0_VFLIP          (1<<15)
#define VC6_CONTROL_PIXEL_ORDER(n)  ((n & 3) << 13)
#define VC6_CONTROL_SCL1(scl)       ((scl) << 8)
#define VC6_CONTROL_SCL0(scl)       ((scl) << 5)
#define VC6_CONTROL_UNITY           (1<<15)
#define VC6_CONTROL_ALPHA_EXPAND    (1<<12)
#define VC6_CONTROL_RGB_EXPAND      (1<<11)

#define VC6_POS0_X(n) (n & 0x2fff)
#define VC6_POS0_Y(n) ((n & 0x2fff) << 16)

#define VC6_POS1_W(n) (n & 0xffff)
#define VC6_POS1_H(n) ((n & 0xffff) << 16)

#define VC6_POS2_W(n) (n & 0xffff)
#define VC6_POS2_H(n) ((n & 0xffff) << 16)

#define VC6_SCALER_POS2_ALPHA_MODE_MASK             0xc0000000
#define VC6_SCALER_POS2_ALPHA_MODE_SHIFT            30
#define VC6_SCALER_POS2_ALPHA_MODE_PIPELINE         0
#define VC6_SCALER_POS2_ALPHA_MODE_FIXED            1
#define VC6_SCALER_POS2_ALPHA_MODE_FIXED_NONZERO    2
#define VC6_SCALER_POS2_ALPHA_MODE_FIXED_OVER_0x07  3
#define VC6_SCALER_POS2_ALPHA_PREMULT               (1 << 29)
#define VC6_SCALER_POS2_ALPHA_MIX                   (1 << 28)
#define VC6_SCALER_POS2_ALPHA(n)                    (((n) << 4) & 0xfff0)

#define VC6_SCALER_POS2_HEIGHT_MASK                 0x3fff0000
#define VC6_SCALER_POS2_HEIGHT_SHIFT                16

#define VC6_SCALER_POS2_WIDTH_MASK                  0x00003fff
#define VC6_SCALER_POS2_WIDTH_SHIFT                 0


void VC4_ConstructUnicamDL(struct UnicamBase *UnicamBase, ULONG kernel);
void VC6_ConstructUnicamDL(struct UnicamBase *UnicamBase, ULONG kernel);

#endif /* _VIDEOCORE_H */
