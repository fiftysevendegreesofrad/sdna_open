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

// sdna_arrays.h: defines array classes with some type checking for the indices
// they can also be indexed by Edge rather than SdnaId - the Edge's id is automatically looked up
// useful for algorithm code, especially when parallel

#include "stdafx.h"
#pragma once
#pragma warning( disable : 4996 )
#include "edge.h"
#include "config_string_parser.h"

//prettifiers: make edge*s and infinite doubles into nice strings for debug printing
template<class T> class Prettifier
{
public:
	static string prettify(T data)
	{
		stringstream ss;
		ss << data;
		return ss.str();
	}
};

template<> class Prettifier<Edge*>
{
public:
	static string prettify(Edge * data);
};

template<> class Prettifier<double>
{
public:
	static string prettify(double data)
	{
		stringstream ss;
		if (data==numeric_limits<double>::infinity())
			ss << "-";
		else
			ss << data;
		return ss.str();
	}
};

template<> class Prettifier<float>
{
public:
	static string prettify(float data)
	{
		stringstream ss;
		if (data==numeric_limits<float>::infinity())
			ss << "-";
		else
			ss << data;
		return ss.str();
	}
};

//IdIndexedArray : array indexed by UniqueIds
template<class T,class INDEX> class IdIndexedArray
{
private:
	IdIndexedArray& operator=(const IdIndexedArray<T,INDEX> &original);
	IdIndexedArray<T,INDEX>(const IdIndexedArray<T,INDEX> &original);
protected:
	T * data;
	size_t size;
	void initialize_inner()
	{
		data = new T[size];
	}
public:
	IdIndexedArray() : size(0) {}
	IdIndexedArray(size_t size) : size(size)
	{
		initialize_inner();
	}
	void initialize(size_t x)
	{
		assert(!isInitialized());
		size = x;
		initialize_inner();
	}
	void set_all_to(T val)
	{
		assert(isInitialized());
		for (size_t i=0;i<size;i++)
			data[i]=val;
	}
	size_t get_size()
	{
		return size;
	}
	~IdIndexedArray()
	{
		if (size>0)
			delete[] data;
	}
	inline T& operator[](const INDEX x)
	{
		assert (x.id<size);
		return data[x.id];
	}
	inline T& operator[](typename INDEX::indexed_class_type const &xe)
	{
		const INDEX x = xe.get_id();
		return operator[](x);
	}
	//the "get_data" methods are same use as [] but guarantee const to allow parallel optimization, maybe
	inline T get_data(const INDEX x) const
	{
		assert (x.id<size);
		return data[x.id];
	}
	inline T get_data(typename INDEX::indexed_class_type const &xe) const
	{
		const INDEX x = xe.get_id();
		return get_data(x);
	}
	bool isInitialized()
	{
		return (size>0);
	}
	virtual void print()
	{
		if (isInitialized())
		{
			for (size_t x=0;x<size;x++)
				std::cout << Prettifier<T>::prettify(data[x]) << ",";
			std::cout << endl;
		}
		else
		{
			std::cout << " (uninitialized)" << endl;
		}
	}
};

template <class INDEX> class IdRadiusIndexed2dArrayBase
{
public:
	virtual float floatvalue(INDEX x, size_t y) = 0;
	virtual bool is_enabled() = 0;
	virtual void enable() = 0;
};

template<class T,class INDEX> class IdRadiusIndexed2dArray : public IdRadiusIndexed2dArrayBase<INDEX>
{
private:
	IdIndexedArray<T*,INDEX> data;
	size_t radii;
	//not the same as initialization: will_be_used records what will be used before
	//the event, so output methods can decide whether to include the array
	bool will_be_used; 

	T dummy; //to return when not enabled

	IdRadiusIndexed2dArray<T,INDEX>& operator=(const IdRadiusIndexed2dArray<T,INDEX> &other);
	IdRadiusIndexed2dArray<T,INDEX>(const IdRadiusIndexed2dArray<T,INDEX> &other);
public:
	IdRadiusIndexed2dArray() : radii(0), will_be_used(false) {}
	IdRadiusIndexed2dArray(size_t n,size_t r,T initval) : will_be_used(true)
	{
		initialize(n,r,initval);
	}
	virtual void enable()
	{
		will_be_used = true;
	}
	virtual bool is_enabled()
	{
		return will_be_used;
	}
	void initialize(size_t n,size_t r,T initval)
	{
		if (is_enabled())
		{
			data.initialize(n);
			radii = r;
			for (INDEX i=INDEX(0);i.id<data.get_size();i.id++)
				data[i] = new T[radii];
			set_all(initval);
		}
	}
	void set_all(T initval)
	{
		if (is_enabled())
		{
			for (INDEX i=INDEX(0);i.id<data.get_size();i.id++)
				for (size_t j=0;j<radii;j++)
					data[i][j] = initval;
		}
	}
	~IdRadiusIndexed2dArray()
	{
		for (INDEX i=INDEX(0);i.id<data.get_size();i.id++)
			delete[] data[i];
	}
	inline T& operator() (INDEX x, size_t y)
	{
		if (is_enabled())
		{
			assert (y<radii);
			return data[x][y];
		}
		else
			return dummy;
	}
	inline T& operator() (const typename INDEX::indexed_class_type * const xe, size_t y)
	{
		return operator()(xe->get_id(),y);
	}
	inline virtual float floatvalue(INDEX x, size_t y)
	{
		return (float)operator()(x,y);
	}
};

template <class T> class SDNAPolylineIdRadiusIndexed2dArray : public IdRadiusIndexed2dArray<T,SDNAPolylineId> {};
template <class T> class SDNAPolylineIdIndexedArray : public IdIndexedArray<T,SDNAPolylineId> {};
typedef IdRadiusIndexed2dArrayBase<SDNAPolylineId> SDNAPolylineIdRadiusIndexed2dArrayBase;

//reserves a block of memory for cleanup in case we run out
class EmergencyMemory
{
	vector<char> mem;
public:
	EmergencyMemory()
	{
		mem.reserve(1024*1024);
	}
	void free()
	{
		vector<char>().swap(mem);
	}
};

template <class T> struct defaultattacheddata;
template <> struct defaultattacheddata<string> {
    static string get() { return string(""); }
};
template <> struct defaultattacheddata<float> {
    static float get() { return 0; }
};
template <class T> T default_attached_data() { return defaultattacheddata<T>::get(); }

//manages per-link named user data on a network
//the data is actually stored in the network links, but names indexed here
//do we allow late adding of named data? currently yes, in case autocad needs it
//(though it probably won't as that would be inefficient use of memory to add data not asked for)
typedef map<string,long> DataNamesType;
typedef DataNamesType::const_iterator DataNamesIterator;
template <typename T> class NetAttachedData
{
public:
	size_t num_cols()
	{
		return name_to_nameindex.size();
	}
	void attach_data(SDNAPolyline *s,long nameindex,T data_to_attach)
	{
		s->set_attached_data(nameindex,data_to_attach);
	}
	void attach_data(SDNAPolyline *s,string name,T data_to_attach)
	{
		const long nameindex = get_nameindex(name,true,default_attached_data<T>());
		attach_data(s,nameindex,data_to_attach);
	}
	T get_data(const SDNAPolyline * const s,long nameindex)
	{
		return get_all_data(s)[nameindex];
	}
	T get_data(const SDNAPolyline* const s,string name)
	{
		const long nameindex = get_nameindex(name,false);
		return get_data(s,nameindex);
	}
	vector<T> const& get_all_data(const SDNAPolyline * const s)
	{
		return s->get_all_data(default_attached_data<T>());
	}
	long add_nameindex(string name,T defaultdata)
	{
		//add to name_to_nameindex
		const long new_nameindex = numeric_cast<long>(num_cols());
		name_to_nameindex[name] = new_nameindex;
		
		//add blank column to data
		for (SDNAPolylineContainer::iterator it=links->begin();it!=links->end(); ++it)
			it->second->append_attached_data(defaultdata);

		return new_nameindex;
	}
	vector<string> get_names()
	{
		vector<string> v(name_to_nameindex.size());
		for (DataNamesIterator it=name_to_nameindex.begin();it!=name_to_nameindex.end();++it)
			v[it->second]=it->first;
		return v;
	}
	long get_nameindex(string name,bool can_create_new,T defaultdata)
	{
		map<string,long>::iterator pos = name_to_nameindex.find(name);
		if (pos==name_to_nameindex.end())
		{
			if (!can_create_new)
				return -1; //you tried to access a nonexistant name
			else
				return add_nameindex(name,defaultdata);
		}
		else
			return pos->second;
	}
	bool has_name(string name) {return name_to_nameindex.find(name)!=name_to_nameindex.end();}
	void print(SDNAPolyline *link)
	{
		cout << "    ";
		for (DataNamesType::iterator it=name_to_nameindex.begin();it!=name_to_nameindex.end();it++)
		{
			const string& name = it->first;
			const long& index = it->second;
			cout << name << "=" << get_data(link,index) << " ";
		}
		cout << endl;
	}
	NetAttachedData<T>(SDNAPolylineContainer *links) : links(links) {}
private:
	//quick test shows vector of names is faster than map for sizes <5
	//for the most part though we access by index anyway
	DataNamesType name_to_nameindex;
	SDNAPolylineContainer *links;
};

