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

#include "sdna_arrays.h"
#include "edge.h"
#pragma once

template <typename T> class ThreadSafeAccumulator : public SDNAPolylineIdRadiusIndexed2dArrayBase
{
	SDNAPolylineIdRadiusIndexed2dArray<T> data; 

public:
	virtual void enable() {data.enable();}
	virtual bool is_enabled() {return data.is_enabled();}
	void initialize(size_t num_links, size_t num_radii)
	{
		data.initialize(num_links,num_radii,0.);
	}
	void increment(const SDNAPolyline * const link,size_t radius,double x)
	{
		T& d=data(link,radius);
		#pragma omp atomic
			d += x;
	}
	virtual float floatvalue(SDNAPolylineId link,size_t radius)
	{
		return (float)data(link,radius);
	}
};

template <typename T> class BidirectionalThreadSafeAccumulator
{
	ThreadSafeAccumulator<T> fwd_data, bwd_data;
	bool use_both_directions, is_initialized;
public:
	BidirectionalThreadSafeAccumulator<T>() : use_both_directions(false), is_initialized(false) {}
	void set_bidirectional() {assert(!is_initialized); use_both_directions=true;}
	void enable()
	{
		fwd_data.enable();
		if (use_both_directions)
			bwd_data.enable();
	}
	bool is_enabled() {assert (fwd_data.is_enabled()==bwd_data.is_enabled()); return fwd_data.is_enabled();}
	void initialize(size_t num_links, size_t num_radii)
	{
		is_initialized=true;
		fwd_data.initialize(num_links,num_radii);
		if (use_both_directions)
			bwd_data.initialize(num_links,num_radii);
	}
	void increment_both(const SDNAPolyline * const link,size_t radius,double x)
	{
		if (use_both_directions)
		{
			fwd_data.increment(link,radius,x);
			bwd_data.increment(link,radius,x);
		}
		else
			fwd_data.increment(link,radius,x*2);
	}
	void increment(const Edge * const edge,size_t radius,double x)
	{
		assert(edge->direction==PLUS || edge->direction==MINUS);
		if (!use_both_directions || edge->direction==PLUS)
			fwd_data.increment(edge->link,radius,x);
		else
			bwd_data.increment(edge->link,radius,x);
	}
	SDNAPolylineIdRadiusIndexed2dArrayBase* get_output_facade(polarity direction)
	{
		assert(use_both_directions);
		assert(direction==PLUS || direction==MINUS);
		if (direction==PLUS)
			return &fwd_data;
		else
			return &bwd_data;
	}
	SDNAPolylineIdRadiusIndexed2dArrayBase* get_output_facade_bidirectional()
	{
		assert(!use_both_directions);
		return &fwd_data;
	}
	bool is_bidirectional()
	{
		return use_both_directions;
	}
};

			


class AccessibilityAccumulator : public SDNAPolylineIdRadiusIndexed2dArrayBase
{
	SDNAPolylineIdRadiusIndexed2dArray<double> accessibility;
	float distance_constant_factor;
	double nqpdn, nqpdd;
public:
	virtual void enable() {accessibility.enable();}
	virtual bool is_enabled() {return accessibility.is_enabled();}
	void initialize(size_t num_links, size_t num_radii,float dcf,double nqpdn_in,double nqpdd_in)
	{
		distance_constant_factor = dcf;
		accessibility.initialize(num_links,num_radii,0.);
		nqpdn = nqpdn_in;
		nqpdd = nqpdd_in;
	}
	void increment(SDNAPolyline *link,size_t radius,double quantity,double cost)
	{
		//to prevent zero angular costs 
		cost += distance_constant_factor;

		//just in case cost was 0, we wouldn't want to make infinite accessibility for a 0-sized piece of network
		//(though if quantity is nonzero then an infinite accessibility is fair)
		if (quantity==0)
			return;

		accessibility(link,radius) += pow(quantity,nqpdn)/pow(cost,nqpdd);
	}
	virtual float floatvalue(SDNAPolylineId link,size_t radius)
	{
		return (float)accessibility(link,radius);
	}
};


class ThreadSafeProgressBar
{
private:
	size_t num_links_computed, last_progress_bar_pos, increment, total_num_links;
	int (__cdecl *set_progressor_callback)(float);

public:
	ThreadSafeProgressBar(size_t total_num_links,int (__cdecl *set_progressor_callback)(float))
		: total_num_links(total_num_links),
		  increment(total_num_links/1000),
		  last_progress_bar_pos(-1),
		  num_links_computed(0),
		  set_progressor_callback(set_progressor_callback)
	{
		if (increment==0)
			increment = total_num_links;
	}

	void increment_bar(long skip_fraction)
	{
		#pragma omp atomic
			num_links_computed+=skip_fraction;

		//if we have computed another [increment] links since last update
		//then signal new progress bar position via callback
		// -- master thread only due to com marshalling
		if (OMP_THREAD==0)
		{
			size_t current_progress_bar_pos = num_links_computed / increment;
			if (last_progress_bar_pos < current_progress_bar_pos)
				set_progressor_callback((float)num_links_computed/(float)total_num_links*100.f);
			last_progress_bar_pos = current_progress_bar_pos;
		}
	}
};

template <class T> class ThreadSafeVector 
{
private:
	vector<T> v;
	boost::mutex mutex;
public:
	//push_back and reserve lock with mutex
	void push_back(T& item)
	{
		boost::lock_guard<boost::mutex> lock(mutex);
		v.push_back(item);
	}
	void reserve(size_t n)
	{
		boost::lock_guard<boost::mutex> lock(mutex);
		v.reserve(n);
	}
	//forward other methods (not thread safe) just to keep track of what's used
	size_t size()
	{
		return v.size();
	}
	typename vector<T>::iterator begin() {return v.begin();}
	typename vector<T>::iterator end() {return v.end();}
	
	//copy constructor creates new mutex
	ThreadSafeVector<T>(ThreadSafeVector<T> &other) : vector<T>(other) {}
	//default constructor needed because copy constructor suppresses it
	ThreadSafeVector<T>() {} 
	//assignment operator keeps same mutex
	ThreadSafeVector<T>& operator=(ThreadSafeVector<T> &other) 
	{
		boost::lock_guard<boost::mutex> lock(mutex);
		v = other.v;
		return *this;
	}
	
};



