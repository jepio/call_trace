CC = gcc
CFLAGS = -g -O2
PIC = -fPIC
CT_CFLAGS = $(PIC) -Wall -std=gnu99 $(CFLAGS) -I/usr/include/libiberty/
CT_LDFLAGS = -Wl,-O1,-z,defs,-soname,$@.0
LDLIBS = -lunwind -liberty

all: shared

static: libcalltrace.a
shared: libcalltrace.so

clean:
	rm -f libcalltrace.so libcalltrace.so.0 libcalltrace.a call_trace_*.o

.PHONY: all clean static shared

libcalltrace.a: call_trace_nopic.o call_trace.h
	ar rcs $@ $<

libcalltrace.so: call_trace_pic.o call_trace.h
	$(CC) $(CT_CFLAGS) $(CT_LDFLAGS) -shared $< $(LDLIBS) -o $@
	ln -fs $@ $@.0

%_nopic.o: PIC =
%_nopic.o: %.c
	$(CC) $(CT_CFLAGS) -c $< -o $@

%_pic.o: %.c
	$(CC) $(CT_CFLAGS) -c $< -o $@
