//sDNA software for spatial network analysis 
//Copyright (C) 2011-2019 Cardiff University

//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include "stdafx.h"
#pragma once
using namespace std;
using namespace boost;

class Net;
struct SDNAPolyline;


template <typename T> class NetExpectedDataSource //data we expect to appear on net
{
private:
	long index; //can hold -1 for "use default" or -2 for uninitialized
	string defaultname; //name to use if we are asked to store data and none is specified
	string name;
	T m_defaultdata;
	Net *net;
	int(__cdecl *print_warning_callback)(const char*);
	bool allow_default;
	bool allow_table;

public:
	//default constructor will just fail when it comes to get/set data
	NetExpectedDataSource() : index(-2),net(NULL),print_warning_callback(NULL) {}
	
	//behaviour: if name is non-empty, insist it must exist on init
	//if it doesn't exist on init, read from default, and write non-default to defaultname
	//currently there are no use cases that would need a default write but not read, so this isn't supported
	NetExpectedDataSource(string name,T defaultdata,Net *net,string defaultname,int(__cdecl *print_warning_callback)(const char*))
	: index(-2), m_defaultdata(defaultdata), net(net), defaultname(defaultname), name(name), allow_default(true), allow_table(false),
	print_warning_callback(print_warning_callback) {}

	//this constructor doesn't allow the data to not exist
	NetExpectedDataSource(string name,Net *net,int(__cdecl *print_warning_callback)(const char*))
		: index(-2), net(net), print_warning_callback(print_warning_callback), allow_default(false), name(name) {}
	
	bool init(); //returns false if name specified but not found
	bool using_default() { return (index==-1);} //only valid after init
	string get_name() const { return name;} //returns the name that was asked for at runtime
	virtual T get_data(const SDNAPolyline * const link);
	virtual void set_data(SDNAPolyline * const link,T d);
	bool is_initialized() { return !(index==-2); } //always must be initialized before use
	bool is_set() { return !(net==NULL); } //to see if it has been configured at all, even if not initialized
	bool is_compulsory() { return !name.empty();}
};

