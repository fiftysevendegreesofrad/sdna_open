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
#pragma once
using namespace std;

const double PI = atan(1.0)*4;

struct Point
{
	double x, y;
	float z;
	Point(double xx,double yy,float zz)
		: x(xx), y(yy), z(zz) {}
	Point() : x(0), y(0), z(0) {}
	Point& operator=(const Point& other)
	{
		x = other.x;
		y = other.y;
		z = other.z;
		return *this;
	}
	friend Point operator+ (const Point &a,const Point &b)
	{
		return Point(a.x+b.x,a.y+b.y,a.z+b.z);
	}
	friend Point operator- (const Point &a,const Point &b)
	{
		return Point(a.x-b.x,a.y-b.y,a.z-b.z);
	}
	Point operator* (const float scalar)
	{
		return Point(x*scalar,y*scalar,z*scalar);
	}
	bool operator==(const Point &other) const
	{
		return x==other.x && y==other.y && z==other.z;
	}
	bool operator!=(const Point &other) const
	{
		return !(*this==other);
	}
	bool operator<(const Point &other) const
	{
		if (x<other.x) 
			return true;
		else
		{
			if (x>other.x)
				return false;
			else
			{
				//x==other.x
				if (y<other.y)
					return true;
				else
				{
					if (y>other.y)
						return false;
					else
					{
						//y==other.y
						return z<other.z;
					}
				}
			}
		}
	}
	double squarelength() const
	{
		return x*x+y*y+z*z;
	}
	float length() const
	{
		return (float)sqrt(squarelength());
	}
	void print()
	{
		std::cout << toString();
	}
	string toString()
	{
		stringstream retval;
		retval << "(" << x << "," << y << "," << z << ")";
		return retval.str();
	}
	static float distance(Point &a, Point &b)
	{
		return (b-a).length();
	}
	static float squaredistance(Point &a, Point &b)
	{
		return (float)(b-a).squarelength();
	}
	static bool too_close_for_turns(Point &a, Point &b)
	{
		//too close if taking the square length and multiplying by a similarly short length returns zero
		return (pow((a-b).squarelength(),2) == 0);
	}
	static float turn_angle_3d(Point &origin, Point &turn, Point &destination)
	{
		if (origin==destination)
			return 180; //should save some time

		// calculates using cosine rule: A^2 = B^2 + C^2 - 2BC cos a
		// where A,B,C are sides and a is angle opposite side A
		// in this case 180-a is the turn cost, which we want
		// so cost = 180 - arccos ((B^2+C^2-A^2)/(2BC))
		// but multiply by 180/pi to convert from radians
		Point A_vector = origin-destination;
		Point B_vector = origin-turn;
		Point C_vector = turn-destination;
		double A2 = A_vector.squarelength();
		double B2 = B_vector.squarelength();
		double C2 = C_vector.squarelength();
		double BC = sqrt(B2*C2);

		if (BC==0)
		{
			//if this happens in practice, fail gracefully:
			//return 90 degrees (the average turn size) rather than nan
			//(the user will have been warned that this will happen in net::warn_about_invalid_turn_angles)
			return 90;
		}

		double side_ratio = (B2+C2-A2)/(2*BC);
		if (side_ratio > 1)
			return 180; // possible on steep turn due to rounding errors
		if (side_ratio < -1) 
			return 0; // possible on shallow turn due to rounding errors
		float result = (float)(180-180/PI * acos(side_ratio));
		return result;
	}
	static float bearing(Point &origin,Point &destination)
	{
		const Point difference = destination-origin;
		const double dx = difference.x;
		const double dy = difference.y;
		if (!(dx==0 && dy==0))
			return (float)fmod(360+90-atan2(dy,dx)/PI*180,360);
		else
			return numeric_limits<float>::infinity(); //because this gets carried into arcgis as null, unlike NaN, which crashes
	}
	static Point midpoint(Point *a,Point *b)
	{
		return proportional_midpoint(a,b,0.5f);
	}
	static Point proportional_midpoint(Point *a,Point *b,float proportion)
	{
		if (!_isnan(proportion))
		{
			assert (a != NULL);
			assert (b != NULL);
			assert (proportion >= 0);
			assert (proportion <= 1);
			return *a + (*b - *a)*proportion;
		}
		else
		{
			assert(*a==*b);
			return *a;
		}
	}
};