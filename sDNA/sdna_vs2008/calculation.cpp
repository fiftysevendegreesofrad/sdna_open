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
#include "prepareoperations.h"

double SDNAIntegralCalculation::getDistanceToJunction(Edge *e)
{
	double dist = e->get_end_traversal_cost_ignoring_oneway().euclidean;
	if (!e->link->is_not_loop())
		return dist; //loop links don't connect to self, so would otherwise follow only outgoing connection to next link
	set<SDNAPolyline*> visited;
	while (e->outgoing_connections_ignoring_oneway().size()==1)
	{
		e = e->outgoing_connections_ignoring_oneway()[0].edge;
		SDNAPolyline * const l = e->link;
		if (visited.find(l)!=visited.end())
			break;
		visited.insert(l);
		dist += e->full_cost_ignoring_oneway().euclidean;
	}
	return dist;
}

double SDNAIntegralCalculation::calculateLinkFraction(SDNAPolyline *s)
{
	const double total_length = getDistanceToJunction(&(s->forward_edge)) + getDistanceToJunction(&(s->backward_edge));
	return s->full_link_cost_ignoring_oneway(PLUS).euclidean / total_length;
}

bool SDNAIntegralCalculation::check_no_zero_length_edges()
{
	vector<long> zero_length_ids;
	for (SDNAPolylineContainer::iterator s = net->link_container.begin(); s != net->link_container.end(); s++)
	{
		SDNAPolyline * const link = &*s->second;
		if (link->full_link_cost_ignoring_oneway(PLUS).euclidean==0)
			zero_length_ids.push_back(link->arcid);
	}
	if (zero_length_ids.size()>0)
	{
		for (vector<long>::iterator it=zero_length_ids.begin(); it!=zero_length_ids.end(); ++it)
		{
			stringstream s;
			s << "WARNING: Polyline " << *it << " has near zero length";
			//what this means in practice is a length of zero in 32-bit float precision as that's how traversal events are stored
			//contrast "too short for valid turn angles" which means a square length of zero in 64-bit double precision 
			print_warning_callback(s.str().c_str());
			if (it-zero_length_ids.begin()>4)
				break;
		}
		if (zero_length_ids.size()>5)
		{
			stringstream s;
			s << "WARNING: " << zero_length_ids.size()-5 << " more links have zero length";
			print_warning_callback(s.str().c_str());
		}
		print_warning_callback("WARNING: Zero length links can lead to unpredictable results;");
		print_warning_callback("WARNING: recommended practice is to remove them using sDNA Prepare.");
		return false;
	}
	else
		return true;
}

bool SDNAIntegralCalculation::check_no_oneway_conflicts()
{
	vector<long> conflict_ids, ambiguous_ids;
	for (SDNAPolylineContainer::iterator s = net->link_container.begin(); s != net->link_container.end(); s++)
	{
		SDNAPolyline * const link = &*s->second;
		const int link_onewaydata = boost::math::sign(onewaydata.get_data(link));
		const int link_vertonewaydata = link->get_vert_oneway_data();
		if (link_vertonewaydata!=0 && (link->get_start_z()==link->get_end_z()))
			ambiguous_ids.push_back(link->arcid);
		if ((link_onewaydata>0 && link_vertonewaydata<0) || (link_onewaydata<0 && link_vertonewaydata>0))
			conflict_ids.push_back(link->arcid);
	}
	bool success = true;
	if (conflict_ids.size())
	{
		for (vector<long>::iterator it=conflict_ids.begin(); it!=conflict_ids.end(); ++it)
		{
			stringstream s;
			s << "ERROR: Polyline " << *it << " has conflicting oneway and vertoneway data";
			print_warning_callback(s.str().c_str());
			if (it-conflict_ids.begin()>4)
				break;
		}
		if (conflict_ids.size()>5)
		{
			stringstream s;
			s << "ERROR: " << conflict_ids.size()-5 << " more lines have conflicting oneway and vertoneway data";
			print_warning_callback(s.str().c_str());
		}
		success = false;
	}
	if (ambiguous_ids.size())
	{
		for (vector<long>::iterator it=ambiguous_ids.begin(); it!=ambiguous_ids.end(); ++it)
		{
			stringstream s;
			s << "ERROR: Polyline " << *it << " starts and ends at the same level, so vertoneway data is ambiguous";
			print_warning_callback(s.str().c_str());
			if (it-ambiguous_ids.begin()>4)
				break;
		}
		if (ambiguous_ids.size()>5)
		{
			stringstream s;
			s << "ERROR: " << ambiguous_ids.size()-5 << " more lines start have ambiguous vertoneway data";
			print_warning_callback(s.str().c_str());
		}
		success = false;
	}
	return success;
}


bool SDNAIntegralCalculation::run_internal()
{
	try
	{
		return run_internal_throwing_exceptions();
	}
	catch (SDNARuntimeException e)
	{
		stringstream error;
		error << "ERROR: " << e.what();
		print_warning_callback(error.str().c_str());
		return false;
	}
}

void SDNAIntegralCalculation::initialize_zones()
{
	BOOST_FOREACH(shared_ptr<Table<float>> zdt, zonedatatables)
	{
		zdt->finalize();
		zdt->initializeItems(net->num_items());
		for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
		{
			SDNAPolyline * const p = it->second;
			Edge * const e = &p->forward_edge;
			zdt->register_polyline(p);
		}
	}

	if (using_od_matrix())
	{
		od_matrix->finalize();
		od_matrix -> initializeItems(net->num_items());

		for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
		{
			SDNAPolyline * const p = it->second;
			Edge * const e = &p->forward_edge;
			const double zone_distribution_metric_this_link = zonedist_evaluator_factory->evaluate_edge(e->full_cost_ignoring_oneway(),e);
			od_matrix->register_polyline(p,zone_distribution_metric_this_link);
		}
	}

	BOOST_FOREACH(ZoneSum& zd, zone_sum_evaluators)
	{
		shared_ptr<Table<long double> > zone_sum_table = zd.table;
		zone_sum_table->setNameDas(zd.zone_das);

		for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
		{
			SDNAPolyline * const p = it->second;
			Edge * const e = &p->forward_edge;
			const double zone_sum_this_link = zd.eval->evaluate_edge(e->full_cost_ignoring_oneway(),e);
			const string zone = zd.zone_das->get_data(p);
			zone_sum_table->incrementrow(zone,zone_sum_this_link);
		}
		zone_sum_table->finalize();
		zone_sum_table->initializeItems(net->num_items());
		for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
		{
			SDNAPolyline * const p = it->second;
			zone_sum_table->register_polyline(p);
		}
	}
}

void SDNAIntegralCalculation::initialize_skim_zones()
{
	skim_origzoneindex.initialize(net->num_items());
	skim_destzoneindex.initialize(net->num_items());
	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
	{
		SDNAPolyline * p = it->second;
		skim_origzoneindex[*p] = skim_origins.add(skim_origzone_das.get_data(p));
		skim_destzoneindex[*p] = skim_destinations.add(skim_destzone_das.get_data(p));
	}
}


bool SDNAIntegralCalculation::run_internal_throwing_exceptions()
{
	if (!bind_network_data()) return false; //this can happen, and stops execution (error message will be printed from deep within init_data)

	if (using_od_matrix() && !od_matrix)
	{
		print_warning_callback("No 2d matrix supplied for OD matrix");
		return false;
	}
	
	net->ensure_junctions_created();
	
	//remove disabled polylines
	vector<SDNAPolyline*> lines_to_remove;
	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
	{
		SDNAPolyline * const line = it->second;
		const float disable_result = line_disable_evaluator->evaluate_edge_no_geometry(&line->forward_edge);
		if (disable_result)
			lines_to_remove.push_back(line);
	}
	if (lines_to_remove.size())
	{
		stringstream ss;
		ss << lines_to_remove.size() << " polylines were disabled";
		print_warning_callback(ss.str().c_str());
		BOOST_FOREACH(SDNAPolyline* line,lines_to_remove)
			net->remove_link(line);
	}

	//sort edge_ptr_container by memory address of edges
	//this is to ensure speed of heap adds in dijkstra
	// - an uncomfortable coupling I admit
	print_warning_callback("Building network and checking for tolerance errors...");
	sort(net->edge_ptr_container.begin(),net->edge_ptr_container.end());

	net->assign_link_edge_ids();

	net->set_oneway_data(&onewaydata,&vertonewaydata);
	if (!check_no_oneway_conflicts()) return false;

	assert(xytol != -1);
	if (net->get_near_miss_clusters(xytol,ztol).size() > 0)
	{
		print_warning_callback("ERROR: Edges closer than Arc's XYTolerance/Cluster Tolerance detected\nthese are not necessarily viewable in ArcMap\nand will lead to incorrect results\nPlease inspect and fix using sDNA Prepare");
		return false;
	}

	net->link_edges();
	net->warn_about_invalid_turn_angles(print_warning_callback);
	net->assign_junction_ids();
	
	int num_radii = (int)radii.size();

	net->add_polyline_centres(link_centre_type);
	check_no_zero_length_edges();

	link_fraction.initialize(net->num_items());
	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
	{
		SDNAPolyline * const link = it->second;
		link_fraction[*link] = numeric_cast<float>(calculateLinkFraction(link));
	}

	if (using_intermediate_link_filter)
	{
		intermediate_filter.initialize(net->num_items());
		for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
		{
			SDNAPolyline * const link = it->second;
			intermediate_filter[*link] = (intermediate_filter_evaluator->evaluate_edge_no_geometry(&link->forward_edge) != 0 );
		}
	}

	initialize_zones(); 

	origweightdata.initialize(net->num_items());
	destweightdata.initialize(net->num_items());

	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
	{
		SDNAPolyline * const p = it->second;
		Edge * const e = &p->forward_edge;
		origweightdata[*p] = origweightexpr? origweightexpr->evaluate_edge(e->full_cost_ignoring_oneway(),e) : origweightsource.get_data(p);
		destweightdata[*p] = destweightexpr? destweightexpr->evaluate_edge(e->full_cost_ignoring_oneway(),e) : destweightsource.get_data(p);
	}

	if (!run_closeness)
		return true;

	print_warning_callback("Running sDNA Integral calculation...");

	if (output_skim)
	{
		initialize_skim_zones();
		skim_matrix_sum_distance.swap(vector<vector<long double>>(skim_origins.size(),vector<long double>(skim_destinations.size(),0.)));
		skim_matrix_weight.swap(vector<vector<long double>>(skim_origins.size(),vector<long double>(skim_destinations.size(),0.)));
		skim_matrix_n.swap(vector<vector<unsigned long>>(skim_origins.size(),vector<unsigned long>(skim_destinations.size(),0l)));
	}

	//initialize sdna integral arrays (computed once, by this calculation)
	closeness.initialize(net->num_items(),num_radii,0.);
	accessibility.initialize(net->num_items(),num_radii,acc_dist_constant_factor,nqpdn,nqpdd);
	betweenness.initialize(net->num_items(),num_radii);
	two_stage_betweenness.initialize(net->num_items(),num_radii);
	total_angular_cost.initialize(net->num_items(),num_radii,0.);
	total_link_length.initialize(net->num_items(),num_radii,0.);
	convex_hull_area.initialize(net->num_items(),num_radii,0.);
	convex_hull_perimeter.initialize(net->num_items(),num_radii,0.);
	convex_hull_max_radius.initialize(net->num_items(),num_radii,0.);
	convex_hull_bearing.initialize(net->num_items(),num_radii,0.);
	total_num_links.initialize(net->num_items(),num_radii,0);
	total_num_junctions.initialize(net->num_items(),num_radii,0);
	total_connectivity.initialize(net->num_items(),num_radii,0);
	total_geodesic_length_weighted.initialize(net->num_items(),num_radii,0);
	total_crow_flies.initialize(net->num_items(),num_radii,0);
	diversion_ratio.initialize(net->num_items(),num_radii,0);
	total_weight.initialize(net->num_items(),num_radii,0);
	two_stage_dest_popularity.initialize(net->num_items(),num_radii);
	prob_route_weight.initialize(net->num_items(),num_radii,0.);
	prob_route_excess.initialize(net->num_items(),num_radii,0.);

	{
		//count weighted links
		size_t num_weighted_origins = 0;
		size_t num_unskippable_origins = 0;
		for (SDNAPolylineContainer::iterator it=net->link_container.begin(); it!=net->link_container.end(); it++)
			if (skiporiginifzerodata.get_data(&*it->second) != 0)
			{
				num_unskippable_origins++;
				if (origweightdata.get_data(*it->second) != 0) num_weighted_origins++;
			}
		
		//the following are approximations, assuming that all links in skiporiginsexcept and skipdestinationsexcept have nonzero weight
		const size_t num_origins = (skiporiginsexcept.size()==0) ? (skip_zero_weight_origins ? num_weighted_origins : num_unskippable_origins)
			: (skiporiginsexcept.size());
		
		if (output_hulls) hulls.reserve(num_origins);
		if (output_netradii) netradii.reserve(num_origins);
		//we can't tell how large the following will need to be - it depends on the radius - just reserve a good chunk to start
		if (output_geodesics) geodesics.reserve(num_origins);
		if (output_destinations) destinationgeoms.reserve(num_origins);
	}

	size_t predicted_max_geodesic_steps = numeric_cast<size_t>(pow(numeric_cast<double>(net->num_items()),0.5));

	ThreadSafeProgressBar thread_safe_progress_bar(net->num_items(),set_progressor_callback);

	//unpack link_container into vector for openmp loop (which can't use iterators)
	vector<SDNAPolyline*> links;
	links.reserve(net->link_container.size());
	for (SDNAPolylineContainer::iterator it = net->link_container.begin(); it != net->link_container.end(); it++)
		links.push_back(&*it->second);
	
	//copy evaluator "factory" classes from calculation class fields to stack
	//so we can provide a different instance to each thread in omp parallel for
	//(so while this isn't strictly a factory pattern, we are using copy constructor as a factory method)
	MetricEvaluatorCopyableWrapper analysis_evaluator_copyable(analysis_evaluator_factory);
	
	bool terminate_early = false;
	shared_ptr<SDNARuntimeException> inner_loop_exception;

	//omp parallel loop.  different threads should stay clear of one another, because they only update
	//arrays for their own origin link, which is different in each case
	//the exceptions to this are:
	//  betweenness 
	//  progress bar (num_links_computed)
	#pragma omp parallel 
	{
		int priority = GetThreadPriority(GetCurrentThread());
		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_BEGIN);
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);

		#pragma omp for schedule (guided) firstprivate (analysis_evaluator_copyable)
		for (long origin_link_index = skip_mod; origin_link_index < numeric_cast<long>(links.size()); origin_link_index+=skip_fraction)
		{
			//mechanism to stop exceptions breaking through parallel for, which isn't allowed by Open MP
			if (!terminate_early)
			{
				try
				{
					scoped_ptr<MetricEvaluator>& analysis_evaluator = analysis_evaluator_copyable.get();
					scoped_ptr<MetricEvaluator> eme(new EuclideanMetricEvaluator());
					sdna_inner_loop(origin_link_index,links,thread_safe_progress_bar,&*eme,&*analysis_evaluator,predicted_max_geodesic_steps);
				}
				catch (SDNARuntimeException &e)
				{
					#pragma omp critical //only the first error to be thrown gets saved
					{
						if (!terminate_early)
						{
							inner_loop_exception.reset(new SDNARuntimeException(e));
							terminate_early = true;
						}
					}
				}
			}
		}
		SetThreadPriority(GetCurrentThread(), THREAD_MODE_BACKGROUND_END);
		SetThreadPriority(GetCurrentThread(), priority);
	}
	if (terminate_early)
		throw SDNARuntimeException(*inner_loop_exception);

	if (output_skim)
	{
		print_warning_callback("Finalizing skim matrix");
		for (size_t skim_orig=0;skim_orig<skim_origins.size();skim_orig++)
			for (size_t skim_dest=0;skim_dest<skim_destinations.size();skim_dest++)
			{
				const long double sumdist = skim_matrix_sum_distance[skim_orig][skim_dest];
				const long double weight = skim_matrix_weight[skim_orig][skim_dest];
				const size_t N = skim_matrix_n[skim_orig][skim_dest];
				shared_ptr<sDNADataMultiGeometry> skimdata(
					new sDNADataMultiGeometry(
						NO_GEOM,
						get_skim_data(skim_origins.index_to_zone(skim_orig),
									  skim_destinations.index_to_zone(skim_dest),
									  (float)(sumdist/weight),(float)weight,numeric_cast<long>(N)),
						0));
				skim_geom.add(skimdata);
			}
	}

	return true;
}

void SDNAIntegralCalculation::sdna_inner_loop(long origin_link_index,
											  vector<SDNAPolyline*> &links,
											  ThreadSafeProgressBar &thread_safe_progress_bar,
											  MetricEvaluator* radial_evaluator,
											  MetricEvaluator* analysis_evaluator,
											  size_t predicted_max_geodesic_steps)
{
	SDNAPolyline *origin = links[origin_link_index];
	
	thread_safe_progress_bar.increment_bar(skip_fraction);

	for (int sample=0;sample<oversample;sample++)
	{

		if (skiporiginsexcept.size()!=0 && skiporiginsexcept.find(origin->arcid)==skiporiginsexcept.end()
			|| skip_zero_weight_origins && origweightdata.get_data(*origin)==0
			|| skiporiginifzerodata.get_data(origin)==0)
			return;

		//cut out networks
		PartialNetCollection cut_networks(net,origin,radii,lower_bound_radii,strict_network_cut,radial_evaluator,cont_space,analysis_evaluator,prob_route_action,handle_prob_routes_if_over);
		
		//used in backtrace; allocated here for speed
		vector<Edge*> intermediate_edges;
		intermediate_edges.reserve(predicted_max_geodesic_steps);
		
		for (int r=0;r<(int)radii.size();r++)
		{
			PartialNet cut_net = cut_networks.getPartialNet(r);
			IdIndexedArray<double,EdgeId> &anal_best_costs_reaching_edge = cut_net.get_anal_best_costs_reaching_edge_array();
			IdIndexedArray<Edge *,EdgeId> &anal_backlinks_edge = cut_net.get_anal_backlinks_edge_array();
				
			if (run_junctions)
				cut_net.count_junctions_accumulate(total_num_junctions(origin,r),total_connectivity(origin,r));
			
			//print arrays
			#ifdef _SDNADEBUG
				std::cout << "R" << radii[r] << " ";
				std::cout << "origin " << origin->arcid << "   (linkid " << origin->id.id << ")" << endl;
				std::cout << "     analytical costs per edge: ";
				anal_best_costs_reaching_edge.print();
				std::cout << "     analytical backlinks per edge: ";
				anal_backlinks_edge.print();
			#endif

			//initialize destination containers
			vector<DestinationSDNAPolylineSegment> destinationsegments; //holds spec of geometric bits of network we're going to visit
			cut_net.get_destination_processing_tasks(destinationsegments);
			vector<DestinationEdgeProcessingTask> destinations; //will hold the bits of network PLUS info on which way we will arrive
			destinations.reserve(destinationsegments.size());
			
			//initialize geometry store for convex hull and netradius
			shared_ptr<sDNADataMultiGeometry> all_edge_segments_in_radius = shared_ptr<sDNADataMultiGeometry>(
				new sDNADataMultiGeometry(MULTIPOLYLINEZ,get_hull_or_netradius_data(origin->arcid,radii[r],origweightdata.get_data(*origin)),destinationsegments.size()));

			//must be stored here as needed by process_origin and process_destination;
			//and total_weight accumulates previous samples during oversampling
			double total_weight_this_origin_sample_radius = 0;

			//process destinations
			for (vector<DestinationSDNAPolylineSegment>::iterator it = destinationsegments.begin(); it != destinationsegments.end(); ++it)
				destinations.push_back(it->getDestinationEdgeProcessingTaskAnalytical(anal_best_costs_reaching_edge,analysis_evaluator,strict_network_cut));

			for (vector<DestinationEdgeProcessingTask>::iterator it = destinations.begin(); it != destinations.end(); ++it)
				process_destination(*it,origin,r,anal_best_costs_reaching_edge,all_edge_segments_in_radius,total_weight_this_origin_sample_radius);

			//at this point, closeness, link length, total weight etc are correct except for origin's contribution
			process_origin(origin,r,all_edge_segments_in_radius,analysis_evaluator,total_weight_this_origin_sample_radius);
			
			finalize_radius_geometry(origin,r,all_edge_segments_in_radius);
			total_weight(origin,r) += total_weight_this_origin_sample_radius;

			//process geodesics - must happen down here as origin's total weight in radius needs to be correct
			if (run_betweenness)
			{
				for (vector<DestinationEdgeProcessingTask>::iterator it = destinations.begin(); it != destinations.end(); ++it)
					process_geodesic(*it,cut_net,r,intermediate_edges,anal_best_costs_reaching_edge,analysis_evaluator,total_weight_this_origin_sample_radius);			
			}

		} // end radial loop
	} // end oversample loop
}

void SDNAIntegralCalculation::process_origin(SDNAPolyline *origin,int r,shared_ptr<sDNADataMultiGeometry> &all_edge_segments_in_radius,MetricEvaluator *analysis_evaluator,double& total_weight_this_origin_sample_radius)
{
	//the origin link is handled separately, because it is traversed starting in the middle, not at one end

	//Two interesting questions: 
	//(A) should origin self-closeness/betweenness respect one way systems.
	//(B) if so should oneway origins start from the end not middle
	//To (B) the answer is no.  We assume centres are an approximation this is implicit in sDNA.  Once the whole link is included
	//it makes most sense to measure from the centre like any other link, as we want behaviour on a macro scale to be accurate.
	//Working back to (A) the answer is also no.  sDNA does not model accessing the origin link via other links.  so we have
	//to assume it's possible to violate the one way system for accessing points on the same link (again to give
	//accurate measures on a macroscopic scale).  Also without doing this there
	//will be a discontinuity when the radius reaches half link length and we jump from half- to whole-link inclusion.
	//All this is is in keeping with the other micromodelling situations where sDNA breaks down (see issue tracker).

	//downcasting here is ok but wouldn't be for routing which requires that double_max and double_infinity are different
	const float origin_effective_radius = origin_cont_space?static_cast<float>(radii[r]):numeric_limits<float>::infinity();
			
	float length_of_origin_within_radius, cost_of_origin_within_radius;
	shared_ptr<sDNAGeometryPointsByEdgeLength> origin_geom(new sDNAGeometryPointsByEdgeLength());
	
	const TraversalEventAccumulator origin_fwd_cost = origin->forward_edge.partial_cost_from_middle_ignoring_oneway(origin_effective_radius);
	const TraversalEventAccumulator origin_bwd_cost = origin->backward_edge.partial_cost_from_middle_ignoring_oneway(origin_effective_radius);
	
	length_of_origin_within_radius = origin_fwd_cost.euclidean + origin_bwd_cost.euclidean;
	cost_of_origin_within_radius = analysis_evaluator->evaluate_edge(origin_fwd_cost,&origin->forward_edge)
		+ analysis_evaluator->evaluate_edge(origin_bwd_cost,&origin->backward_edge);
	total_angular_cost(origin,r) += origin_fwd_cost.angular	+ origin_bwd_cost.angular;
	#ifdef _SDNADEBUG
		if (origin_cont_space)
			cout << "adding origin points to hull for link " << origin->arcid << endl;
	#endif
	origin_geom->add_edge_radius_surrounding_middle_ignoring_oneway(&origin->forward_edge,origin_effective_radius);
	all_edge_segments_in_radius->add_part(origin_geom);

	double origin_full_length = origin->full_link_cost_ignoring_oneway(PLUS).euclidean;
	double proportion_of_origin_within_radius = (origin_full_length!=0) ? length_of_origin_within_radius/origin_full_length : 1;
	double origweight_of_origin_within_radius, destweight_of_origin_within_radius, origin_self_betweenness_weight;
	if (using_od_matrix())
	{
		origin_self_betweenness_weight = proportion_of_origin_within_radius * od_matrix->getData(origin,origin);
		origweight_of_origin_within_radius = origin_self_betweenness_weight;
		destweight_of_origin_within_radius = origin_self_betweenness_weight;
	}
	else
	{
		origweight_of_origin_within_radius = proportion_of_origin_within_radius * origweightdata.get_data(*origin);
		destweight_of_origin_within_radius = proportion_of_origin_within_radius * destweightdata.get_data(*origin);
		origin_self_betweenness_weight = origweight_of_origin_within_radius * destweight_of_origin_within_radius;
	}

	total_link_length(origin,r) += length_of_origin_within_radius;
	total_num_links(origin,r) += proportion_of_origin_within_radius;
	total_weight_this_origin_sample_radius += destweight_of_origin_within_radius;

	//each point on origin is on average 1/3 the distance of the origin from each other point on origin
	const double origin_self_closeness = cost_of_origin_within_radius / 3.;
	closeness(origin,r) += destweight_of_origin_within_radius * origin_self_closeness;
	accessibility.increment(origin,r,destweight_of_origin_within_radius,origin_self_closeness);
	
	if (output_skim)
	{
		long double& smsd = skim_matrix_sum_distance[skim_origzoneindex[*origin]][skim_destzoneindex[*origin]];
		#pragma omp atomic
			smsd += origin_self_closeness;
		long double& smw = skim_matrix_weight[skim_origzoneindex[*origin]][skim_destzoneindex[*origin]];
		#pragma omp atomic
			smw += origin_self_betweenness_weight;
		unsigned long& smn = skim_matrix_n[skim_origzoneindex[*origin]][skim_destzoneindex[*origin]];
		if ( smn < numeric_limits<unsigned long>::max())
		{
			#pragma omp atomic
				smn += 1;
		}
	}

	if (run_betweenness)
	{
		total_geodesic_length_weighted(origin,r) += destweight_of_origin_within_radius * length_of_origin_within_radius / 3.;
		
		//estimate diversion ratios/crow flight for origin in a similar way
		//this is very approximate, but simplifies the appearance of sdna output measures:
		//if we don't estimate origin diversion ratio then that breaks the concept of normalizing by weight in radius
		//as the weight does include the origin itself
		total_crow_flies(origin,r) += origin->estimate_average_inner_crow_flight_distance_ignoring_oneway(proportion_of_origin_within_radius) 
										* destweight_of_origin_within_radius;
		diversion_ratio(origin,r) += origin->estimate_average_inner_diversion_ratio_ignoring_oneway() * destweight_of_origin_within_radius;

		//each point experiences intra-origin traffic of 1/6*(origweight*destweight) in each direction
		betweenness.increment_both(origin,r,origin_self_betweenness_weight/6.);
		
		//...or L*(Lorigin/Lradius)/6 under the two stage model
		//(Lorig*Ldest/Ldestradius in orig/dest framework)
		//nb must happen here as origin's total weight in radius needs to be correct
		const double two_stage_betweenness_origin_weight = 
			(total_weight_this_origin_sample_radius!=0) ? origin_self_betweenness_weight / total_weight_this_origin_sample_radius 
												: 0;
		//assert total_weight can only be 0 if origin itself has zero destweight
		assert(total_weight_this_origin_sample_radius!=0 || destweight_of_origin_within_radius==0);

		two_stage_betweenness.increment_both(origin,r,two_stage_betweenness_origin_weight/6.);
		two_stage_dest_popularity.increment(origin,r,two_stage_betweenness_origin_weight);
		if (output_destinations)
		{
			shared_ptr<sDNADataMultiGeometry> orig_as_destinationdata(
				new sDNADataMultiGeometry(
					POLYLINEZ,
					get_destination_data(origin->arcid,origin->arcid,radii[r],
										 (float)origweight_of_origin_within_radius,
										 (float)two_stage_betweenness_origin_weight,
										 (float)(cost_of_origin_within_radius / 3.),
										 (float)(length_of_origin_within_radius / 3.)),
					 1));
			shared_ptr<sDNAGeometryPointsByEdgeLength> origasdestination(new sDNAGeometryPointsByEdgeLength());
			orig_as_destinationdata->add_part(origin_geom);
			destinationgeoms.add(orig_as_destinationdata);
		}
	}
}

double SDNAIntegralCalculation::get_geodesic_analytical_cost(DestinationEdgeProcessingTask &dest,IdIndexedArray<double  ,EdgeId> &anal_best_costs_reaching_edge)
{
	return anal_best_costs_reaching_edge[*dest.routing_edge] + dest.cost_to_centre;
}

void SDNAIntegralCalculation::process_destination(DestinationEdgeProcessingTask &dest,SDNAPolyline *origin_link, 
											  int r,
											  IdIndexedArray<double  ,EdgeId> &anal_best_costs_reaching_edge,
											  shared_ptr<sDNADataMultiGeometry> &all_edge_segments_in_radius,
											  double& total_weight_this_origin_sample_radius)
{
	const float length_of_edge_inside_radius = dest.length_of_edge_inside_radius;
	
	SDNAPolyline *destination_link = dest.routing_edge->link;
	
	const double radius = radii[r];
	assert(dest.routing_edge->traversal_allowed());
	const double destination_full_length = dest.routing_edge->full_cost_ignoring_oneway().euclidean;
	const double proportion_of_edge_inside_radius = (destination_full_length!=0) ? length_of_edge_inside_radius/destination_full_length : 1;
	
	double dest_weight;
	if (using_od_matrix())
		dest_weight = od_matrix->getData(origin_link,destination_link) * proportion_of_edge_inside_radius;
	else
		dest_weight = destweightdata.get_data(*destination_link) * proportion_of_edge_inside_radius;
	
	//update closeness and count accumulators 
	const double cost_to_centre = get_geodesic_analytical_cost(dest,anal_best_costs_reaching_edge);
					
	closeness(origin_link,r) += cost_to_centre * dest_weight;
	accessibility.increment(origin_link,r,dest_weight,cost_to_centre);
	#ifdef _SDNADEBUG
		if (proportion_of_edge_inside_radius>0)
			cout << "      R" << radius << " cost from link " << origin_link->arcid << "-" << destination_link->arcid << " adds " << cost_to_centre
				<< " to closeness with prop " << proportion_of_edge_inside_radius << " weight " << dest_weight/proportion_of_edge_inside_radius << endl;
	#endif
	total_link_length(origin_link,r) += length_of_edge_inside_radius;
	total_num_links(origin_link,r) += proportion_of_edge_inside_radius;
	total_angular_cost(origin_link,r) += dest.geom_edge->partial_cost_from_start(length_of_edge_inside_radius).angular;
	total_weight_this_origin_sample_radius += dest_weight;
	
	#ifdef _SDNADEBUG
		cout << "     adding points from edge " << dest.geom_edge->id.id << " to hull of link " << origin_link->arcid <<
			" radius " << radius << endl;
	#endif
	
	shared_ptr<sDNAGeometryPointsByEdgeLength> g(new sDNAGeometryPointsByEdgeLength());
	g->add_edge_length_from_start_to_end(dest.geom_edge,length_of_edge_inside_radius);
	all_edge_segments_in_radius->add_part(g);

	if (output_skim)
	{
		const double skim_weight = using_od_matrix() ? dest_weight : dest_weight*origweightdata.get_data(*origin_link);
		long double &smsd = skim_matrix_sum_distance[skim_origzoneindex[*origin_link]][skim_destzoneindex[*destination_link]];
		#pragma omp atomic
			smsd += cost_to_centre * skim_weight;
		long double &smw = skim_matrix_weight[skim_origzoneindex[*origin_link]][skim_destzoneindex[*destination_link]];
		#pragma omp atomic
			smw += skim_weight;
		unsigned long &smn = skim_matrix_n[skim_origzoneindex[*origin_link]][skim_destzoneindex[*destination_link]];
		if ( smn < numeric_limits<unsigned long>::max())
		{
			#pragma omp atomic
				smn++;
		}
	}
}

void SDNAIntegralCalculation::process_geodesic(DestinationEdgeProcessingTask &dest,PartialNet &cut_net, int r,
											  vector<Edge*> &intermediate_edges,
											  IdIndexedArray<double  ,EdgeId> &anal_best_costs_reaching_edge,
											  MetricEvaluator *analysis_evaluator,
											  double& total_weight_this_origin_sample_radius)
{
	SDNAPolyline * const destination_link = dest.routing_edge->link;

	if (skipdestinationsexcept.size()!=0 && skipdestinationsexcept.find(destination_link->arcid)==skipdestinationsexcept.end())
		return;

	//if there is no route to trace back (it's analytically unreachable) ignore geodesic for now
	if (cut_net.get_anal_backlinks_edge_array()[*dest.routing_edge]==NULL)
		return;

	const float length_of_edge_inside_radius = dest.length_of_edge_inside_radius;
	SDNAPolyline * const origin_link = cut_net.origin;
	
	const double radius = radii[r];
	const double destination_full_length = dest.routing_edge->full_cost_ignoring_oneway().euclidean;
	const double proportion_of_edge_inside_radius = (destination_full_length!=0) ? length_of_edge_inside_radius/destination_full_length : 1;
	
	float origin_weight;
	double dest_weight, betweenness_weight;
	if (using_od_matrix())
	{
		betweenness_weight = od_matrix->getData(origin_link,destination_link) * proportion_of_edge_inside_radius;
		dest_weight = betweenness_weight;
		origin_weight = (float)betweenness_weight;
	}
	else
	{
		origin_weight = origweightdata.get_data(*origin_link);
		dest_weight = destweightdata.get_data(*destination_link) * proportion_of_edge_inside_radius;
		betweenness_weight = (double)origin_weight * (double)dest_weight;
	}
	
	if (skip_zero_weight_destinations && dest_weight==0) return; 

	const double two_stage_betweenness_weight = 
		(total_weight_this_origin_sample_radius!=0) ? (double)origin_weight * (double)dest_weight / total_weight_this_origin_sample_radius : 0;
	assert(total_weight_this_origin_sample_radius!=0 || dest_weight==0);
	
	//do the backtrace, get intermediate links, length and origin_exit_edge.  handle as appropriate
	Edge *origin_exit_edge;
	bool include_geodesic;

	double euclidean_cost_of_geodesic = backtrace(dest,origin_link,cut_net.get_anal_backlinks_edge_array(),intermediate_edges,&origin_exit_edge,include_geodesic);
	if (euclidean_cost_of_geodesic/radius > handle_prob_routes_if_over)
	{
		#ifdef _SDNADEBUG
						cout << "Detected problem route to link " << destination_link->arcid << " edge " << dest.routing_edge->get_id().id << " - ";
		#endif
		switch (prob_route_action)
		{
		case DISCARD:
			#ifdef _SDNADEBUG
						cout << "discarded" << endl;
			#endif
			return;
			break;
		case REROUTE:
			#ifdef _SDNADEBUG
						cout << "rerouted" << endl;
			#endif
			euclidean_cost_of_geodesic = backtrace(dest.getRadialEquivalent(cut_net.get_radial_best_costs_reaching_edge_array(),analysis_evaluator),
												   origin_link,cut_net.get_radial_backlinks_edge_array(),intermediate_edges,&origin_exit_edge,include_geodesic);
			break;
		default:
			assert (false);
			break;
		}
	}

	if (using_intermediate_link_filter && !include_geodesic)
		return;

	const double crow_flies_distance = Point::distance(origin_link->get_centre(),dest.geom_edge->get_centre(length_of_edge_inside_radius));
	total_crow_flies(origin_link,r) += crow_flies_distance * dest_weight;
	
	//origin and destination links experience half the traffic, on average, of other points on the route
	betweenness.increment(origin_exit_edge,r,betweenness_weight / 2.);
	betweenness.increment(dest.routing_edge,r,betweenness_weight / 2.);
	two_stage_betweenness.increment(origin_exit_edge,r,two_stage_betweenness_weight / 2.);
	two_stage_betweenness.increment(dest.routing_edge,r,two_stage_betweenness_weight / 2.);
	
	two_stage_dest_popularity.increment(destination_link,r,two_stage_betweenness_weight);

	for (vector<Edge*>::iterator it=intermediate_edges.begin();it!=intermediate_edges.end();it++)
	{
		betweenness.increment(*it,r,betweenness_weight); 
		two_stage_betweenness.increment(*it,r,two_stage_betweenness_weight);
			#ifdef _SDNADEBUG
				if (proportion_of_edge_inside_radius>0)
					cout << "      R" << radii[r] << " cost from link " << origin_link->arcid << "-" << destination_link->arcid << " adds " << 1
						<< " to betweenness of link " << (*it)->link->arcid << "(edge " << (*it)->get_id().id << ") with prop " << proportion_of_edge_inside_radius 
						<< " weight " << (origin_weight*dest_weight/proportion_of_edge_inside_radius) << endl;
			#endif
	}
	total_geodesic_length_weighted(origin_link,r) += euclidean_cost_of_geodesic * dest_weight;

	diversion_ratio(origin_link,r) += euclidean_cost_of_geodesic / crow_flies_distance * dest_weight;
	#ifdef _SDNADEBUG
		cout << "euclidean geodesic from link " << origin_link->arcid << "-" << destination_link->arcid << " is " << euclidean_cost_of_geodesic << endl;
		cout << "crow flies distance is " << crow_flies_distance << endl;
	#endif
	if (output_geodesics)
	{
		shared_ptr<sDNADataMultiGeometry> geodesicdata(
			new sDNADataMultiGeometry(
				POLYLINEZ,
				get_geodesic_data(origin_link->arcid,destination_link->arcid,(float)radii[r],
								  (float)betweenness_weight,
								  (float)two_stage_betweenness_weight,
								  (float)get_geodesic_analytical_cost(dest,anal_best_costs_reaching_edge),
								  (float)euclidean_cost_of_geodesic),
				1));
		shared_ptr<sDNAGeometryPointsByEdgeLength> geodesic(new sDNAGeometryPointsByEdgeLength());
		assert(destination_link != origin_link);
		geodesic->add_edge_length_from_middle_to_end(origin_exit_edge,numeric_limits<float>::infinity());
		for (vector<Edge*>::reverse_iterator it=intermediate_edges.rbegin();it!=intermediate_edges.rend();it++)
			geodesic->add_edge_length_from_start_to_end(*it,numeric_limits<float>::infinity());
		geodesic->add_edge_length_from_start_to_end(dest.routing_edge,dest.length_to_centre);
		geodesicdata->add_part(geodesic);
		geodesics.add(geodesicdata);
	}
	if (output_destinations)
	{
		//this could go in process_destination, which would make output of destinations run faster
		//but only if we ditched two_stage_betweenness_weight and euclidean_cost_of_geodesic
		//so for the time being, here it stays
		shared_ptr<sDNADataMultiGeometry> destinationdata(
			new sDNADataMultiGeometry(
				POLYLINEZ,
				get_destination_data(origin_link->arcid,destination_link->arcid,radii[r],
									 (float)origin_weight,
									 (float)two_stage_betweenness_weight,
									 (float)get_geodesic_analytical_cost(dest,anal_best_costs_reaching_edge),
									 (float)euclidean_cost_of_geodesic),
				 1));
		shared_ptr<sDNAGeometryPointsByEdgeLength> destination(new sDNAGeometryPointsByEdgeLength());
		destination->add_edge_length_from_start_to_end(dest.routing_edge,dest.length_of_edge_inside_radius);
		destinationdata->add_part(destination);
		destinationgeoms.add(destinationdata);
	}

	if (run_probroutes)
	{
		const double excess = euclidean_cost_of_geodesic - radii[r];
		if (excess > 0)
		{
			//problem route
			prob_route_weight(origin_link,r) += dest_weight;
			prob_route_excess(origin_link,r) += dest_weight * excess / radii[r];
		}
	}
}

DestinationEdgeProcessingTask DestinationEdgeProcessingTask::getRadialEquivalent(IdIndexedArray<double,EdgeId> &radialcosts,
																			MetricEvaluator* metric_eval)
{
	return DestinationSDNAPolylineSegment(geom_edge,length_of_edge_inside_radius).getDestinationEdgeProcessingTaskRadial(radialcosts,metric_eval);
}

double SDNAIntegralCalculation::backtrace(DestinationEdgeProcessingTask &t,SDNAPolyline * const origin_link,
										  IdIndexedArray<Edge *  ,EdgeId> &backlinks_edge,
										  vector<Edge*> &intermediate_edges,Edge **origin_exit_edge,bool& passed_intermediate_filter)
{
	passed_intermediate_filter = false;

	Edge * const destination_edge = t.routing_edge;

	double euclidean_cost_of_geodesic = 0;
	intermediate_edges.clear();

	//add length from end of link to centre of analysed segment - which may be at the far end - the DestinationEdgeProcessingTask knows
	euclidean_cost_of_geodesic += t.length_to_centre;
	
	//intermediate_edge starts as penultimate link in route
	Edge *intermediate_edge = backlinks_edge[*destination_edge];
				
	//assert no u-turn on destination edge (see above)
	assert(intermediate_edge->link != destination_edge->link);

	while (intermediate_edge->link != origin_link)
	{
		if (using_intermediate_link_filter && !passed_intermediate_filter && intermediate_filter[*(intermediate_edge->link)])
			passed_intermediate_filter = true;
		
		//assert no u-turn ahead in backtrace/behind on route (see above)
		assert(backlinks_edge[*intermediate_edge]->link != intermediate_edge->link);
		
		assert(intermediate_edge->traversal_allowed());
		intermediate_edges.push_back(intermediate_edge);
		euclidean_cost_of_geodesic += intermediate_edge->full_cost_ignoring_oneway().euclidean;
		intermediate_edge = backlinks_edge[*intermediate_edge];
	}
				
	//now we have reached the origin there should be no more backlinks (ie no u-turn on origin)
	assert(backlinks_edge[*intermediate_edge]==NULL);

	*origin_exit_edge = intermediate_edge;
	assert((*origin_exit_edge)->traversal_allowed());
	euclidean_cost_of_geodesic += (*origin_exit_edge)->get_end_traversal_cost_ignoring_oneway().euclidean;
	return euclidean_cost_of_geodesic;
}

void PartialNet::count_junctions_accumulate(long &num_junctions, float &connectivity)
{
	for (JunctionId i=JunctionId(0);i.id<junction_radial_costs->get_size();++i.id)
	{
		if ((*junction_radial_costs)[i] <= radius)
		{
			Junction * const junction = net->get_junction_from_id(i);
			BOOST_FOREACH(float gs,junction->get_gradeseps())
			{
				const size_t attached_line_ends = junction->get_outgoing_edges_for_gradesep(gs).size();
				if (attached_line_ends > 2) //1 would be dead end; 2 would be split link
				{
					num_junctions++;
					connectivity += junction->get_connectivity_respecting_oneway(net,gs);
				}
			}
		}
	}
}

void SDNAIntegralCalculation::finalize_radius_geometry(SDNAPolyline *origin,int r,shared_ptr<sDNADataMultiGeometry> &all_edge_segments_in_radius)
{
	if (run_convex_hull)
	{
		//unpack points for convex hull algorithm
		BoostLineString3d hull_input_points; 
		DEBUG_PRINT("     unpacking radius geometry");
		for (unsigned long i=0;i<all_edge_segments_in_radius->get_num_parts();i++)
			all_edge_segments_in_radius->append_points_to(i,hull_input_points);

		//flatten hull_input_points
		//not sure whether this inefficiency is optimized away, 
		//ideal would be to adapt sDNA Point class to support boost point concepts 2d and 3d
		BoostLineString2d hull_input_points_2d;
		for (BoostLineString3d::iterator it=hull_input_points.begin(); it!=hull_input_points.end(); ++it)
			hull_input_points_2d.push_back(point_xy<double>(it->get<0>(),it->get<1>()));

		//calculate convex hull area, perimiter, max radius and bearing of max radius
		hull_type convex_hull;
		boost::geometry::convex_hull(hull_input_points_2d,convex_hull);
		convex_hull_area(origin,r) += (float)boost::geometry::area(convex_hull);
		convex_hull_perimeter(origin,r) += (float)boost::geometry::perimeter(convex_hull);

		typedef vector<point_xy<double> > hullpointvector;
		hullpointvector &hull_points = convex_hull.outer();
		Point &centre = origin->get_centre();
		float max_hull_square_radius = 0;
		Point max_hull_point = centre;
		for (hullpointvector::iterator it = hull_points.begin(); it!=hull_points.end(); ++it)
		{
			Point hull_point(it->x(),it->y(),0);
			float squaredistance = Point::squaredistance(centre,hull_point);
			if (squaredistance > max_hull_square_radius)
			{
				max_hull_square_radius = squaredistance;
				max_hull_point = hull_point;
			}
		}
		convex_hull_max_radius(origin,r) += (float)sqrt(max_hull_square_radius);
		convex_hull_bearing(origin,r) += Point::bearing(centre,max_hull_point);

		//save geometries if needed
		if (output_hulls)
		{
			shared_ptr<sDNADataMultiGeometry> saved_hull_with_data(
				new sDNADataMultiGeometry(POLYGON,get_hull_or_netradius_data(origin->arcid,radii[r],origweightdata.get_data(*origin)),1)
			);
			shared_ptr<sDNAGeometry> saved_hull(new sDNAGeometryPointsByValue(convex_hull));
			saved_hull_with_data->add_part(saved_hull);
			hulls.add(saved_hull_with_data);
		}
	}

	if (output_netradii)
		netradii.add(all_edge_segments_in_radius);
	else
		assert(all_edge_segments_in_radius.unique()); //check nobody else has the pointer so it will be destroyed
}

MetricEvaluator* SDNAIntegralCalculation::create_metric_from_config(ConfigStringParser &config,string metric_field,string hybrid_line_expr_field,string hybrid_junc_expr_field,string custom_cost_field)
{
	string out_measure = config.get_string(metric_field);
	to_lower(out_measure);
	
	//check user supplied formulae get used
	if (out_measure != "hybrid")
	{
		if (!config.get_string(hybrid_line_expr_field).empty())
			print_warning_callback("WARNING: lineformula was supplied but is not being used");
		if (config.get_string(hybrid_junc_expr_field)!="0")
			print_warning_callback("WARNING: juncformula was supplied but is not being used");
	}
	if (out_measure != "custom" && !config.get_string(custom_cost_field).empty())
		print_warning_callback("WARNING: custommetric was supplied but is not being used");

	//create metric evaluator
	if (out_measure=="angular")
	{
		return new AngularMetricEvaluator();
	}
	else if (out_measure=="euclidean")
	{
		return new EuclideanMetricEvaluator();
	}
	else if (out_measure=="custom")		
	{
		customcostdata = NetExpectedDataSource<float>(config.get_string(custom_cost_field),1,net,"",print_warning_callback);
		add_expected_data(&customcostdata);
		return new CustomMetricEvaluator(&customcostdata);
	}
	else if (out_measure=="cycle")
	{
		string aadtfield=config.get_string("aadtfield");
		stringstream ss; ss << "Using field "<<aadtfield<<" as estimate of annual average daily traffic";
		print_warning_callback(ss.str().c_str());
		string t=config.get_string("t");
		string a=config.get_string("a");
		string s=config.get_string("s");
		if (t=="default") t="0.04";
		if (a=="default") a="0.2";
		if (s=="default") s="2";
		stringstream ss1; ss1<<"Cycle metric parameters: t="<<t<<", a="<<a<<", s="<<s;
		print_warning_callback(ss1.str().c_str());
		stringstream lf, jf;
		lf << "_a=" << a << ",_s=" << s << ",_t=" << t
			<< ",_slope = hg/euc*100,_slopefac = _slope<2?1:(_slope<4?1.371:(_slope<6?2.203:4.239)),"
			<< "_trafficfac = 0.84*exp(" << aadtfield << "/1000), "
			<< "euc* (_slopefac^_s) * (_trafficfac^_t) + _a*67.2/90*ang";
		jf << a << "*67.2/90*ang";
		stringstream ss2; ss2<<"Equivalent hybrid metric: lineformula="<<lf.str()<<";juncformula="<<jf.str();
		print_warning_callback(ss2.str().c_str());
		HybridMetricEvaluator *hme = new HybridMetricEvaluator(lf.str(),jf.str(),net,this);
		return hme;
	}
	else if (out_measure=="cycle_roundtrip")
	{
		string aadtfield=config.get_string("aadtfield");
		stringstream ss; ss << "Using field "<<aadtfield<<" as estimate of annual average daily traffic";
		print_warning_callback(ss.str().c_str());
		string t=config.get_string("t");
		string a=config.get_string("a");
		string s=config.get_string("s");
		if (t=="default") t="0.04";
		if (a=="default") a="0.2";
		if (s=="default") s="2";
		stringstream ss1; ss1<<"Cycle symmetric metric parameters: t="<<t<<", a="<<a<<", s="<<s;
		print_warning_callback(ss1.str().c_str());
		stringstream lf, jf;
		lf << "_a=" << a << ",_s=" << s << ",_t=" << t
			<< ",_slope = hg/euc*100,_slopefac = _slope<2?1:(_slope<4?1.371:(_slope<6?2.203:4.239))"
			<< ",_slopeb = hl/euc*100,_slopefacb = _slopeb<2?1:(_slopeb<4?1.371:(_slopeb<6?2.203:4.239)),"
			<< "_trafficfac = 0.84*exp(" << aadtfield << "/1000), "
			<< "euc* (_slopefac^_s + _slopefacb^_s) * (_trafficfac^_t) + _a*67.2/90*ang*2";
		jf << a << "*67.2/90*ang*2";
		stringstream ss2; ss2<<"Equivalent hybrid metric: lineformula="<<lf.str()<<";juncformula="<<jf.str();
		print_warning_callback(ss2.str().c_str());
		HybridMetricEvaluator *hme = new HybridMetricEvaluator(lf.str(),jf.str(),net,this);
		return hme;
	}
	else if (out_measure=="euclidean_angular")
	{
		stringstream lf, jf;
		lf << "euc+ang";
		jf << "ang";
		stringstream ss2; ss2<<"Equivalent hybrid metric: lineformula="<<lf.str()<<";juncformula="<<jf.str();
		print_warning_callback(ss2.str().c_str());
		HybridMetricEvaluator *hme = new HybridMetricEvaluator(lf.str(),jf.str(),net,this);
		return hme;
	}
	else throw BadConfigException("Unrecognised metric option: "+out_measure);
}
