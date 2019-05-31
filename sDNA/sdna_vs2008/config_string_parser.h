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
using namespace boost;
#pragma once 

struct BadConfigException : public std::exception
{
public:
	string message;
	BadConfigException(string info) : message(info) {}
	const char* what() const throw()
	{
		return message.c_str();
	}
private:
	BadConfigException();
};


class ConfigStringParser
{
private:
	typedef map<string,string> configdict_t;
	configdict_t configdict;

	set<string> allowable_keywords;

	void parse_string(char *config)
	{
		string s(config);
		vector<string> config_items;
		set<string> seen;
		split( config_items, s, is_any_of(";") );

		for (vector<string>::iterator it=config_items.begin(); it!=config_items.end(); ++it)
		{
			//split on first = only
			vector<string> item;
			size_t pos = it->find("=");
			if (pos!=string::npos)
			{
				item.push_back(it->substr(0,pos));
				item.push_back(it->substr(pos+1));
			}
			else
				item.push_back(*it);
			
			if (item.size()==0)
				continue;

			string &keyword=item[0];
			erase_all(keyword," ");
			to_lower(keyword);

			if (keyword=="")
				continue;

			if (seen.find(keyword) != seen.end())
				throw BadConfigException("Keyword "+keyword+" specified multiple times, or set in both interface and advanced config");

			seen.insert(keyword);

			if (allowable_keywords.find(keyword) == allowable_keywords.end())
				throw BadConfigException("Unrecognised config keyword: '" + keyword +"'");
			
			switch (item.size())
			{
			case 2:
				//name/value pair
				{
					string value = item[1];
					erase_all(value," ");
					configdict[keyword] = value;
				}
				break;
			case 1:
				//boolean switch
				configdict[keyword] = "true";
				break;
			default:
				throw BadConfigException("Syntax error in config");
			}
		}
	}

	void setup_allowable(char *allowable)
	{
		vector<string> a;
		split(a, allowable, is_any_of(";"));
		for (vector<string>::iterator it=a.begin(); it!=a.end(); ++it)
		{
			to_lower(*it);
			allowable_keywords.insert(*it);
		}
	}

public:
	ConfigStringParser(char *allowable_keywords,char *defaults,char *config)
	{
		setup_allowable(allowable_keywords);
		parse_string(defaults);
		parse_string(config);
	}

	bool get_bool(string s)
	{
		string value = get_string(s);
		to_lower(value);
		if (value=="0" || value=="false" || value=="f")
			return false;
		if (value=="1" || value=="true" || value=="t")
			return true;
		throw BadConfigException("Bad boolean value for config parameter "+s);
	}

	string get_string(string s)
	{
		configdict_t::iterator result = configdict.find(s);
		if (result != configdict.end())
			return result->second;
		else
			throw BadConfigException("Missing config keyword: "+s);
	}

	long get_long(string s)
	{
		return safe_atol(get_string(s));
	}

	double get_double(string s)
	{
		return safe_atod(get_string(s));
	}

	vector<string> get_vector(string s)
	{
		string valuestring = get_string(s);
		vector<string> values;
		split (values, valuestring, is_any_of(","));
		if (values.size()==1 && values[0]=="")
			return vector<string>();
		else
			return values;
	}

	static double safe_atod(string s)
	{
		const char *in_string = s.c_str();
		char *out_string = NULL;
		const double result = strtod(in_string,&out_string);
		if (in_string!=out_string)
			return result;
		else
			throw BadConfigException("Bad float value: "+s);
	}

	static long safe_atol(string s)
	{
		const char *in_string = s.c_str();
		char *out_string = NULL;
		const long result = strtol(in_string,&out_string,10);
		if (in_string!=out_string)
			return result;
		else
			throw BadConfigException("Bad integer value: "+s);
	}
};

