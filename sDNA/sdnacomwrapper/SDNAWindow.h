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

// SDNAWindow.h : Declaration of the CSDNAWindow

#pragma once
#include "resource.h"       // main symbols

#include "sdnacomwrapper_i.h"


#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif



// CSDNAWindow
#import "..\sDNAProgressBar\Release\sDNAProgressBar.tlb" no_namespace

class ATL_NO_VTABLE CSDNAWindow :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSDNAWindow, &CLSID_SDNAWindow>,
	public IDispatchImpl<ISDNAWindow, &IID_ISDNAWindow, &LIBID_sdnacomwrapperLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	_sDNAProgressBar *window; 
	CSDNAWindow() 
	{
		CLSID clsid;
		ATLENSURE_SUCCEEDED(CLSIDFromProgID(OLESTR("sDNAProgressBar.sDNAProgressBar"), &clsid));
		HRESULT hr = CoCreateInstance(clsid,NULL,CLSCTX_INPROC_SERVER,
								   __uuidof(_sDNAProgressBar),(LPVOID*) &window);
		ATLENSURE_SUCCEEDED(hr);
	}
	~CSDNAWindow()
	{
		//note window is not destroyed, this only happens when user closes it (maybe)
		ATLENSURE_SUCCEEDED(window->allow_close());
	}
	void set_to(float f)
	{
		ATLENSURE_SUCCEEDED(window->set_to(f));
	}
	void allow_close()
	{
		ATLENSURE_SUCCEEDED(window->allow_close());
	}
	void append_text(_bstr_t b)
	{
		ATLENSURE_SUCCEEDED(window->append_text(b));
	}

DECLARE_REGISTRY_RESOURCEID(IDR_SDNAWINDOW)


BEGIN_COM_MAP(CSDNAWindow)
	COM_INTERFACE_ENTRY(ISDNAWindow)
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

};

OBJECT_ENTRY_AUTO(__uuidof(SDNAWindow), CSDNAWindow)
