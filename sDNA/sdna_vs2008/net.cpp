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
#include "geos_sdna_wrapper.h"

Net::Net() 
		: net_is_built(false), 
		attached_data(&link_container),
		attached_text_data(&link_container),
		onewaydatastrat(NULL),
		vertonewaydatastrat(NULL)
	{
	}

bool Net::add_polyline(long arcid,int geom_length,double *xs,double *ys,float *zs)
{
	try
	{
		vector<Point> points;
		for (int i=0;i<geom_length;i++)
		{
			const float z = zs?zs[i]:0;
			points.push_back(Point(xs[i],ys[i],z));
		}
		add_polyline(arcid,points);
		return true;
	}
	catch (std::bad_alloc&)
	{
		emergencyMemory.free();
		return false;
	}
}

void Net::add_polyline(long arcid,vector<Point> &pointlist)
{
	assert (!net_is_built);

	// create link
	SDNAPolyline *location = link_pool.malloc();
	SDNAPolyline *s = new(location) SDNAPolyline(arcid,pointlist,this);
	
	link_container[arcid] = s;

	// add edges in link to edge list
	Edge *forward_edge = &(s->forward_edge);
	Edge *backward_edge = &(s->backward_edge);
	edge_ptr_container.push_back(forward_edge);
	edge_ptr_container.push_back(backward_edge);

	links_not_in_junction_map_yet.push_back(s);
}

bool Net::add_polyline_data(long arcid,string name,float data)
{
	try
	{
		attached_data.attach_data(link_container[arcid],name,data);
		return true;
	}
	catch (std::bad_alloc&)
	{
		emergencyMemory.free();
		return false;
	}
}

bool Net::add_polyline_text_data(long arcid,string name,char *data)
{
	try
	{
		attached_text_data.attach_data(link_container[arcid],name,string(data));
		return true;
	}
	catch (std::bad_alloc&)
	{
		emergencyMemory.free();
		return false;
	}
}

void Net::remove_link(SDNAPolyline *s)
{
	assert(!net_is_built);

	Edge *forward_edge = &(s->forward_edge);
	Edge *backward_edge = &(s->backward_edge);
	junction_storage.remove_edge(forward_edge ,forward_edge->get_point(0)  ,EDGE_START);
	junction_storage.remove_edge(forward_edge ,forward_edge->get_point(-1) ,EDGE_END  );
	junction_storage.remove_edge(backward_edge,backward_edge->get_point(0) ,EDGE_START);
	junction_storage.remove_edge(backward_edge,backward_edge->get_point(-1),EDGE_END  );
	
	//fixme this is inefficient (linear search) alas edge_ptr_container isn't sorted at this point
	edge_ptr_container.erase(find(edge_ptr_container.begin(),edge_ptr_container.end(),forward_edge));
	edge_ptr_container.erase(find(edge_ptr_container.begin(),edge_ptr_container.end(),backward_edge));
	
	assert(link_container[s->arcid]==s);
	link_container.erase(s->arcid);
	link_pool.destroy(s);
}

void Net::ensure_junctions_created()
{
	// add ends of all edges to junction map
	for (vector<SDNAPolyline*>::iterator it=links_not_in_junction_map_yet.begin();it!=links_not_in_junction_map_yet.end();it++)
	{
		Edge* forward_edge = &((*it)->forward_edge);
		Edge* backward_edge = &((*it)->backward_edge);
		junction_storage.add_edge(forward_edge ,forward_edge->get_point(0)  ,EDGE_START);
		junction_storage.add_edge(forward_edge ,forward_edge->get_point(-1) ,EDGE_END  );
		junction_storage.add_edge(backward_edge,backward_edge->get_point(0) ,EDGE_START);
		junction_storage.add_edge(backward_edge,backward_edge->get_point(-1),EDGE_END  );
	}
	//clear and reduce capacity to 0 to save memory
	links_not_in_junction_map_yet.swap(vector<SDNAPolyline*>());
}

void Net::link_edges()
{
	if (net_is_built) unlink_edges();
	assert(junction_storage.size()!=0 || link_container.size()==0);

	// iterate through junctions
	for (JunctionContainer::iterator junc = junction_storage.begin();	junc != junction_storage.end(); junc++)
	{
		// link edges directly together:
		// iterate through source edges on this node
		for (vector<Edge*>::iterator from_edge = junc->second->incoming_edges.begin(); 
			from_edge != junc->second->incoming_edges.end(); from_edge++)
		{
			// iterate through destination edges
			for (vector <Edge*>::iterator to_edge = junc->second->outgoing_edges.begin(); 
				to_edge != junc->second->outgoing_edges.end(); to_edge++)
			{
				SDNAPolyline *from_link = (*from_edge)->link;
				SDNAPolyline *to_link = (*to_edge)->link;
				// check edges not from same link, i.e. prohibit u-turns on junctions
				// and check grade separation matches
				if (from_link != to_link
					&& (*from_edge)->get_end_gs()==(*to_edge)->get_start_gs())
					// link source to destination edge
					(*from_edge)->addOutgoingConnection(*to_edge);
			}
		}

		// link edge ends directly to junctions (may be invalidated if junction map changes)
		for (vector<Edge*>::iterator from_edge = junc->second->incoming_edges.begin(); 
			from_edge != junc->second->incoming_edges.end(); from_edge++)
				(*from_edge)->end_junction = junc->second;
	}
	net_is_built = true;
}

void Net::unlink_edges()
{
	assert(net_is_built);
	//iterate through edges clearing outgoing connections and end_junction
	for (vector<Edge*>::iterator it=edge_ptr_container.begin(); it!=edge_ptr_container.end(); ++it)
	{
		(*it)->clearOutgoingConnections();
		(*it)->end_junction = NULL;
	}
	net_is_built = false;
}

void Net::assign_junction_ids()
{
	size_t junc_id = 0;
	junction_from_id.clear();
	for (JunctionContainer::iterator it = junction_storage.begin(); it!=junction_storage.end(); ++it)
	{
		it->second->id = JunctionId(junc_id++);
		junction_from_id.push_back(it->second);
	}
}

void Net::assign_link_edge_ids()
{
	size_t id = 0;
	for (SDNAPolylineContainer::iterator it=link_container.begin(); it!=link_container.end(); ++it)
	{
		SDNAPolyline * const link = it->second;
		link->assign_id(id);
		id++;
	}
}


void Net::warn_about_invalid_turn_angles(int(__cdecl *print_warning_callback)(const char*))
{
	//only the master thread is allowed to call callbacks
	#pragma omp master
	{
		for (SDNAPolylineContainer::iterator it = link_container.begin(); it != link_container.end(); ++it)
		{
			if (it->second->is_too_short_for_valid_turn_angles())
			{
				stringstream warning;
				warning << "WARNING: Polyline " << it->second->arcid << " is too short to generate valid turn angles.";
				print_warning_callback(warning.str().c_str());
			}
		}
	}
}

void Net::print()
{
	//print edges
	for (SDNAPolylineContainer::iterator it = link_container.begin(); it!=link_container.end(); ++it)
	{
	    it->second->print();
		attached_data.print(it->second);
	}
}

void Net::add_polyline_centres(traversal_event_type centre_type)
{
	for (SDNAPolylineContainer::iterator s = link_container.begin(); s != link_container.end(); s++)
	{
		SDNAPolyline * const link = &*s->second;
		link->traversal_events.add_centre(centre_type);	
	}
}

ClusterList Net::get_near_miss_clusters(double xytol,double ztol)
{
	ensure_junctions_created();
	return ClusterFinder::get_clusters(junction_storage,xytol,ztol);
}

void Net::add_unlink(long geom_length,double *xs,double *ys)
{
	vector<Point> pv;
	for (long i=0;i<geom_length;i++)
		pv.push_back(Point(xs[i],ys[i],0));
	//close polygon if necessary
	if (pv.front() != pv.back())
		pv.push_back(pv.front());
	unlinks.push_back(pv);
}

template <typename T> bool NetExpectedDataSource<T>::init()
{
	if (!name.empty())
	{
		index = net->get_index_for_data_name(name,false,default_attached_data<T>());
		if (index==-1)
		{
			print_warning_callback(string("ERROR:  Data field '"+name+"' not found on net.").c_str());
			return false;
		}
		else
			return true;
	}
	else //empty name
	{
		assert(allow_default); // any name the user could provide should have a default if missing
		index=-1;
		return true;
	}
}

// No need to call this TemporaryFunction() function, it's just to avoid link error.
void TemporaryFunction ()
{
    NetExpectedDataSource<float> fdas;
	fdas.init();
    NetExpectedDataSource<string> sdas;
	sdas.init();
}

template<typename T> T NetExpectedDataSource<T>::get_data(const SDNAPolyline * const link)
{
	assert(is_initialized());
	if (index==-1)
		return m_defaultdata;
	else
		return net->get_data(link,index,default_attached_data<T>());
}
template<typename T> void NetExpectedDataSource<T>::set_data(SDNAPolyline * const link,T d)
	{
		assert(is_initialized());
		if (index != -1)
			net->attach_data(link,index,d);
		else
		{
			assert(!defaultname.empty());
			if (d!=m_defaultdata)
			{
				//create data here
				if (net->has_data_name(defaultname))
					print_warning_callback(string("Warning: overwriting data "+defaultname+" (no field explicitly set)").c_str());
				index = net->get_index_for_data_name(defaultname,true,m_defaultdata);
				net->attach_data(link,index,d);
			}
			//else, we are trying to store a value identical to default
			//so we can ignore it
		}
	}
