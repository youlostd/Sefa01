#CXX	= distcc i386-obrien-freebsd5-g++
CXX	= $(shell less ../sanitize_cc.txt)

PLATFORM = $(shell file /bin/ls | cut -d' ' -f3 | cut -d'-' -f1)

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)
BSD_VERSION = $(shell uname -v 2>&1 | cut -d' ' -f2 | cut -d'.' -f1)
SVN_VERSION = $(shell svnversion -n -c . | cut -d':' -f2)

BIN     = ./libsql.a

ifeq ($(GCC_VERSION), 4)
IFLAGS = -I../libmysql/7.x-5.1.35
CFLAGS  = $(IFLAGS) -Wall -O2 -pipe -D_THREAD_SAFE -fno-exceptions 
else
IFLAGS = -I../libmysql/5.x-5.1.35
CFLAGS  = $(IFLAGS) -Wall -O2 -pipe -D_THREAD_SAFE -fno-exceptions 
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

OBJFILES = AsyncSQL.o Semaphore.o Tellwait.o Statement.o

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
	$(CXX) $(CFLAGS) -MM *.cpp > Depend

$(OBJFILES):
	$(CXX) $(CFLAGS) -c $<

sinclude Depend
