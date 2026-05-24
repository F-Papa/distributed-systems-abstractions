CFLAGS = -Iinclude -I/usr/include/uuid -Wall -Wextra
LDLIBS = -luuid -lsodium
SRCS = $(shell find src -name "*.c")
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: stacking_components

stacking_components: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

clean:
	$(RM) $(OBJS) stacking_components
