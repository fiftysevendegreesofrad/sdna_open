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

// SpatialNet.h : Declaration of the CSpatialNet

#pragma once
#include "resource.h"       // main symbols

#include "sdnacomwrapper_i.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

// CSpatialNet

class ATL_NO_VTABLE CSpatialNet :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSpatialNet, &CLSID_SpatialNet>,
	public IDispatchImpl<ISpatialNet, &IID_ISpatialNet, &LIBID_sdnacomwrapperLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
	Net *net;
	NetIterator *netiterator;
	long start_gsindex,end_gsindex,isislandindex;
	
public:
	CSpatialNet():net(NULL),netiterator(NULL)
	{
	}
	CSpatialNet(Net *net):net(net),netiterator(NULL) {}
	~CSpatialNet()
	{
		if (netiterator != NULL)
			net_iterator_destroy(netiterator);

		net_destroy(net);
	}

	Net* get_net() {return net;}
	
DECLARE_REGISTRY_RESOURCEID(IDR_SPATIALNET)


BEGIN_COM_MAP(CSpatialNet)
	COM_INTERFACE_ENTRY(ISpatialNet)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()



	DECLARE_PROTECT_FINAL_CONSTRUCT()

	HRESULT FinalConstruct()
	{
		return S_OK;
	}

	void FinalRelease()
	{
	}

public:

	STDMETHOD(add_link)(LONG arcid, VARIANT xs, VARIANT ys, DOUBLE start_gs, DOUBLE end_gs, DOUBLE activity_weight, DOUBLE custom_cost,VARIANT_BOOL isisland);
	STDMETHOD(print)(void);
	STDMETHOD(configure)(IUnknown* sdnawindow);
	STDMETHOD(reset_iterator)(void);
	STDMETHOD(iterator_next)(VARIANT_BOOL *success,VARIANT* xs, VARIANT* ys, DOUBLE* start_gs, DOUBLE* end_gs, VARIANT_BOOL *isisland);
	STDMETHOD(get_split_links)(VARIANT* ids);
	STDMETHOD(fix_split_links)(void);
	STDMETHOD(get_traffic_islands)(VARIANT* ids);
	STDMETHOD(fix_traffic_islands)(void);
	STDMETHOD(get_subsystems)(BSTR* message, VARIANT* ids);
	STDMETHOD(fix_subsystems)(void);
	STDMETHOD(get_duplicates)(VARIANT* ids,VARIANT* ids2);
	STDMETHOD(fix_duplicates)(void);
	STDMETHOD(import_from_link_unlink_format)();
	STDMETHOD(add_unlink)(VARIANT xs_array, VARIANT ys_array);
};

OBJECT_ENTRY_AUTO(__uuidof(SpatialNet), CSpatialNet)
