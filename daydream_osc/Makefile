CC     = gcc
CFLAGS = -Wall -Wextra -ansi -pedantic -std=c99 -lm -lpthread -Wno-sign-compare -Wno-strict-prototypes

UNAME  = $(shell uname)
ifneq ($(UNAME), Darwin)
CFLAGS += -lrt
endif

INC_1  = ./libs/common
INC_2  = ./libs/libetherdream
INC_3  = ./libs/tinyosc
CFLAGS += -I${INC_1} -I${INC_2} -I${INC_3}

OUT_BIN = daydream_osc

build: daydream_osc.c
	$(CC) $(CFLAGS) ./libs/tinyosc/tinyosc.c ./libs/libetherdream/etherdream.c daydream_osc.c -o $(OUT_BIN)

clean:
	rm -rf  $(OUT_TEST) $(OUT_BIN)

.PHONY := clean
.DEFAULT_GOAL := build
