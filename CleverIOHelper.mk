# Makefile template for a shared library in C
# https://www.topbug.net/blog/2019/10/28/makefile-template-for-a-shared-library-in-c-with-explanations/

CC = $(CROSS_COMPILE)gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -std=c99
LDFLAGS = -shared -fPIC  # linking flags
RM = rm -f
TARGET_LIB = libCleverIOHelper.so

#SRCS = gpio.c i2c.c ili9341.c spi.c
SRCS = gpio.c i2c.c spi.c
OBJS = $(SRCS:.c=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	echo DUCK [[[$(CROSS_COMPILE)$(CC)]]]
	$(CROSS_COMPILE)$(CC) -o $@ $^ ${LDFLAGS}

$(SRCS:.c=.d):%.d:%.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -MM $< >$@

include $(SRCS:.c=.d)

.PHONY: clean
clean:
	-${RM} ${TARGET_LIB} ${OBJS} $(SRCS:.c=.d)
