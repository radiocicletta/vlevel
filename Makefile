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

# User-editable options:

# Change this to suit your preferences (maybe add -march=cputype)	

# This is what works fastest with GCC on my system
#
# I'd be interested to see how setting -DEXPECT impacts performance -
# on my system, it makes it a bit worse.

export VERSION = 0.5.2

#export CXXFLAGS = -Wall -O3 -fPIC -DPIC -g -march=pentium4
export CXXFLAGS += -Wall -O3 -fPIC -DPIC -DVLEVEL_VERSION=\"$(VERSION)\"

# On my system, ICC is quite a bit faster, with these options:
#export CC=icc
#export CXX=icc
#export LD=icc
#export CXXFLAGS = -fPIC -DPIC -g  -O3 -rcd

# This is where it will be installed
export DESTDIR := /
export PREFIX = $(DESTDIR)/usr/
export LADSPA_PATH = $(PREFIX)/lib/ladspa/

# End of user-editable options.

export CC = $(CXX)

.PHONY: all install clean

all:
	$(MAKE) -C volumeleveler all
	$(MAKE) -C vlevel-bin all
	$(MAKE) -C vlevel-ladspa all
	$(MAKE) -C vlevel-jack all

install: all
	$(MAKE) -C volumeleveler install
	$(MAKE) -C vlevel-bin install
	$(MAKE) -C vlevel-ladspa install
	$(MAKE) -C vlevel-jack install

clean:
	$(MAKE) -C volumeleveler clean
	$(MAKE) -C vlevel-bin clean
	$(MAKE) -C vlevel-ladspa clean
	$(MAKE) -C vlevel-jack clean
