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
using namespace std;

#define OUTPUT(x) output_map.add_output(ExtraNameWrapper((x),output_name_prefix,output_name_postfix))

void SDNAIntegralCalculation::assign_output_names()
{
	if (suppress_net_data)
		return;

	OUTPUT(SDNAPolylineConnectivityOutputDataWrapper());
	OUTPUT(SDNAPolylineLengthOutputDataWrapper());
	OUTPUT(PolylineIndexedArrayOutputDataWrapper("Link Fraction","LFrac",&link_fraction));
	OUTPUT(SDNAPolylineAngularCostOutputDataWrapper());
	OUTPUT(SDNAPolylineSinuosityOutputDataWrapper());
	OUTPUT(SDNAPolylineBearingOutputDataWrapper());
	if (analysis_evaluator_factory->descriptive_letter()=="H")
	{
		//we don't use RTTI in release versions but the if() above should ensure this is safe:
		OUTPUT(SDNAPolylineMetricOutputDataWrapper(static_cast<HybridMetricEvaluator*>(&*analysis_evaluator_factory),PLUS));
		OUTPUT(SDNAPolylineMetricOutputDataWrapper(static_cast<HybridMetricEvaluator*>(&*analysis_evaluator_factory),MINUS));
	}

	//make different names for euclidean/angular closeness and betweenness (and their short versions)
	string analysis_type_letter = analysis_evaluator_factory->descriptive_letter();
	string analysis_type_abbrev = analysis_evaluator_factory->abbreviation();
	string closeness_shortname = "S"+analysis_type_letter+"D";
	string closeness_name = "Sum "+analysis_type_abbrev+" Dist";

	string weighting_desc;
	if (origweightexpr || destweightexpr)
	{
		weighting_desc = "e";
		OUTPUT(PolylineIndexedArrayOutputDataWrapper("Line Origin Weight","LOrigWt",&origweightdata));
		OUTPUT(PolylineIndexedArrayOutputDataWrapper("Line Destination Weight","LDestWt",&destweightdata));
	}
	else
	{
		assert(origweightsource.get_weighting_type()==destweightsource.get_weighting_type());
		if (origweightsource.get_weighting_type()==LENGTH_WEIGHT)
			weighting_desc = "l";
		else if (origweightsource.get_weighting_type()==POLYLINE_WEIGHT)
			weighting_desc = "p";
	}
	
	for (unsigned int r=0;r<radii.size();r++)
	{
		if (output_sums)
			OUTPUT(RadialOutputDataWrapper(closeness_name,closeness_shortname,&closeness,r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));

		//the inner radial wrapper here is labelled mean_dist_wrapper when it is in fact total distance.  
		//The outer wrapper does not rename it, thus making the name correct (ouch)
		if (!output_decomposable_only)
		{
			RadialOutputDataWrapper mean_dist_wrapper("Mean "+analysis_type_abbrev+" Dist","M"+analysis_type_letter+"D",
				&closeness,r,radii,cont_space,weighting_desc,RAW_VALUE);
		
			OUTPUT(ControlledRadialOutputDataWrapper(mean_dist_wrapper,&total_weight));
		}
		
		OUTPUT(RadialOutputDataWrapper("NetQuantPD "+analysis_type_abbrev,"NQPD"+analysis_type_letter,&accessibility,
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
		
		//ugly polymorphism to handle different betweenness objects, but nicer to see all the code here
		assert(betweenness.is_bidirectional()==two_stage_betweenness.is_bidirectional());
		if (!betweenness.is_bidirectional())
		{
			OUTPUT(RadialOutputDataWrapper("Betweenness "+analysis_type_abbrev,"Bt"+analysis_type_letter,betweenness.get_output_facade_bidirectional(),
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
			OUTPUT(RadialOutputDataWrapper("TPBetweenness "+analysis_type_abbrev,"TPBt"+analysis_type_letter,two_stage_betweenness.get_output_facade_bidirectional(),
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
		}
		else
		{
			OUTPUT(RadialOutputDataWrapper("Betweenness Fwd "+analysis_type_abbrev,"BtF"+analysis_type_letter,betweenness.get_output_facade(PLUS),
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
			OUTPUT(RadialOutputDataWrapper("Betweenness Bwd "+analysis_type_abbrev,"BtB"+analysis_type_letter,betweenness.get_output_facade(MINUS),
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
			OUTPUT(RadialOutputDataWrapper("TPBetweenness Fwd "+analysis_type_abbrev,"TPBtF"+analysis_type_letter,two_stage_betweenness.get_output_facade(PLUS),
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
			OUTPUT(RadialOutputDataWrapper("TPBetweenness Bwd "+analysis_type_abbrev,"TPBtB"+analysis_type_letter,two_stage_betweenness.get_output_facade(MINUS),
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
		}

		OUTPUT(RadialOutputDataWrapper("TPDestination","TPD",&two_stage_dest_popularity,
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
		
		OUTPUT(RadialOutputDataWrapper("Links",         "Lnk", &total_num_links,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Length",        "Len", &total_link_length,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Ang Dist",      "AngD", &total_angular_cost,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Weight",	   "Wt", &total_weight,
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));

		OUTPUT(RadialOutputDataWrapper("Junctions",     "Jnc", &total_num_junctions,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Connectivity",  "Con", &total_connectivity,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		
		if (output_sums)
			OUTPUT(RadialOutputDataWrapper("SumGeoLen "+analysis_type_abbrev,"SGL"+analysis_type_letter,&total_geodesic_length_weighted,
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
		
		//the inner radial wrapper here is labelled MeanGeoLen when it is in fact GeoLen.  The outer wrapper does not rename it,
		//thus making the name correct (ouch)
		if (!output_decomposable_only)
			OUTPUT(ControlledRadialOutputDataWrapper(
						RadialOutputDataWrapper("MeanGeoLen "+analysis_type_abbrev,"MGL"+analysis_type_letter,&total_geodesic_length_weighted,
												r,radii,cont_space,weighting_desc,RAW_VALUE),
						&total_weight));
	
		if (output_sums)
			OUTPUT(RadialOutputDataWrapper("Sum Crow Flight","SCF", &total_crow_flies,
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
		
		//the inner radial wrapper here is labelled Mean when it is in fact Sum.  The outer wrapper does not rename it,
		//thus making the name correct (ouch)
		if (!output_decomposable_only)
			OUTPUT(ControlledRadialOutputDataWrapper(
						RadialOutputDataWrapper("Mean Crow Flight","MCF",&total_crow_flies,
												r,radii,cont_space,weighting_desc,RAW_VALUE),
						&total_weight));

		//the inner radial wrapper here is labelled Diversion Ratio when it is in fact sum of diversion ratios.  The outer wrapper does not rename it,
		//thus making the name correct (ouch)
		OUTPUT(ControlledRadialOutputDataWrapper(
						RadialOutputDataWrapper("Diversion Ratio "+analysis_type_abbrev,"Div"+analysis_type_letter, &diversion_ratio,
											r,radii,cont_space,weighting_desc,RAW_VALUE),
						&total_weight));

		OUTPUT(RadialOutputDataWrapper("Convex Hull Area",   "HullA",&convex_hull_area,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Convex Hull Perimeter","HullP", &convex_hull_perimeter,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Convex Hull Max Radius","HullR", &convex_hull_max_radius,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));
		OUTPUT(RadialOutputDataWrapper("Convex Hull Bearing","HullB", &convex_hull_bearing,
											r,radii,cont_space,"",TAKE_SAMPLE_MEANS));

		OUTPUT(HullShapeIndexWrapper(&convex_hull_area,&convex_hull_perimeter,r,radii,cont_space));

		OUTPUT(RadialOutputDataWrapper("Problem route weight","PrbW", &prob_route_weight,
											r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));

		OUTPUT(RadialOutputDataWrapper("Problem route excess","PrbE", &prob_route_excess,
										r,radii,cont_space,weighting_desc,TAKE_SAMPLE_MEANS));
	}
}