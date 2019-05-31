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
#include "sdna_geometry_collections.h"
#include "tables.h"

#ifndef CALCULATION
#define CALCULATION

class DataExpectedByExpression;

struct ZoneSum
{
	string varname;
	shared_ptr<NetExpectedDataSource<string> > zone_das;
	shared_ptr<HybridMetricEvaluator> eval;
	shared_ptr<Table<long double> > table;
	ZoneSum(string varname,shared_ptr<NetExpectedDataSource<string> > zone_das,shared_ptr<HybridMetricEvaluator> eval,
		shared_ptr<Table<long double> > table)
		: varname(varname),zone_das(zone_das),eval(eval),table(table) {}
};

class Calculation
{
	friend class DataExpectedByExpression;
protected:
	EmergencyMemory emergencyMemory;

	Net *net;

	//bit ugly that these things are here, rather than in sdnaintegralcalculation
	//fixme consider refactor (not trivial as they are coupled with data initialization which belongs here, and maybe prepareoperation will want zone data one day)
	scoped_ptr<Table2d> od_matrix;
	vector<shared_ptr<Table<float> > > zonedatatables;
	vector<shared_ptr<Table<long double> > > zonesumtables;
	vector<ZoneSum > zone_sum_evaluators;

	//for 2d matrices only
	//these are here but they would also be tidier in integralcalculation 
	//it's ok so long as we only have one OD matrix. otherwise we will want to start handling 2d matrices like the 1d tables in calculation
	NetExpectedDataSource<string> origzonedas,destzonedas;

	double xytol,ztol;
	void set_tolerance(ConfigStringParser &config)
	{
		//tolerance order of precedence:
		//xytol,ztol override arcxytol,arcztol which override 0,0
		xytol = 0;
		ztol = 0;
		if (config.get_string("arcxytol")!="")
			xytol = config.get_double("arcxytol");
		if (config.get_string("arcztol")!="")
			ztol = config.get_double("arcztol");
		if (config.get_string("xytol")!="")
			xytol=config.get_double("xytol");
		if (config.get_string("ztol")!="") 
			ztol=config.get_double("ztol");
		stringstream s;
		s << "Using xytolerance=" << xytol << ", ztolerance=" << ztol;
		print_warning_callback(s.str().c_str());
	}
public:
	int (__cdecl *print_warning_callback)(const char*);

	Calculation(Net *net, int (__cdecl *print_warning_callback)(const char*)) 
		: net(net),
		  print_warning_callback(print_warning_callback),
		  xytol(-1), ztol(-1)
	{
	} 
	bool add_table2d(Table2d *t) //currently only allows one table2d but could change in future
	{
		if (od_matrix)
			return false;
		else
		{
			od_matrix.reset(t); //takes ownership
			origzonedas = NetExpectedDataSource<string>(od_matrix->origfieldname(),net,print_warning_callback);
			destzonedas = NetExpectedDataSource<string>(od_matrix->destfieldname(),net,print_warning_callback);
			od_matrix->setNameDas(&origzonedas,&destzonedas);
			add_expected_data(&origzonedas);
			add_expected_data(&destzonedas);
			return true;
		}
	}
	void add_expected_data_for_table_zonefields()
	{
		BOOST_FOREACH(shared_ptr<Table<float>> t,zonedatatables)
		{
			shared_ptr<NetExpectedDataSource<string> > zonedas = shared_ptr<NetExpectedDataSource<string> >(
				new NetExpectedDataSource<string>(t->zonefieldname(),net,print_warning_callback));
			t->setNameDas(zonedas);
			add_expected_data(&*zonedas);
		}
	}
	//data that is registered with these methods will be
	//(1) notified to the client via expected_data() unless it's a table already
	//(2) bound to the data provided by bind_network_data() once calculation begins
	void add_expected_data(NetExpectedDataSource<float> *n) 
	{
		bool matching_table = false;
		BOOST_FOREACH(shared_ptr<Table<float>> zdt,zonedatatables)
			if (zdt->name()==n->get_name())
				matching_table = true; 
		if (!matching_table)
			expecteddata_netonly.push_back(n);
	}
	void add_expected_data(NetExpectedDataSource<string> *n) { expectedtextdata.push_back(n); }
	template <class T> void add_expected_data(vector<T> &v)
	{
		for (vector<T>::iterator it=v.begin();it!=v.end();it++)
			add_expected_data(&*it);
	}
	Net* getNet() {return net;}
private:
	vector<NetExpectedDataSource<float>*> expecteddata_netonly; //built during config
	vector<NetExpectedDataSource<string>*> expectedtextdata; //built during config
	OutputStringArray expected_data_net_only_names_buffer; //buffer created here
	OutputStringArray expected_text_data_names_buffer; //buffer created here
protected:
	vector<NetExpectedDataSource<float>*> expecteddata_netonly_nodupl() { return simplify_expected_data(expecteddata_netonly);}
	vector<NetExpectedDataSource<string>*> expectedtextdata_nodupl() { return simplify_expected_data(expectedtextdata);}
private:
	template <typename T> vector<NetExpectedDataSource<T>* > simplify_expected_data(vector<NetExpectedDataSource<T>* > &source)
	{
		//remove duplicates but keep in order they appear in source
		set<string> already_seen;
		vector<NetExpectedDataSource<T>* > strategies;
		for (vector<NetExpectedDataSource<T>*>::iterator it=source.begin();it!=source.end();it++)
			if ((*it)->is_compulsory() && already_seen.find((*it)->get_name())==already_seen.end())
			{
				already_seen.insert((*it)->get_name());
				strategies.push_back(*it);
			}
		return strategies;
	}
	template <typename T> size_t expected_data_to_string(char*** names,OutputStringArray &internal_buffer,vector<NetExpectedDataSource<T>* > &source)
	{
		internal_buffer.clear();
		vector<NetExpectedDataSource<T>* > simplified_data = simplify_expected_data(source);
		for (vector<NetExpectedDataSource<T>*>::const_iterator it=simplified_data.begin();it!=simplified_data.end();it++)
			internal_buffer.add_string((*it)->get_name());
		*names = internal_buffer.get_string_array();
		return internal_buffer.size();
	}
public:
	size_t expected_data_net_only(char ***names) 
	{
		return expected_data_to_string(names,expected_data_net_only_names_buffer,expecteddata_netonly);
	}
	size_t expected_text_data(char ***names) { return expected_data_to_string(names,expected_text_data_names_buffer,expectedtextdata);}
public:
	bool bind_network_data() //boolean returns success of execution
	{
		for (vector<NetExpectedDataSource<float>*>::iterator it=expecteddata_netonly.begin();it!=expecteddata_netonly.end();it++)
			if (!(*it)->init()) return false;
		for (vector<NetExpectedDataSource<string>*>::iterator it=expectedtextdata.begin();it!=expectedtextdata.end();it++)
			if (!(*it)->init()) return false;
		return true;
	}
protected:
	void set_net_gs_data(ConfigStringParser &config)
	{
		assert(net->start_gsdata.is_set() == net->end_gsdata.is_set());
		
		if (config.get_bool("preserve_net_config"))
		{
			assert(net->start_gsdata.is_set());
		}
		else
		{
			string start_gsname = config.get_string("start_gs");
			string end_gsname = config.get_string("end_gs");
			if (start_gsname.empty() != end_gsname.empty())
				throw BadConfigException("ERROR: If one of start/end grade separation is specified the other must be given too.");
			net->set_gs_data(
				NetExpectedDataSource<float>(start_gsname,0,net,"start_gs",print_warning_callback),
				NetExpectedDataSource<float>(end_gsname,0,net,"end_gs",print_warning_callback)
			);
		}
		add_expected_data(&net->start_gsdata);
		add_expected_data(&net->end_gsdata);
	}
	virtual bool run_internal()=0;

public:
	bool run()
	{
		if (!net)
		{
			print_warning_callback("EROOR: invalid net");
			return false;
		}
		#ifdef _AMD64_
				print_warning_callback("sDNA is running in 64-bit mode");
		#else
				print_warning_callback("sDNA is running in 32-bit mode");
		#endif
		try
		{
			return run_internal();
		}
		catch (std::bad_alloc&)
		{
			emergencyMemory.free();
			print_warning_callback("ERROR: Out of memory.  Calculation aborted.");
			return false;
		}
	}
protected:
	vector<sDNAGeometryCollectionBase*> geometry_outputs;
public:
	size_t get_num_geometry_outputs() { return geometry_outputs.size(); }
	sDNAGeometryCollectionBase * get_geometry_output(size_t index) { return geometry_outputs[index]; }

	virtual ~Calculation() {};

	friend class CalcExpectedDataSDNAPolylineDataSourceWrapper;
};

class CalcExpectedDataSDNAPolylineDataSourceWrapper : public SDNAPolylineDataSource
{
private:
	Calculation *c;
	vector<NetExpectedDataSource<string>* > textdata;
	vector<NetExpectedDataSource<float>* > netdata;
	vector<FieldMetaData> metadata;
	
public:
	CalcExpectedDataSDNAPolylineDataSourceWrapper(Calculation *calc) : c(calc)
	{
		//simplify and store NetExpectedDataSources as there may be duplicates (e.g. when one variable serves two purposes)
		//(zonedata and zonesums aren't subject to this problem)
		//we store strategies to save simplifying for every data line retrieved
		BOOST_FOREACH(NetExpectedDataSource<string>* netd,calc->simplify_expected_data(calc->expectedtextdata))
			textdata.push_back(netd);

		BOOST_FOREACH(NetExpectedDataSource<float>* neds,calc->simplify_expected_data(calc->expecteddata_netonly))
			netdata.push_back(neds);
	}
	CalcExpectedDataSDNAPolylineDataSourceWrapper() : c(NULL) {}
	virtual size_t get_output_length() {assert(c); return textdata.size() + netdata.size() + c->zonedatatables.size() + c->zonesumtables.size();}
	virtual vector<FieldMetaData> get_field_metadata() 
	{
		assert(c); 
		vector<FieldMetaData> result;
		BOOST_FOREACH(NetExpectedDataSource<string>* netd,textdata)
			result.push_back(FieldMetaData(fSTRING,netd->get_name()));
		BOOST_FOREACH(NetExpectedDataSource<float>* netd,netdata)
			result.push_back(FieldMetaData(fFLOAT,netd->get_name()));
		BOOST_FOREACH(shared_ptr<Table<float>> zdt,c->zonedatatables)
			result.push_back(FieldMetaData(fFLOAT,zdt->name()));
		BOOST_FOREACH(shared_ptr<Table<long double>> zst,c->zonesumtables)
			result.push_back(FieldMetaData(fFLOAT,zst->name()));
		metadata=result; //for debug
		return result;
	}
	virtual vector<SDNAVariant> get_outputs(long arcid)
	{
		assert(c); 
		SDNAPolyline *link = c->net->link_container[arcid];
		vector<SDNAVariant> outputs;
		outputs.reserve(get_output_length());
		size_t index=0;
		
		BOOST_FOREACH ( NetExpectedDataSource<string>* s , textdata)
		{
			outputs.push_back(SDNAVariant(s->get_data(link)));
			assert(s->get_name()==metadata[index++].name);
		}
		
		BOOST_FOREACH ( NetExpectedDataSource<float>* s , netdata)
		{
			outputs.push_back(SDNAVariant(s->get_data(link)));
			assert(s->get_name()==metadata[index++].name);
		}
		BOOST_FOREACH(shared_ptr<Table<float>> zdt,c->zonedatatables)
		{
			outputs.push_back(zdt->getData(link));
			assert(zdt->name()==metadata[index++].name);
		}
		BOOST_FOREACH(shared_ptr<Table<long double>> zst,c->zonesumtables)
		{
			outputs.push_back(static_cast<float>(zst->getData(link)));
			assert(zst->name()==metadata[index++].name);
		}
		return outputs;
	}
	virtual Net* getNet() { assert(c); return c->net;}
};

		
#endif