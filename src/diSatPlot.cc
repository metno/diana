/*
  Diana - A Free Meteorological Visualisation Tool

  $Id$

  Copyright (C) 2006 met.no

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

#include <diSatPlot.h>
#include <GL/gl.h>


// Default constructor
SatPlot::SatPlot()
  :Plot(), satdata(0), imagedata(0), previrs(1)
{
}

SatPlot::~SatPlot(){
  if (satdata) delete satdata;
  satdata = 0;
  if (imagedata) delete[] imagedata;
  imagedata=0;
}


void SatPlot::getSatAnnotation(miString &str, Colour &col){
  if (satdata->approved){
    str = satdata->annotation;
    Colour c("black");
    col = c;
  } else
    str.erase();
}



void SatPlot::getSatName(miString &str){
  if (satdata->approved){
    miString sat = satdata->satellite_name + satdata->filetype;
    sat.trim();
    str = sat;
    if (satdata->mosaic)
      str+=" MOSAIKK ";
    else
      str+= " ";
    str+=satdata->time.isoTime() ;
  } else
    str.erase();
}



void SatPlot::setData(Sat *data){
#ifdef DEBUGPRINT
  cerr << "++ SatPlot::setData() ++" << endl;
#endif
  delete satdata;
  satdata = NULL;
  satdata = data;
}

void SatPlot::getCalibChannels(vector<miString>& channels )
{
  channels.insert(channels.end(),
		  satdata->cal_channels.begin(),
		  satdata->cal_channels.end());
}

void SatPlot::values(float x, float y, vector<SatValues>& satval){

  //x og y i pixler

  float scalex = satdata->area.R().width()/(xmax-xmin);
  float scaley = satdata->area.R().height()/(ymax-ymin);

  int  xpos=(int)((x*fullrect.width()/pwidth + fullrect.x1 - xmin)*scalex);
  int  ypos=(int)((y*fullrect.height()/pheight + fullrect.y1 - ymin)*scaley);

  //xpos og ypos i satellittbilde koordinater
  if (satdata!=NULL && satdata->image != NULL && satdata->approved)
    satdata->values(xpos,ypos,satval);

}

bool SatPlot::plot(){
#ifdef DEBUGPRINT
  cerr << "++ SatPlot::plot() ++" << endl;
#endif

  if (!enabled) return false;

  if(satdata == NULL || satdata->image == NULL || !satdata->approved)
    return false;

  int nx= satdata->nx;
  int ny= satdata->ny;

  // scaling
  float scalex= float(pwidth) /fullrect.width();
  float scaley= float(pheight)/fullrect.height();

  // shown image corners in map coordinates
  float grStartx;
  float grStarty;

  // shown image corners in image coordinates
   int bmStartx;
   int bmStarty;
   int bmStopx;
   int bmStopy;

   if (!gc.getCorners(satdata->area, area, maprect,
       xmin, ymin, xmax, ymax, grStartx, grStarty,
       bmStartx, bmStopx, bmStarty, bmStopy, scalex, scaley)){
     return false;
   }

  // exit if image is outside map area
  if (maprect.x1 >= xmax || maprect.x2 <= xmin ||
      maprect.y1 >= ymax || maprect.y2 <= ymin) return true;

  // bitmap offset (out of valid area) ... seams to be in screen pixels! Not used
//  float bmxmove= (maprect.x1>xmin) ? (xstart-grStartx)*scalex : 0;
//  float bmymove= (maprect.y1>ymin) ? (ystart-grStarty)*scaley : 0;

  // for hardcopy
  float pxstart= (grStartx-maprect.x1)*scalex;
  float pystart= (grStarty-maprect.y1)*scaley;

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
    unsigned char * cimage = resampleImage(currwid,currhei,bmStartx,bmStarty,
        scalex,scaley,nx,ny);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // assure valid raster position after OpenGL transformations
    grStartx += fullrect.width() *0.0001;
    grStarty += fullrect.height()*0.0001;

    glPixelZoom(scalex,scaley);
    glPixelStorei(GL_UNPACK_SKIP_ROWS,bmStarty); //pixels
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,bmStartx);//pixels
    glPixelStorei(GL_UNPACK_ROW_LENGTH,nx);//pixels on image
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glRasterPos2f(grStartx,grStarty); //glcoord.

    //Why? Removed 2009-05-26, seems to have no effect
//    if ((bmxmove<0. || bmymove<0.) && satdata->cut>0.3) glBitmap(0,0,0.,0.,bmxmove,bmymove,NULL);

    glDrawPixels((GLint)currwid, (GLint)currhei,
	         GL_RGBA, GL_UNSIGNED_BYTE,
	         cimage);

    // for postscript output, add imagedata to glpfile
    if (hardcopy){

      psAddImage(satdata->image,
	         4*orignx*origny, orignx, origny,
	         origpxstart, origpystart, origscalex, origscaley,
	         origbmStartx, origbmStarty, bmStopx, bmStopy,
	         GL_RGBA, GL_UNSIGNED_BYTE);

      // for postscript output
      UpdateOutput();
    }

    //Reset gl
    glPixelStorei(GL_UNPACK_SKIP_ROWS,0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,0);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    glPixelStorei(GL_UNPACK_ALIGNMENT,4);
    glDisable(GL_BLEND);

#ifdef DEBUGPRINT
  cerr << "++ Returning from SatPlot::plot() ++" << endl;
#endif
  return true;
}

unsigned char * SatPlot::resampleImage(int& currwid, int& currhei,
    int& bmStartx, int& bmStarty,
    float& scalex, float& scaley,int& nx, int& ny)
{

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

bool SatPlot::getAnnotations(vector<miString>& anno){


  if (!enabled) return false;

  if(satdata == NULL || satdata->image == NULL || !satdata->approved)
    return false;

  int nanno = anno.size();

  for(int i=0; i<nanno; i++){
    if(anno[i].contains("$sat"))
      anno[i].replace("$sat", satdata->satellite_name);
  }

    //Colour table
  if (!satdata->palette || !satdata->classtable) return false;

  for(int i=0; i<nanno; i++){

    if(!anno[i].contains("table"))
      continue;

    miString satName = satdata->paletteInfo.name;
    satName.trim();

    miString endString;
    miString startString;
    if(anno[i].contains(",")){
      size_t nn = anno[i].find_first_of(",");
      endString = anno[i].substr(nn);
      startString =anno[i].substr(0,nn);
    } else {
      startString =anno[i];
    }

    if(anno[i].contains("table=")){
      miString name = startString.substr(startString.find_first_of("=")+1);
      if( name[0]=='"' )
	name.remove('"');
      name.trim();
      if(!satName.contains(name)) continue;
    }

    miString str  = "table=\"";
    str += satName;

    int n = satdata->paletteInfo.noofcl;
    //NB: better solution: get step from gui
    int step = n/50 +1;
    for (int j = 0;j< n;j+=step){
      uchar_t col[3];
      miString rgbstr;
      for( int k=0; k<3; k++){
	rgbstr+=miString(satdata->paletteInfo.cmap[k][j+1]);
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

void SatPlot::setSatAuto(bool autoFile,const miString & satellite,
			    const miString & file){
      if (satdata->satellite == satellite && satdata->filetype == file)
	satdata->autoFile=autoFile;
}
