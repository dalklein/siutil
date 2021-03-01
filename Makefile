
DEBUG=no

PROG=siutil
SRCS=siutil.c
OBJS=$(SRCS:.c=.o)

ifeq ($(DEBUG),yes)
CFLAGS=-Wall -g -DDEBUG
else
CFLAGS=-O2 -pipe
endif
CFLAGS+=-I../include/ -I../smalib -I../libs -I../projects/generic-cmake/incprj -I../os -I../core -I../protocol
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

