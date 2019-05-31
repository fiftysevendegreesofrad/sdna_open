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
#include "edge.h"
#include "sdna_output_utils.h"
#include "config_string_parser.h"
using namespace std;
#include "dataacquisitionstrategy.h"

enum weighting_t {LENGTH_WEIGHT,LINK_WEIGHT,POLYLINE_WEIGHT};

class LengthWeightingStrategy : public NetExpectedDataSource<float>
{
private:
	weighting_t weighting_type;
	SDNAPolylineIdIndexedArray<float> *linkfrac;
public:
	LengthWeightingStrategy(NetExpectedDataSource<float> d,weighting_t t,SDNAPolylineIdIndexedArray<float> *linkfrac) 
		: NetExpectedDataSource<float>(d),weighting_type(t),linkfrac(linkfrac) {}
	LengthWeightingStrategy() : NetExpectedDataSource<float>() {}
	weighting_t get_weighting_type(){return weighting_type;}
	virtual float get_data(SDNAPolyline * const link)
	{
		switch (weighting_type)
		{
		case LENGTH_WEIGHT:
			return NetExpectedDataSource<float>::get_data(link) * link->full_link_cost_ignoring_oneway(PLUS).euclidean;
		case POLYLINE_WEIGHT:
			return NetExpectedDataSource<float>::get_data(link);
		case LINK_WEIGHT:
			return NetExpectedDataSource<float>::get_data(link) * (*linkfrac)[*link];
		default:
			assert(false);
			return 0;
		}
	}
	virtual void set_data(SDNAPolyline * const link,float d)
	{
		switch (weighting_type)
		{
		case LENGTH_WEIGHT:
			NetExpectedDataSource<float>::set_data(link,d / link->full_link_cost_ignoring_oneway(PLUS).euclidean);
			return;
		case POLYLINE_WEIGHT:
			NetExpectedDataSource<float>::set_data(link,d);
			return;
		case LINK_WEIGHT: //unsupported - will mean linking edges in prepare
		default:
			assert(false);
			return;
		}
	}
};

// JunctionContainer: allocates, deallocates and indexes nodes, used to detect when edges are joined (see ensure_junctions_created, link_edges)
// JunctionMapKey: spatial key based on x,y,z co-ordinates
struct JunctionMapKey 
{
	double x,y;
	float z;
	JunctionMapKey(double xx,double yy,float zz);
	friend bool operator< (const JunctionMapKey &a,const JunctionMapKey &b);
	bool operator== (const JunctionMapKey &other) { return x==other.x && y==other.y && z==other.z; }
};

typedef vector<JunctionMapKey> Cluster;
typedef vector<Cluster > ClusterList;

class JunctionContainer 
{
	boost::object_pool<Junction> pool;
	typedef boost::bimap<JunctionMapKey,Junction*> JuncBimap;
	JuncBimap key_to_junc;
	
	Junction* add_or_get(JunctionMapKey &key)
	{
		// does this node exist already?
		JuncBimap::left_iterator pos = key_to_junc.left.find(key);
		Junction *j;
		if (pos == end())
		{
			// Junction doesn't exist yet - create
			j = new (pool.malloc()) Junction();
			key_to_junc.insert( JuncBimap::value_type(key,j));
		}
		else
			j = pos->second;
		return j;
	}
	void erase(Junction *j)
	{
		key_to_junc.right.erase(j);
		pool.destroy(j);
	}
public:
	size_t size() {return key_to_junc.size();}
	Junction* const operator[](JunctionMapKey &key) 
	{
		const JuncBimap::left_iterator pos = key_to_junc.left.find(key);
		if (pos != key_to_junc.left.end())
			return pos->second;
		else
			return NULL;
	}
	typedef JuncBimap::left_iterator iterator;
	iterator begin() { return key_to_junc.left.begin(); }
	iterator end() { return key_to_junc.left.end(); }

	void add_edge(Edge *e, Point p,junction_option_type type);
	void remove_edge(Edge *e, Point p,junction_option_type type);
};

typedef vector<Edge*> EdgePointerContainer;

// THE MAIN NETWORK STORAGE OBJECT
class Net
{
	friend class NetExpectedDataSource<float>;
	friend class NetExpectedDataSource<string>;

	EmergencyMemory emergencyMemory;
	boost::object_pool<SDNAPolyline> link_pool;
	NetAttachedData<float> attached_data;
	NetAttachedData<string> attached_text_data;
	vector<SDNAPolyline*> links_not_in_junction_map_yet;
	
	OutputStringArray attached_data_names;

	vector<Junction*> junction_from_id;
	
public:
	Net::Net();

	Junction* get_junction_from_id(JunctionId id) { return junction_from_id[id.id]; }
	NetExpectedDataSource<float> start_gsdata,end_gsdata;
	NetExpectedDataSource<float> *onewaydatastrat, *vertonewaydatastrat;
	void set_gs_data(NetExpectedDataSource<float> se,NetExpectedDataSource<float> ee)
	{ 
		start_gsdata = se;
		end_gsdata = ee;
	}
	void set_oneway_data(NetExpectedDataSource<float> *oneway,NetExpectedDataSource<float> *vertoneway)
	{
		//takes pointers and does not copy - caller must maintain the strategy objects
		onewaydatastrat=oneway;
		vertonewaydatastrat=vertoneway;
	}

	void unlink_edges();
		
	void add_polyline(long arcid,vector<Point> &pointlist);
	void remove_link(SDNAPolyline *s);
	
	//for import operations; deprecated, left for autocad
	vector<vector<Point> > unlinks;

	bool net_is_built;
	

	SDNAPolylineContainer link_container; 
	JunctionContainer junction_storage;
	//although all edges reside within links, we keep this too
	//   - for quick access to edges loading routing algorithm (in memory order too)
	//   - for easy iteration over edges:
	EdgePointerContainer edge_ptr_container;

	bool reserve(size_t num_links)
	{
		try
		{
			edge_ptr_container.reserve(num_links*2);
		}
		catch (std::bad_alloc&)
		{
			emergencyMemory.free();
			return false;
		}
		return true;
	}
	
	//some of the following should be in SDNAIntegralCalculation
	ClusterList get_near_miss_clusters(double xytol,double ztol);
	void ensure_junctions_created();
	void link_edges();
	void warn_about_invalid_turn_angles(int(__cdecl *print_warning_callback)(const char*));
	void assign_junction_ids();
	void assign_link_edge_ids();
		
	size_t num_edges() {assert(num_items()*2==edge_ptr_container.size()); return edge_ptr_container.size();}
	size_t num_items() {return link_container.size();}
	size_t num_junctions() {return junction_storage.size();}

	vector<string> get_attached_data_names_in_order() { return attached_data.get_names(); }
	vector<float> const& get_all_attached_data_for_polyline(const SDNAPolyline *s) { return attached_data.get_all_data(s); }

	bool add_polyline(long arcid,int geom_length,double *xs,double *ys,float *zs);
	bool add_polyline_data(long arcid,string name,float data);
	bool add_polyline_text_data(long arcid,string name,char *data);
	
	void add_unlink(long geom_length,double *xs,double *ys); //not used except in import operation
	
	void add_polyline_centres(traversal_event_type centre_type);

private: 
	long get_index_for_data_name(string name,bool can_create,float defaultdata)
	{
		return attached_data.get_nameindex(name,can_create,defaultdata);
	}
	long get_index_for_data_name(string name,bool can_create,string defaultdata)
	{
		return attached_text_data.get_nameindex(name,can_create,defaultdata);
	}
	float get_data(const SDNAPolyline * const link,long index,float)
	{
		return attached_data.get_data(link,index);
	}
	string get_data(const SDNAPolyline * const link,long index,string)
	{
		return attached_text_data.get_data(link,index);
	}
	void attach_data(SDNAPolyline * const link,long index,float data)
	{
		attached_data.attach_data(link,index,data);
	}
	void attach_data(SDNAPolyline * const link,long index,string data)
	{
		attached_text_data.attach_data(link,index,data);
	}
	bool has_data_name(string name)
	{
		return attached_data.has_name(name);
	}
public:
	size_t attached_data_size()
	{
		return attached_data.num_cols();
	}
	size_t attached_text_data_size()
	{
		return attached_text_data.num_cols();
	}
	size_t get_names(char ***names)
	{
		attached_data_names = OutputStringArray();
		vector<string> vnames = get_attached_data_names_in_order();
		for (vector<string>::iterator it = vnames.begin();it!=vnames.end();it++)
			attached_data_names.add_string(*it);
		*names = attached_data_names.get_string_array();
		return attached_data_names.size();
	}
	//display for debug:
	void print();
};

//netiterator provides a c interface to reading the contents of nets
class NetIterator
{
	SDNAPolylineContainer::iterator it;
	SDNAPolylineContainer::iterator end;
	Net *n;
	
	//points
	double* xs_buffer;
	double* ys_buffer;
	float * zs_buffer;

	//data buffer
	vector<float> data_buffer;
	
public:
	NetIterator(Net *n) : it(n->link_container.begin()), end(n->link_container.end()), n(n)
	{
		size_t longest_link_geometry = 0;

		//allocate buffers
		for (SDNAPolylineContainer::iterator i = n->link_container.begin(); i!=n->link_container.end(); ++i)
		{
			const size_t len = i->second->points.size();
			if (len > longest_link_geometry)
				longest_link_geometry = len;
		}
		xs_buffer = new double[longest_link_geometry];
		ys_buffer = new double[longest_link_geometry];
		zs_buffer = new float[longest_link_geometry];
	}		

	int next(long *arcid,long *geom_length,double **point_array_x,double **point_array_y,float **point_array_z,float **data)
	{
		if (it!=end)
		{
			SDNAPolyline *s = it->second;
			
			//fill buffers
			for (PointVector::iterator i = s->points.begin(); i!=s->points.end(); ++i)
			{
				const size_t index = i - s->points.begin();
				xs_buffer[index] = i->x;
				ys_buffer[index] = i->y;
				zs_buffer[index] = i->z;
			}
			*arcid = s->arcid;
			*geom_length = numeric_cast<long>(s->points.size());
			*point_array_x = xs_buffer;
			*point_array_y = ys_buffer;
			if (point_array_z)
				*point_array_z = zs_buffer;
			data_buffer = n->get_all_attached_data_for_polyline(s);
			if (data_buffer.size())
				*data = &(data_buffer[0]);

			++it;
			return 1;
		}
		else
			return 0;
	}
	Net* getNet() {return n;}
	~NetIterator()
	{
		delete[] xs_buffer;
		delete[] ys_buffer;
		delete[] zs_buffer;
	}
};


