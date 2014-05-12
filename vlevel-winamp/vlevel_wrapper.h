///////////////////////////////////////////////////////////////////////////////
//
// vlevel_wrapper.h - this file is part of vlevel winamp plugin 0.1
// Copyright (C) 2003 Markus Sablatnig
// Copyright (C) 2003 Tom Felker
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_VLEVEL_WRAPPER_H
#define INCLUDED_VLEVEL_WRAPPER_H

// This is now the only volumeleveler header
#include "..\\volumeleveler\\volumeleveler.h"

// This class is a wrapper for VolumeLeveler.
// It caches some settings and de-/interlaces
// Exchange()'s in and output buffers
class CVLWrapper
{
public:
	CVLWrapper();
	~CVLWrapper();

	//these automatically mark the values as 'changed'
	inline void SetCachedChannels(size_t channels); // at least 1
	inline void SetCachedStrength(value_t v_strength); // 0.0 to 1.0
	inline void SetCachedMaxMultiplier(value_t v_maxMultiplier); // negative for no limit

	// these affect the internal representation of ms_samples
	inline void SetCachedLength(value_t v_length); // in seconds
	inline void SetCachedRate(size_t s_rate); // in Hz
	
	//these Get functions might be useful	
	inline size_t GetCachedChannels() { return ms_channels; };
	inline value_t GetCachedStrength() { return mv_strength; };
	inline value_t GetCachedMaxMultiplier() { return mv_maxMultiplier; };
	
	inline value_t GetCachedLength() { return mv_length; };
	inline size_t GetCachedRate() { return ms_rate; };
	inline size_t GetCachedSamples() { return ms_samples; };
	
	// write cached values to the wrapped object
	// This is called automatically in Exchange()
	inline void CacheFlush( void );
	
	// Exchange() handles it's own de- and re-interlacing and CacheFlush()s.
	// Each sample has nch values - just use the same convention as Winamp and VLevel.
	int Exchange(void *samples, int numsamples, int bps, int nch, int rate); 
	
	
private:

	// Instead of setting this directly, use SetLength() and SetRate().
	inline void SetCachedSamples( size_t s_samples );

	// main object
	VolumeLeveler*	mpvl_wrapped;
	
	// cached values
	size_t ms_samples;
	size_t ms_channels;
	value_t mv_strength;
	value_t mv_maxMultiplier;

	// ms_samples is recomputed when these are changed.
	value_t	mv_length;
	size_t ms_rate;
	
	// Using change flags is faster and more accurate.
	// It can cause unnecessary flushes when rate and length are changed inversely,
	// but perfect is the enemy of good in this case.
	bool mb_samplesOrChannelsChanged;
	bool mb_strengthChanged;
	bool mb_maxMultiplierChanged;

}; //CVLWrapper

#endif //#ifndef VLEVEL_WRAPPER_H
