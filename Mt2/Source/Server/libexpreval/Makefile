CXX	= $(shell less ../sanitize_cc.txt)
BIN = ./libexpreval.a

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)

#ifeq ($(GCC_VERSION), 4)
#CFLAGS  = -Wall -O2 -pipe -mtune=i686 -fno-exceptions -fno-rtti
#else
#CFLAGS  = -Wall -O2 -pipe -mcpu=i686 -fno-exceptions -fno-rtti
#endif
CFLAGS  = -std=c++11 -Wall -O2 -pipe -fno-rtti
#CFLAGS  = -Wall -O2 -pipe -fno-exceptions -fno-rtti

# Sanitizer

CFLAGS += $(shell less ../sanitize_flags.txt)

LIBS =

OBJFILES = except.o expr.o func.o funclist.o node.o parser.o vallist.o

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

