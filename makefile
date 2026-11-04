# makefile for bobobot

# by Bill Kendrick
# New Breed Software
# bobobot@newbreedsoftware.com
# http://www.newbreedsoftware.com/bobobot/

# July 8, 1998 - November 30, 2000


# Makefile user-definable variables:-----------------------------------------

# (Defaults)

X11_PREFIX=/usr/X11R6
INSTALLROOT=$(PWD)
DATA=$(INSTALLROOT)

CC=gcc
MATH_LIB=-lm
X11_LIB=-L$(X11_PREFIX)/lib -lX11
SDL_LIB=$(shell sdl-config --libs) -lSDL_mixer
#SDL_LIB=--rpath -L$(SDL_PREFIX)/lib -lSDL_mixer -lpthread -lSDL -ldl

# Make programmer-defined variables:-----------------------------------------

CFLAGS=-I$(X11_PREFIX)/include -DDATA=\"$(DATA)\" \
	-DJOYSTICK_$(JOY) -DSOUND_$(SOUND) -DMUSIC_$(MUSIC) \
	-O2 -Wall -funroll-loops -ffast-math $(SDL_FLAGS_$(SOUND)$(MUSIC))
SDL_FLAGS_YES=$(shell sdl-config --cflags)
SDL_FLAGS_YESYES=$(SDL_FLAGS_YES)
SDL_LIB_YES=$(SDL_LIB)
SDL_LIB_YESYES=$(SDL_LIB_YES)

OBJECTS=obj/bobobot.o obj/window.o obj/connect.o obj/hints.o obj/visual.o \
	obj/gc.o obj/color.o obj/randnum.o obj/text.o


# Makefile commands:---------------------------------------------------------

all:	bobobot
	@echo "... Making sure data directory ($(DATA)/...) exists"
	- mkdir $(DATA)
	chmod 755 $(DATA)
	@echo "... Making sure image, sound and level directories exist"
	- mkdir $(DATA)/sounds $(DATA)/images $(DATA)/levels $(DATA)/mods
	chmod 755 $(DATA)/sounds $(DATA)/images $(DATA)/levels $(DATA)/mods
	@echo "... Copying data into data directories ($(DATA)/...)"
	- cp sounds/* $(DATA)/sounds 2> /dev/null
	- cp images/* $(DATA)/images 2> /dev/null
	- cp levels/* $(DATA)/levels 2> /dev/null
	- cp mods/* $(DATA)/mods 2> /dev/null
	chmod 644 $(DATA)/sounds/* $(DATA)/images/* $(DATA)/levels/* \
		$(DATA)/mods/*
	@echo "... Copying executable into installation location \
		($(INSTALLROOT))"
	- cp bobobot $(INSTALLROOT) 2> /dev/null

clean:
	- rm obj/*.o bobobot


# Applications:--------------------------------------------------------------

bobobot:	$(OBJECTS)
	$(CC)	$(OBJECTS) $(X11_LIB) $(SDL_LIB_$(SOUND)$(MUSIC)) $(MATH_LIB) \
		$(CFLAGS) -o bobobot
	chmod 755 bobobot


# Application objects:-------------------------------------------------------

obj/bobobot.o:	src/bobobot.c src/window.h src/connect.h src/hints.h \
		src/visual.h src/gc.h src/color.h src/randnum.h src/text.h
	$(CC)	src/bobobot.c -c $(CFLAGS) -o obj/bobobot.o


# Library objects:-----------------------------------------------------------

obj/window.o:	src/window.c src/window.h
	$(CC)	src/window.c -c $(CFLAGS) -o obj/window.o

obj/connect.o:	src/connect.c src/connect.h
	$(CC)	src/connect.c -c $(CFLAGS) -o obj/connect.o

obj/hints.o:	src/hints.c src/hints.h
	$(CC)	src/hints.c -c $(CFLAGS) -o obj/hints.o

obj/visual.o:	src/visual.c src/visual.h
	$(CC)	src/visual.c -c $(CFLAGS) -o obj/visual.o

obj/gc.o:	src/gc.c src/gc.h
	$(CC)	src/gc.c -c $(CFLAGS) -o obj/gc.o

obj/color.o:	src/color.c src/color.h
	$(CC)	src/color.c -c $(CFLAGS) -o obj/color.o

obj/randnum.o:	src/randnum.c src/randnum.h
	$(CC)	src/randnum.c -c $(CFLAGS) -o obj/randnum.o

obj/text.o:	src/text.c src/text.h
	$(CC)	src/text.c -c $(CFLAGS) -o obj/text.o

# The end!-------------------------------------------------------------------
