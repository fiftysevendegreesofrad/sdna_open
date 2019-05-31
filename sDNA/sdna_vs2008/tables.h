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
#include "sdna_arrays.h"
#include "dataacquisitionstrategy.h"

struct SDNARuntimeException : public std::exception
{
public:
	string message;
	SDNARuntimeException(string info) : message(info) {}
	const char* what() const throw()
	{
		return message.c_str();
	}
private:
	SDNARuntimeException();
};

//this is for zone tables - different to arrays.h which is for polyline or edge tables
//zones contain multiple polylines and zone data is distributed over them as specified by distribution rule

const float default_table_value = 0; //if you want to set anything other than this, you need to create zonesums for zones not in the table and fix NOTE1 below

//note that when this class is used with differing origin/destination zone sets
//it will use up 4x the memory it needs by storing intra-zone zeros
//this can be fixed inside the class if needed
//shouldn't matter anyway as number of zones << links

//what happens if o/d zone sets have name conflicts?
//same names get assigned to same index in origin and destination tables even though it's notionally a different namespace
//example.  ozones = a,b.  destzones = a,b.
// a a becomes 1,1
// a b becomes 1,2
// a c becomes 1,3
// b a becomes 2,1
// b b becomes 2,2
// b D becomes 2,4
//no chance of mistaking origin zone for dest zone as order in matrix lookup determines that
//but ZONESUMS must be different for o/d
class Table2d
{
	typedef map<pair<string,string>, float> MAPTYPE;
	EmergencyMemory emergencyMemory;
	string name,origname,destname; //should be called tablename,origzonefieldname,destzonefieldname though tablename is irrelevant for 2d tables as we currently only allow 1 to exist
	NetExpectedDataSource<string> *origname_das,*destname_das;
	//although these index the same set of zones in the table, the user may set them differently per link
	SDNAPolylineIdIndexedArray<size_t> origtableindex,desttableindex; 
	SDNAPolylineIdIndexedArray<double> zone_distribution_metric;
	set<string> zoneset;
	MAPTYPE zonepair_to_data;
	map<string,size_t> zone_to_index;
	vector<double> origzonesum,destzonesum;
	vector<vector<float> > index2_to_data;
	bool finalized,zonesums_calculated;
public:
	Table2d(char *name,char *origname,char *destname) 
		: finalized(false),zonesums_calculated(false),
		name(name),origname(origname),destname(destname) {}
	int addrow(char *fromzone,char *tozone,float d)
	{
		zoneset.insert(string(fromzone));
		zoneset.insert(string(tozone));
		const pair<string,string> zonepair = make_pair(string(fromzone),string(tozone));
		try
		{
			if (zonepair_to_data.find(zonepair)==zonepair_to_data.end())
			{
				zonepair_to_data[zonepair]=d;
				return 0;
			}
			else
				return 1;
		}
		catch (std::bad_alloc&)
		{
			emergencyMemory.free();
			return 2;
		}
	}
	void finalize()
	{
		assert (!finalized);
		finalized = true;
		//construct zone to index lookup
		size_t index=0;
		BOOST_FOREACH(const string& zone, zoneset)
			zone_to_index[zone]=index++;
		assert (index==zoneset.size());
		const size_t size = zoneset.size();
		//construct data table
		index2_to_data.swap(vector<vector<float>>(size,vector<float>(size,default_table_value)));
		BOOST_FOREACH(const MAPTYPE::value_type& myPair, zonepair_to_data)
		{
			const string zone1 = myPair.first.first;
			const string zone2 = myPair.first.second;
			const float data = myPair.second;
			index2_to_data[zone_to_index[zone1]][zone_to_index[zone2]]=data;
		}
		MAPTYPE().swap(zonepair_to_data); //clear memory of zone_to_data
		set<string>().swap(zoneset); //clear memory of zoneset

		//initialize zonesums
		origzonesum.swap(vector<double>(size,0.)); 
		destzonesum.swap(vector<double>(size,0.)); 
	}
	void register_polyline(SDNAPolyline *p,const double cost)
	//this is called for every polyline in the network after zone data is loaded
	//to sum up total values per zone and also save the zone distribution metric values
	//and to save zone table index for each polyline
	{
		assert(finalized);
		zonesums_calculated = true;
		
		//populate orig/dest table index, zone distribution metric and orig/dest zone sums
		const size_t origzoneindex = origZoneIndexForPolyline(p);
		origtableindex[*p] = origzoneindex;
		if (origzoneindex!=-1) //we could get zones not in the zone table - just ignore them
			origzonesum[origzoneindex] += cost;
		const size_t destzoneindex = destZoneIndexForPolyline(p);
		desttableindex[*p] = destzoneindex;
		if (destzoneindex!=-1)
			destzonesum[destzoneindex] += cost;
		zone_distribution_metric[*p] = cost;
	}
private:
	enum od_t {ORIGIN,DESTINATION};
	size_t origZoneIndexForPolyline(SDNAPolyline *p) { return zoneToIndex(p,ORIGIN); }
	size_t destZoneIndexForPolyline(SDNAPolyline *p) { return zoneToIndex(p,DESTINATION); }
	size_t zoneToIndex(SDNAPolyline *p,od_t which)
	//only called during register_polyline phase - results cached in orig/desttableindex for later use
	{
		assert(finalized);
		NetExpectedDataSource<string> *das = (which==ORIGIN) ? origname_das : destname_das;
		const string s = das->get_data(p);
		map<string,size_t>::iterator it = zone_to_index.find(s);
		if (it!=zone_to_index.end())
			return it->second;
		else
			return -1;
	}
	float indexToData(size_t index1,size_t index2)
	{
		assert(finalized);
		assert(index1!=-1 && index2!=-1); // zones must be valid (if data is default this was set in finalize())
		return index2_to_data[index1][index2];
	}
public:
	float getData(SDNAPolyline *orig,SDNAPolyline *dest)
	{
		assert(zonesums_calculated);
		const size_t origindex = origtableindex[*orig];
		const size_t destindex = desttableindex[*dest];
		const double orig_zdm = zone_distribution_metric[*orig];
		const double dest_zdm = zone_distribution_metric[*dest];
		if (origindex==-1 || destindex==-1)
			return default_table_value; //see NOTE1 above
		else
		{
			const double zoneproduct = origzonesum[origindex]*destzonesum[destindex];
			const double multiplier = (zoneproduct==0)?1:orig_zdm*dest_zdm/zoneproduct;
			return (float)((double)indexToData(origindex,destindex)*multiplier);
		}
	}
	void initializeItems(size_t n)
	{
		origtableindex.initialize(n);
		desttableindex.initialize(n);
		zone_distribution_metric.initialize(n);
	}
	void setNameDas(NetExpectedDataSource<string> *origdas,NetExpectedDataSource<string> *destdas)
	{
		origname_das = origdas;
		destname_das = destdas;
	}
	string origfieldname() {return origname;}
	string destfieldname() {return destname;}
};

//no automatic zone sums - for 1d tables these are calculated in 'calculation' as specified in zonesums config
template <class T> class Table
{
	typedef map<string, T> MAPTYPE;
	EmergencyMemory emergencyMemory;
	string name_s,zonefieldname_s;
	shared_ptr<NetExpectedDataSource<string> > zonefieldname_das;
	SDNAPolylineIdIndexedArray<size_t> zonetableindex;
	set<string> zoneset;
	MAPTYPE zone_to_data;
	map<string,size_t> zone_to_index;
	vector<T > index_to_data;
	bool finalized;
public:
	Table(const char *name,const char *zonefieldname)
		: finalized(false),
		name_s(name),zonefieldname_s(zonefieldname) {}
	int addrow(const char *zone_char,T d) //takes char* and returns value for external interface
	{
		string zone = string(zone_char);
		zoneset.insert(zone);
		try
		{
			if (zone_to_data.find(zone)==zone_to_data.end())
			{
				zone_to_data[zone]=d;
				return 0;
			}
			else
				return 1;
		}
		catch (std::bad_alloc&)
		{
			emergencyMemory.free();
			return 2;
		}
	}
	void incrementrow(string zone,T d) //takes string and returns void for internal use
	{
		if (zone_to_data.find(zone)==zone_to_data.end())
			addrow(zone.c_str(),d);
		else
			zone_to_data[zone]+=d;
	}
	void finalize()
	{
		assert (!finalized);
		finalized = true;
		//construct zone to index lookup
		size_t index=0;
		BOOST_FOREACH(const string& zone, zoneset)
			zone_to_index[zone]=index++;
		assert (index==zoneset.size());
		const size_t size = zoneset.size();
		//construct data table
		index_to_data.swap(vector<T>(size,default_table_value));
		BOOST_FOREACH(const MAPTYPE::value_type& myPair, zone_to_data)
		{
			const string zone = myPair.first;
			const T data = myPair.second;
			index_to_data[zone_to_index[zone]]=data;
		}
		MAPTYPE().swap(zone_to_data); //clear memory of zone_to_data
		set<string>().swap(zoneset); //clear memory of zoneset
	}
	void register_polyline(SDNAPolyline *p)
	//this is called for every polyline in the network after zone data is loaded
	//to save zone table index for each polyline, but not zone sums
	//nb. this could be slightly inefficient as it will create link->zone lookups once per table even when there is only one zone set
	{
		assert(finalized);
		
		//populate link table index
		const size_t zoneindex = zoneIndexForPolyline(p);
		zonetableindex[*p] = zoneindex;
	}
private:
	size_t zoneIndexForPolyline(SDNAPolyline *p)
	//only called during register_polyline phase - results cached in tableindex for later use
	{
		assert(finalized);
		const string s = zonefieldname_das->get_data(p);
		map<string,size_t>::iterator it = zone_to_index.find(s);
		if (it!=zone_to_index.end())
			return it->second;
		else
			return -1;
	}
	T indexToData(size_t index)
	{
		assert(finalized);
		assert(index!=-1); // zones must be valid (if data is default this was set in finalize())
		return index_to_data[index];
	}
public:
	T getData(SDNAPolyline *p)
	{
		const size_t zoneindex = zonetableindex[*p];
		if (zoneindex==-1)
			return default_table_value; //see NOTE1 above
		else
			return indexToData(zoneindex);
	}
	void initializeItems(size_t n)
	{
		zonetableindex.initialize(n);
	}
	void setNameDas(shared_ptr<NetExpectedDataSource<string> > das)
	{
		zonefieldname_das = das;
	}
	string zonefieldname() {return zonefieldname_s;}
	string name() {return name_s;}
};

//this is used for skim matrix output
//it can probably be used by Table2d but it isn't 
//it could have been used for net attached data too if we had it then
class ZoneIndexMapping
{
public:
	size_t add(string zone) {return zone_to_index(zone,true);}
	size_t zone_to_index(string zone,bool can_create=false)
	{
		map<string,size_t>::iterator it = zone_to_index_map.find(zone);
		if (it!=zone_to_index_map.end())
			return it->second;
		else
		{
			if (!can_create)
				throw SDNARuntimeException("Zone not found in mapping");
			else
			{
				const size_t index = index_to_zone_vector.size();
				index_to_zone_vector.push_back(zone);
				zone_to_index_map[zone]=index;
				return index;
			}
		}
	}
	string index_to_zone(size_t index)
	{
		return index_to_zone_vector[index];
	}
	size_t size() {return index_to_zone_vector.size();}
private:
	vector<string> index_to_zone_vector;
	map<string,size_t> zone_to_index_map;
};



