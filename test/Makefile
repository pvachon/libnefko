TARGETS=nefko_open

.PHONY: all clean

CC = gcc
CFLAGS = -I.. -O0 -g
LDFLAGS = -L../ -lnefko

all: $(TARGETS)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(TARGETS): $(TARGETS).o
	$(CC) $(LDFLAGS) -o $@ $<

clean:
	$(RM) $(OBJ) $(TARGETS)
