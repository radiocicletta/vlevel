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

// volumeleveler.h - declares the VolumeLeveler class
//
// A note on terminology: atoms are indivual value_t values; samples
// are one or more atoms, for example, for stereo, a sample has 2
// atoms, and channels is 2.

#ifndef VOLUMELEVELER_H
#define VOLUMELEVELER_H

#include <sys/types.h>

typedef float value_t;

#define VLEVEL_ABS(x) fabsf(x)

// same speed as above
//#define VLEVEL_ABS(x) ((x) > 0 ? (x) : -(x))

// a bit faster on 2.96, a bit slower(!) on 3.2
//#define VLEVEL_ABS(x) (x)

// TODO: VLEVEL_MAX? or maybe that's not worth it


// Branch Prediction:
//
// NOTE: for some strange reason, doing nothing is fastest, my
// hand-tuned unlikely() macros is slightly slower, and letting
// -fprofile-arcs and -fbranch-probabilities help the optimization is
// slowest yet.  This is the exact opposite of what I'd expect.  Hmm.
// I wish somebody would optimize GCC up to the speed of ICC.
//
// XXX: should be only for GCC versions that support this:
// taken from linux/include/linux/compiler.h
#ifdef EXPECT
#warning using tweaks
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

// Converts from an integer format to value_t
// values is the number of values in out, in needs values*bits/8 bytes.
void ToValues(char *in, value_t *out, size_t values,
              size_t bits_per_value, bool has_sign);

// Converts from value_t to an integer format
// values is the number of values in in, out needs values*bits/8 bytes.
void FromValues(value_t *in, char *out, size_t values,
                size_t bits_per_value, bool has_sign);

class VolumeLeveler {
 public:

	// constructs and destructs a VolumeLeveler with a length of l
	// samples with c channels each, an effect strength of s and a
	// maximum multiplier of m
	VolumeLeveler(size_t l = 44100, size_t c = 2, value_t s = .8, value_t m = 25);
	~VolumeLeveler();

	// Reallocates a buffer of l samples and c channels (contents are
	// lost)
	void SetSamplesAndChannels(size_t l, size_t c);

	// set and get the strength (between 0 and 1) (set doesn't affect
	// the buffer) if undo is true, vlevel will do the exact opposite,
	// so you can remove a vlevel.
	void SetStrength(value_t s);

	// set and get the max multiplier (set doesn't affect the buffer)
	void SetMaxMultiplier(value_t m);

	// get stuff
	inline size_t GetSamples() { return samples; };
	inline size_t GetChannels() { return channels; };
	inline value_t GetStrength() { return strength; };
	inline value_t GetMaxMultiplier() { return max_multiplier; };
	inline size_t GetSilence() { return silence; };

	// get stats
	value_t GetMultiplier(); 
	inline value_t GetAverageAmp() { return avg_amp; };
	
	// fills the buffers with silence
	void Flush();
	
	// replaces raw with processed, returns how many samples are
	// residual silence from when the buffers were empty.
	size_t Exchange(value_t **in_buf, value_t **out_buf, size_t in_samples);
	
 private:
	
	//void Exchange_1(value_t **in_buf, value_t **out_buf, size_t in_samples);
	//void Exchange_2(value_t **in_buf, value_t **out_buf, size_t in_samples);
	void Exchange_n(value_t **in_buf, value_t **out_buf, size_t in_samples);
	
	// the buffer
	value_t **bufs;
	
	// the length of the buffers
	size_t samples;
	
	// the number of channels
	size_t channels;
	
	// the strength of the effect (between 0 and 1)
	value_t strength;
	
	// the maximum value by which a sample will be scaled
	value_t max_multiplier;
	
	// the amount of silence (data that wasn't input) left in the buffer (samples).
	size_t silence;
	
	// position about to be returned (samples)
	size_t pos;
	
	// position of the max slope (samples)
	size_t max_slope_pos;
	
	// the current "blanket" amplitude
	value_t avg_amp;  
	
	// the maximum slope
	value_t max_slope;
	
	// the value at the maximum slope
	value_t max_slope_val;
	
};

#endif // ndef VOLUMELEVELER_H
