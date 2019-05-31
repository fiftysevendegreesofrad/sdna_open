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
#include "point.h"
#include "dataacquisitionstrategy.h"

#pragma once
using namespace std;
using namespace boost;

enum traversal_event_type {ANGULAR_TE, EUCLIDEAN_TE, ENDPOINT_TE, CENTRE_TE};

struct Edge;
template <typename T> class NetExpectedDataSource;
class Net;
class Calculation;
class SDNAIntegralCalculation;
template <class T> class Table;

class DataExpectedByExpression //data that an expression expects to appear
{
	//take a name, net, calculation
	//on calculation create, see whether this will be a zonesum or table (in order created so can't reference selves), 
	//if so handle, otherwise add to net expected data 

	//in an ideal world, this behaviour would all exist inside NetExpectedDataSource thus allowing all data to be tables if desired
	
	enum expr_data_source_t {UNKNOWN,NET,ZONE,ZONESUM};
	expr_data_source_t datasource;
	Net *net;
	SDNAIntegralCalculation *calculation;
	string name;

	shared_ptr<Table<float>> zonedatatable;
	shared_ptr<Table<long double>> zonesumtable;
	shared_ptr<NetExpectedDataSource<float>> netdata;
public:

	DataExpectedByExpression(string name,Net *net,SDNAIntegralCalculation *calculation);
	float get_data(SDNAPolyline * p);
};

//these only evaluate a single edge
//accumulation and evaluation are decoupled.  this may look inefficient as it results in accumulating what we don't need.
//the alternative however would require frequently constructing objects of unknown size.  Unless we got into policies.
//it does mean we can't have stateful metric evaluators, e.g. summing turn radius or cyclist speed, but that's another whole can of worms
struct TraversalEventAccumulator
{
	float angular, euclidean, height_gain, height_loss;
	TraversalEventAccumulator() : angular(0), euclidean(0), height_gain(0), height_loss(0) {}
	//semantically equivalent to the constructor, but used for clarity
	static TraversalEventAccumulator zero() {return TraversalEventAccumulator();}
	//never have a use for negative values here so we can use them to mark invalid data
	void set_bad() {angular=-1;}
	bool is_bad() {return (angular==-1);}
	TraversalEventAccumulator get_reversed_version()
	{ 
		TraversalEventAccumulator retval(*this);
		std::swap(retval.height_gain,retval.height_loss);
		return retval;
	}
	bool operator==(const TraversalEventAccumulator &other) const
	{
		return (angular==other.angular && euclidean==other.euclidean && height_gain==other.height_gain && height_loss==other.height_loss);
	}
};

class MetricEvaluator
{
public:
	virtual float evaluate_edge(TraversalEventAccumulator const& acc,const Edge * const e) = 0;
	virtual float evaluate_junction(float turn_angle,const Edge * const prev,const Edge * const next) = 0;
	virtual string descriptive_letter() = 0;
	virtual string abbreviation() = 0;
	static boost::shared_ptr<MetricEvaluator> from_event_type(traversal_event_type t);

	virtual MetricEvaluator* clone() = 0;
	virtual ~MetricEvaluator() {}
};

//allows copying MetricEvaluator into omp parallel region.  
//duplicates original evaluator and takes ownership of clones.
class MetricEvaluatorCopyableWrapper
{
	boost::scoped_ptr<MetricEvaluator> me;
public:
	MetricEvaluatorCopyableWrapper(boost::scoped_ptr<MetricEvaluator> &m) : me(m->clone()) {}
	MetricEvaluatorCopyableWrapper(MetricEvaluatorCopyableWrapper const& other)
		: me(other.me->clone()) {}
	boost::scoped_ptr<MetricEvaluator>& get() { return me; }
private:
	MetricEvaluatorCopyableWrapper& operator= (const MetricEvaluator &other);
};

class AngularMetricEvaluator : public MetricEvaluator
{
public:
	virtual float evaluate_edge(TraversalEventAccumulator const& acc,const Edge * const) {return acc.angular;}
	virtual float evaluate_junction(float turn_angle,const Edge * const prev,const Edge * const next) {return turn_angle;}
	virtual string descriptive_letter() { return "A";}
	virtual string abbreviation() { return "Ang";}
	virtual MetricEvaluator* clone() { return new AngularMetricEvaluator(*this);}
};

class EuclideanMetricEvaluator : public MetricEvaluator
{
public:
	virtual float evaluate_edge(TraversalEventAccumulator const& acc,const Edge * const) {return acc.euclidean;}
	virtual float evaluate_junction(float turn_angle,const Edge * const prev,const Edge * const next) {return 0;}
	virtual string descriptive_letter() {return "E";}
	virtual string abbreviation() {return "Euc";}
	virtual MetricEvaluator* clone() { return new EuclideanMetricEvaluator(*this);}
};

class CustomMetricEvaluator : public MetricEvaluator
{
	NetExpectedDataSource<float> *strat;
public:
	CustomMetricEvaluator(NetExpectedDataSource<float> *strat) : strat(strat) {}
	virtual float evaluate_edge(TraversalEventAccumulator const& acc,const Edge * const e);
	virtual float evaluate_junction(float turn_angle,const Edge * const prev,const Edge * const next) {return 0;}
	virtual string descriptive_letter() {return "C";}
	virtual string abbreviation() {return "Cust";}
	virtual MetricEvaluator* clone() { return new CustomMetricEvaluator(*this);}
};

enum junc_data_t {PREV,NEXT,TEMP};

struct JuncVariableSource
{
	boost::shared_ptr<NetExpectedDataSource<float> > das;
	junc_data_t type;
	bool initialized;
	string varname;
	JuncVariableSource() : initialized(false) {}
	JuncVariableSource(string vn,junc_data_t t,boost::shared_ptr<NetExpectedDataSource<float> > data) 
		: das(data), type(t), initialized(true), varname(vn)
	{
		assert (t==PREV || t==NEXT);
	}
	JuncVariableSource(string vn,junc_data_t t) 
		: type(t), initialized(true), varname(vn)
	{
		assert (t==TEMP);
	}
	float get_junction_data(const Edge * const prev,const Edge * const next);
	string getvarname()
	{
		assert(initialized);
		return varname;
	}
};

struct LinkVariableSource
{
	//fixme this is very similar to DataExpectedByExpression - perhaps a needless layer of abstraction
	shared_ptr<DataExpectedByExpression> debe;
	bool is_temp;
	bool initialized; //NB this is only false if called with 1st constructor 
	string varname;
	LinkVariableSource() : initialized(false) {}
	LinkVariableSource(string vn) : initialized(true), is_temp(true), varname(vn) {}
	LinkVariableSource(string vn,shared_ptr<DataExpectedByExpression> debe) : initialized(true), is_temp(false), debe(debe), varname(vn) {}
	float get_link_data(const Edge * const e);
	string getvarname()
	{
		assert (initialized);
		return varname;
	}
};

class HybridMetricEvaluator : public MetricEvaluator
{
	mu::Parser parser, junction_parser;
	Net *net;
	SDNAIntegralCalculation *calc;
	string expression,junc_expression;
	enum geom_var_t {GVAR_ANG,GVAR_EUC,GVAR_HG,GVAR_HL,GVAR_FULL_ANG,GVAR_FULL_EUC,GVAR_FULL_HG,GVAR_FULL_HL,GVAR_FWD,GVAR_FULL_LF,NUM_GVAR_TYPES};
	float geometry_variables[NUM_GVAR_TYPES];
	float junction_turn_angle_variable;
	static const int max_user_variables = 100;
	float user_variables[max_user_variables];
	float user_junc_variables[max_user_variables];
	typedef vector<LinkVariableSource> UserVariableSourceVector;
	typedef vector<JuncVariableSource> UserJuncVariableSourceVector;
	UserVariableSourceVector user_variable_sources;
	UserJuncVariableSourceVector user_junc_variable_sources;
	int creating_thread_number;
	bool buffers_set, no_infinity_warning;
	
	void set_buffer_pointers();
	HybridMetricEvaluator& operator=(const HybridMetricEvaluator&) {assert(false);}
	bool test_linearity_inner(float val,float fwd);

public:
	bool test_linearity();
	HybridMetricEvaluator(string expression,string junc_expression,Net *net,SDNAIntegralCalculation *calc);
	static float* staticvariablefactory(const char*,void*);
	static float* staticjuncvariablefactory(const char*,void*);
	float* variablefactory(const char*);
	float* juncvariablefactory(const char*);
	float evaluate_edge_internal(TraversalEventAccumulator const& acc,const Edge * const e,bool warn_if_bad=true,bool provide_geometry_variables=true);
	
	virtual float evaluate_edge(TraversalEventAccumulator const& acc,const Edge * const e)
		{ return evaluate_edge_internal(acc,e); }
	virtual float evaluate_junction(float turn_angle,const Edge * const prev, const Edge * const next);

	float evaluate_edge_no_safety_checks(TraversalEventAccumulator const& acc,const Edge * const e)
		{ return evaluate_edge_internal(acc,e,false); }
	float evaluate_edge_no_geometry(const Edge * const e) 
		{return evaluate_edge_internal(TraversalEventAccumulator(),e,true,false);}
	
	//private copy constructor gives a version that can be used by another thread
	//can copy all vars but parsers must be reinitialized with new pointers to new buffers user_variables, user_junc_variables
	//user_variable_sources and user_junc_variable_sources keep smart pointers to the same strategies though
private:
	HybridMetricEvaluator(const HybridMetricEvaluator &other)
	{
		creating_thread_number = OMP_THREAD;
		net = other.net;
		calc = other.calc;
		expression = other.expression;
		junc_expression = other.junc_expression;
		user_variable_sources = other.user_variable_sources;
		user_junc_variable_sources = other.user_junc_variable_sources;
		buffers_set = false;
		parser.SetExpr(expression);
		junction_parser.SetExpr(junc_expression);
		
		set_buffer_pointers();

		//call Eval to bind variables before data is needed for real
		//should not throw exception as this is a copy of a working expression:
		//(the only way i think they might is if some input data ==0?)
		parser.Eval();
		junction_parser.Eval();
	}
public:
	virtual MetricEvaluator* clone() { return new HybridMetricEvaluator(*this); }

	virtual string descriptive_letter() {return "H";}
	virtual string abbreviation() {return "Hybrid";}
};

////////////////////////////////////////
// ALL THE TRAVERSAL EVENT CLASSES
// IN A VARIANT NOT INHERITANCE HIERARCHY FOR EFFICIENCY OF STORAGE/ACCESS

class TraversalEvent;
enum polarity {PLUS, MINUS};

class EuclideanTE
{
	float distance,height_gain;
public:
	EuclideanTE(float d,float h) : distance(d), height_gain(h) { assert (height_gain <= distance); }
	EuclideanTE(Point *p1,Point *p2)
	{
		const Point vec = *p2-*p1;
		height_gain = vec.z;
		distance = vec.length();
		assert (height_gain <= distance);
	}
	void add_to_accumulator(TraversalEventAccumulator *acc, polarity direction) const
	{
		acc->euclidean += distance;
		const bool uphill = (height_gain>0)==(direction==PLUS);
		const float height_difference = abs(height_gain);
		if (uphill)
			acc->height_gain += height_difference;
		else
			acc->height_loss += height_difference;
	}
	traversal_event_type type() const {return EUCLIDEAN_TE;} 
	Point* physical_location() const {return NULL;} 
	void operator+=(const EuclideanTE& other)
	{
		distance += other.distance;
		height_gain += other.height_gain;
	}
	bool empty_costs() const { return distance==0; }
	pair<TraversalEvent,TraversalEvent> split(float second_half_as_ratio) const;
	string toString() const
	{
		stringstream retval;
		retval << "m" << distance;
		if (height_gain!=0)
			retval << "(h" << height_gain << ")";
		return retval.str();
	}
};
class AngularTE
{
	float angle;
	Point *p;
public:
	AngularTE(float a,Point *p) : angle(a), p(p) { assert (p!=NULL);}
	void add_to_accumulator(TraversalEventAccumulator *acc,polarity) const
	{
		acc->angular += angle;
	}
	traversal_event_type type() const {return ANGULAR_TE;} 
	Point* physical_location() const {return p;} 
	void operator+=(const AngularTE& other)
	{
		assert(*p==*other.p);
		angle += other.angle;
	}
	bool empty_costs() const {return angle==0;}
	pair<TraversalEvent,TraversalEvent> split(float second_half_as_ratio) const;
	string toString() const
	{
		stringstream retval;
		retval << "a" << angle << p->toString();
		return retval.str();
	}
};
class MidpointTE
{
	Point *p;
public:
	MidpointTE(Point *p) : p(p) {}
	void add_to_accumulator(TraversalEventAccumulator *acc,polarity) const { assert (p!=NULL);} 
	traversal_event_type type() const {return CENTRE_TE;} 
	Point* physical_location() const {return p;} 
	bool empty_costs() const {return false;}
	pair<TraversalEvent,TraversalEvent> split(float) const;
	string toString() const
	{
		return "c"+p->toString();
	}
};
class EndpointTE
{
	Point *p;
public:
	EndpointTE(Point *p) : p(p) {}
	void add_to_accumulator(TraversalEventAccumulator *acc,polarity) const { assert(p!=NULL);} 
	traversal_event_type type() const {return ENDPOINT_TE;} 
	Point* physical_location() const {return p;} 
	bool empty_costs() const {return false;}
	pair<TraversalEvent,TraversalEvent> split(float) const;
	string toString() const
	{
		return "e"+p->toString();
	}
};
class TraversalEvent
{
private:
	boost::variant<EuclideanTE,AngularTE,MidpointTE,EndpointTE> data;
	class AddToAccumVisitor : public boost::static_visitor<>
	{
		TraversalEventAccumulator* const acc;
		polarity direction;
	public:
		AddToAccumVisitor(TraversalEventAccumulator *acc,polarity d) : acc(acc),direction(d) {}
		template <typename T> void operator()(T const& traversalevent) const
			{ traversalevent.add_to_accumulator(acc,direction); }
	};
	class SplitVisitor : public boost::static_visitor<pair<TraversalEvent,TraversalEvent> >
	{
		const float second_half_cost;
	public:
		SplitVisitor(float shc) : second_half_cost(shc) {}
		template <typename T> pair<TraversalEvent,TraversalEvent> operator()(T const& traversalevent) const
			{ return traversalevent.split(second_half_cost); }
	};
	class GetTypeVisitor : public boost::static_visitor<typename traversal_event_type>
	{
	public:
		template <typename T> traversal_event_type operator()(T const& traversalevent) const
			{ return traversalevent.type(); }
	};
	class EmptyCostsVisitor : public boost::static_visitor<bool>
	{
	public:
		template <typename T> bool operator()(T const& traversalevent) const
			{ return traversalevent.empty_costs(); }
	};
	class GetLocationVisitor : public boost::static_visitor<typename Point*>
	{
	public:
		template <typename T> Point* operator()(T const& traversalevent) const
			{ return traversalevent.physical_location(); }
	};
	class ToStringVisitor : public boost::static_visitor<typename string>
	{
	public:
		template <typename T> string operator()(T const& traversalevent) const
			{ return traversalevent.toString(); }
	};
	
public:
	void add_to_accumulator(TraversalEventAccumulator *acc,polarity direction) const
		{ boost::apply_visitor(AddToAccumVisitor(acc,direction),data); }
	TraversalEventAccumulator costs(polarity direction) const
	{
		TraversalEventAccumulator retval;
		add_to_accumulator(&retval,direction);
		return retval;
	}
	traversal_event_type type() const
		{ return boost::apply_visitor(GetTypeVisitor(),data);}
	Point* physical_location() const
		{ return boost::apply_visitor(GetLocationVisitor(),data);}
	string toString() const
		{ return boost::apply_visitor(ToStringVisitor(),data);}
	void operator+=(const TraversalEvent& other)
	{
		switch(type())
		{
			//other type must be same as this one so indirection happens here not with visitor
			//boost::get returns null if type is wrong so that will throw null pointer access
		case EUCLIDEAN_TE:
			*(boost::get<EuclideanTE>(&data)) += *(boost::get<EuclideanTE>(&other.data));
			break;
		case ANGULAR_TE:
			*(boost::get<AngularTE>(&data)) += *(boost::get<AngularTE>(&other.data));
			break;
		default:
			assert(false);
		}
	}
	bool empty_costs()
		{ return boost::apply_visitor(EmptyCostsVisitor(),data);}
	pair<TraversalEvent,TraversalEvent> split_te(float first_half_as_ratio)
		{ return boost::apply_visitor(SplitVisitor(first_half_as_ratio),data);}
		
	template <typename T> TraversalEvent(T te) : data(te) {}
};
