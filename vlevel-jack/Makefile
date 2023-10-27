# This file is part of VLevel, a dynamic volume normalizer.
#
# Copyright 2003 Tom Felker <tcfelker@mtco.com>
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA

PKG_CONFIG ?= pkg-config

CC = $(CXX)
CXXFLAGS += $(shell $(PKG_CONFIG) --cflags jack)
LDFLAGS += $(shell $(PKG_CONFIG) --libs-only-L jack)
LDLIBS += $(shell $(PKG_CONFIG) --libs-only-l jack)

# commented out rules were used to build it on OSX (yes, it works)
#LDFLAGS = -L/opt/local/lib
#LDLIBS = -ljack -lpthread -framework CoreAudio -framework CoreServices -framework AudioUnit
#CXXFLAGS= -I/opt/local/include -g -v

.PHONY: all
all: vlevel-jack

.PHONY: install
install: all
	cp -f vlevel-jack $(PREFIX)/bin/

.PHONY: clean
clean:
	-rm -f *.o vlevel-jack

# This isn't ideal - if ../volumeleveler/volumeleveler.cpp was changed,
# but make wasn't yet run there, then the .o file won't be new, so
# the project will be improperly built.  Also, if volumeleveler.o doesn't
# exist yet, then make won't build it.  Unfortunately, it's discouraged
# to make it a .PHONY, and besides, that causes relinking every time
# make is run here, even during install (which leaves root's files).
# Make sucks.

../volumeleveler/volumeleveler.o:
	$(MAKE) -C ../volumeleveler all

vlevel-jack: vlevel-jack.o \
            commandline.o \
            ../volumeleveler/volumeleveler.o

vlevel-jack.o: vlevel-jack.cpp \
              commandline.h \
              ../volumeleveler/volumeleveler.h \

commandline.o: commandline.cpp commandline.h
