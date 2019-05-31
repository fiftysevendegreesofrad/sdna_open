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
#include "edge.h"
#include "sdna_arrays.h"
#pragma once

enum oversample_handling_t {RAW_VALUE, TAKE_SAMPLE_MEANS};

//OutputDataWrapper: root of class hierarchy for code which converts internal data structures
//into data which can be output to the GIS
//Maps on to the structures in sdna_arrays.h, in the cases where we need to output them
class OutputDataWrapper
{
public:
	virtual float get_output(SDNAPolyline* x,int oversample) = 0;
	virtual string get_name() = 0;
	virtual string get_shortname() = 0;
	virtual OutputDataWrapper* clone() = 0;
	virtual bool enabled() = 0;
};

//much of the remainder of this class hierarchy exists only to produce output wrappers that 
//divide one number by another, and name the resulting output correctly
//Overengineered?  maybe, but it's got a kind of bloody-minded thoroughness to it,
//so probably best to leave as it is unless circumstances dictate change
class SplitNameRadialOutputDataWrapper : public OutputDataWrapper
{
public:
	virtual int get_radius_index() = 0;
	virtual string get_name_prefix() = 0;
	virtual string get_name_suffix() = 0;
	virtual string get_shortname_prefix() = 0;
	virtual string get_shortname_suffix() = 0;
};

//RadialOutputDataWrapper: for any output which is calculated at multiple radii
//One wrapper holds data for one output at one radius only
class RadialOutputDataWrapper : public SplitNameRadialOutputDataWrapper
{
private:
	SDNAPolylineIdRadiusIndexed2dArrayBase *data;
	string name, suffix, shortname, shortsuffix;
	int radius_index;
	oversample_handling_t oversample_handling;
public:
	virtual string get_name_prefix() { return name;}
	virtual string get_name_suffix() { return suffix;}
	virtual string get_shortname_prefix() { return shortname;}
	virtual string get_shortname_suffix() { return shortsuffix;}
	virtual RadialOutputDataWrapper* clone() { return new RadialOutputDataWrapper(*this); }
	virtual bool enabled() {return data->is_enabled();}
	RadialOutputDataWrapper(string name_base, string shortname_base,
							SDNAPolylineIdRadiusIndexed2dArrayBase *data,int radius_index,vector<double> &radii,
							bool cont_space,string weighting,oversample_handling_t os)
				: data(data), radius_index(radius_index), oversample_handling(os)
	{
		stringstream n,sn;
		n << name_base << " ";
		sn << shortname_base;
		if (weighting.size()!=0)
		{
			n << "W" << weighting << " ";
			sn << "W" << weighting;
		}
		name = n.str();
		shortname = sn.str();

		//this code is repeated.  ideally the whole hierarchy would be refactored
		stringstream ns, sns;
		if (radii[radius_index]!=GLOBAL_RADIUS)
		{
			ns << "R" << radii[radius_index];
			sns << radii[radius_index];
		}
		else
		{
			ns << "Rn";
			sns << "n";
		}
		if (cont_space)
		{
			ns << "c";
			sns << "c";
		}
		suffix = ns.str();
		shortsuffix = sns.str();
	}
	virtual float get_output(SDNAPolyline* x,int oversample)
	{
		float retval = data->floatvalue(x->id,radius_index);
		if (oversample_handling==TAKE_SAMPLE_MEANS)
			retval /= oversample;
		return retval;
	}
	virtual string get_name()
	{
		return get_name_prefix()+get_name_suffix();
	}
	virtual string get_shortname()
	{
		return get_shortname_prefix()+get_shortname_suffix();
	}
	virtual int get_radius_index()
	{
		return radius_index;
	}
};

//ControlledRadialOutputDataWrapper takes either a RadialOutputDataWrapper, or another ControlledRadialOutputDataWrapper,
//plus some data to divide it by (Controlled is a fancy scientific term for divided by, apparently)
class ControlledRadialOutputDataWrapper : public SplitNameRadialOutputDataWrapper
{
private:
	shared_ptr<SplitNameRadialOutputDataWrapper> data;
	SDNAPolylineIdRadiusIndexed2dArrayBase *control_data;
	string control_description;
public:
	virtual string get_name_prefix() { return data->get_name_prefix() + control_description;}
	virtual string get_name_suffix() { return data->get_name_suffix();}
	virtual string get_shortname_prefix() { return data->get_shortname_prefix() + control_description;}
	virtual string get_shortname_suffix() { return data->get_shortname_suffix();}
	virtual ControlledRadialOutputDataWrapper* clone() { return new ControlledRadialOutputDataWrapper(*this);}
	virtual bool enabled() {return data->enabled() && control_data->is_enabled();}
	ControlledRadialOutputDataWrapper(RadialOutputDataWrapper &data,
									  string control_desc, SDNAPolylineIdRadiusIndexed2dArrayBase *control_data)
					: data(shared_ptr<SplitNameRadialOutputDataWrapper>(data.clone())),
					  control_data(control_data)
	{
		stringstream cd;
		cd << "C" << control_desc;
		control_description = cd.str();
	}
	ControlledRadialOutputDataWrapper(ControlledRadialOutputDataWrapper &data,
									  string control_desc, SDNAPolylineIdRadiusIndexed2dArrayBase *control_data)
					: data(shared_ptr<SplitNameRadialOutputDataWrapper>(data.clone())),
					  control_data(control_data), control_description(control_desc)
	{}
	ControlledRadialOutputDataWrapper(RadialOutputDataWrapper &data,
									  SDNAPolylineIdRadiusIndexed2dArrayBase *control_data)
							  : data(shared_ptr<SplitNameRadialOutputDataWrapper>(data.clone())),
							    control_data(control_data),
								control_description("")
	{}
	virtual float get_output(SDNAPolyline *x,int oversample)
	{
		float denom = control_data->floatvalue(x->id,data->get_radius_index());
		if (denom!=0)
			return data->get_output(x,oversample) / denom;
		else
			return 0;
	}
	virtual int get_radius_index()
	{
		return data->get_radius_index();
	}
	virtual string get_name()
	{
		string intervening_space;
		if (control_description=="")
			intervening_space = "";
		else
			intervening_space = " ";

		return get_name_prefix()+intervening_space+get_name_suffix();
	}
	virtual string get_shortname()
	{
		return get_shortname_prefix()+get_shortname_suffix();
	}
};

class HullShapeIndexWrapper : public SplitNameRadialOutputDataWrapper
{
private:
	SDNAPolylineIdRadiusIndexed2dArrayBase *area, *perim;
	int radius_index;
	string suffix, shortsuffix;
public:
	virtual string get_name_prefix() { return "Convex Hull Shape Index";}
	virtual string get_shortname_prefix() { return "HullSI";}
	virtual string get_name_suffix() { return suffix;}
	virtual string get_shortname_suffix() { return shortsuffix;}
	virtual HullShapeIndexWrapper* clone() { return new HullShapeIndexWrapper(*this);}
	virtual bool enabled() {return perim->is_enabled() && area->is_enabled();}
	HullShapeIndexWrapper(SDNAPolylineIdRadiusIndexed2dArrayBase *area, SDNAPolylineIdRadiusIndexed2dArrayBase *perim,
		int radius_index, vector<double> &radii, bool cont_space)
					: area(area), perim(perim), radius_index(radius_index)
	{
		//this code is repeated.  ideally the whole hierarchy would be refactored
		stringstream ns, sns;
		if (radii[radius_index]!=GLOBAL_RADIUS)
		{
			ns << "R" << radii[radius_index];
			sns << radii[radius_index];
		}
		else
		{
			ns << "Rn";
			sns << "n";
		}
		if (cont_space)
		{
			ns << "c";
			sns << "c";
		}
		suffix = ns.str();
		shortsuffix = sns.str();
	}
	virtual float get_output(SDNAPolyline *x,int oversample)
	{
		return (float)(pow(perim->floatvalue(x->id,radius_index),2)/4./M_PI/area->floatvalue(x->id,radius_index)/oversample);
	}
	virtual int get_radius_index()
	{
		return radius_index;
	}
	virtual string get_name()
	{
		return get_name_prefix()+" "+get_name_suffix();
	}
	virtual string get_shortname()
	{
		return get_shortname_prefix()+get_shortname_suffix();
	}
};

//wrappers for specific things...
class SDNAPolylineAngularCostOutputDataWrapper : public OutputDataWrapper
{
public:
	virtual SDNAPolylineAngularCostOutputDataWrapper* clone() { return new SDNAPolylineAngularCostOutputDataWrapper(*this); }
	virtual bool enabled() {return true;}
	virtual float get_output(SDNAPolyline* x, int oversample)
	{
		return x->full_link_cost_ignoring_oneway(PLUS).angular;
	}
	virtual string get_name()
	{
		return string("Line Ang Curvature");
	}
	virtual string get_shortname()
	{
		return string("LAC");
	}
};

class SDNAPolylineLengthOutputDataWrapper : public OutputDataWrapper
{
public:
	virtual SDNAPolylineLengthOutputDataWrapper* clone() { return new SDNAPolylineLengthOutputDataWrapper(*this); }
	virtual bool enabled() {return true;}
	virtual float get_output(SDNAPolyline* x, int oversample)
	{
		return x->full_link_cost_ignoring_oneway(PLUS).euclidean;
	}
	virtual string get_name()
	{
		return string("Line Length");
	}
	virtual string get_shortname()
	{
		return string("LLen");
	}
};

class PolylineIndexedArrayOutputDataWrapper : public OutputDataWrapper
{
private:
	SDNAPolylineIdIndexedArray<float> *data;
	string name,shortname;
public:
	virtual string get_name()
	{
		return name;
	}
	virtual string get_shortname()
	{
		return shortname;
	}
	virtual PolylineIndexedArrayOutputDataWrapper* clone() { return new PolylineIndexedArrayOutputDataWrapper(*this); }
	PolylineIndexedArrayOutputDataWrapper(string name,string shortname,SDNAPolylineIdIndexedArray<float> *data)
				: name(name),shortname(shortname),data(data)
	{}
	virtual float get_output(SDNAPolyline* x,int oversample)
	{
		return (*data)[*x];
	}
	bool enabled() {return true;}
};

class SDNAPolylineSinuosityOutputDataWrapper : public OutputDataWrapper
{
public:
	virtual SDNAPolylineSinuosityOutputDataWrapper* clone() { return new SDNAPolylineSinuosityOutputDataWrapper(*this); }
	virtual bool enabled() {return true;}
	virtual float get_output(SDNAPolyline* x, int oversample)
	{
		const double crow_flight_length = Point::distance(x->get_point(EDGE_START),x->get_point(EDGE_END));
		return static_cast<float>(x->full_link_cost_ignoring_oneway(PLUS).euclidean/crow_flight_length);
	}
	virtual string get_name()
	{
		return string("Line Sinuosity");
	}
	virtual string get_shortname()
	{
		return string("LSin");
	}
};

class SDNAPolylineBearingOutputDataWrapper : public OutputDataWrapper
{
public:
	virtual SDNAPolylineBearingOutputDataWrapper* clone() { return new SDNAPolylineBearingOutputDataWrapper(*this); }
	virtual bool enabled() {return true;}
	virtual float get_output(SDNAPolyline* x, int oversample)
	{
		return Point::bearing(x->get_point(EDGE_START),x->get_point(EDGE_END));
	}
	virtual string get_name()
	{
		return string("Line Bearing");
	}
	virtual string get_shortname()
	{
		return string("LBear");
	}
};

class SDNAPolylineMetricOutputDataWrapper : public OutputDataWrapper
{
private:
	HybridMetricEvaluator *eval;
	polarity direction;
public:
	SDNAPolylineMetricOutputDataWrapper (HybridMetricEvaluator *e,polarity d) : eval(e), direction(d) {}
	virtual SDNAPolylineMetricOutputDataWrapper * clone() { return new SDNAPolylineMetricOutputDataWrapper (*this); }
	virtual bool enabled() {return true;}
	virtual float get_output(SDNAPolyline* x, int oversample)
	{
		Edge *e;
		if (direction==PLUS)
			e = &x->forward_edge;
		else
			e = &x->backward_edge;
		return eval->evaluate_edge_no_safety_checks(e->full_cost_ignoring_oneway(),e);
	}
	virtual string get_name()
	{
		if (direction==PLUS)
			return string("Hybrid Metric fwd");
		else
			return string("Hybrid Metric bwd");
	}
	virtual string get_shortname()
	{
		if (direction==PLUS)
			return string("HMf");
		else
			return string("HMb");
	}
};

class SDNAPolylineConnectivityOutputDataWrapper : public OutputDataWrapper
{
public:
	virtual SDNAPolylineConnectivityOutputDataWrapper* clone() { return new SDNAPolylineConnectivityOutputDataWrapper(*this); }
	virtual bool enabled() {return true;}
	virtual float get_output(SDNAPolyline* x, int oversample)
	{
		//connectivity is defined as number of ends of other links a link is connected to
		//so we need to make sure edges aren't repeated, as can happen with loops
		//and need to make sure none of the edges are from the same link
		set<Edge*> other_edges;
		for (OutgoingConnectionVector::iterator it = x->forward_edge.outgoing_connections.begin();
			it != x->forward_edge.outgoing_connections.end(); ++it)
			if (it->edge->link != x)
				other_edges.insert(it->edge);
		for (OutgoingConnectionVector::iterator it = x->backward_edge.outgoing_connections.begin();
			it != x->backward_edge.outgoing_connections.end(); ++it)
			if (it->edge->link != x)
				other_edges.insert(it->edge);
		return (float)other_edges.size();
	}

	virtual string get_name()
	{
		return string("Line Connectivity");
	}
	virtual string get_shortname()
	{
		return string("LConn");
	}
};

//for adding user-defined prefix/suffix to names
//note this is different to SplitNameOutputDataWrapper
//which allows definition of names with two internal components
class ExtraNameWrapper : public OutputDataWrapper
{
private:
	shared_ptr<OutputDataWrapper> odw;
	string prefix,postfix;
public:
	ExtraNameWrapper(OutputDataWrapper &od,string pre,string post)
		: odw(shared_ptr<OutputDataWrapper>(od.clone())), 
		  prefix(pre), postfix(post) {}
	virtual bool enabled() {return odw->enabled();}
	virtual float get_output(SDNAPolyline *x, int oversample)
	{
		return odw->get_output(x,oversample);
	}
	virtual ExtraNameWrapper *clone() 
	{
		return new ExtraNameWrapper(*this);
	}
	virtual string get_name()
	{
		return prefix + odw->get_name() + postfix;
	}
	virtual string get_shortname()
	{
		return prefix + odw->get_shortname() + postfix;
	}
};



// OutputStringArray: packages an vector of string objects into a C style char**
// which is useful for sending output data names over a C interface
class OutputStringArray
{
private:
	std::vector<std::string> strings;
	char ** data;
	bool finalized;
	void cleanup_string_array()
	{
		if (finalized)
		{
			for (unsigned int i=0;i<strings.size();i++)
				delete[] data[i];
			if (strings.size())
				delete[] data;
		}
	}
	
public:
	OutputStringArray() : finalized(false) {}
	OutputStringArray(const OutputStringArray& other) : strings(other.strings), finalized(false) { }
	OutputStringArray& operator=(const OutputStringArray& other) { clear(); strings=other.strings; return *this; }
	void add_string(std::string const& s)
	{
		assert (finalized==false);
		strings.push_back(s);
	}
	void append(vector<string> v)
	{
		assert (finalized==false);
		strings.insert( strings.end(), v.begin(), v.end() );
	}
	vector<string> get_strings()
	{
		return strings;
	}
	char ** get_string_array()
	{
		if (!finalized)
		{
			finalized = true;
			data = new char*[strings.size()];
			for (unsigned int i=0;i<strings.size();i++)
			{
				char const * const s = strings[i].c_str();
				data[i] = new char[strlen(s)+1]; //include null termination
				strcpy(data[i],s);
			}
		}
		return data;
	}
	size_t size()
	{
		return strings.size();
	}
	void clear()
	{
		cleanup_string_array();
		strings.clear();
		finalized = false;
	}
	~OutputStringArray()
	{
		cleanup_string_array();
	}
};

// OutputMap: holds the names and retrieval methods of all outputs
class OutputMap
{
private:
	typedef vector<shared_ptr<OutputDataWrapper> > OutputVector;
	OutputVector outputs;
	bool finalized;

	//defined here so as to pass string array over COM
	OutputStringArray long_name_buffer, short_name_buffer;

	void finalize()
	{
		finalized = true;
		for (OutputVector::iterator i = outputs.begin(); i != outputs.end(); i++)
		{
			long_name_buffer.add_string((*i)->get_name());
			short_name_buffer.add_string((*i)->get_shortname());
		}
	}

public:
	OutputMap() : finalized(false) {}
	void add_output(OutputDataWrapper &output)
	{
		assert(!finalized);
		if (output.enabled())
		{
			shared_ptr<OutputDataWrapper> output_ptr(output.clone());
			outputs.push_back(output_ptr);
		}
	}
	//deprecated - autocad
	inline void get_outputs_c(float* buffer, SDNAPolyline* link, int oversample)
	{
		int index=0;
		for (OutputVector::iterator i = outputs.begin(); i != outputs.end(); i++)
		{
			buffer[index++] = (*i)->get_output(link,oversample); 
		}
	}
	vector<float> get_outputs(SDNAPolyline *link, int oversample)
	{
		vector<float> outputfloats;
		outputfloats.reserve(getlength());
		for (OutputVector::iterator i = outputs.begin(); i != outputs.end(); i++)
		{
			outputfloats.push_back((*i)->get_output(link,oversample)); 
		}
		return outputfloats;
	}
	//deprecated - autocad
	char ** get_output_names_c()
	{
		if (!finalized) finalize();
		return long_name_buffer.get_string_array();
	}
	char ** get_short_output_names_c()
	{
		if (!finalized) finalize();
		return short_name_buffer.get_string_array();
	}
	vector<string> get_output_names()
	{
		if (!finalized) finalize();
		return long_name_buffer.get_strings();
	}
	vector<string> get_short_output_names()
	{
		if (!finalized) finalize();
		return short_name_buffer.get_strings();
	}
	size_t getlength()
	{
		if (!finalized) finalize();
		return outputs.size();
	}
};

class OutputBuffer : public vector<long>
{
public:
	long* toCPointer()
	{
		if (size())
			return &(*begin());
		else
			return NULL;
	}
	void operator=(const vector<long> &rhs)
	{
		if (&rhs != this)
		{
			clear();
			insert(begin(),rhs.begin(),rhs.end());
		}
	}
};