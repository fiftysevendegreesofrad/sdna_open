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

//string class that uses only one pointer internally for space saving in variant
class HeapString
{
private:
	char *p;
	void fillstring(const char *other)
	{
		p = new char[strlen(other)+1];
		strcpy(p,other);
	}
	HeapString()
	{
		fillstring("");
	}
public:
	HeapString (const string s)
	{
		fillstring(s.c_str());
	}
	HeapString (HeapString const& other)
	{
		fillstring(other.p);
	}
	HeapString& operator=(HeapString const& other)
	{
		delete[] p;
		fillstring(other.p);
		return *this;
	}
	const char* c_str() const {return p;}
	~HeapString()
	{
		delete[] p;
	}
};
typedef variant<long,float,HeapString> SDNAVariant;

class Net;
struct FieldMetaData;

class SDNAPolylineDataSource
{
public:
	virtual size_t get_output_length()=0;
	virtual vector<FieldMetaData> get_field_metadata()=0;
	virtual vector<SDNAVariant> get_outputs(long arcid)=0;
	virtual Net* getNet()=0;
};