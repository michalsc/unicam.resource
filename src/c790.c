#include "unicam.h"

// GPIO alternative functions ordered by pin function
#define GPIO0_AF_I2C0_SDA GPIO_AF_0
#define GPIO1_AF_I2C0_SCL GPIO_AF_0

#define GPIO2_AF_I2C1_SDA GPIO_AF_0
#define GPIO3_AF_I2C1_SCL GPIO_AF_0

#define GPIO28_AF_I2C0_SDA GPIO_AF_0
#define GPIO29_AF_I2C0_SCL GPIO_AF_0

#define GPIO44_AF_I2C0_SDA GPIO_AF_1
#define GPIO44_AF_I2C1_SDA GPIO_AF_2
#define GPIO45_AF_I2C0_SCL GPIO_AF_1
#define GPIO45_AF_I2C1_SCL GPIO_AF_2

typedef enum tGpioAlternativeFunction {
    GPIO_AF_INPUT = 0b000,
    GPIO_AF_OUTPUT = 0b001,
    GPIO_AF_0 = 0b100,
    GPIO_AF_1 = 0b101,
    GPIO_AF_2 = 0b110,
    GPIO_AF_3 = 0b110,
    GPIO_AF_4 = 0b011,
    GPIO_AF_5 = 0b010,
} tGpioAlternativeFunction;

typedef enum tGpioPull {
    GPIO_PULL_OFF = 0b00, // no pull
    GPIO_PULL_PD = 0b01, // pull down
    GPIO_PULL_PU = 0b10, // pull up
} tGpioPull;

typedef struct tGpioRegs {
    ULONG GPFSEL[6];
    ULONG RESERVED0; // reserved
    ULONG GPSET[2];
    ULONG RESERVED1; // reserved
    ULONG GPCLR[2];
    ULONG RESERVED2;
    ULONG GPLEV[2];
    ULONG RESERVED3;
    ULONG GPEDS[2];
    ULONG RESERVED4;
    ULONG GPREN[2];
    ULONG RESERVED5;
    ULONG GPFEN[2];
    ULONG RESERVED6;
    ULONG GPHEN[2];
    ULONG RESERVED7;
    ULONG GPLEN[2];
    ULONG RESERVED8;
    ULONG GPAREN[2];
    ULONG RESERVED9;
    ULONG GPAFEN[2];
    ULONG RESERVED10;
    ULONG GPPUD;
    ULONG GPPUDCLK[2];
    ULONG RESERVED11;
    ULONG TEST;
} tGpioRegs;

static void busyWait(uint32_t ulCycles)
{
    volatile uint32_t i = ulCycles;
    while(i--) continue;
}

void gpioSetPull(volatile tGpioRegs *pGpio, UBYTE ubIndex, tGpioPull ePull)
{
    UBYTE ubRegIndex = ubIndex / 32;
    UBYTE ubRegShift = ubIndex % 32;

    // This is kinda like bitbanging of the pulse on the 2-lane bus
    wr32le(&pGpio->GPPUD, ePull);
    busyWait(150);
    wr32le(&pGpio->GPPUDCLK[ubRegIndex], 1 << ubRegShift);
    busyWait(150);
    wr32le(&pGpio->GPPUD, 0);
    wr32le(&pGpio->GPPUDCLK[ubRegIndex], 0);
}

void gpioSetAlternate(
    volatile tGpioRegs *pGpio,
    UBYTE ubIndex,
    tGpioAlternativeFunction eAlternativeFunction
)
{
    static const UBYTE ubBitsPerGpio = 3;
    UBYTE ubRegIndex = ubIndex / 10;
    UBYTE ubRegShift = (ubIndex % 10) * ubBitsPerGpio;
    uint32_t ulClearMask = ~(0b111 << ubRegShift);
    uint32_t ulWriteMask = eAlternativeFunction << ubRegShift;
    wr32le(
        &pGpio->GPFSEL[ubRegIndex],
        (rd32le(&pGpio->GPFSEL[ubRegIndex]) & ulClearMask) | ulWriteMask
    );
}

void gpioSetLevel(volatile tGpioRegs *pGpio, UBYTE ubIndex, UBYTE ubState)
{
    UBYTE ubRegIndex = ubIndex / 32;
    UBYTE ubRegShift = ubIndex % 32;
    uint32_t ulRegState = (1 << ubRegShift);
    if(ubState) {
        wr32le(&pGpio->GPSET[ubRegIndex], ulRegState);
    }
    else {
        wr32le(&pGpio->GPCLR[ubRegIndex], ulRegState);
    }
}

#define I2C_C_I2CEN (1 << 15) // I2C Enable
#define I2C_C_INTR (1 <<  10) // Interrupt on RX
#define I2C_C_INTT (1 << 9) // Interrupt on TX
#define I2C_C_INTD (1 << 8) // Interrupt on Done
#define I2C_C_ST (1 << 7) // Start transfer
#define I2C_C_CLEAR_FIFO_NONE (0b00 << 4)
#define I2C_C_CLEAR_FIFO_ONE_SHOT (0b01 << 4)
#define I2C_C_CLEAR_FIFO_ONE_SHOT2 (0b10 << 4)
#define I2C_C_WRITE_PACKET (0 << 0)
#define I2C_C_READ_PACKET (1 << 0)

#define I2C_S_CLKT (1 << 9) // Clock stretch timeout
#define I2C_S_ERR (1 << 8) // ERR Ack error
#define I2C_S_RXF (1 << 7) // RX FIFO full
#define I2C_S_TXE (1 << 6) // TX FIFO empty
#define I2C_S_RXD (1 << 5) // RX contains data
#define I2C_S_TXD (1 << 4) // TX contains data
#define I2C_S_RXR (1 << 3) // RX needs reading
#define I2C_S_TXW (1 << 2) // TX needs writing
#define I2C_S_DONE (1 << 1) // Transfer done
#define I2C_S_TA (1 << 0) // Transfer active

typedef struct tI2cRegs {
    ULONG C;
    ULONG S;
    ULONG DLEN;
    ULONG A;
    ULONG FIFO;
    ULONG DIV;
    ULONG DEL;
    ULONG CLKT;
} tI2cRegs;

#define BCM2708_PERI_BASE 0xF2000000

#define BCM_GPIO ((volatile tGpioRegs*)(BCM2708_PERI_BASE + 0x200000));
#define BCM_I2C0 (volatile tI2cRegs*)(BCM2708_PERI_BASE + 0x205000);

#define CORE_CLOCK (150*1000*1000)
#define I2C_SPEED 100000


#define RESULT(isSuccess, ubIoError, ubAllocError) ((ubAllocError << 16) | (ubIoError << 8) | (isSuccess))


// Allocation Errors
// (as returned by AllocI2C, BringBackI2C, or found in the middle high
// byte of the error codes from V39's SendI2C/ReceiveI2C)
enum {
    I2C_OK=0, // Hardware allocated successfully
    I2C_PORT_BUSY, // \_Allocation is actually done in two steps:
    I2C_BITS_BUSY, // / port & bits, and each step may fail
    I2C_NO_MISC_RESOURCE, // Shouldn't occur, something's very wrong
    I2C_ERROR_PORT, // Failed to create a message port
    I2C_ACTIVE, // Some other I2C client has pushed us out
    I2C_NO_TIMER // Failed to open the timer.device
};

// I/O Errors
// (as found in the middle low byte of the error codes from V39's
// SendI2C/ReceiveI2C)
enum {
    // I2C_OK=0, // Last send/receive was OK
    I2C_REJECT=1, // Data not acknowledged (i.e. unwanted) */
    I2C_NO_REPLY, // Chip address apparently invalid */
    SDA_TRASHED, // SDA line randomly trashed. Timing problem? */
    SDA_LO, // SDA always LO \_wrong interface attached, */
    SDA_HI, // SDA always HI / or none at all? */
    SCL_TIMEOUT, // \_Might make sense for interfaces that can */
    SCL_HI,      // / read the clock line, but currently none can. */
    I2C_HARDW_BUSY // Hardware allocation failed
};

ULONG SendI2C(
    UBYTE ubAddress,
    UWORD uwDataSize,
    UBYTE pData[],
    volatile tI2cRegs *pI2c
)
{
    // bcm expects read/write bit to be omitted from address
    ubAddress >>= 1;

    if(rd32le(&pI2c->S) & I2C_S_TA) {
        return RESULT(0, I2C_HARDW_BUSY, 0);
    }

    UBYTE isSuccess = 1, ubIoError = I2C_OK, ubAllocError = I2C_OK;
    wr32le(&pI2c->A, ubAddress);
    wr32le(&pI2c->C, I2C_C_CLEAR_FIFO_ONE_SHOT);
    wr32le(&pI2c->S, I2C_S_CLKT | I2C_S_ERR | I2C_S_DONE);
    wr32le(&pI2c->DLEN, uwDataSize);
    wr32le(&pI2c->C, I2C_C_I2CEN | I2C_C_ST | I2C_C_WRITE_PACKET);

    UWORD uwBytesCopied = 0;
    while(!(rd32le(&pI2c->S) & I2C_S_DONE)) {
        while(uwDataSize && (rd32le(&pI2c->S) & I2C_S_TXW)) {
            wr32le(&pI2c->FIFO, *(pData++));
            uwBytesCopied++;
            --uwDataSize;
        }
    }

    ULONG ulStatus = rd32le(&pI2c->S);
    wr32le(&pI2c->S, I2C_S_DONE);

    if((ulStatus & (I2C_S_ERR | I2C_S_CLKT)) || uwDataSize) {
        isSuccess = 0;
        ubIoError = I2C_REJECT;
    }
    
    return RESULT(isSuccess, ubIoError, ubAllocError);
}

#define TC358743_I2C_ADDR 0x1e

int write_reg(int reg, const uint8_t *data, int nbytes, volatile tI2cRegs *pI2c) {
    UBYTE i2cbuf[6];
    i2cbuf[0] = (reg >> 8) & 0xFF;
    i2cbuf[1] = reg & 0xFF;
    switch(nbytes) {
        case 4:
            i2cbuf[4] = data[2];
            i2cbuf[5] = data[3];
        case 2: // Fallthrough
            i2cbuf[3] = data[1];
        case 1: // Fallthrough
            i2cbuf[2] = data[0];
            break;
        default:
            break;
    }
    LONG res = SendI2C(TC358743_I2C_ADDR, 2 + nbytes, i2cbuf, pI2c);
    if( (res & 0xff)==0 ) {
        bug("[unicam] I2C write error: reg 0x%04lx, code %ld\n", reg, res);
        return 0;
    }
    return 0;
}

int write_reg8(int reg, uint8_t val, volatile tI2cRegs *pI2c) {
    return write_reg(reg, &val, 1, pI2c);
}

void u16_to_bytes(uint16_t val, uint8_t *buf, int big_endian) {
    if (big_endian) {
        buf[0] = (val >> 8) & 0xFF;
        buf[1] = val & 0xFF;
    } else {
        buf[0] = val & 0xFF;
        buf[1] = (val >> 8) & 0xFF;
    }
}

void u32_to_bytes(uint32_t val, uint8_t *buf, int big_endian) {
    if (big_endian) {
        buf[0] = (val >> 24) & 0xFF;
        buf[1] = (val >> 16) & 0xFF;
        buf[2] = (val >> 8) & 0xFF;
        buf[3] = val & 0xFF;
    } else {
        buf[0] = val & 0xFF;
        buf[1] = (val >> 8) & 0xFF;
        buf[2] = (val >> 16) & 0xFF;
        buf[3] = (val >> 24) & 0xFF;
    }
}

int write_reg16(int reg, uint16_t val, int big_endian, volatile tI2cRegs *pI2c) {
    uint8_t data[2];
    u16_to_bytes(val, data, big_endian);
    return write_reg(reg, data, 2, pI2c);
}

int write_reg32(int reg, uint32_t val, int big_endian, volatile tI2cRegs *pI2c) {
    uint8_t data[4];
    u32_to_bytes(val, data, big_endian);
    return write_reg(reg, data, 4, pI2c);
}

void _memcpy(uint8_t *to, const uint8_t *from, int n) {
    while(n--) *to++ = *from++;
}

static const uint8_t EDID_576p[256]  = {
  0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x05, 0xd7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xff, 0x22, 0x01, 0x03, 0x80, 0x32, 0x1f, 0x78, 0x07, 0xee, 0x95, 0xa3, 0x54, 0x4c, 0x99, 0x26,
  0x0f, 0x50, 0x54, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
  0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x8c, 0x0a, 0xd0, 0x90, 0x20, 0x40, 0x31, 0x20, 0x0c, 0x40,
  0x55, 0x00, 0xd8, 0xac, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17, 0xf0, 0x0f,
  0xff, 0x0b, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x45,
  0x44, 0x54, 0x56, 0x20, 0x35, 0x37, 0x36, 0x70, 0x0a, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xee,
  0x02, 0x03, 0x16, 0x00, 0x41, 0xa5, 0x67, 0x03, 0x0c, 0x00, 0x00, 0x00, 0x11, 0x16, 0x67, 0xd8,
  0x5d, 0xc4, 0x01, 0x16, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6b
};


void delay(ULONG us)
{
    ULONG timer = LE32(*(volatile ULONG*)0xf2003004);
    ULONG end = timer + us;

    if (end < timer) {
        while (end < LE32(*(volatile ULONG*)0xf2003004)) asm volatile("nop");
    }
    while (end > LE32(*(volatile ULONG*)0xf2003004)) asm volatile("nop");
}

// ---- EDID Programming ----
void program_edid(const uint8_t edid[256], int big_endian, volatile tI2cRegs *pI2c) {
    for (int i = 0; i < 256; i += 16) {
        uint8_t buf[18];
        //if (big_endian) {
            buf[0] = ((0x8C00 + i) >> 8) & 0xFF;
            buf[1] = (0x8C00 + i) & 0xFF;
        //} else {
        //    buf[0] = (0x8C00 + i) & 0xFF;
        //    buf[1] = ((0x8C00 + i) >> 8) & 0xFF;
        //}
        _memcpy(&buf[2], &edid[i], 16);
        LONG res = SendI2C(TC358743_I2C_ADDR, 18, buf, pI2c);
        if( (res & 0xff)==0 ){
            bug("[unicam] I2C EDID write error at 0x%04lx, code %ld\n", 0x8C00 + i, res);
            return;
        }
    }
}

void init_c790_ic(struct UnicamBase * UnicamBase)
{
    volatile tI2cRegs *pI2c = BCM_I2C0;
    volatile tGpioRegs *pGpio = BCM_GPIO;
    int big_endian = 0;

    UBYTE ubPinSda = 44;
    UBYTE ubPinScl = 45;

    gpioSetAlternate(pGpio, ubPinSda, GPIO44_AF_I2C0_SDA);
    gpioSetAlternate(pGpio, ubPinScl, GPIO45_AF_I2C0_SCL);
    gpioSetPull(pGpio, ubPinSda, GPIO_PULL_OFF);
    gpioSetPull(pGpio, ubPinScl, GPIO_PULL_OFF);


    //  --- CSI and Chip
    write_reg16(0x0004, 0x0000, big_endian, pI2c);
    write_reg16(0x0002, 0x0F00, big_endian, pI2c);
    delay(1 * 20000);
    write_reg16(0x0002, 0x0000, big_endian, pI2c);
    write_reg16(0x0006, 0x0080, big_endian, pI2c); //sub 720 = 0x80 , >720p =0x08
    write_reg16(0x0008, 0x005F, big_endian, pI2c);
    write_reg16(0x0014, 0xFFFF, big_endian, pI2c);
    write_reg16(0x0016, 0x051F, big_endian, pI2c);
    write_reg16(0x0020, 0x8111, big_endian, pI2c);
    write_reg16(0x0022, 0x0213, big_endian, pI2c);
    write_reg16(0x0004, 0x0E24, big_endian, pI2c);  // datatype

    write_reg32(0x0140, 0x00000000, big_endian, pI2c);
    write_reg32(0x0144, 0x00000000, big_endian, pI2c);
    write_reg32(0x0148, 0x00000001, big_endian, pI2c); // one Lane = 0x1 , two lane = 0x0
    write_reg32(0x014C, 0x00000001, big_endian, pI2c);
    write_reg32(0x0150, 0x00000001, big_endian, pI2c);

    write_reg32(0x0210, 0x00002988, big_endian, pI2c);
    write_reg32(0x0214, 0x00000005, big_endian, pI2c);
    write_reg32(0x0218, 0x00001D04, big_endian, pI2c);
    write_reg32(0x021C, 0x00000002, big_endian, pI2c);
    write_reg32(0x0220, 0x00000504, big_endian, pI2c);
    write_reg32(0x0224, 0x00004600, big_endian, pI2c);
    write_reg32(0x0228, 0x0000000A, big_endian, pI2c);
    write_reg32(0x022C, 0x00000004, big_endian, pI2c);
    write_reg32(0x0234, 0x0000001F, big_endian, pI2c);
    write_reg32(0x0204, 0x00000001, big_endian, pI2c);

    write_reg32(0x0518, 0x00000001, big_endian, pI2c);
    write_reg32(0x0500, 0xA3008080, big_endian, pI2c);

    write_reg8(0x8502, 0x01, pI2c);
    write_reg8(0x8512, 0xFE, pI2c);
    write_reg8(0x8513, 0xDF, pI2c);
    write_reg8(0x8515, 0xFD, pI2c);

    write_reg8(0x8531, 0x01, pI2c);
    write_reg16(0x8540, 0x0A8C, big_endian, pI2c);
    write_reg32(0x8630, 0x00041EB0, big_endian, pI2c);
    write_reg8(0x8670, 0x01, pI2c);
    write_reg8(0x8532, 0x80, pI2c);
    write_reg8(0x8536, 0x40, pI2c);
    write_reg8(0x853F, 0x0A, pI2c);
    write_reg8(0x8543, 0x32, pI2c);
    write_reg8(0x8544, 0x10, pI2c);
    write_reg8(0x8545, 0x31, pI2c);
    write_reg8(0x8546, 0x2D, pI2c);
    write_reg8(0x85C7, 0x01, pI2c);
    write_reg8(0x85CB, 0x01, pI2c);
    
    // --- EDID programming ---
    program_edid(EDID_576p, big_endian, pI2c);

    // --- HDMI RX ----
    write_reg8(0x8544, 0x01, pI2c);
    write_reg8(0x8544, 0x00, pI2c);
    delay(10 * 20000);
    write_reg8(0x8544, 0x10, pI2c);

    write_reg8(0x85D1, 0x01, pI2c);
    write_reg8(0x8560, 0x24, pI2c);
    write_reg8(0x8563, 0x11, pI2c);
    write_reg8(0x8564, 0x0F, pI2c);

    write_reg8(0x8574, 0x00, pI2c);  //RGB
    write_reg8(0x8573, 0x00, pI2c);  //RGB
    write_reg8(0x8576, 0x00, pI2c);  //RGB

    write_reg8(0x8600, 0x00, pI2c);
    write_reg8(0x8602, 0xF3, pI2c);
    write_reg8(0x8603, 0x02, pI2c);
    write_reg8(0x8604, 0x0C, pI2c);
    write_reg8(0x8606, 0x05, pI2c);
    write_reg8(0x8607, 0x00, pI2c);
    write_reg8(0x8620, 0x00, pI2c);
    write_reg8(0x8640, 0x01, pI2c);
    write_reg8(0x8641, 0x65, pI2c);
    write_reg8(0x8642, 0x07, pI2c);
    write_reg8(0x8652, 0x02, pI2c);
    write_reg8(0x8665, 0x10, pI2c);

    write_reg8(0x8709, 0xFF, pI2c);
    write_reg8(0x870B, 0x2C, pI2c);
    write_reg8(0x870C, 0x53, pI2c);
    write_reg8(0x870D, 0x01, pI2c);
    write_reg8(0x870E, 0x30, pI2c);
    write_reg8(0x9007, 0x10, pI2c);
    write_reg8(0x854A, 0x01, pI2c);

    delay(10 * 20000);
    // --- Final TX buffer enable ---
    write_reg16(0x0004, 0x0E27, big_endian, pI2c);
}
