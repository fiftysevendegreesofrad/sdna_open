.. include:: <isonum.txt>

.. _`installation and first usage`:

****************************
Installation and first usage
****************************

Requirements
************

sDNA requires Windows XP or later.  We suggest a minimum of 1GB RAM, but obviously faster computers and more memory will be of use in analyzing larger networks.  sDNA can run in 32- or 64-bit mode depending on the host application.

The toolbox can be used in any of a number of ways:

-  As a plug-in to ArcGIS 10.1 or later, or

-  As a plug-in to QGIS 2.0 or later, or

-  As a plug-in to Autocad (various versions; 2010-2013 have been
   tested), or

-  From the windows command line, or

-  From sdnapy, the python API to sDNA.

Note that Autocad usage is limited to network shape analysis using sDNA Prepare and Integral.  For more detailed analysis, we recommend using Autocad Map3d or other suitable software to convert CAD to 3d Shapefile, then processing in the free QGIS.

Installation
************

sDNA Open can be downloaded an installed from Github_.  You will need a serial number; these can be obtained for free.  

.. _Github: https://github.com/fiftysevendegreesofrad/sdna_open/releases

Further steps are then needed depending on how you plan to use sDNA.

.. _firstuse:

Using sDNA for the first time
*****************************

How you use sDNA depends on your host application.

ArcGIS
======

1. From inside ArcGIS, go to ArcToolbox.

2. Right click the root of the ArcToolbox tree and choose ``Add
   Toolbox...``".

3. Navigate to the place you have installed sDNA (usually ``c:\Program
   Files (x86)\sDNA``) and select the toolbox ``sdna.pyt``.
   
4. (Optional) Repeat steps 2 and 3 to add the toolbox ``sDNA_ArcGIS_extra_tools.tbx``.

5. (Optional) To permanently add sDNA to ArcToolbox, right click the
   root of ArcToolbox and choose ``Save settings`` |rarr| ``to Default``.

sDNA appears as a set of tools within ArcToolbox. Results can be
displayed using the ``Symbology`` tab of the ``Layer Properties``
dialog. If you are not familiar with using tools from ArcToolbox, or
changing layer symbology, visit the *ArcGIS Desktop Help* website for
further details.

QGIS
====

1. From inside QGIS, choose ``Plugins`` |rarr| ``Manage and install plugins...``.  At present, QGIS support is considered experimental, so go to ``Settings`` and click ``Also show experimental plugins``.

2. Type "sdna" into the search box; you should find the Spatial Design Network Analysis plugin

3. Click ``Install Plugin``, then ``Close``

4. Go to ``Processing`` |rarr| ``Toolbox`` to show the processing toolbox

5. At the bottom of the processing toolbox, change from ``Simplified interface`` to ``Advanced interface``

6. "Spatial Design Network Analysis" should now appear in the processing toolbox

7. Go to ``Processing`` |rarr| ``Options`` |rarr| ``General`` and ensure ``Keep dialog open after running an algorithm`` is switched on.

Results of sDNA operations can be displayed using layer styles.  After running sDNA, right-click the relevant layer in the layers panel, choose ``Properties`` |rarr| ``Style``, change ``Single Symbol`` to ``Graduated`` and select the data you want to display.

Autocad
=======

When we originally created sDNA, we envisaged urban designers using it via Autocad.  As sDNA has become more advanced, the data handling capabilities of Autocad no longer support all the features we offer; in particular, use of user data attached to links is not possible.  We are not fixing this because the Urban Design world mostly uses BIM systems these days, and we plan to implement sDNA for BIM in future.  If you are interested in this possibility, please get in touch with us!

If you are an **Autocad Map3d** user, there is a workflow for using fully featured sDNA models that involves exporting/importing data from the free QGIS.  See our notes on `Advanced sDNA models in Autocad Map3d`_.

For other products in the Autocad family, use of basic models (without user data) is supported.  Installation is as follows:

1. On your start menu, in the sDNA program group, click on ``Register sDNA for Autocad`` (32 or 64 bit depending on your Windows installation).  You may need to provide an administrator password.

2. From the Ribbon, choose ``Manage`` |rarr| ``Load Application``

3. Under ``Startup Suite`` click ``Contents…`` then ``Add…``

4. Navigate to the place where you installed sDNA (usually ``c:\Program
   Files (x86)\sDNA``) and select the application *sdna.vlx*.

5. Click ``Close`` on the Startup Suite dialog

6. Click ``Close`` on the Load Application dialog

7. In Autocad versions 2010 onwards, load the sDNA buttons:

   a. From the Ribbon, choose ``Manage`` |rarr| ``Custom User Interface (CUI)``

   b. Locate the button for loading a Partial Customization File – the icon
      is a folder symbol with a green plus sign

   c. Navigate to the place where you installed sDNA (usually
      ``c:\Program Files (x86)\sDNA``) and select ``sDNA.cuix``

   d. Click ``OK``

8. Quit Autocad

9. When Autocad is restarted, the sDNA application will be loaded.

In Autocad 2010 onwards, sDNA will appear as a series of buttons on the
ribbon toolbar labelled “sDNA”. Simply click these buttons to load the
tools.

In older versions of Autocad, it is necessary to know the commands for
running sDNA. (These can also be used in Autocad 2010 onwards, if
preferred). Enter one of the following at the command prompt:

-  **sdnaloaditn** to load ITN data

-  **sdnaprepare** to prepare the network

-  **sdnaintegral** to analyze the network

-  **sdnacolor** or **sdnacolour** to display the results of
   **sdnaintegral**

Advanced sDNA models in Autocad Map3d
=====================================

To use the full data capabilities of sDNA from Autocad Map3d, we recommend the following workflow:

1. Export data as a shapefile.
2. Process in the free QGIS_ or by `using sDNA from the command line`_.
3. Re-import into Map3d.

This enables the use of Map3d's sophisticated 3d editing and snapping features in sDNA models.  However, please take note of the following:

* Do not edit shapefiles as a Map3d mapping layer, as this discards 3d information.  
* Instead, create your network using Autocad polylines.  
* Models can be exported from Autocad polylines to shapefiles.  Note that (1) it is necessary to manually specify export of all attached object data, (2) it is necessary to select the 3d export driver to preserve height data, and (3) care must be taken to preserve the spatial referencing.
* Shapefiles can be imported into Map3d as Autocad objects, with data attached as object data, and preserving spatial reference. 
   
.. _command line:   

Using sDNA from the command line
================================

sDNA can also be used to process shapefiles from the command line. 
Before starting, you will need to install Python, if you don’t have it
already. We have tested with versions 2.6 and 2.7; other versions may
work as well. You can download Python 2.7.3 from here_:

.. _here: http://www.python.org/ftp/python/2.7.3/python-2.7.3.msi

If your file associations are set up correctly for python (the python
installer should have done this) and the sDNA bin directory (usually
``c:\program files\sdna\bin``) has been added to your path (the sDNA
installer should have done this), then you can use command line sDNA as
follows.

The commands are ``sdnaprepare.py``, ``sdnaintegral.py``, ``sdnalearn.py`` and ``sdnapredict.py``.  Note that from the command line, various functions handled separately in QGIS and ArcGIS (geodesics, convex hulls, link measures, destination maps, network radii) are all handled by ``sdnaintegral.py``.  See :ref:`advanced_config` for more details; alternatively to learn the command for a given operation, try performing the operation from QGIS and see the command QGIS calls (shown in the algorithm dialog).

If you have ArcGIS 10.1 or later installed, then the command line
interface to sDNA will also support work on geodatabase paths. Of course
you can use sDNA from inside ArcGIS as well, but some of us like to make
batch scripts to run outside of Arc. 

Troubleshooting
---------------

If you haven't, can't or don't want to set up file associations, then
the instructions above won’t work. You will have to load python
explicitly, e.g. (assuming python is on your system path):

``python -u sdnaprepare.py --help``

(or if it isn't):

``c:\path\to\python -u sdnaprepare.py --help``

(or if neither python nor the sdna bin folder are on your path)

``c:\path\to\python -u c:\path\to\sdna\bin\sdnaprepare.py
–help``

and so on, for the other sDNA commands detailed above.

Using sDNA through the Python interface
=======================================

Those experienced in programming may want to use sDNA Prepare and sDNA Integral directly through our Python API.
This is called ``sdnapy``; the canonical example of its use can be found in ``runcalculation.py`` in the sDNA program folder.

