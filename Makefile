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

export CXXFLAGS = -Wall -O3 -fPIC -DPIC -g -march=pentium4

# On my system, ICC is quite a bit faster, with these options:
#export CC=icc
#export CXX=icc
#export LD=icc
#export CXXFLAGS = -fPIC -DPIC -g  -O3 -rcd

# This is where it will be installed
export PREFIX = /usr/local/
export LADSPA_PATH = $(PREFIX)/lib/ladspa/

# End of user-editable options.


# Note: this probably isn't the best way to have one makefile for
# source in several directories.  Someday I'll figure out automake.
# Writing Makefiles always makes me feel like I'm reinventing the
# wheel.

# This is evil, but it makes implicit link rules use g++, not gcc
export CC = $(CXX)

.PHONY: all install clean

all:
	make -C volumeleveler all
	make -C vlevel-bin all
	make -C vlevel-ladspa all

install: all
	make -C volumeleveler install
	make -C vlevel-bin install
	make -C vlevel-ladspa install

clean:
	make -C volumeleveler clean
	make -C vlevel-bin clean
	make -C vlevel-ladspa clean
