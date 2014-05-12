// This file is part of VLevel, a dynamic volume normalizer.
//
// Copyright 2003 Tom Felker <tcfelker@mtco.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA

// vlevel.h - included by all

#ifndef VLEVEL_H
#define VLEVEL_H

// Float actually is slightly less accurate (900/65536 in my tests),
// but is needed by LADSPA.

typedef float value_t;

#define VLEVEL_ABS(x) fabsf(x)

// same speed as above
//#define VLEVEL_ABS(x) ((x) > 0 ? (x) : -(x))

// a bit faster on 2.96, a bit slower(!) on 3.2
//#define VLEVEL_ABS(x) (x)

#endif // ndef VLEVEL_H
