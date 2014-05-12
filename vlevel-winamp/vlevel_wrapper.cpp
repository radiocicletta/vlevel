///////////////////////////////////////////////////////////////////////////////
//
// vlevel_wrapper.cpp - this file is part of vlevel winamp plugin 0.1
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

#include <new>
#include <assert.h>

#include "..\\volumeleveler\\volumeleveler.h"
#include "vlevel_wrapper.h"

CVLWrapper::CVLWrapper():
	ms_samples(44100),
	ms_channels(2),
	mv_strength(.8),
	mv_maxMultiplier(25),
	mv_length(1),
	ms_rate(44100),
	mb_samplesOrChannelsChanged(false),
	mb_strengthChanged(false),
	mb_maxMultiplierChanged(false)
{
	mpvl_wrapped = new VolumeLeveler(ms_samples, ms_channels, mv_strength, mv_maxMultiplier);
	
	// I'm surprised this isn't automatic.
	if(!mpvl_wrapped)
		throw std::bad_alloc();
} // CVLWrapper::CVLWrapper

CVLWrapper::~CVLWrapper()
{
	delete mpvl_wrapped;
	// TODO: when I cache the buffer allocations, I must delete them here.
}

void CVLWrapper::SetCachedChannels(size_t s_channels)
{
	assert(s_channels > 0);
	if(ms_channels != s_channels) {
		ms_channels = s_channels;
		mb_samplesOrChannelsChanged = true;
	}
} // CVLWrapper::SetCachedChannels

void CVLWrapper::SetCachedStrength(value_t v_strength)
{	
	mv_strength = v_strength;
	mb_strengthChanged = true;
} // CVLWrapper::SetCachedStrength

void CVLWrapper::SetCachedMaxMultiplier(value_t v_maxMultiplier)
{	
	mv_maxMultiplier = v_maxMultiplier;
	mb_maxMultiplierChanged = true;
} // CVLWrapper::SetCachedMaxMultiplier

void CVLWrapper::SetCachedLength(value_t v_length)
{
	mv_length = v_length;
	SetCachedSamples(GetCachedLength() * GetCachedRate());
} // CVLWrapper::SetCachedLength

void CVLWrapper::SetCachedRate(size_t s_rate)
{
	ms_rate = s_rate;
	ms_samples = mv_length * ms_rate;
	
	SetCachedSamples(GetCachedLength() * GetCachedRate());
} // CVLWrapper::SetCachedRate

// Don't call this unless you've just computed s_amples from mv_length and ms_rate.
void CVLWrapper::SetCachedSamples(size_t s_samples)
{	
	assert(s_samples > 1);
	if(ms_samples != s_samples) {
		ms_samples = s_samples;
		mb_samplesOrChannelsChanged = true;
	}
} // CVLWrapper::SetCachedSamples

void CVLWrapper::CacheFlush()
{
	if( mb_samplesOrChannelsChanged)
	{
		mpvl_wrapped->SetSamplesAndChannels(ms_samples, ms_channels);
		mb_samplesOrChannelsChanged = false;
	} // if

	if( mb_strengthChanged )
	{
		mpvl_wrapped->SetStrength(mv_strength);
		mb_strengthChanged = false;
	} // if

	if(mb_maxMultiplierChanged)
	{
		mpvl_wrapped->SetMaxMultiplier(mv_maxMultiplier);
		SetCachedMaxMultiplier(mpvl_wrapped->GetMaxMultiplier()); // turns negative mms to HUGE_VAL
		mb_maxMultiplierChanged = false;
	} // if
} // CVLWrapper::CacheFlush

int CVLWrapper::Exchange(void *raw_buf, int samples, int bits_per_value, int channels, int rate)
{
	SetCachedChannels(channels);
	SetCachedRate(rate);
	CacheFlush();
	
	size_t values = samples * channels;
	
	// TODO: cache these allocations for performance.
	// This buffer holds interleaved value_t data.
	value_t *raw_value_buf = new value_t[values];
	// Allocate our per-channel value_t buffers.
	// This is the format VolumeLeveler requires.
	value_t **bufs = new value_t*[channels];
	for(size_t ch = 0; ch < channels; ++ch)
		bufs[ch] = new value_t[samples];
	
	// Take data from supplied integer raw_buf to allocated value_t interleaved raw_value_buf.
	ToValues((char *)raw_buf, raw_value_buf, values, bits_per_value, true); // true because data is signed.
	
	// De-interleave the data.
	for(size_t s = 0; s < samples; ++s)
		for(size_t ch = 0; ch < channels; ++ch)
			bufs[ch][s] = raw_value_buf[s * channels + ch];
	
	// Perform the effect.
	// silence_samples is how many samples at the beginning of the returned buffer
	// are silent because the real data hasn't worked through VLevel's buffer yet.
	size_t silence_samples = mpvl_wrapped->Exchange(bufs, bufs, samples);
	
	size_t good_samples = samples - silence_samples;
	size_t good_values = good_samples * channels;

	// Re-interleave the data.
	// Visual C++ improperly lets the s defined above persist outside that loop.
	// Notice that leading silence_samples are stripped and good data is shifted.
	for(/* size_t */ s = 0; s < good_samples; ++s)
		for(size_t ch = 0; ch < channels; ++ch)
			raw_value_buf[s * channels + ch] = bufs[ch][s + silence_samples];
	
	// Put the good data back into the supplied integer buffer.
	FromValues(raw_value_buf, (char *)raw_buf, good_values, bits_per_value, true);
	
	// Deallocate our buffers.
	for(/* size_t */ ch = 0; ch < channels; ++ch)
		delete [] bufs[ch];
	delete [] bufs;
	delete [] raw_value_buf;

	// Winamp is sloppy about using int when size_t is correct.  Oh, well.	
	// dsp.h says we shouldn't return less than half as many samples as we're given,
	// but returning 0 seems to work fine.	
	return good_samples;	
} // CVLWrapper::Exchange

