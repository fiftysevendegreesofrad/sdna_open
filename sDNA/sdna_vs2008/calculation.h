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
#include "calculationbase.h"
#include "sdna_output_utils.h"
#include "sdna_spatial_accumulators.h"
#include "metricevaluator.h"
#include "random.h"
#include "tables.h"
#pragma once

void run_dijkstra(PartialNet &partialnet, 
	IdIndexedArray<double,EdgeId> &costs_to_edge_start, MetricEvaluator*, double max_depth,
	IdIndexedArray <Edge *,EdgeId> *backlinks = NULL, JunctionCosts *jcosts = NULL);

enum prob_route_action_t {DISCARD, REROUTE};

//In contrast to DestinationSDNAPolylineSegment this *does* represent the correct routing and geometric edges to take, along with costs to centre of segment
struct DestinationEdgeProcessingTask
{
	Edge *routing_edge, *geom_edge;
	float length_of_edge_inside_radius;
	float cost_to_centre, length_to_centre;
	DestinationEdgeProcessingTask(Edge *re, Edge *ge, float len, float cc, float lc)
		: routing_edge(re), geom_edge(ge), length_of_edge_inside_radius(len), cost_to_centre(cc), length_to_centre(lc) 
	{
		//cout << "constructed DEPT redge=" << re->get_id().id << " cc=" << cc << " length_inside=" << len << endl; 
	}
	DestinationEdgeProcessingTask getRadialEquivalent(IdIndexedArray<double,EdgeId> &radialcosts,
																			MetricEvaluator* metric_eval);
};

//This represents a segment of a link to process as a destination
//It is specified by the edge that begins at the relevant end of the link, plus a length
//Note that this is NOT necessarily the destination edge, which may be the twin edge in the link, with an offset
//That's what the methods in this class create - actual destination edges for routing...
struct DestinationSDNAPolylineSegment
{
private:
	Edge *edge;
	float length_of_edge_inside_radius;
public:
	DestinationSDNAPolylineSegment(Edge *e, float len) 
		: edge(e), length_of_edge_inside_radius(len) {}

	DestinationEdgeProcessingTask getDestinationEdgeProcessingTaskAnalytical(IdIndexedArray<double,EdgeId> &analcosts,
																			MetricEvaluator* metric_eval,bool strict_network_cut)
	{
		assert(edge->traversal_allowed());
		//cout << "called gdept for edge " << edge->get_id().id << " lengthinside=" << length_of_edge_inside_radius << endl;
		const TraversalEventAccumulator fwd_end_to_centre_costs = edge->csccl(length_of_edge_inside_radius);
		const float fwd_end_to_centre_anal_cost = metric_eval->evaluate_edge(fwd_end_to_centre_costs,edge);
		
		if ((strict_network_cut && length_of_edge_inside_radius < edge->full_cost_ignoring_oneway().euclidean)
			|| !edge->get_twin()->traversal_allowed())
		{
			//cout << "strict partial or oneway edge" << endl;
			return DestinationEdgeProcessingTask(edge,edge,length_of_edge_inside_radius,fwd_end_to_centre_anal_cost,
												fwd_end_to_centre_costs.euclidean); 
		}
		assert(edge->get_twin()->traversal_allowed()); 
		const double fwd_anal_cost_to_start = analcosts[*edge];
		const double bwd_anal_cost_to_start = analcosts[*edge->get_twin()];
		const TraversalEventAccumulator bwd_end_to_centre_costs = edge->get_twin()->cscclfe(length_of_edge_inside_radius);
		const float bwd_end_to_centre_anal_cost = metric_eval->evaluate_edge(bwd_end_to_centre_costs,edge);
		const double fwd_anal_cost = fwd_anal_cost_to_start + fwd_end_to_centre_anal_cost;
		const double bwd_anal_cost = bwd_anal_cost_to_start + bwd_end_to_centre_anal_cost;

		if (fwd_anal_cost <= bwd_anal_cost)
			return DestinationEdgeProcessingTask(edge,edge,length_of_edge_inside_radius,fwd_end_to_centre_anal_cost,
												fwd_end_to_centre_costs.euclidean); 
		else
			return DestinationEdgeProcessingTask(edge->get_twin(),edge,length_of_edge_inside_radius,bwd_end_to_centre_anal_cost,
												bwd_end_to_centre_costs.euclidean);
	}
	DestinationEdgeProcessingTask getDestinationEdgeProcessingTaskRadial(IdIndexedArray<double,EdgeId> &radialcosts,
																			MetricEvaluator* metric_eval)
	{
		//picks the routing edge that is better according to the radial (not analytical) metric
		assert (edge->traversal_allowed());
		
		const TraversalEventAccumulator fwd_end_to_centre_costs = edge->csccl(length_of_edge_inside_radius);
		
		if (!edge->get_twin()->traversal_allowed())
		{
			//i don't think the output of metric_eval, stored in the DEPT, gets used anywhere at present
			//(though i'm not certain).  it would be used if we made closeness work for rerouted problem geodesics though
			const float fwd_end_to_centre_anal_cost = metric_eval->evaluate_edge(fwd_end_to_centre_costs,edge);
			return DestinationEdgeProcessingTask(edge,edge,length_of_edge_inside_radius,fwd_end_to_centre_anal_cost,fwd_end_to_centre_costs.euclidean);
		}

		assert (edge->get_twin()->traversal_allowed());
		const double fwd_radial_cost_to_start = radialcosts[*edge];
		const double bwd_radial_cost_to_start = radialcosts[*edge->get_twin()];
		const TraversalEventAccumulator bwd_end_to_centre_costs = edge->get_twin()->cscclfe(length_of_edge_inside_radius);
		const double fwd_radial_cost = fwd_radial_cost_to_start + fwd_end_to_centre_costs.euclidean;
		const double bwd_radial_cost = bwd_radial_cost_to_start + bwd_end_to_centre_costs.euclidean;

		//i don't think the output of metric_eval, stored in the DEPT, gets used anywhere
		//(though i'm not certain).  it would be used if we made closeness work for rerouted problem geodesics though
		if (fwd_radial_cost <= bwd_radial_cost)
		{
			const float fwd_end_to_centre_anal_cost = metric_eval->evaluate_edge(fwd_end_to_centre_costs,edge);
			return DestinationEdgeProcessingTask(edge,edge,length_of_edge_inside_radius,fwd_end_to_centre_anal_cost,fwd_end_to_centre_costs.euclidean);
		}
		else
		{
			assert(edge->get_twin()->traversal_allowed()); //if it wasn't the radial cost would have been infinity
			const float bwd_end_to_centre_anal_cost = metric_eval->evaluate_edge(bwd_end_to_centre_costs,edge);
			return DestinationEdgeProcessingTask(edge->get_twin(),edge,length_of_edge_inside_radius,bwd_end_to_centre_anal_cost,bwd_end_to_centre_costs.euclidean);
		}
	}
};

class SDNAIntegralCalculation : public Calculation, public SDNAPolylineDataSource
{
	friend class DataExpectedByExpression;

public: //for HybridMetricEvaluator
	float get_link_fraction(SDNAPolyline *p) 
	{
		if (net->num_items()==link_fraction.get_size())
			return link_fraction[*p];
		else
			return -1;
	}
private:
	bool check_no_zero_length_edges();
	bool check_no_oneway_conflicts();
	virtual bool run_internal();
	bool run_internal_throwing_exceptions();
	double calculateLinkFraction(SDNAPolyline *s);
	double getDistanceToJunction(Edge *e);

	OutputMap output_map;

	string output_name_prefix, output_name_postfix;
	
	//----------------------
	//config params:
	
	//not strictly a factory (as in factory method) but have to use it as such by copying it to each thread
	//hence it's a kind of blueprint
	scoped_ptr<MetricEvaluator> analysis_evaluator_factory;
	scoped_ptr<HybridMetricEvaluator> zonedist_evaluator_factory;
	
	scoped_ptr<HybridMetricEvaluator> line_disable_evaluator, intermediate_filter_evaluator;
	
	traversal_event_type link_centre_type;
	float acc_dist_constant_factor; //for accessibilty measurement, to prevent infinities
	bool cont_space, origin_cont_space, skip_zero_weight_origins, skip_zero_weight_destinations, strict_network_cut;
	double handle_prob_routes_if_over;
	prob_route_action_t prob_route_action;
	long skip_fraction,skip_mod;
	double nqpdn, nqpdd;
	int oversample;
	weighting_t weight_type;
	
	int (__cdecl *set_progressor_callback)(float);
	vector<double> radii, lower_bound_radii; 

	bool run_closeness, run_betweenness, run_junctions, run_convex_hull, run_probroutes, output_sums, output_decomposable_only, using_intermediate_link_filter;


	//helper funcs
	void assign_output_names();
	void initialize_zones();
	void initialize_skim_zones();
	void sdna_inner_loop(long origin_link_index,
						  vector<SDNAPolyline*> &links,
						  ThreadSafeProgressBar &thread_safe_progress_bar,
						  MetricEvaluator* radial_evaluator,
						  MetricEvaluator* analysis_evaluator,
						  size_t predicted_max_geodesic_steps);
	void pick_better_edge(SDNAPolyline *dest,IdIndexedArray<double,EdgeId> &best_costs_edge,IdIndexedArray<Edge*,EdgeId> &backlinks_edge,
		IdIndexedArray<double,SDNAPolylineId> &best_costs_link, IdIndexedArray<Edge*,SDNAPolylineId> &backtrace_start_edge);
	void process_destination(DestinationEdgeProcessingTask &dest,SDNAPolyline *origin_link, 
											  int r,
											  IdIndexedArray<double  ,EdgeId> &anal_best_costs_reaching_edge,
											  shared_ptr<sDNADataMultiGeometry> &all_edge_segments_in_radius,
											  double& total_weight_this_origin_sample_radius);
	void process_geodesic(DestinationEdgeProcessingTask &dest,PartialNet &cut_net, int r,
											  vector<Edge*> &intermediate_edges,
											  IdIndexedArray<double  ,EdgeId> &anal_best_costs_reaching_edge,
											  MetricEvaluator *analysis_evaluator,
											  double& total_weight_this_origin_sample_radius);
	double backtrace(DestinationEdgeProcessingTask &t,SDNAPolyline * origin_link,
										  IdIndexedArray<Edge *  ,EdgeId> &anal_backlinks_edge,
										  vector<Edge*> &intermediate_edges,Edge **origin_exit_edge,bool &passed_intermediate_filter);
	void process_origin(SDNAPolyline *origin,int r,shared_ptr<sDNADataMultiGeometry> &all_edge_segments_in_radius,
										MetricEvaluator *analysis_evaluator,double& total_weight_this_origin_sample_radius);
	void finalize_radius_geometry(SDNAPolyline *origin,int r,shared_ptr<sDNADataMultiGeometry> &all_edge_segments_in_radius);
	static double get_geodesic_analytical_cost(DestinationEdgeProcessingTask &dest,IdIndexedArray<double  ,EdgeId> &anal_best_costs_reaching_edge);

	MetricEvaluator* create_metric_from_config(ConfigStringParser &config,string metric_field,string hybrid_line_expr_field,string hybrid_junc_expr_field,string custom_cost_field);

	//global calc arrays - to store results - these have dimensions of net->link_container.size() * radii.size()
	//mega big precision needed for summing lots of little bits
	//though in msvc long double is the same as double - see precision analysis in issue tracker
	BidirectionalThreadSafeAccumulator<long double> betweenness, two_stage_betweenness; 
	ThreadSafeAccumulator<double> two_stage_dest_popularity;
	AccessibilityAccumulator accessibility; 
	SDNAPolylineIdRadiusIndexed2dArray<double> closeness, total_angular_cost, total_link_length,	//some precision needed for the same reason
											total_geodesic_length_weighted,									//some precision needed for the same reason
											diversion_ratio, total_crow_flies,								//some precision needed for the same reason
											total_weight,													//some precision needed for the same reason
											prob_route_weight, prob_route_excess;
	SDNAPolylineIdRadiusIndexed2dArray<float> convex_hull_area, convex_hull_perimeter, convex_hull_max_radius, convex_hull_bearing, total_connectivity; //precision not an issue
	SDNAPolylineIdRadiusIndexed2dArray<long> total_num_junctions;
	SDNAPolylineIdRadiusIndexed2dArray<double> total_num_links; // double? yes, we can have fractional link counts in continuous space
	SDNAPolylineIdIndexedArray<float> link_fraction, origweightdata, destweightdata;
	SDNAPolylineIdIndexedArray<bool> intermediate_filter;
	SDNAPolylineIdIndexedArray<size_t> skim_origzoneindex,skim_destzoneindex;
	
	vector<vector<long double> > skim_matrix_sum_distance, skim_matrix_weight;
	vector<vector<unsigned long> > skim_matrix_n;
	ZoneIndexMapping skim_origins,skim_destinations;
	NetExpectedDataSource<string> skim_origzone_das,skim_destzone_das;

	NetExpectedDataSource<float> customcostdata, skiporiginifzerodata, onewaydata, vertonewaydata;
	LengthWeightingStrategy origweightsource, destweightsource;
	shared_ptr<HybridMetricEvaluator> origweightexpr, destweightexpr;

	SDNAPolylineDataSourceGeometryCollectionWrapper netdata;
	sDNAGeometryCollection geodesics,hulls,netradii,destinationgeoms,skim_geom;
	bool output_geodesics, output_hulls, output_netradii, output_destinations, output_skim, suppress_net_data;
	set<long> skiporiginsexcept, skipdestinationsexcept; //if non-empty then skip all links except those with these arcids

	bool using_od_matrix_bool;
	bool using_od_matrix() {return using_od_matrix_bool;}

	vector<LengthWeightingStrategy> datatokeep;
	vector<NetExpectedDataSource<string>> textdatatokeep;

	void unpack_config(char *configstring)
	{
		ConfigStringParser config(//allowable keywords
								  "preserve_net_config;start_gs;end_gs;radii;cont;metric;pre;post;nobetweenness;nojunctions;nohull;linkonly;forcecontorigin;nqpdn;nqpdd;"
								  "arcxytol;arcztol;xytol;ztol;skipzeroweightorigins;skipzeroweightdestinations;skipfraction;skipmod;outputsums;outputdecomposableonly;skiporiginifzero;"
								  "probroutes;probroutethreshold;probrouteaction;nostrictnetworkcut;weight;custommetric;weight_type;origweight;destweight;"
								  "outputgeodesics;outputhulls;outputnetradii;outputdestinations;outputskim;origins;destinations;nonetdata;linkcentretype;lineformula;juncformula;ignorenonlinear;bidir;oneway;vertoneway;oversample;"
								  "aadtfield;t;a;s;jp;disable;zonedist;odmatrix;intermediates;linefield;c;e;"
								  "skimorigzone;skimdestzone;skimzone;bandedradii;linerand;juncrand;zonesums;origweightformula;destweightformula;datatokeep;textdatatokeep",

								  //default values for those keywords if unspecified by user (0 is false for booleans)
								  "juncformula=0;preserve_net_config=0;start_gs=;end_gs=;radii=n;metric=angular;cont=0;pre=;post=;nobetweenness=0;nojunctions=0;nohull=0;linkonly=0;"
								  "forcecontorigin=0;nqpdn=1;nqpdd=1;arcxytol=;arcztol=;xytol=;ztol=;skipzeroweightorigins=0;skipzeroweightdestinations=1;skipfraction=1;skipmod=0;"
								  "nostrictnetworkcut=0;outputsums=0;probroutes=0;probroutethreshold=1.2;probrouteaction=ignore;outputdecomposableonly=0;skiporiginifzero=;"
								  "weight=;custommetric=;weight_type=link;origweight=;destweight=;linkcentretype=;lineformula=;"
								  "outputgeodesics=0;outputhulls=0;outputnetradii=0;outputdestinations=0;outputskim=0;origins=;destinations=;nonetdata=0;ignorenonlinear=0;bidir=0;oneway=;vertoneway=;oversample=1;"
								  "aadtfield=aadt;t=default;a=default;s=default;jp=default;disable=;zonedist=euc;odmatrix=0;intermediates=;"
								  "linefield=line;c=default;e=default;"
								  "skimorigzone=;skimdestzone=;skimzone=;bandedradii=0;linerand=0;juncrand=0;zonesums=;origweightformula=;destweightformula=;datatokeep=;textdatatokeep=",

							      configstring);

		set_net_gs_data(config);
		
		//this functionality kinda duplicated from prepareoperation.h
		vector<string> keepdatanames = config.get_vector("datatokeep");
		vector<string> textdatanames = config.get_vector("textdatatokeep");
		for (vector<string>::iterator it=keepdatanames.begin();it!=keepdatanames.end();it++)
			datatokeep.push_back(LengthWeightingStrategy(NetExpectedDataSource<float>(*it,net,print_warning_callback),POLYLINE_WEIGHT,NULL));
		for (vector<string>::iterator it=textdatanames.begin();it!=textdatanames.end();it++)
			textdatatokeep.push_back(NetExpectedDataSource<string>(*it,net,print_warning_callback));

		//add_expected_data will maintain pointers into these, so they must not be changed after
		add_expected_data(datatokeep);
		add_expected_data(textdatatokeep);

		vector<string> zs = config.get_vector("zonesums");
		BOOST_FOREACH(string& s,zs)
		{
			//sum expressions have form sumname=expr@zonename
			//split on first = only
			vector<string> item;
			size_t pos = s.find("=");
			if (pos==string::npos) throw BadConfigException("Encountered zone sum not of the form x=y@z: "+s);
			string varname = s.substr(0,pos);
			string rest = s.substr(pos+1);
			//split on first @ (more @s don't make sense but will throw error when searching for data with @ in the name)
			pos = rest.find("@");
			if (pos==string::npos) throw BadConfigException("Encountered zone sum without @ to specify zone field name: "+s);
			string expr = rest.substr(0,pos);
			string zfn = rest.substr(pos+1);
			shared_ptr<NetExpectedDataSource<string> > das(new NetExpectedDataSource<string>(zfn,net,print_warning_callback));
			add_expected_data(&*das);

			shared_ptr<HybridMetricEvaluator> eval(new HybridMetricEvaluator(expr,"0",net,this));
			shared_ptr<Table<long double> > zone_sum_table(new Table<long double>(varname.c_str(),das->get_name().c_str()));
			zonesumtables.push_back(zone_sum_table);
			zone_sum_evaluators.push_back(ZoneSum(varname,das,eval,zone_sum_table));
		}

		if (config.get_string("disable")!="")
		{
			line_disable_evaluator.reset(new HybridMetricEvaluator(config.get_string("disable"),"0",net,this));
		}
		else
			line_disable_evaluator.reset(new HybridMetricEvaluator("0","0",net,this));

		if (config.get_string("intermediates")!="")
		{
			using_intermediate_link_filter = true;
			intermediate_filter_evaluator.reset(new HybridMetricEvaluator(config.get_string("intermediates"),"0",net,this));
		}
		else
			using_intermediate_link_filter = false;

		if (config.get_bool("bidir"))
		{
			betweenness.set_bidirectional();
			two_stage_betweenness.set_bidirectional();
		}

		suppress_net_data = config.get_bool("nonetdata");

		output_name_prefix = config.get_string("pre");
		output_name_postfix = config.get_string("post");

		output_geodesics = config.get_bool("outputgeodesics");
		output_hulls = config.get_bool("outputhulls");
		output_netradii = config.get_bool("outputnetradii");
		output_destinations = config.get_bool("outputdestinations");
		output_skim = config.get_bool("outputskim");

		if (output_skim)
		{
			skim_geom = sDNAGeometryCollection(output_name_prefix+"skim"+output_name_postfix,NO_GEOM,get_skim_field_metadata());
		  	geometry_outputs.push_back(&skim_geom);

			string origzone = config.get_string("skimorigzone");
			string destzone = config.get_string("skimdestzone");
			string zone = config.get_string("skimzone");
			if (zone!="")
			{
				if (origzone!="")
					throw BadConfigException("Can't use skimzone and skimorigzone together");
				if (destzone!="")
					throw BadConfigException("Can't use skimzone and skimdestzone together");
				skim_origzone_das = NetExpectedDataSource<string>(zone,net,print_warning_callback);
				skim_destzone_das = NetExpectedDataSource<string>(zone,net,print_warning_callback);
			}
			else if (origzone!="" && destzone !="")
			{
				if (zone!="")
					throw BadConfigException("Can't use skimzone with skimorigzone and skimdestzone");
				skim_origzone_das = NetExpectedDataSource<string>(origzone,net,print_warning_callback);
				skim_destzone_das = NetExpectedDataSource<string>(destzone,net,print_warning_callback);
			}
			else
				throw BadConfigException("Must specify either skimzone, or skimorigzone and skimdestzone for skim matrix");

			add_expected_data(&skim_origzone_das);
			add_expected_data(&skim_destzone_das);
		}

		if (!suppress_net_data)
			geometry_outputs.push_back(&netdata);
		if (output_geodesics)
		{
			geodesics = sDNAGeometryCollection(output_name_prefix+"geodesics"+output_name_postfix,POLYLINEZ,get_geodesic_field_metadata());
		  	geometry_outputs.push_back(&geodesics);
		}
		if (output_hulls)
		{
			hulls = sDNAGeometryCollection(output_name_prefix+"hulls"+output_name_postfix,POLYGON,get_hull_or_netradius_field_metadata());
		  	geometry_outputs.push_back(&hulls);
		}
		if (output_netradii)
		{
			netradii = sDNAGeometryCollection(output_name_prefix+"netradii"+output_name_postfix,MULTIPOLYLINEZ,get_hull_or_netradius_field_metadata());
			geometry_outputs.push_back(&netradii);
		}
		if (output_destinations)
		{
			destinationgeoms = sDNAGeometryCollection(output_name_prefix+"destinations"+output_name_postfix,POLYLINEZ,get_destination_field_metadata());
			geometry_outputs.push_back(&destinationgeoms);
		}

		vector<string> skiporiginsexcept_s = config.get_vector("origins");
		for (vector<string>::iterator it=skiporiginsexcept_s.begin();it!=skiporiginsexcept_s.end();it++)
			skiporiginsexcept.insert(ConfigStringParser::safe_atol(*it));
		
		vector<string> skipdestinationsexcept_s = config.get_vector("destinations");
		for (vector<string>::iterator it=skipdestinationsexcept_s.begin();it!=skipdestinationsexcept_s.end();it++)
			skipdestinationsexcept.insert(ConfigStringParser::safe_atol(*it));
		
		string skiporiginifzero_fieldname = config.get_string("skiporiginifzero");
		skiporiginifzerodata = NetExpectedDataSource<float>(skiporiginifzero_fieldname,1,net,"",print_warning_callback);
		add_expected_data(&skiporiginifzerodata);
		
		string weighting_type_s = config.get_string("weight_type");
		to_lower(weighting_type_s);
		if (weighting_type_s=="link")
			weight_type = LINK_WEIGHT;
		else if (weighting_type_s=="polyline")
			weight_type = POLYLINE_WEIGHT;
		else if (weighting_type_s=="length")
			weight_type = LENGTH_WEIGHT;
		else
			throw BadConfigException("Unrecognized weighting type: "+weighting_type_s);

		string weightname = config.get_string("weight");
		string origweightname = config.get_string("origweight");
		string destweightname = config.get_string("destweight");
		if (weightname.size())
		{
			//weight option just sets origweight and destweight to the same thing
			if (origweightname.size())
				throw BadConfigException("Options 'weight' and 'origweight' cannot be used together.");
			if (destweightname.size())
				throw BadConfigException("Options 'weight' and 'destweight' cannot be used together.");
			origweightname = weightname;
			destweightname = weightname;
		}
		string origweightformula = config.get_string("origweightformula");
		string destweightformula = config.get_string("destweightformula");
		if (origweightformula.size())
		{
			if (origweightname.size())
				throw BadConfigException("Options 'origweight' and 'origweightformula' cannot be used together.");
			origweightexpr.reset(new HybridMetricEvaluator(origweightformula,"0",net,this));
			print_warning_callback(("Using origin weight expression: "+origweightformula).c_str());
		}
		else
		{
			origweightsource = LengthWeightingStrategy(NetExpectedDataSource<float>(origweightname,1,net,"",print_warning_callback),weight_type,&link_fraction);
			add_expected_data(&origweightsource);
		}
		if (destweightformula.size())
		{
			if (destweightname.size())
				throw BadConfigException("Options 'destweight' and 'destweightformula' cannot be used together.");
			destweightexpr.reset(new HybridMetricEvaluator(destweightformula,"0",net,this));
			print_warning_callback(("Using destination weight expression: "+destweightformula).c_str());
		}
		else
		{
			destweightsource = LengthWeightingStrategy(NetExpectedDataSource<float>(destweightname,1,net,"",print_warning_callback),weight_type,&link_fraction);
			add_expected_data(&destweightsource);
		}
		

		string onewayname = config.get_string("oneway");
		string vertonewayname = config.get_string("vertoneway");
		onewaydata = NetExpectedDataSource<float>(onewayname,0,net,"",print_warning_callback);
		vertonewaydata = NetExpectedDataSource<float>(vertonewayname,0,net,"",print_warning_callback);
		add_expected_data(&onewaydata);
		add_expected_data(&vertonewaydata);
		
		cont_space = config.get_bool("cont");

		vector<string> radii_s = config.get_vector("radii");
		//change any radius of -1 (which means global, or 'n' in space syntax notation) to max representable finite number
		//(nb don't use infinity - that represents literally unreachable links)
		for (vector<string>::iterator it=radii_s.begin(); it!=radii_s.end(); it++)
			if (*it=="n" || *it=="N")
				radii.push_back(GLOBAL_RADIUS);
			else
				radii.push_back(ConfigStringParser::safe_atod(*it));

		sort(radii.begin(),radii.end());
		if (!config.get_bool("bandedradii"))
			lower_bound_radii.swap(vector<double>(radii.size(),0.));
		else
		{
			if (cont_space)
				throw BadConfigException("Cannot use banded radius and continuous space together.\nIf you need this feature, please contact the sDNA team.");
			lower_bound_radii.swap(vector<double>(1,0.));
			lower_bound_radii.insert(lower_bound_radii.end(),radii.begin(),radii.end()-1);
		}

		
		HybridMetricEvaluator *zonedist = new HybridMetricEvaluator(config.get_string("zonedist"),"0",net,this);
		zonedist_evaluator_factory.reset(zonedist); //takes ownership

		using_od_matrix_bool = config.get_bool("odmatrix");
		
		string out_measure = config.get_string("metric");
		to_lower(out_measure);

		analysis_evaluator_factory.reset(create_metric_from_config(config,"metric","lineformula","juncformula","custommetric"));

		acc_dist_constant_factor = 0;
		string default_centre_type;
		if	(out_measure=="angular")
		{
			link_centre_type = ANGULAR_TE;
			default_centre_type = "angular";
			acc_dist_constant_factor = 180; //assume 90+90 degree turns to enter and leave network
		}
		else 
		{
			link_centre_type = EUCLIDEAN_TE;
			default_centre_type = "euclidean";
		}

		string centre_type = config.get_string("linkcentretype");
		to_lower(centre_type);
		if (centre_type=="angular")
		{
			print_warning_callback("Forcing link centres to Angular");
			link_centre_type = ANGULAR_TE;
		}
		else if (centre_type=="euclidean")
		{
			print_warning_callback("Forcing link centres to Euclidean");
			link_centre_type = EUCLIDEAN_TE;
		}
		else if (centre_type=="")
			print_warning_callback(("Using default of "+default_centre_type+" link centres for "+out_measure+" analysis").c_str());
		else
			throw BadConfigException("Unrecogised link centre type: "+centre_type);

		//origin is processed same as everything else 
		//unless forced to continuous space mode by cont_origin
		origin_cont_space = cont_space || config.get_bool("forcecontorigin");
		nqpdn = config.get_double("nqpdn");
		nqpdd = config.get_double("nqpdd");
		skip_zero_weight_origins = config.get_bool("skipzeroweightorigins");
		skip_zero_weight_destinations = config.get_bool("skipzeroweightdestinations");
		skip_fraction = config.get_long("skipfraction");
		skip_mod = config.get_long("skipmod");
		strict_network_cut = !config.get_bool("nostrictnetworkcut");

		run_closeness = !config.get_bool("linkonly");
		run_betweenness = !config.get_bool("nobetweenness");
		run_junctions = !config.get_bool("nojunctions");
		run_convex_hull = !config.get_bool("nohull");
		run_probroutes = config.get_bool("probroutes");

		if (output_hulls && (!run_convex_hull || !run_closeness))
			throw BadConfigException("Option outputhulls is incompatible with options linkonly/nohull");
		if (output_destinations && (!run_closeness))
			throw BadConfigException("Option outputdestinations is incompatible with option linkonly");
		if (output_geodesics && (!run_closeness || !run_betweenness))
			throw BadConfigException("Option outputgeodesics is incompatible with options linkonly/nobetweenness");
		if (output_netradii && (!run_closeness))
			throw BadConfigException("Option outputnetradii is incompatible with option linkonly");

		if ((output_hulls || output_destinations || output_geodesics || output_netradii) && skip_zero_weight_destinations && destweightname.size())
		{
			print_warning_callback("WARNING: skip_zero_weight_destinations is enabled,\nthis will cause zero-weighted destinations to be excluded from geometry outputs.");
			print_warning_callback("To change, add skipzeroweightdestinations=0 to advanced config.");
		}
		if ((output_hulls || output_destinations || output_geodesics || output_netradii) && skip_zero_weight_origins && origweightname.size())
		{
			print_warning_callback("WARNING: skip_zero_weight_origins is enabled,\nthis will cause zero-weighted origins to be excluded from geometry outputs.");
			print_warning_callback("To change, remove skipzeroweightorigins from advanced config.");
		}
		output_sums = config.get_bool("outputsums");
		output_decomposable_only = config.get_bool("outputdecomposableonly");

		handle_prob_routes_if_over = config.get_double("probroutethreshold");
		string probrouteaction = config.get_string("probrouteaction");
		to_lower(probrouteaction);
		if (probrouteaction=="ignore")
		{
			handle_prob_routes_if_over = numeric_limits<double>::infinity();
			prob_route_action = DISCARD; // shouldn't matter anyway
		}
		else 
		{
			if (probrouteaction=="discard") prob_route_action = DISCARD;
			else if (probrouteaction=="reroute") prob_route_action = REROUTE;
			else throw BadConfigException("Unrecognised problem route action: "+out_measure);
		}

		set_tolerance(config);
		
		oversample = config.get_long("oversample");
		if (oversample!=1)
		{
			if (oversample<1)
				throw BadConfigException("Bad value (less than 1) for oversample option");
		}

		//check no duplicate zone sums, zone tables and expected data
		vector<string> data_names;
		BOOST_FOREACH(ZoneSum zs, zone_sum_evaluators)
			data_names.push_back(zs.varname);
		BOOST_FOREACH(shared_ptr<Table<float> > zdt, zonedatatables)
			data_names.push_back(zdt->name());
		BOOST_FOREACH(NetExpectedDataSource<float>* neds,expecteddata_netonly_nodupl())
			data_names.push_back(neds->get_name());
		BOOST_FOREACH(NetExpectedDataSource<string>* neds,expectedtextdata_nodupl())
			data_names.push_back(neds->get_name());

		set<string> name_set;
		BOOST_FOREACH(string s,data_names)
		{
			if (name_set.find(s)==name_set.end())
				name_set.insert(s);
			else
			{
				print_warning_callback(("Data name "+s+" detected more than once in zonesums, zone data or net data.").c_str());
				throw BadConfigException("Duplicate data name is ambiguous");
			}
		}
	}

	void enable_arrays()
	{
		//these all need closeness to work:
		run_betweenness &= run_closeness;
		run_junctions &= run_closeness;
		run_convex_hull &= run_closeness;
		
		if (run_betweenness)
		{
			betweenness.enable();
			total_crow_flies.enable();
			total_geodesic_length_weighted.enable();
			diversion_ratio.enable();
			if (!using_od_matrix())
			{
				two_stage_betweenness.enable();
				two_stage_dest_popularity.enable();
			}
		}
		if (run_closeness)
		{
			accessibility.enable();
			closeness.enable();
			total_angular_cost.enable();
			total_link_length.enable();
			total_weight.enable();
			total_num_links.enable();
		}
		if (run_convex_hull)
		{
			convex_hull_area.enable();
			convex_hull_perimeter.enable();
			convex_hull_max_radius.enable();
			convex_hull_bearing.enable();
		}
		if (run_junctions)
		{
			total_num_junctions.enable();
			total_connectivity.enable();
		}
		if (run_probroutes)
		{
			prob_route_weight.enable();
			prob_route_excess.enable();
		}
	}

#define FIELD(x,y,z) result.push_back(FieldMetaData(x,y,z))

	static vector<SDNAVariant> get_skim_data(string orig,string dest,float meandist,float weight,long N)
	{
		vector<SDNAVariant> result(5);
		result[0] = SDNAVariant(orig);
		result[1] = SDNAVariant(dest);
		result[2] = SDNAVariant(meandist);
		result[3] = SDNAVariant(weight);
		result[4] = SDNAVariant(numeric_cast<long>(N));
		return result;
	}
	static vector<FieldMetaData> get_skim_field_metadata()
	{
		vector<FieldMetaData> result;
		FIELD(fSTRING,"Origin Zone","OrigZone");
		FIELD(fSTRING,"Destination Zone","DestZone");
		FIELD(fFLOAT,"Mean Distance","MD");
		FIELD(fFLOAT,"Weight","Wt");
		FIELD(fLONG,"Num Geodesics","NGeo");
		return result;
	}
	static vector<SDNAVariant> get_geodesic_data(long orig,long dest,double radius,float weight,float tpweight,float anal_cost,float radial_cost)
	{
		vector<SDNAVariant> result(7);
		result[0]=SDNAVariant(orig);
		result[1]=SDNAVariant(dest);
		result[2]=SDNAVariant((float)radius);
		result[3]=SDNAVariant(weight);
		result[4]=SDNAVariant(tpweight);
		result[5]=SDNAVariant(anal_cost);
		result[6]=SDNAVariant(radial_cost);
		return result;
	}
	static vector<FieldMetaData> get_geodesic_field_metadata()
	{
		vector<FieldMetaData> result;
		FIELD(fLONG,"Origin","Orig");
		FIELD(fLONG,"Destination","Dest");
		FIELD(fFLOAT,"Radius","Radius");
		FIELD(fFLOAT,"Weight","Weight");
		FIELD(fFLOAT,"Two Phase Weight","TPWeight");
		FIELD(fFLOAT,"Analytic Metric","AnalMet");
		FIELD(fFLOAT,"Length","Length");
		return result;
	}
	static vector<SDNAVariant> get_hull_or_netradius_data(long orig,double radius,float weight)
	{
		vector<SDNAVariant> result(3);
		result[0]=SDNAVariant(orig);
		result[1]=SDNAVariant((float)radius);
		result[2]=SDNAVariant(weight);
		return result;
	}
	static vector<FieldMetaData> get_hull_or_netradius_field_metadata()
	{
		vector<FieldMetaData> result;
		FIELD(fLONG,"Origin","Orig");
		FIELD(fFLOAT,"Radius","Radius");
		FIELD(fFLOAT,"Weight","Weight");
		return result;
	}
	static vector<SDNAVariant> get_destination_data(long orig,long dest,double radius,float origweight,float tpweight,float anal_cost,float radial_cost,
		float crowflight)
	{
		vector<SDNAVariant> result(9);
		result[0]=SDNAVariant(orig);
		result[1]=SDNAVariant(dest);
		result[2]=SDNAVariant((float)radius);
		result[3]=SDNAVariant(origweight);
		result[4]=SDNAVariant(tpweight);
		result[5]=SDNAVariant(anal_cost);
		result[6]=SDNAVariant(radial_cost);
		result[7]=SDNAVariant(crowflight);
		result[8]=SDNAVariant(radial_cost-crowflight);
		return result;
	}
	static vector<FieldMetaData> get_destination_field_metadata()
	{
		vector<FieldMetaData> result;
		FIELD(fLONG,"Origin","Orig");
		FIELD(fLONG,"Destination","Dest");
		FIELD(fFLOAT,"Radius","Radius");
		FIELD(fFLOAT,"Weight from origin","WeightO");
		FIELD(fFLOAT,"TPWeight from origin","TPWeightO");
		FIELD(fFLOAT,"Analytic Metric","AnalMet");
		FIELD(fFLOAT,"Length","Length");
		FIELD(fFLOAT,"Crow flight","CrowFlt");
		FIELD(fFLOAT,"Diversion Abs","Div");
		return result;
	}
		
	CalcExpectedDataSDNAPolylineDataSourceWrapper inputdatawrapper;
public:

	//sdnapolylinedatasource implementation for output of calculation (not input data)
	virtual size_t get_output_length() { return output_map.getlength(); }
	virtual vector<FieldMetaData> get_field_metadata()
	{
		vector<FieldMetaData> result;
		for (size_t i=0;i<output_map.getlength();i++)
			result.push_back(FieldMetaData(fFLOAT,output_map.get_output_names()[i],output_map.get_short_output_names()[i]));
		return result;
	}
	virtual vector<SDNAVariant> get_outputs(long arcid) 
	{
		vector<SDNAVariant> result;
		result.reserve(get_output_length());
		BOOST_FOREACH (const float &f , output_map.get_outputs(net->link_container[arcid],oversample))
			result.push_back(SDNAVariant(f));
		return result;
	}
	virtual Net* getNet() {return net;}
	
	//deprecated interface still used by autocad:
	char ** get_all_output_names_c() { return output_map.get_output_names_c(); }
	char ** get_short_output_names_c() { return output_map.get_short_output_names_c(); }
	void get_all_outputs_c(float* buffer, long arcid)	{ output_map.get_outputs_c(buffer,net->link_container[arcid],oversample); }
	
	SDNAIntegralCalculation(Net *net,char *configstring,
		int (__cdecl *set_progressor_callback)(float), int(__cdecl *print_warning_callback)(const char*), vector<shared_ptr<Table<float>>>* tables1d_in=NULL)
		: Calculation(net,print_warning_callback), 
	      set_progressor_callback(set_progressor_callback),
		  using_od_matrix_bool(false),
		  weight_type((weighting_t)-1)
	{
		if (tables1d_in)
		{
			zonedatatables=*tables1d_in;
			delete tables1d_in; 
		}
		add_expected_data_for_table_zonefields();
		
		//unpack config first; enable_arrays is config dependent
		unpack_config(configstring);

		//enable arrays must happen before assign_output_names
		//as the enable logic determines which arrays get output
		enable_arrays();

		assign_output_names();

		vector<SDNAPolylineDataSource*> ldsv;
		inputdatawrapper = CalcExpectedDataSDNAPolylineDataSourceWrapper(this);
		ldsv.push_back(&inputdatawrapper); //input data
		ldsv.push_back(this); //output data 
		netdata = SDNAPolylineDataSourceGeometryCollectionWrapper(ldsv);
	}
	
};

//this class defines a partial net (or a full one, which is a special case of a partial one)
class PartialNet
{
	friend class PartialNetCollection;

private:
	Net *net;
	double radius, lower_bound_radius;
	bool cont_space;
	MetricEvaluator * radial_eval;

	//the following are null in the special case of this being a full net
	//and are owned by the factory PartialNetCollection
	IdIndexedArray<double,EdgeId> *radial_costs_to_edge_start; 
	JunctionCosts *junction_radial_costs;

	//this is null unless prob_route_method==REROUTE
	//and also owned by factory PartialNetCollection
	IdIndexedArray<Edge *,EdgeId> *radial_backlinks_edge;

	//the following are null if uncomputed
	shared_ptr<IdIndexedArray<double,EdgeId>> anal_best_costs_reaching_edge; 
	shared_ptr<IdIndexedArray<Edge *,EdgeId> > anal_backlinks_edge; 
	
	//generates partial net radius r, requires radial costs to be precomputed
	//anal_best_costs and anal_backlinks are default constructed to NULL shared ptr
	PartialNet(Net *net,double r,double lbr,bool cont,MetricEvaluator* radial_eval,
			IdIndexedArray<double,EdgeId> *radial_costs,IdIndexedArray<Edge *,EdgeId> *rbl,SDNAPolyline *origin,JunctionCosts *jc) 
		: cont_space(cont), radial_eval(radial_eval),
		  net(net), radius(r), lower_bound_radius(lbr),
		  radial_costs_to_edge_start(radial_costs), radial_backlinks_edge(rbl), origin(origin), junction_radial_costs(jc) {};

	//generates full net
	PartialNet(Net *net,SDNAPolyline *origin) 
		: net(net), origin(origin), radial_eval(NULL), radius(GLOBAL_RADIUS), lower_bound_radius(0), 
		radial_costs_to_edge_start(NULL), junction_radial_costs(NULL) {}
	
	void compute_analytical_costs(MetricEvaluator* metric_eval)
	{
		anal_best_costs_reaching_edge.reset(new IdIndexedArray<double,EdgeId>(net->num_edges()));
		anal_backlinks_edge.reset(new IdIndexedArray<Edge *,EdgeId>(net->num_edges()));	
		//max_depth for dijkstra is GLOBAL_RADIUS - edges beyond the radius of this partial net are filtered by 
		//the inclusion predicate rather than max_depth
		run_dijkstra(*this,*anal_best_costs_reaching_edge,metric_eval,GLOBAL_RADIUS,
				&*anal_backlinks_edge);
	}

	void assign_analytical_costs(shared_ptr<IdIndexedArray<double,EdgeId>> shared_anal_best_costs_reaching_edge,
		shared_ptr<IdIndexedArray<Edge *,EdgeId>> shared_anal_backlinks_edge)
	{
		anal_best_costs_reaching_edge = shared_anal_best_costs_reaching_edge;
		anal_backlinks_edge = shared_anal_backlinks_edge;
	}


public:
	SDNAPolyline *origin;

	void count_junctions_accumulate(long &num_junctions, float &connectivity);

	IdIndexedArray<double,EdgeId> &get_anal_best_costs_reaching_edge_array()
		{ assert(anal_best_costs_reaching_edge); return *anal_best_costs_reaching_edge; }
	IdIndexedArray<double,EdgeId> &get_radial_best_costs_reaching_edge_array()
		{ assert(radial_costs_to_edge_start); return *radial_costs_to_edge_start; }
	IdIndexedArray<Edge *,EdgeId> &get_anal_backlinks_edge_array() 
		{ assert(anal_backlinks_edge); return *anal_backlinks_edge; }
	IdIndexedArray<Edge *,EdgeId> &get_radial_backlinks_edge_array() 
		{ assert(radial_backlinks_edge->isInitialized()); return *radial_backlinks_edge; }
	
	void get_outgoing_connections(CandidateEdgeVector &options,Edge *e,double cost_to_date,MetricEvaluator *evaluator,edge_position from,
		JunctionCosts *jcosts)
	{
		double remaining_radius;
		if (radial_costs_to_edge_start) // are radial costs defined?
			remaining_radius = radius - (*radial_costs_to_edge_start)[*e];
		else
			remaining_radius = GLOBAL_RADIUS;
		
		e->get_outgoing_connections(options,cost_to_date,remaining_radius,evaluator,radial_eval,from,jcosts);
	}

	//inclusion predicate for edges for routing algorithm defined as operator() for filter_iterator
	//note that the partial net inclusion predicate is permissive: it includes edges that get_outgoing_connections may later prohibit
	//due to oneway systems or strict routing
	struct PartialNetInclusionPredicate
	{
		IdIndexedArray<double,EdgeId> *radial_costs_to_edge_start; 
		double radius;
		PartialNetInclusionPredicate(IdIndexedArray<double,EdgeId> *radial_costs_to_edge_start,double radius)
			: radial_costs_to_edge_start(radial_costs_to_edge_start),
			radius(radius) {}
		bool operator()(Edge *e)
		{
			//if radial costs don't exist, return true as all edges are in partial net
			//if they do exist, check them
			//note we don't use lower bound radius here as all edges inside are needed for routing
			return (!radial_costs_to_edge_start || (*radial_costs_to_edge_start)[*e]<radius);
		}
	};
	
	typedef boost::filter_iterator<PartialNetInclusionPredicate, EdgePointerContainer::iterator > RoutingEdgeIter;
	
	RoutingEdgeIter get_routing_edges_begin()
	{
		PartialNetInclusionPredicate p(radial_costs_to_edge_start,radius);
		return RoutingEdgeIter(p,net->edge_ptr_container.begin(), net->edge_ptr_container.end());
	}
	RoutingEdgeIter get_routing_edges_end()
	{
		PartialNetInclusionPredicate p(radial_costs_to_edge_start,radius);
		return RoutingEdgeIter(p,net->edge_ptr_container.end(), net->edge_ptr_container.end());
	}
	
	void get_destination_processing_tasks(vector<DestinationSDNAPolylineSegment> &destinations)
	{
		assert(destinations.size()==0);
		destinations.reserve(net->edge_ptr_container.size());

		for (SDNAPolylineContainer::iterator destination_it = net->link_container.begin(); 
			destination_it != net->link_container.end(); destination_it++)
		{
			SDNAPolyline * const destination_link = &*destination_it->second;
			if (destination_link==origin) continue; 
		
			//get 0, 1 or 2 edges to use as analytical destinations
			get_dest_link_segments_to_process(destinations,destination_link);
		}
	}
	
private:
	void get_dest_link_segments_to_process(vector<DestinationSDNAPolylineSegment> &destinations,SDNAPolyline *destination_link)
	{
		//this method pulls out the segments of a link that are within the PartialNet to process
		assert(radial_costs_to_edge_start);
		assert(destination_link != origin);

		//if we want to allow non-euclidean radius with cont_space, see comments in unpack_config for list of required changes including the rest of this routine
		//(in which length_of_forward_edge_within_radius etc are changed to radcost and inclusion of segments is determined by cost not length)
		assert(!cont_space || radial_eval->abbreviation()=="Euc");

		Edge * const forward_edge = &destination_link->forward_edge;
		Edge * const backward_edge = &destination_link->backward_edge;
		const float edge_length = destination_link->full_link_cost_ignoring_oneway(PLUS).euclidean;
		const float full_radial_cost_fwd = radial_eval->evaluate_edge(forward_edge->full_cost_ignoring_oneway(),forward_edge);
		const float full_radial_cost_bwd = radial_eval->evaluate_edge(backward_edge->full_cost_ignoring_oneway(),backward_edge);

		//get radial costs
		const double fwd_radial_cost_to_start  = (*radial_costs_to_edge_start)[*forward_edge];
		const double bwd_radial_cost_to_start = (*radial_costs_to_edge_start)[*backward_edge];
		
		//zero length links that fall inside the radius will have a length of zero inside the radius
		//zero length links that fall outside the radius will have a length equal to their full length inside the radius
		//really we should disallow all zero length links, but for now...
		//these annoyances break the rest of the routine, so handle explicitly:
		if (full_radial_cost_fwd==0)
		{
			if (fwd_radial_cost_to_start < radius)
				destinations.push_back(DestinationSDNAPolylineSegment(forward_edge,0)); 
			return; //we have included the whole edge so no more segments allowed
		}
		if (full_radial_cost_bwd==0) //implicitly the forward cost wasn't zero or we wouldn't get here - this prevents double counting
		{
			if (bwd_radial_cost_to_start < radius) 
				destinations.push_back(DestinationSDNAPolylineSegment(backward_edge,0));
			return; //we have included the whole edge so no more segments allowed
		}

		//calculate length of each edge within radius
		float length_of_forward_edge_within_radius, length_of_backward_edge_within_radius;
		bool no_edges_below_discrete_lower_bound_radius = true;
		if (forward_edge->traversal_allowed())
		{
			if (cont_space)
				length_of_forward_edge_within_radius = forward_edge->partial_cost_from_start((float)(radius-fwd_radial_cost_to_start)).euclidean;
			else
			{
				//discretize: edge is either within radius or not
				const double radial_cost_to_centre = fwd_radial_cost_to_start + radial_eval->evaluate_edge(forward_edge->get_start_traversal_cost_ignoring_oneway(),forward_edge);
				length_of_forward_edge_within_radius = (radial_cost_to_centre < radius && radial_cost_to_centre >= lower_bound_radius)
					? edge_length : 0;
				if (radial_cost_to_centre < lower_bound_radius)
					no_edges_below_discrete_lower_bound_radius = false;
			}
		}
		else
			length_of_forward_edge_within_radius = 0; //one way
		if (backward_edge->traversal_allowed())
		{
			if (cont_space)
				length_of_backward_edge_within_radius = backward_edge->partial_cost_from_start((float)(radius-bwd_radial_cost_to_start)).euclidean;
			else
			{
				//discretize: edge is either within radius or not
				const double radial_cost_to_centre = bwd_radial_cost_to_start + radial_eval->evaluate_edge(backward_edge->get_start_traversal_cost_ignoring_oneway(),backward_edge);
				length_of_backward_edge_within_radius = (radial_cost_to_centre < radius && radial_cost_to_centre >= lower_bound_radius)
					? edge_length : 0;
				if (radial_cost_to_centre < lower_bound_radius)
					no_edges_below_discrete_lower_bound_radius = false;
			}
		}
		else
			length_of_backward_edge_within_radius = 0; //one way

		if (!no_edges_below_discrete_lower_bound_radius)
		{
			//discard BOTH edges from higher banded radius if ONE of them is in lower banded radius
			length_of_forward_edge_within_radius = 0;
			length_of_backward_edge_within_radius = 0;
		}

		//decide which edge(s) to return
		if (length_of_forward_edge_within_radius + length_of_backward_edge_within_radius < edge_length)
		{
			//none, one or both of the edges have parts needing to be analysed
			//but the parts don't overlap
			//(in discrete space, impossible to return both edges as they would overlap)
			if (length_of_forward_edge_within_radius > 0)
				destinations.push_back(DestinationSDNAPolylineSegment(forward_edge,length_of_forward_edge_within_radius));
			if (length_of_backward_edge_within_radius > 0)
				destinations.push_back(DestinationSDNAPolylineSegment(backward_edge,length_of_backward_edge_within_radius));
		}
		else
		{
			//either the parts overlap, or one of the parts (hence the entire link) is fully within the radius
			//push back either (so long as traversal is allowed) - the better cost will be selected later
			if (length_of_forward_edge_within_radius > 0)
				destinations.push_back(DestinationSDNAPolylineSegment(forward_edge,edge_length));
			else
			{
				assert(length_of_backward_edge_within_radius > 0);
				destinations.push_back(DestinationSDNAPolylineSegment(backward_edge,edge_length));
			}
		}
	}

	private:
		static double sum(vector<float> &v)
		{
			double sum = 0;
			for (vector<float>::iterator it=v.begin(); it!=v.end(); ++it)
				sum += *it;
			return sum;
		}
};

//used to generate a partial net for each radius from origin
class PartialNetCollection
{
private:
	IdIndexedArray<double,EdgeId> radial_best_costs_reaching_edge;
	IdIndexedArray<Edge *,EdgeId> radial_backlinks_edge;
	Net *net;
	SDNAPolyline *origin;
	vector<double> radii, lower_bound_radii;
	MetricEvaluator* radial_eval;
	MetricEvaluator* metric_eval;
	bool cont_space;
	JunctionCosts junction_radial_costs;

	//these may or may not be computed for all nets together:
	shared_ptr<IdIndexedArray<double,EdgeId>> shared_anal_best_costs_reaching_edge; 
	shared_ptr<IdIndexedArray<Edge *,EdgeId>> shared_anal_backlinks_edge; 

public:
	IdIndexedArray<Edge *,EdgeId> *get_radial_backlinks() { assert(radial_backlinks_edge.isInitialized()); return &radial_backlinks_edge;}

	PartialNetCollection(Net *net,SDNAPolyline *origin,vector<double> &radii,vector<double> &lbr,bool strict_network_cut,
		MetricEvaluator* radial_eval,bool cont,MetricEvaluator* metric_eval,prob_route_action_t probrouteaction,double handle_prob_routes_if_over)
		: radii(radii), lower_bound_radii(lbr), net(net), origin(origin), metric_eval(metric_eval), radial_eval(radial_eval), cont_space(cont)
	{
		radial_best_costs_reaching_edge.initialize(net->num_edges());
		junction_radial_costs.initialize(net->num_junctions());

		//make pointer to backlinks for sending to dijkstra
		IdIndexedArray<Edge *,EdgeId> *backlinks;
		if (probrouteaction==REROUTE)
		{
			//radial backlinks are needed
			radial_backlinks_edge.initialize(net->num_edges());
			backlinks = &radial_backlinks_edge;
		}
		else
			backlinks = NULL;

		PartialNet full_net(net,origin);

		double max_depth = *max_element(radii.begin(),radii.end());
		if (!strict_network_cut)
			//max search depth needs to be larger to account for permissible problem routes too
			max_depth *= handle_prob_routes_if_over;
		if (max_depth > GLOBAL_RADIUS) max_depth=GLOBAL_RADIUS;

		run_dijkstra(full_net,radial_best_costs_reaching_edge,radial_eval,max_depth,backlinks,&junction_radial_costs);

		if (!strict_network_cut)
		{
			//run analytical dijkstra for all partial nets together
			//on a net of size max_depth
			PartialNet max_radius_net(net,max_depth,0.,cont_space,radial_eval,&radial_best_costs_reaching_edge,&radial_backlinks_edge,origin,&junction_radial_costs);
			max_radius_net.compute_analytical_costs(metric_eval);
			shared_anal_best_costs_reaching_edge = max_radius_net.anal_best_costs_reaching_edge;
			shared_anal_backlinks_edge = max_radius_net.anal_backlinks_edge;
		}
	}

	PartialNet getPartialNet(long i)
	{
		PartialNet pn(net,radii[i],lower_bound_radii[i],cont_space,radial_eval,&radial_best_costs_reaching_edge,&radial_backlinks_edge,origin,&junction_radial_costs);
		if (shared_anal_best_costs_reaching_edge)
		{
			assert (shared_anal_backlinks_edge);
			pn.assign_analytical_costs(shared_anal_best_costs_reaching_edge,shared_anal_backlinks_edge);
		}
		else
			pn.compute_analytical_costs(metric_eval);
		return pn;
	}
};