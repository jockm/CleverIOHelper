#include <unistd.h>
#include <stdio.h>
#include "cleveriohelper.h"

int spiId;
int dcPin;


#define ILI9341_TFTWIDTH   240
#define ILI9341_TFTHEIGHT  320

#define ILI9341_NOP        0x00
#define ILI9341_SWRESET    0x01
#define ILI9341_RDDID      0x04
#define ILI9341_RDDST      0x09

#define ILI9341_SLPIN      0x10
#define ILI9341_SLPOUT     0x11
#define ILI9341_PTLON      0x12
#define ILI9341_NORON      0x13

#define ILI9341_RDMODE     0x0A
#define ILI9341_RDMADCTL   0x0B
#define ILI9341_RDPIXFMT   0x0C
#define ILI9341_RDIMGFMT   0x0D
#define ILI9341_RDSELFDIAG 0x0F

#define ILI9341_INVOFF     0x20
#define ILI9341_INVON      0x21
#define ILI9341_GAMMASET   0x26
#define ILI9341_DISPOFF    0x28
#define ILI9341_DISPON     0x29

#define ILI9341_CASET      0x2A
#define ILI9341_PASET      0x2B
#define ILI9341_RAMWR      0x2C
#define ILI9341_RAMRD      0x2E

#define ILI9341_PTLAR      0x30
#define ILI9341_MADCTL     0x36
#define ILI9341_VSCRSADD   0x37
#define ILI9341_PIXFMT     0x3A

#define ILI9341_FRMCTR1    0xB1
#define ILI9341_FRMCTR2    0xB2
#define ILI9341_FRMCTR3    0xB3
#define ILI9341_INVCTR     0xB4
#define ILI9341_DFUNCTR    0xB6

#define ILI9341_PWCTR1     0xC0
#define ILI9341_PWCTR2     0xC1
#define ILI9341_PWCTR3     0xC2
#define ILI9341_PWCTR4     0xC3
#define ILI9341_PWCTR5     0xC4
#define ILI9341_VMCTR1     0xC5
#define ILI9341_VMCTR2     0xC7

#define ILI9341_RDID1      0xDA
#define ILI9341_RDID2      0xDB
#define ILI9341_RDID3      0xDC
#define ILI9341_RDID4      0xDD

#define ILI9341_GMCTRP1    0xE0
#define ILI9341_GMCTRN1    0xE1

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

static void writeData(uint8_t *c, int len) {
    gpioWrite(dcPin, 1); // Data Mode
    spiWrite(spiId, c, len);
}

static void writeByte(uint8_t value) {
    uint8_t buf[1];

    buf[0] = value;

    gpioWrite(dcPin, 1); // Data Mode
    spiWrite(spiId, buf, sizeof (buf));
}

//static void write16(uint16_t value) {
//    uint8_t buf[2];
//
//
//    buf[0] = value >> 8; // MSB
//    buf[1] = value; // LSB
//
//    gpioWrite(dcPin, 1); // Data Mode
//    spiWrite(spiId, buf, sizeof (buf));
//}
//
//static void write32(uint32_t value) {
//    uint8_t buf[4];
//
//    buf[0] = value >> 24; // MSB
//    buf[1] = value >> 16;
//    buf[2] = value >> 8;
//    buf[3] = value; // LSB
//
//    gpioWrite(dcPin, 1); // Data Mode
//    spiWrite(spiId, buf, sizeof (buf));
//}

static void writeCommand(uint8_t c) {
    uint8_t buf[1];

    buf[0] = c;

    gpioWrite(dcPin, 0); // Command Mode
    spiWrite(spiId, buf, sizeof (buf));
}

int ili9341Init(void) {
    return 0;
}

int ili9341Open(int bus, int dev, int dcPinNo, int mhz, int rot, int colorMode, int invert) {
    spiId = spiOpen(bus, dev);
    if (spiId < 0) {
        return spiId;
    }

    dcPin = dcPinNo;
    gpioUnexport(dcPin);
    gpioExport(dcPin);
    gpioSetDirection(dcPin, DIR_OUTPUT);

    spiSetCSHigh(spiId, 0);
    spiSetMode(spiId, 0);
    spiSetMaxSpeedHz(spiId, mhz * 1000000);

    writeCommand(0xEF);
    writeByte(0x03);
    writeByte(0x80);
    writeByte(0x02);

    writeCommand(0xCF);
    writeByte(0x00);
    writeByte(0XC1);
    writeByte(0X30);

    writeCommand(0xED);
    writeByte(0x64);
    writeByte(0x03);
    writeByte(0X12);
    writeByte(0X81);

    writeCommand(0xE8);
    writeByte(0x85);
    writeByte(0x00);
    writeByte(0x78);

    writeCommand(0xCB);
    writeByte(0x39);
    writeByte(0x2C);
    writeByte(0x00);
    writeByte(0x34);
    writeByte(0x02);

    writeCommand(0xF7);
    writeByte(0x20);

    writeCommand(0xEA);
    writeByte(0x00);
    writeByte(0x00);

    writeCommand(ILI9341_PWCTR1); //Power control
    writeByte(0x23); //VRH[5:0]

    writeCommand(ILI9341_PWCTR2); //Power control
    writeByte(0x10); //SAP[2:0];BT[3:0]

    writeCommand(ILI9341_VMCTR1); //VCM control
    writeByte(0x3e);
    writeByte(0x28);

    writeCommand(ILI9341_VMCTR2); //VCM control2
    writeByte(0x86); //--

    writeCommand(ILI9341_MADCTL); // Memory Access Control
    //    _spiWrite(MADCTL_MV | MADCTL_BGR);
    writeByte(rot | colorMode);

    writeCommand(ILI9341_VSCRSADD); // Vertical scroll
//    write16(0); // Zero
    writeByte(0);
    writeByte(0);
            
    writeCommand(ILI9341_PIXFMT);
    writeByte(0x55);

    writeCommand(ILI9341_FRMCTR1);
    writeByte(0x00);
    writeByte(0x18);

    writeCommand(ILI9341_DFUNCTR); // Display Function Control
    writeByte(0x08);
    writeByte(0x82);
    writeByte(0x27);

    writeCommand(0xF2); // 3Gamma Function Disable
    writeByte(0x00);

    writeCommand(ILI9341_GAMMASET); //Gamma curve selected
    writeByte(0x01);

    writeCommand(ILI9341_GMCTRP1); //Set Gamma
    writeByte(0x0F);
    writeByte(0x31);
    writeByte(0x2B);
    writeByte(0x0C);
    writeByte(0x0E);
    writeByte(0x08);
    writeByte(0x4E);
    writeByte(0xF1);
    writeByte(0x37);
    writeByte(0x07);
    writeByte(0x10);
    writeByte(0x03);
    writeByte(0x0E);
    writeByte(0x09);
    writeByte(0x00);

    writeCommand(ILI9341_GMCTRN1); //Set Gamma
    writeByte(0x00);
    writeByte(0x0E);
    writeByte(0x14);
    writeByte(0x03);
    writeByte(0x11);
    writeByte(0x07);
    writeByte(0x31);
    writeByte(0xC1);
    writeByte(0x48);
    writeByte(0x08);
    writeByte(0x0F);
    writeByte(0x0C);
    writeByte(0x31);
    writeByte(0x36);
    writeByte(0x0F);

    writeCommand(ILI9341_SLPOUT); //Exit Sleep
    usleep(120000);
    writeCommand(ILI9341_DISPON); //Display on
    usleep(120000);

    return spiId;
}

int ili9341Close(void) {
    gpioUnexport(dcPin);

    int ret = spiClose(spiId);
    return ret;
}


// TODO these should be parameters
#define WIDTH 240
#define HEIGHT 320
#define SCREEN_LEN (WIDTH * HEIGHT * 2)

#define CHUNKSIZE 4096

uint8_t xBuf[CHUNKSIZE];


int ili9341Draw(int x1, int y1, int x2, int y2, uint8_t *displayBuffer) {
    int w = x2 - x1 + 1;
    int h = y2 - y1 + 1;
    int s = w * h * 2;
    
    uint8_t tBuf[4];

    tBuf[0] = (x1 >> 8) & 0xFF;
    tBuf[1] = x1 & 0xFF;
    tBuf[2] = (x2 >> 8) & 0xFF;
    tBuf[3] = x2 & 0xFF;
    writeData(tBuf, sizeof(tBuf));
    
            
    // for now we are ignoring the bounding box and just splatting the whole
    // screen out at once
    writeCommand(ILI9341_CASET); // Column addr set
    
    tBuf[0] = (x1 >> 8) & 0xFF;
    tBuf[1] = x1 & 0xFF;
    tBuf[2] = (x2 >> 8) & 0xFF;
    tBuf[3] = x2 & 0xFF;
    writeData(tBuf, sizeof(tBuf));
    
    writeCommand(ILI9341_PASET); // Row addr set
    
    tBuf[0] = (y1 >> 8) & 0xFF;
    tBuf[1] = y1 & 0xFF;
    tBuf[2] = (y2 >> 8) & 0xFF;
    tBuf[3] = y2 & 0xFF;
    writeData(tBuf, sizeof(tBuf));

    writeCommand(ILI9341_RAMWR); // write to RAM
    
    
    int numChunks = s / CHUNKSIZE;
    int lastChunkSize = s % CHUNKSIZE;
    

    uint8_t *buf = displayBuffer;
    
    gpioWrite(dcPin, 1); // Data Mode
    for(int i = 0; i < numChunks; ++i) {
        
        spiXfer2(spiId, buf, CHUNKSIZE, xBuf, 0);
        buf += CHUNKSIZE;
    }
    
    if(lastChunkSize > 0) {
        spiXfer2(spiId, buf, lastChunkSize, xBuf, 0);
    }
}


