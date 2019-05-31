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
#include "random.h"

//will have to move random number generator to stack of calculation::run_internal
//if we want to have deterministic rng
//many threads and processes can access this, only the mutex keeps it safe
boost::random::mt19937 random_number_generator;
boost::mutex random_mutex;

float randuni(float lower,float upper)
{
	boost::random::uniform_real_distribution<float> dist(lower,upper);
	boost::lock_guard<boost::mutex> lock(random_mutex);
	return dist(random_number_generator);
}

float randnorm(float mean,float sigma)
{
	boost::random::normal_distribution<float> dist(mean,sigma);
	boost::lock_guard<boost::mutex> lock(random_mutex);
	return dist(random_number_generator);
}

