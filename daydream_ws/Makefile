CC = gcc
CFLAGS = -Wall -Wextra -ansi -pedantic -std=c99 -lm -lpthread -Wno-sign-compare -Wno-strict-prototypes -Wno-unused-parameter

UNAME = $(shell uname)
ifneq ($(UNAME), Darwin)
CFLAGS += -lrt
endif

# Include paths
INC_1 = ./libs/common
INC_2 = ./libs/libetherdream
INC_3 = ./libs/wsServer/include
CFLAGS += -I${INC_1} -I${INC_2} -I${INC_3}

# Source files
WS_SRCS = libs/wsServer/src/ws.c \
          libs/wsServer/src/sha1.c \
          libs/wsServer/src/utf8.c \
          libs/wsServer/src/base64.c \
          libs/wsServer/src/handshake.c

# Object files
WS_OBJS = $(WS_SRCS:.c=.o)

# Target executable
OUT_BIN = daydream_ws

build: $(OUT_BIN)

$(OUT_BIN): $(WS_OBJS) daydream_ws.c
	$(CC) $(CFLAGS) $(WS_OBJS) ./libs/libetherdream/etherdream.c daydream_ws.c -o $(OUT_BIN)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(WS_OBJS) $(OUT_BIN)

.PHONY: all clean
.DEFAULT_GOAL := build
