/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   cleveriohelper.h
 * Author: jock
 *
 * Created on March 3, 2020, 7:00 AM
 */

#ifndef CLEVERIOHELPER_H
#   define CLEVERIOHELPER_H

#   ifdef __cplusplus
        extern "C" {
#   endif

            // This file is based on the following example code: 
//      https://github.com/embeddedarm/gpio-sysfs-demo

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <fcntl.h>
//#include <unistd.h>
//#include <sys/select.h>
//#include <sys/stat.h>

#include <stdint.h>
#include "cleveriohelper.h"

#define DIR_INPUT 0
#define DIR_OUTPUT 1
#define DIR_OUTPUT_HIGH 2

#define HIGH 1
#define LOW 0
        
int gpioInit(void);
int gpioExport(int gpio);
int gpioSetDirection(int gpio, int dir);
int gpioSetEdge(int gpio, int rising, int falling);
int gpioRead(int gpio);
int gpioWrite(int gpio, int val);
int gpioSelect(int gpio, int timeout);
int gpioUnexport(int gpio);

int gpioGetFd(int gpio);

//////////////////////////////////////////////////////////////////

int i2cInit(void);
int i2cOpen(int bus, int addr);
int i2cClose(int i2cId);
int i2cRead(int i2cId);
int i2cWrite(int i2cId, int data);
int i2cWriteRegister8(int i2cId, int reg, int value);
int i2cWriteRegister16(int i2cId, int reg, int value);
int i2cReadRegister8(int i2cId, int reg);
int i2cReadRegister16(int i2cId, int reg);

//////////////////////////////////////////////////////////////////

int spiInit(void);
int spiOpen(int bus, int device);
int spiClose(int spiId);

int spiSetMode(int spiId, int newMode);
int spiSetCSHigh(int spiId, int val);
int spiSetLSBFirst(int spiId, int val);
int spiSet3wire(int spiId, int val);
int spiSetNoCS(int spiId, int val);
int spiSetLoop(int spiId, int val);
int spiSetMaxSpeedHz(int spiId, int max_speed_hz);

int spiGetMode(int spiId);
int spiGetCSHigh(int spiId);
int spiGetLsbFirst(int spiId);
int spiGet3wire(int spiId);
int spiGetLoop(int spiId);
int spiGetNoCS(int spiId);
int spiGetBitsPerWord(int spiId);
int spiGetMaxSpeedHz(int spiId);
int spiSetBitsPerWord(int spiId, int bits);

int spiWrite(int spiId, uint8_t *data, int dataLen);
int spiRead(int spiId, uint8_t *data, int dataLen);
int spiXfer(int spiId, uint8_t *data, int dataLen, uint8_t *recvData, int usecDelay);
int spiXfer2(int spiId, uint8_t *data, int dataLen, uint8_t *recvData, int usecDelay);

//////////////////////////////////////////////////////////////////

#define ILI9341_DEFAULT_ROT 0x40
#define ILI9341_DEFAULT_CMODE 0x08

int ili9341Init(void);
int ili9341Open(int bus, int dev, int dcPin, int mhz, int rot, int colorMode, int invert);
int ili9341Close(void);
int ili9341Draw(int x1, int y1, int x2, int y2, uint8_t *displayBuffer);


#   ifdef __cplusplus
    }
#   endif

#endif /* CLEVERIOHELPER_H */

