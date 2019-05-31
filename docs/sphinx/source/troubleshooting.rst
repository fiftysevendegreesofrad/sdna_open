.. _`troubleshooting`:

======================
Troubleshooting Models
======================

Is the model wrong, or am I?
----------------------------

If a model doesn’t meet expectations then it is possible that either
expectations or the model are out of line with reality. The difficulty
is, of course, how to tell which. In cases where predictions can be
compared to actual measurements, these can be used to determine what is
wrong with the model. Are all outliers in a cycle model on a certain type of road, for
example? The model is probably over or underestimating the effect of
traffic. All in a certain part of town? The model could be misestimating
the weight of a particular origin or destination. Isolated errors? These can be
caused by the spatial network model misrepresenting what is actually
there; for example, roads disconnected in the model when they are
connected in reality.

In other cases, we have no predictions with which to be sure, but the
model doesn’t seem to match expectations. Two techniques can be used to
drill into why the model predicts what it does; and hence help decide
whether the results are likely to be correct.

Links with low flow in the model which we didn’t expect
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If a flow is unexpectedly low, then you as the user likely have an
expectation of which origins and destinations should cause that flow.
sDNA allows plotting of individual routes between origins and
destinations, so it is possible to select some and try it.

1. Pick an origin/destination pair. Note down their object IDs in the
   network dataset. Suppose for this example, the origin has id 482 and
   the destination is 9907.

2. Run sDNA Geodesics *with the same configuration as the model run*. :

   a. Add “n” to the list of radii, e.g. 1000, 2000, 3000, 4000, n

   b. Set origins to 482 and destinations to 9907
      (or whichever origin and destination id you noted down in step
      1)

Be careful not to run Geodesics without specifying an origin and
destination – or a LOT of data will be generated (routes for every
origin/destination pair).

On inspecting the geodesic, one of two errors may be apparent:

   a. Geodesic is generated for radius n but not the radius used in
      analysis. This means the endpoints were too distant for the chosen radius.

   b. Geodesic is generated between the endpoints but not passing
      through the expected link. This implies that the analysis metric for routes through the expected link must
      be higher than the analysis metric for the actual geodesic.  Inspecting the geodesic’s analysis metric and 
      compare to metrics generated from the expected route.  The latter is shown for each link in the original
      cycle model output, as *Hybrid Metric Fwd (HmF)* and *Hybrid Metric Bwd (HmB)*.
      Inspection may reveal why the model regards the unexpected route
      as preferable, or may hint at problems in the spatial model.

Links with high flow in the model which we didn’t expect
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If a flow is unexpectedly high, run a betweenness or geodesics analysis with an :ref:`intermediate link filter <intermediate link filter>` to find out where the flow comes from.

1. Create a new data field on the network, set to 0 everywhere but 1 on the link we are expecting.

2. Run sDNA Integral or Geodesics with an intermediate link filter specifying the new data field.  

sDNA Integral will produce a map of flows passing through the link; sDNA Geodesics will allow identification of individual paths.  Either way, trace the source of the predicted flows to establish whether or not there is an error; and if so whether it is in the origin/destination weighting, the analysis metric affecting the choice of routes, or the network geometry.
