# DIANA

Diana is the core visualisation tool from MET Norway (Norwegian Meteorological
Institute) and is distributed under the GPL license. See gpl.txt for details
concerning the GPL license.

Diana is a graphical viewer and editor developed for use with meteorological
and oceanographic data. It uses Qt for the graphics the user interface.

Diana shows fields, observations and satellite and radar images on a map.
2D-trajectories can be computed from wind and ocean current fields. The tool
also displays vertical profiles (soundings), vertical crossections, and wave
spectrum data in separate windows, all from preprocessed data. Preprocessing
software is however not a part of Diana. The editor tools consist of a field
editor and a drawing tool for fronts, symbols and area types.

Diana supports multiple languages via the Qt Linguist system. Additional
languages can be added by preparing new `share/diana/lang/diana_XX.ts` files
(using the Qt Linguist program) and updating `src/CMakeLists.txt`.

In addition to the interactive tool, there is also a command line version of
Diana for batch plotting (`bdiana`).

## Installation

Diana 3.44 and above is built with
[cmake](https://www.cmake.org). Example build steps are:

    SRC=/path/to/diana/source
    BLD=/path/to/diana/build
    INS=/path/to/diana/install
    
    cd "$BLD"
    cmake \
        -DCMAKE_INSTALL_PREFIX="$INS" \
        -DENABLE_DIANA_OMP=1 \
        -DENABLE_PERL=1 \
        -DENABLE_OBS_BUFR=1 \
        -DENABLE_GEOTIFF=1 \
        "$SRC"
    
    cmake --build . --target all
    
    export CTEST_OUTPUT_ON_FAILURE=1
    cmake --build . --target test
    
    cmake --build . --target install

You might want to pass other options to cmake depending on the
features you want to enable in Diana. Please consult the
`CMakeLists.txt` files.

To read maps from ESRI shape files, libshp and GDAL (`gdalsrsinfo`)
are required.

For video export, `avconv` is required to merge single frames into a
video.

If you use an IDE like [QtCreator](https://wiki.qt.io/Qt_Creator), you
might want to change the generator for `cmake` with the `-G` option
(see `cmake --help`), e.g. `-G "CodeBlocks - Unix Makefiles"` or
`-G "CodeBlocks - Ninja"`.


## Running

### Running bdiana

In order to run `bdiana` on a server without display, you need to tell
Qt not to try to open a display. This might be achieved by issuing the
commands:

    unset DISPLAY
    export QT_QPA_FONTDIR=/usr/share/fonts/truetype
    export QT_QPA_PLATFORM=offscreen

before starting `bdiana`.

## File formats for diana

The current version supports following file formats:

* Fields are read via [fimex](http://fimex.met.no)
  * MetnoFieldFile (proprietary met.no)
  * NetCDF
  * GRIB
* Observations
  * MetnoObs (proprietary met.no)
  * BUFR (using libemos or [ecCodes](https://confluence.ecmwf.int/display/ECC/ecCodes+Home))
  * ASCII
* Image (satellite, radar)
  * mitiff (proprietary met.no)
  * geotiff
  * HDF5
* Prognostic sounding
  * NetCDF
* vertical crossections
  * NetCDF (met.no structure)
* Wave spectrum
  * NetCDF (met.no structure)
* Maps
  * ESRI shape (via libshp and GDAL)
  * ASCII
  * metno-triangle-format

## Resource files

Some files included with Diana are obtained from third parties and are made
available under distinct licenses.

The weather symbol files in share/diana/images/symbols were obtained from
[OGCMetOceanDWG](https://github.com/OGCMetOceanDWG/WorldWeatherSymbols) and are licensed under
the Creative Commons Attribution 3.0 Unported (CC BY 3.0) license. See the
http://creativecommons.org/licenses/by/3.0/ page for the license terms.

## Contact

Norwegian Meteorological Institute (MET Norway)  
Box 43 Blindern  
0313 OSLO  
NORWAY  

Email: [diana@met.no](email:diana@met.no)
