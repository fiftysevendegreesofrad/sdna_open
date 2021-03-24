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
#include "calculationbase.h"
#pragma once 

typedef pair<JunctionMapKey,float> SplitSDNAPolylineIdentifier;
typedef vector<SplitSDNAPolylineIdentifier> SplitLinkVector;

typedef pair<vector<Point>,pair<float,float> > hash_t;

class PrepareOperation : public Calculation
{
private:
	sDNAGeometryCollection errors;

	OutputBuffer output_buffer_1,output_buffer_2;
	OutputStringArray output_string_array;
	
	SplitLinkVector get_split_link_junction_keys(unsigned long max_ids=LONG_MAX);
	vector<long> get_split_link_ids(unsigned long max_ids=LONG_MAX);
	void fix_split_link(JunctionMapKey &key,float gradesep);
	vector<SDNAPolyline*> get_traffic_islands();
	SDNAPolyline* straighten_link_remove_weight_cost(SDNAPolyline *s);
	void merge_junctions(vector<JunctionMapKey> &junctions);
	void move_junction(JunctionMapKey &key, double new_x, double new_y, float new_z);
	void move_link_endpoint(SDNAPolyline* link, junction_option_type end, double new_x, double new_y, float new_z);
	void get_duplicate_links(vector<SDNAPolyline*> &duplicates, vector<SDNAPolyline*> &originals,
		bool partial_search = false, vector<SDNAPolyline*> &subset = vector<SDNAPolyline*>());
	hash_t get_link_hash(SDNAPolyline *s);
	bool all_enforced_data_identical(SDNAPolyline *s1,SDNAPolyline *s2);
	
	typedef vector<shared_ptr<vector<SDNAPolyline*> > > subsystem_group;
	subsystem_group get_subsystems();
	shared_ptr<vector<SDNAPolyline*> > flood_fill(set<SDNAPolyline*> &unreached,SDNAPolyline *start_link);
	
	//for internally generated links
	long next_new_arcid;
	long get_new_arcid() { return next_new_arcid--; }

	NetExpectedDataSource<float> islanddata;
	vector<NetExpectedDataSource<float>> islandfieldstozero;
	vector<LengthWeightingStrategy> datatokeep;
	vector<NetExpectedDataSource<string>> textdatatokeep;
	vector<NetExpectedDataSource<float>*> enforce_identical_numeric_data;
	vector<NetExpectedDataSource<string>*> enforce_identical_text_data;

	SDNAPolylineDataSourceGeometryCollectionWrapper netwrapper;
	CalcExpectedDataSDNAPolylineDataSourceWrapper inputdatawrapper;
	
public:
	//only kept public for autocad now - deprecated - could be private for new mechanism
	size_t get_split_link_ids(long **output);
	size_t fix_split_links();
	size_t get_traffic_islands(long **output);
	size_t fix_traffic_islands();
	size_t get_duplicate_links(long **duplicates,long **originals);
	size_t fix_duplicate_links();
	size_t get_subsystems(char **message,long **links);
	size_t fix_subsystems();
	size_t get_near_miss_connections(long **links);
	size_t fix_near_miss_connections();
	Net* import_from_link_unlink_format();
	
	//new mechanism: all operations conducted inside run_internal, governed by the following params:
	bool check; // check errors if true, fix if false
	bool handle_near_misses, handle_traffic_islands, handle_duplicates, handle_isolated, handle_split_links;
	bool prepare_no_operation; // overrides above and does nothing except initialize data - useful for debug. differs from internal interface which expects run() not to be called and doesn't init data

	PrepareOperation(Net *net, char *configstring,int (__cdecl *print_warning_callback)(const char*)) 
		: next_new_arcid(-1), 
		Calculation(net,print_warning_callback)
	{
		ConfigStringParser config(//allowable keywords
								  "start_gs;end_gs;island;islandfieldstozero;data_unitlength;data_absolute;data_text;preserve_net_config;arcxytol;arcztol;xytol;ztol;"\
								  "action;nearmisses;trafficislands;duplicates;isolated;splitlinks;internal_interface;null;merge_if_identical",
								  //default values for those keywords if unspecified by user (0 is false for booleans)
								  "start_gs=;end_gs=;island=;islandfieldstozero=;data_unitlength=;data_absolute=;data_text=;merge_if_identical=;preserve_net_config=0;arcxytol=;arcztol=;"\
								  "action=detect;nearmisses=0;trafficislands=0;duplicates=0;isolated=0;splitlinks=0;internal_interface=0;"\
								  "xytol=;ztol=;null=0",
							      configstring);

		islanddata = NetExpectedDataSource<float>(config.get_string("island"),0,net,"island",print_warning_callback);
		add_expected_data(&islanddata);

		set_tolerance(config);
		set_net_gs_data(config);

		vector<string> absolutedatanames = config.get_vector("data_absolute");
		vector<string> perunitlengthdatanames = config.get_vector("data_unitlength");
		vector<string> textdatanames = config.get_vector("data_text");
		vector<string> islandfieldstozeronames = config.get_vector("islandfieldstozero");
		vector<string> mergeifidenticalnames = config.get_vector("merge_if_identical");
		
		for (vector<string>::iterator it=islandfieldstozeronames.begin();it!=islandfieldstozeronames.end();it++)
		{
			islandfieldstozero.push_back(NetExpectedDataSource<float>(*it,net,print_warning_callback));
			if (find(perunitlengthdatanames.begin(),perunitlengthdatanames.end(),*it)==perunitlengthdatanames.end()
				&& find(absolutedatanames.begin(),absolutedatanames.end(),*it)==absolutedatanames.end())
			{
				print_warning_callback(string("Assuming data field '"+*it+"' is absolute (unspecified in config).").c_str());
				absolutedatanames.push_back(*it);
			}
		}
		add_expected_data(islandfieldstozero);

		for (vector<string>::iterator it=perunitlengthdatanames.begin();it!=perunitlengthdatanames.end();it++)
			datatokeep.push_back(LengthWeightingStrategy(NetExpectedDataSource<float>(*it,net,print_warning_callback),LENGTH_WEIGHT,NULL));
		for (vector<string>::iterator it=absolutedatanames.begin();it!=absolutedatanames.end();it++)
			datatokeep.push_back(LengthWeightingStrategy(NetExpectedDataSource<float>(*it,net,print_warning_callback),POLYLINE_WEIGHT,NULL));
		for (vector<string>::iterator it=textdatanames.begin();it!=textdatanames.end();it++)
			textdatatokeep.push_back(NetExpectedDataSource<string>(*it,net,print_warning_callback));

		//add_expected_data will maintain pointers into these, so they must not be changed after
		add_expected_data(datatokeep);
		add_expected_data(textdatatokeep);

		for (vector<string>::iterator name=mergeifidenticalnames.begin();name!=mergeifidenticalnames.end();name++)
		{
			bool found_something = false;
			vector<LengthWeightingStrategy>::iterator found = datatokeep.end();
			for (vector<LengthWeightingStrategy>::iterator it=datatokeep.begin();it!=datatokeep.end();it++)
				if (it->get_name()==*name)
				{
					found = it;
					found_something=true;
				}
			if (found!=datatokeep.end())
				enforce_identical_numeric_data.push_back(&*found);
			vector<NetExpectedDataSource<string>>::iterator foundtext = textdatatokeep.end();
			for (vector<NetExpectedDataSource<string>>::iterator itt=textdatatokeep.begin();itt!=textdatatokeep.end();itt++)
				if (itt->get_name()==*name)
					foundtext = itt;
			if (foundtext!=textdatatokeep.end())
			{
				enforce_identical_text_data.push_back(&*foundtext);
				found_something=true;
			}
			if (!found_something)
				throw BadConfigException(string("merge_if_identical field not found in data_unitlength, data_absolute or data_text: ")+*name);
		}

		
		prepare_no_operation = config.get_bool("null");
		if (!config.get_bool("internal_interface"))
		{
			string action = config.get_string("action");
			to_lower(action);
			if (action=="detect")
				check=true;
			else if (action=="repair")
				check=false;
			else
				throw BadConfigException(string("Unrecognized prepare action: ")+action);

			handle_near_misses = config.get_bool("nearmisses");
			handle_traffic_islands = config.get_bool("trafficislands");
			handle_duplicates = config.get_bool("duplicates");
			handle_isolated = config.get_bool("isolated");
			handle_split_links = config.get_bool("splitlinks");

			if (!prepare_no_operation && handle_traffic_islands && config.get_string("island")=="")
				throw BadConfigException("Detect or repair of islands requested but no traffic island field specified");

			if (!check || prepare_no_operation)
			{
				//output is wrapper of repaired net
				vector<SDNAPolylineDataSource*> ldsv;
				inputdatawrapper = CalcExpectedDataSDNAPolylineDataSourceWrapper(this);
				ldsv.push_back(&inputdatawrapper);
				netwrapper = SDNAPolylineDataSourceGeometryCollectionWrapper(ldsv);
				geometry_outputs.push_back(&netwrapper); 
			}
			else
			{
				//output is geometry collection of errors
				vector<FieldMetaData> error_metadata;
				error_metadata.push_back(FieldMetaData(fLONG,"Original id","id"));
				error_metadata.push_back(FieldMetaData(fSTRING,"Error","Error"));
				errors = sDNAGeometryCollection("errors",POLYLINEZ,error_metadata);
				geometry_outputs.push_back(&errors);
			}
		}
	}
private:
	void add_arcid_c_array_to_errors(size_t size,long* arcids,string desc)
	{
		assert(check); // if fixing then link pointers will be incorrect
		stringstream s;
		s << size << " links found";
		print_warning_callback(s.str().c_str());

		for (size_t i=0;i<size;i++)
		{
			const long arcid = arcids[i];
			vector<SDNAVariant> data;
			data.reserve(2);
			data.push_back(arcid);
			data.push_back(desc);
			shared_ptr<sDNADataMultiGeometry> linkdata(
				new sDNADataMultiGeometry(
					POLYLINEZ,
					data,
					1));
			shared_ptr<sDNAGeometryPointsByEdgeLength> link(new sDNAGeometryPointsByEdgeLength());
			link->add_edge_length_from_start_to_end(&(net->link_container[arcid]->forward_edge),numeric_limits<float>::infinity());
			linkdata->add_part(link);
			errors.add(linkdata);
		}
	}
	virtual bool run_internal() 
	{
		if (!bind_network_data()) return false;
		if (prepare_no_operation) return true;

		if (handle_near_misses)
		{
			if (check)
			{
				print_warning_callback("Checking for near misses (link ends closer than cluster tolerance)");
				long *near_misses;
				size_t num_near_misses = get_near_miss_connections(&near_misses);
				add_arcid_c_array_to_errors(num_near_misses,near_misses,"Near miss");
			} else {
				print_warning_callback("Fixing near miss connections");
				fix_near_miss_connections();
			}
		}
		if (handle_traffic_islands)
		{
			if (check)
			{
				print_warning_callback("Checking for traffic islands");
				long *errors;
				size_t num_errors = get_traffic_islands(&errors);
				add_arcid_c_array_to_errors(num_errors,errors,"Traffic Island");
			} else {
				print_warning_callback("Fixing traffic islands");
				fix_traffic_islands();
			}
		}
		if (handle_duplicates)
		{
			if (check)
			{
				print_warning_callback("Checking for duplicate links");
				long *errors, *originals;
				size_t num_errors = get_duplicate_links(&errors, &originals);
				add_arcid_c_array_to_errors(num_errors,errors,"Duplicate");
			} else {
				print_warning_callback("Fixing duplicate links");
				fix_duplicate_links();
			}
		}
		if (handle_isolated)
		{
			//run check anyway to get message
			print_warning_callback("Checking for isolated systems");
			long *errors;
			char *message;
			size_t num_errors = get_subsystems(&message,&errors);
			print_warning_callback(message);
			if (check)
				add_arcid_c_array_to_errors(num_errors,errors,"Isolated");
			else {
				print_warning_callback("Fixing isolated systems");
				fix_subsystems();
			}
		}
		if (handle_split_links)
		{
			if (check)
			{
				print_warning_callback("Checking for split links");
				long *errors;
				size_t num_errors = get_split_link_ids(&errors);
				add_arcid_c_array_to_errors(num_errors,errors,"Split Link");
			} else {
				print_warning_callback("Fixing split links");
				fix_split_links();
			}
		}
		//if all above are fixed (!check), net will now be written
		//if in detect mode (check) errors will be written
		return true;
	}
};