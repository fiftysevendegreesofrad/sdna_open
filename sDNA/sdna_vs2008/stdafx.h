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

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define _SCL_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#ifndef _DEBUG
#define _SECURE_SCL 0 //disable iterator bounds checking for release builds
#endif

// Windows Header Files:
#define NOMINMAX
#include <windows.h>
#include <wininet.h>
#include <stdlib.h>

#include <vector>
#include "IteratorTypeErasure\any_iterator\any_iterator.hpp"
#include <iostream>
#include <limits>
#include <set>
#include <cassert>
#include <string>
#include <map>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/variant.hpp>
#include <boost/bimap.hpp>
using boost::numeric_cast;
#include <utility>
#define _USE_MATH_DEFINES
#include <cmath>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/sign.hpp>
#include <ctime>
#include <numeric>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <functional>
#include <boost/iterator/filter_iterator.hpp>
#include <stdexcept>
#include <cstdio>
#include <cstdarg>
#include <boost/thread.hpp>

#include <boost/static_assert.hpp>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include <boost/geometry/multi/geometries/multi_point.hpp>
#include <boost/geometry/geometries/linestring.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/foreach.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include <boost/random/normal_distribution.hpp>


#define UNICODE_SAVE _UNICODE
#undef _UNICODE
#define MUP_BASETYPE float
#include "muparser.h"
#define _UNICODE UNICODE_SAVE

using boost::geometry::model::d2::point_xy;
typedef boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian> point_xyz;
typedef boost::geometry::model::linestring<point_xy<double> > BoostLineString2d; 
typedef boost::geometry::model::linestring<point_xyz > BoostLineString3d; 

#ifdef _OPENMP
#include <omp.h>
#define OMP_THREAD omp_get_thread_num()
#define OMP_NUM_THREADS omp_get_num_threads()
#else
#define OMP_THREAD 0
#define OMP_NUM_THREADS 1
#endif

#ifdef _MSC_VER // msvc compiler
#include <omp.h>
#endif

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SDNA_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// SDNA_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#define SDNA_EXPORTS

#ifndef _SDNADEBUG
#define DEBUG_PRINT(x)
#else
#define DEBUG_PRINT(x) std::cout << x << std::endl;
#endif
