MAKE = gmake
CC = $(shell less ../../sanitize_cc.txt)

DB_VERSION = $(shell cat ../../__REVISION__)

INCDIR =
LIBDIR =
BINDIR = ..
OBJDIR = .obj

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)
BSD_VERSION = $(shell uname -v 2>&1 | cut -d' ' -f2 | cut -d'.' -f1)
#$(shell less ../../common/Dbversion.txt)

$(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

# Depend Path File
DEPFILE = Depend

# Removed -fno-rtti -fno-exceptions
CFLAGS = -g -Wall -O0 -pipe -pthread -D_THREAD_SAFE -Werror=return-type -Werror=return-local-addr -w -Wunused -Wuninitialized -std=c++11

# Warning to Error
CFLAGS += -Werror=dynamic-class-memaccess

CFLAGS += $(shell less ../../sanitize_flags.txt)
#CFLAGS += -fsanitize=null
#CFLAGS += -fsanitize=unreachable
#CFLAGS += -fsanitize=return
#CFLAGS += -fsanitize=bool
#CFLAGS += -fsanitize=pointer-overflow
#CFLAGS += -fsanitize=signed-integer-overflow
#CFLAGS += -fsanitize=integer-divide-by-zero
#CFLAGS += -fsanitize=float-divide-by-zero

# Use pipes for communicating between sub-processes. Faster.
CFLAGS += -pipe
# Intel Core 2 CPU with 64-bit extensions, MMX, SSE, SSE2, SSE3 and SSSE3 instruction set support.
CFLAGS += -march=core2
# Use SSE for floating point operations
CFLAGS += -mfpmath=sse

CFLAGS += -mstack-alignment=16 -mstackrealign

#  is necessary to ensure $ORIGIN is passed unchanged
CFLAGS += -Wl,-rpath=$ORIGIN -Wl,-rpath-link= -Wl,-z,origin

# Newer binutils don't set DT_RPATH unless you disable "new" dtags
# and the new DT_RUNPATH doesn't work without --no-as-needed flag.
CFLAGS += -Wl,--disable-new-dtags

#ifeq ($(GCC_VERSION), 4)
#CFLAGS += -mtune=i686
#else
#CFLAGS += -mcpu=i686
#endif

# MySQL
LIBDIR += -L/usr/local/lib/mysql

# boost
INCDIR += -I../../Extern/include/boost

LIBDIR += -L../../libthecore/lib -L../../libsql -L../../libpoly -L../../libgame/lib

# Extern
INCDIR += -I../../Extern/include
LIBDIR += -I../../Extern/lib

# Project Library
INCDIR += -I/usr/local/include
LIBDIR += -L/usr/local/lib

ifeq ($(BSD_VERSION), 7)
INCDIR += -I../../libmysql/7.x-5.1.35
LIBDIR += -L../../libmysql/7.x-5.1.35
else
INCDIR += -I../../libmysql/5.x-5.1.35
LIBDIR += -L../../libmysql/5.x-5.1.35
endif

LIBS = -lthecore -lmysqlclient -lsql -lpoly -lgame -lm -lz

# Protobuf
LIBS += -lprotobuf

CPPFILE =	Config.cpp NetBase.cpp Peer.cpp PeerBase.cpp Main.cpp DBManager.cpp \
		Cache.cpp LoginData.cpp ClientManager.cpp ClientManagerPlayer.cpp ClientManagerLogin.cpp \
		ClientManagerBoot.cpp ClientManagerParty.cpp ClientManagerGuild.cpp GuildManager.cpp \
		PrivManager.cpp ItemAwardManager.cpp ClientManagerEventFlag.cpp Marriage.cpp \
		ItemIDRangeManager.cpp ProtoReader.cpp CsvReader.cpp GuildSafeboxManager.cpp


CPPFILE += protobuf_data.cpp\
		protobuf_data_item.cpp\
		protobuf_data_player.cpp\
		protobuf_data_pet.cpp\
		protobuf_dg_packets.cpp\
		protobuf_gd_packets.cpp\

CPPOBJS = $(CPPFILE:%.cpp=$(OBJDIR)/%.o)

# Target Paths
MAIN_TARGET = $(BINDIR)/db_r$(DB_VERSION)

default: $(MAIN_TARGET)

$(MAIN_TARGET): $(CPPOBJS)
	@echo linking $(MAIN_TARGET)...
	@$(CC) $(CFLAGS) $(LIBDIR) $(CPPOBJS) $(LIBS) -o $(MAIN_TARGET)

$(OBJDIR)/%.o: %.cpp
	@echo compile $<
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

$(OBJDIR):
	@mkdir $(OBJDIR)

symlink:
	@ln -fs db_r$(DB_VERSION) $(BINDIR)/db_symlink

strip:
	@cp $(BINDIR)/db_r$(DB_VERSION) $(BINDIR)/db_r
	@strip $(BINDIR)/db_r

clean:
	@rm -f $(CPPOBJS) $(BINDIR)/db_r*

dep:
	@touch $(DEPFILE)
	makedepend -f $(DEPFILE) $(INCDIR) -I/usr/include/c++/3.3 -I/usr/include/c++/4.2 -p$(OBJDIR)/ $(CPPFILE) 2> /dev/null > $(DEPFILE)

sinclude $(DEPFILE)
