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
#include "geos_sdna_wrapper.h"

void my_geos_message_handler(const char *fmt, ...)
{
	#ifdef _SDNADEBUG
		va_list args;
		va_start( args, fmt );
		vprintf( fmt, args );
		va_end( args );
		printf( "\n" );
	#endif
}

#undef CLUSTERFINDER_DEBUG

vector<GEOSGeometry*> unlink_respecting_planarize(ExplicitSDNAPolylineToGeosWrapper &geos,vector<GEOSGeometry *> links,vector<GEOSGeometry *> unlinks_in)
{
	GEOSGeometry *network, *unlinks;
	
	if (links.size())
		network = geos.Geom_createCollection(GEOS_MULTILINESTRING,&links[0],numeric_cast<long>(links.size())); //takes ownership of line objects but not array of pointers
	else
		network = geos.Geom_createEmptyCollection(GEOS_MULTILINESTRING);

	if (unlinks_in.size())
		unlinks = geos.Geom_createCollection(GEOS_MULTIPOLYGON,&unlinks_in[0],numeric_cast<long>(unlinks_in.size()));
	else
		unlinks = geos.Geom_createEmptyCollection(GEOS_MULTIPOLYGON);

	//the output is saved as vector rather than GEOSCollection as GEOSUnion (which performs automatic noding) is useless to us
	vector<GEOSGeometry*> result;
	
	//first add the intersection of network and unlinks, un-noded
	//but "GEOSGeometry *unlink_network = GEOSIntersection(network,unlinks)" will emerge noded
	//so instead we iterate through each existing link individually intersecting with unlinks
	for (vector<GEOSGeometry*>::iterator i=links.begin();i<links.end();i++)
	{
		GEOSGeometry *g = geos.Intersection(*i,unlinks);  //result must be freed
		if (!geos.isEmpty(g))
			result.push_back(g);
	}

	//now remove unlinks from the rest of the network and node it
	GEOSGeometry *noded_network;
	if (unlinks_in.size())
		noded_network = geos.Difference(network,unlinks); //noding happens automatically here providing unlinks exist
	else
		noded_network = geos.UnaryUnion(network); //noding happens automatically here
	
	//alas, though, there's no way to tell which input lines map to which output ones afaik
	//at first glance it looks like order is preserved, but if links have overlapping sections, it isn't
	//so for now we discard that information and don't preserve data

	int num_links = geos.GetNumGeometries(noded_network);
	for (int i=0;i<num_links;i++)
	{
		GEOSGeometry * g = geos.GetGeometryN(noded_network,i); //return value doesn't need freeing
		result.push_back(geos.Geom_clone(g)); //this must be freed
	}	

	geos.Geom_destroy(network);
	geos.Geom_destroy(unlinks);
	geos.Geom_destroy(noded_network);
	
	return result; //callee must destroy all geoms in result
}

GEOSCoordSequence *point_vector_to_coordseq2d(ExplicitSDNAPolylineToGeosWrapper &geos,vector<Point> &pv)
{
	GEOSCoordSequence *coords = geos.CoordSeq_create(numeric_cast<unsigned long>(pv.size()),2); //2d
	for (vector<Point>::iterator p=pv.begin();p!=pv.end();p++)
	{
		unsigned long index = numeric_cast<unsigned long>(p-pv.begin());
		geos_ensure_succeeded(geos.CoordSeq_setX(coords,index,p->x));
		geos_ensure_succeeded(geos.CoordSeq_setY(coords,index,p->y));
	}
	return coords; //must be freed or ownership assumed
}

vector<Point> linestring_to_pointvector(ExplicitSDNAPolylineToGeosWrapper &geos,const GEOSGeometry* geom)
{
	vector<Point> result;
	int type = geos.GeomTypeId(geom);
	assert(type==GEOS_LINESTRING);
	long num_points = geos.GeomGetNumPoints(geom);
	for (long i=0;i<num_points;i++)
	{
		GEOSGeometry *p = geos.GeomGetPointN(geom,i); //must free return arg
		double x,y;
		geos.GeomGetX(p, &x);
		geos.GeomGetY(p, &y);
		result.push_back(Point(x,y,0));
		geos.Geom_destroy(p); //oh look it's freed here
	}
	return result;
}

vector<vector<Point>> unlink_respecting_planarize(vector<vector<Point>> &links, vector<vector<Point>> &unlinks)
{
	//wrapper for equivalent GEOS routine
	ExplicitSDNAPolylineToGeosWrapper geos;
	geos.initGEOS(&my_geos_message_handler,&my_geos_message_handler);

	//pack data
	vector<GEOSGeometry*> geoslinks;
	for (vector<vector<Point>>::iterator it=links.begin();it!=links.end();it++)
	{
		GEOSCoordSequence *coords = point_vector_to_coordseq2d(geos,*it);
		GEOSGeometry * line = geos.Geom_createLineString(coords); //line assumes ownership of coords
		geoslinks.push_back(line);
	}

	vector<GEOSGeometry*> geosunlinks;
	for (vector<vector<Point>>::iterator it=unlinks.begin();it!=unlinks.end();it++)
	{
		GEOSCoordSequence *coords = point_vector_to_coordseq2d(geos,*it);
		GEOSGeometry * ring = geos.Geom_createLinearRing(coords); //ring assumes ownership of coords
		GEOSGeometry * poly = geos.Geom_createPolygon(ring,NULL,0); //poly assumes ownership of ring
		geosunlinks.push_back(poly);
	}

	//call geos routine
	vector<GEOSGeometry*> geosresult = unlink_respecting_planarize(geos,geoslinks,geosunlinks);

	//unpack result, freeing geosresults
	vector<vector<Point>> result;
	for (vector<GEOSGeometry*>::iterator it=geosresult.begin();it!=geosresult.end();it++)
	{
		const int type = geos.GeomTypeId(*it);
		if (!(type==GEOS_LINESTRING || type==GEOS_MULTILINESTRING))
		{
			assert (type==GEOS_POINT || type==GEOS_MULTIPOINT); 
			//we may get these when links end exactly on unlinks, but we don't care
			continue; 
		}
		if (type==GEOS_LINESTRING)
			result.push_back(linestring_to_pointvector(geos,*it));
		else
		{
			//multilinestring occurs when one link intersects the unlink set multiple times
			const long num_geoms = geos.GetNumGeometries(*it);
			for (long i=0;i<num_geoms;i++)
			{
				GEOSGeometry *g = geos.GetGeometryN(*it,i); //doesn't need freeing
				result.push_back(linestring_to_pointvector(geos,g));
			}
		}
		geos.Geom_destroy(*it);
	}

	//links and unlinks had ownership assumed by inner call to unlink_respecting_planarize

	geos.finishGEOS();
	return result;
}
/*
void detect_unlinks(GEOSGeometry **line_array,int array_length)
{
	//to reverse the process we intersect each line with the entire network, then any that come out split reveal unlinks at the split points
	//shame this will be slow
	//if we just intersected entire network with itself, we could only identify unlinks by their non-presence in the output (hard)
	GEOSGeometry *network = GEOSGeom_createCollection(GEOS_MULTILINESTRING,line_array,array_length); //takes ownership of line objects but not array of pointers
	
	//keep track of unlinks too so we can subtract them from network and check output polys don't overlap anything else

	vector<GEOSGeometry*> unlink_points;
	vector<GEOSGeometry*> non_unlinks;
	for (long i=0;i<array_length;i++)
	{
		GEOSGeometry *g = GEOSIntersection(line_array[i],network);
		if ((int)GEOSGeomTypeId(g)==GEOS_MULTILINESTRING) //i.e. if it got split
		{
			//then it contains one or more unlinks 
			//record endpoint of every line except last as an unlink
			long num_lines = GEOSGetNumGeometries(g);
			GEOSGeometry *last_line_end = GEOSGeomGetStartPoint(GEOSGetGeometryN(g,0));
			for (int i=0;i<num_lines-1;i++) 
			{
				const GEOSGeometry *line = GEOSGetGeometryN(g,i); //return args don't need freeing
				GEOSGeometry *start = GEOSGeomGetStartPoint(line); //point does need freeing
				GEOSGeometry *end = GEOSGeomGetEndPoint(line); //does need freeing, but will be absorbed by createCollection below
				
				//check line segments come in order
				assert(GEOSEquals(last_line_end,start));
				GEOSGeom_destroy(last_line_end);
				last_line_end = start;

				unlink_points.push_back(end);
			}
			GEOSGeom_destroy(last_line_end);
		}
		else
		{
			//it isn't an unlink
			non_unlinks.push_back(GEOSGeom_clone(line_array[i])); //does need freeing, but will be absorbed by createCollection below
		}
		GEOSGeom_destroy(g);
	}
	GEOSGeom_destroy(network);

	//at this point we still need to free non_unlinks and unlink_points, which happen in different ways depending on circumstance (ugh)
	if (unlink_points.size()==0)
	{
		for (vector<GEOSGeometry*>::iterator it=non_unlinks.begin();it<non_unlinks.end();++it)
			GEOSGeom_destroy(*it);
		return;
	}
	
	GEOSGeometry *unlink_point_geom = GEOSGeom_createCollection(GEOS_MULTIPOINT,&unlink_points[0],unlink_points.size());

	//but anything touching an unlink point is itself a T junction unlink
	const GEOSPreparedGeometry *unlinks_prepared = GEOSPrepare(unlink_point_geom); //does not take ownership
	vector<GEOSGeometry*>::iterator it=non_unlinks.begin();
	while (it<non_unlinks.end())
	{
		if (GEOSPreparedTouches(unlinks_prepared,*it)) 
		{
			GEOSGeom_destroy(*it);
			it = non_unlinks.erase(it);
		}
		else
			++it;
	}
	GEOSPreparedGeom_destroy(unlinks_prepared);
	
	double unlink_poly_size;
	if (non_unlinks.size()==0)
		unlink_poly_size = 1; // it doesn't matter, as everything is an unlink
	else
	{
		//make a geometry out of the non_unlinks
		//both the following take ownership of objects but not array of pointers
		GEOSGeometry *non_unlink_geom = GEOSGeom_createCollection(GEOS_MULTILINESTRING,&non_unlinks[0],non_unlinks.size()); 
		GEOSDistance(unlink_point_geom, non_unlink_geom, &unlink_poly_size);
		unlink_poly_size /= 5.;
		GEOSGeom_destroy(non_unlink_geom);
	}
	
	//now we just need to free unlink_point_geom
	//but first, make buffers
	GEOSGeometry *result = GEOSBuffer(unlink_point_geom,unlink_poly_size,1);
	
	if ((int)GEOSGeomTypeId(result)!=GEOS_MULTIPOLYGON)
	{
		//must be a single poly
		result = GEOSGeom_createCollection(GEOS_MULTIPOLYGON,&result,1); //takes ownership
	}
	
	long num_polys = GEOSGetNumGeometries(result);
	for (int i=0;i<num_polys;i++)
		poly_to_pointvector(GEOSGetGeometryN(result,i)); //return args don't need freeing

	GEOSGeom_destroy(unlink_point_geom);
	GEOSGeom_destroy(result);
}
*/