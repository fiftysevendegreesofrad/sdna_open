.. _`guide to individual tools`:

*************************
Guide to individual tools
*************************


--------------------------------
Options common to multiple tools
--------------------------------

Several tools share the following parameters:

* *Input*, *Output*
    Shape layers for input and output

* *Start grade separation*, *End grade separation*
    Select fields for grade separation.  If given, links will only be deemed connected if
        1. they share the same endpoints (x,y,z)
        2. they share the same grade separation value at those endpoints
        
    Grade separation can also be used to emulate multilayer/multimode networks.  For example, use values of 0,1,2... for grade separation on roads, 10,11,12... for railways, etc.

* *Analysis Metric*
    Select metric for analysis: Euclidean, angular, custom, other (see :ref:`metric` for details).
    
    Custom metrics require selection of a custom metric data field.
    
    Some metrics provided are preset forms of hybrid metric designed for specific network types (pedestrian, vehicle, cycle, public transport).  You can inspect
    the message output of sDNA to discover exactly what formula they use.  Some presets include user variables, which have a default value but can be changed in advanced config; see `preset metric variables`_.
    
* *Weighting*, *Origin weight*, *Destination weight*
    Select :ref:`weighting type` (link, length or polyline) and the data to use.  If *origweightformula* or *destweightformula* are supplied in advanced config, they override origin and destination weight.
    
* *Radii*
    Enter one or more radii to use as radius of analysis (see :ref:`radii` for details).  By default these are Euclidean and expressed in the spatial units of the data, which should usually be metres if your data is `projected correctly`_.  If entering several, separate them by commas.  Radius 'n' can be used to specify global analysis, limited only by the size of the network (but computationally intensive).  Example: ``100, 200, 400, 800, 1600, n``
    
.. _`projected correctly`: :ref:`projection`

* *Continuous space*
    Select whether :ref:`continuous space <continuous space>` analysis is used.
    
* *Banded radius*
    If this mode is chosen, measures for each radius exclude links contained in the next smallest radius.  This allows multivariate analysis across radii while keeping cross-correlation of measures to a minimum.
    
* *Disable lines*
    Allows discarding of certain polylines from the analysis.  This is useful for testing multiple design 
    options, or for switching on and off parts of the network such as cycle paths.  
    
    This parameter takes an :ref:`expression <expression>` which, if it
    evaluates to a nonzero result for a given polyline, will disable that polyline.  In the simplest case, this can
    be a field name such as ``walk_net`` (which would disable all lines for which the walk_net field is nonzero) or a combination such as ``walk_net||cycle_net`` (which would disable both the walking and cycling network - useful for a vehicle analysis).
    
* *Origin Destination Matrix file*

    Optionally allows weights to be determined by an Origin-Destination (OD) matrix.  The file must be formatted correctly, see :ref:`Creating a zone table or matrix file`.  All geodesic and destination weights are replaced by values read from the matrix.  The matrix is defined between sets of *zones*; polylines must contain fields to indicate their zone.
    
    In the case of sDNA Integral, :ref:`Two Phase Betweenness` is disabled, because use of a two phase model for determining geodesic and destination weights conflicts with use of an OD matrix to determine these.

.. _`intermediate link filter`:
    
* *Intermediate link filter*

    Optionally restricts the analysis to include only geodesics that pass through a given line or set of lines specified by the filter.  Affects output of Betweenness, Two Phase Betweenness, Two Phase Destination, Mean Crow Flight, Mean Geodesic Length, Diversion Ratio measures; also Geodesic and Destination geometries.
    
    This parameter takes an :ref:`expression <expression>` which, if it
    evaluates to a nonzero result for a given line, will include geodesics which pass through that line.  In the simplest case, this can
    be a field name such as ``my_link_filter`` (which would disable all lines for which the field *my_link_filter* is nonzero).
    
    It is not sufficient for a geodesic's origin or destination to pass the filter; an intermediate line must pass in order for the geodesic to be included.
    
* *Advanced config*
    Allows setting of parameters not shown in the interface.  These are described in `advanced_config`_.

-----------------------
Individual tool details
-----------------------
    
Preparation
***********

.. _prepare:

===============
Prepare network
===============

Prepares spatial networks for analysis by checking and optionally repairing various kinds of error.

**Note that the functions offered by sDNA prepare are only a small subset of those needed for preparing networks.**  A good understanding of :ref:`network preparation` is needed, and other (free) tools can complement sDNA Prepare.

The errors fixed by sDNA Prepare are:

* *endpoint near misses* (XY and Z tolerance specify how close a near miss)
* *duplicate lines*
* *traffic islands* (requires traffic island field set to 0 for no island and 1 for island).  Traffic island lines are straightened; if doing so creates duplicate lines then these are removed.
* *split links*. Note that fixing split links is no longer necessary as of sDNA 3.0 so this is not done by default
* *isolated systems*

See `Options common to multiple tools`_.

Optionally, numeric data can be preserved through a prepare operation by providing the desired field names, separated by commas, to the parameters *Absolute data to preserve* and *Unit length data to preserve*.  

========================
Individual Line Measures
========================

Outputs connectivity, bearing, euclidean, angular and hybrid metrics for individual polylines.  

This tool can be useful for checking and debugging spatial networks.  In particular, connectivity output can reveal geometry errors.

See `Options common to multiple tools`_.

Analysis
********

.. _`integral analysis`:

=================
Integral Analysis
=================

sDNA Integral is the core analysis tool of sDNA.  It computes several flow, accessibility, severance and efficiency measures on networks.  Full details of the analysis are given in :ref:`Analysis: friendly guide` and :ref:`Analysis: full specification`.

Integral allows output of various groups of measures to be switched on and off.

See `Options common to multiple tools`_.

==================================
Specific Origin Accessibility Maps
==================================

Outputs accessibility maps for specific origins.  

See `Options common to multiple tools`_.

The accessibility map tool also allows a list of origin polyline IDs to be supplied (separated by commas).  Leave this parameter blank to output maps for all origins.  

If outputting "maps" for multiple origins, these will be output in the same feature class as overlapping polylines.  It may be necessary to split the result by origin link ID in order to display results correctly.

==========================================
Integral from OD Matrix (assignment model)
==========================================

A simplified version of sDNA Integral geared towards use of an external Origin Destination matrix.  Note that several other tools (including Integral) allow Origin Destination matrix input as well.

The file must be formatted correctly, see :ref:`Creating a zone table or matrix file`.  All geodesic and destination weights are replaced by values read from the matrix.  The matrix is defined between sets of *zones*; polylines must contain text fields to indicate their zone.

===========
Skim Matrix
===========

Skim Matrix outputs a table of inter-zonal mean distance (as defined by whichever sDNA Metric is chosen), allowing high spatial resolution sDNA models of accessibility to be fed into existing zone-base transport models.

Geometries
**********

The geometry tools output individual geometries used in an integral analysis.  These may be useful either for visualization, or for exporting to external analysis tools.  For example, you could join geodesics to a pollution dataset to estimate exposure to pollution along everyday travel routes.

============
Convex Hulls
============

Outputs the convex hulls of network radii used in `Integral Analysis`_.  

See `Options common to multiple tools`_.

The convex hulls tool also allows a list of origin polyline IDs to be supplied (separated by commas).  Leave this parameter blank to output hulls for all origins.

=========
Geodesics
=========

Outputs the geodesics (shortest paths) used by `Integral Analysis`_.  

See `Options common to multiple tools`_.

The geodesics tool also allows a list of origin and destination polyline IDs to be supplied (separated by commas).  Leave the origin or destination parameter blank to output geodesics for all origins or destinations.  (Caution: this can produce a very large amount of data).

=============
Network Radii
=============

Outputs the network radii used in `Integral Analysis`_.  

See `Options common to multiple tools`_.

The network radii tool also allows a list of origin polyline IDs to be supplied (separated by commas).  Leave this parameter blank to output radii for all origins.

Calibration
***********

sDNA Learn and Predict provide a way to calibrate sDNA outputs against measured variables (flows, house prices, etc).  Currently they offer bivariate regression with Box-Cox transformation.  Multiple predictor variables (the outputs of sDNA) can be tested to see which gives the best cross-validated correlation with the target variable.

.. _`learn`:

=====
Learn
=====

sDNA Learn selects the best model for predicting a target variable, then computes GEH and cross-validated :math:`R^2`.  If an output model file is set, the best model is saved and can be applied to fresh data using sDNA Predict.

Available methods for finding models are:

* *Single best variable* - performs bivariate regression of target against all variables and picks single predictor with best cross-validated fit
* *Multiple variables* - regularized multivariate lasso regression
* *All variables* - regularized multivariate ridge regression (may not use all variables, but will usually use more than lasso regression)

Candidate predictor variables can either be entered as field names separated by commas, or alternatively as a *regular expression*.  The latter follows `Python regex syntax`_.  A wildcard is expressed as ``.*``, thus, ``Bt.*`` would test all Betweenness variables (which in abbreviated form begin with *Bt*) for correlation with the target.

.. _`Python regex syntax`: https://docs.python.org/2/library/re.html#regular-expression-syntax

Box-Cox transformations can be disabled, and the parameters for cross-validation can be changed.

*Weighting lambda* weights data points by :math:`\frac{y^\lambda}{y}`, where :math:`y` is the target variable.  Setting to 1 gives unweighted regression.  Setting to around 0.7 can encourage selection of a model with better GEH statistic, when used with traffic count data.  Setting to 0 is somewhat analagous to using a log link function to handle Poisson distributed residuals, while preserving the model structure as a linear sum of predictors.  Depending on what you read, the literature can treat traffic count data as either normally or Poisson distributed, so something in between the two is probably safest.

Ridge and Lasso regression can cope with multicollinear predictor variables, as is common in spatial network models.  The techniques can be interpreted as frequentist (adding a penalty term to prevent overfit); Bayesian (imposing a hyperprior on coefficient values); or a mild form of entropy maximization (that limits itself in the case of overspecified models).  More generally it's a machine learning technique that is tuned using cross-validation.  The :math:`r^2` values reported by learn are always cross-validated, giving a built-in test of effectiveness in making predictions.

*Regularization Lambda* allows manual input of the minimum and maximum values for regularization parameter :math:`\lambda` in ridge and lasso regression. Enter two values separated by a comma. If this field is left blank, the software attempts to guess a suitable range, but is not always correct. If you are familiar with the theory of regularized regression you may wish to inpect a plot of cross validated :math:`r^2` against :math:`\lambda` to see what is going on. The data to do this is saved with the output model file (if specified), with extension ``.regcurve.csv``.

.. _`predict`:

=======
Predict
=======

Predict takes an output model file from sDNA Learn, and applies it to fresh data.  For example, suppose we wish to calibrate a traffic model, using measured traffic flows at a small number of points on the network.  

* First run a Betweenness analysis at a number of radii using `Integral Analysis`_.  
* Use a GIS spatial join to join Betweenness variables (the output of Integral) to the measured traffic flows.
* Run `Learn`_ on the joined data to select the best variable for predicting flows (where measured).
* Run `Predict`_ on the output of Integral to estimate traffic flow for all unmeasured polylines.

.. _advanced_config:

-----------------------------------------------
Advanced configuration and command line options
-----------------------------------------------

sDNA supports a wide
variety of options for customizing the analysis beyond what is shown in the user interface.  All of these are accessed through the advanced config system.

Advanced config options are specified in a long string with options
separated by semicolons (;) like this::

  nohull;probroutethreshold=1.2;skipzeroweightorigins

This is an example of an advanced config for sDNA Integral, which means

-  Don’t compute convex hull

-  Problem route threshold = 1.2

-  Skip zero weight origins

When calling sDNA `Integral Analysis`_ and `Prepare Network`_ from the command line (:ref:`command line`), the entire configuration is specified as an advanced config.  Therefore, the advanced config options include some which are usually set via the graphical interface.  If these options are given as advanced config in the sDNA graphical interface, an error ("Keyword specified multiple times") will result.

Advanced config options for sDNA Prepare
****************************************

.. csv-table::
   :file: prepare-advanced-config.csv
   :widths: 10,80
   :header-rows: 1
   

*xytol and ztol are manual overrides for tolerance. sDNA, running
on geodatabases from command line or ArcGIS, will read tolerance values from each feature class as
appropriate. sDNA running in QGIS or on shapefiles will use a default tolerance of
0, as shapefiles do not store tolerance information:- manual override is
necessary to fix tolerance on shapefiles.*

Advanced config options for sDNA Integral and geometry tools
************************************************************

*sDNA Convex Hulls, Network Radii, Geodesics and Accessibility Map are all different interfaces applied to sDNA Integral, so will in some cases accept these options as well.*

.. csv-table::
   :file: integral-advanced-config.csv
   :widths: 10,10,80
   :header-rows: 1

.. _preset metric variables:

Preset metric variables
***********************

A number of preset metrics are provided.  These are special cases of hybrid metrics, sometimes with a fairly complex formula.  To inspect the formula for a given metric, run `Individual Line Measures`_ with the metric selected, and inspect the message output where the full formula will be shown.

The CYCLE_ROUNDTRIP metric, as the name implies, measures a round trip to take account of hills in both directions.

Certain variables within the preset metric formulae can be changed by assigning to them in advanced config.  To date, the list is:

.. csv-table::
   :file: preset-metric-vars.csv
   :widths: 20,10,10,80
   :header-rows: 1
   
Interpretation of one way data
******************************

One way data is interpreted as follows:

* 0 – traversal allowed in both directions (so long as *vertoneway* allows this too)
*  positive number – forward traversal only
*  negative number – backward traversal only

Forwards/backwards are taken with respect to the direction in which the
link is drawn in the network (ordering of points in the data).

Vertical one way data is interpreted as follows:

* 0 – traversal allowed in both directions (so long as *oneway* allows this too)
*  positive number – upward traversal only
*  negative number – downward traversal only

Upward/downward are deduced by measuring the endpoints of the link only.
In the event that these have the same elevation/height and this leads to
ambiguity, sDNA will print an error message and exit.

If conflicting *oneway* and *vertoneway* data are provided, sDNA
will print an error message and exit. Note that if either field is zero,
the other is permitted to override it without conflict.

.. _`Creating a zone table or matrix file`:

Creating a zone table or matrix file
************************************

sDNA can read custom zone data, that is, data attached to *zones* rather than individual lines in the network.  This can come from 

* one-dimensional zone tables: provide the zone files to sDNA's inputs, and then reference the variables in expressions in the same way as you would use network data. This performs a function similar to a database join, to link zonal data to individual polylines. See `Zone Data and Zone Sums`_.
* a custom origin-destination (OD) matrix: provide sDNA with a two-dimensional table and it will override all other weights

One dimensional tables can be provided in *list* format, and two dimensional tables can be provided in *list* or *matrix* format.  The *list* format allows for sparse data, that is, data need not be given for all zones, and is assumed to be zero where not given.

All tables must be saved in CSV (comma separated) format.

=======================
1d table in list format
=======================

.. csv-table::
   :file: table-1dlist.csv
   :header-rows: 0
   
A 1d table in list format must have

* *list* and *1* in the header row
* zone field name and data names in the second row.  The network must contain a text field with name matching the zone field name (in this case "zone")
* zones and data below

=======================
2d table in list format
=======================
   
.. csv-table::
   :file: table-2dlist.csv
   :header-rows: 0
      
A 2d table in list format must have

* *list* and *2* in the header row
* origin and destination zone field names followed by data names in the second row.  The network must contain a text field with name matching the zone field names.  In this case, the origin and destination zones are drawn from the same set so these are both named "zone".  Different sets of zones for origin and destination are supported however (e.g. for use with census residential and workplace zones).
* zones and data below
   
=========================
2d table in matrix format
=========================   
   
.. csv-table::
   :file: table-2dmatrix.csv
   :header-rows: 0
   
*This table shows the same data as the 2d table in list format above* 
   
A 2d table in matrix format must have

* *matrix* in the first line followed by the origin zone field name then the destination zone field name.  The network must contain a text field with name matching the zone field names.  In this case, the origin and destination zones are drawn from the same set so these are both named "zone".  Different sets of zones for origin and destination are supported however (as with 2d list tables above).
* the second row starts with the name of the data, then the name of each destination zone
* the left column from row 3 downwards contains the name of each origin zone
* the remainder of the matrix contains the data

Zone Data and Zone Sums
***********************

Zone data is accessed in the same way as field data in expressions, described below. The following computes origin weights by multiplying *zoneweight* (taken from a table provided to sDNA) with the euclidean length of each polyline::

  origweightformula = zoneweight * euc 

Using *zonesums* in sDNA Integral's advanced config, it is possible to sum data over network zones. This is useful for controlling how zonal weights are distributed over polylines. The following example

* gives an example of how to use multiple zone schemes. It assumes two zonal variables are provided; *residential_weight* is defined for each zone in *res_zone*, and *retail_weight* is defined for each zone in *ret_zone*. In each case, the zone file will specify the fieldname which tells sDNA which zone each polyline belongs to.

* gives an example of how to compute multiple zone sums. These are specified in the form *sum1=expr1@zonefield1,sum2=expr2@zonefield2,...*. The config creates two zone sum fields, *eucsum* which is the total Euclidean length in each residential zone (*res_zone*), and *linksum* which is the total link count in each retail zone (*ret_zone*).

* gives an example of how to distribute zonal weights over the zones. *origweightformula* distributes the *residential_weight* zonal variable evenly over network length in each residential zone, while *destweightformula* distributes the *retail_weight* zonal variable evenly over links in each retail zone. (Note that polylines may constitute partial links, hence the use of *FULLlf*)::

    zonesums = eucsum=euc@origzonefield, linksum=FULLlf@destzonefield; origweightformula = residential_weight*proportion(euc,eucsum); destweightformula = retail_weight*proportion(FULLlf,linksum)

The ``proportion(x,y)`` function divides ``x`` by ``y``, which is useful to work out what proportion of zone weight is found in the current link.  It correctly handles the special cases where the zone contains no weight.
    
Note that *origweightformula* and *destweightformula* are always computed in discrete, rather than continuous space.

.. _expression:

Expression reference
********************

+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| Operator (in reverse order of precedence)   | Name                    | Example   | Meaning                           |
+=============================================+=========================+===========+===================================+
| ,                                           | Statement separator     | a,b,c     | Do a, then b, then output c       |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| =                                           | Assignment              | \_a=b     | Set \_a equal to b                |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| ?:                                          | If-then-else            | p?x:y     | If p then x else y                |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| &&                                          | Logical and             | a&&b      | a and b                           |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| \|\|                                        | Logical or              | a\|\|b    | a or b                            |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| <=                                          | Less than or equal      | a<=b      | a is less than or equal to b      |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| >=                                          | Greater than or equal   | a>=b      | a is greater than or equal to b   |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| !=                                          | Not equal               | a!=b      | a is not equal to b               |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| ==                                          | Equal                   | a==b      | a is equal to b                   |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| >                                           | Greater than            | a>b       | a is greater than b               |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| <                                           | Less than               | a<b       | a is less than b                  |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| \+                                          | Addition                | a+b       | a plus b                          |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| \-                                          | Subtraction             | a-b       | a minus b                         |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| \*                                          | Multiplication          | a\*b      | a times b                         |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| /                                           | Division                | a/b       | a divided by b                    |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| ^                                           | Exponentiation          | a^b       | a to the power of b               |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+
| ()                                          | Parentheses             | 2*(x+1)   | add one to x then multiply by 2   |
+---------------------------------------------+-------------------------+-----------+-----------------------------------+

+--------------------------------+-------------------------------------------------------------------------------------+
| Builtin functions              |                                                                                     |
+================================+=====================================================================================+
| sin(x), cos(x), tan(x)         | Trigonometric functions of x (in radians).                                          |
|                                |                                                                                     |
| asin(x), acos(x), atan(x)      |                                                                                     |
|                                |                                                                                     |
| sinh(x), cosh(x), tanh(x)      |                                                                                     |
|                                |                                                                                     |
| asinh(x), acosh(x), atanh(x)   |                                                                                     |
+--------------------------------+-------------------------------------------------------------------------------------+
| log2(x)                        | Logarithm of x base 2                                                               |
+--------------------------------+-------------------------------------------------------------------------------------+
| log10(x), log(x)               | Logarithm of x base 10                                                              |
+--------------------------------+-------------------------------------------------------------------------------------+
| ln(x)                          | Logarithm of x base e                                                               |
+--------------------------------+-------------------------------------------------------------------------------------+
| exp(x)                         | e to the power of x                                                                 |
+--------------------------------+-------------------------------------------------------------------------------------+
| sqrt(x)                        | Square root of x                                                                    |
+--------------------------------+-------------------------------------------------------------------------------------+
| sign(x)                        | -1 if x is negative, else 1                                                         |
+--------------------------------+-------------------------------------------------------------------------------------+
| rint(x)                        | x rounded to nearest integer                                                        |
+--------------------------------+-------------------------------------------------------------------------------------+
| abs(x)                         | Absolute value of x                                                                 |
+--------------------------------+-------------------------------------------------------------------------------------+
| min(a,b,c,…)                   | Minimum, maximum, sum and average of all arguments                                  |
|                                |                                                                                     |
| max(a,b,c,…)                   |                                                                                     |
|                                |                                                                                     |
| sum(a,b,c,…)                   |                                                                                     |
|                                |                                                                                     |
| avg(a,b,c,…)                   |                                                                                     |
+--------------------------------+-------------------------------------------------------------------------------------+
| trunc(x,l,u)                   | Truncate x to the range [l,u] (including endpoints)                                 |
+--------------------------------+-------------------------------------------------------------------------------------+
| randnorm(m,s)                  | Random number drawn from normal distribution with mean m and standard deviation s   |
+--------------------------------+-------------------------------------------------------------------------------------+
| randuni(l,u)                   | Random number drawn from uniform distribution on range [l,u]                        |
+--------------------------------+-------------------------------------------------------------------------------------+
| proportion(x,y)                | Divides x by y. Returns 0 if x=y=0 and stops calculation with error if x>0 and y=0. |
|                                | Useful for distributing zonal weights over links.                                   |
+--------------------------------+-------------------------------------------------------------------------------------+

Random numbers are generated from Mersenne Twister mt19937 algorithm.
*"Mersenne Twister: A 623-dimensionally equidistributed uniform
pseudo-random number generator", Makoto Matsumoto and Takuji Nishimura,
ACM Transactions on Modeling and Computer Simulation: Special Issue on
Uniform Random Number Generation, Vol. 8, No. 1, January 1998, pp.
3-30.*

+-------------+------------+
| Constants   |            |
+=============+============+
| inf         | infinity   |
+-------------+------------+
| pi          | pi         |
+-------------+------------+

+-------------+-------------------------------------------------------------------------------------------+
| Variables   |                                                                                           |
+=============+===========================================================================================+
| ang         | Angular change                                                                            |
+-------------+-------------------------------------------------------------------------------------------+
| euc         | Euclidean distance                                                                        |
+-------------+-------------------------------------------------------------------------------------------+
| hg          | Height gain                                                                               |
+-------------+-------------------------------------------------------------------------------------------+
| hl          | Height loss                                                                               |
+-------------+-------------------------------------------------------------------------------------------+
| FULLang     | Angular change for entire polyline                                                        |
+-------------+-------------------------------------------------------------------------------------------+
| FULLeuc     | Euclidean distance for entire polyline                                                    |
+-------------+-------------------------------------------------------------------------------------------+
| FULLhg      | Height gain for entire polyline                                                           |
+-------------+-------------------------------------------------------------------------------------------+
| FULLhl      | Height loss for entire polyline                                                           |
+-------------+-------------------------------------------------------------------------------------------+
| FULLlf      | Link fraction for entire polyline                                                         |
+-------------+-------------------------------------------------------------------------------------------+
| *\_x*       | *(where x is any name)*: Temporary variable (initialized to 0)                            |
+-------------+-------------------------------------------------------------------------------------------+
| *x*         | *(where x is any name not used as function or other value)*: field data on polyline       |
+-------------+-------------------------------------------------------------------------------------------+

*Any variable can be assigned to with =, but the new value will only
affect the current formula being evaluated (assigning to ``ang`` will
not change the shape of the network, for example!). It is recommended to
use only temporary variables of the form ``_x`` as targets for
assignment.*