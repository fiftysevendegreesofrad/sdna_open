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
#include "sdna_spatial_accumulators.h"
#include "linkdatasource.h"
#pragma once 

//this is a generic geometry type for multigeometries with attached data
//now extended to take no-geometries with attached data i.e. tables
//it was created for buffer/geodesic geometry output and is now used for inputs too
//Net still retains its own internal geometry as primary input to sdna backend
//If we ever think of a reason to input a 2nd net it will probably come as a geomitem

union SDNAOutputUnion
{
	long l;
	float f;
	const char *s;
	SDNAOutputUnion() {}
	SDNAOutputUnion(long l) : l(l) {}
	SDNAOutputUnion(float f) : f(f) {}
	SDNAOutputUnion(char* s) : s(s) {}
};

SDNAOutputUnion variant_to_union(SDNAVariant const &v);

enum OutputFieldType {fLONG,fFLOAT,fSTRING};

struct FieldMetaData
{
	OutputFieldType type;
	string name,shortname;
	FieldMetaData(OutputFieldType type,string name,string shortname)
		: type(type),name(name),shortname(shortname) {}
	FieldMetaData(OutputFieldType type,string name)
		: type(type),name(name),shortname(name) {}
};

const char* outfieldtype_to_pythontype(OutputFieldType t);

typedef boost::geometry::model::polygon<point_xy<double> > hull_type;
		
enum sDNAGeom_t {POLYLINEZ, POLYGON, MULTIPOLYLINEZ, NO_GEOM};

class sDNAGeometry
{
public:
	virtual BoostLineString3d get_points() = 0;
	virtual void append_points_to(BoostLineString3d&) = 0;
};

class sDNAGeometryPointsByValue : public sDNAGeometry
{
	BoostLineString3d points;
	class AddPointFunctor
	{
		sDNAGeometryPointsByValue *parent;
	public:
		AddPointFunctor(sDNAGeometryPointsByValue *parent) : parent(parent) {}
		void operator()(point_xy<double>& p) //add_point function for boost for_each
		{
			parent->add_point_2d_to_3d(p);
		}
	};
	friend class AddPointFunctor;
public:
	sDNAGeometryPointsByValue() {}
	sDNAGeometryPointsByValue(hull_type points_in) 
	{
		//copy 2d polygon to points, adding 0s for z coordinates
		boost::geometry::for_each_point(points_in,AddPointFunctor(this)); //calls add_point function above
	}
	void add_point(point_xyz &p)
	{
		using boost::geometry::append;
		append(points,p);
	}
	void add_point(Point &p) 
	{
		using boost::geometry::make;
		//add_point(boost::geometry::make<point_xyz >(p.x,p.y,p.z));
		point_xyz bp(p.x,p.y,p.z);
		add_point(bp);
	}
	void add_point_2d_to_3d(point_xy<double>& p) 
	{
		using boost::geometry::make;
		//add_point(boost::geometry::make<point_xyz >(p.x(),p.y(),0));
		point_xyz p3(p.x(),p.y(),0.);
		add_point(p3);
	}
	virtual BoostLineString3d get_points() {return points;}
	virtual void append_points_to(BoostLineString3d& result)
	{
		boost::geometry::append(result,points);
	}
};

class sDNAGeometryPointsByEdgeLength : public sDNAGeometry
{
	enum pos_t {START_TO_END,MIDDLE_TO_END,SURROUNDING_MIDDLE};
	struct EdgeLengthPos
	{
		Edge * edge;
		float length;
		pos_t pos;
		EdgeLengthPos(Edge* e, float l, pos_t p) : edge(e),length(l),pos(p) {}
	};
	vector<const EdgeLengthPos> parts;
public:
	void add_edge_length_from_start_to_end(Edge* e, float length)
	{
		parts.push_back(EdgeLengthPos(e,length,START_TO_END));
	}
	void add_edge_length_from_middle_to_end(Edge *e, float length)
	{
		parts.push_back(EdgeLengthPos(e,length,MIDDLE_TO_END));
	}
	void add_edge_radius_surrounding_middle_ignoring_oneway(Edge *e, float length)
	{
		parts.push_back(EdgeLengthPos(e,length,SURROUNDING_MIDDLE));
	}
	void append_points_to(BoostLineString3d &result)
	{
		for (vector<const EdgeLengthPos>::iterator it=parts.begin(); it!=parts.end(); ++it)
		{
			switch(it->pos)
			{
			case START_TO_END: 
				it->edge->add_partial_points_to_geometry(result,it->length);
				break;
			case MIDDLE_TO_END: 
				it->edge->add_partial_points_from_middle_to_geometry_ignoring_oneway(result,it->length); 
				break;
			case SURROUNDING_MIDDLE:
				{
					boost::geometry::model::linestring<point_xyz > first_half;
					it->edge->get_twin()->add_partial_points_from_middle_to_geometry_ignoring_oneway(first_half,it->length);
					boost::geometry::reverse(first_half);
					boost::geometry::append(result,first_half);
					it->edge->add_partial_points_from_middle_to_geometry_ignoring_oneway(result,it->length);
				}
				break;
			default: assert(false);
			}
		}
	}
	virtual BoostLineString3d get_points()
	{
		BoostLineString3d result;
		append_points_to(result);
		return result;
	}
};

//this can also hold a single geometry; if the type is a single type then an assert will fire if you try to add multiple geoms to it
class sDNADataMultiGeometry
{
private:
	sDNAGeom_t type;
	vector<shared_ptr<sDNAGeometry> > items;
	vector<SDNAVariant> data;
public:
	sDNADataMultiGeometry(sDNAGeom_t type, vector<SDNAVariant> data, size_t reserve) 
		: type(type), data(data)
	{
		items.reserve(reserve);
	}
	void add_part(shared_ptr<sDNAGeometry> g) 
	{ 
		assert(items.size()==0 || type==MULTIPOLYLINEZ);
		assert(type!=NO_GEOM);
		items.push_back(g);
	}

	size_t get_num_parts() {return items.size();}
	void append_points_to(size_t part,BoostLineString3d &p) 
	{ 
		return items[part]->append_points_to(p);
	}
	BoostLineString3d get_points(size_t part)
	{
		return items[part]->get_points();
	}
	const vector<SDNAVariant>& get_data() {return data;}
	sDNAGeom_t get_type() {return type;}
};

//collection of objects of identical type, each with attached data
class sDNAGeometryCollectionIteratorBase
{
public:
	virtual size_t getpart(size_t index,double** point_array_x,double** point_array_y,float **point_array_z) = 0;
	virtual int next(long *num_parts,SDNAOutputUnion **data) = 0;
	virtual ~sDNAGeometryCollectionIteratorBase() {}
};

class sDNAGeometryCollection;
class sDNAGeometryCollectionIterator : public sDNAGeometryCollectionIteratorBase
{
	vector<shared_ptr<sDNADataMultiGeometry> >::iterator it, it_for_getpart;
	sDNAGeometryCollection *gc;
	vector<double> xs_buffer, ys_buffer;
	vector<float> zs_buffer;
public:
	sDNAGeometryCollectionIterator(sDNAGeometryCollection *gc);		

	class UnpackPointFunctor
	{
		sDNAGeometryCollectionIterator *parent;
	public:
		UnpackPointFunctor(sDNAGeometryCollectionIterator *parent) : parent(parent) {}
		void operator()(point_xyz& p) //unpack_point function for boost for_each
		{
			parent->xs_buffer.push_back(p.get<0>());
			parent->ys_buffer.push_back(p.get<1>());
			parent->zs_buffer.push_back(static_cast<float>(p.get<2>()));
		}
	};
	friend class UnpackPointFunctor;

	virtual size_t getpart(size_t index,double** point_array_x,double** point_array_y,float **point_array_z)
	{
		BoostLineString3d points = (*it_for_getpart)->get_points(index); //because next() below increments it before this is called
		const size_t size = boost::geometry::num_points(points);
		xs_buffer.clear();
		ys_buffer.clear();
		zs_buffer.clear();
		xs_buffer.reserve(size);
		ys_buffer.reserve(size);
		zs_buffer.reserve(size);
		boost::geometry::for_each_point(points,UnpackPointFunctor(this)); //calls unpack_point function
		if (size)
		{
			*point_array_x = &(xs_buffer[0]);
			*point_array_y = &(ys_buffer[0]);
			*point_array_z = &(zs_buffer[0]);
		}
		return size;
	}
	vector<SDNAOutputUnion> databuffer;
	virtual int next(long *num_parts,SDNAOutputUnion **data);
	virtual ~sDNAGeometryCollectionIterator() {}
};

class sDNAGeometryCollectionBase
{
public:
	virtual const char* get_name() = 0;
	virtual const char* get_type() = 0;
	virtual size_t get_field_metadata(char*** names,char*** shortnames,char ***pythontypes) = 0;
	virtual size_t get_num_items() = 0;
	virtual sDNAGeometryCollectionIteratorBase* getIterator() = 0;
};

class sDNAGeometryCollection : public sDNAGeometryCollectionBase
{
public:
	//for external use
	virtual const char* get_name() {return name.c_str();}
	virtual const char* get_type() {return type_as_string();}
	const sDNAGeom_t getType() {return type;}
	const long dataLength() {return numeric_cast<long>(m_datanames.size());}
	virtual size_t get_field_metadata(char*** names,char*** shortnames,char ***pythontypes)
	{
		*names = m_datanames.get_string_array();
		*shortnames = m_shortdatanames.get_string_array();
		*pythontypes = m_pythontypes.get_string_array();
		assert(m_datanames.size()==m_shortdatanames.size());
		assert(m_datanames.size()==m_pythontypes.size());
		return m_datanames.size();
	}
	virtual size_t get_num_items() {return items.size();}
	virtual sDNAGeometryCollectionIteratorBase* getIterator() { return new sDNAGeometryCollectionIterator(this);}
	
	//for internal use
	sDNAGeometryCollection() {} //constructs an object will throw assertion fail if you try to use it
	sDNAGeometryCollection(string name,sDNAGeom_t type,vector<FieldMetaData> fieldmetadata)
		: name(name), type(type)
	{ 
		BOOST_FOREACH (FieldMetaData &fmd , fieldmetadata)
		{
			m_datanames.add_string(fmd.name);
			m_shortdatanames.add_string(fmd.shortname);
			m_pythontypes.add_string(outfieldtype_to_pythontype(fmd.type));
		}
	}
	void reserve(size_t n)
	{
		items.reserve(n); 
	}
	void add(shared_ptr<sDNADataMultiGeometry> g)
	{
		assert(is_initialized());
		assert(g->get_type()==type);
		assert(g->get_data().size()==m_datanames.size());
		items.push_back(g);
	}
	void emergencyMemoryFree() {emergencyMemory.free();}
	
private:		
	EmergencyMemory emergencyMemory;
	string name;
	sDNAGeom_t type;
	string type_s;
	ThreadSafeVector<shared_ptr<sDNADataMultiGeometry> > items;
	OutputStringArray m_datanames, m_shortdatanames, m_pythontypes;
	
	bool is_initialized() {return !name.empty();}
	const char* type_as_string()
	{
		if (type==MULTIPOLYLINEZ) type_s="MULTIPOLYLINEZ";
		else if (type==POLYGON) type_s="POLYGON";
		else if (type==POLYLINEZ) type_s="POLYLINEZ";
		else if (type==NO_GEOM) type_s="NO_GEOM";
		else assert(false);
		return type_s.c_str();
	}

	friend class sDNAGeometryCollectionIterator;
};

class SDNAIntegralCalculation;
class SDNAPolylineDataSourceGeometryCollectionIteratorWrapper : public sDNAGeometryCollectionIteratorBase
{
	NetIterator net_it;
	vector<SDNAPolylineDataSource*> datasources;
	long geom_length;
	double *xs, *ys;
	float *zs;
	size_t numoutputs;
	vector<SDNAOutputUnion> databuffer;
	vector<SDNAVariant> variantbuffer;
public:
	SDNAPolylineDataSourceGeometryCollectionIteratorWrapper(vector<SDNAPolylineDataSource*> ds);

	virtual size_t getpart(size_t index,double** point_array_x,double** point_array_y,float **point_array_z)
	{
		assert(index==0);
		*point_array_x = xs;
		*point_array_y = ys;
		*point_array_z = zs;
		return geom_length;
	}
	virtual int next(long *num_parts,SDNAOutputUnion **data);
};

class SDNAPolylineDataSourceGeometryCollectionWrapper : public sDNAGeometryCollectionBase
{
private:
	vector<SDNAPolylineDataSource*> datasources;
	Net *net;
	OutputStringArray names,shortnames,pythontypes;
public:
	SDNAPolylineDataSourceGeometryCollectionWrapper(vector<SDNAPolylineDataSource*> ds);
	SDNAPolylineDataSourceGeometryCollectionWrapper() {}
	virtual const char* get_name() { return "net";}
	virtual const char* get_type() { return "POLYLINEZ"; }
	virtual size_t get_field_metadata(char*** names,char*** shortnames,char*** pythontypes);
	virtual size_t get_num_items();
	virtual sDNAGeometryCollectionIteratorBase* getIterator()
	{
		return new SDNAPolylineDataSourceGeometryCollectionIteratorWrapper(datasources);
	}
};

