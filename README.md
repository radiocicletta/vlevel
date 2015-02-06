#VLevel 0.5

##What is VLevel?

VLevel is a tool to amplify the soft parts of music so you don't
have to fiddle with the volume control.  It looks ahead a few
seconds, so it can change the volume gradually without ever
clipping. Because the volume is changed gradually, "dynamic
contrast" is preserved.

**This repository is a fork of the original vlevel code** since
the last release of the original source is from 2004.
Although this, the code is robust and it works pretty much
unmodified nowadays. We imported form the old svn repository and
added a new module in order to use it as a JACK client. After several
months of test in a working production (a 24/7 broadcasting environment)
we can confirm the software is reliable.

##What are the supported platform?

We known VLevel to works in Linux and OSX Environments. We cannot at this moment 
test it on other platform, but we encourage you to collaborate

##How do I install VLevel?
  
See the file INSTALL.  For the impatient: `sudo make install`.

##How do I use VLevel?

Original VLevel is a filter, meaning you pipe raw data to it, and it outputs
the leveled data.  For example:

    vlevel-bin < in.cdda > out.cdda

There are options to control the length of the look-ahead buffer,
the strength of the effect, and the maximum amplification, as well
as the format of the raw data.  Type "vlevel-bin --help" for
details.

Vlevel works also as a JACK client. for Example:

    vlevel-jack --length 22050 --max-multiplier 20 --strength 0.8

will create 2 capture ports and 2 playback ports on the JACK graph
that can be used in combination with any other JACK client.

VLevel also works as a LADSPA plugin. See http://www.ladspa.org for
a lists of hosts that VLevel can plug into.

##What other features are planed?

a Lv2 Plugin and a GUI (especially for vlevel-jack)

##Can I distribute VLevel?

Please do.  VLevel is licenced under the GPL, for more information,
see the COPYING file.

##Where can I get more info?

There is documentation in the docs directory. Documentation, 
help, and more are available from the website: http://vlevel.sourceforge.net

Also support http://www.radiocicletta.it, our beloved webradio :3

