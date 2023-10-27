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

// vlevel-ladspa.cpp - the ladspa plugin

#include "../volumeleveler/volumeleveler.h"
#include "ladspa.h"
#include "vlevel-ladspa.h"

using namespace std;

// Is there a reason this must be allocated?  It seems to work without it.

// Why not just a LADSPA_Port struct with names and hints?

LADSPA_PortDescriptor vlevel_port_descriptors[] = {
	LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL,
	LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
	LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
	LADSPA_PORT_INPUT | LADSPA_PORT_AUDIO,
	LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

const char *vlevel_port_names[] = {
	"Look-ahead (seconds)",
	"Strength",
	"Limit Multiplier",
	"Multiplier Limit",
	"Undo",
	"Current Multiplier",
	"Input 1",
	"Output 1",
	"Input 2",
	"Output 2"
};

// Why can't I just specify the default, instead of _LOW _HIGH masks?

LADSPA_PortRangeHint vlevel_port_range_hints[] = {
	{
		LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
		LADSPA_HINT_DEFAULT_MIDDLE,
		0, 5
	},
	{
		LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
		LADSPA_HINT_DEFAULT_HIGH,
		0, 1
	},
	{
		LADSPA_HINT_TOGGLED |
		LADSPA_HINT_DEFAULT_1,
		0, 0
	},
	{
		LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE |
		LADSPA_HINT_DEFAULT_MIDDLE,
		0, 20    
	},
	{
		LADSPA_HINT_TOGGLED |
		LADSPA_HINT_DEFAULT_0,
		0, 0
	},
	{
		LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE,
		0, 20  
	},
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 },
	{ 0, 0, 0 }
};

LADSPA_Descriptor vlevel_descriptor_mono = {
	// UniqueID
	UID_MONO,
	// Label
	"vlevel_mono",
	// Properties
	0,
	// Name
	"VLevel (Mono)",
	// Maker
	"Tom Felker",
	// Copyright
	"GPL",
	// PortCount
	CONTROL_PORT_COUNT + 2,
	// PortDescriptors
	vlevel_port_descriptors,
	// PortNames
	vlevel_port_names,
	// PortRangeHints
	vlevel_port_range_hints,
	// ImplementationData
	0,
	// instantiate
	Instantiate,
	// connect_port
	ConnectPort,
	// activate
	Activate,
	// run
	Run,
	// run_adding
	0,
	// set_run_adding_gain
	0,
	// deactivate
	Deactivate,
	// cleanup
	Cleanup
};


LADSPA_Descriptor vlevel_descriptor_stereo = {
	// UniqueID
	UID_STEREO,
	// Label
	"vlevel_stereo",
	// Properties
	0,
	// Name
	"VLevel (Stereo)",
	// Maker
	"Tom Felker",
	// Copyright
	"GPL",
	// PortCount
	CONTROL_PORT_COUNT + 4,
	// PortDescriptors
	vlevel_port_descriptors,
	// PortNames
	vlevel_port_names,
	// PortRangeHints
	vlevel_port_range_hints,
	// ImplementationData
	0,
	// instantiate
	Instantiate,
	// connect_port
	ConnectPort,
	// activate
	Activate,
	// run
	Run,
	// run_adding
	0,
	// set_run_adding_gain
	0,
	// deactivate
	Deactivate,
	// cleanup
	Cleanup
};

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
	switch(index) {
	case 0:
		return &vlevel_descriptor_mono;
	case 1:
		return &vlevel_descriptor_stereo;
	default:
		return 0;
	}
}


VLevelInstance::VLevelInstance(size_t channels, unsigned long rate):
	vl(2, channels), nch(channels), sample_rate(rate)
{
	ports = new value_t*[CONTROL_PORT_COUNT + 2 * nch];
	in = new value_t*[nch];
	out = new value_t*[nch];
}

VLevelInstance::~VLevelInstance()
{
	delete [] ports;
	delete [] in;
	delete [] out;
}
void VLevelInstance::ConnectPort(unsigned long port, value_t *data_location)
{
	ports[port] = data_location;
	
	if(port >= CONTROL_PORT_COUNT) { // is a control port
		if((port - CONTROL_PORT_COUNT) % 2 == 0) // is an input port
			in[(port - CONTROL_PORT_COUNT) / 2] = data_location;
		else if((port - CONTROL_PORT_COUNT) % 2 == 1) // is an output port
			out[(port - CONTROL_PORT_COUNT) / 2] = data_location;
	}
}

void VLevelInstance::Activate()
{
	vl.Flush();
}

void VLevelInstance::Run(unsigned long sample_count)
{
	
	size_t samples = (size_t) (*ports[CONTROL_PORT_LOOK_AHEAD] * sample_rate);
	if(samples != vl.GetSamples()) {
		if(samples > 60 * sample_rate) samples = 60 * sample_rate;
		if(samples < 2) samples = 2;
		vl.SetSamplesAndChannels(samples, nch);
	}
	
	if(*ports[CONTROL_PORT_USE_MAX_MULTIPLIER] > 0) {
		vl.SetMaxMultiplier(*ports[CONTROL_PORT_MAX_MULTIPLIER]);
	} else {
		vl.SetMaxMultiplier(-1);
	}
	
	value_t strength = *ports[CONTROL_PORT_STRENGTH];
	if(*ports[CONTROL_PORT_UNDO] > 0)
		strength /= (strength - 1);
	vl.SetStrength(strength);
	
	vl.Exchange(in, out, sample_count);
	
	*ports[CONTROL_PORT_OUTPUT_MULTIPLIER] = vl.GetMultiplier();
}

void VLevelInstance::Deactivate() {}

LADSPA_Handle Instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
	switch(descriptor->UniqueID) {
	case UID_MONO:
		return (LADSPA_Handle)new VLevelInstance(1, sample_rate);
	case UID_STEREO:
		return (LADSPA_Handle)new VLevelInstance(2, sample_rate);  
	}
	return 0;
}

void ConnectPort(LADSPA_Handle instance, unsigned long port, value_t *data_location)
{
	((VLevelInstance *)instance)->ConnectPort(port, data_location);
}

void Activate(LADSPA_Handle instance)
{
	((VLevelInstance *)instance)->Activate(); 
}

void Run(LADSPA_Handle instance, unsigned long sample_count)
{
	((VLevelInstance *)instance)->Run(sample_count);
}

void Deactivate(LADSPA_Handle instance)
{
	((VLevelInstance *)instance)->Deactivate();
}

void Cleanup(LADSPA_Handle instance)
{
	delete (VLevelInstance *)instance;  
}
