
DEBUG=no

PROG=siutil
SRCS=siutil.c
OBJS=$(SRCS:.c=.o)

YASDI_SRC=..
ifeq ($(DEBUG),yes)
CFLAGS=-Wall -g -DDEBUG
else
CFLAGS=-O2 -pipe
endif
CFLAGS+=-I$(YASDI_SRC)/include/ -I$(YASDI_SRC)/smalib -I$(YASDI_SRC)/libs -I$(YASDI_SRC)/projects/generic-cmake/incprj -I$(YASDI_SRC)/os -I$(YASDI_SRC)/core -I$(YASDI_SRC)/protocol
LIBS=-lyasdimaster -lyasdi -lpthread
LDFLAGS=-static

all: $(PROG)

CC=gcc
$(PROG): $(OBJS)  $(L1) $(L2) $(L3)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)
        
include Makefile.dep

install: $(PROG)
	install -m 755 -o bin -g bin $(PROG) /usr/local/bin/
	install -m 755 -o bin -g bin yasdi.ini /usr/local/etc/

zip: $(PROG)
	rm -f siutil_pi_static.zip
	zip siutil_pi_static.zip $(PROG)

push: clean
	git add -A .
	git commit -m refresh
	git push

pull: clean
	git reset --hard
	git pull


.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f $(PROG) $(OBJS)

