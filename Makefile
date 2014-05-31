CC=gcc
AR=ar
CFLAGS=-c -Wall -D__PFM_MULTI_DEBUG__ -I../perfmon2-libpfm4/include/ -I../common_toolx/
LDFLAGS=-L../perfmon2-libpfm4/lib/ -L../common_toolx/ -static
LIBS=-lpthread -lpfm -lcommontoolx
ARFLAGS=rcs
SOURCES=pfm_multi.c pfm_operations.c perf_util.c pfm_trigger.c
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

.c.o:
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm pfm_multi $(OBJECTS) $(USERLIB) $(USERLIBOBJECTS)

test: test.c $(USERLIB)
	$(CC) $(LDFLAGS) test.c -o test $(USERLIB) $(LIBS)
