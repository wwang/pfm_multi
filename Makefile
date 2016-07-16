CC=gcc
AR=ar
LIBPFM4DIR=../libpfm-4.7.0
CFLAGS=-c -Wall -D__PFM_MULTI_DEBUG__ -I$(LIBPFM4DIR)/include/ -I../common_toolx/
LDFLAGS=-L../common_toolx/
LIBS=-lcommontoolx -lpthread -lrt $(LIBPFM4DIR)/lib/libpfm.a
ARFLAGS=rcs
SOURCES=pfm_multi.c pfm_operations.c perf_util.c pfm_trigger.c
INCLUDES=$(wildcard ./*.h)
OBJECTS=$(SOURCES:.c=.o)
USERLIBSOURCES=pfm_trigger_lib.c
USERLIBOBJECTS=$(USERLIBSOURCES:.c=.o)
EXECUTABLE=pfm_multi
USERLIB=libpfmtrigger.a

all: $(EXECUTABLE) $(USERLIB)

$(EXECUTABLE): $(OBJECTS) 
	$(CC)  $(OBJECTS) $(LDFLAGS) -o $@ $(LIBS)

$(USERLIB): $(USERLIBOBJECTS)
	$(AR) $(ARFLAGS) $@ $(USERLIBOBJECTS)

%.o: %.c $(INCLUDES)
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm pfm_multi $(OBJECTS) $(USERLIB) $(USERLIBOBJECTS)

test: test.c $(USERLIB)
	$(CC) $(LDFLAGS) test.c -o test $(USERLIB) $(LIBS)
