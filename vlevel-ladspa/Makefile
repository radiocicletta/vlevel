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

# This is evil, but it makes implicit link rules use g++, not gcc
CC = $(CXX)

.PHONY: all
all: vlevel-ladspa.so

.PHONY: install
install: all
	cp -f vlevel-ladspa.so $(LADSPA_PATH)/

.PHONY: clean
clean:
	-rm -f *.o vlevel-ladspa.so

# This isn't ideal - if ../volumeleveler/volumeleveler.cpp was changed,
# but make wasn't yet run there, then the .o file won't be new, so
# the project will be improperly built.  Also, if volumeleveler.o doesn't
# exist yet, then make won't build it.  Unfortunately, it's discouraged
# to make it a .PHONY, and besides, that causes relinking every time
# make is run here, even during install (which leaves root's files).
# Make sucks.

../volumeleveler/volumeleveler.o:
	$(MAKE) -C ../volumeleveler all

vlevel-ladspa.so: vlevel-ladspa.o ../volumeleveler/volumeleveler.o
	$(CXX) $(CXXFLAGS) -shared -o vlevel-ladspa.so vlevel-ladspa.o ../volumeleveler/volumeleveler.o


vlevel-ladspa.o: vlevel-ladspa.cpp \
                 vlevel-ladspa.h \
                 ladspa.h \
                 ../volumeleveler/volumeleveler.h \
