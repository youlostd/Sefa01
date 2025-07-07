GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)
BSD_VERSION = $(shell uname -v 2>&1 | cut -d' ' -f2 | cut -d'.' -f1)
GAME_VERSION := $(shell cat ../../__REVISION__)

CC = $(shell less ../../sanitize_cc.txt)

INCDIR =
LIBDIR =
BINDIR = ..
OBJDIR = OBJDIR
$(shell if [ ! -d $(OBJDIR) ]; then mkdir $(OBJDIR); fi)

# Depend Path File
DEPFILE = Depend

# Standard Setting
LIBS = -pthread -lm -lmd

# Removed -fno-rtti 
CFLAGS = -g -Wall -O2 -pipe -fexceptions -D_THREAD_SAFE -DNDEBUG -D__GAME_VERSION__=\"$(GAME_VERSION)\" -Werror=return-type -Werror=return-local-addr -w -Wunused -Wuninitialized -std=c++11

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


CFLAGS += -ferror-limit=100

#ifeq ($(GCC_VERSION), 4)
#CFLAGS += -mtune=i686 -fstack-protector-all
#else
#CFLAGS += -mcpu=i686
#endif

# expreval
LIBS += -lexpreval

# boost
INCDIR += -I../../Extern/include/boost

# DevIL
INCDIR += -I../../libdevil
LIBDIR += -L../../libdevil
LIBS += -lIL -lpng -ltiff -lmng -llcms -ljpeg

# MySQL
ifeq ($(BSD_VERSION), 7)
INCDIR += -I../../libmysql/7.x-5.1.35
LIBDIR += -L../../libmysql/7.x-5.1.35
else
INCDIR += -I../../libmysql/5.x-5.1.35
LIBDIR += -L../../libmysql/5.x-5.1.35
endif

LIBDIR += -L/usr/local/lib/mysql
LIBS += -lmysqlclient -lz

# Miscellaneous external libraries
INCDIR += -I../../Extern/include
LIBDIR += -L../../Extern/lib
#LIBS += -lcryptopp

# CryptoPP
LIBS += ../../../Extern/lib/libcryptopp.a

# openssl
#INCDIR += -I/usr/include
#LIBS += -lssl
LIBS += -lssl -lcrypto

# Lua
INCDIR += -I../../liblua/5.0/include
LIBDIR += -L../../liblua/5.0/lib
LIBS += ../../liblua/5.0/lib/liblua.a ../../liblua/5.0/lib/liblualib.a

# Protobuf
LIBS += -lprotobuf

# Project Library
#INCDIR += -I../../liblua/5.0/include
INCDIR += -I/usr/local/include
#LIBDIR += -L../../libthecore/lib -L../../libpoly -L../../libsql -L../../libgame/lib -L../../liblua/5.0/lib -L../../libexpreval
LIBDIR += -L../../libthecore/lib -L../../libpoly -L../../libsql -L../../libgame/lib -L../../libexpreval
LIBDIR += -L/usr/local/lib
LIBS += -lthecore -lpoly -llua -llualib -lsql -lgame
USE_STACKTRACE = 0
ifeq ($(USE_STACKTRACE), 1)
LIBS += /usr/local/lib/libexecinfo.a
endif

# PROJECT_SRC_FILES BEGIN
MAINCPP = main.cpp

CFILE	= minilzo.c

CPPFILE = FSM.cpp\
	MarkConvert.cpp\
	MarkImage.cpp\
	MarkManager.cpp\
	OXEvent.cpp\
	ani.cpp\
	arena.cpp\
	battle.cpp\
	blend_item.cpp\
	buffer_manager.cpp\
	building.cpp\
	char.cpp\
	char_affect.cpp\
	char_battle.cpp\
	char_horse.cpp\
	char_item.cpp\
	char_manager.cpp\
	char_quickslot.cpp\
	char_resist.cpp\
	char_skill.cpp\
	char_state.cpp\
	PetSystem.cpp\
	cmd.cpp\
	cmd_emotion.cpp\
	cmd_general.cpp\
	cmd_gm.cpp\
	cmd_oxevent.cpp\
	config.cpp\
	constants.cpp\
	crc32.cpp\
	cube.cpp\
	db.cpp\
	desc.cpp\
	desc_client.cpp\
	desc_manager.cpp\
	desc_p2p.cpp\
	dev_log.cpp\
	dungeon.cpp\
	entity.cpp\
	entity_view.cpp\
	event.cpp\
	event_queue.cpp\
	exchange.cpp\
	file_loader.cpp\
	fishing.cpp\
	gm.cpp\
	guild.cpp\
	guild_manager.cpp\
	guild_war.cpp\
	input.cpp\
	input_auth.cpp\
	input_db.cpp\
	input_login.cpp\
	input_main.cpp\
	input_p2p.cpp\
	item.cpp\
	item_addon.cpp\
	item_attribute.cpp\
	item_manager.cpp\
	item_manager_idrange.cpp\
	locale.cpp\
	log.cpp\
	login_data.cpp\
	lzo_manager.cpp\
	marriage.cpp\
	messenger_manager.cpp\
	mining.cpp\
	mob_manager.cpp\
	motion.cpp\
	over9refine.cpp\
	p2p.cpp\
	packet_info.cpp\
	party.cpp\
	polymorph.cpp\
	priv_manager.cpp\
	pvp.cpp\
	questevent.cpp\
	questlua.cpp\
	questlua_affect.cpp\
	questlua_arena.cpp\
	questlua_building.cpp\
	questlua_dungeon.cpp\
	questlua_game.cpp\
	questlua_global.cpp\
	questlua_guild.cpp\
	questlua_pet.cpp\
	questlua_item.cpp\
	questlua_marriage.cpp\
	questlua_npc.cpp\
	questlua_oxevent.cpp\
	questlua_party.cpp\
	questlua_pc.cpp\
	questlua_quest.cpp\
	questlua_target.cpp\
	questmanager.cpp\
	questnpc.cpp\
	questpc.cpp\
	refine.cpp\
	regen.cpp\
	safebox.cpp\
	sectree.cpp\
	sectree_manager.cpp\
	sequence.cpp\
	shop.cpp\
	skill.cpp\
	start_position.cpp\
	target.cpp\
	text_file_loader.cpp\
	trigger.cpp\
	utils.cpp\
	vector.cpp\
	war_map.cpp\
	wedding.cpp\
	xmas_event.cpp\
	map_location.cpp\
	BlueDragon.cpp\
	BlueDragon_Binder.cpp\
	DragonLair.cpp\
	skill_power.cpp\
	affect.cpp\
	cipher.cpp\
	buff_on_attributes.cpp\
	group_text_parse_tree.cpp\
	shop_manager.cpp\
	shopEx.cpp\
	item_manager_read_tables.cpp\
	public_table.cpp\
	empire_text_convert.cpp\
	guild_safebox.cpp\
	mount_system.cpp\
	questlua_mount.cpp\
	questlua_v4b.cpp\
	char_fakepc.cpp\
	DragonSoul.cpp\
	dragon_soul_table.cpp\
	char_dragonsoul.cpp\
	questlua_dragonsoul.cpp\
	general_manager.cpp\
	auction_manager.cpp\
	char_cards.cpp\
	MeleyLair.cpp\
	questlua_MeleyLair.cpp\
	char_fakebuff.cpp\
	attrtree_manager.cpp\
	event_manager.cpp\
	questlua_event.cpp\
	event_tagteam.cpp\
	combat_zone.cpp\
	HydraDungeon.cpp\
	rune_manager.cpp\
	mtrand.cpp\
	SpamFilter.cpp\
	char_blackjack.cpp\
	dmg_ranking.cpp\
	pet_advanced.cpp

CPPFILE += protobuf_data.cpp\
			protobuf_data_item.cpp\
			protobuf_data_player.cpp\
			protobuf_data_pet.cpp\
			protobuf_cg_packets.cpp\
			protobuf_dg_packets.cpp\
			protobuf_gc_packets.cpp\
			protobuf_gd_packets.cpp\
			protobuf_gg_packets.cpp
# PROJECT_SRC_FILES END

# PROJECT_OBJ_FILES BEGIN
COBJS	= $(CFILE:%.c=$(OBJDIR)/%.o)
CPPOBJS	= $(CPPFILE:%.cpp=$(OBJDIR)/%.o)

MAINOBJ = $(OBJDIR)/main.o
# PROJECT_OBJ_FILES END

# Target Paths
MAIN_TARGET  = $(BINDIR)/game_r$(GAME_VERSION)

default: $(MAIN_TARGET)

$(OBJDIR)/%.o: %.c
	@echo -e "\033[0;32m [OK] \033[0m       \033[0;33m Compiling:\033[0m" $<
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

$(OBJDIR)/%.o: %.cpp
	@echo compile $<
	@$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@

$(MAIN_TARGET): $(CPPOBJS) $(COBJS) $(MAINOBJ)
	@echo linking $(MAIN_TARGET)....
	@$(CC) $(CFLAGS) $(LIBDIR) $(COBJS) $(CPPOBJS) $(MAINOBJ) $(LIBS) -o $(MAIN_TARGET)

symlink:
	@ln -fs game_r$(GAME_VERSION) $(BINDIR)/game_symlink

strip:
	@cp $(MAIN_TARGET) $(BINDIR)/game_r
	@strip $(BINDIR)/game_r

clean:
	@echo cleaning $(MAIN_TARGET) $(OBJDIR)
	@rm -f $(COBJS) $(CPPOBJS) $(MAINOBJ)
	@rm -f $(BINDIR)/game_r*

dep:
	makedepend -f $(DEPFILE) $(INCDIR) -I/usr/include/c++/3.3 -I/usr/include/c++/4.2 -p$(OBJDIR)/ $(CPPFILE) $(CFILE) $(MAINCPP) 2> /dev/null > $(DEPFILE)

sinclude $(DEPFILE)
