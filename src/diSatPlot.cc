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

  float cvsat2map[6],cvmap2sat[6];

  int mapconvert,ix1,ix2,iy1,iy2;
  if(!gc.getGridConversion(satdata->area, area, maprect, mapconvert,
		           cvsat2map, cvmap2sat, ix1, ix2, iy1, iy2))
	return false;
  if (mapconvert==1) {
    // exit if rotation or x,y scaling<0
    if (cvsat2map[1]<=0. || cvsat2map[2]!=0. ||
	cvsat2map[4]!=0. || cvsat2map[5]<=0.) return false;
    if (cvmap2sat[1]<=0. || cvmap2sat[2]!=0. ||
	cvmap2sat[4]!=0. || cvmap2sat[5]<=0.) return false;
  } else if (mapconvert!=0) {
    // impossible mapconversion (without remaking image)
    return false;
  }

  //Member variables, used in values().
  xmin= cvsat2map[0];
  ymin= cvsat2map[3];
  xmax= cvsat2map[0]+cvsat2map[1]*nx;
  ymax= cvsat2map[3]+cvsat2map[5]*ny;
  

  // exit if image is outside map area
//###  if (maprect.x1 >= satdata->nx || maprect.x2 <= 0. ||
//###      maprect.y1 >= satdata->ny || maprect.y2 <= 0.) return true;
  if (maprect.x1 >= xmax || maprect.x2 <= xmin ||
      maprect.y1 >= ymax || maprect.y2 <= ymin) return true;

//###  // width of image (pixels)
//###  int currwid, currhei;

  // scaling
  float scalex= float(pwidth) /fullrect.width();
  float scaley= float(pheight)/fullrect.height();

//###  // bitmaps lower left corner
//###  int bmStartx=0, bmStarty=0;
//###
//###  // bitmaps upper right corner
//###  int bmStopx=0, bmStopy=0;
//###
  // bitmaps screen position (lower left) in world-coordinates
//###  float grStartx, grStarty;
//########  float grStopx, grStopy;
//###  
//###  // bitmap offset (out of valid area) ... seams to be in screen pixels!
//###  float bmxmove=0., bmymove=0.;

  // shown image corners in map coordinates
  float grStartx= (maprect.x1>xmin) ? maprect.x1 : xmin;
  float grStarty= (maprect.y1>ymin) ? maprect.y1 : ymin;
//########  float grStopx=  (maprect.x2<xmax) ? maprect.x2 : xmax;
//########  float grStopy=  (maprect.y2<ymax) ? maprect.y2 : ymax;

  // total image corners in map coordinates
  float x1= cvmap2sat[0]+cvmap2sat[1]*maprect.x1;
  float y1= cvmap2sat[3]+cvmap2sat[5]*maprect.y1;
  float x2= cvmap2sat[0]+cvmap2sat[1]*maprect.x2;
  float y2= cvmap2sat[3]+cvmap2sat[5]*maprect.y2;


  // shown image corners in image coordinates
  int bmStartx= (maprect.x1>xmin) ? int(x1) : 0;
  int bmStarty= (maprect.y1>ymin) ? int(y1) : 0;
  int bmStopx=  (maprect.x2<xmax) ? int(x2) : nx-1;
  int bmStopy=  (maprect.y2<ymax) ? int(y2) : ny-1;

  // lower left corner of displayed image part, in map coordinates
  // (part of lower left pixel may well be outside screen)
  float xstart= cvsat2map[0]+cvsat2map[1]*bmStartx;
  float ystart= cvsat2map[3]+cvsat2map[5]*bmStarty;

  // bitmap offset (out of valid area) ... seams to be in screen pixels!
  float bmxmove= (maprect.x1>xmin) ? (xstart-grStartx)*scalex : 0;
  float bmymove= (maprect.y1>ymin) ? (ystart-grStarty)*scaley : 0;

  // for hardcopy
  float pxstart= (xstart-maprect.x1)*scalex;
  float pystart= (ystart-maprect.y1)*scaley;

  // update scaling with ratio image to map (was map to screen pixels)
  scalex*=cvsat2map[1];
  scaley*=cvsat2map[5];

  // width of image (pixels)
  int currwid= bmStopx - bmStartx + 1;  // use pixels in image
  int currhei= bmStopy - bmStarty + 1;  // use pixels in image

  // large scale values means disaster (in OpenGL pixel opertions),
  // this is a workaround....
  if (currwid<3 && currhei<3) {
      int i, j, adr;
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
      for (j=bmStarty; j<=bmStopy; j++) {
          for (i=bmStartx; i<=bmStopx; i++) {
	      adr=(j*nx+i)*3;
	      glColor3ubv(&satdata->image[adr]);
	      glRectf(GLfloat(i), GLfloat(j), GLfloat(i+1), GLfloat(j+1));
	  }
      }
      UpdateOutput();

  } else {

    // keep original copies (for hardcopy purposes)
    int orignx =       nx;
    int origny =       ny;
    float origscalex=  scalex;
    float origscaley=  scaley;
    int origbmStartx=  bmStartx;
    int origbmStarty=  bmStarty;
    int origbmStopx=   bmStopx;
    int origbmStopy=   bmStopy;
    float origpxstart= pxstart;
    float origpystart= pystart;

    /*
      If rasterimage wider than OpenGL-maxsizes:
      For now, temporarily resample image..
    */
    unsigned char * cimage;// pointer to imagedata, either sat_image or 
                           // resampled data
    int irs= 1;            // resample-size

    GLint maxdims[2];      // find OpenGL maximums
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS,maxdims);
    int maxww= maxdims[0];
    int maxhh= maxdims[1];

    if (  currwid > maxww || currhei > maxhh ){
      if ( (currwid - maxww) > (currhei - maxhh) )
	irs = (currwid / maxww) + 1;
      else 
	irs = (currhei / maxhh) + 1;
      currwid /= irs;
      currhei /= irs;
      bmStartx/= irs;
      bmStarty/= irs;
      bmStopx  = bmStartx+currwid-1;
      bmStopy  = bmStarty+currhei-1;
      nx      /= irs;
      ny      /= irs;
      scalex  *= irs;
      scaley  *= irs;

      
      // check if correct resampling already available..
      if (irs != previrs || !imagedata){
	// 	cerr << " diSatPlot::plot() resampling image:" << irs << endl;
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

    // always needed (if not, slow oper...) ??????????????
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
    
    if (bmxmove<0. || bmymove<0.) glBitmap(0,0,0.,0.,bmxmove,bmymove,NULL); 

    glDrawPixels((GLint)currwid, (GLint)currhei,
	         GL_RGBA, GL_UNSIGNED_BYTE,
	         cimage);

    // for postscript output, add imagedata to glpfile
    if (hardcopy){
     
      psAddImage(satdata->image,
	         4*orignx*origny, orignx, origny,
	         origpxstart, origpystart, origscalex, origscaley,
	         origbmStartx, origbmStarty, origbmStopx, origbmStopy,
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
  }
#ifdef DEBUGPRINT
  cerr << "++ Returning from SatPlot::plot() ++" << endl;
#endif
  return true;
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
