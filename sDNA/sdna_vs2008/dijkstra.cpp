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
#include "calculation.h"

// Here be Dragons!
// (be careful modifying this file - if the dijkstra queue comparison goes wrong,
//  you'll be lucky if you only get a segfault)

//Definition of DijkstraQueue:
//it's a set with a lexical comparison operator based first on cost then pointers
//this allows for 
// 1. efficient find and deque of minimum element
// 2. efficient find and reprioritization of arbitrary edges

//The queue runs faster if edges are added to it in order.  Note that when adding,
//all costs are equal, so ordering is determined by pointer address.  The memory
//pool called link_pool in net.h ensures that edges are created with increasing
//addresses, mostly.

class EdgePointerCompare 
{
	private:
		IdIndexedArray<double,EdgeId> *best_costs;
	public:
		EdgePointerCompare(IdIndexedArray<double,EdgeId> *best_costs) : best_costs(best_costs) {}
		bool operator()(const Edge *x,const Edge *y) const //x<y
		{
			if ((*best_costs)[*x] != (*best_costs)[*y])
				return ((*best_costs)[*x] < (*best_costs)[*y]);
			else
				// still necessary to distinguish x and y for multiset comparator
				// so compare pointers
				return x<y;
		}
};

class DijkstraQueue : public set<Edge*,EdgePointerCompare> 
{
private:
	IdIndexedArray<double,EdgeId> *costs;
public:
	void reprioritize(Edge * const edge,double newcost)
	{
		erase(find(edge));
		(*costs)[*edge] = newcost;
		insert(edge);
	}
	DijkstraQueue(IdIndexedArray<double,EdgeId> *best_costs)
		: costs(best_costs), set<Edge*,EdgePointerCompare>(EdgePointerCompare(best_costs)) {}
};

// the main function
// run_dijkstra ALWAYS calculates cost to edge STARTS - to get middles clients must add on the start_traversal_cost
// (and compare the two edges on the destination link to see which is better, if necessary)

// dijkstra always resets costs_to_edge_start and backlinks even for edges that aren't in the PartialNet

void run_dijkstra(PartialNet &partialnet, 
	IdIndexedArray<double,EdgeId> &costs_to_edge_start, MetricEvaluator* evaluator, 
	double max_depth, IdIndexedArray <Edge *,EdgeId> *backlinks, JunctionCosts *jcosts)
{
	// note that in this alg, edge costs/backlinks are stored not within the edges,
	// but within the arrays 'costs' and 'backlinks' which are indexed by edges
	// this allows better paralellization though means we have to pass costs to dijkstraqueue
	costs_to_edge_start.set_all_to(UNCONNECTED_RADIUS);
	if (backlinks)
		backlinks->set_all_to(NULL);
	if (jcosts)
		jcosts->set_all_to(UNCONNECTED_RADIUS);

	// make priority queue which compares edges based on current costs
	// and add only the edges within radius to it 
	// (note that costs/backlinks OUTSIDE radius are still correctly initialized - the data are used elsewhere,
	//  they don't have to be, but there's nothing wrong with that for now & it doesn't cost much)
	DijkstraQueue pq(&costs_to_edge_start);
	DijkstraQueue::iterator last_insert = pq.begin(); //insert position hint to speed up queue build
	for (PartialNet::RoutingEdgeIter it = partialnet.get_routing_edges_begin(); it!=partialnet.get_routing_edges_end(); ++it)
	{
		#if __cplusplus > 199711L
			#error priority queue behaviour has changed - sort out the next line for it+1
		#endif
		last_insert = pq.insert(last_insert,*it);
	}

	// set origin costs to zero
	pq.reprioritize(&partialnet.origin->forward_edge,0);
	pq.reprioritize(&partialnet.origin->backward_edge,0);

	CandidateEdgeVector options;
	CandidateEdgeVector::iterator it;
	options.reserve(10);
	while (!pq.empty())
	{
		//pop
		DijkstraQueue::iterator top = pq.begin();
		Edge *current_edge = *top;
		double current_edge_best_cost = costs_to_edge_start[*current_edge];
		if (current_edge_best_cost > max_depth)
			// then all remaining unexplored edges are unconnected to origin, or beyond max_depth
			break;
		pq.erase(top);

		//compute costs of candidate connections from this edge
		if (current_edge->link!=partialnet.origin)
			partialnet.get_outgoing_connections(options,current_edge,current_edge_best_cost,evaluator,START,jcosts); 
		else
			partialnet.get_outgoing_connections(options,current_edge,current_edge_best_cost,evaluator,MIDDLE,jcosts);
		
		//check whether each candidate edge is better than the existing best; if so, reprioritize
		for (it = options.begin(); it != options.end(); it++)
		{
			if (it->cost < costs_to_edge_start[*it->edge])
			{
				pq.reprioritize(it->edge,it->cost);
				if (backlinks != NULL)
					(*backlinks)[*it->edge] = it->backlink;
			}
		}
	}
}
