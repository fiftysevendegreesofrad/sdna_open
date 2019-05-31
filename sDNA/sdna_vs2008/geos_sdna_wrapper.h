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

using namespace std;

typedef void GEOSSTRtree;
typedef void GEOSCoordSequence;
typedef void GEOSGeometry;
typedef void GEOSEnvelope;

//begin paste from geos_c.h
enum GEOSGeomTypes {
    GEOS_POINT,
    GEOS_LINESTRING,
    GEOS_LINEARRING,
    GEOS_POLYGON,
    GEOS_MULTIPOINT,
    GEOS_MULTILINESTRING,
    GEOS_MULTIPOLYGON,
    GEOS_GEOMETRYCOLLECTION
};
//end paste from geos_c.h - note under 10 lines as per LGPL

#define geos_ensure_succeeded(x) {int result = ( x ); assert(result!=0);}

//this could be better expressed with preprocessor macros but I think we're slowly factoring out geos anyway
class ExplicitSDNAPolylineToGeosWrapper
{
private:
	ExplicitSDNAPolylineToGeosWrapper(const ExplicitSDNAPolylineToGeosWrapper& you_cant_copy_this_class) {};
public:
	//callbacks
	typedef void (*GEOSMessageHandler_t)(const char *fmt, ...);
	typedef void (*GEOSQueryCallback)(void *item, void *userdata);

	//function pointers
	typedef void (*initGEOS_t)(GEOSMessageHandler_t,GEOSMessageHandler_t);
	initGEOS_t initGEOS;
	typedef void (*finishGEOS_t)(void);
	finishGEOS_t finishGEOS;

	//functions
	GEOSCoordSequence* CoordSeq_create(unsigned int size,unsigned int dims) { return (*GEOSCoordSeq_create)(size,dims); }
	int CoordSeq_setX(GEOSCoordSequence* s,unsigned int idx,double val) { return (*GEOSCoordSeq_setX)(s,idx,val); }
	int CoordSeq_setY(GEOSCoordSequence* s,unsigned int idx,double val) { return (*GEOSCoordSeq_setY)(s,idx,val); }
	GEOSGeometry* Geom_createLineString(GEOSCoordSequence *s) { return (*GEOSGeom_createLineString)(s); }
	GEOSGeometry* Geom_createPoint(GEOSCoordSequence *s) { return (*GEOSGeom_createPoint)(s); }
	void Geom_destroy(GEOSGeometry *g) { (*GEOSGeom_destroy)(g); }
	GEOSSTRtree* STRtree_create(size_t nodeCapacity) { return (*GEOSSTRtree_create)(nodeCapacity); }
	void STRtree_insert(GEOSSTRtree *tree,const GEOSGeometry *g,void *item) { (*GEOSSTRtree_insert)(tree,g,item); }
	void STRtree_iterate(GEOSSTRtree *tree,GEOSQueryCallback callback,void *userdata) { (*GEOSSTRtree_iterate)(tree,callback,userdata); }
	void STRtree_query(GEOSSTRtree *tree,const GEOSGeometry *g,GEOSQueryCallback callback,void *userdata) { (*GEOSSTRtree_query)(tree,g,callback,userdata); }
	void STRtree_destroy(GEOSSTRtree *tree) { (*GEOSSTRtree_destroy)(tree); }
	GEOSGeometry* Envelope(GEOSGeometry* g1) { return (*GEOSEnvelope)(g1); }
	GEOSGeometry *Geom_createCollection(int type,GEOSGeometry* *geoms, unsigned int ngeoms) { return (*GEOSGeom_createCollection)(type,geoms,ngeoms); }
	GEOSGeometry *Geom_createEmptyCollection(int type) { return (*GEOSGeom_createEmptyCollection)(type); }
	GEOSGeometry *Intersection(GEOSGeometry *g1,GEOSGeometry* g2) { return (*GEOS_Intersection)(g1,g2); }
	char isEmpty(GEOSGeometry *g) { return (*GEOSisEmpty)(g); }
	GEOSGeometry *Difference(GEOSGeometry *g1,GEOSGeometry* g2) { return (*GEOSGeom_Difference)(g1,g2); }
	int GetNumGeometries(GEOSGeometry *g) {return (*GEOSGetNumGeometries)(g);}
	GEOSGeometry* GetGeometryN(GEOSGeometry *g,int n) {return (*GEOSGetGeometryN)(g,n);}
	GEOSGeometry* Geom_clone(GEOSGeometry *g) {return (*GEOSGeom_clone)(g);}
	int GeomGetNumPoints(const GEOSGeometry *g) {return (*GEOSGeomGetNumPoints)(g);}
	GEOSGeometry *GeomGetPointN(const GEOSGeometry *g1, int n) { return (*GEOSGeomGetPointN)(g1,n);}
	char GeomGetX (GEOSGeometry *g1,double *x) { return (*GEOSGeomGetX)(g1,x);}
	char GeomGetY (GEOSGeometry *g1,double *y) { return (*GEOSGeomGetY)(g1,y);}
	GEOSGeometry *Geom_createLinearRing(GEOSCoordSequence* s) {return (*GEOSGeom_createLinearRing)(s);}
	GEOSGeometry *Geom_createPolygon(GEOSGeometry *shell,GEOSGeometry **holes,int num_holes) {return (*GEOSGeom_createPolygon)(shell,holes,num_holes);}
	GEOSGeometry *UnaryUnion(GEOSGeometry *g1) {return (*GEOSUnaryUnion)(g1);}
	int GeomTypeId(const GEOSGeometry *g1) {return (*GEOSGeomTypeId)(g1);}

	static const char address_in_this_module = 0;
	ExplicitSDNAPolylineToGeosWrapper()
	{
		//find path of this dll and look for geos_c.dll in the same place
		HMODULE this_dll_handle;
		HRESULT retval = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCTSTR)&address_in_this_module,
			&this_dll_handle
			);
		assert(retval);
		string this_dll_path = dllPathFromHMODULE(this_dll_handle);
		char drive[_MAX_DRIVE];
	    char dir[_MAX_DIR];
	    char fname[_MAX_FNAME];
	    char ext[_MAX_EXT];
		_splitpath(this_dll_path.c_str(),drive,dir,fname,ext);
		//fname holds filename of this dll, but is now discarded
		assert(0==strcmp(ext,".dll"));
		char geos_dll_path[_MAX_PATH];
		_makepath(geos_dll_path,drive,dir,"geos_c",ext);
		wchar_t geos_dll_path_w[_MAX_PATH];
		mbstowcs(geos_dll_path_w, geos_dll_path, strlen(geos_dll_path)+1);//Plus null
				
		hDLL = LoadLibrary(geos_dll_path_w);
		if (hDLL == NULL)
		{
			cout << "GEOS DLL not found" << endl;
		}
		else
		{
			initGEOS = (initGEOS_t)GetProcAddress(hDLL,"initGEOS");
			assert(initGEOS != NULL);
			finishGEOS = (finishGEOS_t)GetProcAddress(hDLL,"finishGEOS");
			assert(finishGEOS != NULL);
			GEOSCoordSeq_create = (GEOSCoordSeq_create_t)GetProcAddress(hDLL,"GEOSCoordSeq_create");
			assert(GEOSCoordSeq_create != NULL);
			GEOSCoordSeq_setX = (GEOSCoordSeq_set_t)GetProcAddress(hDLL,"GEOSCoordSeq_setX");
			assert(GEOSCoordSeq_setX != NULL);
			GEOSCoordSeq_setY = (GEOSCoordSeq_set_t)GetProcAddress(hDLL,"GEOSCoordSeq_setY");
			assert(GEOSCoordSeq_setY != NULL);
			GEOSGeom_createLineString = (GEOSGeom_createLineString_t)GetProcAddress(hDLL,"GEOSGeom_createLineString");
			assert(GEOSGeom_createLineString != NULL);
			GEOSGeom_createPoint = (GEOSGeom_createPoint_t)GetProcAddress(hDLL,"GEOSGeom_createPoint");
			assert(GEOSGeom_createPoint != NULL);
			GEOSGeom_destroy = (GEOSGeom_destroy_t)GetProcAddress(hDLL,"GEOSGeom_destroy");
			assert(GEOSGeom_destroy != NULL);
			GEOSSTRtree_create = (GEOSSTRtree_create_t)GetProcAddress(hDLL,"GEOSSTRtree_create");
			assert(GEOSSTRtree_create != NULL);
			GEOSSTRtree_insert = (GEOSSTRtree_insert_t)GetProcAddress(hDLL,"GEOSSTRtree_insert");
			assert(GEOSSTRtree_insert != NULL);
			GEOSSTRtree_iterate = (GEOSSTRtree_iterate_t)GetProcAddress(hDLL,"GEOSSTRtree_iterate");
			assert(GEOSSTRtree_iterate != NULL);
			GEOSSTRtree_query = (GEOSSTRtree_query_t)GetProcAddress(hDLL,"GEOSSTRtree_query");
			assert(GEOSSTRtree_query != NULL);
			GEOSSTRtree_destroy = (GEOSSTRtree_destroy_t)GetProcAddress(hDLL,"GEOSSTRtree_destroy");
			assert(GEOSSTRtree_destroy != NULL);
			GEOSEnvelope = (GEOSEnvelope_t)GetProcAddress(hDLL,"GEOSEnvelope");
			assert(GEOSEnvelope != NULL);
			GEOSGeom_createCollection = (GEOSGeom_createCollection_t)GetProcAddress(hDLL,"GEOSGeom_createCollection");
			assert(GEOSGeom_createCollection != NULL);
			GEOSGeom_createEmptyCollection = (GEOSGeom_createEmptyCollection_t)GetProcAddress(hDLL,"GEOSGeom_createEmptyCollection");
			assert(GEOSGeom_createEmptyCollection != NULL);
			GEOS_Intersection = (GEOS_Intersection_t)GetProcAddress(hDLL,"GEOSIntersection");
			assert(GEOS_Intersection != NULL);
			GEOSisEmpty = (GEOSisEmpty_t)GetProcAddress(hDLL,"GEOSisEmpty");
			assert(GEOSisEmpty != NULL);
			GEOSGeom_Difference = (GEOS_Intersection_t)GetProcAddress(hDLL,"GEOSDifference");
			assert(GEOSGeom_Difference != NULL);
			GEOSGetNumGeometries = (GEOSGetNumGeometries_t)GetProcAddress(hDLL,"GEOSGetNumGeometries");
			assert(GEOSGetNumGeometries != NULL);
			GEOSGetGeometryN = (GEOSGetGeometryN_t)GetProcAddress(hDLL,"GEOSGetGeometryN");
			assert(GEOSGetGeometryN != NULL);
			GEOSGeom_clone = (GEOSGeom_clone_t)GetProcAddress(hDLL,"GEOSGeom_clone");
			assert(GEOSGeom_clone != NULL);
			GEOSGeomGetNumPoints = (GEOSGeomGetNumPoints_t)GetProcAddress(hDLL,"GEOSGeomGetNumPoints");
			assert(GEOSGeomGetNumPoints != NULL);
			GEOSGeomGetPointN = (GEOSGeomGetPointN_t)GetProcAddress(hDLL,"GEOSGeomGetPointN");
			assert(GEOSGeomGetPointN != NULL);
			GEOSGeomGetX = (GEOSGeomGetX_t)GetProcAddress(hDLL,"GEOSGeomGetX");
			assert(GEOSGeomGetX != NULL);
			GEOSGeomGetY = (GEOSGeomGetY_t)GetProcAddress(hDLL,"GEOSGeomGetY");
			assert(GEOSGeomGetY != NULL);
			GEOSGeom_createLinearRing = (GEOSGeom_createLinearRing_t)GetProcAddress(hDLL,"GEOSGeom_createLinearRing");
			assert(GEOSGeom_createLinearRing != NULL);
			GEOSGeom_createPolygon = (GEOSGeom_createPolygon_t)GetProcAddress(hDLL,"GEOSGeom_createPolygon");
			assert(GEOSGeom_createPolygon != NULL);
			GEOSUnaryUnion = (GEOSUnaryUnion_t)GetProcAddress(hDLL,"GEOSUnaryUnion");
			assert(GEOSUnaryUnion != NULL);
			GEOSGeomTypeId = (GEOSGeomTypeId_t)GetProcAddress(hDLL,"GEOSGeomTypeId");
			assert(GEOSGeomTypeId != NULL);
		}
	}
	~ExplicitSDNAPolylineToGeosWrapper()
	{
		FreeLibrary(hDLL);
	}


private:
	HINSTANCE hDLL;

	typedef GEOSCoordSequence* (*GEOSCoordSeq_create_t)(unsigned int size,unsigned int dims);
	GEOSCoordSeq_create_t GEOSCoordSeq_create;
	typedef int (*GEOSCoordSeq_set_t)(GEOSCoordSequence* s,unsigned int idx,double val);
	GEOSCoordSeq_set_t GEOSCoordSeq_setX;
	GEOSCoordSeq_set_t GEOSCoordSeq_setY;

	typedef GEOSGeometry* (*GEOSGeom_createLineString_t)(GEOSCoordSequence *s);
	GEOSGeom_createLineString_t GEOSGeom_createLineString;
	typedef GEOSGeometry* (*GEOSGeom_createPoint_t)(GEOSCoordSequence *s);
	GEOSGeom_createPoint_t GEOSGeom_createPoint;
	typedef void (*GEOSGeom_destroy_t)(GEOSGeometry *g);
	GEOSGeom_destroy_t GEOSGeom_destroy;
	
	typedef GEOSGeometry* (*GEOSEnvelope_t)(const GEOSGeometry* g1);
	GEOSEnvelope_t GEOSEnvelope;

	typedef GEOSSTRtree* (*GEOSSTRtree_create_t)(size_t nodeCapacity);
	GEOSSTRtree_create_t GEOSSTRtree_create;
	typedef void (*GEOSSTRtree_insert_t)(GEOSSTRtree *tree,const GEOSGeometry *g,void *item);
	GEOSSTRtree_insert_t GEOSSTRtree_insert;
	typedef void (*GEOSSTRtree_iterate_t)(GEOSSTRtree *tree,GEOSQueryCallback callback,void *userdata);
	GEOSSTRtree_iterate_t GEOSSTRtree_iterate;
	typedef void (*GEOSSTRtree_query_t)(GEOSSTRtree *tree,const GEOSGeometry *g,GEOSQueryCallback callback,void *userdata);
	GEOSSTRtree_query_t GEOSSTRtree_query;
	typedef void (*GEOSSTRtree_destroy_t)(GEOSSTRtree *tree);
	GEOSSTRtree_destroy_t GEOSSTRtree_destroy;
	typedef GEOSGeometry* (*GEOSGeom_createCollection_t)(int type,GEOSGeometry* *geoms, unsigned int ngeoms);
	GEOSGeom_createCollection_t GEOSGeom_createCollection;
	typedef GEOSGeometry* (*GEOSGeom_createEmptyCollection_t)(int type);
	GEOSGeom_createEmptyCollection_t GEOSGeom_createEmptyCollection;
	typedef GEOSGeometry* (*GEOS_Intersection_t)(GEOSGeometry*,GEOSGeometry*);
	GEOS_Intersection_t GEOS_Intersection;
	typedef char (*GEOSisEmpty_t)(GEOSGeometry*);
	GEOSisEmpty_t GEOSisEmpty;
	GEOS_Intersection_t GEOSGeom_Difference;
	typedef int (*GEOSGetNumGeometries_t)(GEOSGeometry*);
	GEOSGetNumGeometries_t GEOSGetNumGeometries;
	typedef GEOSGeometry* (*GEOSGetGeometryN_t)(GEOSGeometry*,int);
	GEOSGetGeometryN_t GEOSGetGeometryN;
	typedef GEOSGeometry* (*GEOSGeom_clone_t)(GEOSGeometry*);
	GEOSGeom_clone_t GEOSGeom_clone;
	typedef	int (*GEOSGeomGetNumPoints_t)(const GEOSGeometry *g);
	GEOSGeomGetNumPoints_t GEOSGeomGetNumPoints;
	typedef GEOSGeometry * (*GEOSGeomGetPointN_t)(const GEOSGeometry *g1, int n);
	GEOSGeomGetPointN_t GEOSGeomGetPointN;
	typedef char (*GEOSGeomGetX_t) (GEOSGeometry *g1,double *x);
	GEOSGeomGetX_t GEOSGeomGetX;
	typedef char (*GEOSGeomGetY_t) (GEOSGeometry *g1,double *y);
	GEOSGeomGetY_t GEOSGeomGetY;
	typedef GEOSGeometry *(*GEOSGeom_createLinearRing_t)(GEOSCoordSequence* s);
	GEOSGeom_createLinearRing_t GEOSGeom_createLinearRing;
	typedef GEOSGeometry *(*GEOSGeom_createPolygon_t)(GEOSGeometry *shell,GEOSGeometry **holes,int num_holes);
	GEOSGeom_createPolygon_t GEOSGeom_createPolygon;
	typedef GEOSGeometry *(*GEOSUnaryUnion_t)(GEOSGeometry*);
	GEOSUnaryUnion_t GEOSUnaryUnion;
	typedef int (*GEOSGeomTypeId_t)(const GEOSGeometry*);
	GEOSGeomTypeId_t GEOSGeomTypeId;

	std::string dllPathFromHMODULE(HMODULE hm) {
		std::vector<char> executablePath(256);

	  // Try to get the executable path with a buffer of MAX_PATH characters.
	  DWORD result = ::GetModuleFileNameA(
		hm, &executablePath[0], static_cast<DWORD>(executablePath.size())
	  );

	  // As long the function returns the buffer size, it is indicating that the buffer
	  // was too small. Keep enlarging the buffer by a factor of 2 until it fits.
	  while(result == executablePath.size()) {
		executablePath.resize(executablePath.size() * 2);
		result = ::GetModuleFileNameA(
		  hm, &executablePath[0], static_cast<DWORD>(executablePath.size())
		);
	  }

	  // If the function returned 0, something went wrong
	  if(result == 0) {
		throw std::runtime_error("GetModuleFileName() failed");
	  }

	  // We've got the path, construct a standard string from it
	  return std::string(executablePath.begin(), executablePath.begin() + result);
	}

};

void my_geos_message_handler(const char *fmt, ...);

#undef CLUSTERFINDER_DEBUG

class ClusterFinder 
{
public:
	static ClusterList get_clusters(JunctionContainer &jmap,double xytolerance,double ztolerance)
	{
		if (xytolerance <= 0 && ztolerance <=0)
			return ClusterList(); //empty clusterlist

		ClusterFinder cf(xytolerance,ztolerance);
		for (JunctionContainer::iterator junc = jmap.begin();	junc != jmap.end(); junc++)
		{
			const JunctionMapKey &j = junc->first;
			cf.add(j);
		}
		ClusterList clusters = cf.get_clusters();
		return clusters;
	}

private:
	struct ClusterFinderNode
	{
		JunctionMapKey jmkey;
		bool visited;
		ClusterFinderNode(const JunctionMapKey &j)
			:jmkey(j),visited(false) {}
	};

	typedef boost::geometry::model::point<double, 3, boost::geometry::cs::cartesian> boost3dpoint;
	typedef boost::geometry::model::box<boost3dpoint> box;
	typedef std::pair<boost3dpoint, ClusterFinderNode*> value; //mega inefficient as point is stored outside node, inside node and in junction map 
	//ultimately we should make sdna points support boost

	//<8,2> is quite a guess.  if performance is ever a problem we should revisit that.
	boost::geometry::index::rtree< value, boost::geometry::index::linear<8,2> > tree;
	vector<ClusterFinderNode*> nodelist; //only used as primary container for nodes because we can't iterate through tree, also inefficient
	boost::object_pool<ClusterFinderNode> node_pool;

	ClusterList clusters;
	vector<ClusterFinderNode*> searchqueue;
	double xytol,ztol;

	ClusterFinder(double xytol,double ztol) : xytol(xytol),ztol(ztol) {}

	void add_node_to_queue(ClusterFinderNode *node)
	{
		if (!node->visited)
		{
			searchqueue.push_back(node);
			#ifdef CLUSTERFINDER_DEBUG
				cout << "    queued node at " << node->jmkey.x << "," << node->jmkey.y << "," << node->jmkey.z << endl;
			#endif
		}
		else
		{
			#ifdef CLUSTERFINDER_DEBUG
				cout << "    ignored node at " << node->jmkey.x << "," << node->jmkey.y << "," << node->jmkey.z << endl;
			#endif
		}
	}

	void check_node_for_neighbours(ClusterFinderNode *node)
	{
		#ifdef CLUSTERFINDER_DEBUG
			cout << "  exploring node: " << node->jmkey.x << "," << node->jmkey.y << "," << node->jmkey.z << endl;
		#endif

		//mark visited and add to end of list
		node->visited = true;
		clusters.back().push_back(node->jmkey);

		//add all neighbours within tolerance to search queue
		const double x = node->jmkey.x;
		const double y = node->jmkey.y;
		const double z = node->jmkey.z;
		
		box query_box(boost3dpoint(x-xytol, y-xytol, z-ztol), boost3dpoint(x+xytol, y+xytol, z+ztol));

		std::vector<value> result_s;
		tree.query(boost::geometry::index::intersects(query_box), std::back_inserter(result_s));
		BOOST_FOREACH(value& p, result_s)
			add_node_to_queue(p.second); 
	}

	void try_to_find_cluster_starting_from_node(ClusterFinderNode *initial_node)
	{
		#ifdef CLUSTERFINDER_DEBUG
			cout.precision(numeric_limits<double>::digits10 + 1);
			cout << "looking for cluster from node at " << initial_node->jmkey.x << "," << initial_node->jmkey.y << "," << initial_node->jmkey.z << endl;
		#endif

		if (initial_node->visited)
			return; //node was already discovered when starting from another node

		//push back new empty cluster
		clusters.push_back(Cluster());

		//initialize exploration queue
		assert(searchqueue.size()==0);
		searchqueue.push_back(initial_node);

		//explore node to fill cluster vector
		while (searchqueue.size()>0)
		{
			ClusterFinderNode* node_to_search_next = searchqueue.back();
			searchqueue.pop_back();
			check_node_for_neighbours(node_to_search_next);
		}

		//all found nodes will now be on end of clusters
		//remove cluster vector if no neighbours found,
		Cluster &latest_cluster = clusters.back();
		if (latest_cluster.size() <= 1)
		{
			//should be a vector containing only the starting point
			//should never be size 0
			assert(latest_cluster.size()==1 && latest_cluster[0] == initial_node->jmkey); 
			clusters.pop_back();
		}
		else
		{
			#ifdef CLUSTERFINDER_DEBUG
					cout << "Found cluster" << endl;
			#endif
		}
	}

	void add(const JunctionMapKey &j)
	{
		ClusterFinderNode *n = new (node_pool.malloc()) ClusterFinderNode(j);
		nodelist.push_back(n);

		const double x = j.x;
		const double y = j.y;
		const double z = j.z;

	    boost3dpoint p(x,y,z);
	    tree.insert(std::make_pair(p, n));
	}

	ClusterList get_clusters()
	{
		#ifdef CLUSTERFINDER_DEBUG
				cout << "finding clusters xytol=" << xytol << " ztol=" << ztol << endl;
		#endif
		BOOST_FOREACH(ClusterFinderNode *n,nodelist)
			try_to_find_cluster_starting_from_node(n);
		return clusters;
	}

};

vector<GEOSGeometry*> unlink_respecting_planarize(ExplicitSDNAPolylineToGeosWrapper &geos,vector<GEOSGeometry *> links,vector<GEOSGeometry *> unlinks_in);

GEOSCoordSequence *point_vector_to_coordseq2d(ExplicitSDNAPolylineToGeosWrapper &geos,vector<Point> &pv);

vector<Point> linestring_to_pointvector(ExplicitSDNAPolylineToGeosWrapper &geos,const GEOSGeometry* geom);

vector<vector<Point>> unlink_respecting_planarize(vector<vector<Point>> &links, vector<vector<Point>> &unlinks);

//void detect_unlinks(GEOSGeometry **line_array,int array_length);
