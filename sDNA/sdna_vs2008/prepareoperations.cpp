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
#include "prepareoperations.h"
#include "geos_sdna_wrapper.h"

size_t PrepareOperation::get_split_link_ids(long **output)
{
	output_buffer_1 = get_split_link_ids();
	*output = output_buffer_1.toCPointer();
	return output_buffer_1.size();
}

size_t PrepareOperation::get_traffic_islands(long **output)
{
	output_buffer_1.clear();
	vector<SDNAPolyline*> traffic_islands = get_traffic_islands();
	for (vector<SDNAPolyline*>::iterator it=traffic_islands.begin(); it!=traffic_islands.end(); ++it)
		output_buffer_1.push_back((*it)->arcid);
	*output = output_buffer_1.toCPointer();
	return output_buffer_1.size();
}

vector<SDNAPolyline*> PrepareOperation::get_traffic_islands()
{
	vector<SDNAPolyline*> result;
	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it!=net->link_container.end(); ++it)
		if (islanddata.get_data(it->second))
			result.push_back(it->second);
	return result;
}


size_t PrepareOperation::fix_split_links()
{
	SplitLinkVector juncs_to_merge = get_split_link_junction_keys();
	for (SplitLinkVector::iterator it=juncs_to_merge.begin(); it!=juncs_to_merge.end(); ++it)
		fix_split_link(it->first,it->second);
	return juncs_to_merge.size();
}

size_t PrepareOperation::fix_traffic_islands()
{
	net->ensure_junctions_created();

	//straighten all islands
	vector<SDNAPolyline*> traffic_islands = get_traffic_islands();
	const size_t num_islands = traffic_islands.size();
	vector<SDNAPolyline*> straightened_traffic_islands;
	for (vector<SDNAPolyline*>::iterator it=traffic_islands.begin(); it!=traffic_islands.end();++it)
	{
		SDNAPolyline * const newlink = straighten_link_remove_weight_cost(*it);
		if (newlink)
			straightened_traffic_islands.push_back(newlink); 
	}

	//now some will be duplicates, so remove them, but keep track of what's left
	set<SDNAPolyline*> remaining_islands(straightened_traffic_islands.begin(),straightened_traffic_islands.end());
	vector<SDNAPolyline*> duplicates, originals;
	get_duplicate_links(duplicates,originals,true,straightened_traffic_islands);
	for (vector<SDNAPolyline*>::iterator it=duplicates.begin(); it!=duplicates.end(); ++it)
	{
		net->remove_link(*it);
		remaining_islands.erase(*it);
	}
	
	//now some will be split links: identify which
	//(not the most efficient as it checks all split links in net;
	// but this is necessary as edge->end_junction *s aren't valid here)
	SplitLinkVector split_links = get_split_link_junction_keys();
	SplitLinkVector split_links_to_fix;
	for (SplitLinkVector::iterator it=split_links.begin(); it!=split_links.end(); ++it)
	{
		vector<Edge*> outgoing_edges = net->junction_storage[it->first]->get_outgoing_edges_for_gradesep(it->second);
		assert(outgoing_edges.size()==2);
		//is this split link connected to one of our former islands?
		if (remaining_islands.count(outgoing_edges[0]->link)
			|| remaining_islands.count(outgoing_edges[1]->link))
			//don't fix yet otherwise we mess up the process of deciding what to fix
			split_links_to_fix.push_back(*it); 
	}
	//now fix them
	for (SplitLinkVector::iterator it=split_links_to_fix.begin(); it!=split_links_to_fix.end(); ++it)
			fix_split_link(it->first,it->second);

	return num_islands;
}

void PrepareOperation::fix_split_link(JunctionMapKey &key,float gradesep)
{
	Junction *j = net->junction_storage[key];

	//get a pair of edges from different links, use them to deduce links to fix
	vector<Edge*> matching_edges = j->get_outgoing_edges_for_gradesep(gradesep);
	assert(matching_edges.size()==2);
	Edge *edge1 = matching_edges[0];
	Edge *edge2 = matching_edges[1];
	SDNAPolyline *link1 = edge1->link;
	SDNAPolyline *link2 = edge2->link;

	if (link1 == link2) return; //possible if we fixed other split links since obtaining the initial list
	
	vector<Point> new_pointlist;
	edge1->add_copy_of_points_from_end_to_vector(EDGE_END,new_pointlist);
	edge2->add_copy_of_points_from_end_to_vector(EDGE_START,new_pointlist);
	
	//add link
	const long new_arcid = get_new_arcid();
	net->add_polyline(new_arcid,new_pointlist);
	SDNAPolyline * const newlink = net->link_container[new_arcid];
	
	//move over data
	for (vector<LengthWeightingStrategy>::iterator it=datatokeep.begin();it!=datatokeep.end();it++)
		it->set_data(newlink,it->get_data(link1)+it->get_data(link2));
	for (vector<NetExpectedDataSource<string>>::iterator it=textdatatokeep.begin();it!=textdatatokeep.end();it++)
		if (it->get_data(link1)==it->get_data(link2))
			it->set_data(newlink,it->get_data(link1));
		else
			it->set_data(newlink,"");
		
	//but that said...
	//only treat as island if both halves were islands
	const bool is_island = islanddata.get_data(link1) && islanddata.get_data(link2); 
	islanddata.set_data(newlink,is_island);

	//and fix grade separation data
	net->start_gsdata.set_data(newlink,edge1->get_end_gs());
	net->end_gsdata.set_data(newlink,edge2->get_end_gs());
	
	net->ensure_junctions_created();
	net->remove_link(link1);
	net->remove_link(link2);
}

bool point_vector_zero_length(vector<Point> &points)
{
		return (points.size() == 2 && points[0] == points[1])
			|| (points.size() < 2);
}

SDNAPolyline* PrepareOperation::straighten_link_remove_weight_cost(SDNAPolyline *s)
{
	vector<Point> new_pointlist;
	new_pointlist.push_back(s->points[0]);
	new_pointlist.push_back(s->points[s->points.size()-1]);
	
	SDNAPolyline * newlink = NULL;
	if (!point_vector_zero_length(new_pointlist))
	{
		const long new_arcid = get_new_arcid();
		net->add_polyline(new_arcid,new_pointlist);
		newlink = net->link_container[new_arcid];
		
		//move over data
		for (vector<LengthWeightingStrategy>::iterator it=datatokeep.begin();it!=datatokeep.end();it++)
			it->set_data(newlink,it->get_data(s));
		for (vector<NetExpectedDataSource<string>>::iterator it=textdatatokeep.begin();it!=textdatatokeep.end();it++)
			it->set_data(newlink,it->get_data(s));
		
		//but then change...
		for (vector<NetExpectedDataSource<float>>::iterator it=islandfieldstozero.begin();it!=islandfieldstozero.end();++it)
			it->set_data(newlink,0);
		islanddata.set_data(newlink,0);
		net->ensure_junctions_created();
	}
	net->remove_link(s);
	return newlink;
}

size_t PrepareOperation::get_duplicate_links(long **duplicates,long **originals)
{
	vector<SDNAPolyline*> dup, orig;
	get_duplicate_links(dup,orig);
	assert(dup.size()==orig.size());
	output_buffer_1.clear();
	output_buffer_2.clear();
	for (unsigned long i=0;i<dup.size();++i)
	{
		output_buffer_1.push_back(dup[i]->arcid);
		output_buffer_2.push_back(orig[i]->arcid);
	}
	*duplicates = output_buffer_1.toCPointer();
	*originals = output_buffer_2.toCPointer();
	return dup.size();
}

size_t PrepareOperation::fix_duplicate_links()
{
	net->ensure_junctions_created();
	vector<SDNAPolyline*> dup,orig;
	get_duplicate_links(dup,orig);
	for (vector<SDNAPolyline*>::iterator it=dup.begin();it!=dup.end();++it)
		net->remove_link(*it);
	return dup.size();
}

void PrepareOperation::get_duplicate_links(vector<SDNAPolyline*> &duplicates, vector<SDNAPolyline*> &originals,
							  bool partial_search, vector<SDNAPolyline*> &subset)
{
	assert (duplicates.size()==0);
	assert (originals.size()==0);
	assert (partial_search || subset.size()==0);

	set<SDNAPolyline*> subset_set(subset.begin(),subset.end()); //the subset, if any, to search
	map<hash_t,SDNAPolyline*> duplicate_check; //maps unique spatial identifier of any link to * of FIRST such link found
	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it!=net->link_container.end(); ++it)
	{
		SDNAPolyline * const s = it->second;
		if (!partial_search || subset_set.find(s)!=subset_set.end())
		{
			const hash_t hash = get_link_hash(s);
			map<hash_t,SDNAPolyline*>::iterator lookup = duplicate_check.find(hash);
			if (lookup==duplicate_check.end())
			{
				//first time we've seen this hash
				duplicate_check[hash] = s;
			}
			else
			{
				//duplicate
				duplicates.push_back(s);
				originals.push_back(lookup->second);
			}
		}
	}
}

hash_t PrepareOperation::get_link_hash(SDNAPolyline *s)
{
	vector<Point> points_copy(s->points);
	pair<float,float> gspair = make_pair(s->get_start_gs(),s->get_end_gs());
	
	//always order points from the bottom left end
	
	//lexicographic compare on link points: will almost always terminate on first loop iteration
	//nb apparently < applied to vectors does this anyway, so I could cut this,
	//but on the plus side this one compares the data in place
	bool hash_forwards = true; 
	const size_t num_points = points_copy.size();
	for (size_t i=0; i<num_points; ++i)
	{
		const Point &next_forward_point = points_copy[i];
		const Point &next_backward_point = points_copy[num_points-1-i];
		if (next_forward_point < next_backward_point)
		{
			hash_forwards = true;
			break;
		}
		else if (next_backward_point < next_forward_point)
		{
			hash_forwards = false;
			break;
		}
		//if loop actually runs to completion that means the edge is a palindrome (!) so order doesn't matter
	}

	if (!hash_forwards)
	{
		reverse(points_copy.begin(),points_copy.end());
		std::swap(gspair.first,gspair.second);
	}
	
	return make_pair(points_copy,gspair);
}

size_t PrepareOperation::get_subsystems(char **message,long **links)
{
	output_buffer_1.clear();
	output_string_array.clear();
	stringstream ss;

	subsystem_group subsystems = get_subsystems();
	ss << "Largest system contains " << subsystems[0]->size() << " links";
	for (subsystem_group::iterator it=subsystems.begin()+1;it!=subsystems.end();++it)
	{
		vector<SDNAPolyline*> const& subsystem = **it;
		ss << endl << subsystem.size() << "-link subsystem contains link with id = "
			<< subsystem[0]->arcid;
		for (vector<SDNAPolyline*>::const_iterator s=subsystem.begin();s!=subsystem.end();++s)
			output_buffer_1.push_back((*s)->arcid);
	}

	output_string_array.add_string(ss.str());
	
	*links = output_buffer_1.toCPointer();
	*message = output_string_array.get_string_array()[0];
	return output_buffer_1.size();
}

bool compare_vector_size(shared_ptr<vector<SDNAPolyline*> > i,shared_ptr<vector<SDNAPolyline*> > j)
{
	return (i->size() > j->size());
}

PrepareOperation::subsystem_group PrepareOperation::get_subsystems()
{
	net->ensure_junctions_created();
	net->link_edges();
	
	subsystem_group subsystems;
	set<SDNAPolyline*> unreached;
	for (SDNAPolylineContainer::iterator it=net->link_container.begin();it!=net->link_container.end();++it)
		unreached.insert(it->second);

	while (!unreached.empty())
	{
		SDNAPolyline *arbitrary_link = *unreached.begin();
		unreached.erase(unreached.begin());
		shared_ptr<vector<SDNAPolyline*> > reached = flood_fill(unreached,arbitrary_link);
		subsystems.push_back(reached);
	}

	net->unlink_edges();
	sort(subsystems.begin(),subsystems.end(),compare_vector_size);
    return subsystems;
}

size_t PrepareOperation::fix_subsystems()
{
	subsystem_group subsystems = get_subsystems();

	//delete all but first (largest) subsystem
	for (subsystem_group::iterator it=subsystems.begin()+1;it!=subsystems.end();++it)
	{
		vector<SDNAPolyline*> const& subsystem = **it;
		for (vector<SDNAPolyline*>::const_iterator s=subsystem.begin();s!=subsystem.end();++s)
			net->remove_link(*s);
	}
	return subsystems.size()-1;
}

shared_ptr<vector<SDNAPolyline*> > PrepareOperation::flood_fill(set<SDNAPolyline*> &unreached,SDNAPolyline *start_link)
{
	assert(unreached.count(start_link)==0); //starting link not in unreached
	assert(net->net_is_built);
	
	shared_ptr<vector<SDNAPolyline*> > reached(new vector<SDNAPolyline*>());
	reached->push_back(start_link);

	vector<SDNAPolyline*> to_explore;
	to_explore.push_back(start_link);

	while(to_explore.size())
	{
		SDNAPolyline *current = to_explore.back();
		to_explore.pop_back();

		//may be more efficient using boost::range(?) as SDNAPolyline::get_linked_links() copies vectors
		vector<SDNAPolyline*> linked_links = current->get_linked_links();
		for (vector<SDNAPolyline*>::iterator it=linked_links.begin();it!=linked_links.end();++it)
		{
			SDNAPolyline * candidate_next = *it;
			if (unreached.count(candidate_next)==1)
			{
				unreached.erase(candidate_next);
				reached->push_back(candidate_next);
				to_explore.push_back(candidate_next);
			}
		}
	}
	return reached;
}

size_t PrepareOperation::get_near_miss_connections(long **links)
{
	output_buffer_1.clear();
	
	ClusterList clusters = net->get_near_miss_clusters(xytol,ztol);
	
	for (ClusterList::iterator c = clusters.begin(); c!=clusters.end(); ++c)
	{
		for (Cluster::iterator i = c->begin(); i!=c->end(); ++i)
		{
			set<SDNAPolyline*> links;
			net->junction_storage[*i]->get_attached_links(links);
			for (set<SDNAPolyline*>::iterator s = links.begin(); s!=links.end(); ++s)
				output_buffer_1.push_back((*s)->arcid);
		}
	}
	
	*links = output_buffer_1.toCPointer();
	return output_buffer_1.size();
}

size_t PrepareOperation::fix_near_miss_connections()
{
	assert(!net->net_is_built);

	ClusterList clusters = net->get_near_miss_clusters(xytol,ztol);

	for (ClusterList::iterator c = clusters.begin(); c!=clusters.end(); ++c)
	{
		merge_junctions(*c);
	}
	
	return clusters.size();
}

//change all points to the mean of the group
void PrepareOperation::merge_junctions(vector<JunctionMapKey> &junctions)
{
	double n = static_cast<double>(junctions.size());
	assert(n>0);
	double xsum=0,ysum=0,zsum=0;
	for (vector<JunctionMapKey>::iterator it=junctions.begin(); it!=junctions.end(); ++it)
	{
		xsum += it->x;
		ysum += it->y;
		zsum += it->z;
	}
	double xmean = xsum / n;
	double ymean = ysum / n;
	float zmean = numeric_cast<float>(zsum / n);

	for (vector<JunctionMapKey>::iterator it=junctions.begin(); it!=junctions.end(); ++it)
	{
		move_junction(*it,xmean,ymean,zmean);
	}
}

void PrepareOperation::move_junction(JunctionMapKey &key, double new_x, double new_y, float new_z)
{
	//n.b. must handle case of looped links, so
	//handles each link end separately
	//through net interface to ensure correct update of junction map

	#ifdef _SDNADEBUG
		cout << "moving junction at " << key.x << "," << key.y << " to " << new_x << "," << new_y << endl;
	#endif

	//this next bit is a wee bit inefficient but it gets around the fact that link pointers are invalidated by, 
	//and junctions are modified by, move_link_endpoint
	//... a problem that's particularly harrowing in the case of loop links which must be moved twice
	//repeatedly look up the junction and find a link with the incorrect endpoint, then change it
	
	const Point new_point = Point(new_x,new_y,new_z);

	while (Junction *junc = net->junction_storage[key])
	{
		Junction &j = *junc;
		
		vector<Edge*>::iterator it;

		//only iterate through outgoing edges (though incoming would be fine too): we're moving data in links not edges
		for (it=j.outgoing_edges.begin(); it!=j.outgoing_edges.end(); ++it)
			if ((*it)->link->get_point((*it)->link_end_of_edge(EDGE_START)) != new_point)
				break;

		if (it != j.outgoing_edges.end())
			move_link_endpoint((*it)->link,(*it)->link_end_of_edge(EDGE_START),new_x,new_y,new_z);
		else
			break;
	}
}

void PrepareOperation::move_link_endpoint(SDNAPolyline* link, junction_option_type end, double new_x, double new_y, float new_z)
{
	#ifdef _SDNADEBUG
		string desc = end==EDGE_START?"start":"end";
		cout << "moving link " << desc << "point of link " << link->arcid << endl;
	#endif
	//grab copy of points
	vector<Point> points(link->points.begin(),link->points.end());
	long arcid = link->arcid;
	
	//change points
	switch(end)
	{
	case EDGE_START:
		points[0] = Point(new_x,new_y,new_z);
		break;
	case EDGE_END:
		points.back() = Point(new_x,new_y,new_z);
		break;
	default:
		assert(false);
	}

	//remove and add link
	if (!point_vector_zero_length(points))
	{
		long new_arcid = get_new_arcid();
		net->add_polyline(new_arcid,points);
		SDNAPolyline * const newlink = net->link_container[new_arcid];

		//move over data
		for (vector<LengthWeightingStrategy>::iterator it=datatokeep.begin();it!=datatokeep.end();it++)
			it->set_data(newlink,it->get_data(link));
		for (vector<NetExpectedDataSource<string>>::iterator it=textdatatokeep.begin();it!=textdatatokeep.end();it++)
			it->set_data(newlink,it->get_data(link));

		net->ensure_junctions_created();
	}
	else
	{
		#ifdef _SDNADEBUG
				cout << " - link erased as now zero length" << endl;
		#endif
	}
	net->remove_link(link);
}

Net* PrepareOperation::import_from_link_unlink_format()
{
	//prepare data
	vector<vector<Point>> link_coordinates;
	for (SDNAPolylineContainer::iterator it=net->link_container.begin();it!=net->link_container.end();it++)
	{
		PointVector &link_points = it->second->points; 
		vector<Point> current_link_points(link_points.begin(),link_points.end());
		link_coordinates.push_back(current_link_points);
	}

	//planarize
	vector<vector<Point>> new_net_geom = unlink_respecting_planarize(link_coordinates, net->unlinks);

	//unpack data to new net
	Net *result = new Net();
	result->set_gs_data(
		NetExpectedDataSource<float>("",0,result,"start_gs",NULL),
		NetExpectedDataSource<float>("",0,result,"end_gs",NULL));
	for (vector<vector<Point>>::iterator it=new_net_geom.begin();it!=new_net_geom.end();it++)
	{
		const long new_arcid = numeric_cast<long>((it-new_net_geom.begin()));
		result->add_polyline(new_arcid,*it);
	}
	
	//fix split links
	PrepareOperation p(result,"preserve_net_config;internal_interface",print_warning_callback);
	p.bind_network_data();
	p.fix_split_links();

	return result;
}

SplitLinkVector PrepareOperation::get_split_link_junction_keys(unsigned long max_ids)
{
	net->ensure_junctions_created();
	SplitLinkVector result;
	for (JunctionContainer::iterator junc = net->junction_storage.begin();junc != net->junction_storage.end(); junc++)
	{
		//this works differently to how calculateLinkFraction detects split links (the latter uses outgoing_connections)
		//as this here doesn't require edges to be linked in the net
		JunctionMapKey const key=junc->first;
		Junction * const j=junc->second;
		assert (j->two_edges_per_line_end());
		//...therefore ignore incoming edges - they will be for the same links as the outgoing ones
		map<float, set<SDNAPolyline*> > gradeseps;
		BOOST_FOREACH(float gs,j->get_gradeseps())
		{
			set<SDNAPolyline*> polylines;
			BOOST_FOREACH(Edge *e,j->get_outgoing_edges_for_gradesep(gs))
			{
				polylines.insert(e->link);
			}
			gradeseps[gs]=polylines;
		}
		typedef pair<float,set<SDNAPolyline*> > GSPAIR;
		BOOST_FOREACH(GSPAIR gspair,gradeseps)
		{
			const float gs = gspair.first;
			set<SDNAPolyline*> attached_links = gspair.second;
			if (attached_links.size()==2)
			{
				//possible split link
				SDNAPolyline *s1,*s2;
				set<SDNAPolyline*>::iterator it = attached_links.begin();
				s1 = *it++;
				s2 = *it;
				
				if (s1->is_not_loop() && s2->is_not_loop())
					//definite split link
					result.push_back(make_pair(key,gs));
			}
			if (result.size() >= max_ids)
				break;
		}
	}
	return result;
}

vector<long> PrepareOperation::get_split_link_ids(unsigned long max_ids)
{
	set<long> split_links;
	SplitLinkVector split_junctions = get_split_link_junction_keys(max_ids);
	for (SplitLinkVector::iterator junc = split_junctions.begin(); junc != split_junctions.end(); ++junc)
	{
		vector<Edge*> sls = net->junction_storage[junc->first]->get_outgoing_edges_for_gradesep(junc->second);
		assert(sls.size()==2);
		split_links.insert(sls[0]->link->arcid);
		split_links.insert(sls[1]->link->arcid);
	}
	return vector<long>(split_links.begin(),split_links.end());
}