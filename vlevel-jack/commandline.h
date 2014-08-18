// This file is part of CommandLine, a C++ argument processing class.
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

// commandline.h
// declares the CommandLine class

#include <string>
#include <stack>

////////////////////////
// note: ill rewrite this to use a list of strings

class CommandLine {
 public:
	// argv is a pointer to a const pointer to a const char
	CommandLine(int argc, const char * const * argv);
	
	std::string GetProgramName();
	std::string GetOption();
	std::string GetArgument();
	bool End() const;
	
 private:
	std::string program_name;
	std::stack<std::string> args;
	bool in_short;
	bool cur_is_arg;
	bool no_more_options;
};
