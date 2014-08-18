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

//  ___         _ _         _    _     _   _        
// | _ \__ _ __| (_)___  __(_)__| |___| |_| |_ __ _ 
// |   / _` / _` | / _ \/ _| / _| / -_)  _|  _/ _` |
// |_|_\__,_\__,_|_\___/\__|_\__|_\___|\__|\__\__,_|
//           La radio sempre in bolletta.

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <limits.h>
#include <assert.h>

#include <jack/jack.h>
#include <jack/ringbuffer.h>
#include <unistd.h>

#include "../volumeleveler/volumeleveler.h"
#include "commandline.h"

using namespace std;

void Help();
void LevelRaw(VolumeLeveler *, unsigned int);

#define MAX_PORTS 2 
typedef jack_default_audio_sample_t sample_t;

jack_port_t       *input_port  [MAX_PORTS];
jack_port_t       *output_port [MAX_PORTS];
jack_ringbuffer_t *ring        [MAX_PORTS];

size_t sample_size = sizeof(jack_default_audio_sample_t);
size_t jack_opened_ports = 0;

// Options
size_t          CHANNELS          =  2;
value_t         STRENGHT          = .8;
value_t         MAX_MULTIPLIER    = 20;

// State
char          * JACK_FRAME_BUFFER = NULL;
VolumeLeveler * LEVELER           = NULL;

void LevelRaw()
{
    size_t bits_per_value = sizeof(sample_t) * 8;

    // figure out the size of things
    size_t samples = LEVELER->GetSamples();
    size_t channels = LEVELER->GetChannels();
    size_t values = samples * channels;
    size_t bytes_per_value = sample_size;
    size_t bytes = values * bytes_per_value;

    size_t s, ch; // VC++ 5.0's scoping rules are wrong, oh well.

    // allocate our interleaved buffers
    char *raw_buf = new char[bytes];
    value_t *raw_value_buf = new value_t[values];

    // allocate our per-channel buffers
    value_t **bufs = new value_t*[channels];
    value_t **bufs_out = new value_t*[channels];
    //value_t *bufs[channels];

    // how much data in the buffer is good
    size_t good_values, good_samples;
    // how much from the front of the buffer should be ignored
    size_t silence_values, silence_samples;
    
    // read and convert to value_t
    char *buf_in = (char *) malloc(bytes);
    for (int i = 0; i < channels; i++){
        good_values = jack_ringbuffer_read_space(ring[i]);
        good_samples = good_values; // sizeof(sample_t); //good_values / channels;
        bufs[i] = new value_t[samples];
        bufs_out[i] = new value_t[samples];
        jack_ringbuffer_read(ring[i], buf_in, good_values);
        for (size_t s = 0; s < good_values; s++){
            raw_buf[s] = *(buf_in + s);
        }
        ToValues(raw_buf, raw_value_buf, good_values, bits_per_value, true);
        for (size_t s = 0; s < good_values; s++)
            bufs[i][s] = raw_value_buf[s];
    }


    silence_samples = LEVELER->Exchange(bufs, bufs_out, good_values/sizeof(sample_t) / channels);


    // write the data
    sample_t *out;
    char *buf_out = (char *) malloc(bytes);
    for (int i = 0; i < channels; i++){
        for (size_t s = 0; s < good_values; s++)
            raw_value_buf[s] = bufs[i][s];
        FromValues(raw_value_buf, raw_buf, good_samples, bits_per_value, true);
        out = (jack_default_audio_sample_t *) jack_port_get_buffer(output_port[i], good_values);
        for (size_t s = 0; s < good_values; s++)
            *(buf_out + s) = raw_buf[s];
        memcpy(out, buf_out, good_values);
    }

    delete [] raw_value_buf;
    delete [] raw_buf;
    free(buf_in);
    free(buf_out);
    for(ch = 0; ch < channels; ch++) {
        delete [] bufs[ch];
    }
    delete [] bufs;

}



int jack_buffer_size_change_callback(jack_nframes_t nframes, void *arg)
{
    if (JACK_FRAME_BUFFER)
        free(JACK_FRAME_BUFFER);

    if (LEVELER)
        delete LEVELER;

    JACK_FRAME_BUFFER = (char *) malloc(sizeof(sample_t) * nframes);

    jack_nframes_t bufsize = nframes * sizeof(value_t) * CHANNELS * 2;
    LEVELER = new VolumeLeveler(bufsize,
                                CHANNELS,
                                STRENGHT,
                                MAX_MULTIPLIER);


    return (JACK_FRAME_BUFFER != NULL);
}

int callback_jack(jack_nframes_t nframes, void *arg)
{
    size_t channels = jack_opened_ports / 2;
    sample_t *in[channels];

    for (int i = 0; i < channels ; i++)
    {
        in[i] = (sample_t *) jack_port_get_buffer(input_port[i], nframes);
        memcpy(JACK_FRAME_BUFFER, in[i], sizeof(sample_t) * nframes);
        jack_ringbuffer_write(ring[i], JACK_FRAME_BUFFER,  sizeof(sample_t) * nframes);
    }

    LevelRaw();

    return 0;
}

void jack_shutdown(void *arg)
{
    cout << "Jack server shutdown" << endl;
    exit(0);
}

void Help()
{
    cerr << "VLevel v0.5 JACK edition" << endl
         << endl
         << "usage:" << endl
         << "\tvlevel-bin [options] < infile > outfile" << endl
         << endl
         << "options: (abbreviations also work)" << endl
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
    bool undo = false;
    string option, argument;
    const char *jack_name = "vlevel";

    while(option = cmd.GetOption(), !option.empty()) {
        
        if(option == "channels" || option == "c") {
            if((istringstream(cmd.GetArgument()) >> CHANNELS).fail()) {
                cerr << cmd.GetProgramName() << ": bad or no option for --channels" << endl;
                return 2;
            }
            if(CHANNELS < 1) {
                cerr << cmd.GetProgramName() << ": --channels must be greater than 0" << endl;
                return 2;
            }
        } else if(option == "strength" || option == "s") {
            if((istringstream(cmd.GetArgument()) >> STRENGHT).fail()) {
                cerr << cmd.GetProgramName() << ": bad or no option for --strength" << endl;
                return 2;
            }
            if(STRENGHT < 0 || STRENGHT > 1) {
                cerr << cmd.GetProgramName() << ": --strength must be between 0 and 1 inclusive." << endl;
                return 2;
            }
        } else if(option == "max-multiplier" || option == "m") {
            if((istringstream(cmd.GetArgument()) >> MAX_MULTIPLIER).fail()) {
                cerr << cmd.GetProgramName() << ": bad or no option for --max-multiplier" << endl
                     << cmd.GetProgramName() << ": for no max multiplier, give a negative number" << endl;
                return 2;
            }
        } else if(option == "undo" || option == "u") {
            undo = true;
        } else if(option == "help" || option == "h") {
            Help();
            return 0;
        } else {
            cerr << cmd.GetProgramName() << ": unrecognized option " << option << endl;
            Help();
            return 2;
        }
    }
    
    // This works, see docs/technical.txt
    if(undo) STRENGHT = STRENGHT / (STRENGHT - 1);
    
    cerr << "Beginning VLevel with:" << endl
         << "length: " << length << endl
         << "channels: " << CHANNELS << endl
         << "strength: " << STRENGHT << endl
         << "max_multiplier: " << MAX_MULTIPLIER << endl;
    
    jack_client_t* client = NULL;
    
    if ((client = jack_client_open(jack_name, JackNullOption, NULL)) == 0) {
        cerr << "jack server not running?" << endl;
        return 1;
    }

    if (jack_set_buffer_size_callback(client, jack_buffer_size_change_callback, NULL) == 0) {
        cerr << "failed to set buffer size callback" << endl;
        return 1;
    }

    jack_on_shutdown (client, jack_shutdown, NULL);

    for (int i = 0; i < CHANNELS; i++){
        char out[256], in[256];
        sprintf(in, "capture_%d", i+1);
        sprintf(out, "playback_%d", i+1);
        input_port[i] = jack_port_register (client, in, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
        output_port[i] = jack_port_register (client, out, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
        ring[i] = jack_ringbuffer_create(sample_size * length);
        memset(ring[i]->buf, 0, ring[i]->size);
        jack_opened_ports += 2;
    }

    if (jack_set_process_callback(client, callback_jack, NULL)) {
        cerr << "cannot set process callback" << endl;
        return 1;
    }

    if (jack_activate (client)) {
        cerr << "cannot activate client" << endl;
        return 1;
    }

    sleep(-1);
    return 0;
}
