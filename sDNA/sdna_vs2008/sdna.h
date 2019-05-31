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

#pragma once
#ifdef SDNA_EXPORTS

#define SDNA_API extern "C" __declspec(dllexport) 

#else

#define SDNA_API extern "C" __declspec(dllimport) 
#include <boost/shared_ptr.hpp>
#include <vector>

#endif

class Net;
class Calculation;
class NetIterator;
class Calculation;
class PrepareOperation;
class SDNAIntegralCalculation;
class sDNAGeometryCollectionBase;
class sDNAGeometryCollectionIteratorBase;
class sDNAGeometryCollection;
template <class T> class Table;
class Table2d;
union SDNAOutputUnion;
using namespace std;
using namespace boost;

SDNA_API Net* __stdcall net_create() ;
SDNA_API int __stdcall net_add_polyline(Net *c,long arcid,int geom_length,double *xs, double *ys); //deprecated but used by autocad
SDNA_API int __stdcall net_add_polyline_3d(Net *c,long arcid,int geom_length,double *xs, double *ys, float *zs);
SDNA_API int __stdcall net_add_polyline_data(Net *n,long arcid,char *name,float data);
SDNA_API int __stdcall net_add_polyline_text_data(Net *n,long arcid,char *name,char *data);
SDNA_API int __stdcall net_add_unlink(Net *c,int geom_length,double *xs, double *ys); //not used except in import_from_link_unlink_format
SDNA_API void __stdcall net_print(Net *c) ;
SDNA_API void __stdcall net_destroy(Net *c) ;

SDNA_API int __stdcall net_reserve(Net *n,long num_items);

SDNA_API long __stdcall net_get_num_items(Net *n);
SDNA_API NetIterator* __stdcall net_create_iterator(Net *n);
SDNA_API long __stdcall net_get_data_names(Net *n,char ***names);
SDNA_API int __stdcall net_iterator_next(NetIterator* it, long *arcid,long *geom_length,double **point_array_x,double **point_array_y,float **data); //left for autocad
SDNA_API int __stdcall net_iterator_next_3d(NetIterator* it, long *arcid,long *geom_length,double **point_array_x,double **point_array_y,float **point_array_z,float **data);
SDNA_API void __stdcall net_iterator_destroy(NetIterator *it);

SDNA_API Calculation* __stdcall calc_create(char *name, char *config, Net *net,
									  int (__cdecl *set_progressor_callback)(float),
									  int (__cdecl *print_warning_callback)(const char*),
									  vector<shared_ptr<Table<float>>>* tables1d=NULL);

//deprecated, kept for autocad, use factory method now (calc_create)
SDNA_API SDNAIntegralCalculation* __stdcall integral_calc_create(Net *n, char *config,
															int (__cdecl *set_progressor_callback)(float),
															int (__cdecl *print_warning_callback)(const char*));

SDNA_API int __stdcall calc_add_table2d(Calculation *c,Table2d *t); //assumes ownership

SDNA_API int __stdcall calc_run(Calculation *c) ;

SDNA_API long __stdcall calc_get_num_geometry_outputs(Calculation *c);
SDNA_API sDNAGeometryCollectionBase* __stdcall calc_get_geometry_output(Calculation *c,long i);

SDNA_API const char* __stdcall geom_get_name(sDNAGeometryCollectionBase *g);
SDNA_API const char* __stdcall geom_get_type(sDNAGeometryCollectionBase *g);

SDNA_API long __stdcall geom_get_field_metadata(sDNAGeometryCollectionBase *g,char*** names,char*** shortnames,char*** types);
SDNA_API long __stdcall geom_get_num_items(sDNAGeometryCollectionBase *g);

SDNA_API sDNAGeometryCollectionIteratorBase* __stdcall geom_iterator_create(sDNAGeometryCollectionBase *g);
SDNA_API int __stdcall geom_iterator_next(sDNAGeometryCollectionIteratorBase *it,long *num_parts,SDNAOutputUnion **data);
SDNA_API long __stdcall geom_iterator_getpart(sDNAGeometryCollectionIteratorBase *it,long index,double** point_array_x,double** point_array_y,float **point_array_z);
SDNA_API void __stdcall geom_iterator_destroy(sDNAGeometryCollectionIteratorBase *it);

//the following are deprecated and left here for autocad support and testing
//recommended usage is now to get calculation outputs through the GeometryCollection interface above
SDNA_API int __stdcall icalc_get_output_length(SDNAIntegralCalculation *c);
SDNA_API char** __stdcall icalc_get_all_output_names(SDNAIntegralCalculation *c) ;
SDNA_API char** __stdcall icalc_get_short_output_names(SDNAIntegralCalculation *c) ;
SDNA_API void __stdcall icalc_get_all_outputs(SDNAIntegralCalculation *c,float* buffer,long arcid) ; //caller must allocate buffer unlike other methods above

SDNA_API void __stdcall calc_destroy(Calculation *c);

//deprecated, kept for autocad, use factory method now
//ditto all the prep_ methods below
SDNA_API PrepareOperation* __stdcall prep_create(Net *n,char *config,int (__cdecl *print_warning_callback)(const char*));
SDNA_API int __stdcall prep_bind_network_data(PrepareOperation *p);
SDNA_API long __stdcall prep_get_split_link_ids(PrepareOperation *p,long **output);
SDNA_API long __stdcall prep_fix_split_links(PrepareOperation *p);
SDNA_API long __stdcall prep_get_traffic_islands(PrepareOperation *p,long **output);
SDNA_API long __stdcall prep_fix_traffic_islands(PrepareOperation *p);
SDNA_API long __stdcall prep_get_duplicate_links(PrepareOperation *p,long **duplicates,long **originals);
SDNA_API long __stdcall prep_fix_duplicate_links(PrepareOperation *p);
SDNA_API long __stdcall prep_get_subsystems(PrepareOperation *p,char **message,long **links);
SDNA_API long __stdcall prep_fix_subsystems(PrepareOperation *p);
SDNA_API long __stdcall prep_get_near_miss_connections(PrepareOperation *p,long **links);
SDNA_API long __stdcall prep_fix_near_miss_connections(PrepareOperation *p);
SDNA_API Net* __stdcall prep_import_from_link_unlink_format(PrepareOperation *p);

SDNA_API long __stdcall calc_expected_data_net_only(Calculation *c,char*** names);
SDNA_API long __stdcall calc_expected_text_data(Calculation *c,char*** names);

SDNA_API Table<float>* __stdcall table_create(char *name, char *zonefieldname);
SDNA_API Table2d* __stdcall table2d_create(char *name,char *origfieldname,char *destfieldname);
SDNA_API int __stdcall table_addrow(Table<float> *t,char *zone,float data);
SDNA_API int __stdcall table2d_addrow(Table2d *t,char *fromzone,char *tozone,float data);

SDNA_API vector<shared_ptr<Table<float>>>* __stdcall table_collection_create();
SDNA_API int __stdcall table_collection_add_table(vector<shared_ptr<Table<float>>>*,Table<float>*); //assumes ownership (and calc will take it from there)

SDNA_API void __stdcall run_unit_tests();

