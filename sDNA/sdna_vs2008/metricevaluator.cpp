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
#include "metricevaluator.h"
#include "dataacquisitionstrategy.h"
#include "edge.h"
#include "calculation.h"
#include "random.h"

float JuncVariableSource::get_junction_data(const Edge * const prev,const Edge * const next)
{
	assert(initialized);
	switch(type)
	{
	case TEMP:
		return 0;
	case PREV:
		return das->get_data(prev->link);
	case NEXT:
		return das->get_data(next->link);
	default:
		assert(false);
		return 0;
	}
}

float LinkVariableSource::get_link_data(const Edge * const e)
{
	assert(initialized);
	if (is_temp)
		return 0;
	else
		return debe->get_data(e->link);
}

float CustomMetricEvaluator::evaluate_edge(TraversalEventAccumulator const& acc,const Edge * const e)
{
	const float total_cust_cost = strat->get_data(e->link);
	const float total_dist = e->full_cost_ignoring_oneway().euclidean;
	return (total_dist==0) ? total_cust_cost : total_cust_cost*acc.euclidean/total_dist;
}

float* HybridMetricEvaluator::staticvariablefactory(const char* name, void *instance)
{
	return ((HybridMetricEvaluator*)instance)->variablefactory(name);
}

float* HybridMetricEvaluator::staticjuncvariablefactory(const char* name, void *instance)
{
	return ((HybridMetricEvaluator*)instance)->juncvariablefactory(name);
}

float* HybridMetricEvaluator::variablefactory(const char* name)
{
	//assume the data will exist - add a dataacquisitionstrategy to wanted_data for the calculation
	//(and calculation will complain to the user if it doesn't)

	//do we have a strategy already?
	UserVariableSourceVector::iterator it;
	for (it=user_variable_sources.begin();it!=user_variable_sources.end();++it)
		if (it->getvarname()==name) break;

	int index;
	if (it!=user_variable_sources.end())
	{
		index = (int)(it-user_variable_sources.begin());
	}
	else
	{
		assert (OMP_THREAD==0);
		LinkVariableSource lvs;
		if (name[0]!='_')
		{
			//get data from network, zone table or zone sum - we don't know which yet
			shared_ptr<DataExpectedByExpression> d(new DataExpectedByExpression(name,net,calc));
			lvs = LinkVariableSource(name,d);
		}
		else
		{
			lvs = LinkVariableSource(name);
		}
		index = (int)user_variable_sources.size();
		if (index+1 > max_user_variables)
			throw mu::Parser::exception_type("Too many user variables in expression");
		user_variable_sources.push_back(lvs);
	}
	user_variables[index]=0;
	return &user_variables[index];
}



float* HybridMetricEvaluator::juncvariablefactory(const char* name_cstr)
{
	//assume the data will exist - add a dataacquisitionstrategy to wanted_data for the calculation
	//(and calculation will complain to the user if it doesn't)

	//do we have a strategy already?
	string name(name_cstr);
	UserJuncVariableSourceVector::iterator it;
	for (it=user_junc_variable_sources.begin();it!=user_junc_variable_sources.end();++it)
		if (it->getvarname()==name) break;

	int index;
	if (it!=user_junc_variable_sources.end())
	{
		index = (int)(it-user_junc_variable_sources.begin());
	}
	else
	{
		assert(OMP_THREAD==0);
		JuncVariableSource jvs;
		if (!(name[0]=='_'))
		{
			//get data from network link
			//name must begin with PREV or NEXT to specify which
			const char* junc_parse_error = "Junction expression variable doesn't begin with _, PREV or NEXT";
			if (name.length()<5)
				throw mu::Parser::exception_type(junc_parse_error);

			string prefix = name.substr(0,4);
			string dataname = name.substr(4);
			junc_data_t which;
			if (prefix=="PREV")
				which=PREV;
			else if (prefix=="NEXT")
				which=NEXT;
			else
				throw mu::Parser::exception_type(junc_parse_error);

			shared_ptr<NetExpectedDataSource<float>> d;
			d.reset(new NetExpectedDataSource<float>(dataname,net,calc->print_warning_callback));
			jvs = JuncVariableSource(name,which,d);
			calc->add_expected_data(&*d);
		}
		else
		{
			jvs = JuncVariableSource(name,TEMP);
		}
		index = (int)user_junc_variable_sources.size();
		if (index+1 > max_user_variables)
			throw mu::Parser::exception_type("Too many user variables in expression");
		user_junc_variable_sources.push_back(jvs);
	}
	user_junc_variables[index]=0;
	return &user_junc_variables[index];
}

float truncate(float value,float lower,float upper)
{
	if (value<lower)
		return lower;
	else 
	{
		if (value>upper)
			return upper;
		else
			return value;
	}
}

float safedivide(float numerator,float denominator)
{
	if (denominator==0)
	{
		if (numerator==0)
			return 0.f;
		else
			throw SDNARuntimeException("proportion() function returned infinity (something divided by zero)\nThis should never happen unless prop() has been misused");
	}
	else
		return numerator/denominator;
}

void HybridMetricEvaluator::set_buffer_pointers()
{
	assert(!buffers_set);
	parser.DefineVar("ang",&geometry_variables[GVAR_ANG]);
	parser.DefineVar("euc",&geometry_variables[GVAR_EUC]);
	parser.DefineVar("hg",&geometry_variables[GVAR_HG]);
	parser.DefineVar("hl",&geometry_variables[GVAR_HL]);
	parser.DefineVar("FULLang",&geometry_variables[GVAR_FULL_ANG]);
	parser.DefineVar("FULLeuc",&geometry_variables[GVAR_FULL_EUC]);
	parser.DefineVar("FULLhg",&geometry_variables[GVAR_FULL_HG]);
	parser.DefineVar("FULLhl",&geometry_variables[GVAR_FULL_HL]);
	parser.DefineVar("FULLlf",&geometry_variables[GVAR_FULL_LF]);
	parser.DefineVar("fwd",&geometry_variables[GVAR_FWD]);
	parser.DefineConst("inf",numeric_limits<float>::infinity());
	parser.DefineConst("pi",(float)M_PI);
	parser.DefineFun("randnorm",&randnorm,true);
	parser.DefineFun("randuni",&randuni,true);
	parser.DefineFun("proportion",&safedivide,true);
	parser.DefineFun("trunc",&truncate,true);
	parser.SetVarFactory(&staticvariablefactory,this);
	
	junction_parser.DefineVar("ang",&junction_turn_angle_variable);
	junction_parser.DefineConst("inf",numeric_limits<float>::infinity());
	junction_parser.DefineConst("pi",(float)M_PI);
	junction_parser.DefineFun("randnorm",&randnorm,true);
	junction_parser.DefineFun("randuni",&randuni,true);
	junction_parser.DefineFun("proportion",&safedivide,true);
	junction_parser.DefineFun("trunc",&truncate,true);
	junction_parser.SetVarFactory(&staticjuncvariablefactory,this);
	
	buffers_set = true;
}

HybridMetricEvaluator::HybridMetricEvaluator(string expression,string junc_expression,Net *net,SDNAIntegralCalculation *calc)
: net(net),calc(calc),creating_thread_number(OMP_THREAD),buffers_set(false),no_infinity_warning(true),
expression(expression), junc_expression(junc_expression)
{
	parser.SetExpr(expression);
	junction_parser.SetExpr(junc_expression);
	set_buffer_pointers();

	//run a trial evaluation to force binding of variables
	for (size_t i=0;i<NUM_GVAR_TYPES;i++)
		geometry_variables[i] = 0;
	try
	{
		parser.Eval();
	}
	catch (mu::Parser::exception_type &e)
	{
		calc->print_warning_callback("ERROR: in formula for link metric");
		calc->print_warning_callback(expression.c_str());
		calc->print_warning_callback(e.GetMsg().c_str());
		throw BadConfigException("Error in formula for link metric");
	}
	junction_turn_angle_variable = 0;
	try
	{
		junction_parser.Eval();
	}
	catch (mu::Parser::exception_type &e)
	{
		calc->print_warning_callback("ERROR: in formula for junction metric");
		calc->print_warning_callback(junc_expression.c_str());
		calc->print_warning_callback(e.GetMsg().c_str());
		throw BadConfigException("Error in formula for junction metric");
	}
}

bool HybridMetricEvaluator::test_linearity()
{
	bool linear = true;
	linear &= test_linearity_inner(2,0);
	linear &= test_linearity_inner(2,1);
	linear &= test_linearity_inner(3,0);
	linear &= test_linearity_inner(3,1);
	return linear;
}

bool HybridMetricEvaluator::test_linearity_inner(float val,float fwd)
{
	//set all net data to 2
	for (size_t index=0; index<user_variable_sources.size();++index)
		user_variables[index] = 2;
	//set geom data from val,fwd
	geometry_variables[GVAR_ANG] = val;
	geometry_variables[GVAR_EUC] = val;
	geometry_variables[GVAR_HG] = val;
	geometry_variables[GVAR_HL] = val;
	geometry_variables[GVAR_FULL_ANG] = val*3;
	geometry_variables[GVAR_FULL_EUC] = val*3;
	geometry_variables[GVAR_FULL_HG] = val*3;
	geometry_variables[GVAR_FULL_HL] = val*3;
	geometry_variables[GVAR_FWD] = fwd;
	float result1 = parser.Eval();
	geometry_variables[GVAR_ANG] = val*2;
	geometry_variables[GVAR_EUC] = val*2;
	geometry_variables[GVAR_HG] = val*2;
	geometry_variables[GVAR_HL] = val*2;
	float result2 = parser.Eval();
	return (result2 == result1*2);
}

float HybridMetricEvaluator::evaluate_edge_internal(TraversalEventAccumulator const& acc,const Edge * const e,bool warn_if_bad,bool provide_geometry_variables)
{
	if (!creating_thread_number==OMP_THREAD)
	{
		//not an assert as parallel version doesn't have them
		throw SDNARuntimeException("MetricEvaluator threading issue");
	}
	
	//copy required parser data to parser_variables
	for (UserVariableSourceVector::iterator it=user_variable_sources.begin();it!=user_variable_sources.end();++it)
	{
		const int index = (int)(it-user_variable_sources.begin());
		user_variables[index] = it->get_link_data(e);
	}
	//initialize geometry variables
	geometry_variables[GVAR_ANG] = acc.angular;
	geometry_variables[GVAR_EUC] = acc.euclidean;
	geometry_variables[GVAR_HG] = acc.height_gain;
	geometry_variables[GVAR_HL] = acc.height_loss;
	TraversalEventAccumulator full_edge;
	if (provide_geometry_variables)
		full_edge = e->full_cost_ignoring_oneway();
	geometry_variables[GVAR_FULL_ANG] = full_edge.angular;
	geometry_variables[GVAR_FULL_EUC] = full_edge.euclidean;
	geometry_variables[GVAR_FULL_HG] = full_edge.height_gain;
	geometry_variables[GVAR_FULL_HL] = full_edge.height_loss;
	geometry_variables[GVAR_FWD] = (e->direction==PLUS)?1.f:0.f;
	geometry_variables[GVAR_FULL_LF] = calc->get_link_fraction(e->link);
	float retval = numeric_limits<float>::quiet_NaN();
	try
	{
		retval = parser.Eval();
	}
	catch (mu::Parser::exception_type &exc)
	{
		stringstream s;
		s << "Formula evaluation failed for link " << e->link->arcid << endl;
		s << exc.GetMsg();
		if (warn_if_bad)
			throw SDNARuntimeException(s.str());
	}
	if (retval<0)
	{
		stringstream s;
		s << "Formula evaluation gave negative result for link " << e->link->arcid;
		if (warn_if_bad)
			throw SDNARuntimeException(s.str());
	}
	if (boost::math::isnan(retval))
	{
		stringstream s;
		s << "Formula evaluation gave NaN (not a number) result for link " << e->link->arcid << endl;
		s << "(This is usually the result of dividing zero by zero)";
		if (warn_if_bad)
			throw SDNARuntimeException(s.str());
	}
	if (no_infinity_warning && boost::math::isinf(retval))
	{
		no_infinity_warning = false;
		stringstream s;
		s << "WARNING: Formula evaluation gave infinite result for link " << e->link->arcid << endl;
		s << "(Further infinities may exist but may not be reported)";
		calc->print_warning_callback(s.str().c_str());
	}
	return retval;
}

float HybridMetricEvaluator::evaluate_junction(float turn_angle,const Edge * const prev,const Edge * const next)
{
	if (!creating_thread_number==OMP_THREAD)
	{
		//not an assert as parallel version doesn't have them
		throw SDNARuntimeException("MetricEvaluator threading issue");	
	}

	//only one geometry variable for a turn
	junction_turn_angle_variable = turn_angle;
	
	//copy required parser data to parser variables
	for (UserJuncVariableSourceVector::iterator it=user_junc_variable_sources.begin();it!=user_junc_variable_sources.end();++it)
	{
		const int index = (int)(it-user_junc_variable_sources.begin());
		user_junc_variables[index] = it->get_junction_data(prev,next);
	}
	
	float retval;
	try
	{
		retval = junction_parser.Eval();
	}
	catch (mu::Parser::exception_type &e)
	{
		stringstream s;
		s << "Formula evaluation failed for junction between lines " << prev->link->arcid << " and " << next->link->arcid << endl;
		s << e.GetMsg();
		throw SDNARuntimeException(s.str());
	}
	if (retval<0)
	{
		stringstream s;
		s << "Formula evaluation gave negative result for junction turn angle " << turn_angle;
		throw SDNARuntimeException(s.str());
	}
	if (boost::math::isnan(retval))
	{
		stringstream s;
		s << "Formula evaluation gave NaN (not a number) for junction turn angle " << turn_angle << endl;
		s << "(This is usually the result of dividing zero by zero)";
		throw SDNARuntimeException(s.str());
	}
	if (no_infinity_warning && boost::math::isinf(retval))
	{
		no_infinity_warning = false;
		stringstream s;
		s << "WARNING: Formula evaluation gave infinite result for junction turn angle " << turn_angle << endl;
		s << "(Further infinities may exist but may not be reported)";
		calc->print_warning_callback(s.str().c_str());
	}
	return retval;
}

shared_ptr<MetricEvaluator> MetricEvaluator::from_event_type(traversal_event_type t)
{
	shared_ptr<MetricEvaluator> result;
	switch (t)
	{
	case ANGULAR_TE:
		result.reset(new AngularMetricEvaluator());
		break;
	case EUCLIDEAN_TE:
		result.reset(new EuclideanMetricEvaluator());
		break;
	default:
		assert(false);
	}
	return result;
}

pair<TraversalEvent,TraversalEvent> EuclideanTE::split(float first_half_as_ratio) const
{
	const float d1 = distance * first_half_as_ratio;
	const float d2 = distance - d1;
	const float h1 = height_gain * first_half_as_ratio;
	const float h2 = height_gain - h1;
	return pair<TraversalEvent,TraversalEvent>(
		TraversalEvent(EuclideanTE(d1,h1)),
		TraversalEvent(EuclideanTE(d2,h2)));
}
pair<TraversalEvent,TraversalEvent> AngularTE::split(float first_half_as_ratio) const
{
	const float a1 = angle * first_half_as_ratio;
	const float a2 = angle - a1;
	return pair<TraversalEvent,TraversalEvent>(
		TraversalEvent(AngularTE(a1,p)),
		TraversalEvent(AngularTE(a2,p)));
}
//weird but these do actually get called by PartialEdge when it wants to return half an event
//return an identical event (same type and location):
pair<TraversalEvent,TraversalEvent> MidpointTE::split(float) const
{
	return pair<TraversalEvent,TraversalEvent>(*this,*this);
}
pair<TraversalEvent,TraversalEvent> EndpointTE::split(float) const
{
	return pair<TraversalEvent,TraversalEvent>(*this,*this);
}


DataExpectedByExpression::DataExpectedByExpression(string name,Net *net,SDNAIntegralCalculation *calculation) : name(name),net(net),calculation(calculation),
		datasource(UNKNOWN)
	{
		BOOST_FOREACH(ZoneSum &zs,calculation->zone_sum_evaluators)
		{
			if (zs.varname==name)
			{
				datasource=ZONESUM;
				zonesumtable = zs.table;
				break;
			}
		}
		if (datasource!=ZONESUM)
		{
			BOOST_FOREACH(shared_ptr<Table<float> > zdt,calculation->zonedatatables)
			{
				if (zdt->name()==name)
				{
					datasource=ZONE;
					zonedatatable = zdt;
					break;
				}
			}
			if (datasource==UNKNOWN)
			{
				datasource=NET;
				netdata.reset(new NetExpectedDataSource<float>(name,net,calculation->print_warning_callback));
				calculation->add_expected_data(&*netdata); 
			}
		}
		assert(datasource!=UNKNOWN);
	}

float DataExpectedByExpression::get_data(SDNAPolyline * p)
	{
		switch(datasource)
		{
		case UNKNOWN:
			assert(false);
			return -65535;
		case NET:
			return netdata->get_data(p);
		case ZONE:
			return zonedatatable->getData(p);
		case ZONESUM:
			return static_cast<float>(zonesumtable->getData(p));
		default:
			assert(false);
			return -65535;
		}
	}