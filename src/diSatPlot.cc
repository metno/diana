/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no

  This file is part of Diana

  Diana is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  Diana is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Diana; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diSatPlot.h>

#include <puTools/miStringFunctions.h>

#include <QtGlobal>
#if defined(USE_PAINTGL)
#include <QtGui>
#include <QtSvg>
#include "PaintGL/paintgl.h"
#else
#include <QtOpenGL>
#endif

#define MILOGGER_CATEGORY "diana.MainWindow"
#include <miLogger/miLogging.h>

#define NO_TEXTTURE

using namespace::miutil;
using namespace std;

SatPlot::SatPlot()
 :Plot(), imagedata(0), previrs(1), satdata(0)
{
    texture = 0;
    hasTexture = false;
}

SatPlot::~SatPlot(){
  if (satdata) delete satdata;
  satdata = 0;
  if (imagedata) delete[] imagedata;
  imagedata=0;
  if (hasTexture) {
    glDeleteTextures( 1, &texture );
  }
}


void SatPlot::getSatAnnotation(std::string &str, Colour &col)
{
  if (satdata->approved){
    str = satdata->annotation;
    Colour c("black");
    col = c;
  } else
    str.erase();
}



void SatPlot::getSatName(std::string &str)
{
  if (satdata->approved){
    std::string sat = miutil::trimmed(satdata->satellite_name + satdata->filetype);
    str = sat;
    if (satdata->mosaic)
      str+=" MOSAIKK ";
    else
      str+= " ";
    str+=satdata->time.isoTime();
  } else
    str.erase();
}



void SatPlot::setData(Sat *data){
  METLIBS_LOG_SCOPE();
  delete imagedata;
  imagedata = NULL;
  delete satdata;
  satdata = NULL;
  satdata = data;
}
void SatPlot::clearData(){
  METLIBS_LOG_SCOPE();
  delete imagedata;
  imagedata = NULL;
}

void SatPlot::getCalibChannels(std::vector<std::string>& channels)
{
  channels.insert(channels.end(),
      satdata->cal_channels.begin(),
      satdata->cal_channels.end());
}

void SatPlot::values(float x, float y, std::vector<SatValues>& satval)
{
  if (not isEnabled())
    return;

  if ((satdata == NULL)||
      (satdata->image == NULL)||
      (!satdata->approved)) {
    return;
  }

  //x, y in map coordinates
  //Convert to satellite proj coordiantes
  getStaticPlot()->MapToProj(satdata->area.P(), 1, &x, &y);
  // convert to satellite pixel
  int xpos = x/satdata->gridResolutionX;
  int ypos = y/satdata->gridResolutionY;

  satdata->values(xpos,ypos,satval);
}

void SatPlot::plot(PlotOrder porder)
{
  METLIBS_LOG_TIME();

  if (porder != SHADE_BACKGROUND)
    return;

  if (!isEnabled())
    return;

  if (!satdata || !satdata->image || !satdata->approved)
    return;

  if (!getStaticPlot()->getMapArea().P().isAlmostEqual(satdata->area.P()))
    plotFillcell();
  else
    plotPixmap();
}

bool SatPlot::plotFillcell()
{

  int nx = satdata->nx;
  int ny = satdata->ny;

  int ix1, ix2, iy1, iy2;
  float *x, *y;

  //todo: reduce resolution when zooming out
//  int factor = getStaticPlot()->getPlotSize().width()/nx/2000;

  float cx[2], cy[2];
  cx[0] = satdata->area.R().x1;
  cy[0] = satdata->area.R().y1;
  cx[1] = satdata->area.R().x2;
  cy[1] = satdata->area.R().y2;
  getStaticPlot()->ProjToMap(satdata->area.P(), 2, cx, cy);

  double gridW = nx*getStaticPlot()->getPhysToMapScaleX()/double(cx[1] - cx[0]);
  double gridH = ny*getStaticPlot()->getPhysToMapScaleY()/double(cy[1] - cy[0]);
  int factor = std::max(1, int(std::min(gridW, gridH)));

  int rnx = nx / factor;
  int rny = ny / factor;
  getStaticPlot()->gc.getGridPoints(satdata->area,satdata->gridResolutionX * factor, satdata->gridResolutionY * factor,
      getStaticPlot()->getMapArea(), getStaticPlot()->getMapSize(), true,
      rnx, rny, &x, &y, ix1, ix2, iy1, iy2);
  if (ix1>ix2 || iy1>iy2)
    return false;

  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glBegin(GL_QUADS);
  vector<float>::iterator it;
  for (int iy=iy1; iy<=iy2-1; iy++) {
    for (int ix = ix1; ix <= ix2-1; ix++) {
      float x1 = x[iy * (rnx+1) + ix];
      float x2 = x[iy * (rnx+1) + (ix+1)];
      float x3 = x[(iy+1) * (rnx+1) + (ix+1)];
      float x4 = x[(iy+1) * (rnx+1) + ix];
      float y1 = y[iy * (rnx+1) +ix];
      float y2 = y[(iy) * (rnx+1) +(ix+1)];
      float y3 = y[(iy+1) * (rnx+1) +(ix+1)];
      float y4 = y[(iy+1) * (rnx+1) +(ix)];

      char f1 = satdata->image[(ix * factor + (iy * (nx) * factor))*4];
      char f2 = satdata->image[(ix * factor + (iy * (nx) * factor))*4+1];
      char f3 = satdata->image[(ix * factor + (iy * (nx) * factor))*4+2];
      char f4 = satdata->image[(ix * factor + (iy * (nx) * factor))*4+3];
      if(int(f4)==0  ) {
        continue;
      }
      glColor4ub(f1,f2,f3,f4);
      glVertex2f(x1, y1);
      // lower-right corner of gridcell
      glVertex2f(x2, y2);
      // upper-right corner of gridcell
      glVertex2f(x3, y3);
      // upper-left corner of gridcell
      glVertex2f(x4, y4);
    }
  }
  
  glEnd();
  glDisable(GL_BLEND);

  return true;
}

bool SatPlot::plotPixmap()
{
  int nx = satdata->nx;
  int ny = satdata->ny;

  int npos = 1;

  //Member variables, used in values().
  //Corners of total image (map coordinates)
  xmin = 0.;
  ymin = 0.;
  if (!getStaticPlot()->ProjToMap(satdata->area.P(), 1, &xmin, &ymin))
    return false;
  xmax = nx* satdata->gridResolutionX;
  ymax = ny* satdata->gridResolutionY;

  if (!getStaticPlot()->ProjToMap(satdata->area.P(), 1, &xmax, &ymax))
    return false;

  // exit if image is outside map area
  if (getStaticPlot()->getMapSize().x1 >= xmax || getStaticPlot()->getMapSize().x2 <= xmin ||
      getStaticPlot()->getMapSize().y1 >= ymax || getStaticPlot()->getMapSize().y2 <= ymin)
    return true;

  // scaling
  float scalex = 1/getStaticPlot()->getPhysToMapScaleX();
  float scaley = 1/getStaticPlot()->getPhysToMapScaleY();

  // Corners of image shown (map coordinates)
  float grStartx = std::max(getStaticPlot()->getMapSize().x1, xmin);
  float grStarty = std::max(getStaticPlot()->getMapSize().y1, ymin);
//  float grStopx = (maprect.x2<xmin) ? maprect.x2 : xmax;
//  float grStopy = (maprect.y2<ymin) ? maprect.y2 : ymax;

  // Corners of total image (image coordinates)
  float x1= getStaticPlot()->getMapSize().x1;
  float y1= getStaticPlot()->getMapSize().y1;
  if (!getStaticPlot()->MapToProj(satdata->area.P(), 1, &x1, &y1))
    return false;
  float x2= getStaticPlot()->getMapSize().x2;
  float y2= getStaticPlot()->getMapSize().y2;
  if (!getStaticPlot()->MapToProj(satdata->area.P(), 1, &x2, &y2))
    return false;
  x1/=satdata->gridResolutionX;
  x2/=satdata->gridResolutionX;
  y1/=satdata->gridResolutionY;
  y2/=satdata->gridResolutionY;

  // Corners of image shown (image coordinates)
  int bmStartx= (getStaticPlot()->getMapSize().x1>xmin) ? int(x1) : 0;
  int bmStarty= (getStaticPlot()->getMapSize().y1>ymin) ? int(y1) : 0;
  int bmStopx=  (getStaticPlot()->getMapSize().x2<xmax) ? int(x2) : nx-1;
  int bmStopy=  (getStaticPlot()->getMapSize().y2<ymax) ? int(y2) : ny-1;

  // lower left corner of displayed image part, in map coordinates
  // (part of lower left pixel may well be outside screen)
  float xstart = bmStartx*satdata->gridResolutionX;
  float ystart = bmStarty*satdata->gridResolutionY;
  if (!getStaticPlot()->ProjToMap(satdata->area.P(), 1, &xstart, &ystart))
    return false;

  //Strange, but needed
  float bmxmove= (getStaticPlot()->getMapSize().x1>xmin) ? (xstart-grStartx)*scalex : 0;
  float bmymove= (getStaticPlot()->getMapSize().y1>ymin) ? (ystart-grStarty)*scaley : 0;

  // for hardcopy
  float pxstart= (xstart-getStaticPlot()->getMapSize().x1)*scalex;
  float pystart= (ystart-getStaticPlot()->getMapSize().y1)*scaley;

  // update scaling with ratio image to map (was map to screen pixels)
  scalex*= satdata->gridResolutionX;
  scaley*= satdata->gridResolutionY;

  // width of image (pixels)
  int currwid= bmStopx - bmStartx + 1;  // use pixels in image
  int currhei= bmStopy - bmStarty + 1;  // use pixels in image

  // keep original copies (for hardcopy purposes)
  int orignx =       nx;
  int origny =       ny;
  float origscalex=  scalex;
  float origscaley=  scaley;
  int origbmStartx=  bmStartx;
  int origbmStarty=  bmStarty;
  float origpxstart= pxstart;
  float origpystart= pystart;

  /*
    If rasterimage wider than OpenGL-maxsizes: For now, temporarily resample image..
    cImage: Pointer to imagedata, either sat_image or resampled data
   */
#ifdef NO_TEXTTURE
  unsigned char * cimage = resampleImage(currwid,currhei,bmStartx,bmStarty,
					 scalex,scaley,nx,ny);

  // always needed (if not, slow oper...) ??????????????
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // assure valid raster position after OpenGL transformations
  grStartx += getStaticPlot()->getPlotSize().width() *0.0001;
  grStarty += getStaticPlot()->getPlotSize().height()*0.0001;

  glPixelZoom(scalex,scaley);
  glPixelStorei(GL_UNPACK_SKIP_ROWS,bmStarty); //pixels
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,bmStartx);//pixels
  glPixelStorei(GL_UNPACK_ROW_LENGTH,nx);//pixels on image
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glRasterPos2f(grStartx,grStarty); //glcoord.

  //Strange, but needed
  if (bmxmove<0. || bmymove<0.) glBitmap(0,0,0.,0.,bmxmove,bmymove,NULL);

  glDrawPixels((GLint)currwid, (GLint)currhei,
      GL_RGBA, GL_UNSIGNED_BYTE,
      cimage);
  //Reset gl
  glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glPixelStorei(GL_UNPACK_ALIGNMENT,4);
  glDisable(GL_BLEND);
#else  
  bool wrap =true;

  // allocate a texture name
  if (!hasTexture) {
    glGenTextures( 1, &texture );
    //cerr << "Gentext: " << texture << endl;
  }

  // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glBindTexture(GL_TEXTURE_2D, texture);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, 
                   GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, 
                   GL_NEAREST);

   // Generate The Texture
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nx, ny, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, satdata->image);

   /*
      int t[4];
      glGetIntegerv(GL_VIEWPORT, t);
      cerr << "GL_VIEWPORT" << endl;
      cerr<< t[0] << " " << t[1] << " " << t[2] << " " << t[3] << endl;
      cerr << "nx " << nx<<" ny " << ny<< endl;
   */
    //Enable texturing on all models for now on.

   //  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glEnable(GL_BLEND);
   glEnable(GL_TEXTURE_2D);
   //   glEnable(GL_ALPHA_TEST);

   //   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
   glBindTexture(GL_TEXTURE_2D, texture);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_LIGHTING);
   glDisable(GL_FOG);
   glDisable(GL_DEPTH_TEST);
   glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);

   glPushMatrix();

   glBegin(GL_QUADS);
      float w = nx -1;
      float h = ny -1;
   

     //cerr << "nx " << nx << " ny " << ny << endl;
     //cerr << "grStartx " << grStartx << " grStarty " << grStarty << " grStopx " << grStopx << " grStopy " << grStopy << endl;
     //cerr << "bmStartx/nx bmStarty/ny bmStopx/nx bmStopy/ny" << endl;
     //cerr << bmStartx << " " << bmStarty << " " << bmStopx << " " << bmStopy << endl;
     //cerr << bmStartx/w << " " << bmStarty/h << " " << bmStopx/w << " " << bmStopy/h << endl;
   

      glTexCoord2f(bmStartx/w, bmStarty/h); glVertex3f(grStartx, grStarty, 0);
      glTexCoord2f(bmStopx/w,  bmStarty/h); glVertex3f(grStopx, grStarty, 0);
      glTexCoord2f(bmStopx/w,  bmStopy/h);  glVertex3f(grStopx, grStopy, 0);
      glTexCoord2f(bmStartx/w, bmStopy/h);  glVertex3f(grStartx, grStopy, 0);
    
    glEnd();
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
#endif

  // for postscript output, add imagedata to glpfile
  if (getStaticPlot()->hardcopy){

    getStaticPlot()->psAddImage(satdata->image,
        4*orignx*origny, orignx, origny,
        origpxstart, origpystart, origscalex, origscaley,
        origbmStartx, origbmStarty, bmStopx, bmStopy,
        GL_RGBA, GL_UNSIGNED_BYTE);

    // for postscript output
    getStaticPlot()->UpdateOutput();
  }
  return true;
}

unsigned char * SatPlot::resampleImage(int& currwid, int& currhei,
    int& bmStartx, int& bmStarty,
    float& scalex, float& scaley,int& nx, int& ny)
{
  METLIBS_LOG_TIME(LOGVAL(scalex));
  unsigned char * cimage;
  int irs= 1;            // resample-size

  GLint maxdims[2];      // find OpenGL maximums
  glGetIntegerv(GL_MAX_VIEWPORT_DIMS,maxdims);
  int maxww= maxdims[0];
  int maxhh= maxdims[1];
  int orignx = nx;
  if (  currwid > maxww || currhei > maxhh ){

    if ( (currwid - maxww) > (currhei - maxhh) ) {
      irs = (currwid / maxww) + 1;
    } else {
      irs = (currhei / maxhh) + 1;
    }

    currwid /= irs;
    currhei /= irs;
    bmStartx/= irs;
    bmStarty/= irs;
    nx      /= irs;
    ny      /= irs;
    scalex  *= irs;
    scaley  *= irs;


    // check if correct resampling already available..
    if (irs != previrs || !imagedata){
      //  cerr << " diSatPlot::plot() resampling image:" << irs << endl;
      previrs=irs;
      if (imagedata) delete[] imagedata;
      imagedata = new unsigned char [4*nx*ny];
      for (int iy=0; iy<ny; iy++)
        for (int ix=0; ix<nx; ix++){
          int newi = (iy*nx + ix)*4;
          int oldi = (irs*iy*orignx + irs*ix)*4;
          imagedata[newi + 0] = satdata->image[oldi + 0];
          imagedata[newi + 1] = satdata->image[oldi + 1];
          imagedata[newi + 2] = satdata->image[oldi + 2];
          imagedata[newi + 3] = satdata->image[oldi + 3];
        }
    }
    // Point to resampled data
    cimage = imagedata;
  } else {
    // No resampling: use original image
    cimage = satdata->image;
  }

  return cimage;

}

bool SatPlot::getAnnotations(vector<string>& anno)
{
  if (!isEnabled())
    return false;

  if(satdata == NULL || satdata->image == NULL || !satdata->approved)
    return false;

  int nanno = anno.size();

  for(int i=0; i<nanno; i++){
    if (miutil::contains(anno[i], "$sat"))
      miutil::replace(anno[i], "$sat", satdata->satellite_name);
  }

  //Colour table
  if (!satdata->palette || !satdata->classtable)
    return false;

  for(int i=0; i<nanno; i++){

    if(! miutil::contains(anno[i], "table"))
      continue;

    std::string satName = satdata->paletteInfo.name;
    miutil::trim(satName);

    std::string endString;
    std::string startString;
    if (miutil::contains(anno[i], ",")){
      size_t nn = anno[i].find_first_of(",");
      endString = anno[i].substr(nn);
      startString =anno[i].substr(0,nn);
    } else {
      startString =anno[i];
    }

    if (miutil::contains(anno[i], "table=")) {
      std::string name = startString.substr(startString.find_first_of("=")+1);
      if( name[0]=='"' )
        miutil::remove(name, '"');
      miutil::trim(name);
      if (not miutil::contains(satName, name))
        continue;
    }

    std::string str  = "table=\"";
    str += satName;

    int n = satdata->paletteInfo.noofcl;
    //NB: better solution: get step from gui
    int step = n/50 +1;
    for (int j = 0;j< n;j+=step){
      std::string rgbstr;
      for( int k=0; k<3; k++){
        rgbstr+=miutil::from_number(satdata->paletteInfo.cmap[k][j+1]);
        if(k<2) rgbstr+=":";
      }
      str +=";";
      str +=rgbstr;
      str +=";;";
      str +=satdata->paletteInfo.clname[j];

    }

    str += "\" ";
    str += endString;

    anno.push_back(str);
  }

  return true;
}

void SatPlot::setSatAuto(bool autoFile,const string& satellite, const string& file)
{
  if (satdata->satellite == satellite && satdata->filetype == file)
    satdata->autoFile=autoFile;
}
