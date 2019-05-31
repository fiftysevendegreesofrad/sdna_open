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
#pragma once

#include "unique_id.h"
#include "point.h"

using namespace boost;
using namespace std;

//we represent the largest possible radius with max_double, and unconnected links with infinity
const double GLOBAL_RADIUS = std::numeric_limits<double>::max();
const double UNCONNECTED_RADIUS = numeric_limits<double>::infinity();

struct Edge;
struct SDNAPolyline;
struct Junction;

typedef UniqueId<Edge> EdgeId;
typedef UniqueId<SDNAPolyline> SDNAPolylineId;
typedef UniqueId<Junction> JunctionId;

typedef map<long,SDNAPolyline* > SDNAPolylineContainer;

#include "metricevaluator.h"

enum junction_option_type {EDGE_START, EDGE_END}; 

struct Junction : public IndexedObject<JunctionId>
{
	//fixme these vectors should be for the same links in each case
	//and identical in size
	//best to refactor all access to these into some access methods
	//- we could move a lot inside a junction class really
	vector<Edge*> incoming_edges, outgoing_edges;
	JunctionId id;

	Junction() : id(JunctionId(-1)) {}; 

	virtual inline JunctionId get_id() const {return id;}

	vector<float> get_gradeseps();
	void get_attached_links(set<SDNAPolyline*> &attached_links);
	float get_connectivity_respecting_oneway(Net *net,float gradesep);
	bool two_edges_per_line_end();
	vector<Edge*> get_outgoing_edges_for_gradesep(float gradesep);
};

template<class T,class INDEX> class IdIndexedArray;
typedef IdIndexedArray<double,JunctionId> JunctionCosts;

enum edge_position {START, MIDDLE};

inline polarity reverse_polarity(polarity p)
{
	switch (p)
	{
	case PLUS:
		return MINUS;
	case MINUS:
		return PLUS;
	default:
		assert(0);
		return (polarity)-1;
	}
}

struct OutgoingConnection;
typedef vector<OutgoingConnection > OutgoingConnectionVector;
struct CandidateEdge;
typedef vector<const CandidateEdge> CandidateEdgeVector;
typedef IteratorTypeErasure::any_iterator<TraversalEvent,random_access_iterator_tag> TraversalEventIterator;

typedef vector<Point > PointVector;
typedef IteratorTypeErasure::make_any_iterator_type<PointVector::iterator>::type PointIterator;

class TraversalEventContainer : public vector<TraversalEvent >
{
protected:
	void simplify();
	//these are virtual as subclass needs to cache_costs afterwards
	virtual void set_centre_split(TraversalEventContainer::iterator centre,float new_centre_second_half_value);
	virtual void give_centre_to_zero_length_tev();
	
	//beginning and end are ENDPOINT events, so...
	TraversalEventContainer::iterator cost_events_begin(polarity direction) 
	{ 
		assert(direction==PLUS); //only used to add centres but I want awareness of directionality when this is called
		return vector<TraversalEvent>::begin()+1; 
	}
	TraversalEventContainer::iterator cost_events_end(polarity direction) 
	{ 
		assert(direction==PLUS); //only used to add centres but I want awareness of directionality when this is called
		return vector<TraversalEvent>::end()-1; 
	}
	size_t cost_events_size()
	{
		if (has_centre)
			return size()-3;
		else
			return size()-2;
	}

	//used to store physical location of centre if and only if it doesn't fall on an existing point in the network
	Point extra_centre_point;
	
	TraversalEventContainer::iterator centre_located_on_event;
	bool has_centre;
	traversal_event_type centre_type;

public:
	traversal_event_type get_centre_type() {return centre_type;}

	TraversalEventIterator begin(polarity direction);
	TraversalEventIterator centre(polarity direction);
	TraversalEventIterator end(polarity direction);


	TraversalEventContainer() : has_centre(false) {}

	TraversalEventAccumulator partial_cost_from_iterators_ignoring_oneway(TraversalEventIterator start, TraversalEventIterator end, float partial_length, polarity direction) ;
	
	void print();
	void add_centre(traversal_event_type mtype);

	Point& get_centre()
	{
		assert(has_centre);
		return *(centre_located_on_event->physical_location());
	}
		
	//these are overridden by cacheing subclass
	virtual TraversalEventAccumulator full_cost_ignoring_oneway(polarity direction) ;                //must be virtual as partialedge calls it as well as edge
	virtual TraversalEventAccumulator get_start_traversal_cost_ignoring_oneway(polarity direction);  //could try making this non virtual for performance increase at cost of coupling
	virtual TraversalEventAccumulator get_end_traversal_cost_ignoring_oneway(polarity direction);    //ditto (also see the subclass methods below)
};

class CachedTraversalEventContainer : public TraversalEventContainer
{
private:
	TraversalEventAccumulator cached_full_cost,cached_start_traversal_cost,cached_end_traversal_cost;
	
	//these are virtual as they need to override parent class call in order to cache_costs afterwards
	virtual void set_centre_split(TraversalEventContainer::iterator centre,float new_centre_second_half_value);
	virtual void give_centre_to_zero_length_tev();
	
	void mark_cache_invalid();

public:
	CachedTraversalEventContainer() { mark_cache_invalid(); }
	virtual TraversalEventAccumulator full_cost_ignoring_oneway(polarity direction);
	virtual TraversalEventAccumulator get_start_traversal_cost_ignoring_oneway(polarity direction);
	virtual TraversalEventAccumulator get_end_traversal_cost_ignoring_oneway(polarity direction);
	void clear() { TraversalEventContainer::clear(); mark_cache_invalid(); }
	void push_back(const value_type& t) { TraversalEventContainer::push_back(t); mark_cache_invalid(); }
};

class PartialEdge
{
private:
	TraversalEventContainer * const parent_traversal_event_vector;
	//for now, the iterators may be type erased reverse iterators so will naturally run the right way
	TraversalEventIterator next, end;
	polarity direction; 
	float remaining_length;
	bool valid;
	bool has_endpoint_left_to_emit;
	Point end_point_storage; //somewhere to put endpoints that don't exist already

	//the real logic for cutting links resides here:
	TraversalEvent next_event();
	TraversalEvent next_event_inner();
	bool has_more_events() {return next!=end || has_endpoint_left_to_emit;}
	void printinternal();
	#ifdef _SDNADEBUG
		static bool debug;
	#else
		const static bool debug = false;
	#endif

public:
	PartialEdge(TraversalEventIterator from, TraversalEventIterator to, float partial_length, 
		TraversalEventContainer * const parent_traversal_event_vector, polarity direction);

	//PartialEdges are very disposable.  
	//You can only do one of these things with them, once, and then they get invalidated:
	operator TraversalEventContainer();
	TraversalEventAccumulator full_cost();
	void add_points_to_geometry(BoostLineString3d &geom);
	void print() { PartialEdge copy(*this); copy.printinternal();}

	#ifdef _SDNADEBUG
		static void debug_on() {debug=true;}
		static void debug_off() {debug=false;}
	#else
		static void debug_on() {}
		static void debug_off() {}
	#endif
};

//point_index_t, for indexing points in edges,
//is a signed version of size_t
#if defined(_X86_)
	typedef long point_index_t;
#elif defined(_AMD64_)
	typedef long long point_index_t;
#else
#error "Unrecognised architecture"
#endif

struct Edge : public IndexedObject<EdgeId> // the algorithm edge structure
{	
	friend class TestHooks;

	//unique and numbered from zero by Net
	EdgeId id; 
	virtual inline EdgeId get_id() const {return id;}

	// Edges are one-directional components of a SDNAPolyline
	SDNAPolyline *link;
	polarity direction;
	OutgoingConnectionVector outgoing_connections;
	Junction* end_junction;

	//evaluates TraversalEventAccumulator in the context of this edge
	float evaluate_me(MetricEvaluator* e,TraversalEventAccumulator& acc) const;

	//inner methods
	TraversalEventAccumulator get_start_traversal_cost_ignoring_oneway() const;
	TraversalEventAccumulator get_end_traversal_cost_ignoring_oneway() const;
	TraversalEventAccumulator full_cost_ignoring_oneway() const;
	TraversalEventAccumulator partial_cost_from_start(float partial_length) const;
	TraversalEventAccumulator partial_cost_from_middle_ignoring_oneway(float partial_length) const;
	TraversalEventAccumulator csccl(float partial_length) const;
	TraversalEventAccumulator cscclfe(float partial_length) const;
	TraversalEventAccumulator csccl_internal(float partial_length) const;
	
	void add_partial_points_to_geometry(BoostLineString3d &geom,float partial_length) const;
	void add_partial_points_from_middle_to_geometry_ignoring_oneway(BoostLineString3d &geom,float partial_length) const;
	Point get_centre(double length_of_edge_inside_radius) const;

	Point &get_point(point_index_t index) const;
	float get_start_gs() const;
	float get_end_gs() const;
	Edge* get_twin() const;
	void add_copy_of_points_from_end_to_vector(junction_option_type end,vector<Point> &v) const;

	Edge(SDNAPolyline *s, polarity d);
	
	void addOutgoingConnection(Edge *destination);
	void clearOutgoingConnections();
	void assign_turn_costs();
	const OutgoingConnectionVector& outgoing_connections_ignoring_oneway() {return outgoing_connections;}

	bool traversal_allowed() const; 

	junction_option_type link_end_of_edge(junction_option_type edge_end);

	void print() const;

	void assign_id(size_t id_in)
	{
		id = EdgeId(id_in);
	}

private:
	TraversalEventIterator Edge::traversal_events_begin() const;
	TraversalEventIterator Edge::traversal_events_end() const;
	TraversalEventIterator Edge::traversal_events_centre() const;

	void get_outgoing_connections(CandidateEdgeVector &options,double cost_to_date,double remaining_radius,
		MetricEvaluator* anal_evaluator,MetricEvaluator* radial_evaluator,edge_position from,
		JunctionCosts *jcosts);
	friend class PartialNet;
};

class Net;

struct SDNAPolyline : public IndexedObject<SDNAPolylineId> 
{
	//passed in from GIS
	long arcid;
	
	PointVector points; 
private:
	vector<float> attached_data;
	vector<string> attached_text_data;
public:
	//unique id
	SDNAPolylineId id;
	virtual inline SDNAPolylineId get_id() const {return id;}

	Net *net;

	//worked out by sDNA
	//So you thought a link was a vector of geometric points?  
	//Nope. They're a collection of TraversalEvents.
	CachedTraversalEventContainer traversal_events;
	
	Edge forward_edge, backward_edge;

	SDNAPolyline(long a,vector<Point> &pointlist,Net *net);
	
	int get_vert_oneway_data();

	TraversalEventAccumulator full_link_cost_ignoring_oneway(polarity direction) 
	{
		return traversal_events.full_cost_ignoring_oneway(direction);
	}
	
	bool is_not_loop()
	{
		return !(points[0]==points[points.size()-1] && 
			get_start_gs()==get_end_gs());
	}
	
	Point& get_centre()
	{
		return traversal_events.get_centre();
	}

	float get_start_gs();
	float get_end_gs();
	float get_start_z() {return points[0].z;}
	float get_end_z() {return points.back().z;}

	double estimate_average_inner_crow_flight_distance_ignoring_oneway(double proportion_within_radius);
	double estimate_average_inner_diversion_ratio_ignoring_oneway();

	bool is_too_short_for_valid_turn_angles();

	void print();

	Point& get_point(junction_option_type end)
	{
		if (end==EDGE_START)
			return forward_edge.get_point(0);
		else
		{
			assert(end==EDGE_END);
			return backward_edge.get_point(0);
		}
	}

	//does not return costs - intended for finding subsystems not navigation
	vector<SDNAPolyline*> SDNAPolyline::get_linked_links();

	void assign_id(size_t id_in)
	{
		id = SDNAPolylineId(id_in);
		forward_edge.assign_id(id_in*2);
		backward_edge.assign_id(id_in*2+1);
	}
	void append_attached_data(float data) { attached_data.push_back(data); }
	void append_attached_data(string data) { attached_text_data.push_back(data); }
	vector<float> const& get_all_data(float) const {return attached_data;}
	vector<string> const& get_all_data(string) const {return attached_text_data;}
	void set_attached_data(long index,float data) {attached_data[index]=data;}
	void set_attached_data(long index,string data) {attached_text_data[index]=data;}

private:
	void create_traversal_events();
	
};


struct OutgoingConnection //other edge plus turn angle 
{
	Edge *edge;
	float turn_angle;
	OutgoingConnection (Edge *e,float angular_turn_cost) : edge(e), turn_angle(angular_turn_cost) {}
	float get_turn_cost(MetricEvaluator *e,const Edge * const prev) const { return e->evaluate_junction(turn_angle,prev,edge); }
};

//CandidateEdge:  an edge with associated cost and backlink, for consideration by routing alg
struct CandidateEdge
{
	Edge * edge;
	Edge * backlink;
	double cost;

	CandidateEdge( Edge * const e , double cost , Edge * const backlink ) 
		: edge(e), cost(cost), backlink(backlink) {}

	void print() const
	{
		cout << "e" << edge->get_id().id << "b" << backlink->get_id().id << "c" << cost << " ";
	}
};
