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

// SpatialNet.cpp : Implementation of CSpatialNet

#include "stdafx.h"
#include "SpatialNet.h"
#include "utils.h"
#include "SDNAWindow.h"
#include "globalwindow.h"

// CSpatialNet

bool to_bool(int x)
{
	return !(x==0);
}

STDMETHODIMP CSpatialNet::add_link(LONG arcid, VARIANT xs_array, VARIANT ys_array, DOUBLE start_gs, DOUBLE end_gs, DOUBLE activity_weight, DOUBLE custom_cost,
								   VARIANT_BOOL isisland)
{
	assert(net != NULL);

	CComSafeArray<double> xs, ys;
	xs.Attach(xs_array.parray);
	ys.Attach(ys_array.parray);
	ULONG geom_length = xs.GetCount();
	assert (geom_length == ys.GetCount());

	//unpack xs_array and ys_array into c-style arrays and call net_add_link
	double *xs_carray = new double[geom_length];
	double *ys_carray = new double[geom_length];
	for (unsigned long i=0;i<geom_length;i++)
	{
		xs_carray[i] = xs.GetAt(i);
		ys_carray[i] = ys.GetAt(i);
	}

	xs.Detach();
	ys.Detach();

	bool result = to_bool(net_add_polyline(net,arcid,geom_length,xs_carray,ys_carray));
	delete[] xs_carray;
	delete[] ys_carray;

	// for now we preserve the old interface to add_link externally
	// in future these will be factored out into a separate method on CCalculation which autocad must call
	result |= to_bool(net_add_polyline_data(net,arcid,"start_gs",(float)start_gs));
	result |= to_bool(net_add_polyline_data(net,arcid,"end_gs",(float)end_gs));
	result |= to_bool(net_add_polyline_data(net,arcid,"weight",(float)activity_weight));
	result |= to_bool(net_add_polyline_data(net,arcid,"custom_cost",(float)custom_cost));
	result |= to_bool(net_add_polyline_data(net,arcid,"is_island",(float)isisland));

	if (result)
		return S_OK;
	else
		return E_ABORT;
}

STDMETHODIMP CSpatialNet::add_unlink(VARIANT xs_array, VARIANT ys_array)
{
	assert(net != NULL);
	CComSafeArray<double> xs, ys;
	xs.Attach(xs_array.parray);
	ys.Attach(ys_array.parray);
	ULONG geom_length = xs.GetCount();
	assert (geom_length == ys.GetCount());

	//unpack xs_array and ys_array into c-style arrays and call net_add_unlink
	double *xs_carray = new double[geom_length];
	double *ys_carray = new double[geom_length];
	for (unsigned long i=0;i<geom_length;i++)
	{
		xs_carray[i] = xs.GetAt(i);
		ys_carray[i] = ys.GetAt(i);
	}

	xs.Detach();
	ys.Detach();

	int result = net_add_unlink(net,geom_length,xs_carray,ys_carray);
	delete[] xs_carray;
	delete[] ys_carray;

	if (result==1)
		return S_OK;
	else
		return E_ABORT;
}

STDMETHODIMP CSpatialNet::print(void)
{
	assert(net != NULL);
	net_print(net);
	return S_OK;
}

STDMETHODIMP CSpatialNet::configure(IUnknown* sdnawindow)
{
	assert(net==NULL);
	globalsdnawindow = ((CSDNAWindow*)sdnawindow);
	net = net_create();
	return S_OK;
}

STDMETHODIMP CSpatialNet::reset_iterator(void)
{
	if (netiterator != NULL)
		net_iterator_destroy(netiterator);

	netiterator = net_create_iterator(net);
	char **names;
	long num_names = net_get_data_names(net,&names);
	start_gsindex = -1;
	end_gsindex=-1;
	isislandindex=-1;
	for (long i=0;i<num_names;i++)
	{
		if (!strcmp(names[i],"start_gs"))
			start_gsindex=i;
		if (!strcmp(names[i],"end_gs"))
			end_gsindex=i;
		if (!strcmp(names[i],"is_island"))
			isislandindex=i;
	}
	return S_OK;
}

STDMETHODIMP CSpatialNet::iterator_next(VARIANT_BOOL *success,VARIANT* xs, VARIANT* ys, DOUBLE* start_gs, DOUBLE* end_gs, VARIANT_BOOL *is_island)
{
	assert (netiterator != NULL);

	long arcid; //discarded
	long geom_length; //used
	double *point_array_x; //used
	double *point_array_y; //used
	float *data;

	*success = net_iterator_next(netiterator,&arcid,&geom_length,&point_array_x,&point_array_y,&data);

	if (*success)
	{
		double_array_to_double_safearray_variant(geom_length,point_array_x,xs);
		double_array_to_double_safearray_variant(geom_length,point_array_y,ys);
		*start_gs = 0;
		*end_gs = 0;
		*is_island = 0;
		if (start_gsindex!=-1)
			*start_gs = data[start_gsindex];
		if (end_gsindex!=-1)
			*end_gs = data[end_gsindex];
		if (isislandindex!=-1)
			*is_island = (data[isislandindex]==1);
	}
	else
	{
		double EMPTY_ARRAY = 0;
		double_array_to_double_safearray_variant(0,&EMPTY_ARRAY,xs);
		double_array_to_double_safearray_variant(0,&EMPTY_ARRAY,ys);
		*start_gs = 0;
		*end_gs = 0;
		*is_island = false;
	}

	return S_OK;
}

STDMETHODIMP CSpatialNet::get_split_links(VARIANT* ids)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	long *output;
	long length = prep_get_split_link_ids(p,&output);
	long_array_to_long_safearray_variant(length,output,ids);
	//PrepareOperation inherits from Calculation so we can do this (ouch, though):
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::fix_split_links(void)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	prep_fix_split_links(p);
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::get_traffic_islands(VARIANT* ids)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	long *output;
	long length = prep_get_traffic_islands(p,&output);
	long_array_to_long_safearray_variant(length,output,ids);
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::fix_traffic_islands(void)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	prep_fix_traffic_islands(p);
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::get_subsystems(BSTR* message, VARIANT* ids)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	long *output;
	char *message_s;
	long length = prep_get_subsystems(p,&message_s,&output);
	long_array_to_long_safearray_variant(length,output,ids);
	*message = CComBSTR(message_s).Detach();
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::fix_subsystems(void)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	prep_fix_subsystems(p);
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::get_duplicates(VARIANT* ids,VARIANT* ids2)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	long *duplicates, *originals; 
	long length = prep_get_duplicate_links(p,&duplicates,&originals);
	long_array_to_long_safearray_variant(length,duplicates,ids);
	long_array_to_long_safearray_variant(length,originals,ids2);
	calc_destroy((Calculation*)p);
	return S_OK;
}

STDMETHODIMP CSpatialNet::fix_duplicates(void)
{
	assert (net != NULL);
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	prep_fix_duplicate_links(p);
	calc_destroy((Calculation*)p);
	return S_OK;
}
STDMETHODIMP CSpatialNet::import_from_link_unlink_format()
{
	PrepareOperation* p = prep_create(net,"start_gs=start_gs;end_gs=end_gs;island=is_island;internal_interface",&warning_callback);
	prep_bind_network_data(p);
	//com wrapper just replaces this net with the result
	Net* result = prep_import_from_link_unlink_format(p);
	net_destroy(net);
	net = result;
	calc_destroy((Calculation*)p);
	return S_OK;
}




