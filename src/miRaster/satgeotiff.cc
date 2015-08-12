
/*
  libmiRaster - met.no tiff interface

  $Id: satgeotiff.cc 2348 2009-09-07 07:01:38Z ariunaa.bertelsen@smhi.se $

  Copyright (C) 2006 met.no

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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <satgeotiff.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef GDAL
//#include <gdal/gdal.h>
#include "gdal/gdal_priv.h"
#else
#ifdef LIBGEOTIFF
#include <libgeotiff/geotiff.h>
#include <libgeotiff/geotiffio.h>
#include <libgeotiff/geo_tiffp.h>
#else
#include <geotiff/geotiff.h>
#include <geotiff/geotiffio.h>
#include <geotiff/geo_tiffp.h>
#endif
#endif

#include <tiffio.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <miLogger/logger.h>
#include <miLogger/LogHandler.h>

using namespace milogger;
using namespace miutil;
using namespace satimg;
using namespace std;

#define TIFFGetR(abgr)((abgr) & 0xff)
#define TIFFGetG(abgr)(((abgr) >> 8) & 0xff)
#define TIFFGetB(abgr)(((abgr) >> 16) & 0xff)
#define TIFFGetA(abgr)(((abgr) >> 24) & 0xff)

#define MILOGGER_CATEGORY "metno.GeoTiff"
#include "miLogger/miLogging.h"

//#define M_TIME 1
//#define DEBUG

int metno::GeoTiff::read_diana(const std::string& infile, unsigned char *image[],
    int nchan, int chan[], dihead &ginfo)
{
  //LogHandler::getInstance()->setObjectName("metno.GeoTiff.read_diana");
#ifdef DEBUG
  cout << "infile =  " << infile << endl;
  cout <<"GEOTIFF_read_diana::zsize = " <<ginfo.zsize<< endl;
  cout << "nchan =  " << nchan << endl;
#endif

#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif

  //  int i,status,size,tilesize, x,y, counter, l,w, k, red,green, blue,alpha;
  int status,size,tilesize;
  int pal;
  int compression = 0;

  TIFF *in;
  //  tdata_t buf;

  ginfo.noofcl = 0;
  ginfo.zsize =1;
  // read header
  pal = head_diana(infile,ginfo);
#ifdef DEBUG
  cerr << "pal =  " << pal << endl;
#endif
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
  // unsigned char *tileBuf;
  //  uint32 *RGBAtileBuf, *bigRGBAtileBuf;
  short planar_config;   //Planar_Configuration 284 short (1 or 2)
  //  short tileByteCount;   //TileByteCount 325 short or long (1 or 2)
  //  long*  tileOffsets;     //TileOffsets 324 long (an array
  //of offsets to the first byte of each tile)
  tsample_t samplesperpixel, bitspersample;
  // Read image data into matrix.
  //TIFFGetField(in, 256, &ginfo.xsize);
  //TIFFGetField(in, 257, &ginfo.ysize);

  if (!TIFFGetField(in, TIFFTAG_IMAGEWIDTH, &ginfo.xsize)) {
    cerr << "No TIFFTAG_IMAGEWIDTH" << endl;
  }
  if (!TIFFGetField(in, TIFFTAG_IMAGELENGTH, &ginfo.ysize)) {
    cerr << "No TIFFTAG_IMAGELENGTH" << endl;
  }
  if (!TIFFGetField(in, TIFFTAG_TILEWIDTH, &tileWidth)) {
    cerr << "No TIFFTAG_TILEWIDTH" << endl;
    tileWidth = 0;
  }
  if (!TIFFGetField(in, TIFFTAG_TILELENGTH, &tileLength)) {
    cerr << "No TIFFTAG_TILELENGTH" << endl;
    tileLength = 0;
  }


  short pmi = 0;
  status = TIFFGetField(in, 262, &pmi);

  TIFFGetField(in, 284, &planar_config);

  // tileOffsets = (long* ) malloc(80 * sizeof(long));

  //TIFFGetField(in, 324, &tileOffsets);
  //TIFFGetField(in, 325, &tileByteCount);
  TIFFGetField(in, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
  TIFFGetField(in, TIFFTAG_BITSPERSAMPLE, &bitspersample);

  if (tileWidth != 0 && tileLength != 0) {
    tilesize = tileWidth * tileLength;

    tilesAcross = (ginfo.xsize + (tileWidth -1)) / tileWidth;
    tilesDown = (ginfo.ysize + (tileLength -1)) / tileLength;
    if (tilesAcross * tileWidth >  ginfo.xsize)
      ginfo.xsize = tilesAcross * tileWidth;
    if (tilesDown * tileLength > ginfo.ysize)
      ginfo.ysize=tilesDown * tileLength;
  }
  size = ginfo.xsize * ginfo.ysize;

  // For proj4 tag, needs to be adjusted.....
  if (ginfo.projection == "Geographic")
  {
    ginfo.proj_string += " +x_0=" + miutil::from_number(ginfo.Bx*-1.);
    ginfo.proj_string += " +y_0=" + miutil::from_number((ginfo.By*-1.)+(ginfo.Ay*ginfo.ysize*1.));
  }
  else
  {
    ginfo.proj_string += " +x_0=" + miutil::from_number(ginfo.Bx*-1000.);
    ginfo.proj_string += " +y_0=" + miutil::from_number((ginfo.By*-1000.)+(ginfo.Ay*ginfo.ysize*1000.));
  }

  uint16  count;
  void    *data;
  /*
	Tag 42112: <GDALMetadata>
	<Item name="scale">-0.309189452289</Item>
	<Item name="offset">287.511418063</Item>
	</GDALMetadata>

	where T=scale*value+offset

   */
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
    //       printf("Air = %f Bir = %f  %s\n",  ginfo.AIr,  ginfo.BIr, ginfo.cal_ir.c_str() );


    image[1] = (unsigned char *) malloc(ginfo.ysize*ginfo.xsize);
    int nStrips = TIFFNumberOfStrips(in);
    int s = 0;
    int tiles = TIFFNumberOfTiles(in);
    //printf("tiles: %d %d\n", tiles, nStrips);
    //printf("TIFFTileRowSize: %d\n", TIFFTileRowSize(in));
    //    printf("TIFFVTileSize: %d\n", TIFFVTileSize (in));
    //printf("TIFFTileSize: %d\n", TIFFTileSize  (in));



    if (tiles > nStrips) {
      uint32 imageWidth,imageLength;
      uint32 x, y;
      tdata_t buf;
      //cerr << "Tiled" << endl;

      imageWidth  = ginfo.xsize;
      imageLength = ginfo.ysize;
      int tileSize = TIFFTileSize(in);
      //cerr << "imageWidth " << imageWidth << endl;
      //cerr << "imageLength " << imageLength << endl;
      //cerr << "tileWidth " << tileWidth << endl;
      //cerr << "tileLength " << tileLength << endl;
      //cerr << "tileSize " << tileSize << endl;

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
          }
          else {
            cerr << "ReadTile Failed!" << endl;
          }
        }
      }
      _TIFFfree(buf);
    }
    else {
      //cerr << "Striped" << endl;
      for (int i=0; i < nStrips; ++i) {
        s += TIFFReadEncodedStrip(in, i, image[1]+s, -1);
      }
      /*
			for (int p = 0; p < 255; p++) {
			printf("0x%x \n", image[1][p] &0xFF);
			}
       */
    }

  }

#ifdef DEBUG
  cout <<"SATGEOTIFF_read_diana::ginfo.proj_string = " << ginfo.proj_string << endl;
  cout <<"SATGEOTIFF_read_diana::pmi = " << pmi << endl;
  cout <<"SATGEOTIFF_read_diana::size = " <<size<< endl;
  cout <<"SATGEOTIFF_read_diana::xsize = " <<ginfo.xsize<< endl;
  cout <<"SATGEOTIFF_read_diana::ysize = " <<ginfo.ysize<< endl;
  cout <<"SATGEOTIFF_read_diana::zsize = " <<ginfo.zsize<< endl;
  cout <<"SATGEOTIFF_read_diana::tileWidth = " <<tileWidth<< endl;
  cout <<"SATGEOTIFF_read_diana::tileLength = " <<tileLength<< endl;
  cout <<"SATGEOTIFF_read_diana::tilesAcross = " <<tilesAcross<< endl;
  cout <<"SATGEOTIFF_read_diana::tilesDown = " <<tilesDown<< endl;
  cout <<"SATGEOTIFF_read_diana::Planar_Configuration = " <<planar_config<< endl;
  cout <<"SATGEOTIFF_read_diana::samplesperpixel = " <<samplesperpixel<< endl;
  cout <<"SATGEOTIFF_read_diana::bitspersample = " <<bitspersample<< endl;
  cout <<"SATGEOTIFF_read_diana::ginfo.time() = " <<ginfo.time<< endl;
#endif

  /*
   * Memory allocated for image data in this function (*image) is freed
   * in function main process.
   */
  if (ginfo.zsize > MAXCHANNELS) {
    printf("\n\tNOT ENOUGH POINTERS AVAILABLE TO HOLD DATA!\n");
    TIFFClose(in);
    return(-1);
  }

  // RGBA buffer
  image[0] = (unsigned char *) malloc((size)*4);
  memset(image[0], 0, size*4);

#ifdef DEBUG
  cout <<"-----SATGEOTIFF_read_diana::nchan =  " << nchan << endl;
#endif
  status = TIFFGetField(in, TIFFTAG_COMPRESSION, &compression);
#ifdef DEBUG
  cout <<"SATGEOTIFF_read_diana::status1 =  " <<status << endl;
  cout <<"SATGEOTIFF_read_diana::compression =  " <<compression << endl;
#endif
  if ( !status ) {

    compression = COMPRESSION_NONE;

#ifdef DEBUG
    cout <<"SATGEOTIFF_read_diana::compression status2 =  " <<status << endl;
    cout <<"SATGEOTIFF_read_diana::compression =  " <<compression << endl;
#endif
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
        0)) {
      cerr << " RGB READ failed" << endl;
    }
    mImageCache->putInCache(file, (uint8_t*)image[0], size*4);
  }
  TIFFClose(in);
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s1 = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  COMMON_LOG::getInstance("common").infoStream() << "image read in: " << s1 << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
  return(pal);

}


int metno::GeoTiff::head_diana(const std::string& infile, dihead &ginfo)
{
  //LogHandler::getInstance()->setObjectName("metno.GeoTiff.head_diana");
#ifdef GDAL
  GDALDataset  *poDataset;

  GDALAllRegister();

  poDataset = (GDALDataset *) GDALOpen( infile.c_str(), GA_ReadOnly );
  if  ( poDataset == NULL ) {
    cerr << "Faild to open file" << endl;
  }

  double        adfGeoTransform[6];

  printf( "Driver: %s/%s\n",
      poDataset->GetDriver()->GetDescription(),
      poDataset->GetDriver()->GetMetadataItem( GDAL_DMD_LONGNAME ) );

  printf( "Size is %dx%dx%d\n",
      poDataset->GetRasterXSize(), poDataset->GetRasterYSize(),
      poDataset->GetRasterCount() );

  if( poDataset->GetProjectionRef()  != NULL )
    printf( "Projection is `%s'\n", poDataset->GetProjectionRef() );

  if( poDataset->GetGeoTransform( adfGeoTransform ) == CE_None )
  {
    printf( "Origin = (%.6f,%.6f)\n",
        adfGeoTransform[0], adfGeoTransform[3] );

    printf( "Pixel Size = (%.6f,%.6f)\n",
        adfGeoTransform[1], adfGeoTransform[5] );
  }



  GDALClose(poDataset);
#else


#ifdef DEBUG
  cout <<"SATGEOTIFF_head_diana" << endl;
#endif
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
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

  char* datetime = NULL;
  ginfo.noofcl = 0;


  // Open TIFF files and initialize IFD

  in=XTIFFOpen(infile.c_str(), "rc");
  if (!in) {
    printf(" This is no TIFF file! (2)\n");
    return(-1);
  }

  //    Test whether this is a color palette image or not.
  //    pmi= 3 : color palette

  status = TIFFGetField(in, 262, &pmi);
  ginfo.pmi = pmi;
#ifdef DEBUG
  cout <<"SATGEOTIFF_head_diana:pmi= " <<pmi<< endl;
#endif
  if(pmi==3){
    status = TIFFGetField(in, 320, &red, &green, &blue);
#ifdef DEBUG
    cout <<"SATGEOTIFF_head_diana:TIFFGetField RGB status= " << status<< endl;
#endif
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

  TIFFGetField(in, 34736, &projsize, &projarray);

  TIFFGetField(in, TIFFTAG_DATETIME, &datetime);
  int hour,minute,day,month,year,sec;
  if (datetime != NULL) {
#ifdef DEBUG
    cout <<"-----SATGEOTIFF_head_diana: datetime = '" << datetime << "'"<< endl;
#endif
    //Format hour:min day/month-year
    // Format 2009:11:09 10:30:00
    if (sscanf(datetime, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &sec) != 6) {
      if (sscanf(datetime, "%4d:%2d:%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &sec) != 6) {
        cerr << "-----SATGEOTIFF_head_diana: Invalid time in GeoTiff tag '" << datetime << "'" << endl;
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

#ifdef DEBUG
  cout << "-----SATGEOTIFF_head_diana: time " << ginfo.time << endl;
#endif


  gtifin = GTIFNew(in);

  if (gtifin){
    /* Coordinate Transformation Codes */
    if (!GTIFKeyGet(gtifin, ProjLinearUnitsGeoKey, &linearUnitsCode, 0, 1))
    {
      // Try this as default
      linearUnitsCode = 32767;
    };
#ifdef DEBUG
    cout <<"GTIFKeyGet: ProjLinearUnitsGeoKey, linearUnitsCode = " <<linearUnitsCode<< endl;
#endif
    if (linearUnitsCode == 9001 || linearUnitsCode == 0)
      meter=1000;
#ifdef DEBUG
    for (int i=0; i<tiepointsize; i++) {
      cerr << tiepoints[i] << ", " << i << endl;
    }

    for (int i=0; i<pixscalesize; i++) {
      cerr << pixscale[i] << ", " << i << endl;
    }

    for (int i=0; i<projsize; i++) {
      cerr << projarray[i] << ", " << i << endl;
    }

#endif
    // Is tiepoints in correct unit, lets guess..
    // latitude
    double x_0, y_0, x_scale, y_scale, latrad;
    //double one_long = 60.0 * 1852.0;
    //double one_lat = 60.0 * 1852.0;

    double one_long = 111320.0;
    double one_lat = 111320.0;
    if (abs(tiepoints[3]) <= 90.0)
    {
      // Compute from degree to meter
      latrad = tiepoints[3] *3.14/180.0;
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
      cerr << "Error getting GTModelType from file" << endl;
    }

    GTIFKeyGet(gtifin, ProjLinearUnitsGeoKey, &ProjLinearUnits , 0, 1);
    // CHECK
    if (!GTIFKeyGet(gtifin, GeogSemiMajorAxisGeoKey, &GeogSemiMajorAxis , 0, 1)) {
      cerr << "No GeogSemiMajorAxisGeoKey in geotiff, set to 6.37814e+06" << endl;
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
      ginfo.projection="User defined";
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
        cerr << "This don't seem to be a Geostationary Satellite projection" << endl;
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
      ginfo.projection="Geographic";
#ifdef DEBUG		
      cerr << "GTIFKeyGet: GTModelTypeGeoKey = " <<modeltype<< endl;
      cerr << "GTIFKeyGet: GeographicType = " <<GeographicType<< endl;
      cerr << "GTIFKeyGet: GeogCitation = " <<GTCitation<< endl;
      cerr << "GTIFKeyGet: GeogAngularUnits = " << GeogAngularUnits << endl;
      cerr << "GTIFKeyGet: GeogSemiMajorAxis = " << GeogSemiMajorAxis << endl;
      cerr << "GTIFKeyGet: GeogInvFlattening = " << GeogInvFlattening << endl;
#endif

    } else  if (modeltype == ModelTypeProjected) {
      // PROJECTED
      GTIFKeyGet(gtifin, ProjCoordTransGeoKey, &ProjCoordTrans, 0, 1);
      GTIFKeyGet(gtifin, ProjectedCSTypeGeoKey, &ProjectedCSType, 0, 1); // ALL
      GTIFKeyGet(gtifin, ProjectionGeoKey, &Projection, 0, 1); // ALL
      GTIFKeyGet(gtifin, ProjFalseEastingGeoKey, &ProjFalseEasting , 0, 1);
      GTIFKeyGet(gtifin, ProjFalseNorthingGeoKey, &ProjFalseNorthing , 0, 1);
      GTIFKeyGet(gtifin, GeogPrimeMeridianLongGeoKey, &GeogPrimeMeridianLong , 0, 1);
      ginfo.projection="Projected";

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
          cerr << "Could not determine GeogSemiMinorAxis or GeogInvFlattening" << endl;
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
          cerr << "Could not determine GeogSemiMinorAxis or GeogInvFlattening" << endl;
          tmp_proj_string << " +rf=298.257";
          //return(-1);
        }
        ginfo.proj_string = tmp_proj_string.str();

      } else {
        std::cerr << "Projection " << ProjCoordTrans << " not yet supported" << std::endl;
        return false;
      }

    } else {
      cerr << "Grid type not supported, GTModelType = " << modeltype << endl;
      exit(1);
    }


#ifdef DEBUG
    cout <<"TIFFTAG_GEOTIEPOINTS tiepointsize= " <<tiepointsize << ", tiepoints= " << tiepoints<< endl;
    cout << "ginfo.Ax = " << ginfo.Ax<< endl;
    cout << "ginfo.Ay = " << ginfo.Ay<< endl;

    cout <<"TIFFTAG_GEOPIXELSCALE pixscalesize= " <<pixscalesize << ", pixscale= " << pixscale<< endl;
    cout << "ginfo.Bx = " << ginfo.Bx<< endl;
    cout << "ginfo.By = " << ginfo.By<< endl;
#endif
  } //end of gtifin

  XTIFFClose(in);
  GTIFFree(gtifin);
#ifdef DEBUG
  cerr << "satgeotiff::GEOTIFF_head_diana header proj: " << ginfo.proj_string << endl;
  cerr << "satgeotiff::GEOTIFF_head_diana header time: " << ginfo.time << endl;
  cerr << "satgeotiff::GEOTIFF_head_diana header pmi: " << pmi << endl;
#endif
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  COMMON_LOG::getInstance("common").infoStream() << "header read in: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
  if(pmi==3)
    return(2);
  else
    return(0);

#endif
}



int metno::GeoTiff::day_night(const std::string& infile)
{

  dihead sinfo;

  if (head_diana(infile, sinfo)!= 0){
    cerr <<" ssatgeotiff: Error reading met.no/TIFF file:" << infile << endl;
    return -1;
  }
  ;


  struct ucs upos;
  struct dto d;
  upos.Ax = sinfo.Ax;
  upos.Ay = sinfo.Ay;
  upos.Bx = sinfo.Bx;
  upos.By = sinfo.By;
  upos.iw = sinfo.xsize;
  upos.ih = sinfo.ysize;
  d.ho = sinfo.time.hour();
  d.mi = sinfo.time.min();
  d.dd = sinfo.time.day();
  d.mm = sinfo.time.month();
  d.yy = sinfo.time.year();

  //  int aa = selalg(d, upos, 5., -2.); //Why 5 and -2? From satsplit.c

  return 0;
}

/*
 * FUNCTION:
 * JulianDay
 *
 * PURPOSE:
 * Computes Julian day number (day of the year).
 *
 * RETURN VALUES:
 * Returns the Julian Day.
 */


int metno::GeoTiff::JulianDay(usi yy, usi mm, usi dd) {
  static unsigned short int daytab[2][13]=
  {{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
      {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}};
  unsigned short int i, leap;
  int dn;

  leap = 0;
  if ((yy%4 == 0 && yy%100 != 0) || yy%400 == 0) leap=1;

  dn = (int) dd;
  for (i=1; i<mm; i++)
  {dn += (int) daytab[leap][i];}

  return(dn);
}



/*
 * NAME:
 * selalg
 *
 * PURPOSE:
 * This file contains functions that are used for selecting the correct
 * algoritm according to the available daylight in a satellite scene.
 * The algoritm uses only corner values to chose from.
 *
 * AUTHOR:
 * Oystein Godoy, met.no/FOU, 23/07/1998
 * MODIFIED:
 * Oystein Godoy, met.no/FOU, 06/10/1998
 * Selection conditions changed. Error in nighttime test removed.
 */

#if 0
short GeoTiff::selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin) {

  int i, ch, countx, county, elem, overcompensated[2];
  int size;
  float inclination, hourangle, coszenith, sunh, xval, yval;
  float max = 0., min = 0.;
  float northings,  eastings,  latitude,  longitude;
  float RadPerDay =  0.01721420632;
  float DistPolEkv, daynr, gmttime;
  float Pi = 3.141592654;
  float TrueScaleLat = 60.;
  float CentralMer = 0.;
  float tempvar;
  float theta0, lat;
  double radian, Rp, Pl, TrueLatRad;

  radian = Pi/180.;
  TrueLatRad = TrueScaleLat*radian;
  DistPolEkv = 6378.*(1.+sin(TrueLatRad));
  size = upos.iw*upos.ih;

  /*
   * Decode day and time information for use in formulas.
   */
  daynr = (float) JulianDay((int) d.yy, (int) d.mm, (int) d.dd);
  gmttime = (float) d.ho+((float) d.mi/60.);

  theta0 = (2*Pi*daynr)/365;
  inclination = 0.006918-(0.399912*cos(theta0))+(0.070257*sin(theta0))
                    -(0.006758*cos(2*theta0))+(0.000907*sin(2*theta0))
                    -(0.002697*cos(3*theta0))+(0.001480*sin(3*theta0));

  for (i = 0; i < 2; i++) {
    overcompensated[i] = 0;
  }

  /*
    Estimates latitude and longitude for the corner pixels.
   */
  countx = 0;
  county = 0;
  for (i = 0; i < 4; i++) {
    xval = upos.Bx + upos.Ax*((float) countx + 0.5);
    yval = upos.By - fabsf(upos.Ay)*((float) county + 0.5);

    countx += upos.iw;
    if (countx > upos.iw) {
      countx = 0;
      county += upos.ih;
    }
    northings = yval;
    eastings  = xval;

    Rp = pow(double(eastings*eastings + northings*northings),0.5);

    latitude = 90.-(1./radian)*atan(Rp/DistPolEkv)*2.;
    longitude = CentralMer+(1./radian)*atan2(eastings,-northings);

    latitude=latitude*radian;
    longitude=longitude*radian;

    /*
     * Estimates zenith angle in the pixel.
     */
    lat = gmttime+((longitude/radian)*0.0667);
    hourangle = fabsf(lat-12.)*0.2618;

    coszenith = (cos(latitude)*cos(hourangle)*cos(inclination)) +
        (sin(latitude)*sin(inclination));
    sunh = 90.-(acosf(coszenith)/radian);

    if (sunh < min) {
      min = sunh;
    } else if ( sunh > max) {
      max = sunh;
    }

  }

  /*
    hmax and hmin are input variables to the function determining
    the maximum and minimum sunheights. A twilight scene is defined
    as being in between these limits. During daytime scenes all corners
    of the image have sunheights larger than hmax, during nighttime
    all corners have sunheights below hmin (usually negative values).

    Return values,
    0= no algorithm chosen (twilight)
    1= nighttime algorithm
    2= daytime algorithm.
   */
  if (max > hmax && fabs(max) > (fabs(min)+hmax)) {
    return(2);
  } else if (min < hmin && fabs(min) > (fabs(max)+hmin)) {
    return(1);
  } else {
    return(0);
  }
  return(0);
}


#endif

