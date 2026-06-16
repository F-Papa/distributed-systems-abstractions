CC = gcc

SAN = -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer

CFLAGS = -Iinclude -I/usr/include/uuid \
         -Wall -Wextra \
         $(SAN)

LDLIBS = -luuid -lsodium $(SAN)

SRCS = $(shell find src -name "*.c")
OBJS = $(SRCS:.c=.o)

.PHONY: all clean gen_keys

all: stacking_components

stacking_components: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

gen_keys: tools/gen_keys.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)
	./gen_keys
	$(RM) gen_keys

clean:
	$(RM) $(OBJS) stacking_components
