GCC = gcc
CFLAGS = -g -Wall -Wshadow
LDFLAGS = -lrt -pthread

SRCS = oss.c slave.c

%: %.c
	$(GCC) $(CFLAGS) $< -o $@ $(LDFLAGS)

all:  oss slave

clean:
	rm -f oss slave cstest logfile.*
