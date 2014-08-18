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

// volumeleveler.cpp - defines the VolumeLeveler class

#include <sys/types.h>
#include <assert.h>
#include <math.h>

#include "volumeleveler.h"

using namespace std;

VolumeLeveler::VolumeLeveler(size_t l, size_t c,  value_t s, value_t m)
{
	bufs = 0;
	SetSamplesAndChannels(l, c);
	SetStrength(s);
	SetMaxMultiplier(m);
}

VolumeLeveler::~VolumeLeveler()
{
	for(size_t ch = 0; ch < channels; ++ch)
		delete [] bufs[ch];
	delete [] bufs;
}

void VolumeLeveler::SetStrength(value_t s)
{
	strength = s;
}

void VolumeLeveler::SetMaxMultiplier(value_t m)
{
	if(m <= 0) m = HUGE_VAL;
	max_multiplier = m;
}

void VolumeLeveler::SetSamplesAndChannels(size_t s, size_t c)
{
	assert(s > 1 && c > 0);
	
	if(bufs) {
		for(size_t ch = 0; ch < channels; ++ch)
			delete [] bufs[ch];
		delete [] bufs;
	}
	
	bufs = new value_t*[c];
	
	for(size_t ch = 0; ch < c; ++ch)
		bufs[ch] = new value_t[s];
	
	samples = s;
	channels = c;
	Flush();
}

void VolumeLeveler::Flush()
{
	for(size_t ch = 0; ch < channels; ++ch)
		for(size_t i = 0; i < samples; ++i)
			bufs[ch][i] = 0;
	
	silence = samples;
	pos = max_slope_pos = 0;
	max_slope = max_slope_val = avg_amp = 0;
}

value_t VolumeLeveler::GetMultiplier()
{
	value_t multiplier = pow(avg_amp, -strength);
	if(multiplier > max_multiplier) multiplier = max_multiplier;
	return multiplier;
}

size_t VolumeLeveler::Exchange(value_t **in_bufs, value_t **out_bufs, size_t in_samples)
{
	switch(channels) {
		//case 1:
		//  Exchange_1(in_bufs, out_bufs, in_samples);
		//  break;
		//case 2:
		//  Exchange_2(in_bufs, out_bufs, in_samples);
		//  break;
	default:
		Exchange_n(in_bufs, out_bufs, in_samples);
	}
	
	if(silence >= in_samples) {
		silence -= in_samples;
		return in_samples;
	} else {
		size_t returned_silence = silence;
		silence = 0;
		return returned_silence;
	}
}

void VolumeLeveler::Exchange_n(value_t **in_bufs, value_t **out_bufs, size_t in_samples)
{
	// for each user_pos in user_buf
	for(size_t user_pos = 0; user_pos < in_samples; ++user_pos) {
		
		// compute multiplier
		value_t multiplier = pow(avg_amp, -strength);
		
		// if avg_amp <= 0, then the above line sets multiplier to Inf, so
		// samples will scale to Inf or NaN.  This causes a tick on the
		// first sample after a Flush() unless max_multiplier is not Inf.
		// hopefully this fix isn't too slow.
		if(unlikely(avg_amp <= 0)) multiplier = 0;

		// untested!
		// The advantage of using floats is that you can be
		// sloppy with going over 1.  Since we have this nifty
		// average_amp calculation, let's apply it to limit
		// the audio to varying normally below 1.  Again,
		// hopefully this won't slow things down too much.
		if(unlikely(avg_amp > 1)) multiplier = 1 / avg_amp;
		
		// limit multiplier to max_multiplier.  max_multiplier can be Inf
		// to disable this.
		if(unlikely(multiplier > max_multiplier)) multiplier = max_multiplier;
		
		// swap buf[pos] with user_buf[user_pos], scaling user[buf] by
		// multiplier and finding max of the new sample
		value_t new_val = 0;
		for(size_t ch = 0; ch < channels; ++ch) {
			value_t in = in_bufs[ch][user_pos];
			out_bufs[ch][user_pos] = bufs[ch][pos] * multiplier;
			bufs[ch][pos] = in;
			if(VLEVEL_ABS(in) > new_val) new_val = VLEVEL_ABS(in);
		}
		
		pos = (pos + 1) % samples; // now pos is the oldest, new one is pos-1
		
		avg_amp += max_slope;
		
		if(unlikely(pos == max_slope_pos)) {
			// recompute (this is expensive)
			max_slope = -HUGE_VAL;
			for(size_t i = 1; i < samples; ++i) {
				value_t sample_val = 0;
				for(size_t ch = 0; ch < channels; ++ch) {
					value_t ch_val = VLEVEL_ABS(bufs[ch][(pos + i) % samples]);
					if(ch_val > sample_val) sample_val = ch_val;
				}
				value_t slope = (sample_val - avg_amp) / i;
				// must be >=, otherwise clipping causes excessive computation
				// TODO: maybe optimize - just save i, then compute slope, pos, and val only once later.
				// maybe unlikely()
				if(unlikely(slope >= max_slope)) { 
					max_slope_pos = (pos + i) % samples;
					max_slope = slope;
					max_slope_val = sample_val;
				}
			}
		} else {
			// only chance of higher slope is the new sample
			
			// recomputing max_slope isn't really necessary...
			max_slope = (max_slope_val - avg_amp) / ((max_slope_pos - pos + samples) % samples);
			// ...but it doesn't take long and has a small effect.

			value_t slope = (new_val - avg_amp) / (samples - 1);

			// probably needs to be >= for same reason as above
			// maybe unlikely()
			if(unlikely(slope >= max_slope)) {
				max_slope_pos = (pos - 1) % samples;
				max_slope = slope;
				max_slope_val = new_val;
			}
		}
	}
}

// this code has been proven correct, but not tested much.  ;-)
void ToValues(char *in, value_t *out, size_t values,
              size_t bits_per_value, bool has_sign)
{
	switch(bits_per_value) {
    case 32:
		if(has_sign) {
			for(size_t i = 0; i < values; ++i)
				out[i] = ((value_t)((int32_t *)in)[i]) / 2147483648;
		} else {
			for(size_t i = 0; i < values; ++i)
				out[i] = (((value_t)((u_int32_t *)in)[i]) - 2147483648) / 2147483648;
		}
		break;
	case 16:
		if(has_sign) {
			for(size_t i = 0; i < values; ++i)
				out[i] = ((value_t)((int16_t *)in)[i]) / 32768;
		} else {
			for(size_t i = 0; i < values; ++i)
				out[i] = (((value_t)((u_int16_t *)in)[i]) - 32768) / 32768;
		}
		break;
	case 8:
		if(has_sign) {
			for(size_t i = 0; i < values; ++i)
				out[i] = ((value_t)((int8_t *)in)[i]) / 128;
		} else {
			for(size_t i = 0; i < values; ++i)
				out[i] = (((value_t)((u_int8_t *)in)[i]) - 128) / 128;
		}
		break;
	default:
		assert(false);
	}
}


// note: no clipping, just wrap.  I don't know how badly clipping will perform.
void FromValues(value_t *in, char *out, size_t values,
                size_t bits_per_value, bool has_sign)
{
	switch(bits_per_value) {
	case 32:
		if(has_sign) {
			for(size_t i = 0; i < values; ++i)
				((int32_t *)out)[i] = (int32_t)(in[i] * 2147483647);
		} else {
    	for(size_t i = 0; i < values; ++i)
				((u_int32_t *)out)[i] = (u_int32_t)((in[i] * 2147483647) + 2147483647);
		}
		break;
	case 16:
		if(has_sign) {
			for(size_t i = 0; i < values; ++i)
				((int16_t *)out)[i] = (int16_t)(in[i] * 32767);
		} else {
    	for(size_t i = 0; i < values; ++i)
				((u_int16_t *)out)[i] = (u_int16_t)((in[i] * 32767) + 32767);
		}
		break;
	case 8:
		if(has_sign) {
			for(size_t i = 0; i < values; ++i)
				((int8_t *)out)[i] = (int8_t)(in[i] * 127);
		} else {
      for(size_t i = 0; i < values; ++i)
				((u_int8_t *)out)[i] = (u_int8_t)((in[i] * 127) + 127);
		}
		break;
	default:
		assert(false);
	}
}

