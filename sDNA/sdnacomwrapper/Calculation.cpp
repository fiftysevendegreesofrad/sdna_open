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

// Calculation.cpp : Implementation of CCalculation

#include "stdafx.h"
#include "Calculation.h"
#include "SDNAWindow.h"
#include "globalwindow.h"
#include "SpatialNet.h"
#include "utils.h"
#include "..\sdna_vs2008\sdna.h"
#include "DispatchPtr.h"
#include "comutil.h"

// CCalculation
STDMETHODIMP CCalculation::configure(VARIANT config_bs, 
									 IUnknown* in_net, 
									 IUnknown* sdnawindow)
{
	globalsdnawindow = ((CSDNAWindow*)sdnawindow);
	net = ((CSpatialNet*)in_net)->get_net();

	char *config_cs = _com_util::ConvertBSTRToString(config_bs.bstrVal);
	
	calc = integral_calc_create(net,
								config_cs,
								&progressor_callback,
								&warning_callback);

	delete[] config_cs;

	if (calc)
	{
		output_length = icalc_get_output_length(calc);
		output_buffer = new float[output_length];
		return S_OK;
	}
	else
	{
		globalsdnawindow->allow_close();
		return E_ABORT;
	}
}

//note this doesn't check that the pointer handed to it is actually a net - dodgy eh
STDMETHODIMP CCalculation::run()
{
	assert (calc != NULL);
	
	int result = calc_run((Calculation*)calc);

	globalsdnawindow->set_to(100);
	warning_callback("\nDone.");
	globalsdnawindow->allow_close();
		
	if (result==1)	return S_OK; else return E_ABORT;
}

STDMETHODIMP CCalculation::get_output_shortnames(VARIANT* names)
{
	char** names_array = icalc_get_short_output_names(calc);
	string_array_to_bstr_safearray_variant(output_length,names_array,names);
	return S_OK;
}

STDMETHODIMP CCalculation::get_output_names(VARIANT* names)
{
	char** names_array = icalc_get_all_output_names(calc);
	string_array_to_bstr_safearray_variant(output_length,names_array,names);
	return S_OK;
}

STDMETHODIMP CCalculation::get_all_outputs(LONG id, VARIANT* outputs)
{
	assert(output_buffer != NULL);
	icalc_get_all_outputs(calc,output_buffer,id);
	float_array_to_double_safearray_variant(output_length,output_buffer,outputs);
	return S_OK;
}

