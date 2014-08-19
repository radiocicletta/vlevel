// This file is part of VLevel, a dynamic volume normalizer.

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

typedef jack_default_audio_sample_t sample_t;

size_t sample_size = sizeof(jack_default_audio_sample_t);

size_t             channels          =  2;
value_t            strength          = .8;
value_t            max_multiplier    = 20;

jack_client_t     * client           = NULL;
VolumeLeveler     * leveler          = NULL;

jack_port_t     ** input_ports       = NULL;
jack_port_t     ** output_ports      = NULL;

value_t          * input_buffers     = NULL;
value_t          * output_buffers    = NULL;

value_t         ** input_bufferlist  = NULL;
value_t         ** output_bufferlist = NULL;


int vlevel_buffer_size_change_callback(jack_nframes_t nframes, void *arg)
{
    size_t buffer_size = sizeof(value_t) * nframes;

    if (leveler)
    {
        delete leveler;

        free(input_buffers);
        free(output_buffers);
    }

    input_bufferlist  = (value_t **) calloc(channels, sizeof(value_t *));
    output_bufferlist = (value_t **) calloc(channels, sizeof(value_t *));
    input_buffers     = (value_t  *) calloc(channels, buffer_size);
    output_buffers    = (value_t  *) calloc(channels, buffer_size);

    for (int i = 0; i < channels; i++)
    {
        input_bufferlist[i]  = &input_buffers[i];
        output_bufferlist[i] = &output_buffers[i];
    }

    leveler = new VolumeLeveler(jack_get_sample_rate(client),
                                channels,
                                strength,
                                max_multiplier);

    return 0;
}

int vlevel_process_callback(jack_nframes_t nframes, void *arg)
{
    for (int i = 0; i < channels; i++)
    {
        sample_t * jack_input = (sample_t *)jack_port_get_buffer(input_ports[i], nframes);
        ToValues((char *)jack_input, &input_buffers[i], nframes, sizeof(sample_t) * 8, true);
    }

    leveler->Exchange(input_bufferlist, output_bufferlist, nframes);

    for (int i = 0; i < channels; i++)
    {
        sample_t * jack_output = (sample_t *)jack_port_get_buffer(output_ports[i], nframes);
        FromValues(&output_buffers[i], (char *)jack_output, nframes, sizeof(sample_t) * 8, true);
    }

    return 0;
}

void jack_shutdown(void *arg)
{
    cout << "Jack server shutdown" << endl;
    exit(0);
}

void vlevel_help()
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

int vlevel_parse_options(
    // in
    int                argc,
    const char *const* argv,

    // out
    size_t  * channels,
    value_t * strength,
    value_t * max_multiplier
)
{
    string option;
    string argument;

    bool        undo(false);
    CommandLine cmd(argc, argv);

    while(option = cmd.GetOption(), !option.empty()) {
        if(option == "channels" || option == "c") {
            if((istringstream(cmd.GetArgument()) >> *channels).fail()) {
                cerr << cmd.GetProgramName() << ": bad or no option for --channels" << endl;
                return 2;
            }
            if(*channels < 1) {
                cerr << cmd.GetProgramName() << ": --channels must be greater than 0" << endl;
                return 2;
            }
        } else if(option == "strength" || option == "s") {
            if((istringstream(cmd.GetArgument()) >> *strength).fail()) {
                cerr << cmd.GetProgramName() << ": bad or no option for --strength" << endl;
                return 2;
            }
            if(*strength < 0 || *strength > 1) {
                cerr << cmd.GetProgramName() << ": --strength must be between 0 and 1 inclusive." << endl;
                return 2;
            }
        } else if(option == "max-multiplier" || option == "m") {
            if((istringstream(cmd.GetArgument()) >> *max_multiplier).fail()) {
                cerr << cmd.GetProgramName() << ": bad or no option for --max-multiplier" << endl
                     << cmd.GetProgramName() << ": for no max multiplier, give a negative number" << endl;
                return 2;
            }
        } else if(option == "undo" || option == "u") {
            undo = true;
        } else if(option == "help" || option == "h") {
            vlevel_help();
            return 0;
        } else {
            cerr << cmd.GetProgramName() << ": unrecognized option " << option << endl;
            vlevel_help();
            return 2;
        }
    }
    
    // This works, see docs/technical.txt
    if(undo)
        *strength = *strength / (*strength - 1);

    return 0;
}

int main(int argc, char *argv[])
{
    int retval = vlevel_parse_options(argc, argv, &channels, &strength, &max_multiplier);
    if (retval != 0)
        exit(retval);

    cerr << "Beginning VLevel with:" << endl
         << "channels:       " << channels << endl
         << "strength:       " << strength << endl
         << "max_multiplier: " << max_multiplier << endl;

    if ((client = jack_client_open("vlevel", JackNullOption, NULL)) == 0) {
        cerr << "jack server not running?" << endl;
        return 1;
    }

    if (jack_set_buffer_size_callback(client, vlevel_buffer_size_change_callback, NULL)) {
        cerr << "failed to set buffer size callback" << endl;
        return 1;
    }

    jack_on_shutdown (client, jack_shutdown, NULL);

    input_ports  = (jack_port_t **)calloc(channels, sizeof(jack_port_t *));
    output_ports = (jack_port_t **)calloc(channels, sizeof(jack_port_t *));

    char out [256];
    char in  [256];

    for (int i = 0; i < channels; i++){
        sprintf(in,  "capture_%d",  i + 1);
        sprintf(out, "playback_%d", i + 1);

        input_ports[i]  = jack_port_register(client,
                                             in,
                                             JACK_DEFAULT_AUDIO_TYPE,
                                             JackPortIsInput,
                                             0);

        output_ports[i] = jack_port_register(client,
                                             out,
                                             JACK_DEFAULT_AUDIO_TYPE,
                                             JackPortIsOutput,
                                             0);
    }

    if (jack_set_process_callback(client, vlevel_process_callback, NULL)) {
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
