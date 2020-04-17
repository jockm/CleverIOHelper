// This file is based on the following example code: 
//      https://github.com/embeddedarm/gpio-sysfs-demo

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "cleveriohelper.h"

#ifndef UNUSED
#	define	UNUSED(a) a = a
#endif

char buf[101];

const char *getGPIOPath(int pin, const char *func) {
    sprintf(buf, "/sys/class/gpio/gpio%d/%s", pin, func);
    return (const char *) buf;
}

int gpioInit(void) {
    return 0;
}

int gpioSetDirection(int gpio, int dir) {
    int ret = 0;
    const char *buf = getGPIOPath(gpio, "direction");

    int gpiofd = open(buf, O_WRONLY);
    if (gpiofd < 0) {
        perror("Couldn't open IRQ file");
        ret = -1;
    }

    if (dir == 2 && gpiofd) {
        if (3 != write(gpiofd, "high", 3)) {
            perror("Couldn't set GPIO direction to out");
            ret = -2;
        }
    }

    if (dir == 1 && gpiofd) {
        if (3 != write(gpiofd, "out", 3)) {
            perror("Couldn't set GPIO direction to out");
            ret = -3;
        }
    } else if (gpiofd) {
        if (2 != write(gpiofd, "in", 2)) {
            perror("Couldn't set GPIO directio to in");
            ret = -4;
        }
    }

    close(gpiofd);
    return ret;
}

int gpioSetEdge(int gpio, int rising, int falling) {
    int ret = 0;
    const char *buf = getGPIOPath(gpio, "edge");
    int gpiofd = open(buf, O_WRONLY);
    if (gpiofd < 0) {
        perror("Couldn't open IRQ file");
        ret = -1;
    }

    if (gpiofd && rising && falling) {
        if (4 != write(gpiofd, "both", 4)) {
            perror("Failed to set IRQ to both falling & rising");
            ret = -2;
        }
    } else {
        if (rising && gpiofd) {
            if (6 != write(gpiofd, "rising", 6)) {
                perror("Failed to set IRQ to rising");
                ret = -2;
            }
        } else if (falling && gpiofd) {
            if (7 != write(gpiofd, "falling", 7)) {
                perror("Failed to set IRQ to falling");
                ret = -3;
            }
        }
    }

    close(gpiofd);

    return ret;
}

int gpioExport(int gpio) {
    int efd;
    char buf[50];
    int ret;

    /* Quick test if it has already been exported */
    const char *buf1 = getGPIOPath(gpio, "value");
    efd = open(buf1, O_WRONLY);
    if (efd != -1) {
        close(efd);
    }


    efd = open("/sys/class/gpio/export", O_WRONLY);

    if (efd != -1) {
        sprintf(buf, "%d", gpio);
        ret = write(efd, buf, strlen(buf));
        if (ret < 0) {
            perror("Export failed");
            return -2;
        }
        close(efd);
    } else {
        // If we can't open the export file, we probably
        // dont have any gpio permissions
        return -1;
    }
    return 0;
}

int gpioUnexport(int gpio) {
    int gpiofd, ret;
    char buf[50];
    gpiofd = open("/sys/class/gpio/unexport", O_WRONLY);
    sprintf(buf, "%d", gpio);
    ret = write(gpiofd, buf, strlen(buf));
    close(gpiofd);

    return ret;
}

int gpioRead(int gpio) {
    char in[3] = {0, 0, 0};
    int nread, gpiofd;

    const char *buf = getGPIOPath(gpio, "value");
    gpiofd = open(buf, O_RDWR);
    if (gpiofd < 0) {
        fprintf(stderr, "Failed to open gpio %d value\n", gpio);
        perror("gpio failed");
    }

    do {
        nread = read(gpiofd, in, 1);
    } while (nread == 0);
    if (nread == -1) {
        perror("GPIO Read failed");
        return -1;
    }

    close(gpiofd);
    return atoi(in);
}

int gpioWrite(int gpio, int val) {
    int ret, gpiofd;

    const char *buf = getGPIOPath(gpio, "value");
    gpiofd = open(buf, O_RDWR);
    if (gpiofd > 0) {
        char tStr[10];
        snprintf(tStr, 2, "%d", val);
        ret = write(gpiofd, tStr, 2);
        if (ret < 0) {
            perror("failed to set gpio");
            return 1;
        }

        close(gpiofd);
        if (ret == 2) return 0;
    }
    return 1;
}

int gpioSelect(int gpio, int timeout) {
    int buf, irqfd;
    fd_set fds;
	struct timeval tv;
    FD_ZERO(&fds);

    const char *gpio_irq = getGPIOPath(gpio, "value");
    irqfd = open(gpio_irq, O_RDONLY, S_IRUSR);
    if (irqfd < 1) {
        perror("Couldn't open the value file");
        return -1;
    }

    if(timeout > 0) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
    }
    
    // Read first since there is always an initial status
    int err = read(irqfd, &buf, sizeof (buf));
    UNUSED(err);

    while (1) {
        FD_SET(irqfd, &fds);
        /*int numSets = */select(irqfd + 1, NULL, NULL, &fds, timeout <= 0 ? NULL : & tv);
	// printf("Duck numSets %d\n", numSets);

        if (FD_ISSET(irqfd, &fds)) {
            FD_CLR(irqfd, &fds); //Remove the filedes from set
            // Clear the junk data in the IRQ file
            int err = read(irqfd, &buf, sizeof (buf));
            UNUSED(err);
            
            close(irqfd);
            return 1;
        } else {
            close(irqfd);
            return 0;
        }
    }

    return 0;
}
