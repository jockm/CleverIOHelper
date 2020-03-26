// This file is based on the following example code: 
//      https://www.acmesystems.it/i2c

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#include "cleveriohelper.h"

#define I2C_SLAVE	0x0703
#define I2C_SMBUS	0x0720

#define I2C_SMBUS_READ	1
#define I2C_SMBUS_WRITE	0

// SMBus transaction types

#define I2C_SMBUS_QUICK		    0
#define I2C_SMBUS_BYTE		    1
#define I2C_SMBUS_BYTE_DATA	    2 
#define I2C_SMBUS_WORD_DATA	    3
#define I2C_SMBUS_PROC_CALL	    4
#define I2C_SMBUS_BLOCK_DATA	    5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7
#define I2C_SMBUS_I2C_BLOCK_DATA    8

// SMBus messages

#define I2C_SMBUS_BLOCK_MAX	32
#define I2C_SMBUS_I2C_BLOCK_MAX	32

union i2c_smbus_data
{
  uint8_t  byte ;
  uint16_t word ;
  uint8_t  block [I2C_SMBUS_BLOCK_MAX + 2] ;	// block [0] is used for length + one more for PEC
} ;

int i2cInit(void) {
    return 0;
}

int i2cOpen(int bus, int addr) {
    char path[101];

    snprintf(path, sizeof(path), "/dev/i2c-%d", bus);
    int ret = open(path, O_RDWR);
    if (ret < 0) {
        return ret;
    }

    if (ioctl(ret, I2C_SLAVE, addr) < 0) {
        printf("ioctl error: %s\n", strerror(errno));
        close(ret);
        return -1000;
    }
	
	return ret;
}

int i2cClose(int i2cId) {
    int ret = close(i2cId);
    return ret;
}

int i2cRead(int i2cId) {
    struct i2c_smbus_ioctl_data ioArgs;
    union  i2c_smbus_data       data;

    ioArgs.read_write = I2C_SMBUS_READ;
    ioArgs.command = 0;
    ioArgs.size = I2C_SMBUS_BYTE;
    ioArgs.data = &data;

    int ret = ioctl(i2cId, I2C_SMBUS, &ioArgs);
    if (ret < 0) {
        return ret;
    }

    return data.byte & 0xFF;
}

int i2cWrite(int i2cId, int data) {
    struct i2c_smbus_ioctl_data ioArgs;

    ioArgs.read_write = I2C_SMBUS_WRITE;
    ioArgs.command = data;
    ioArgs.size = I2C_SMBUS_BYTE;
    ioArgs.data = NULL;

    int ret = ioctl(i2cId, I2C_SMBUS, &ioArgs);
    
    return ret;

}

int i2cWriteRegister8(int i2cId, int reg, int value) {
    struct i2c_smbus_ioctl_data ioArgs;
    union  i2c_smbus_data       data;

    data.byte = value;
    
    ioArgs.read_write = I2C_SMBUS_WRITE;
    ioArgs.command = reg;
    ioArgs.size = I2C_SMBUS_BYTE_DATA;
    ioArgs.data = &data;

    int ret = ioctl(i2cId, I2C_SMBUS, &ioArgs);
    return ret;
}

int i2cWriteRegister16(int i2cId, int reg, int value) {
    struct i2c_smbus_ioctl_data ioArgs;
    union  i2c_smbus_data       data;

    data.word = value;
    
    ioArgs.read_write = I2C_SMBUS_WRITE;
    ioArgs.command = reg;
    ioArgs.size = I2C_SMBUS_WORD_DATA;
    ioArgs.data = &data;

    int ret = ioctl(i2cId, I2C_SMBUS, &ioArgs);
    return ret;
}

int i2cReadRegister8(int i2cId, int reg) {
    struct i2c_smbus_ioctl_data ioArgs;
    union  i2c_smbus_data       data;

    ioArgs.read_write = I2C_SMBUS_READ;
    ioArgs.command = reg;
    ioArgs.size = I2C_SMBUS_BYTE_DATA;
    ioArgs.data = &data;

    int err = ioctl(i2cId, I2C_SMBUS, &ioArgs);
    if(err < 0) {
        return err;
    }
    
    return data.byte & 0xFF;
}

int i2cReadRegister16(int i2cId, int reg) {
    struct i2c_smbus_ioctl_data ioArgs;
    union  i2c_smbus_data       data;

    ioArgs.read_write = I2C_SMBUS_READ;
    ioArgs.command = reg;
    ioArgs.size = I2C_SMBUS_WORD_DATA;
    ioArgs.data = &data;

    int err = ioctl(i2cId, I2C_SMBUS, &ioArgs);
    if(err < 0) {
        return err;
    }
    
    return data.byte & 0xFFFF;
}


static pthread_mutex_t transationLock = PTHREAD_MUTEX_INITIALIZER;

int i2cBeginTransaction(int wait)
{
	int  ret = 0;
	
	do {
		ret = pthread_mutex_trylock(&transationLock);
	} while(wait && ret != 0);
		
	return ret;
}


int i2cEndTransaction(void)
{
	int ret = pthread_mutex_unlock(&transationLock);
	return ret;
}



