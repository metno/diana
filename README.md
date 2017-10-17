# DIANA

Diana is the core visualisation tool for StormGeo and is based on latest 
release of DIANA from MET Norway (Norwegian Meteorological Institute) and is 
distributed under the GPL license. See gpl.txt for details concerning the 
GPL license.

Diana is a graphical viewer and editor developed for use with meteorological
and oceanographic data. It uses OpenGL and/or Qt for the graphics and Qt for
the user interface.

Diana shows fields, observations and satellite and radar images on a map.
2D-trajectories can be computed from wind and ocean current fields. The tool
also displays vertical profiles (soundings), vertical crossections, and wave
spectrum data in separate windows, all from preprocessed data. Preprocessing
software is however not a part of Diana. The editor tools consist of a field
editor and a drawing tool for fronts, symbols and area types.

Diana supports multiple languages via the Qt Linguist system. Additional
languages can be added by preparing new diana/lang/diana_XX.ts files (using
the Qt Linguist program).

In addition to the interactive tool, there is also a command line version of
Diana for batch plotting (bdiana). This can be built against embedded
versions of the Qt libraries in order to run on systems without displays,
such as servers.

## Installation

Please see https://diana.wiki.met.no/doku.php

## File formats for diana

The current version supports following file formats:

* Fields
  * MetnoFieldFile (proprietary met.no)
  * NetCDF
  * Grib
* Observations
  * MetnoObs (proprietary met.no)
  * BUFR
  * ascii
* Image (satellite, radar)
  * mitiff (proprietary met.no)
  * HDF5
* Prognostic sounding
  * NetCDF
* vertical crossections
  * NetCDF (met.no structure)
* Wave spectrum
  * NetCDF (met.no structure)
* Maps
  * shape
  * ascii
  * metno-format (documentation available)

## Resource files

Some files included with Diana are obtained from third parties and are made
available under distinct licenses.

The weather symbol files in share/diana/images/symbols were obtained from
https://github.com/OGCMetOceanDWG/WorldWeatherSymbols and are licensed under
the Creative Commons Attribution 3.0 Unported (CC BY 3.0) license. See the
http://creativecommons.org/licenses/by/3.0/ page for the license terms.

## Contact

StormGeo Group HQ
Nordre Nøstekaien 1
5011 Bergen
NORWAY

email:info@stormgeo.com
