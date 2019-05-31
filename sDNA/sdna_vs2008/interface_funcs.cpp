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
#include "net.h"
#include "calculation.h"
#include "prepareoperations.h"
#include "sdna.h"
#include "sDNACalculationFactory.h"
#include "tables.h"

//thinly veiled OO access methods to export

//note the predominance of casting size_t to long in this file
//sdna specs do not currently permit trying to put bigger numbers than longs through the interface
//most internal handling is now done by size_t though with some exceptions (GEOS library, count junctions to name two)

//for nets...

SDNA_API Net* __stdcall net_create() 
{
	return new Net();
}

SDNA_API int __stdcall net_add_polyline_3d(Net *c,long arcid,int geom_length,double *xs, double *ys, float *zs) 
{ 
	if (geom_length > 1)
	{
		bool success = c->add_polyline(arcid,geom_length,xs,ys,zs);
		return success ? 1 : 0;
	}
	else
		return 0;
}
SDNA_API int __stdcall net_add_polyline(Net *c,long arcid,int geom_length,double *xs, double *ys) 
{ 
	return net_add_polyline_3d(c,arcid,geom_length,xs,ys,NULL);
}

SDNA_API int __stdcall net_add_polyline_data(Net *n,long arcid,char *name,float data)
{
	if (n->add_polyline_data(arcid,string(name),data))
		return 1;
	else
		return 0;
}

SDNA_API int __stdcall net_add_polyline_text_data(Net *n,long arcid,char *name,char *data)
{
	if (n->add_polyline_text_data(arcid,string(name),data))
		return 1;
	else
		return 0;
}

SDNA_API int __stdcall net_add_unlink(Net *c,int geom_length,double *xs, double *ys)
{ 
	if (geom_length > 3)
	{
		c->add_unlink(geom_length,xs,ys);
		return 1;
	}
	else
		return 0;
}

SDNA_API int __stdcall net_reserve(Net *n,long num_items)
{
	if (n->reserve(num_items)) return 1; else return 0;
}

SDNA_API void __stdcall net_print(Net *c) { c->print(); }
SDNA_API void __stdcall net_destroy(Net *c) { delete c; }

//for calculations...

SDNA_API SDNAIntegralCalculation* __stdcall integral_calc_create(Net *net,char *config,
															int (__cdecl *set_progressor_callback)(float),
															int (__cdecl *print_warning_callback)(const char*))
{
	try
	{
		return new SDNAIntegralCalculation(net,config,
											set_progressor_callback, print_warning_callback); 
	}
	catch (BadConfigException e)
	{
		stringstream error;
		error << "ERROR: " << e.what();
		print_warning_callback(error.str().c_str());
		return NULL;
	}
}

SDNA_API PrepareOperation* __stdcall prep_create(Net *net,char *config,int (__cdecl *print_warning_callback)(const char*))
{
	try
	{
		return new PrepareOperation(net,config,print_warning_callback);
	}
	catch (BadConfigException e)
	{
		stringstream error;
		error << "ERROR: " << e.what();
		print_warning_callback(error.str().c_str());
		return NULL;
	}
}

SDNA_API int __stdcall calc_run(Calculation *c) { if (c->run()) return 1; else return 0; }

SDNA_API int __stdcall icalc_get_output_length(SDNAIntegralCalculation *c) {return (int)c->get_output_length();}
SDNA_API char** __stdcall icalc_get_all_output_names(SDNAIntegralCalculation *c) {return c->get_all_output_names_c();}
SDNA_API char** __stdcall icalc_get_short_output_names(SDNAIntegralCalculation *c) {return c->get_short_output_names_c();}
SDNA_API void __stdcall icalc_get_all_outputs(SDNAIntegralCalculation *c,float* buffer,long arcid) {return c->get_all_outputs_c(buffer,arcid);}

SDNA_API void __stdcall calc_destroy(Calculation *c) { delete c; }

SDNA_API long __stdcall net_get_num_items(Net *n)
{
	return numeric_cast<long>(n->num_items());
}
SDNA_API NetIterator* __stdcall net_create_iterator(Net *n)
{
	return new NetIterator(n);
}
SDNA_API long __stdcall net_get_data_names(Net *n,char ***names)
{
	return numeric_cast<long>(n->get_names(names));
}
SDNA_API int __stdcall net_iterator_next(NetIterator* it, long *arcid,long *geom_length,double **point_array_x,double **point_array_y,float **data)
{
	return it->next(arcid,geom_length,point_array_x,point_array_y,NULL,data);
}
SDNA_API int __stdcall net_iterator_next_3d(NetIterator* it, long *arcid,long *geom_length,double **point_array_x,double **point_array_y,float **point_array_z,float **data)
{
	return it->next(arcid,geom_length,point_array_x,point_array_y,point_array_z,data);
}
SDNA_API void __stdcall net_iterator_destroy(NetIterator *it)
{
	delete it;
}

SDNA_API long __stdcall prep_get_split_link_ids(PrepareOperation *p,long **output)
{
	return numeric_cast<long>(p->get_split_link_ids(output));
}
SDNA_API long __stdcall prep_fix_split_links(PrepareOperation *p)
{
	return numeric_cast<long>(p->fix_split_links());
}
SDNA_API long __stdcall prep_get_traffic_islands(PrepareOperation *p,long **output)
{
	return numeric_cast<long>(p->get_traffic_islands(output));
}
SDNA_API long __stdcall prep_fix_traffic_islands(PrepareOperation *p)
{
	return numeric_cast<long>(p->fix_traffic_islands());
}
SDNA_API long __stdcall prep_get_duplicate_links(PrepareOperation *p,long **duplicates,long **originals)
{
	return numeric_cast<long>(p->get_duplicate_links(duplicates,originals));
}
SDNA_API long __stdcall prep_fix_duplicate_links(PrepareOperation *p)
{
	return numeric_cast<long>(p->fix_duplicate_links());
}
SDNA_API long __stdcall prep_get_subsystems(PrepareOperation *p,char **message,long **links)
{
	return numeric_cast<long>(p->get_subsystems(message,links));
}
SDNA_API long __stdcall prep_fix_subsystems(PrepareOperation *p)
{
	return numeric_cast<long>(p->fix_subsystems());
}
SDNA_API long __stdcall prep_get_near_miss_connections(PrepareOperation *p,long **links)
{
	return numeric_cast<long>(p->get_near_miss_connections(links));
}
SDNA_API long __stdcall prep_fix_near_miss_connections(PrepareOperation *p)
{
	return numeric_cast<long>(p->fix_near_miss_connections());
}
SDNA_API int __stdcall prep_bind_network_data(PrepareOperation *p)
{
	return (p->bind_network_data())? 1 : 0;
}

SDNA_API Net* __stdcall prep_import_from_link_unlink_format(PrepareOperation *p)
{
	return p->import_from_link_unlink_format();
}

SDNA_API long __stdcall calc_expected_data_net_only(Calculation *c,char*** names)
{
	return numeric_cast<long>(c->expected_data_net_only(names));
}

SDNA_API long __stdcall calc_expected_text_data(Calculation *c,char*** names)
{
	return numeric_cast<long>(c->expected_text_data(names));
}

SDNA_API long __stdcall calc_get_num_geometry_outputs(Calculation *c)
{
	return numeric_cast<long>(c->get_num_geometry_outputs());
}
SDNA_API sDNAGeometryCollectionBase* __stdcall calc_get_geometry_output(Calculation *c,long i)
{
	return c->get_geometry_output(i);
}

SDNA_API const char* __stdcall geom_get_name(sDNAGeometryCollectionBase *g)
{
	return g->get_name();
}
SDNA_API const char* __stdcall geom_get_type(sDNAGeometryCollectionBase *g)
{
	return g->get_type();
}

SDNA_API long __stdcall geom_get_field_metadata(sDNAGeometryCollectionBase *g,char*** names,char*** shortnames,char ***pythontypes)
{
	return numeric_cast<long>(g->get_field_metadata(names,shortnames,pythontypes));
}
SDNA_API long __stdcall geom_get_num_items(sDNAGeometryCollectionBase *g)
{
	return numeric_cast<long>(g->get_num_items());
}

SDNA_API sDNAGeometryCollectionIteratorBase* __stdcall geom_iterator_create(sDNAGeometryCollectionBase *g)
{
	sDNAGeometryCollectionIteratorBase* it = g->getIterator();
	//std::cout << "created gc iterator " << it << std::endl;
	return it;
}
SDNA_API int __stdcall geom_iterator_next(sDNAGeometryCollectionIteratorBase *it,long *num_parts,SDNAOutputUnion **data)
{
	//std::cout << "received gc iterator " << it << std::endl;
	return it->next(num_parts,data); //"it" got given invalid address
}
SDNA_API long __stdcall geom_iterator_getpart(sDNAGeometryCollectionIteratorBase *it,long index,double** point_array_x,double** point_array_y,float **point_array_z)
{
	return numeric_cast<long>(it->getpart(index,point_array_x,point_array_y,point_array_z));
}
SDNA_API void __stdcall geom_iterator_destroy(sDNAGeometryCollectionIteratorBase *it)
{
	delete it;
}

SDNA_API Calculation* __stdcall calc_create(char *name, char *config, Net *net,
									  int (__cdecl *set_progressor_callback)(float),
									  int (__cdecl *print_warning_callback)(const char*),
									  vector<shared_ptr<Table<float>>>* tables1d)
{
	try
	{
		Calculation* c = sDNACalculationFactory(name,config,net,set_progressor_callback,print_warning_callback,tables1d);
		return c;
	}
	catch (BadConfigException e)
	{
		stringstream error;
		error << "ERROR: " << e.what();
		print_warning_callback(error.str().c_str());
		return NULL;
	}
}

SDNA_API int __stdcall calc_add_table2d(Calculation *c,Table2d *t) { return c->add_table2d(t) ? 0 : 1; }

SDNA_API Table<float>* __stdcall table_create(char *name,char *zonefieldname) { return new Table<float>(name, zonefieldname); }
SDNA_API Table2d* __stdcall table2d_create(char *name,char *origzonefieldname,char *destzonefieldname) { return new Table2d(name,origzonefieldname,destzonefieldname); }
SDNA_API int __stdcall table_addrow(Table<float> *t,char *zone,float data) { return t->addrow(zone,data);}
SDNA_API int __stdcall table2d_addrow(Table2d *t,char *fromzone,char *tozone,float data) { return t->addrow(fromzone,tozone,data);}

SDNA_API vector<shared_ptr<Table<float>>>* __stdcall table_collection_create() { return new vector<shared_ptr<Table<float>>>(); }
SDNA_API int __stdcall table_collection_add_table(vector<shared_ptr<Table<float>>>* collection,Table<float>* table)
{
	collection->push_back(shared_ptr<Table<float>>(table));
	return 0;
}
