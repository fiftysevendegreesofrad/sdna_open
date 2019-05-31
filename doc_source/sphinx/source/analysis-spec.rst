.. _`analysis: full specification`:

============================
Analysis: full specification
============================

This section outlines a more formal specification of the network analysis concepts on which sDNA Integral, Accessibility Map, Geodesics, Network Radii and Convex Hulls are built.

Note that as of sDNA Version 3, some of this specification has changed, in particular the handling of split links (the rationale for which is explained in a `blog post`_).  The `specification for versions 1 and 2`_ is still available.

.. _blog post: http://www.cardiff.ac.uk/sdna/repairing-split-links-is-no-longer-necessary/
.. _specification for versions 1 and 2: http://www.cardiff.ac.uk/sdna/software/documentation/

-----------
Definitions
-----------

Link
====

One or more polylines spanning the gap between two junctions, or a
junction and an end point.  

sDNA accepts data in *link-node format* (though strictly speaking, the nodes aren't necessary).  It also accepts data in *polyline-node* format.  Note that we make a distinction between *link* and *polyline*: some users of these formats don't, and will treat them as the same thing.  This is importatant for your understanding of sDNA, but not important when using the software as it will accept either format.

To represent transport networks, polylines can be centred on a road or path centre line.
Polylines can also be used to outline pedestrian paths and pedestrian
crossings, thus creating a pedestrian centre line network.  The largest
to date, to our knowledge, is 1400km long; created for the Commnunauté
d’Agglomération de Saint Quentin en Yvelines, Paris, France.

Polyline
========

A link consists of one or more *polylines*.  This matches the GIS or CAD notion of a polyline: a continuous chain
of line segments treated as a single unit, usually with a number of attached data fields. Polylines vary in length, in angularity (directness/sinuosity), in connectivity
(number of other ends of polylines connected).

Internally to sDNA, polylines are treated as indivisible origins; all flows originating from a polyline are treated as originating from its middle (which is defined as the Euclidean or angular centre according to the Distance Metric).  As destinations, polylines are treated as indivisible in Discrete Space mode, or divisible in Continuous Space mode.

Junction
========

A point where at least 3 link ends are connected. 

.. _metric:

Distance Metric
===============

A metric is a function used to define what it means for
points on the network to be "close to" or "far away from" each
other.  sDNA currently supports

* *Euclidean metric*
    Distance measured along the network in the distance units the network is defined in.  This is the everyday notion of distance
* *Angular metric*
    Distance measured in terms of angular change; i.e. corners on links and turns at junctions
* *Custom metric*
    Distance measured according to a user chosen data field defined on Polylines
* *Other metrics*
    We provide some other specific metrics suitable for urban network analysis 

.. _radii:

Radius
======

Analyses are performed from each origin within a user defined radius
expressed in the spatial units of the data.  Usually the set of origins is equal to the set of Polylines, so an analysis is conducted around each Polyline.

The user will usually wish to express a radius in metres, 
therefore care must be taken to project spatial data into a coordinate system measured in metres before using sDNA.  Radius can be understood as variable floating catchment area, a locality of analysis, or a maximum trip distance.

Two modes exist for handling of the radius boundary:

**Discrete space**
  all polylines are included in the radius of each origin if and only if their centres fall within the radius

.. _`continuous space`:  
  
**Continuous space**
  polylines are dynamically split, and only those parts that exactly fall within the radius of each origin are included in the analysis of that origin.  Fractional polylines have weights adjusted downwards according to the fraction of the link length they represent.
  
  Continuous space takes slightly more computation than discrete space, but is recommended for analyses in which the radius has the same (or smaller) order of magnitude as the longest polylines.  This usually implies small scale analysis e.g. a pedestrian or cyclist network.

A radius is expressed as a Euclidean distance by default, in units of the source data projection. If *banded radius* is used, each radius excludes links from the next smallest radius.
  
.. _geodesic:
  
Geodesic
========
A geodesic is the shortest route between two points on the network, according to a given Distance Metric.

Various special cases exist where this is not strictly true.  Note that when using non-Euclidean metrics, the network radius may be shorter than a geodesic.  As the network radius is supposed to specify a locality of analysis, we constrain such geodesics to use only network within the radius.  This behaviour can be modified in :ref:`advanced_config`.  The implications of handling these edge cases are explored in [1]_.

.. [1] Cooper (2015) Spatial localization of closeness and betweenness measures: a self-contradictory but useful form of network analysis.  International Journal of Geographical Information Science 29 (8).  http://www.tandfonline.com/doi/abs/10.1080/13658816.2015.1018834
  
.. _`weighting type`:
  
---------
Weighting
---------

The importance of each origin and destination in the analysis is determined by weighting.  Weights can be 

* per polyline
* per link
* per unit length

All of these can be based on custom data.  If weights are not given, all weights will default to 1, though this default can be interpreted (as above) per polyline, link or unit length.

The choice of weighting should reflect what you consider to be the
best measure of *network quantity* in
your analysis: number of links, length, or a custom defined property
(such as population plus number of jobs, or number of address points). The recommended choice for
urban networks – if you don’t have actual census data – is number of
links. This is because link density increases with number of jobs
and population: thus measuring network by the number of links goes
some way towards capturing these other variables through network
geometry.

.. csv-table::
   :file: weighting.csv
   :widths: 10,10,10,80
   :header-rows: 1

Note that address points (attached to individual buildings) can be processed by using a GIS Spatial Join to transfer data such as floor area from the point data to the network. If sub link accuracy is needed, links should be split into multiple sub links.   
   
---------------------------
Handling of one-way systems
---------------------------

When computing geodesics, the analysis respects specified one-way and
vertical one-way links. However, when computing the contribution of a
single link to its own closeness/betweenness/weight in radius etc, it is
assumed that all points on the link are directly reachable from one
another regardless of one-way status. This is to maintain consistency
with origin approximations and choice of link centres, and the handling
of other micromodelling situations within sDNA. In the absence of
black/white holes (points within a one way system that are impossible to escape/enter), this will result in a correct computation of links/length/weight in
radius for all sufficiently large radii (that is to say, if the radius
exceeds the maximum geodesic from any link to itself respecting the one
way system). Origin self-closeness/self-betweenness usually makes only a
small contribution to the overall analysis and is included principally for
consistency.

----------------------------------
Mathematical definition of outputs
----------------------------------

Notation
========

In the following sections,

* The set of polylines in the global spatial system is denoted :math:`N`

* The set of polylines in the network radius from link :math:`x` is denoted :math:`R_x`

* The proportion of any polyline :math:`y` within the radius is denoted P(y). In discrete space analysis, this always equals 0 or 1, i.e. :math:`y \in R_x \Leftrightarrow P(y) = 1`. In continuous space, :math:`0 \leq P(y) \leq 1`

* Length of a polyline :math:`y` is denoted :math:`L(y)`

* The distance according to a metric :math:`M`, along a geodesic defined by :math:`M`, between an origin polyline :math:`x` and a destination polyline :math:`y` is denoted :math:`d_M(x,y)`

* The network Euclidean distance along a geodesic defined by a metric :math:`M`, between an origin polyline :math:`x` and a destination polyline :math:`y` is denoted :math:`d^E_M(x,y)`

* Weight of a polyline :math:`y` is denoted :math:`W(y)`.  

:math:`W(y)` is computed differently according to the weighting scheme.  If

.. math::
  U(y) = \begin{cases}
  \text{the user defined weight for polyline }y,&&\text{if defined}\\
  1, &&\text{otherwise}
  \end{cases}

then
  
.. math::
  W(y) = \begin{cases}
  U(y), &&\text{for polyline weighting}\\
  U(y)L(y), &&\text{for length weighting}\\
  U(y)\frac{L(y)}{L(Z)}, &&\text{for link weighting}
  \end{cases}
  
where :math:`L(Z)` is the length of the *link* :math:`Z` formed of one or more polylines :math:`P` such that :math:`y \in P`

Centrality measures
===================

Farness
-------

Farness is measured as the Mean Angular, Euclidean, Custom or Hybrid distance according to the chosen distance metric.  It is abbreviated as MAD, MED, MCD and MHD respectively.

.. math::
  \mathbf{Farness}(x) = \dfrac{\sum\limits_{y \in R_x}d_M(x,y)W(y)P(y)}{\sum\limits_{y \in R_x} W(y)P(y)}

Note that on average, the distance of traversing between two arbitrary points within the same link is :math:`1/3` the distance of traversing the entire link.  The contribution of the origin link to its own farness is included in this manner.
  
Closeness
---------

sDNA doesn't measure closeness, it measures farness, which tells you exactly the same thing in a different way.  The literature often defines closeness as :math:`1/\mathbf{farness}`, though this has an exponential distribution so statistically is harder to work with.  An alternative definition of closeness that doesn't suffer from this problem is :math:`-\mathbf{farness}`.  We don't tend to use either, preferring to use either Farness or NQPD for the same purpose.

Mean geodesic length
--------------------

Mean geodesic length (MeanGeoLen or MGL) is the mean length (always in Euclidean metric) of all geodesics in the radius (defined by the chosen metric). 

.. math::
  \mathbf{MGL}(x) = \dfrac{\sum\limits_{y \in R_x}d_M^E(x,y)W(y)P(y)}{\sum\limits_{y \in R_x} W(y)P(y)}

MGL can be used as an invariant measure to compare geodesics from different hybrid metrics.  For example, a cyclist metric that accounts for road traffic can be compared to a cyclist metric without motor vehicle traffic by calculating :math:`MGL_\text{traffic}/MGL_\text{no traffic}`.  This would show areas where cyclists must make large detours to avoid motor vehicle traffic.
  
Network quantity penalized by distance (gravity model)
------------------------------------------------------

NQPD is a form of closeness, commonly referred to as a gravity model, that takes into account both quantity and accessibility of network weight.  By contrast, Farness takes into account only accessibility, while Weight takes into account only weight.  

.. math::
  \mathbf{NQPD}(x) = \sum_{y \in R_x} \frac{(W(y)P(y))^\text{nqpdn}}{d_M(x,y)^\text{nqpdd}}

Note that on average, the distance of traversing between two arbitrary points within the same link is :math:`1/3` the distance of traversing the entire link.  The contribution of the origin link to its own NQPD is included.
  
:math:`\text{nqpdn}` and :math:`\text{nqpdd}` default to 1, but can be set to other values in advanced config (they stand for NQPD numerator and denominator, respectively).  The problem, for any given application, is determining the correct values to use for each i.e. the relative importance of network quantity and accessibility.  To answer that question we recommend approximating NQPD with a multivariate translog linear regression based on Farness and Weight, i.e. using the model

.. math::
  \log(\text{variable of interest}) = \beta_0 + \beta_1\log(\mathbf{farness}) + \beta_2\log(\mathbf{weight}) + \epsilon

Once suitable values for :math:`\beta` have been obtained, these can be applied to :math:`\text{nqpdn}\approx\beta_2` and :math:`\text{nqpdd}\approx-\beta_1`.
  
Betweenness
-----------
Betweenness counts the number of geodesic paths
that pass through a vertex, i.e, the number of times the vertex lies
on the shortest path between other pairs of vertices.

.. math::
  \mathbf{Betweenness}(x)=\sum_{y \in N} \sum_{z \in R_y} W(y)W(z)P(z)OD(y,z,x)
  
where

.. math::
  OD(y,z,x) = \begin{cases}
  1,&&\text{if x is on the first geodesic found from y to z}\\
  1/2,&&\text{if } x = y \neq z\\
  1/2,&&\text{if } x = z \neq y\\
  1/3,&&\text{if } x = y = z\\
  0,&&\text{otherwise}
  \end{cases}

Note that the geodesic endpoints are y and z, *not* x where the betweenness is being measured.  The contributions of :math:`1/2` to :math:`OD(y,z,x)` reflect the end links of geodesics which are traversed half as often on average, as journeys begin and end in the link centre on average.  The contributions of :math:`1/3` represent origin self-betweenness.

Note that, in cases where a number of equal length geodesics exist between an origin and destination pair, sDNA will only consider the first such geodesic found.  This differs from some literature where betweenness is distributed over all geodesics of equal length.  If this is of concern (e.g. analysing a perfect grid pattern) we recommend adding a small amount of randomness to the analysis using a hybrid metric, and if necessary, combining this approach with oversampling.  (Oversampling is not usually necessary as randomness is reapplied per origin; i.e. every link analyzed represents a different sample of the random distribution even with no oversampling).  Randomness and oversampling can be set in :ref:`advanced_config`.


.. _`two phase betweenness`:

Two phase betweenness
---------------------

Two phase betweenness (TPBt) represents Betweenness, but rather than being weighted by a product of origin and destination weights, the origin weight is distributed over destination weights.  Since implementing this I have realized that it represents the flows from what is referred to in some older literature as the Huff model for accessibility. It is thus the sum of geodesics that pass through a link x, weighted by the proportion of network quantity accessible from geodesic origin y that is represented by geodesic destination z.

.. math::
  \mathbf{TPBt}(x) = \sum_{y \in N}\sum_{z \in R_y} OD(y,z,x) \frac{W(z)P(z)}{\text{total weight}(y)}
  
where :math:`\text{total weight}(y)` is the total weight in radius from each :math:`y`. 

Two phase destination
---------------------

Two phase destination (TPD) measures the proportion of origin weight received by each destination in the two phase betweenness model.  It is  similar to a two-step floating catchment, and the Huff accessibility model.  

.. math::

  \mathbf{TPD}(x)=\sum_{y \in N}\sum_{x \in R_y} \frac{W(x)P(x)}{\text{total weight}(y)}

In a normal betweenness analysis, this quantity would be equivalent to Weight, but as geodesic weight from each origin is limited in the two phase model, the weight transferred to destinations along geodesics becomes dependent not only on the weight within radius of the destination, but also on what that destination is competing with.  Thus this measure is more discriminating of spatial hierarchy than the Links, Length and Weight measures described below.

Note that TPBt has units of – and scales with - network quantity, rather than the square of network quantity as is the case with standard Betweenness.  Thus it corresponds to transport models with trip generation and distribution phases, while normal betweenness can be seen as an opportunity model.

Network detour analysis
=======================

Network detour analysis compares straight line distance to actual network distance, answering the question, “By how much does the network deviate from the most direct path?” 

Mean Crow Flight
----------------

Mean Crow Flight (MCF) is the mean of the crow flight distance between each origin and all links within the radius.  

.. math::
   \mathbf{MCF}(x)=\frac{\sum_{y \in R_x} CFD(x,y)W(y)P(y)}{\sum_{y \in R_x} W(y)P(y)}
   
where :math:`CFD(x,y)` is the crow flight distance between the centers of x and y.

MCF can be compared to Mean Geodesic Length (MGL); that is to say, :math:`\frac{MGL}{MCF}` gives a measure of the extent to which geodesics must divert from desire lines.

Diversion Ratio
---------------

Diversion Ratio (Div) is the mean ratio of geodesic length to crow flight distance over all links in the radius.  It differs from :math:`\frac{MGL}{MCF}` in that ratios are computed individually before averaging.

.. math::
   \mathbf{Div}(x) = \dfrac{\sum\limits_{y \in R_x}\dfrac{d_M(x,y)}{CFD(x,y)}W(y)P(y)}{\sum\limits_{y \in R_x}W(y)P(y)}

Network shape analysis
======================

Network shape analysis refers to the form of the overall spatial footprint of the network within the radius.  This can be used to compute measures of efficiency, for example,  :math:`\mathbf{Convex\ Hull\ Area}/\mathbf{Length}` provides a measure of the coverage of Eucidean space per unit length of network.

All of the following measures are based on a 2-d convex hull of all points within the network radius.  For 3-d networks, the network is projected onto the x-y plane before computing a convex hull.

Convex Hull Area
----------------

Convex Hull Area (HullA) is the area of the convex hull covered by the network within the radius.  

Convex Hull Perimeter
---------------------

Convex Hull Perimeter (HullP) is the perimeter of the convex hull covered by the network within the radius.  

Convex Hull Maximum Radius
--------------------------

Convex Hull Maximum Radius (HullR) is the distance (as the crow flies) from the origin to the point where the convex hull has its greatest radius (as the crow flies).  In other words, it is the largest crow flight distance to any point within the network radius, and as such represents the single route accessible from the origin that can cover the most distance as the crow flies.

Convex Hull Bearing
-------------------

This is the compass bearing of the `Convex Hull Maximum Radius`_, as measured from the positive y direction of the projected grid (this is usually grid north).

Convex Hull Shape Index
-----------------------

This is defined as

.. math::
   \mathbf{Hull\ Shape\ Index}=\frac{(\mathbf{Hull\ Perimeter})^2}{4\pi(\mathbf{Hull\ area})}
   
Note that the minimum possible shape index is 1 (for a circle), and the maximum is infinity (for a straight line).
   
Radius description measures
===========================

For each radius from each origin the following measures are given:

Links
-----

Links (Lnk) is the number of links in the radius, :math:`\sum_{y \in R_x}P(y)`

Length
------

Length (Len) is the total network length in the radius, :math:`\sum_{y \in R_x}L(y)P(y)`

Weight
------

Weight (Wt) is the total weight in the radius, :math:`\sum_{y \in R_x}W(y)P(y)`.  If you wish to normalize any other output measure for quantity of network, this is the best control to use as it adapts to the analysis type as appropriate.
    
Angular Distance
----------------

Angular distance (Ang Dist or AngD) is the total angular curvature on all links in the radius :math:`\sum_{y \in R_x}d_\theta(y_R)`, where :math:`y_R` is the proportion of y that falls within the radius only.

Junctions
---------

Junctions (Jnc) counts the number of junctions in the radius.  Note that only junctions between links, not polylines, are counted.  

Connectivity
------------

Connectivity (Con) is the total connectivity in the radius: sum of number of links ends connected at each junction.  Note that one way streets count as half a link end in this measure (unlike LConn where they are counted fully).
        
Individual polyline descriptive measures
========================================

Line Length
-----------

Line length (LLen) is the Euclidean length of polyline, :math:`L(y)`.

Line Connectivity
-----------------

Line Connectivity (LConn) is the number of other line ends to which this line is connected.  Also called *degree centrality*.

Line Angular Curvature
----------------------

Line angular curvature (LAC) is the cumulative angular curvature along the full length of the line, in degrees: :math:`d_\theta(y)`.

Line Hybrid Metrics
-------------------

In a hybrid analysis, the hybrid metrics for the polyline are given:  Hybrid Metric forward (HMf) and Hybrid metric backward (HMb).  The metric can be different per direction due to height gain, or custom behaviour that relates to direction of traversal, for example escalators, traffic priority or one way tolls.

Line Sinuosity
--------------

Line Sinuosity (LSin) is the line length divided by distance as the crow flies between its endpoints.  Similar to diversion ratio but for a single line only.  

Line Bearing
------------

Line Bearing (LBear) is the compass bearing between line endpoints, as measured from the positive y direction of the projected grid (this is usually grid north).

Link Fraction
-------------

Link Fraction (LFrac) is the proportion of a link that this line represents, :math:`P(y)`.

---------------------------------
List of outputs and abbreviations
---------------------------------

.. csv-table::
   :file: output-abbrev.csv
   :widths: 10,90
   :header-rows: 1
   
*Outputs marked \* are only shown if* ``outputsums`` *keyword is given in* :ref:`advanced_config`.