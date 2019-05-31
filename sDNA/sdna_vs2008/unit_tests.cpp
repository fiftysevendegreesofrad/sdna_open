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
#include "sdna.h"
#include "net.h"
#include "calculation.h"
#include "metricevaluator.h"

//deceptively this doesn't contain many tests
//that's because the majority of testing is at module level (python calling normal analysis functions with debug output enabled)

class TestHooks
{
public:
	static TraversalEventAccumulator test_pci(SDNAPolyline *link,float partial_length)
	{
		Edge *edge = &link->forward_edge;
		return link->traversal_events.partial_cost_from_iterators_ignoring_oneway(edge->traversal_events_begin(),edge->traversal_events_end(),partial_length,PLUS);
	}
	static void test_pci_from_middle(SDNAPolyline *link,float partial_length)
	{
		Edge *edge = &link->forward_edge;
		link->traversal_events.partial_cost_from_iterators_ignoring_oneway(edge->traversal_events_centre(),edge->traversal_events_end(),partial_length,PLUS);
	}
	static float evaluate_me(SDNAPolyline *link,MetricEvaluator &e,polarity direction)
	{
		Edge &edge = (direction==PLUS)?link->forward_edge:link->backward_edge;
		return edge.evaluate_me(&e,edge.full_cost_ignoring_oneway());
	}
};

class DummyCallbacks
{
public:
	static bool print;
	static int __cdecl warning(const char* msg)
	{
		if (print)
			cout << msg << endl; 
		return 0;
	}
	static int __cdecl progressor(float) {return 0;}
};
bool DummyCallbacks::print = false;

void test_evaluator(string expr,vector<Point> &p3)
{
	try
	{
		Net net4;
		net4.add_polyline(0,p3);
		net4.add_polyline_data(0,"one",1);
		net4.add_polyline_data(0,"two",2);
		SDNAIntegralCalculation c(&net4,"linkonly",&DummyCallbacks::progressor,&DummyCallbacks::warning);
		DummyCallbacks::print = true;
		HybridMetricEvaluator e(expr,"0",&net4,&c);
		if (c.bind_network_data())
		{
			cout << expr << ": fwd=" << TestHooks::evaluate_me(net4.link_container[0],e,PLUS)
				<< " bwd=" << TestHooks::evaluate_me(net4.link_container[0],e,MINUS);
			if (!e.test_linearity()) cout << " TESTED NONLINEAR";
			cout << endl;
		}
		else
			cout << expr << ": did not compile" << endl;
	}
	catch (SDNARuntimeException &e)
	{
		cout << expr << ": threw runtime exception " << e.what() << endl;
	}
	catch (BadConfigException &e)
	{
		cout << expr << ": threw config exception " << e.what() << endl;
	}
	DummyCallbacks::print = false;
}

void pci_tests(Net &net)
{
	SDNAPolyline *link = net.link_container[0];
	PartialEdge::debug_on();
	cout << "full traversal" << endl;
	float length = TestHooks::test_pci(link,numeric_limits<float>::infinity()).euclidean;
	cout << "half traversal ending exactly on centre" << endl;
	TestHooks::test_pci(link,length/2);
	cout << "quarter traversal starting from centre" << endl;
	TestHooks::test_pci_from_middle(link,length/4);
	PartialEdge::debug_off();
}

SDNA_API void __stdcall run_unit_tests()
{
	cout << "PartialEdge test" << endl;
	Net net;
	vector<Point> p;
	p.push_back(Point(0,0,0));
	p.push_back(Point(1,0,0));
	p.push_back(Point(1,1,0));
	net.add_polyline(0,p);
	net.add_polyline_centres(ANGULAR_TE);
	cout << "ANGULAR, CENTRE ON CORNER" << endl;
	pci_tests(net);
	net.add_polyline_centres(EUCLIDEAN_TE);
	cout << "EUCLIDEAN, CENTRE ON CORNER" << endl;
	pci_tests(net);

	Net net2;
	vector<Point> p2;
	p2.push_back(Point(0,0,0));
	p2.push_back(Point(1,0,0));
	p2.push_back(Point(1,1,0));
	p2.push_back(Point(2,1,0));
	net2.add_polyline(0,p2);
	net2.add_polyline_centres(ANGULAR_TE);
	cout << "ANGULAR, CENTRE ON STRAIGHT" << endl;
	pci_tests(net2);
	net2.add_polyline_centres(EUCLIDEAN_TE);
	cout << "EUCLIDEAN, CENTRE ON STRAIGHT" << endl;
	pci_tests(net2);

	Net net3;
	vector<Point> p3;
	p3.push_back(Point(0,0,0));
	p3.push_back(Point(1,0,0));
	p3.push_back(Point(1,3,0));
	net3.add_polyline(0,p3);
	net3.add_polyline_centres(EUCLIDEAN_TE);
	cout << "EUCLIDEAN ASYMMETRIC" << endl;
	pci_tests(net3);
	cout << endl;

	cout << "muparser test" << endl;
	vector<Point> parserlink;
	cout << "(0,0,0)(1,0,1)(1,3,-1)" << endl;
	parserlink.push_back(Point(0,0,0));
	parserlink.push_back(Point(1,0,1));
	parserlink.push_back(Point(1,3,-1));
	test_evaluator("ang/FULLang",parserlink);
	test_evaluator("euc/FULLeuc",parserlink);
	test_evaluator("hg/FULLhg",parserlink);
	test_evaluator("hl/FULLhl",parserlink);
	test_evaluator("one+two",parserlink);
	test_evaluator("nonexistantvariable",parserlink);
	test_evaluator("ang",parserlink);
	test_evaluator("euc",parserlink);
	test_evaluator("hg",parserlink);
	test_evaluator("hl",parserlink);
	test_evaluator("hg+2.2*(hl+1.01)",parserlink);
	test_evaluator("inf",parserlink);
	test_evaluator("-inf",parserlink);
	test_evaluator("0/0",parserlink);
	test_evaluator("1/0",parserlink);
	test_evaluator("-1/0",parserlink);
	test_evaluator("proportion(0,0)",parserlink);
	test_evaluator("proportion(1,0)",parserlink);
	test_evaluator("proportion(-1,0)",parserlink);
	test_evaluator("proportion(2,2)",parserlink);
	test_evaluator("x=2,x",parserlink);
	test_evaluator("_x=2,_x",parserlink);
	test_evaluator("_x",parserlink);
	test_evaluator("_x=fwd?one:two,_x*2+1",parserlink);
	test_evaluator("randnorm(0,1)",parserlink);
	test_evaluator("randuni(0,1)",parserlink);
	test_evaluator("1+*)",parserlink);
	test_evaluator("5.7723*ang+69.3*euc+0.77*hg+1000*hl",parserlink);
	test_evaluator("sin(90)",parserlink);
	test_evaluator("sin(pi/2)",parserlink);
	test_evaluator("",parserlink);
	cout << endl;
}

