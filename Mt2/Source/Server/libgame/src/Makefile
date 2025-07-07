CXX	= $(shell less ../../sanitize_cc.txt)
BIN_DIR = ../lib
BIN = $(BIN_DIR)/libgame.a

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)

ifeq ($(GCC_VERSION), 4)
CFLAGS = -Wall -O2 -pipe -fno-exceptions -I../include
else
CFLAGS = -Wall -O2 -pipe -fno-exceptions -I../include
endif

CFLAGS += -std=c++11

CFLAGS += $(shell less ../../sanitize_flags.txt)

# Use pipes for communicating between sub-processes. Faster.
CFLAGS += -pipe
# Intel Core 2 CPU with 64-bit extensions, MMX, SSE, SSE2, SSE3 and SSSE3 instruction set support.
CFLAGS += -march=core2
# Use SSE for floating point operations
CFLAGS += -mfpmath=sse

CFLAGS += -mstack-alignment=16 -mstackrealign

LIBS =

OBJFILES = grid.o attribute.o targa.o

default:
	$(MAKE) $(BIN)

$(BIN): $(OBJFILES)
	if [ ! -d $(BIN_DIR) ]; then mkdir $(BIN_DIR); fi
	ar cru $(BIN) $(OBJFILES) $(LIBS)
	ranlib $(BIN)
	chmod 700 $(BIN)

clean:
	rm -f *.o
	rm -f $(BIN)

dep:
	touch Depend
	$(CXX) $(CFLAGS) -MM *.cc > Depend

$(OBJFILES):
	$(CXX) $(CFLAGS) -c $<

include Depend

