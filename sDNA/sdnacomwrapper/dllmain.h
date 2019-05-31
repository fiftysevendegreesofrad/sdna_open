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

// dllmain.h : Declaration of module class.

class CsdnacomwrapperModule : public CAtlDllModuleT< CsdnacomwrapperModule >
{
public :
	DECLARE_LIBID(LIBID_sdnacomwrapperLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_SDNACOMWRAPPER, "{C34461CD-AA35-44B4-B6DF-5416C2A77B1D}")
};

extern class CsdnacomwrapperModule _AtlModule;
