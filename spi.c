// This code is based on:
//      https://github.com/doceme/py-spidev
//      Under the MIT License


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
//#include "structmember.h"
#include <linux/spi/spidev.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <unistd.h>
#include <stdint.h>

#include "cleveriohelper.h"

//JEM
//#define SPIDEV_SINGLE

#define SPIDEV_MAXPATH 4096

#define BLOCK_SIZE_CONTROL_FILE "/sys/module/spidev/parameters/bufsiz"

typedef struct {
    int fd; /* open file descriptor: /dev/spidevX.Y */
    uint8_t mode; /* current SPI mode */
    uint8_t bits_per_word; /* current SPI bits per word setting */
    uint32_t max_speed_hz; /* current SPI max speed setting in Hz */
} SpiDevObject;


#define MAX_SPI 4
SpiDevObject spiObjects[MAX_SPI];
int spiCount;

// TODO the table needs to have unique slots based on BUS/DEV

int spiInit(void) {
    static int initialized;
    
    if(initialized) {
        return 0;
    }
    
    initialized = 1;
    spiCount = 0;

    for (int i = 0; i < MAX_SPI; ++i) {
        spiObjects[i].fd = -1;
        spiObjects[i].mode = 0;
        spiObjects[i].bits_per_word = 0;
        spiObjects[i].max_speed_hz = 0;
    }
    
    return 0;
}
//static char *wrmsg_list0 = "Empty argument list.";
//static char *wrmsg_listmax = "Argument list size exceeds %d bytes.";
//static char *wrmsg_val = "Non-Int/Long value in arguments: %x.";
//static char *wrmsg_oom = "Out of memory.";

int spiWrite(int spiId, uint8_t *data, int dataLen) {
    int status;
    uint16_t ii, len;
    uint8_t buf[SPIDEV_MAXPATH];
    char wrmsg_text[4096];

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    len = dataLen;
    if (!data || len <= 0) {
        return -1;
    }

    if (len > SPIDEV_MAXPATH) {
        return -2;
    }

    memcpy(buf, data, len);

    status = write(spiObjects[spiId].fd, &buf[0], len);

    if (status < 0) {
        return -3;
    }

    if (status != len) {
        return -4;
    }

    return 0;
}

int spiRead(int spiId, uint8_t *data, int dataLen) {
    uint8_t rxbuf[SPIDEV_MAXPATH];
    int status, len, ii;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    len = dataLen;

    /* read at least 1 byte, no more than SPIDEV_MAXPATH */
    if (len < 1)
        len = 1;
    else if ((unsigned) len > sizeof (rxbuf))
        len = sizeof (rxbuf);

    memset(rxbuf, 0, sizeof rxbuf);
    status = read(spiObjects[spiId].fd, &rxbuf[0], len);

    if (status < 0) {
        return -1;
    }

    memcpy(data, rxbuf, status);

    return status;
}

int spiXfer(int spiId, uint8_t *data, int dataLen, uint8_t *recvData, int usecDelay) {
    uint16_t ii, len;
    int status;
    uint16_t delay_usecs = usecDelay;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }


#ifdef SPIDEV_SINGLE
    struct spi_ioc_transfer *xferptr;
    memset(&xferptr, 0, sizeof (xferptr));
#else
    struct spi_ioc_transfer xfer;
    memset(&xfer, 0, sizeof (xfer));
#endif 


    uint8_t *txbuf, *rxbuf;

    len = dataLen;
    if (!data || len <= 0) {
        return -1;
    }

    if (len > SPIDEV_MAXPATH) {
        return -2;
    }

    txbuf = malloc(sizeof (__u8) * len);
    rxbuf = malloc(sizeof (__u8) * len);

#ifdef SPIDEV_SINGLE
    xferptr = (struct spi_ioc_transfer*) malloc(sizeof (struct spi_ioc_transfer) * len);

    memcpy(txbuf, data, len);

    for (ii = 0; ii < len; ii++) {
        xferptr[ii].tx_buf = (unsigned long) &txbuf[ii];
        xferptr[ii].rx_buf = (unsigned long) &rxbuf[ii];
        xferptr[ii].len = 1;
        xferptr[ii].delay_usecs = delay_usecs;
        xferptr[ii].speed_hz = speed_hz ? speed_hz : spiObjects[spiId].max_speed_hz;
        xferptr[ii].bits_per_word = bits_per_word ? bits_per_word : spiObjects[spiId].bits_per_word;
#ifdef SPI_IOC_WR_MODE32
        xferptr[ii].tx_nbits = 0;
#endif
#ifdef SPI_IOC_RD_MODE32
        xferptr[ii].rx_nbits = 0;
#endif
    }

    status = ioctl(spiObjects[spiId].fd, SPI_IOC_MESSAGE(len), xferptr);
    free(xferptr);
    if (status < 0) {
        free(txbuf);
        free(rxbuf);
        return -1;
    }
#else
    
    memcpy(txbuf, data, len);

    xfer.tx_buf = (unsigned long) txbuf;
    xfer.rx_buf = (unsigned long) rxbuf;
    xfer.len = len;
    xfer.delay_usecs = delay_usecs;
    xfer.speed_hz = spiObjects[spiId].max_speed_hz;
    xfer.bits_per_word = spiObjects[spiId].bits_per_word;
#ifdef SPI_IOC_WR_MODE32
    xfer.tx_nbits = 0;
#endif
#ifdef SPI_IOC_RD_MODE32
    xfer.rx_nbits = 0;
#endif

    status = ioctl(spiObjects[spiId].fd, SPI_IOC_MESSAGE(1), &xfer);
    if (status < 0) {
        free(txbuf);
        free(rxbuf);
        return -1;
    }
#endif

    memcpy(recvData, rxbuf, len);

    // WA:
    // in CS_HIGH mode CS isn't pulled to low after transfer, but after read
    // reading 0 bytes doesnt matter but brings cs down
    // tomdean:
    // Stop generating an extra CS except in mode CS_HOGH
    if (spiObjects[spiId].mode & SPI_CS_HIGH) {
        status = read(spiObjects[spiId].fd, &rxbuf[0], 0);
    }

    free(txbuf);
    free(rxbuf);

    return len;
 }

int spiXfer2(int spiId, uint8_t *data, int dataLen, uint8_t *recvData, int usecDelay) {
    int status;
    uint16_t delay_usecs = usecDelay;
    uint16_t ii, len;
    struct spi_ioc_transfer xfer;
    uint8_t *txbuf, *rxbuf;
    char wrmsg_text[4096];

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    len = dataLen;
    memset(&xfer, 0, sizeof (xfer));
    
    if (len > SPIDEV_MAXPATH) {
        return -2;
    }

    txbuf = malloc(sizeof (__u8) * len);
    rxbuf = malloc(sizeof (__u8) * len);

    memcpy(txbuf, data, len);

    xfer.tx_buf = (unsigned long) txbuf;
    xfer.rx_buf = (unsigned long) rxbuf;
    xfer.len = len;
    xfer.delay_usecs = delay_usecs;
    xfer.speed_hz = spiObjects[spiId].max_speed_hz;
    xfer.bits_per_word = spiObjects[spiId].bits_per_word;

    status = ioctl(spiObjects[spiId].fd, SPI_IOC_MESSAGE(1), &xfer);
    if (status < 0) {
        free(txbuf);
        free(rxbuf);
        return -1;
    }

    memcpy(recvData, rxbuf, len);

    // WA:
    // in CS_HIGH mode CS isnt pulled to low after transfer
    // reading 0 bytes doesn't really matter but brings CS down
    // tomdean:
    // Stop generating an extra CS except in mode CS_HOGH
    if (spiObjects[spiId].mode & SPI_CS_HIGH) {
        status = read(spiObjects[spiId].fd, &rxbuf[0], 0);
    }

    free(txbuf);
    free(rxbuf);


    return len;
}

static int __spidev_set_mode(int spiId, uint8_t mode) {
    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    __u8 test;
    if (ioctl(spiObjects[spiId].fd, SPI_IOC_WR_MODE, &mode) == -1) {
        return -1;
    }

    if (ioctl(spiObjects[spiId].fd, SPI_IOC_RD_MODE, &test) == -1){
            return -2;
        }

        if (test != mode) {

            return -3;
        }

    return 0;
}


int spiGetMode(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return spiObjects[spiId].mode;
}


int spiGetCSHigh(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return !!(spiObjects[spiId].mode & SPI_CS_HIGH);
}


int spiGetLSBFirst(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return !!(spiObjects[spiId].mode & SPI_LSB_FIRST);
}

int spiGet3wire(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return !!(spiObjects[spiId].mode & SPI_3WIRE);
}

int spiGetLoop(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return !!(spiObjects[spiId].mode & SPI_LOOP);
}

int spiGetNoCS(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return !!(spiObjects[spiId].mode & SPI_NO_CS);
}


int spiSetMode(int spiId, int newMode) {
    uint8_t mode, tmp;
            int ret;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    mode = newMode;


    if (mode > 3) {
        return -1;
    }

    // clean and set CPHA and CPOL bits
    tmp = (spiObjects[spiId].mode & ~(SPI_CPHA | SPI_CPOL)) | mode;

            ret = __spidev_set_mode(spiObjects[spiId].fd, tmp);

    if (ret != -1) {

        spiObjects[spiId].mode = tmp;
    }

    return ret;
}

int spiSetCSHigh(int spiId, int val) {
    uint8_t mode, tmp;
            int ret;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (val)
            tmp = spiObjects[spiId].mode | SPI_CS_HIGH;
    else
        tmp = spiObjects[spiId].mode & ~SPI_CS_HIGH;

            ret = __spidev_set_mode(spiObjects[spiId].fd, tmp);

        if (ret >= 0) {

        spiObjects[spiId].mode = tmp;
    }

    return ret;
}

int spiSetLSBFirst(int spiId, int val) {
    uint8_t mode, tmp;
            int ret;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (val)
            tmp = spiObjects[spiId].mode | SPI_LSB_FIRST;
    else
        tmp = spiObjects[spiId].mode & ~SPI_LSB_FIRST;

            ret = __spidev_set_mode(spiObjects[spiId].fd, tmp);

        if (ret >= 0) {

        spiObjects[spiId].mode = tmp;
    }

    return ret;
}

int spiSet3wire(int spiId, int val) {
    uint8_t mode, tmp;
            int ret;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (val)
            tmp = spiObjects[spiId].mode | SPI_3WIRE;
    else
        tmp = spiObjects[spiId].mode & ~SPI_3WIRE;

            ret = __spidev_set_mode(spiObjects[spiId].fd, tmp);

        if (ret >= 0) {

        spiObjects[spiId].mode = tmp;
    }

    return ret;
}

int spiSetNoCS(int spiId, int val) {
    uint8_t mode, tmp;
            int ret;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (val)
            tmp = spiObjects[spiId].mode | SPI_NO_CS;
    else
        tmp = spiObjects[spiId].mode & ~SPI_NO_CS;

            ret = __spidev_set_mode(spiObjects[spiId].fd, tmp);

        if (ret >= 0) {

        spiObjects[spiId].mode = tmp;
    }

    return ret;
}


int spiSetLoop(int spiId, int val) {
    uint8_t mode, tmp;
            int ret;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (val)
            tmp = spiObjects[spiId].mode | SPI_LOOP;
    else
        tmp = spiObjects[spiId].mode & ~SPI_LOOP;

            ret = __spidev_set_mode(spiObjects[spiId].fd, tmp);

        if (ret >= 0) {

        spiObjects[spiId].mode = tmp;
    }

    return ret;
}

int spiGetBitsPerWord(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return spiObjects[spiId].bits_per_word;
}

int spiSetBitsPerWord(int spiId, int newBits) {
    uint8_t bits = newBits;

    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (spiObjects[spiId].bits_per_word != bits){
            if (ioctl(spiObjects[spiId].fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1) {
                return -1;
            }

            spiObjects[spiId].bits_per_word = bits;
        }

        return 0;
    }

int spiGetMaxSpeedHz(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {

        return -1000;
    }

    return spiObjects[spiId].max_speed_hz;
}

int spiSetMaxSpeedHz(int spiId, int max_speed_hz) {
    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if (spiObjects[spiId].max_speed_hz != max_speed_hz){
            if (ioctl(spiObjects[spiId].fd, SPI_IOC_WR_MAX_SPEED_HZ, &max_speed_hz) == -1) {
                return -1;
            }

            spiObjects[spiId].max_speed_hz = max_speed_hz;
        }

        return 0;
    }

int spiOpen(int bus, int device) {
    char path[SPIDEV_MAXPATH];

    if (spiCount >= MAX_SPI) {
        return -1000;
    }

    if (snprintf(path, SPIDEV_MAXPATH, "/dev/spidev%d.%d", bus, device) >= SPIDEV_MAXPATH) {
        return -2;
    }

    if ((spiObjects[spiCount].fd = open(path, O_RDWR, 0)) == -1) {
        return -1;
    }

    if (ioctl(spiObjects[spiCount].fd, SPI_IOC_RD_MODE, &spiObjects[spiCount].mode) == -1) {
        return -2;
    }

    if (ioctl(spiObjects[spiCount].fd, SPI_IOC_RD_BITS_PER_WORD, &spiObjects[spiCount].bits_per_word) == -1) {
        return -3;
    }
    if (ioctl(spiObjects[spiCount].fd, SPI_IOC_RD_MAX_SPEED_HZ, &spiObjects[spiCount].max_speed_hz) == -1) {

        return -4;
    }


    return spiCount++;
}

int spiClose(int spiId) {
    if (spiId < 0 || spiId >= MAX_SPI) {
        return -1000;
    }

    if ((spiObjects[spiId].fd != -1) && (close(spiObjects[spiId].fd) == -1)) {
        return -1;
    }

    spiObjects[spiId].fd = -1;
            spiObjects[spiId].mode = 0;
            spiObjects[spiId].bits_per_word = 0;
            spiObjects[spiId].max_speed_hz = 0;

    return 0;
}

