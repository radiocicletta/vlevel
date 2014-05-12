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

// vlevel-ladspa.h - for the LADSPA plugin

#ifndef VLEVEL_LADSPA_H
#define VLEVEL_LADSPA_H

#include <sys/types.h>

#include "../volumeleveler/volumeleveler.h"

#include "ladspa.h"

typedef LADSPA_Data value_t;

#define CONTROL_PORT_COUNT 6

#define CONTROL_PORT_LOOK_AHEAD 0
#define CONTROL_PORT_STRENGTH 1
#define CONTROL_PORT_USE_MAX_MULTIPLIER 2
#define CONTROL_PORT_MAX_MULTIPLIER 3
#define CONTROL_PORT_UNDO 4
#define CONTROL_PORT_OUTPUT_MULTIPLIER 5
#define AUDIO_PORT_INPUT_1 6
#define AUDIO_PORT_OUTPUT_1 7
#define AUDIO_PORT_INPUT_2 8
#define AUDIO_PORT_OUTPUT_2 9

#define UID_MONO 1981
#define UID_STEREO 1982

class VLevelInstance {
 public:
	VLevelInstance(size_t channels, unsigned long rate);
	~VLevelInstance();
	void ConnectPort(unsigned long port, value_t *data_location);
	void Activate();
	void Run(unsigned long sample_count);
	void Deactivate();
 private:
	VolumeLeveler vl;
	size_t nch;
	value_t **ports;
	value_t **in;
	value_t **out;  
	unsigned long sample_rate;
};

LADSPA_Handle Instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate);
void ConnectPort(LADSPA_Handle instance, unsigned long port, value_t *data_location);
void Activate(LADSPA_Handle instance);
void Run(LADSPA_Handle instance, unsigned long sample_count);
void Deactivate(LADSPA_Handle instance);
void Cleanup(LADSPA_Handle instance);

#endif // ndef VLEVEL_LADSPA_H
