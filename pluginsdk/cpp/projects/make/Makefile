# This is a rudimentary Makefile that compiles files from the pluginsdk & src directories into a
# shared library. Adapt to your needs.

CXX = $(PREFIX)g++$(SUFFIX)
CXXFLAGS += -std=gnu++0x -Wall -Wextra -g -pipe -O3 -I../../
LINKFLAGS += -static-libgcc -static-libstdc++ -g -O3 -shared -Wl,--add-stdcall-alias

VPATH = ../../pluginsdk:../../src

OUTPUT_DIR = build
OUTPUT_OPTION = -o $(OUTPUT_DIR)/$@

OBJS = \
	Config.o \
	Connections.o \
	Core.o \
	Hooks.o \
	Hubs.o \
	Logger.o \
	Queue.o \
	Tagger.o \
	UI.o \
	Util.o \
	main.o \
	Plugin.o \
	stdafx.o

ifeq ($(OS), Windows_NT)
	OBJS += resource.o
	LIBEXT = .dll
else
	LIBEXT = .so
endif

all: \
	ensure-dirs \
	Plugin

Plugin: $(OBJS)
	cd $(OUTPUT_DIR); $(CXX) $(LINKFLAGS) -o $@$(LIBEXT) $^

ensure-dirs:
	mkdir -p $(OUTPUT_DIR)

resource.o: resource.rc
	windres ../../src/resource.rc $(OUTPUT_OPTION)

clear:
	$(RM) -r $(OUTPUT_DIR)
