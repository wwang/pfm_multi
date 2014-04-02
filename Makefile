CC=gcc
CFLAGS=-c -Wall -D__PFM_MULTI_DEBUG__ -I../perfmon2-libpfm4/include/ -I../common_toolx/
LDFLAGS=-L../perfmon2-libpfm4/lib/ -L../common_toolx/ -static
LIBS=-lpthread -lpfm -lcommontoolx
SOURCES=pfm_multi.c pfm_operations.c perf_util.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=pfm_multi

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC)  $(OBJECTS) $(LDFLAGS) -o $@ $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@
clean:
	rm pfm_multi pfm_multi.o pfm_operations.o perf_util.o
