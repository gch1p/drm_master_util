CC = gcc
CFLAGS = -O2 -Wall -W
CFLAGS += `pkg-config --cflags libdrm`
LDFLAGS = `pkg-config --libs libdrm`
PROGRAM = drm_master_util
INSTALL = /usr/bin/install
PREFIX	= /usr

all: $(PROGRAM)

$(PROGRAM): drm_master_util.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

install: $(PROGRAM)
	$(INSTALL) $(PROGRAM) $(PREFIX)/bin
	chmod u+s $(PREFIX)/bin/${PROGRAM}

clean:
	rm -f *.o $(PROGRAM)

distclean: clean

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -I. -o $@

.PHONY: all install clean distclean
