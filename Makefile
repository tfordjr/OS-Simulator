GCC = gcc
CFLAGS = -g -Wall -Wshadow
LDFLAGS = -lrt -pthread

SRCS = master.c slave.c

%: %.c
	$(GCC) $(CFLAGS) $< -o $@ $(LDFLAGS)

all: master slave

clean:
	rm -f master slave cstest logfile.*
