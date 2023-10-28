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

// vlevel-bin.cpp - the vlevel-bin command, uses VolumeLeveler

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <stdio.h>

#include <limits.h>
#include <assert.h>

#include "../volumeleveler/volumeleveler.h"
#include "commandline.h"

using namespace std;

void LevelRaw(istream &in, ostream& out, VolumeLeveler &vl, unsigned int bits_per_value);
void Help();

void LevelRaw(FILE *in, FILE *out, VolumeLeveler &vl, unsigned int bits_per_value)
{
	assert(bits_per_value % 8 == 0);
	
	// figure out the size of things
	size_t samples = vl.GetSamples();
	size_t channels = vl.GetChannels();
	size_t values = samples * channels;
	size_t bytes_per_value = bits_per_value / 8;
	size_t bytes = values * bytes_per_value;

	size_t s, ch; // VC++ 5.0's scoping rules are wrong, oh well.

	// allocate our interleaved buffers
	char *raw_buf = new char[bytes];
	value_t *raw_value_buf = new value_t[values];

	// allocate our per-channel buffers
	value_t **bufs = new value_t*[channels];
	for(ch = 0; ch < channels; ++ch)
		bufs[ch] = new value_t[samples];

	// how much data in the buffer is good
	size_t good_values, good_samples;
	// how much from the front of the buffer should be ignored
	size_t silence_values, silence_samples;
	
	while(!ferror(in) && !feof(in)) {
		// read and convert to value_t
		good_values = fread(raw_buf, bytes_per_value, values, in);
		good_samples = good_values / channels;
		ToValues(raw_buf, raw_value_buf, good_values, bits_per_value, true);

		// de-interleave the data
		for(size_t s = 0; s < good_samples; ++s)
			for(size_t ch = 0; ch < channels; ++ch)
				bufs[ch][s] = raw_value_buf[s * channels + ch];

		// do the exchange
		silence_samples = vl.Exchange(bufs, bufs, good_samples);
		silence_values = silence_samples * channels;
		good_samples -= silence_samples;
		good_values -= silence_values;

		// interleave the data
		for(s = silence_samples; s < silence_samples + good_samples; ++s)
			for(size_t ch = 0; ch < channels; ++ch)
				raw_value_buf[s * channels + ch] = bufs[ch][s];

		// write the data
		FromValues(&raw_value_buf[silence_values], raw_buf, good_values, bits_per_value, true);
		fwrite(raw_buf, bytes_per_value, good_values, out);
	}

	// silence the data
	for(s = 0; s < samples; ++s)
		for(size_t ch = 0; ch < channels; ++ch)
			bufs[ch][s] = 0;

	// exchange the data,
	silence_samples = vl.Exchange(bufs, bufs, samples);
	silence_values = silence_samples * channels;
	// good_samples = samples - silence_samples;
	good_values = values - silence_values;

	//interlace
	for(s = silence_samples; s < samples; ++s)
		for(size_t ch = 0; ch < channels; ++ch)
			raw_value_buf[s * channels + ch] = bufs[ch][s];

	FromValues(&raw_value_buf[silence_values], raw_buf, good_values, bits_per_value, true);
	fwrite(raw_buf, bytes_per_value, good_values, out);

	delete [] raw_buf;
	delete [] raw_value_buf;
	for(ch = 0; ch < channels; ++ch)
		delete [] bufs[ch];
	delete [] bufs;

}

void Help(const char *program)
{
	cerr << "VLevel v" << VLEVEL_VERSION << endl
	     << endl
	     << "usage:" << endl
	     << "\t" << program << " [options] < infile.cdda > outfile.cdda" << endl
	     << endl
	     << "options: (abbreviations also work)" << endl
	     << "\t--length num" << endl
	     << "\t\tSets the buffer to num samples long" << endl
	     << "\t\tDefault is 132300 (three seconds at 44.1kHz)" << endl
	     << "\t--channels num" << endl
	     << "\t\tEach sample has num channels" << endl
	     << "\t\tDefault is 2" << endl
	     << "\t--strength num" << endl
	     << "\t\tEffect strength, 1 is max, 0 is no effect." << endl
	     << "\t\tDefault is .8" << endl
	     << "\t--max-multiplier num" << endl
	     << "\t\tSets the maximum amount a sample will be multiplied" << endl
	     << "\t\tDefault is 20" << endl
	     << "\t--undo" << endl
	     << "\t\tReverses the effect of a previous VLevel" << endl;
}

int main(int argc, char *argv[])
{
	CommandLine cmd(argc, argv);
	size_t length = 3 * 44100;
	size_t channels = 2;
	value_t strength = .8, max_multiplier = 20;
	bool undo = false;
	string option, argument;
	
	while(option = cmd.GetOption(), !option.empty()) {
		
		if(option == "length" || option == "l") {
			if((istringstream(cmd.GetArgument()) >> length).fail()) {
				cerr << cmd.GetProgramName() << ": bad or no option for --length" << endl;
				return 2;
			}
			if(length < 2) {
				cerr << cmd.GetProgramName() << ": --length must be greater than 1" << endl;
				return 2;
			}
		} else if(option == "channels" || option == "c") {
			if((istringstream(cmd.GetArgument()) >> channels).fail()) {
				cerr << cmd.GetProgramName() << ": bad or no option for --channels" << endl;
				return 2;
			}
			if(channels < 1) {
				cerr << cmd.GetProgramName() << ": --channels must be greater than 0" << endl;
				return 2;
			}
		} else if(option == "strength" || option == "s") {
			if((istringstream(cmd.GetArgument()) >> strength).fail()) {
				cerr << cmd.GetProgramName() << ": bad or no option for --strength" << endl;
				return 2;
			}
			if(strength < 0 || strength > 1) {
				cerr << cmd.GetProgramName() << ": --strength must be between 0 and 1 inclusive." << endl;
				return 2;
			}
		} else if(option == "max-multiplier" || option == "m") {
			if((istringstream(cmd.GetArgument()) >> max_multiplier).fail()) {
				cerr << cmd.GetProgramName() << ": bad or no option for --max-multiplier" << endl
				     << cmd.GetProgramName() << ": for no max multiplier, give a negative number" << endl;
				return 2;
			}
		} else if(option == "undo" || option == "u") {
			undo = true;
		} else if(option == "help" || option == "h") {
			Help(argv[0]);
			return 0;
		} else {
			cerr << cmd.GetProgramName() << ": unrecognized option " << option << endl;
			Help(argv[0]);
			return 2;
		}
	}
	
	// This works, see docs/technical.txt
	if(undo) strength = strength / (strength - 1);
	
	cerr << "Beginning VLevel with:" << endl
	     << "length: " << length << endl
	     << "channels: " << channels << endl
	     << "strength: " << strength << endl
	     << "max_multiplier: " << max_multiplier << endl;
	
	FILE *in = stdin;
	FILE *out = stdout;
	
	argument = cmd.GetArgument();
	if(!argument.empty() && argument != "-") {
		in = fopen(argument.c_str(), "rb");
		if(!in) {
			cerr << "Couldn't open input file '" << argument << "'.\n" << endl;
			return 2;
		}
	}
	argument = cmd.GetArgument();
	if(!argument.empty() && argument != "-") {
		out = fopen(argument.c_str(), "wb");
		if(!in) {
			cerr << "Couldn't open output file '" << argument << "'.\n" << endl;
			return 2;
		}
	}
	
	VolumeLeveler l(length, channels, strength, max_multiplier);
	LevelRaw(in, out, l, 16);
	return 0;
}
