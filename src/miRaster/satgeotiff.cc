
/*
  libmiRaster - met.no tiff interface

  Copyright (C) 2006-2019 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* Changed by Lisbeth Bergholt 1999
 * RETURN VALUES:
 *-1 - Something is rotten
 * 0 - Normal and correct ending - no palette
 * 1 - Only header read
 * 2 - Normal and correct ending - palette-color image
 *
 * GEOTIFF_head_diana reads only image head
 */

/*
 * PURPOSE:
 * Read and decode TIFF image files with satellite data on the
 * multichannel format.
 *
 * RETURN VALUES:
 * 0 - Normal and correct ending - no palette
 * 2 - Normal and correct ending - palette-color image
 *
 * NOTES:
 * At present only single strip images are supported.
 * Support for tile-based image with compression packbits is
 *        is implemented by SMHI/Ariunaa Bertelsen
 * REQUIRES:
 * Calls to other libraries:
 * The routine use the libtiff version 3.0 to read TIFF files.
 * The routine use the geotiff library to read geotiff header.
 *
 * AUTHOR: Oystein Godoy, DNMI, 05/05/1995, changed by LB, changed by SMHI/AB in 09/09/2009
 */

#include "diana_config.h"

#include "satgeotiff.h"

#include "ImageCache.h"

#include <puTools/miStringFunctions.h>
#include <puTools/miTime.h>

#if defined(HAVE_GEOTIFF_GEOTIFF_H)
#include <geotiff/geotiff.h>
#include <geotiff/geotiffio.h>
#include <geotiff/geo_tiffp.h>
#elif defined(HAVE_LIBGEOTIFF_GEOTIFF_H)
#include <libgeotiff/geotiff.h>
#include <libgeotiff/geotiffio.h>
#include <libgeotiff/geo_tiffp.h>
#elif defined(HAVE_GEOTIFF_H)
#include <geotiff.h>
#include <geotiffio.h>
#include <geo_tiffp.h>
#else
#error "no location for geotiff.h"
#endif

#include <tiffio.h>

#include <sstream>
#include <cstdlib>
#include <cmath>

using namespace miutil;
using namespace satimg;
using namespace std;

#define MILOGGER_CATEGORY "metno.GeoTiff"
#include "miLogger/miLogging.h"

static const int MAXCHANNELS = 16;

int metno::GeoTiff::read_diana(const std::string& infile, unsigned char* image[], int nchan, int /*chan*/[], dihead& ginfo)
{
  METLIBS_LOG_TIME();


  //  int i,status,size,tilesize, x,y, counter, l,w, k, red,green, blue,alpha;
  int status,size;
  int pal;
  int compression = 0;

  TIFF *in;
  //  tdata_t buf;

  ginfo.noofcl = 0;
  ginfo.zsize =1;
  // read header
  pal = head_diana(infile,ginfo);

  METLIBS_LOG_DEBUG("Palette: " << pal);

  if(pal == -1)
    return(-1);

  if( nchan == 0 ){
    return(1);
  }

  // Open TIFF files
  in=TIFFOpen(infile.c_str(), "rc");
  if (!in) {
    printf(" This is no TIFF file! (2)\n");
    return(-1);
  }

  unsigned int tileWidth, tileLength;
  int tilesAcross=1, tilesDown=1;

  short planar_config;   //Planar_Configuration 284 short (1 or 2)

  tsample_t samplesperpixel, bitspersample;
  // Read image data into matrix.

  if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &ginfo.xsize)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_IMAGEWIDTH");
  }
  if (!TIFFGetField(in, TIFFTAG_IMAGELENGTH, &ginfo.ysize)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_IMAGELENGTH");
  }
  if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tileWidth)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_TILEWIDTH");
    tileWidth = 0;
  }
  if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tileLength)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_TILELENGTH");
    tileLength = 0;
  }


  short pmi = 0;
  status = TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &pmi);

  TIFFGetField(in, TIFFTAG_PLANARCONFIG, &planar_config);

  TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bitspersample);

  if (tileWidth != 0 && tileLength != 0) {
    tilesAcross = (ginfo.xsize + (tileWidth -1)) / tileWidth;
    tilesDown = (ginfo.ysize + (tileLength -1)) / tileLength;
    if (tilesAcross * tileWidth >  ginfo.xsize)
      ginfo.xsize = tilesAcross * tileWidth;
    if (tilesDown * tileLength > ginfo.ysize)
      ginfo.ysize=tilesDown * tileLength;
  }
  size = ginfo.xsize * ginfo.ysize;

  uint16  count;
  void    *data;
  // TIFFTAG_GDAL_METADATA 42112 defined in some projets
  if (samplesperpixel == 1 && TIFFGetField(in, 42112, &count, &data)) {
    //    printf("Tag %d: %s, count %d0 \n", 42112, (char *)data, count);
    char* t = strstr((char *)data, "scale");
    if (t) {
      t += 7;
      ginfo.AIr = atof(t);
    }

    t = strstr((char *)data, "offset");
    if (t) {
      t += 8;
      ginfo.BIr = atof(t);
    }

    // Why not use BIr and AIr ?
    ostringstream oss;
    oss << "T=(" << ginfo.BIr << ")+(" << ginfo.AIr << ")*C";
    ginfo.cal_ir = oss.str();

    image[1] = (unsigned char *) malloc(ginfo.ysize*ginfo.xsize);
    int nStrips = TIFFNumberOfStrips(in);
    int s = 0;
    int tiles = TIFFNumberOfTiles(in);



    if (tiles > nStrips) {
      uint32 imageWidth,imageLength;
      uint32 x, y;
      tdata_t buf;

      imageWidth  = ginfo.xsize;
      imageLength = ginfo.ysize;
      int tileSize = TIFFTileSize(in);

      buf = _TIFFmalloc(tileSize);

      for (y = 0; y < imageLength; y += tileLength) {
        for (x = 0; x < imageWidth; x += tileWidth) {
          tsize_t res = TIFFReadTile(in, buf, x, y, 0, -1);
          if (res > 0) {
            // place tile buf in the larger image buffer in the right place
            // t = to, f = from
            for (int t=y*imageWidth + x, f=0;  f < tileSize; t += imageWidth, f += tileWidth) {
              memcpy(&image[1][t], (unsigned char *)buf + f, tileWidth);
            }
          } else {
            METLIBS_LOG_ERROR("TIFFReadTile Failed at tile: " << x << "," << y << " result: " << res);
          }
        }
      }
      _TIFFfree(buf);
    }
    else {
      for (int i=0; i < nStrips; ++i) {
        s += TIFFReadEncodedStrip(in, i, image[1]+s, -1);
      }
    }

  }
  if (METLIBS_LOG_DEBUG_ENABLED()) {
    METLIBS_LOG_DEBUG("ginfo.proj_string = " << ginfo.proj_string);
    METLIBS_LOG_DEBUG("pmi = " << pmi);
    METLIBS_LOG_DEBUG("size = " <<size);
    METLIBS_LOG_DEBUG("xsize = " <<ginfo.xsize);
    METLIBS_LOG_DEBUG("ysize = " <<ginfo.ysize);
    METLIBS_LOG_DEBUG("zsize = " <<ginfo.zsize);
    METLIBS_LOG_DEBUG("tileWidth = " <<tileWidth);
    METLIBS_LOG_DEBUG("tileLength = " <<tileLength);
    METLIBS_LOG_DEBUG("tilesAcross = " <<tilesAcross);
    METLIBS_LOG_DEBUG("tilesDown = " <<tilesDown);
    METLIBS_LOG_DEBUG("Planar_Configuration = " <<planar_config);
    METLIBS_LOG_DEBUG("samplesperpixel = " <<samplesperpixel);
    METLIBS_LOG_DEBUG("bitspersample = " <<bitspersample);
    METLIBS_LOG_DEBUG("ginfo.time() = " <<ginfo.time);
  }
  /*
   * Memory allocated for image data in this function (*image) is freed
   * in function main process.
   */
  if (ginfo.zsize > MAXCHANNELS) {
    METLIBS_LOG_ERROR("ginfo.zsize > MAXCHANNELS!");
    TIFFClose(in);
    return(-1);
  }

  // RGBA buffer
  image[0] = (unsigned char *) malloc((size)*4);
  memset(image[0], 0, size*4);

  status = TIFFGetField(in, TIFFTAG_COMPRESSION, &compression);
  METLIBS_LOG_DEBUG("TIFFGetField (TIFFTAG_COMPRESSION): nchan: " << nchan << " status: " << status << " compression: " << compression);
  if ( !status ) {
    compression = COMPRESSION_NONE;
  }

  std::string file = infile.substr(infile.rfind("/") + 1);
  ImageCache* mImageCache = ImageCache::getInstance();

  if (!mImageCache->getFromCache(file, (uint8_t*)image[0])) {
    // Dos not work correctly
    /* uint16 orientation;
	 if(!TIFFGetField(in,TIFFTAG_ORIENTATION, &orientation))
	 {
		 orientation = ORIENTATION_BOTLEFT;
	 }*/

    if (!TIFFReadRGBAImageOriented(in, ginfo.xsize, ginfo.ysize, (uint32*)image[0],
        ORIENTATION_BOTLEFT,
        //ORIENTATION_TOPLEFT,
        0))
    {
      METLIBS_LOG_ERROR("TIFFReadRGBAImageOriented (ORIENTATION_BOTLEFT) failed: size " <<  ginfo.xsize << "," << ginfo.ysize);
    }
    mImageCache->putInCache(file, (uint8_t*)image[0], size*4);
  }
  TIFFClose(in);
  return(pal);
}


int metno::GeoTiff::head_diana(const std::string& infile, dihead &ginfo)
{
  METLIBS_LOG_TIME();

  int status;
  TIFF *in;
  GTIF* gtifin;
  short pmi; // PhotometricInterpretation, 262, if value=3 then Baseline pallete-color
  // value = 2 then Baseline RGB
  short meter = 1;
  unsigned short int *red, *green, *blue;
  // Geospecific Tags
  short tiepointsize, pixscalesize, linearUnitsCode, projsize;
  double* tiepoints;//[6];
  double* pixscale;//[3];
  double* projarray; // [8]
  //  double projStraightVertPoleLong;

  char* datetime = 0;
  ginfo.noofcl = 0;


  // Open TIFF files and initialize IFD

  in=XTIFFOpen(infile.c_str(), "rc");
  if (!in) {
    METLIBS_LOG_ERROR("XTIFFOpen failed, probably not a TIFF file. " << infile);
    return(-1);
  }
  //    Test whether this is a color palette image or not.
  //    pmi= 3 : color palette

  status = TIFFGetField(in, TIFFTAG_PHOTOMETRIC, &pmi);

  METLIBS_LOG_DEBUG("TIFFGetField (TIFFTAG_PHOTOMETRIC) status: " << status << " pmi: " << pmi);

  if(pmi==3){
    status = TIFFGetField(in, TIFFTAG_COLORMAP, &red, &green, &blue);

    METLIBS_LOG_DEBUG("TIFFGetField (TIFFTAG_COLORMAP) status: " << status);

    if (status != 1) {
      TIFFClose(in);
      return(2);
    }
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = red[i];
      ginfo.cmap[1][i] = green[i];
      ginfo.cmap[2][i] = blue[i];
    }
    // Rest is added by Niklas, for making greyscales of Storm single channel images...
  } else if (pmi==0) {
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = (255-i)*256;
      ginfo.cmap[1][i] = (255-i)*256;
      ginfo.cmap[2][i] = (255-i)*256;
    }
  } else if (pmi==1) {
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = i*256;
      ginfo.cmap[1][i] = i*256;
      ginfo.cmap[2][i] = i*256;
    }
  }

  // Geospecific Tags
  TIFFGetField(in, TIFFTAG_GEOTIEPOINTS,  &tiepointsize, &tiepoints);
  TIFFGetField(in, TIFFTAG_GEOPIXELSCALE, &pixscalesize, &pixscale);

  TIFFGetField(in, TIFFTAG_GEODOUBLEPARAMS, &projsize, &projarray);

  status = TIFFGetField(in, TIFFTAG_DATETIME, &datetime);
  METLIBS_LOG_DEBUG("TIFFGetField (TIFFTAG_DATETIME) status: " << status);
  if ((status == 1)&&(datetime != 0)) {
    METLIBS_LOG_DEBUG("datetime: " << datetime);

    int year, month, day, hour, minute, sec;
    if (sscanf(datetime, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &sec) != 6) {
      if (sscanf(datetime, "%4d:%2d:%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &sec) != 6) {
         METLIBS_LOG_WARN("Invalid time in TIFFTAG_DATETIME '" << datetime << "'");
      }
    }
    if (hour == 24) {
      hour = 0;
      ginfo.time = miutil::miTime(year,month,day,hour,minute,0);
      ginfo.time.addDay(1);
    }
    if( year == 0 ) year = 2000;
    ginfo.time = miutil::miTime(year,month,day,hour,minute,0);
  }
  METLIBS_LOG_DEBUG("time " << ginfo.time);

  gtifin = GTIFNew(in);

  if (gtifin){
    /* Coordinate Transformation Codes */
    if (!GTIFKeyGet(gtifin, ProjLinearUnitsGeoKey, &linearUnitsCode, 0, 1))
    {
      // Try this as default
      linearUnitsCode = 32767;
    };
    METLIBS_LOG_DEBUG("GTIFKeyGet: ProjLinearUnitsGeoKey, linearUnitsCode = " <<linearUnitsCode);
    if (linearUnitsCode == 9001 || linearUnitsCode == 0)
      meter=1000;
    if (METLIBS_LOG_DEBUG_ENABLED()) {
      METLIBS_LOG_DEBUG("tiepointsize: " << tiepointsize);
      for (int i=0; i<tiepointsize; i++) {
        METLIBS_LOG_DEBUG(tiepoints[i] << ", " << i);
      }
      METLIBS_LOG_DEBUG("pixscalesize: " << pixscalesize);
      for (int i=0; i<pixscalesize; i++) {
        METLIBS_LOG_DEBUG(pixscale[i] << ", " << i);
      }
      METLIBS_LOG_DEBUG("projsize: " << projsize);
      for (int i=0; i<projsize; i++) {
        METLIBS_LOG_DEBUG(projarray[i] << ", " << i);
      }
    }

    // Is tiepoints in correct unit, lets guess..
    // latitude
    double x_0, y_0, x_scale, y_scale;
    //double one_long = 60.0 * 1852.0;
    //double one_lat = 60.0 * 1852.0;

    double one_long = 111320.0;
    double one_lat = 111320.0;
    if (abs(tiepoints[3]) <= 90.0)
    {
      // Compute from degree to meter
      x_0 = tiepoints[3]*one_lat;
    }
    else
    {
      x_0 = tiepoints[3];
    }
    // longitude
    if (abs(tiepoints[4]) <= 180.0)
    {
      // Compute from degree to meter
      // If mercartor, do not multiply with cos(latrad).
      //y_0 = tiepoints[4]*one_long*cos(latrad);
      y_0 = tiepoints[4]*one_long;
    }
    else
    {
      y_0 = tiepoints[4];
    }
    if (abs(pixscale[0]) < 1.0)
    {
      // Compute from degree to meter
      x_scale = pixscale[0]*one_lat;
    }
    else
    {
      x_scale = pixscale[0];
    }
    if (abs(pixscale[1]) < 1.0)
    {
      // Compute from degree to meter
      y_scale = pixscale[1]*one_long;
    }
    else
    {
      y_scale = pixscale[1];
    }
    ginfo.Bx = (float) (x_0/meter);
    ginfo.By = (float) (y_0/meter);
    //    }

    ginfo.Ax = (float) (x_scale/meter);
    ginfo.Ay = (float) (y_scale/meter);
    //    }

    // NIKLAS TESTS
    unsigned short Projection;
    unsigned short modeltype;
    unsigned short GeographicType;
    unsigned short ProjectedCSType;
    unsigned short ProjCoordTrans;
    unsigned short ProjLinearUnits;
    double ProjNatOriginLat;
    double ProjNatOriginLong;
    double ProjScaleAtNatOrigin ;
    double ProjFalseEasting ;
    double ProjFalseNorthing ;
    unsigned short GeogAngularUnits;
    unsigned short GeogEllipsoid;
    double GeogSemiMajorAxis = 0.0;
    double GeogSemiMinorAxis = 0.0;
    //    unsigned short GeogGeodeticDatum;
    //    unsigned short GeogLinearUnits;
    double GeogInvFlattening;
    double ProjCenterLong = 0.0;
    double ProjCenterLat = 0.0;
    //    char* GTCitationGeo;
    //    char* PCSCitationGeo;
    double ProjSatelliteHeight;
    double GeogPrimeMeridianLong;
    double ProjStraightVertPoleLong;
    double ProjStdParallel1;
    std::string PCSCitation;
    std::string GTCitation;
    int cit_size;
    int cit_length;
    char* citation;
    std::string projDesc;
    std::string projstr;

    if (!GTIFKeyGet(gtifin, GTModelTypeGeoKey, &modeltype, 0, 1) ) {
      METLIBS_LOG_ERROR("getting GTModelType from file");
    }

    GTIFKeyGet(gtifin, ProjLinearUnitsGeoKey, &ProjLinearUnits , 0, 1);
    // CHECK
    if (!GTIFKeyGet(gtifin, GeogSemiMajorAxisGeoKey, &GeogSemiMajorAxis , 0, 1)) {
      METLIBS_LOG_INFO("No GeogSemiMajorAxisGeoKey in geotiff, set to 6.37814e+06");
      GeogSemiMajorAxis = 6.37814e6;
    }
    if (!GTIFKeyGet(gtifin, GeogAngularUnitsGeoKey, &GeogAngularUnits , 0, 1)) {
      GeogAngularUnits = 32767;
    }
    if (!GTIFKeyGet(gtifin, GeogEllipsoidGeoKey, &GeogEllipsoid , 0, 1)) {
      GeogEllipsoid = 32767;
    }
    if (!GTIFKeyGet(gtifin, GeogSemiMinorAxisGeoKey, &GeogSemiMinorAxis , 0, 1)) {
      GeogSemiMinorAxis = 6356752.314;
    }
    if (!GTIFKeyGet(gtifin, GeogInvFlatteningGeoKey, &GeogInvFlattening , 0, 1)) {
      GeogInvFlattening = 298.257;
    }
    if (!GTIFKeyGet(gtifin, GeographicTypeGeoKey, &GeographicType , 0, 1)) {
      //Default value
      GeographicType = 32767;
    }

    if (modeltype == 32767) {
      // User defined, assumed GEOS SATELLITE PROJ
      cit_length = GTIFKeyInfo(gtifin,PCSCitationGeoKey,&cit_size,NULL);
      if (cit_length > 0) {
        citation = new char[cit_length];
        GTIFKeyGet(gtifin, PCSCitationGeoKey, citation , 0, cit_length);
        PCSCitation = citation;
        delete [] citation;
      }
      cit_length = GTIFKeyInfo(gtifin,GTCitationGeoKey,&cit_size,NULL);
      if (cit_length > 0) {
        citation = new char[cit_length];
        GTIFKeyGet(gtifin, GTCitationGeoKey, citation , 0, cit_length);
        GTCitation = citation;
        delete [] citation;
      }

      if (!miutil::contains(GTCitation,"Geostationary_Satellite")) {
        METLIBS_LOG_ERROR("This don't seem to be a Geostationary Satellite projection");
        return(-1);
      }
      std::string tmpStr;
      size_t pos, pos2;

      projDesc = PCSCitation;
      replace( projDesc, "ESRI PE String = ", "");
      pos = projDesc.find("PROJECTION", 0);
      projstr = projDesc.substr(pos,projDesc.size()-pos-1);

      pos = projstr.find(',', projstr.find("central_meridian", 0));
      pos2 = projstr.find(']', pos);
      tmpStr = projstr.substr(pos+1, pos2-pos-1);
      ProjCenterLong = to_double(tmpStr);

      pos = projstr.find(',', projstr.find("satellite_height", 0));
      pos2 = projstr.find(']', pos);
      tmpStr = projstr.substr(pos+1, pos2-pos-1);
      ProjSatelliteHeight = to_double(tmpStr);

      pos = projstr.find(',', projstr.find("false_easting", 0));
      pos2 = projstr.find(']', pos);
      tmpStr = projstr.substr(pos+1, pos2-pos-1);
      ProjFalseEasting = to_double(tmpStr);

      pos = projstr.find(',', projstr.find("false_northing", 0));
      pos2 = projstr.find(']', pos);
      tmpStr = projstr.substr(pos+1, pos2-pos-1);
      ProjFalseNorthing = to_double(tmpStr);

      std::stringstream tmp_proj_string;
      tmp_proj_string << "+proj=geos +lon_0=" << ProjCenterLong;
      tmp_proj_string << " +units=km +h=" << ProjSatelliteHeight;

      ginfo.proj_string = tmp_proj_string.str();

    } else if (modeltype == ModelTypeGeographic) {
      /* Geographic latitude-longitude System */
      /* Assume mercartor for now */
      cit_length = GTIFKeyInfo(gtifin,GeogCitationGeoKey,&cit_size,NULL);
      if (cit_length > 0) {
        citation = new char[cit_length];
        GTIFKeyGet(gtifin, GeogCitationGeoKey, citation , 0, cit_length);
        GTCitation = citation;
        delete [] citation;
      }

      std::string tmpStr;

      std::stringstream tmp_proj_string;
      tmp_proj_string << "+proj=merc";
      tmp_proj_string << " +a=" << GeogSemiMajorAxis;

      if (GeogSemiMinorAxis  <  1e20 )
        tmp_proj_string << " +b=" << GeogSemiMinorAxis;
      if (GeogInvFlattening < 1e20 )
        tmp_proj_string << " +rf=" << GeogInvFlattening;
      if (GeographicType == GCS_WGS_84)
        tmp_proj_string << " +ellps=WGS84 +datum=WGS84";
      tmp_proj_string << " +lon_0=0.0";
      tmp_proj_string << " +lat_ts=0.0";
      tmp_proj_string << " +no_defs";


      ginfo.proj_string = tmp_proj_string.str();
      if (METLIBS_LOG_DEBUG_ENABLED()) {
        METLIBS_LOG_DEBUG("GTIFKeyGet: GTModelTypeGeoKey = " << modeltype);
        METLIBS_LOG_DEBUG("GTIFKeyGet: GeographicType = " << GeographicType);
        METLIBS_LOG_DEBUG("GTIFKeyGet: GeogCitation = " << GTCitation);
        METLIBS_LOG_DEBUG("GTIFKeyGet: GeogAngularUnits = " << GeogAngularUnits);
        METLIBS_LOG_DEBUG("GTIFKeyGet: GeogSemiMajorAxis = " << GeogSemiMajorAxis);
        METLIBS_LOG_DEBUG("GTIFKeyGet: GeogInvFlattening = " << GeogInvFlattening);
      }

    } else  if (modeltype == ModelTypeProjected) {
      // PROJECTED
      GTIFKeyGet(gtifin, ProjCoordTransGeoKey, &ProjCoordTrans, 0, 1);
      GTIFKeyGet(gtifin, ProjectedCSTypeGeoKey, &ProjectedCSType, 0, 1); // ALL
      GTIFKeyGet(gtifin, ProjectionGeoKey, &Projection, 0, 1); // ALL
      GTIFKeyGet(gtifin, ProjFalseEastingGeoKey, &ProjFalseEasting , 0, 1);
      GTIFKeyGet(gtifin, ProjFalseNorthingGeoKey, &ProjFalseNorthing , 0, 1);
      GTIFKeyGet(gtifin, GeogPrimeMeridianLongGeoKey, &GeogPrimeMeridianLong , 0, 1);

      if (ProjCoordTrans == CT_PolarStereographic ) {
        // polar_stereographic
        GTIFKeyGet(gtifin, ProjNatOriginLatGeoKey, &ProjNatOriginLat , 0, 1);
        GTIFKeyGet(gtifin, ProjScaleAtNatOriginGeoKey, &ProjScaleAtNatOrigin , 0, 1);
        GTIFKeyGet(gtifin, ProjStraightVertPoleLongGeoKey, &ProjStraightVertPoleLong , 0, 1);

        ginfo.trueLat= ProjNatOriginLat; //(float) atof(str.cStr());
        ginfo.gridRot= 0;

        std::stringstream tmp_proj_string;
        tmp_proj_string << "+proj=stere";
        tmp_proj_string << " +lat_ts=" << ProjNatOriginLat;
        if ( ProjNatOriginLat < 0 ) {
          tmp_proj_string << " +lat_0=-90";
        } else {
          tmp_proj_string << " +lat_0=90";
        }
        tmp_proj_string << " +lon_0=" << ProjStraightVertPoleLong;
        tmp_proj_string << " +k_0=" << ProjScaleAtNatOrigin;
        tmp_proj_string << " +units=km";
        tmp_proj_string << " +a=" << GeogSemiMajorAxis;
        // TODO -  is b and rf always present ???
        if (GeogSemiMinorAxis  <  1e20 )
          tmp_proj_string << " +b=" << GeogSemiMinorAxis;
        if (GeogInvFlattening < 1e20 )
          tmp_proj_string << " +rf=" << GeogInvFlattening;
        ginfo.proj_string = tmp_proj_string.str();

      } else if (ProjCoordTrans == CT_Mercator ) {
        // tmerc
        GTIFKeyGet(gtifin, ProjNatOriginLongGeoKey, &ProjNatOriginLong , 0, 1);
        GTIFKeyGet(gtifin, ProjNatOriginLatGeoKey, &ProjNatOriginLat , 0, 1);
        GTIFKeyGet(gtifin, ProjScaleAtNatOriginGeoKey, &ProjScaleAtNatOrigin , 0, 1);

        std::stringstream tmp_proj_string;
        tmp_proj_string << "+proj=merc";
        //tmp_proj_string << " +lat_ts=" << ProjNatOriginLat; // Not used for now
        tmp_proj_string << " +lat_0=" << ProjNatOriginLat;
        tmp_proj_string << " +lon_0=" << ProjNatOriginLong;
        tmp_proj_string << " +k_0=" << ProjScaleAtNatOrigin;
        tmp_proj_string << " +units=km";
        tmp_proj_string << " +a=" << GeogSemiMajorAxis;
        // TODO -  is b and rf always present ???
        if (GeogSemiMinorAxis  <  1e20 )
          tmp_proj_string << " +b=" << GeogSemiMinorAxis;
        else if (GeogInvFlattening < 1e20 )
          tmp_proj_string << " +rf=" << GeogInvFlattening;
        else {
          METLIBS_LOG_ERROR("Could not determine GeogSemiMinorAxis or GeogInvFlattening");
          tmp_proj_string << " +rf=298.257";
          //return(-1);
        }
        ginfo.proj_string = tmp_proj_string.str();

      } else if (ProjCoordTrans == CT_Equirectangular ) {
        // eqc
        GTIFKeyGet(gtifin, ProjCenterLongGeoKey, &ProjCenterLong , 0, 1);
        GTIFKeyGet(gtifin, ProjCenterLatGeoKey, &ProjCenterLat , 0, 1);
        GTIFKeyGet(gtifin, ProjStdParallel1GeoKey, &ProjStdParallel1 , 0, 1);

        std::stringstream tmp_proj_string;
        tmp_proj_string << "+proj=eqc";
        tmp_proj_string << " +lat_ts=" << ProjStdParallel1; // Not used for now
        tmp_proj_string << " +lat_0=" << ProjCenterLat;
        tmp_proj_string << " +lon_0=" << ProjCenterLong;
        tmp_proj_string << " +units=km";
        tmp_proj_string << " +a=" << GeogSemiMajorAxis;
        // TODO -  is b and rf always present ???
        if (GeogSemiMinorAxis  <  1e20 )
          tmp_proj_string << " +b=" << GeogSemiMinorAxis;
        else if (GeogInvFlattening < 1e20 )
          tmp_proj_string << " +rf=" << GeogInvFlattening;
        else {
          METLIBS_LOG_ERROR("Could not determine GeogSemiMinorAxis or GeogInvFlattening");
          tmp_proj_string << " +rf=298.257";
          //return(-1);
        }
        ginfo.proj_string = tmp_proj_string.str();

      } else {
        METLIBS_LOG_ERROR("Projection " << ProjCoordTrans << " not yet supported");
        return -1;
      }

    } else {
      METLIBS_LOG_ERROR("Grid type not supported, GTModelType = " << modeltype);
      return -1;
    }
    METLIBS_LOG_DEBUG("ginfo.Ax = " << ginfo.Ax << " ginfo.Ay = " << ginfo.Ay << " ginfo.Bx = " << ginfo.Bx << " ginfo.By = " << ginfo.By);
  } //end of gtifin

  XTIFFClose(in);
  GTIFFree(gtifin);

  METLIBS_LOG_DEBUG("projstring: " << ginfo.proj_string);

  if(pmi==3)
    return(2);
  else
    return(0);
}

int metno::GeoTiff::day_night(const std::string& infile)
{
  dihead sinfo;

  if (head_diana(infile, sinfo)!= 0){
    METLIBS_LOG_ERROR("Error reading met.no/TIFF file:" << infile);
    return -1;
  }

  return 0;
}
