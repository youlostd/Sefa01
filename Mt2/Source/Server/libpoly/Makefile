CXX	= $(shell less ../sanitize_cc.txt)
BIN = ./libpoly.a

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)

ifeq ($(GCC_VERSION), 4)
CFLAGS  = -Wall -O2 -pipe -fno-exceptions -fno-rtti
else
CFLAGS  = -Wall -O2 -pipe -fno-exceptions -fno-rtti
endif

CFLAGS += -std=c++11

CFLAGS += $(shell less ../sanitize_flags.txt)

# Use pipes for communicating between sub-processes. Faster.
CFLAGS += -pipe
# Intel Core 2 CPU with 64-bit extensions, MMX, SSE, SSE2, SSE3 and SSSE3 instruction set support.
CFLAGS += -march=core2
# Use SSE for floating point operations
CFLAGS += -mfpmath=sse

CFLAGS += -mstack-alignment=16 -mstackrealign

LIBS = 

OBJFILES = Base.o Poly.o SymTable.o Symbol.o

default:
	$(MAKE) $(BIN)

$(BIN): $(OBJFILES)
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

