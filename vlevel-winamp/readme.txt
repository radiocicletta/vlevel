vlevel winamp plugin 0.1
Copyright (C) 2003  Markus Sablatnig

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


vlevel
======
A big part of this software is the vlevel-library. VLevel is a dynamic compressor which amplifies the quiet parts of music. It uses a look-ahead buffer, so the changes are gradual and the audio never clips.
VLevel was written by Tom Felker and is distributed under the Terms of the GNU Lesser General Public License(lGPL).
More information at: www.sf.net/projects/vlevel


What does this plugin do?
=========================
It brings the benefits of vLevel to Nullsoft WinAmp.


Basics
======
Version: 	Winamp2 or higher
Platform: 	Win32
License:	GPL


Todo
====
- test conversion function in wrapper class
- add config-dialog resource
- save/restore values to/from registry(or ini) on init/quit
- write installer/uninstaller
- maybe skinning support?

Done
====
- DLL framework
- wrapper class
- reorder project files (use ..\volumeleveler; directory structure; ...)

