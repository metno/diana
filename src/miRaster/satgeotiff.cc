
/*
  libmiRaster - met.no tiff interface

  Copyright (C) 2006-2022 met.no

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

using namespace satimg;

#define MILOGGER_CATEGORY "metno.GeoTiff"
#include "miLogger/miLogging.h"

namespace {
const int MAXCHANNELS = 64;

struct CloseTIFF
{
  void operator()(TIFF* tiff) { TIFFClose(tiff); }
};
struct CloseXTIFF
{
  void operator()(TIFF* tiff) { XTIFFClose(tiff); }
};
struct FreeGTIFF
{
  void operator()(GTIF* gtif) { GTIFFree(gtif); }
};
} // namespace

int metno::GeoTiff::read_diana(const std::string& infile, unsigned char* image[], int nchan, int /*chan*/[], dihead& ginfo)
{
  METLIBS_LOG_TIME();

  ginfo.noofcl = 0;
  ginfo.zsize =1;

  const int pal = head_diana(infile, ginfo);
  METLIBS_LOG_DEBUG(LOGVAL(pal));
  if (pal == -1)
    return -1;

  if (nchan == 0)
    return 1;

  std::unique_ptr<TIFF, CloseTIFF> in(TIFFOpen(infile.c_str(), "rc"));
  if (!in) {
    METLIBS_LOG_ERROR("TIFFOpen failed, probably not a TIFF file: '" << infile << "'");
    return -1;
  }

  tsample_t samplesperpixel;
  TIFFGetField(in.get(), TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);

  const int size = ginfo.xsize * ginfo.ysize;

  uint32  count;
  void    *data;
  // TIFFTAG_GDAL_METADATA 42112 defined in some projets
  // see https://www.awaresystems.be/imaging/tiff/tifftags/geo_metadata.html
  if (samplesperpixel == 1 && TIFFGetField(in.get(), 42112, &count, &data)) {
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
    std::ostringstream oss;
    oss << "T=(" << ginfo.BIr << ")+(" << ginfo.AIr << ")*C";
    ginfo.cal_ir = oss.str();
  }
  METLIBS_LOG_DEBUG(LOGVAL(ginfo.projection.getProj4Definition()) << LOGVAL(size)
                    << LOGVAL(ginfo.xsize) << LOGVAL(ginfo.ysize) << LOGVAL(ginfo.zsize)
                    << LOGVAL(samplesperpixel) << LOGVAL(ginfo.time));
  /*
   * Memory allocated for image data in this function (*image) is freed
   * in function main process.
   */
  if (ginfo.zsize > MAXCHANNELS) {
    METLIBS_LOG_ERROR("ginfo.zsize > MAXCHANNELS!");
    return(-1);
  }

  // RGBA buffer
  image[0] = (unsigned char *) malloc((size)*4);
  memset(image[0], 0, size*4);

  std::string file = infile.substr(infile.rfind("/") + 1);
  ImageCache* mImageCache = ImageCache::getInstance();

  if (!mImageCache->getFromCache(file, (uint8_t*)image[0])) {
    if (!TIFFReadRGBAImageOriented(in.get(), ginfo.xsize, ginfo.ysize, (uint32*)image[0])) {
      METLIBS_LOG_ERROR("TIFFReadRGBAImageOriented (ORIENTATION_BOTLEFT) failed: size " <<  ginfo.xsize << "," << ginfo.ysize);
    }
    // GDAL_NODATA, see https://www.awaresystems.be/imaging/tiff/tifftags/gdal_nodata.html
    if (samplesperpixel == 1 && TIFFGetField(in.get(), 42113, &count, &data) && count > 0) {
      const char* ascii = (const char*)data;
      const unsigned int nodata = atoi(ascii);
      const uint32_t rgba_nodata = 0xFF << 24 | nodata << 16 | nodata << 8 | nodata;
      for (int i=0; i<size; ++i) {
        uint32_t* rgba = (uint32_t*)(&image[0][i*4]);
        if (*rgba == rgba_nodata)
          *rgba = 0;
      }
    }
    mImageCache->putInCache(file, (uint8_t*)image[0], size*4);
  }
  return(pal);
}


int metno::GeoTiff::head_diana(const std::string& infile, dihead &ginfo)
{
  METLIBS_LOG_TIME();

  ginfo.noofcl = 0;

  std::unique_ptr<TIFF, CloseXTIFF> in(XTIFFOpen(infile.c_str(), "rc"));
  if (!in) {
    METLIBS_LOG_ERROR("XTIFFOpen failed, probably not a TIFF file: '" << infile << "'");
    return -1;
  }

  // test whether this is a color palette image
  short pmi; // PhotometricInterpretation, 3: Baseline palette-color; 2: Baseline RGB
  if (!TIFFGetField(in.get(), TIFFTAG_PHOTOMETRIC, &pmi)) {
    METLIBS_LOG_ERROR("no TIFFTAG_PHOTOMETRIC in '" << infile << "'");
    return -1;
  }
  if (pmi == PHOTOMETRIC_PALETTE) { // see tiff.h for the constants
    unsigned short int *red, *green, *blue;
    if (!TIFFGetField(in.get(), TIFFTAG_COLORMAP, &red, &green, &blue)) {
      METLIBS_LOG_ERROR("no TIFFTAG_COLORMAP in '" << infile << "'");
      return -1;
    }

    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = red[i];
      ginfo.cmap[1][i] = green[i];
      ginfo.cmap[2][i] = blue[i];
    }
  }
  // Rest is added by Niklas, for making greyscales of Storm single channel images...
  else if (pmi == PHOTOMETRIC_MINISWHITE) {
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = (255-i)*256;
      ginfo.cmap[1][i] = (255-i)*256;
      ginfo.cmap[2][i] = (255-i)*256;
    }
  } else if (pmi == PHOTOMETRIC_MINISBLACK) {
    for (int i=0; i<256; i++) {
      ginfo.cmap[0][i] = i*256;
      ginfo.cmap[1][i] = i*256;
      ginfo.cmap[2][i] = i*256;
    }
  }

  char* datetime = 0;
  if (TIFFGetField(in.get(), TIFFTAG_DATETIME, &datetime) && datetime) {
    METLIBS_LOG_DEBUG(LOGVAL(datetime));

    int year, month, day, hour, minute, sec;
    if (sscanf(datetime, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &sec) != 6) {
      if (sscanf(datetime, "%4d:%2d:%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &sec) != 6) {
         METLIBS_LOG_WARN("Invalid time in TIFFTAG_DATETIME '" << datetime << "'");
      }
    }
    if (year == 0)
      year = 2000;
    if (hour == 24) {
      hour = 0;
      ginfo.time = miutil::miTime(year, month, day, hour, minute, 0);
      ginfo.time.addDay(1);
    } else {
      ginfo.time = miutil::miTime(year, month, day, hour, minute, 0);
    }
    METLIBS_LOG_DEBUG(LOGVAL(ginfo.time));
  } else {
    METLIBS_LOG_WARN("TIFFTAG_DATETIME error");
  }

  std::unique_ptr<GTIF, FreeGTIFF> gtifin(GTIFNew(in.get()));
  if (!gtifin) {
    METLIBS_LOG_ERROR("could not init GTIF object");
    return -1;
  }

  double x_0, y_0, x_scale, y_scale;

  // Geospecific Tags
  uint32 transmatrix_size = 0;
  double* transmatrix = 0;
  const bool have_transmatrix = (TIFFGetField(in.get(), GTIFF_TRANSMATRIX, &transmatrix_size, &transmatrix) == 1) && (transmatrix_size == 16) && transmatrix;
  if (have_transmatrix) {
    x_scale = transmatrix[0];
    x_0 = transmatrix[3];
    y_scale = transmatrix[5];
    y_0 = transmatrix[7];
    if (transmatrix[1] != 0 || transmatrix[1] != 0 || transmatrix[4] != 0 || transmatrix[6] != 0) {
      METLIBS_LOG_WARN("only the linear part of GTIFF_TRANSMATRIX is supported in '" << infile << "'");
    }
  } else { // !have_transmatrix
    uint32 tiepointsize = 0;
    double* tiepoints = 0; //[6];
    const bool have_tiepoints = (TIFFGetField(in.get(), TIFFTAG_GEOTIEPOINTS, &tiepointsize, &tiepoints) == 1) && (tiepointsize >= 6) && tiepoints;

    uint32 pixscalesize = 0;
    double* pixscale = 0; //[3];
    const bool have_pixelscale = (TIFFGetField(in.get(), TIFFTAG_GEOPIXELSCALE, &pixscalesize, &pixscale) == 1) && (pixscalesize == 3) && pixscale;

    if (METLIBS_LOG_DEBUG_ENABLED()) {
      METLIBS_LOG_DEBUG("tiepointsize: " << tiepointsize);
      if (tiepoints) {
        for (uint32 i = 0; i < tiepointsize; i++)
          METLIBS_LOG_DEBUG("tiepoints[" << i << "]=" << tiepoints[i]);
      }
      METLIBS_LOG_DEBUG("pixscalesize: " << pixscalesize);
      if (pixscale) {
        for (uint32 i = 0; i < pixscalesize; i++)
          METLIBS_LOG_DEBUG("pixscale[" << i << "]=" << pixscale[i]);
      }
    }

    if (!(have_tiepoints && have_pixelscale)) {
      METLIBS_LOG_ERROR("neither GTIFF_TRANSMATRIX nor TIFFTAG_GEOTIEPOINTS+TIFFTAG_GEOPIXELSCALE in '" << infile << "'");
      return -1;
    }

    x_scale = pixscale[0];
    y_scale = -pixscale[1];
    x_0 = tiepoints[3];
    y_0 = tiepoints[4];
    METLIBS_LOG_DEBUG(LOGVAL(x_scale) << LOGVAL(y_scale) << LOGVAL(x_0) << LOGVAL(y_0));
  } // !have_transmatrix

  /* Coordinate Transformation Codes */
  short linearUnitsCode;
  if (!GTIFKeyGet(gtifin.get(), ProjLinearUnitsGeoKey, &linearUnitsCode, 0, 1)) {
    linearUnitsCode = 32767;
  }
  double unit_scale_factor = 1;

  unsigned short modeltype;
  if (!GTIFKeyGet(gtifin.get(), GTModelTypeGeoKey, &modeltype, 0, 1)) {
    METLIBS_LOG_ERROR("getting GTModelType from file");
    return -1;
  }

  if (modeltype == 32767) {
    // User defined WKT, use gdalsrsrinfo to convert to proj4
    int cit_size;
    int cit_length = GTIFKeyInfo(gtifin.get(), PCSCitationGeoKey, &cit_size, NULL);
    if (cit_length <= 0) {
      METLIBS_LOG_ERROR("Missing PCSCitationGeoKey");
      return -1;
    }
    std::unique_ptr<char[]> citation(new char[cit_length]);
    GTIFKeyGet(gtifin.get(), PCSCitationGeoKey, citation.get(), 0, cit_length);
    std::string PCSCitation(citation.get());

    miutil::replace(PCSCitation, "ESRI PE String = ", "");
    ginfo.projection.setFromWKT(PCSCitation);

  } else if (modeltype == ModelTypeGeographic) {
    /* Geographic latitude-longitude System */
    /* Assume mercartor for now */

    double GeogSemiMajorAxis = 0, GeogSemiMinorAxis = 0, GeogInvFlattening = 0;
    if (!GTIFKeyGet(gtifin.get(), GeogSemiMajorAxisGeoKey, &GeogSemiMajorAxis, 0, 1)) {
      METLIBS_LOG_INFO("No GeogSemiMajorAxisGeoKey in geotiff, set to 6.37814e+06");
      GeogSemiMajorAxis = 6.37814e6;
    }
    unsigned short GeogAngularUnits, GeogEllipsoid, GeographicType;
    if (!GTIFKeyGet(gtifin.get(), GeogAngularUnitsGeoKey, &GeogAngularUnits, 0, 1)) {
      GeogAngularUnits = Angular_Degree;
    }
    if (!GTIFKeyGet(gtifin.get(), GeogEllipsoidGeoKey, &GeogEllipsoid, 0, 1)) {
      GeogEllipsoid = 32767;
    }
    if (!GTIFKeyGet(gtifin.get(), GeogSemiMinorAxisGeoKey, &GeogSemiMinorAxis, 0, 1)) {
      GeogSemiMinorAxis = 6356752.314;
    }
    if (!GTIFKeyGet(gtifin.get(), GeogInvFlatteningGeoKey, &GeogInvFlattening, 0, 1)) {
      GeogInvFlattening = 298.257;
    }
    if (!GTIFKeyGet(gtifin.get(), GeographicTypeGeoKey, &GeographicType, 0, 1)) {
      GeographicType = 32767; // Default value
    }

    int cit_size;
    int cit_length = GTIFKeyInfo(gtifin.get(), GeogCitationGeoKey, &cit_size, NULL);
    std::string GTCitation;
    if (cit_length > 0) {
      std::unique_ptr<char[]> citation(new char[cit_length]);
      GTIFKeyGet(gtifin.get(), GeogCitationGeoKey, citation.get(), 0, cit_length);
      GTCitation = std::string(citation.get());
    }

    ginfo.projection = Projection("+proj=lonlat +ellps=WGS84 +towgs84=0,0,0 +no_defs");
    switch (GeogAngularUnits) { // see http://geotiff.maptools.org/spec/geotiff6.html#6.3.1.4
    case Angular_Degree:
      unit_scale_factor = 1;
      break;
    case Angular_Radian:
      unit_scale_factor = 180 / M_PI;
      break;
    case Angular_Arc_Minute:
      unit_scale_factor = 1 / 60;
      break;
    case Angular_Arc_Second:
      unit_scale_factor = 1 / 3600;
      break;
    case Angular_Gon:
      unit_scale_factor = 0.9;
      break;
    default:
      METLIBS_LOG_WARN("GeogAngularUnits = " << GeogAngularUnits << " are not supported");
    }

    if (METLIBS_LOG_DEBUG_ENABLED()) {
      METLIBS_LOG_DEBUG("GTIFKeyGet: GTModelTypeGeoKey = " << modeltype);
      METLIBS_LOG_DEBUG("GTIFKeyGet: GeographicType = " << GeographicType);
      METLIBS_LOG_DEBUG("GTIFKeyGet: GeogCitation = " << GTCitation);
      METLIBS_LOG_DEBUG("GTIFKeyGet: GeogAngularUnits = " << GeogAngularUnits);
      METLIBS_LOG_DEBUG("GTIFKeyGet: GeogSemiMajorAxis = " << GeogSemiMajorAxis);
      METLIBS_LOG_DEBUG("GTIFKeyGet: GeogSemiMinorAxis = " << GeogSemiMinorAxis);
      METLIBS_LOG_DEBUG("GTIFKeyGet: GeogInvFlattening = " << GeogInvFlattening);
    }

  } else if (modeltype == ModelTypeProjected) {
    unsigned short ProjectedCSType = 0;
    if (!GTIFKeyGet(gtifin.get(), ProjectedCSTypeGeoKey, &ProjectedCSType, 0, 1)) {
      METLIBS_LOG_ERROR("geotiff key ProjectedCSTypeGeoKey could not be read");
      return -1;
    }
    std::ostringstream proj4;
    if (ProjectedCSType >= 1024 && ProjectedCSType <= 32766) {
      // EPSG code; note: this range is for geotiff 1.1, in geotiff 1.0 the range was 20000 to 32766
      proj4 << "+init=epsg:" << ProjectedCSType;
    } else /*if (ProjectedCSType == 32767)*/ {
      unsigned short ProjCoordTrans = 0;
      if (!GTIFKeyGet(gtifin.get(), ProjCoordTransGeoKey, &ProjCoordTrans, 0, 1)) {
        METLIBS_LOG_ERROR("geotiff key ProjectionGeoKey could not be read");
        return -1;
      }
      if (ProjCoordTrans == CT_PolarStereographic) {
        // see http://geotiff.maptools.org/proj_list/polar_stereographic.html
        double ProjNatOriginLat, ProjScaleAtNatOrigin = 1, ProjStraightVertPoleLong;
        GTIFKeyGet(gtifin.get(), ProjNatOriginLatGeoKey, &ProjNatOriginLat, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjScaleAtNatOriginGeoKey, &ProjScaleAtNatOrigin, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjStraightVertPoleLongGeoKey, &ProjStraightVertPoleLong, 0, 1);
        // clang-format off
        proj4 << "+proj=stere"
              << " +lat_ts=" << ProjNatOriginLat
              << " +lat_0=" << (ProjNatOriginLat < 0 ? "-" : "") << "90"
              << " +lon_0=" << ProjStraightVertPoleLong
              << " +ellps=WGS84"
              << " +units=m"
              << " +k_0=" << ProjScaleAtNatOrigin;
        // clang-format on
      } else if (ProjCoordTrans == CT_Mercator) {
        // see http://geotiff.maptools.org/proj_list/mercator_1sp.html
        double ProjNatOriginLong, ProjNatOriginLat, ProjScaleAtNatOrigin;
        GTIFKeyGet(gtifin.get(), ProjNatOriginLongGeoKey, &ProjNatOriginLong, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjNatOriginLatGeoKey, &ProjNatOriginLat, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjScaleAtNatOriginGeoKey, &ProjScaleAtNatOrigin, 0, 1);
        // clang-format off
        proj4 << "+proj=merc"
              << " +lon_0=" << ProjNatOriginLong
              << " +lat_0=" << ProjNatOriginLat
              << " +k_0=" << ProjScaleAtNatOrigin;
        // clang-format on
      } else if (ProjCoordTrans == CT_Equirectangular) {
        // see http://geotiff.maptools.org/proj_list/equirectangular.html
        double ProjCenterLong, ProjCenterLat, ProjStdParallel1;
        GTIFKeyGet(gtifin.get(), ProjCenterLongGeoKey, &ProjCenterLong, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjCenterLatGeoKey, &ProjCenterLat, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjStdParallel1GeoKey, &ProjStdParallel1, 0, 1);
        // clang-format off
        proj4 << "+proj=eqc"
              << " +lat_ts=" << ProjCenterLat
              << " +lon_0=" << ProjCenterLong;
        // clang-format on
      } else if (ProjCoordTrans == CT_LambertConfConic_2SP) {
        // see http://geotiff.maptools.org/proj_list/lambert_conic_conformal_2sp.html
        double ProjStdParallel1 = 63, ProjStdParallel2 = 63, ProjFalseOriginLat = 63, ProjFalseOriginLong = 15;
        GTIFKeyGet(gtifin.get(), ProjStdParallel1GeoKey, &ProjStdParallel1, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjStdParallel2GeoKey, &ProjStdParallel2, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjFalseOriginLatGeoKey, &ProjFalseOriginLat, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjFalseOriginLongGeoKey, &ProjFalseOriginLong, 0, 1);
        // clang-format off
        proj4 << "+proj=lcc"
              << " +lat_1=" << ProjStdParallel1     // latitude of first standard parallel
              << " +lat_2=" << ProjStdParallel2     // latitude of second standard parallel
              << " +lat_0=" << ProjFalseOriginLat   // latitude of false origin
              << " +lon_0=" << ProjFalseOriginLong; // longitude of false origin
        // clang-format on
      } else if (ProjCoordTrans == CT_HotineObliqueMercatorAzimuthCenter || ProjCoordTrans == CT_ObliqueMercator_Hotine) {
        // see http://geotiff.maptools.org/proj_list/oblique_mercator.html
        // and http://geotiff.maptools.org/proj_list/hotine_oblique_mercator.html
        double ProjCenterLat = 63, ProjCenterLon = 15, ProjScaleAtCenter = 1;
        GTIFKeyGet(gtifin.get(), ProjCenterLatGeoKey, &ProjCenterLat, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjCenterLongGeoKey, &ProjCenterLon, 0, 1);
        GTIFKeyGet(gtifin.get(), ProjScaleAtCenterGeoKey, &ProjScaleAtCenter, 0, 1);
        double ProjAzimuthAngle = 63, ProjRectifiedGridAngle = 0;
        const bool have_alpha = (GTIFKeyGet(gtifin.get(), ProjAzimuthAngleGeoKey, &ProjAzimuthAngle, 0, 1) > 0);
        const bool have_gamma = (GTIFKeyGet(gtifin.get(), ProjRectifiedGridAngleGeoKey, &ProjRectifiedGridAngle, 0, 1) > 0);

        double GeogSemiMajorAxis = 6370997, GeogSemiMinorAxis = GeogSemiMajorAxis, GeogInvFlattening = 298;
        bool have_semiminor_axis = false;
        if (GTIFKeyGet(gtifin.get(), GeogSemiMajorAxisGeoKey, &GeogSemiMajorAxis, 0, 1)) {
          METLIBS_LOG_DEBUG(LOGVAL(GeogSemiMajorAxis));
        }
        if (GTIFKeyGet(gtifin.get(), GeogSemiMinorAxisGeoKey, &GeogSemiMinorAxis, 0, 1)) {
          METLIBS_LOG_DEBUG(LOGVAL(GeogSemiMinorAxis));
          have_semiminor_axis = true;
        } else if (GTIFKeyGet(gtifin.get(), GeogInvFlatteningGeoKey, &GeogInvFlattening, 0, 1)) {
          METLIBS_LOG_DEBUG(LOGVAL(GeogInvFlattening));
        }

        // clang-format off
        proj4 << "+proj=omerc"
              << " +lat_0=" << ProjCenterLat
              << " +lonc=" << ProjCenterLon;
        // clang-format on
        if (have_alpha)
          proj4 << " +alpha=" << ProjAzimuthAngle;
        if (have_gamma)
          proj4 << " +gamma=" << ProjRectifiedGridAngle;
        // clang-format off
        proj4 << " +k_0=" << ProjScaleAtCenter
              << " +a=" << GeogSemiMajorAxis
              << " +units=m +no_defs";
        // clang-format on
        if (have_semiminor_axis)
          proj4 << " +b=" << GeogSemiMinorAxis;
        else
          proj4 << " +rf=" << GeogInvFlattening;
      } else {
        METLIBS_LOG_ERROR("Projection CT " << ProjCoordTrans << " not yet supported");
        return -1;
      }

      double ProjFalseEasting = 0, ProjFalseNorthing = 0;
      GTIFKeyGet(gtifin.get(), ProjFalseEastingGeoKey, &ProjFalseEasting, 0, 1);
      GTIFKeyGet(gtifin.get(), ProjFalseNorthingGeoKey, &ProjFalseNorthing, 0, 1);
      if (ProjFalseEasting != 0)
        proj4 << " +x_0=" << ProjFalseEasting;
      if (ProjFalseNorthing != 0)
        proj4 << " +y_0=" << ProjFalseNorthing;
    }
    ginfo.projection = Projection(proj4.str());
  } else {
    METLIBS_LOG_ERROR("Grid type not supported, GTModelType = " << modeltype);
    return -1;
  }

  ginfo.Bx = x_0 * unit_scale_factor;
  ginfo.By = y_0 * unit_scale_factor;
  ginfo.Ax = x_scale * unit_scale_factor;
  ginfo.Ay = y_scale * unit_scale_factor;

  if (!TIFFGetField(in.get(), TIFFTAG_IMAGEWIDTH, &ginfo.xsize)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_IMAGEWIDTH");
  }
  if (!TIFFGetField(in.get(), TIFFTAG_IMAGELENGTH, &ginfo.ysize)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_IMAGELENGTH");
  }

  int tilesAcross = 1, tilesDown = 1;

  unsigned int tileWidth;
  if (!TIFFGetField(in.get(), TIFFTAG_TILEWIDTH, &tileWidth)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_TILEWIDTH");
    tileWidth = 0;
  }
  unsigned int tileLength;
  if (!TIFFGetField(in.get(), TIFFTAG_TILELENGTH, &tileLength)) {
    METLIBS_LOG_DEBUG("No TIFFTAG_TILELENGTH");
    tileLength = 0;
  }

  if (tileWidth != 0 && tileLength != 0) {
    tilesAcross = (ginfo.xsize + (tileWidth - 1)) / tileWidth;
    tilesDown = (ginfo.ysize + (tileLength - 1)) / tileLength;
    if (tilesAcross * tileWidth > ginfo.xsize)
      ginfo.xsize = tilesAcross * tileWidth;
    if (tilesDown * tileLength > ginfo.ysize)
      ginfo.ysize = tilesDown * tileLength;
  }

  METLIBS_LOG_DEBUG(LOGVAL(ginfo.Ax) << LOGVAL(ginfo.Ay) << LOGVAL(ginfo.Bx) << LOGVAL(ginfo.By) << LOGVAL(ginfo.projection.getProj4Definition())
                                     << LOGVAL(tileWidth) << LOGVAL(tileLength) << LOGVAL(tilesAcross) << LOGVAL(tilesDown));

  if (pmi == PHOTOMETRIC_PALETTE)
    return 2;
  else
    return 0;
}
