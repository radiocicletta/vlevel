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

#include "vlevel.h"

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
