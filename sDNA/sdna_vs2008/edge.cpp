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

/////////////////////////////////////////////////////////////
// SDNAPolyline: a RoadSDNAPolyline in the ITN      
/////////////////////////////////////////////////////////////

// allow using 'this' in constructor - which is ok, but only because the Edge and CachedTraversalEventContainer
//constructors do nothing with it
#pragma warning(disable:4355) 
		
SDNAPolyline::SDNAPolyline(long a,vector<Point> &pointlist,Net *n)
	: arcid(a), net(n), 
	forward_edge(Edge(this,PLUS)), backward_edge(Edge(this,MINUS)),
	attached_data(vector<float>(n->attached_data_size())),
	attached_text_data(vector<string>(n->attached_text_data_size()))
{
	points.insert(points.end(),pointlist.begin(),pointlist.end());

	//check no points are too close together to generate a valid turn angle
	//but preserve endpoints in place to ensure connection to junctions
	PointVector::iterator it = points.begin()+1;
	while (it != points.end())
		if (Point::too_close_for_turns(*(it-1),*it))
		{
			//we need to erase something, but what?
			if (it != points.end()-1)
				//*it is middle point - fine, remove it
				it = points.erase(it);
			else
			{
				//*it is end point, remove the preceding point
				//so long as the preceding point isn't the beginning
				if (it-points.begin()>1)
					it = points.erase(it-1);
				else
				{
					assert(is_too_short_for_valid_turn_angles());
					//user will be warned of this in net::warn_about_invalid_turn_angles
					break;
				}
			}
		}
		else
			++it;
	
	create_traversal_events();
}

void SDNAPolyline::create_traversal_events()
{	
	traversal_events.clear();
	traversal_events.push_back(TraversalEvent(EndpointTE(&points[0])));
	size_t i;
	for (i=1;i<points.size()-1;i++)
	{
		traversal_events.push_back(TraversalEvent(EuclideanTE(&points[i-1],&points[i])));
		traversal_events.push_back(TraversalEvent(AngularTE(Point::turn_angle_3d(points[i-1],points[i],points[i+1]),
													&points[i])));
	}
	assert(i==points.size()-1);
	traversal_events.push_back(TraversalEvent(EuclideanTE(&points[i-1],&points[i])));
	traversal_events.push_back(TraversalEvent(EndpointTE(&points[i])));
}

bool SDNAPolyline::is_too_short_for_valid_turn_angles()
{
	return (points.size()==2 && Point::too_close_for_turns(points[0],points[1]));
}

void SDNAPolyline::print()
{
	std::cout << "Polyline " << arcid << " id=" << id.id << endl 
		<< "    Points: ";
	for (PointVector::iterator it = points.begin(); it!=points.end(); ++it)
		std::cout << "(" << it->x << "," << it->y << ") ";
	std::cout << endl;
	forward_edge.print();
	backward_edge.print();
}

// enable warning again
#pragma warning(default:4355)

double SDNAPolyline::estimate_average_inner_crow_flight_distance_ignoring_oneway(double proportion_within_radius)
{
	if (is_not_loop())
		// 1/3 of the end-to-end distance
		return Point::distance(forward_edge.get_point(0),forward_edge.get_point(-1)) / 3. * proportion_within_radius;
	else
		// take the end-to-centre distance
		return Point::distance(get_centre(),forward_edge.get_point(0)) * proportion_within_radius;
}

double SDNAPolyline::estimate_average_inner_diversion_ratio_ignoring_oneway()
{
	const double length = full_link_cost_ignoring_oneway(PLUS).euclidean;
	if (length==0) 
		return 0; //silly zero length edge
	else
	{
		if (is_not_loop())
			// estimate ratio based on traversing full link
			return (float)length / Point::distance(forward_edge.get_point(0),forward_edge.get_point(-1));
		else
			// estimate ratio based on traversing half link
			return (float)length / 2. / Point::distance(forward_edge.get_point(0),get_centre());
	}
}

vector<SDNAPolyline*> SDNAPolyline::get_linked_links()
{
	vector<SDNAPolyline*> linked_links;
	for (OutgoingConnectionVector::iterator it=forward_edge.outgoing_connections.begin();it!=forward_edge.outgoing_connections.end();++it)
		linked_links.push_back(it->edge->link);
	for (OutgoingConnectionVector::iterator it=backward_edge.outgoing_connections.begin();it!=backward_edge.outgoing_connections.end();++it)
		linked_links.push_back(it->edge->link);
	return linked_links;
}

int SDNAPolyline::get_vert_oneway_data()
{
	const int rawdata = boost::math::sign(net->vertonewaydatastrat->get_data(this));
	const bool link_is_uphill = get_end_z()>get_start_z();
	return link_is_uphill?rawdata:-rawdata;
}

/////////////////////////////////////////////////////////////
// Junction
/////////////////////////////////////////////////////////////

bool Junction::two_edges_per_line_end()
{
	//test assumption that each link has an incoming and outgoing edge
	//(one way streets are modelled with disabled edges, so don't affect this)
	vector<SDNAPolyline*> incoming_links, outgoing_links;
	BOOST_FOREACH(Edge* e,incoming_edges)
		incoming_links.push_back(e->link);
	BOOST_FOREACH(Edge* e,outgoing_edges)
		outgoing_links.push_back(e->link);
	sort(incoming_links.begin(),incoming_links.end());
	sort(outgoing_links.begin(),outgoing_links.end());
	return incoming_links==outgoing_links;
}

vector<Edge*> Junction::get_outgoing_edges_for_gradesep(float gradesep)
{
	vector<Edge*> result;
	result.reserve(10);
	BOOST_FOREACH(Edge *e,outgoing_edges)
		if (e->get_start_gs()==gradesep)
			result.push_back(e);
	return result;
}

void Junction::get_attached_links(set<SDNAPolyline*> &attached_links)
{
	assert(attached_links.size()==0);
	for (vector<Edge*>::iterator it=incoming_edges.begin(); it!=incoming_edges.end(); it++)
		attached_links.insert((*it)->link);
	for (vector<Edge*>::iterator it=outgoing_edges.begin(); it!=outgoing_edges.end(); it++)
		attached_links.insert((*it)->link);
}

float Junction::get_connectivity_respecting_oneway(Net *net,float gs)
{
	if (net->onewaydatastrat->using_default() && net->vertonewaydatastrat->using_default())
	{
		assert(two_edges_per_line_end());
		return numeric_cast<float>(get_outgoing_edges_for_gradesep(gs).size());
	}
	else
	{
		long result = 0;
		for (vector<Edge*>::iterator it=incoming_edges.begin();it!=incoming_edges.end();it++)
			if ((*it)->get_end_gs()==gs && (*it)->traversal_allowed()) result++;
		for (vector<Edge*>::iterator it=outgoing_edges.begin();it!=outgoing_edges.end();it++)
			if ((*it)->get_start_gs()==gs && (*it)->traversal_allowed()) result++;
		return ((float)result)/2.f;
	}
}

vector<float> Junction::get_gradeseps()
{
	assert (two_edges_per_line_end());
	//...therefore ignore incoming edges - they will be for the same links as the outgoing ones
	vector<float> gradeseps;
	gradeseps.reserve(3);
	BOOST_FOREACH(Edge *e,outgoing_edges)
	{
		const float gs = e->get_start_gs();
		if (find(gradeseps.begin(),gradeseps.end(),gs)==gradeseps.end())
			gradeseps.push_back(gs);
	}
	return gradeseps;
}

/////////////////////////////////////////////////////////////
// Edge: one direction on one link, i.e. a network edge      
/////////////////////////////////////////////////////////////

Edge::Edge(SDNAPolyline *s,polarity d)
		: link(s), direction(d), end_junction(NULL)
{}

Edge* Edge::get_twin() const
{
	Edge *e;
	switch(direction)
	{
	case PLUS:
		e = &link->backward_edge;
		break;
	case MINUS:
		e = &link->forward_edge;
		break;
	default:
		assert (0);
	}
	return e;
}

void Edge::addOutgoingConnection(Edge *destination)
{
	OutgoingConnection ogc(destination,
		Point::turn_angle_3d(get_point(-2),get_point(-1),destination->get_point(1)));
	outgoing_connections.push_back(ogc);
}

void Edge::clearOutgoingConnections()
{
	outgoing_connections.clear();
}

float Edge::get_start_gs() const
{
	float e;
	switch(direction)
	{
	case PLUS:
		e = link->get_start_gs();
		break;
	case MINUS:
		e = link->get_end_gs();
		break;
	default:
		assert (0);
	}
	return e;
}

float Edge::get_end_gs() const
{
	float e;
	switch(direction)
	{
	case PLUS:
		e = link->get_end_gs();
		break;
	case MINUS:
		e = link->get_start_gs();
		break;
	default:
		assert (0);
	}
	return e;
}

Point & Edge::get_point(point_index_t i) const
{
	if (i<0)
		i += link->points.size();
	Point *p;
	switch(direction)
	{
	case PLUS:
		p = &link->points[i];
		break;
	case MINUS:
		p = &link->points[link->points.size()-1-i];
		break;
	default:
		assert (0);
	}
	return *p;
}

void Edge::print() const
{
	string dirstring;
	switch(direction)
	{
	case PLUS:
		dirstring = "+";
		break;
	case MINUS:
		dirstring = "-";
		break;
	default:
		dirstring = "?";
		break;
	}
	std::cout << "    edge " << id.id << " (s" << link->arcid << dirstring << "), conn: " ;
	for (OutgoingConnectionVector::const_iterator it = outgoing_connections.begin(); it!=outgoing_connections.end(); ++it)
	{
		std::cout << (*it).edge->id.id;
		std::cout << "(€ang" << (*it).turn_angle << ") ";
	}
	std::cout << endl;
}


void Edge::get_outgoing_connections(CandidateEdgeVector &output,double cost_to_date,double remaining_radius,MetricEvaluator* evaluator,MetricEvaluator *radial_evaluator,
									edge_position from,	JunctionCosts *jcosts) 
{
	//this method has to keep track of remaining_radius as well as cost_to_data
	//why?! surely we know already whether the edges returned are within the radius already, from the last run of dijkstra?!
	//WELL, we do, but with strict network cutting we don't know whether we're allowed to reach them by traversing this edge
	//so that's why we keep track of remaining_radius

	assert(from==START || from==MIDDLE);
	assert(link->net->onewaydatastrat && link->net->vertonewaydatastrat);
	output.clear();
	
	if (!traversal_allowed())
		return; //return empty output

	// calculate cost of reaching edges linked from end
	//i'm not quite sure what happens here in the case of zero length edges with custom costs
	//it seems to be ok, even though get_end_traversal_cost should return a full cost in this case in theory
	TraversalEventAccumulator edge_exit_acc = (from==START)?full_cost_ignoring_oneway():get_end_traversal_cost_ignoring_oneway();
	double cost_reaching_turn = cost_to_date + evaluate_me(evaluator,edge_exit_acc);
	if (remaining_radius < GLOBAL_RADIUS)
	{
		assert(radial_evaluator);
		remaining_radius -= evaluate_me(radial_evaluator,edge_exit_acc);
	}
	
	//optimization but also essential for jcost accuracy
	if (remaining_radius < 0)
		return;

	//we only use jcosts for radial analysis 
	assert(end_junction);
	if (jcosts && (*jcosts)[*end_junction]>cost_reaching_turn)
		(*jcosts)[*end_junction]=cost_reaching_turn;

	for (OutgoingConnectionVector::const_iterator it = outgoing_connections.begin(); it != outgoing_connections.end(); it++)
	{
		Edge * const destination_edge = it->edge;
		if (!destination_edge->traversal_allowed())
			continue;
		if (remaining_radius < GLOBAL_RADIUS
			&& remaining_radius - it->get_turn_cost(radial_evaluator,this) <= 0)
			continue;
		assert(traversal_allowed());
		assert(destination_edge->traversal_allowed());
		double destination_edge_cost = cost_reaching_turn + it->get_turn_cost(evaluator,this); 
		const CandidateEdge ce(destination_edge,destination_edge_cost,this);
		output.push_back(ce);
	}
}

TraversalEventIterator Edge::traversal_events_begin() const
{
	return link->traversal_events.begin(direction);
}

TraversalEventIterator TraversalEventContainer::begin(polarity direction)
{
	switch (direction)
	{
	case PLUS:
		return TraversalEventIterator(vector<TraversalEvent>::begin());
	case MINUS:
		return TraversalEventIterator(vector<TraversalEvent>::rbegin());
	default:
		assert(0);
		return TraversalEventIterator();
	}
}

TraversalEventIterator Edge::traversal_events_end() const
{
	return link->traversal_events.end(direction);
}

TraversalEventIterator TraversalEventContainer::end(polarity direction)
{
	switch (direction)
	{
	case PLUS:
		return TraversalEventIterator(vector<TraversalEvent>::end());
	case MINUS:
		return TraversalEventIterator(vector<TraversalEvent>::rend());
	default:
		assert(0);
		return TraversalEventIterator();
	}
}

//handy function, because reverse iterators point to the preceding element
TraversalEventIterator reverse_iterator_to_same_element(TraversalEventContainer::iterator it)
{
	return TraversalEventIterator(std::reverse_iterator<TraversalEventContainer::iterator>(it + 1));
}

TraversalEventIterator Edge::traversal_events_centre() const
{
	return link->traversal_events.centre(direction);
}

TraversalEventIterator TraversalEventContainer::centre(polarity direction)
{
	assert(has_centre);
	switch (direction)
	{
	case PLUS:
		return TraversalEventIterator(centre_located_on_event);
	case MINUS:
		return reverse_iterator_to_same_element(centre_located_on_event);
	default:
		assert(0);
		return TraversalEventIterator();
	}
}

float Edge::evaluate_me(MetricEvaluator* e,TraversalEventAccumulator& acc) const
{
	return e->evaluate_edge(acc,this);
}

TraversalEventAccumulator Edge::full_cost_ignoring_oneway() const
{
	return link->traversal_events.full_cost_ignoring_oneway(direction);
}

TraversalEventAccumulator Edge::get_start_traversal_cost_ignoring_oneway() const
{
	assert(direction==PLUS || direction==MINUS);
	if (direction==PLUS)
		return link->traversal_events.get_start_traversal_cost_ignoring_oneway(PLUS);
	else
		return link->traversal_events.get_end_traversal_cost_ignoring_oneway(MINUS);
}

TraversalEventAccumulator Edge::get_end_traversal_cost_ignoring_oneway() const
{
	assert(direction==PLUS || direction==MINUS);
	if (direction==PLUS)
		return link->traversal_events.get_end_traversal_cost_ignoring_oneway(PLUS);
	else
		return link->traversal_events.get_start_traversal_cost_ignoring_oneway(MINUS);
}

//used for crow flight, etc
Point Edge::get_centre(double partial_length) const
{
	if (partial_length >= full_cost_ignoring_oneway().euclidean)
		return link->get_centre();
	else
	{
		assert(traversal_allowed());
		TraversalEventContainer temp = PartialEdge(traversal_events_begin(),traversal_events_end(),(float)partial_length,
			&link->traversal_events,direction);
		temp.add_centre(link->traversal_events.get_centre_type());
		Point &p = temp.get_centre();
		#ifdef _SDNADEBUG
				cout << "centre of cut link is at ";
				p.print();
				cout << endl;
		#endif
		return p;
	}
}

TraversalEventAccumulator Edge::partial_cost_from_start(float partial_length) const
{
	if (partial_length==0)
		return TraversalEventAccumulator::zero();
	else
	{
		if (partial_length >= full_cost_ignoring_oneway().euclidean)
			return full_cost_ignoring_oneway();
		else
			return link->traversal_events.partial_cost_from_iterators_ignoring_oneway(traversal_events_begin(),traversal_events_end(),partial_length,direction);
	}
}

TraversalEventAccumulator Edge::partial_cost_from_middle_ignoring_oneway(float partial_length) const
{
	if (partial_length <=0)
		return TraversalEventAccumulator::zero();
	else if (partial_length >= get_end_traversal_cost_ignoring_oneway().euclidean)
		return get_end_traversal_cost_ignoring_oneway();
	else
		return link->traversal_events.partial_cost_from_iterators_ignoring_oneway(traversal_events_centre(),traversal_events_end(),partial_length,direction);
}

//csccl = cost from start to centre of cut link
TraversalEventAccumulator Edge::csccl(float partial_length) const
{
	if (partial_length==0)
		return TraversalEventAccumulator::zero();
	else
	{
		if (partial_length>=full_cost_ignoring_oneway().euclidean)
			return get_start_traversal_cost_ignoring_oneway();
		else
			return csccl_internal(partial_length);
	}
}

//the costly csccl logic without optimization wrapper
TraversalEventAccumulator Edge::csccl_internal(float partial_length) const
{
	TraversalEventContainer temp = PartialEdge(traversal_events_begin(),traversal_events_end(),partial_length,
				&link->traversal_events,direction);
	temp.add_centre(link->traversal_events.get_centre_type());
	return temp.get_start_traversal_cost_ignoring_oneway(PLUS); //reversal happened when temp was created
}

//cscclfe = cost from start to centre of cut link at far end
//in other words, cuts off partial_length from the FAR end of the link, finds the centre of the cut section,
//then works out the cost from THIS END to the centre of the cut section at the FAR END
//can it work with get_twin?  where is it used?
TraversalEventAccumulator Edge::cscclfe(float partial_length) const
{
	if (partial_length==0)
		return full_cost_ignoring_oneway();
	else
	{
		if (partial_length>=full_cost_ignoring_oneway().euclidean)
			return get_start_traversal_cost_ignoring_oneway();
		else
		{
			const float forward_length_to_centre_far_end = full_cost_ignoring_oneway().euclidean - get_twin()->csccl_internal(partial_length).euclidean;
			//as for partial_cost_from_start without the optimizations:
			return link->traversal_events.partial_cost_from_iterators_ignoring_oneway(traversal_events_begin(),traversal_events_end(),
				forward_length_to_centre_far_end,direction);
		}
	}
}

void Edge::add_partial_points_to_geometry(BoostLineString3d &geom,float partial_length) const
{
	#ifdef _SDNADEBUG
		cout << "     unpacking partial points from end of edge " << id.id << endl;
	#endif
	PartialEdge(traversal_events_begin(),traversal_events_end(),partial_length,&link->traversal_events,direction)
		.add_points_to_geometry(geom);
}

void Edge::add_partial_points_from_middle_to_geometry_ignoring_oneway(BoostLineString3d &geom,float partial_length) const
{
	#ifdef _SDNADEBUG
		cout << "     unpacking partial points from middle of edge " << id.id << endl;
	#endif
	PartialEdge(traversal_events_centre(),traversal_events_end(),partial_length,&link->traversal_events,direction)
		.add_points_to_geometry(geom);
}

void Edge::add_copy_of_points_from_end_to_vector(junction_option_type end,vector<Point> &v) const
{
	//IteratorTypeErasure doesn't allow wrapping of erased type iterators
	//inside other erased type iterators, which would be a neat way to solve this
	//so we use a more brute force method instead
	assert(end==EDGE_START || end==EDGE_END);
	polarity return_link_points_direction;
	switch (direction)
	{
	case PLUS:
		return_link_points_direction = (end==EDGE_START)?PLUS:MINUS;
		break;
	case MINUS:
		return_link_points_direction = (end==EDGE_START)?MINUS:PLUS;
		break;
	default:
		assert(false);
	}

	switch (return_link_points_direction)
	{
	case PLUS:
		v.insert(v.end(),link->points.begin(),link->points.end());
		break;
	case MINUS:
		v.insert(v.end(),link->points.rbegin(),link->points.rend());
		break;
	default:
		assert(false);
	}
}

junction_option_type Edge::link_end_of_edge(junction_option_type edge_end)
{
	switch (direction)
	{
	case PLUS:
		return edge_end;
	case MINUS:
		switch (edge_end)
		{
		case EDGE_START:
			return EDGE_END;
		case EDGE_END:
			return EDGE_START;
		default:
			assert(false);
		}
	default:
		assert(false);
	}
	return EDGE_START; //to shut up compiler error C2440
}

bool Edge::traversal_allowed() const
{ 
	const int onewaydata = boost::math::sign(link->net->onewaydatastrat->get_data(link));
	const int vertonewaydata = link->get_vert_oneway_data();
	//tested in check_no_oneway_conflicts():
	assert(!(onewaydata>0 && vertonewaydata<0) && !(onewaydata<0 && vertonewaydata>0));
	const int actualonewaydata = (onewaydata==0)?vertonewaydata:onewaydata;
	assert(direction==PLUS||direction==MINUS);
	//invert depending on fwd/bwdness of edge
	return (actualonewaydata==0
		|| ((direction==PLUS)==(actualonewaydata>0)));
}

/////////////////////////////////////////////////////////////
// TraversalEventContainer: holds the cost events in the edge      
/////////////////////////////////////////////////////////////

TraversalEventAccumulator TraversalEventContainer::full_cost_ignoring_oneway(polarity direction) 
{
	return partial_cost_from_iterators_ignoring_oneway(TraversalEventIterator(vector<TraversalEvent>::begin()),
										TraversalEventIterator(vector<TraversalEvent>::end()),
										numeric_limits<float>::infinity(),direction);
}

TraversalEventAccumulator TraversalEventContainer::get_start_traversal_cost_ignoring_oneway(polarity direction)
{
	assert(has_centre);
	return partial_cost_from_iterators_ignoring_oneway(TraversalEventIterator(vector<TraversalEvent>::begin()),
										TraversalEventIterator(centre_located_on_event),
										numeric_limits<float>::infinity(),direction);
}

TraversalEventAccumulator TraversalEventContainer::get_end_traversal_cost_ignoring_oneway(polarity direction)
{
	assert(has_centre);
	return partial_cost_from_iterators_ignoring_oneway(TraversalEventIterator(centre_located_on_event),
										TraversalEventIterator(vector<TraversalEvent>::end()),
											numeric_limits<float>::infinity(),direction);
}

TraversalEventAccumulator TraversalEventContainer::partial_cost_from_iterators_ignoring_oneway(TraversalEventIterator start, TraversalEventIterator end, 
	float partial_length, polarity direction) 
{
	return PartialEdge(start,end,partial_length,this,direction).full_cost();
}

void TraversalEventContainer::simplify()
{
	//erase any centre 
	TraversalEventContainer::iterator it = cost_events_begin(PLUS);
	while(it < cost_events_end(PLUS) )
		if (it->empty_costs() || it->type()==CENTRE_TE)
			it = this->erase(it); 
		else
			it++;

	has_centre = false;

	//merge adjacent elements with same type
	if (cost_events_size()>1)
	{
		it = cost_events_begin(PLUS)+1;
		while(it<cost_events_end(PLUS))
			if ((it-1)->type() == it->type())
			{
				*(it-1) += *it;
				it = this->erase(it);
			}
			else
				it++;
	}
}

void TraversalEventContainer::add_centre(traversal_event_type tetype)
{
	assert (tetype==ANGULAR_TE || tetype==EUCLIDEAN_TE);

	//remove existing centre and join events of same type
	simplify();

	//if no cost events (possible when adding centres to cut links, or zero length edges...)
	//still add a centre
	if (cost_events_size()==0)
		give_centre_to_zero_length_tev();

	//otherwise, traverse vector twice to find centre
	else
	{
		shared_ptr<MetricEvaluator> me = MetricEvaluator::from_event_type(tetype);

		//NULL here is safe as our evaluator should not be a kind that accesses it
		float half_cost = me->evaluate_edge(full_cost_ignoring_oneway(PLUS),NULL) / 2;
		float cumulative_cost = 0;
		TraversalEventContainer::iterator it = cost_events_begin(PLUS);
		float current_event_cost;
		for (;it!=cost_events_end(PLUS);it++)
		{
			//NULL here is safe as our evaluator should not be a kind that accesses it
			current_event_cost = me->evaluate_edge(it->costs(PLUS),NULL);
			cumulative_cost += current_event_cost;
			if (cumulative_cost >= half_cost)
				break;
		}

		if (half_cost==0)
		{
			//a link with only costs of the type we're not measuring.  Split on Euclidean measure
			add_centre(EUCLIDEAN_TE);
		}
		else
		{
			TraversalEventContainer::iterator centre;
			if (cumulative_cost==half_cost)
			{
				//centre falls exactly between elements
				//so we put it on the next element (not that it matters) 
				//split following element
				centre = it+1;
				assert(it->type() != centre->type()); //should be guaranteed by simplify()
				assert(centre->type() != (centre+1)->type()); //ditto
				TraversalEventAccumulator centre_cost = centre->costs(PLUS);
				assert (centre_cost.euclidean !=0 || centre_cost.angular !=0);
				assert(centre < cost_events_end(PLUS));
				//the following element is thus guaranteed to be of different type to what we are measuring
				//so costs of the central element are irrelevant to us - split evenly
				set_centre_split(centre,0.5);
			}
			else
				//centre is on current element (which is of type tetype)
				//split element
				set_centre_split(it,1-(cumulative_cost-half_cost)/current_event_cost);
		}
	}
	centre_type = tetype;
}

void TraversalEventContainer::give_centre_to_zero_length_tev()
{
	TraversalEventContainer::iterator centre_pos = vector<TraversalEvent>::begin()+1;
	centre_pos = insert(centre_pos,TraversalEvent(MidpointTE((*vector<TraversalEvent>::begin()).physical_location())));
	centre_located_on_event = centre_pos;
	has_centre = true;
}

void TraversalEventContainer::set_centre_split(TraversalEventContainer::iterator event_to_split,
											float split_ratio)
{
	const pair<TraversalEvent,TraversalEvent> split_halves = event_to_split->split_te(split_ratio);
	
	//calculate centre physical location
	Point *centre_physical_location;
	if (event_to_split->physical_location() != NULL)
	{
		//angular centre
		centre_physical_location = event_to_split->physical_location();
		assert(event_to_split->type() == ANGULAR_TE);
	}
	else
	{
		//euclidean centre
		extra_centre_point = Point::proportional_midpoint((event_to_split-1)->physical_location(),
							  							  (event_to_split+1)->physical_location(),
														  split_ratio);
		centre_physical_location= &extra_centre_point;
	}

	//update event to split
	*event_to_split = split_halves.second;

	//insert CENTRE event and split first half (working backwards through container from second half)
	typedef TraversalEventContainer::iterator Pos;
	const Pos place_to_put_first_half = insert(event_to_split,TraversalEvent(MidpointTE(centre_physical_location)));
	const Pos first_half_location = insert(place_to_put_first_half,split_halves.first);
	centre_located_on_event = first_half_location + 1;
	has_centre = true;
}

void TraversalEventContainer::print()
{
	if (has_centre)
		cout << "has centre offset=" << centre_located_on_event-vector<TraversalEvent>::begin() << " type==" << centre_type << endl;
	else
		cout << "no centre" << endl;

	for (TraversalEventContainer::iterator it = vector<TraversalEvent>::begin(); it!=vector<TraversalEvent>::end(); it++)
		cout << it->toString() << ",";
	cout << endl;
}

/////////////////////////////////////////////////////////////
// CachedTraversalEventContainer: like its parent but caches frequently requested data
/////////////////////////////////////////////////////////////

TraversalEventAccumulator CachedTraversalEventContainer::full_cost_ignoring_oneway(polarity direction) 
{
	assert(direction==PLUS || direction==MINUS);
	if (cached_full_cost.is_bad())
		cached_full_cost = TraversalEventContainer::full_cost_ignoring_oneway(PLUS);
	TraversalEventAccumulator retval;
	if (direction==PLUS)
		retval = cached_full_cost;
	else
		retval = cached_full_cost.get_reversed_version(); //bodge
	//inefficient but checks bodge works and is optimized out in release
	assert (retval == TraversalEventContainer::full_cost_ignoring_oneway(direction));
	return retval;
}

TraversalEventAccumulator CachedTraversalEventContainer::get_start_traversal_cost_ignoring_oneway(polarity direction)
{
	assert(direction==PLUS || direction==MINUS);
	if (cached_start_traversal_cost.is_bad())
		cached_start_traversal_cost = TraversalEventContainer::get_start_traversal_cost_ignoring_oneway(PLUS);
	TraversalEventAccumulator retval;
	if (direction==PLUS)
		retval = cached_start_traversal_cost;
	else
		retval = cached_start_traversal_cost.get_reversed_version(); //bodge
	//inefficient but checks bodge works and is optimized out in release
	assert (retval == TraversalEventContainer::get_start_traversal_cost_ignoring_oneway(direction));
	return retval;
}

TraversalEventAccumulator CachedTraversalEventContainer::get_end_traversal_cost_ignoring_oneway(polarity direction)
{
	if (cached_end_traversal_cost.is_bad())
		cached_end_traversal_cost = TraversalEventContainer::get_end_traversal_cost_ignoring_oneway(PLUS);
	TraversalEventAccumulator retval;
	if (direction==PLUS)
		retval = cached_end_traversal_cost;
	else
		retval = cached_end_traversal_cost.get_reversed_version(); //bodge
	//inefficient but checks bodge works and is optimized out in release
	assert (retval == TraversalEventContainer::get_end_traversal_cost_ignoring_oneway(direction));
	return retval;
}

void CachedTraversalEventContainer::mark_cache_invalid()
{
	cached_full_cost.set_bad();
	cached_start_traversal_cost.set_bad();
	cached_end_traversal_cost.set_bad();
}

void CachedTraversalEventContainer::set_centre_split(TraversalEventContainer::iterator centre,float new_centre_second_half_value)
{
	TraversalEventContainer::set_centre_split(centre,new_centre_second_half_value);
	mark_cache_invalid();
}

void CachedTraversalEventContainer::give_centre_to_zero_length_tev()
{
	TraversalEventContainer::give_centre_to_zero_length_tev();
	mark_cache_invalid();
}

/////////////////////////////////////////////////////////////
// PartialEdge: used to divide edges in continuous space
// Can either give full cost or convert to a TraversalEventContainer for adding centre           
/////////////////////////////////////////////////////////////

#ifdef _SDNADEBUG
	bool PartialEdge::debug = false;
#endif

PartialEdge::PartialEdge(TraversalEventIterator from, TraversalEventIterator to, float partial_length, 
						 TraversalEventContainer* const parent_traversal_event_vector,polarity direction)
	: next(from), end(to), remaining_length(partial_length), valid(true), has_endpoint_left_to_emit(false),
	parent_traversal_event_vector(parent_traversal_event_vector), direction(direction)
{
	assert(next->physical_location()!=NULL);
	assert(from != to); // so we get startpoints and endpoints

	if (debug)
		cout << "partial edge called length=" << partial_length << " ";

	//ensure no points whatsoever come out (not even start/endpoints) if partial length is negative
	if (partial_length < 0)
		next = end;
}

TraversalEvent PartialEdge::next_event_inner()
{
	if (has_endpoint_left_to_emit)
	{
		has_endpoint_left_to_emit = false;
		return TraversalEvent(EndpointTE(&end_point_storage));
	}

	const TraversalEventIterator current = next++;
	
	if (current->type()==EUCLIDEAN_TE)
	{
		//keep track of length
		const float current_length = current->costs(PLUS).euclidean;
		remaining_length -= current_length;
		if (remaining_length < 0)
		{
			//prevent further events from being returned
			next = end;

			//calculate how much cost is included in final euclidean event, 
			//by subtracting (adding the negative) remaining length
			const float included_length = current_length + remaining_length;
			const float ratio = included_length/current_length;

			//setup PartialEdge for emission of endpoint
			has_endpoint_left_to_emit = true;
			assert((current+1)->physical_location()!=NULL);
			assert((current-1)->physical_location()!=NULL);
			end_point_storage = Point::proportional_midpoint((current-1)->physical_location(),
															 (current+1)->physical_location(),
															 ratio);
			
			//return final euclidean event
			return TraversalEvent(current->split_te(ratio).first);
		}
	}

	//if we reach here, the edge has not ended, so just return the next event 
	if (remaining_length > 0)
		return *current;
	else
	{
		assert (remaining_length==0);
		// ...unless we are exactly on the end of the partial link
		// If the cost is euclidean, return in full; else return half the cost
		// (this has the effect of splitting corners in the middle)
		// (this point also gets reached three times in the case of a split angular centre)
		if (current->type()==EUCLIDEAN_TE)
			return *current;
		else
			//this might be an ENDPOINT, MIDPOINT or ANGULAR TE 
			return current->split_te(0.5).first;
	}
}

TraversalEvent PartialEdge::next_event()
{
	TraversalEvent retval = next_event_inner();
	if (debug)
	{
		cout << retval.toString();
		if (!has_more_events() && !has_endpoint_left_to_emit)
			cout << endl;
	}
	return retval;
}

PartialEdge::operator TraversalEventContainer()
{
	assert(valid);
	valid = false;

	//this should only happen if partial length is negative,
	//in which case we shouldn't try to convert to a traversaleventvector
	//which has start and end points so is inconsistent with the empty vector abstraction
	assert(next != end); 

	TraversalEventContainer v;
	v.reserve(parent_traversal_event_vector->size()+2); //+2 as add_centre will be called
	while (has_more_events())
		v.push_back(next_event());
	return v;
}

TraversalEventAccumulator PartialEdge::full_cost()
{
	assert(valid);
	valid = false;
	TraversalEventAccumulator acc;
	while (has_more_events())
		next_event().add_to_accumulator(&acc,direction);
	return acc;
}

void PartialEdge::add_points_to_geometry(BoostLineString3d &geom)
{
	assert(valid);
	valid = false;

	//todo: maybe make my points support boost
	//and work out how to use multi_point and icc
	using boost::geometry::append;
	using boost::geometry::make;
	#ifdef _SDNADEBUG
		cout << "     ";
	#endif
	while (has_more_events())
	{
		//should work because code in constructor checks for this
		assert(remaining_length >= 0 || has_endpoint_left_to_emit);

		TraversalEvent te = next_event();
		if (te.physical_location()!= NULL)
		{
			append(geom,
				   point_xyz(te.physical_location()->x,te.physical_location()->y,static_cast<float>(te.physical_location()->z)));
			#ifdef _SDNADEBUG
				te.physical_location()->print();
			#endif
		}
	}
	#ifdef _SDNADEBUG
		cout << endl;
	#endif
}

void PartialEdge::printinternal()
{
	assert(valid);
	valid = false;

	cout << "     ";
	while (has_more_events())
	{
		TraversalEvent te = next_event();
		if (te.physical_location() != NULL)
		{
			te.physical_location()->print();
		}
	}
	cout << endl;
}

/////////////////////////////////////////////////////////////
// JunctionContainer: indexes Junctions 
// JunctionMapKey: spatial index for Junction map                  
/////////////////////////////////////////////////////////////

JunctionMapKey::JunctionMapKey(double xx,double yy,float zz)
		: x(xx), y(yy), z(zz) {}

bool operator< (const JunctionMapKey &a,const JunctionMapKey &b)
{
	if (a.x != b.x)
		return a.x < b.x;
	else
		if (a.y != b.y)
			return a.y < b.y;
		else
			return a.z < b.z;
}

void JunctionContainer::add_edge(Edge *e,Point p,junction_option_type type)
{
	JunctionMapKey key(p.x,p.y,p.z);
	Junction *j = add_or_get(key);

	// add edge to junction
	switch (type)
	{
	case EDGE_START:
		j->outgoing_edges.push_back(e);
		break;
	case EDGE_END:
		j->incoming_edges.push_back(e);
		break;
	default:
		assert (0);
	}
}

void JunctionContainer::remove_edge(Edge *e,Point p,junction_option_type type)
{
	JunctionMapKey key(p.x,p.y,p.z);
	
	Junction* j = operator[](key);
	
	// remove edge from junction
	switch (type)
	{
	case EDGE_START:
		j->outgoing_edges.erase(std::find(j->outgoing_edges.begin(),j->outgoing_edges.end(),e));
		break;
	case EDGE_END:
		j->incoming_edges.erase(std::find(j->incoming_edges.begin(),j->incoming_edges.end(),e));
		break;
	default:
		assert (0);
	}

	if (j->outgoing_edges.size()==0 && j->incoming_edges.size()==0)
		erase(j);
}

float SDNAPolyline::get_start_gs()
{
	return net->start_gsdata.get_data(this);
}

float SDNAPolyline::get_end_gs()
{
	return net->end_gsdata.get_data(this);
}



string Prettifier<Edge*>::prettify(Edge * data)
	{
		stringstream ss;
		if (data==NULL)
			ss << "-";
		else
			ss << data->get_id().id;
		return ss.str();
	}
