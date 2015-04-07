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

#include <diShapeObject.h>

#include <diColourShading.h>
#include <diTesselation.h>
#include <diFontManager.h>

#include <puTools/miStringFunctions.h>

#include <GL/glu.h>

#define MILOGGER_CATEGORY "diana.ShapeObject"
#include <miLogger/miLogging.h>

/* Created at Wed Oct 12 15:31:16 2005 */

using namespace std;
using namespace miutil;
//#define DEBUGPRINT

ShapeObject::ShapeObject()
{
  typeOfObject = ShapeXXX;
  //standard colours
  ColourShading cs("standard");
  colours=cs.getColourShading();
  colourmapMade=false;
}

ShapeObject::~ShapeObject()
{
  int n=shapes.size();
  int i=0;
  for (i=0; i<n; i++) {
    SHPDestroyObject(shapes[i]);
  }
  n=orig_shapes.size();
  for (i=0; i<n; i++) {
    SHPDestroyObject(orig_shapes[i]);
  }
}


ShapeObject::ShapeObject(const ShapeObject &rhs)
  : ObjectPlot(rhs)
{
}

ShapeObject& ShapeObject::operator=(const ShapeObject &rhs)
{
  METLIBS_LOG_SCOPE();
  if (this == &rhs)
    return *this;
  memberCopy(rhs);
  return *this;
}

void ShapeObject::memberCopy(const ShapeObject& rhs)
{
  ObjectPlot::memberCopy(rhs);

  int n=shapes.size();
  for (int i=0; i<n; i++) {
    SHPDestroyObject(shapes[i]);
  }

  shapes.clear();

  colours = rhs.colours;
  fname = rhs.fname;
  dbfIntName = rhs.dbfIntName;
  dbfDoubleName = rhs.dbfDoubleName;
  dbfStringName = rhs.dbfStringName;

  // Copy shapes
  int m=rhs.shapes.size();
  for (int i=0; i<m; i++) {
     shapes.push_back(rhs.shapes[i]);
  }
}

bool ShapeObject::changeProj(const Area& fromArea)
{
  METLIBS_LOG_SCOPE();

  int nEntities = shapes.size();
  bool success = false;
  bool success2 = false;
  for (int i=0; i<nEntities; i++) {
    int nVertices=shapes[i]->nVertices;
    float *tx;
    float *ty;
    tx = new float[nVertices];
    ty = new float[nVertices];

    for (int j=0; j<nVertices; j++) {
      tx[j] = orig_shapes[i]->padfX[j];
	  // an ugly fix to avoid problem with -180.0, 180.0
	  if (tx[j] == -180.0)
		tx[j] = -179.999;
      ty[j] = orig_shapes[i]->padfY[j];
    }
    success = getStaticPlot()->GeoToMap(nVertices, tx, ty);

    for (int k=0; k<nVertices; k++) {
      shapes[i]->padfX[k] = tx[k];
      shapes[i]->padfY[k] = ty[k];
    }
    delete[] tx;
    delete[] ty;
    nVertices = 2;
    tx = new float[2];
    ty = new float[2];
    tx[0] = orig_shapes[i]->dfXMin;
	if (tx[0] == -180.0)
		tx[0] = -179.999;
    tx[1] = orig_shapes[i]->dfXMax;
	if (tx[1] == -180.0)
		tx[1] = -179.999;
    ty[0] = orig_shapes[i]->dfYMin;
    ty[1] = orig_shapes[i]->dfYMax;
    success2 = getStaticPlot()->GeoToMap(nVertices, tx, ty);

    shapes[i]->dfXMin = tx[0];
    shapes[i]->dfXMax = tx[1];
    shapes[i]->dfYMin = ty[0];
    shapes[i]->dfYMax = ty[1];
    delete[] tx;
    delete[] ty;
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("done!");
#endif
  return (success && success2);
}

bool ShapeObject::read(std::string filename)
{
  return read(filename, false);
}

bool ShapeObject::read(std::string filename, bool convertFromGeo)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("ShapeObject::read(" << filename << "," << convertFromGeo);
#endif
  // shape reading
  SHPHandle hSHP;
  int nShapeType, nEntities, i;
  double adfMinBound[4], adfMaxBound[4];
  hSHP = SHPOpen(filename.c_str(), "rb");

  if (hSHP == NULL) {
    METLIBS_LOG_ERROR("Unable to open: "<<filename);
    return false;
  }

  SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

  for (i = 0; i < nEntities; i++) {
    SHPObject *psShape;
    psShape = SHPReadObject(hSHP, i);
	orig_shapes.push_back(psShape);
	psShape = SHPReadObject(hSHP, i);
    if (convertFromGeo) {
      float *tx;
      float *ty;
      int nVertices = psShape->nVertices;
      tx = new float[nVertices];
      ty = new float[nVertices];
      for (int j=0; j<nVertices; j++) {
        tx[j] = psShape->padfX[j];
		// an ugly fix to avoid problem with -180.0, 180.0
		if (tx[j] == -180.0)
			tx[j] = -179.999;
        ty[j] = psShape->padfY[j];
      }
      getStaticPlot()->GeoToMap(nVertices, tx, ty);
      for (int j=0; j<nVertices; j++) {
        psShape->padfX[j] = tx[j];
        psShape->padfY[j] = ty[j];
      }
      delete[] tx;
      delete[] ty;
      tx = new float[2];
      ty = new float[2];
      tx[0] = psShape->dfXMin;
	  if (tx[0] == -180.0)
		tx[0] = -179.999;
      tx[1] = psShape->dfXMax;
	  if (tx[1] == -180.0)
		tx[1] = -179.999;
      ty[0] = psShape->dfYMin;
      ty[1] = psShape->dfYMax;
      nVertices = 2;
      getStaticPlot()->GeoToMap(nVertices, tx, ty);
      psShape->dfXMin = tx[0];
      psShape->dfXMax = tx[1];
      psShape->dfYMin = ty[0];
      psShape->dfYMax = ty[1];
      delete[] tx;
      delete[] ty;
    }
    shapes.push_back(psShape);
  }

  SHPClose(hSHP);
  //


  //read dbf file

  int idbf= readDBFfile(filename, dbfIntName, dbfIntDesc, dbfDoubleName,
      dbfDoubleDesc, dbfStringName, dbfStringDesc);

  colourmapMade=false;
  stringcolourmapMade=false;
  doublecolourmapMade=false;
  intcolourmapMade=false;

  //writeCoordinates writes a file with coordinates to teddb
  //writeCoordinates();
  return (idbf == 0);
}

void ShapeObject::plot(PlotOrder porder)
{
  makeColourmap();
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
  int n=shapes.size();
  for (int i=0; i<n; i++) {
    if (shapes[i]->nSHPType!=5)
      continue;
    if (intcolourmapMade) {
      int descr=dbfIntDescr[shapes[i]->nShapeId];
      glColor4ubv(intcolourmap[descr].RGBA());
    } else if (doublecolourmapMade) {
      double descr=dbfDoubleDescr[shapes[i]->nShapeId];
      glColor4ubv(doublecolourmap[descr].RGBA());
    } else if (stringcolourmapMade) {
      std::string descr=dbfStringDescr[shapes[i]->nShapeId];
      glColor4ubv(stringcolourmap[descr].RGBA());
    }
    //
    glLineWidth(2);
    int nparts=shapes[i]->nParts;
    int *countpos= new int[nparts];//# of positions for each part
    int nv= shapes[i]->nVertices;
    GLdouble *gldata= new GLdouble[nv*3];
    int j=0;
    for (int jpart=0; jpart<nparts; jpart++) {
      int nstop, ncount=0;
      int nstart=shapes[i]->panPartStart[jpart];
      if (jpart==nparts-1)
        nstop=nv;
      else
        nstop=shapes[i]->panPartStart[jpart+1];
      if (shapes[i]->padfX[0] == shapes[i]->padfX[nstop-1]
          && shapes[i]->padfY[0] == shapes[i]->padfY[nstop-1])
        nstop--;
      for (int k = nstart; k < nstop; k++) {
        gldata[j] = shapes[i]->padfX[k];
        gldata[j+1]= shapes[i]->padfY[k];
        gldata[j+2]= 0.0;
        j+=3;
        ncount++;
      }
      countpos[jpart]=ncount;
    }
    beginTesselation();
    tesselation(gldata, nparts, countpos);
    endTesselation();
    delete[] gldata;
  }
}

bool ShapeObject::plot(Area area, // current area
		   double gcd, // size of plotarea in m
		   bool land, // plot triangles
		   bool cont, // plot contour-lines
		   bool keepcont, // keep contourlines for later
                   bool special, // special case, when plotting symbol instead of a point
                   int symbol, // symbol number to be plotted
                   std::string dbfcol, // column name in dfb file, text to be plotted
 		   GLushort linetype, // contour line type
		   float linewidth, // contour linewidth
		   const unsigned char* lcolour, // contour linecolour
		   const unsigned char* fcolour, // triangles fill colour
		   const unsigned char* bcolour)
{
	float x1, y1, x2, y2;
    int symbol_rad = 0; 
	//GLenum errCode;
    //const GLubyte *errString;
	float scalefactor = gcd/7000000;
	int fontSizeToPlot = int(2/scalefactor);

    //also scale according to windowheight and width (standard is 500)
    scalefactor = sqrtf(getStaticPlot()->getPhysHeight()*getStaticPlot()->getPhysHeight()+getStaticPlot()->getPhysWidth()*getStaticPlot()->getPhysWidth())/500;
    //METLIBS_LOG_DEBUG("scalefactor =" <<scalefactor); 
    fontSizeToPlot = int(fontSizeToPlot*scalefactor);
    //symbol_rad = int(symbol * scalefactor);
    symbol_rad = symbol;
    //METLIBS_LOG_DEBUG("symbol_rad = " << symbol_rad); 
    //METLIBS_LOG_DEBUG("fontSizeToPlot = " << fontSizeToPlot); 

	x1= area.R().x1 -1.;
	x2= area.R().x2 +1.;
	y1= area.R().y1 -1.;
	y2= area.R().y2 +1.;

	float sizeWX, sizeWY;

	sizeWX = x2 - x1;
	sizeWY = y2 - y1;

	// Compute the smallest visible part of map (approx 1/1000).
	// the points should be reduced for this and the polygon should not be filled
	float dX = sizeWX * .001;
	float dY = sizeWY * .001;

	// Compute the smallest visible part of map (approx 1/50).
	// that should not be filled
	float dsX = sizeWX * .01;
	float dsY = sizeWY * .01;

#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("x1=" << x1);
	METLIBS_LOG_DEBUG("x2=" << x2);
	METLIBS_LOG_DEBUG("y1=" << y1);
	METLIBS_LOG_DEBUG("y2=" << y2);

	METLIBS_LOG_DEBUG("sizeWX: " << sizeWX << " dX: " << dX); 
	METLIBS_LOG_DEBUG("sizeWY: " << sizeWY << " dY: " << dY); 
#endif

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    /* from shapefil.h */
	/* -------------------------------------------------------------------- */
/*      Shape types (nSHPType)                                          */
/* -------------------------------------------------------------------- */
/*
#define SHPT_NULL       0
#define SHPT_POINT      1
#define SHPT_ARC        3
#define SHPT_POLYGON    5
#define SHPT_MULTIPOINT 8
#define SHPT_POINTZ     11
#define SHPT_ARCZ       13
#define SHPT_POLYGONZ   15
#define SHPT_MULTIPOINTZ 18
#define SHPT_POINTM     21
#define SHPT_ARCM       23
#define SHPT_POLYGONM   25
#define SHPT_MULTIPOINTM 28
#define SHPT_MULTIPATCH 31
*/

/* -------------------------------------------------------------------- */
/*      Part types - everything but SHPT_MULTIPATCH just uses           */
/*      SHPP_RING.                                                      */
/* -------------------------------------------------------------------- */
/*
#define SHPP_TRISTRIP   0
#define SHPP_TRIFAN     1
#define SHPP_OUTERRING  2
#define SHPP_INNERRING  3
#define SHPP_FIRSTRING  4
#define SHPP_RING       5
*/
    // Retrieving text for plotting from the dbf file and col=dbfcol
    vector<std::string> tmpDesc=dbfPlotDesc[dbfcol];

	int n=shapes.size();
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("***Map contains " << n <<  " shapes. ");
#endif
	for (int i=0; i<n; i++) {
		// Debug......
		//if (i != 13) continue;
		if ((shapes[i]->nSHPType!=SHPT_POLYGON)&&(shapes[i]->nSHPType!=SHPT_ARC)&&(shapes[i]->nSHPType!=SHPT_POINT)){
			METLIBS_LOG_ERROR("shapes["<<i<<"]=" << shapes[i]->nSHPType << " unsupported shape type!");
			continue;
		}
		// Check if shape is outside
		if((((shapes[i]->dfXMin > x2) || (shapes[i]->dfXMin < x1 && shapes[i]->dfXMax < x1)) && (shapes[i]->dfYMin > y2))
		    || (shapes[i]->dfYMin < y1 && shapes[i]->dfYMax < y1)) {
#ifdef DEBUGPRINT
				METLIBS_LOG_DEBUG("minX: " << shapes[i]->dfXMin << " maxX: " << shapes[i]->dfXMax << " minY: " << shapes[i]->dfYMin << " maxY: " << shapes[i]->dfYMax);
				METLIBS_LOG_DEBUG("x1: " << x1 << " x2: " << x2 << " y1: " << y1 << " y2: " << y2);
				METLIBS_LOG_DEBUG("shapes["<<i<<"] is outside");
#endif
				continue;
		}

		// Check if shape is to small
		float xSize = fabs(shapes[i]->dfXMax - shapes[i]->dfXMin);
		float ySize = fabs(shapes[i]->dfYMax - shapes[i]->dfYMin);
		// there is no use reducing a line map
		if (shapes[i]->nSHPType==SHPT_POLYGON)
		{
			if ((xSize < dY) && (ySize < dY))
			{
#ifdef DEBUGPRINT
				METLIBS_LOG_DEBUG("shapes["<<i<<"] is to small, xSize: " << xSize << " ySize: " << ySize << " dy: " << dY << " dx: " << dX);
#endif
				continue;
			}
		}
		int nparts=shapes[i]->nParts;
		int nv= shapes[i]->nVertices;
		/// CHECK IF MAP HAVE NO PARTS, the point map has no parts
		// Here we have a special case to take care of...
		if (shapes[i]->nSHPType==SHPT_POINT)
		{
			if (nparts == 0)
			{
                           if (special==true && nv==1){
                               float cw,ch;
                               std::string astring = " "+tmpDesc[i];
                               getStaticPlot()->getFontPack()->set(poptions.fontname,"NORMAL",fontSizeToPlot); //getStaticPlot()->getFontPack()->set("Arial","BOLD",8);
                               getStaticPlot()->getFontPack()->getStringSize(astring.c_str(),cw,ch);
                               //getStaticPlot()->getFontPack()->drawStr(astring.c_str(),shapes[i]->padfX[0]-cw/2, shapes[i]->padfY[0]+ch/2,0.0);
                               getStaticPlot()->getFontPack()->drawStr("SSS",shapes[i]->padfX[0], shapes[i]->padfY[0],0.0);
                               //glDrawPixels((GLint)10, (GLint)10,GL_RGBA, GL_UNSIGNED_BYTE,cimage);

                               glLineWidth(2);
			       glColor4ubv(lcolour);
                               glBegin(GL_POLYGON);
                                     
                               GLfloat xc,yc;
                               GLfloat radius=symbol_rad;          //16271;
                               for(int j=0;j<150;j++){
                                   xc = radius*cos(j*2*M_PI/150.0);
                                   yc = radius*sin(j*2*M_PI/150.0);
                                   glVertex2f(shapes[i]->padfX[0]+xc,shapes[i]->padfY[0]+yc);
                               }
                               glEnd(); 
                           }
                           else {
				// set the point size
				//glPointSize(linewidth*10);
				glEnable(GL_POINT_SMOOTH);
				glPointSize(linewidth*2);
				glColor4ubv(lcolour);
				// not so fast but accurate
						
				glBegin(GL_POINTS);
				// just display the point(s)
				for (int k = 0; k < nv; k++) {
					glVertex2f(shapes[i]->padfX[k],shapes[i]->padfY[k]);
	
                        	}
					
				glEnd();
				glDisable(GL_POINT_SMOOTH);
                            }
			}
		}

		int *countpos= new int[nparts];
		//# of positions for each part
		int *small= new int[nparts];
		// should be set to 1 if part should not be filled ?!
/*		
#ifdef DEBUGPRINT
		METLIBS_LOG_DEBUG("shapes["<<i<<"] contains " << nv << " vertices and " << nparts << " parts. ");
#endif*/
		GLdouble *gldata= new GLdouble[nv*3];
		GLdouble *pdata= new GLdouble[nv*2];
		int j=0;
		int pj=0;
		bool visible = false;
		for (int jpart=0; jpart<nparts; jpart++) {
			visible = false;
			int nstop, ncount=0;

			// Get the starting point of this part (shapeobject consists of several parts)
			int nstart=shapes[i]->panPartStart[jpart];

			// Get the end point
			if (jpart==nparts-1)
				nstop=nv;
			else
				nstop=shapes[i]->panPartStart[jpart+1];

			float minX=shapes[i]->padfX[nstart];
			float minY=shapes[i]->padfY[nstart];
			float maxX=shapes[i]->padfX[nstart];
			float maxY=shapes[i]->padfY[nstart];

			// Compute max and min for part
			for (int k = nstart + 1; k < nstop; k++) {
				if(shapes[i]->padfY[k] > maxY)
					maxY=shapes[i]->padfY[k];
				if(shapes[i]->padfY[k] < minY)
					minY=shapes[i]->padfY[k];
				if(shapes[i]->padfX[k] > maxX)
					maxX = shapes[i]->padfX[k];
				if(shapes[i]->padfX[k] < minX)
					minX =shapes[i]->padfX[k];
			}

			// Reduce part that is to SMALL. Skipp it when filling polygon
			// Compute size of plotting area and part
			float sizepX, sizepY;

			sizepX = maxX - minX;
			sizepY = maxY - minY;


			// Skip i less than .1%
			/* some maps dont work well so we must skip this */

			bool to_small = false;
			small[jpart] = 0;
			if ((sizepX < dX) && (sizepY < dY))
			{
				to_small = true;
				small[jpart]=1;
			}
			
			int pv = 0;
			// Allocate temporary buffer
			// Assume, all points are valid.
			int psize = nstop-nstart;
			//METLIBS_LOG_DEBUG("Size of part[ " << jpart << " ]: " << psize);
			GLdouble * xTemparr = new GLdouble[psize];
			GLdouble * yTemparr = new GLdouble[psize];
			int incr = 1;
			
			xTemparr[pv] = shapes[i]->padfX[nstart];
			yTemparr[pv] = shapes[i]->padfY[nstart];
			pv++;

			for (int k = nstart + 1; k < nstop; k=k+incr) {
				xTemparr[pv] = shapes[i]->padfX[k];
				yTemparr[pv] = shapes[i]->padfY[k];
				pv++;

			} // End first preprocess step, inside window.
			
			int pk = 0;
			if (shapes[i]->nSHPType!=SHPT_POINT)
			{
				if (to_small)
				{
					// reduce points, start, minX, maxX, minY, maxY and stop
					// HM, not so good, disabled for the moment....
					if (pv > 3)
					{
						//pj = 0;

						gldata[j] = xTemparr[0];
						gldata[j+1]= yTemparr[0];
						gldata[j+2]= 0.0;
						pdata[pj] = xTemparr[0];
						pdata[pj+1]= yTemparr[0];
						j+=3;
						pj+=2;
						ncount++;
						for (pk = 1; pk < pv - 1; pk++)
						{
							if (xTemparr[pk] == minX)
							{
								gldata[j] = xTemparr[pk];
								gldata[j+1]= yTemparr[pk];
								gldata[j+2]= 0.0;
								pdata[pj] = xTemparr[pk];
								pdata[pj+1]= yTemparr[pk];
								j+=3;
								pj+=2;
								ncount++;
							}
							if (xTemparr[pk] == minY)
							{
								gldata[j] = xTemparr[pk];
								gldata[j+1]= yTemparr[pk];
								gldata[j+2]= 0.0;
								pdata[pj] = xTemparr[pk];
								pdata[pj+1]= yTemparr[pk];
								j+=3;
								pj+=2;
								ncount++;
							}
							if (xTemparr[pk] == maxX)
							{
								gldata[j] = xTemparr[pk];
								gldata[j+1]= yTemparr[pk];
								gldata[j+2]= 0.0;
								pdata[pj] = xTemparr[pk];
								pdata[pj+1]= yTemparr[pk];
								j+=3;
								pj+=2;
								ncount++;
							}
							if (xTemparr[pk] == maxY)
							{
								gldata[j] = xTemparr[pk];
								gldata[j+1]= yTemparr[pk];
								gldata[j+2]= 0.0;
								pdata[pj] = xTemparr[pk];
								pdata[pj+1]= yTemparr[pk];
								j+=3;
								pj+=2;
								ncount++;
							}
						}
						gldata[j] = xTemparr[pv - 1];
						gldata[j+1]= yTemparr[pv - 1];
						gldata[j+2]= 0.0;
						pdata[pj] = xTemparr[pv - 1];
						pdata[pj+1]= yTemparr[pv - 1];
						j+=3;
						pj+=2;
						ncount++;

					}
					else
					{
						//j = 0;
						for (pk = 0; pk < pv; pk++)
						{
							// always display the 3 or less points

							gldata[j] = xTemparr[pk];
							gldata[j+1]= yTemparr[pk];
							gldata[j+2]= 0.0;
							pdata[pj] = xTemparr[pk];
							pdata[pj+1]= yTemparr[pk];
							j+=3;
							pj+=2;
							ncount++;

						}
					}
				}
				else
				{
					//j = 0;
					int prev_k = 0;
					for (pk = 0; pk < pv; pk++)
					{
						
						if (pk == 0)
						{
							// always display the first point

							gldata[j] = xTemparr[pk];
							gldata[j+1]= yTemparr[pk];
							gldata[j+2]= 0.0;
							pdata[pj] = xTemparr[pk];
							pdata[pj+1]= yTemparr[pk];
							j+=3;
							pj+=2;
							ncount++;

						}
						else
						{	
							if (pk == pv - 1)
							{

								gldata[j] = xTemparr[pk];
								gldata[j+1]= yTemparr[pk];
								gldata[j+2]= 0.0;
								pdata[pj] = xTemparr[pk];
								pdata[pj+1]= yTemparr[pk];
								j+=3;
								pj+=2;
								ncount++;
							}
							else
							{
								// Here, we should in some way reduce information, how ?
								// Unfortunately, we kill the zoom, if we have the same dx an dy outside the visible window as in the window
								// check x
								double dx = dX;
								double dy = dY;
								if (xTemparr[pk] > x2)
									dx = dsX;
								else if (xTemparr[pk] < x1)
									dx = dsX;
								// check y
								if (yTemparr[pk] > y2)
									dy = dsY;
								else if (yTemparr[pk] < y1)
									dy = dsY;
								if ((fabs(xTemparr[pk] - xTemparr[prev_k]) < dx)&&(fabs(yTemparr[pk] - yTemparr[prev_k]) < dy))
								{
									continue;
								}
								gldata[j] = xTemparr[pk];
								gldata[j+1]= yTemparr[pk];
								gldata[j+2]= 0.0;
								pdata[pj] = xTemparr[pk];
								pdata[pj+1]= yTemparr[pk];
								j+=3;
								pj+=2;
								ncount++;
								prev_k = pk;
							}
						}
					}
				} // to_small
			} // Not a point
			else // a point map
			{
				for (pk = 0; pk < pv; pk++)
				{
					// always display all points

					gldata[j] = xTemparr[pk];
					gldata[j+1]= yTemparr[pk];
					gldata[j+2]= 0.0;
					pdata[pj] = xTemparr[pk];
					pdata[pj+1]= yTemparr[pk];
					j+=3;
					pj+=2;
					ncount++;
				}
			}
			// get rid of temporary buffer
			delete [] xTemparr;
			delete [] yTemparr;
			if (shapes[i]->nSHPType!=SHPT_POINT)
			{
				if(ncount > 1) {
					visible = true;
				}
			}
			else
				visible = true;

			countpos[jpart]=ncount;
#ifdef DEBUGPRINT
			METLIBS_LOG_DEBUG("Points to draw and fill [ " << jpart << " ]: " << ncount);
#endif

		}
		// Draw the shape object

		// draw the contour lines
		if (cont && visible)
		{
			// NOTE: The opengl library do som optimizing stuff when using the glDrawArrays for more complex shapes
			// therfore we must use the little slover method for shapes with more than one part.

			//void glEnableClientState(GLenum array) 
			//Specifies the array to enable.
			//Symbolic constants GL_VERTEX_ARRAY, GL_COLOR_ARRAY, GL_INDEX_ARRAY, GL_NORMAL_ARRAY, GL_TEXTURE_COORD_ARRAY, and GL_EDGE_FLAG_ARRAY
			//are acceptable parameters. 
			if (nparts == 1)
				glEnableClientState (GL_VERTEX_ARRAY);

			if (shapes[i]->nSHPType!=SHPT_POINT)
			{
				glLineWidth(linewidth);
				if (linetype!=0xFFFF) {
					glLineStipple(1, linetype);
					glEnable(GL_LINE_STIPPLE);
				}
			}
			else
			{
				// set the point size
				glPointSize(linewidth);
				glEnable(GL_POINT_SMOOTH);

			}
			glColor4ubv(lcolour);

			//void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer); 
			//Specifies where spatial coordinate data can be accessed.
			//pointer is the memory address of the first coordinate of the first vertex in the array.
			//type specifies the data type (GL_SHORT, GL_INT, GL_FLOAT, or GL_DOUBLE) of each coordinate in the array.
			//size is the number of coordinates per vertex, which must be 2, 3, or 4.
			//stride is the byte offset between consecutive vertexes.
			//If stride is 0, the vertices are understood to be tightly packed in the array. 
			if (nparts == 1)
				glVertexPointer(2, GL_DOUBLE, 0, pdata);

			//void glDrawArrays(GLenum mode, GLint first, GLsizei count); 
			//Constructs a sequence of geometric primitives using array elements starting at first and ending at first+count-1 of each enabled array.
			//mode specifies what kinds of primitives are constructed and is one of the same values accepted by glBegin();
			//for example, GL_POLYGON, GL_LINE_LOOP, GL_LINES, GL_POINTS, and so on. 
			
			int a, p,npos,pos;
			pos = 0;
			int apos = 0;
			for (p=0; p<nparts; p++) {
				npos= countpos[p];
				if (npos > 0)
				{
					if (nparts > 1)
					{
						// not so fast but accurate
						if (shapes[i]->nSHPType!=SHPT_POINT)
							glBegin(GL_LINE_STRIP);
						else
							glBegin(GL_POINTS);

						for (a=0; a<npos; a++) {
							glVertex2dv(&pdata[apos]);
							apos+=2;
						}
						glEnd();
					}
					else
					{
						// Fast but some optimizing error may happen
						if (shapes[i]->nSHPType!=SHPT_POINT)
							glDrawArrays(GL_LINE_STRIP, pos, npos);
						else
							glDrawArrays(GL_POINTS, pos, npos);
					}

				}
				pos+=npos;
			}
			glFlush();

			if (shapes[i]->nSHPType!=SHPT_POINT)
				glDisable(GL_LINE_STIPPLE);
			else
				glDisable(GL_POINT_SMOOTH);
			if (nparts == 1)
				glDisableClientState(GL_VERTEX_ARRAY);

		}
		// fill the polygons
		if (land && visible && shapes[i]->nSHPType==SHPT_POLYGON) {
			// sanity check, how?
			int p,npos,pos;
			pos = 0;
			
			for (p=0; p<nparts; p++) {
				npos= countpos[p];
				if (npos > 0)
				{

					if ((gldata[pos] != gldata[npos*3 + pos-3])&&(gldata[pos+1] != gldata[npos*3 + pos -2]))
						METLIBS_LOG_WARN("shapes["<<i<<"] part["<<p<<"] not closed");
				}
				pos = pos + npos*3;
			}


			glColor4ubv(fcolour);
			glLineWidth(linewidth);
			optimized_tesselation(gldata, nparts, countpos, small);
			glFlush();
		}
		delete[] gldata;
		delete[] pdata;
		delete[] countpos;
		delete[] small;

	}
	return true;
}

bool ShapeObject::getAnnoTable(std::string & str)
{
  makeColourmap();
  str="table=\""+ fname;
  if (stringcolourmapMade) {
    map <std::string,Colour>::iterator q=stringcolourmap.begin();
    for (; q!=stringcolourmap.end(); q++) {
      Colour col=q->second;
      ostringstream rs;
      //write colour
      rs << ";" << (int) col.R() << ":" << (int) col.G() << ":"
          << (int) col.B() <<";;";
      str+=rs.str()+q->first;
    }
  } else if (intcolourmapMade) {
    map <int,Colour>::iterator q=intcolourmap.begin();
    for (; q!=intcolourmap.end(); q++) {
      Colour col=q->second;
      ostringstream rs;
      //write colour
      rs << ";" << (int) col.R() << ":" << (int) col.G() << ":"
          << (int) col.B() <<";;" << q->first;
      str+=rs.str();
    }
  } else if (doublecolourmapMade) {
    map <double,Colour>::iterator q=doublecolourmap.begin();
    for (; q!=doublecolourmap.end(); q++) {
      Colour col=q->second;
      ostringstream rs;
      //write colour
      rs << ";" << (int) col.R() << ":" << (int) col.G() << ":"
          << (int) col.B() <<";;" << q->first;
      str+=rs.str();
    }
  }

  return true;
}

void ShapeObject::makeColourmap()
{
  if (colourmapMade)
    return;
  colourmapMade=true;
  if (poptions.palettecolours.size())
    colours= poptions.palettecolours;
  if (not poptions.fname.empty())
    fname=poptions.fname;
  else if (dbfStringName.size())
    fname=dbfStringName[0];
  else if (dbfDoubleName.size())
    fname=dbfDoubleName[0];
  else if (dbfIntName.size())
    fname=dbfIntName[0];
  int n=shapes.size();
  int ncolours= colours.size();
  int ii=0;
  //strings, first find which vector of strings to use, dbfStringDescr
  int ndsn =dbfStringName.size();
  for (int idsn=0; idsn < ndsn; idsn++) {
    if (dbfStringName[idsn]==fname) {
      dbfStringDescr=dbfStringDesc[idsn];
      stringcolourmapMade=true;
    }
  }
  if (stringcolourmapMade) {
    for (int i=0; i<n; i++) {
      std::string descr=dbfStringDescr[shapes[i]->nShapeId];
      if (stringcolourmap.find(descr)==stringcolourmap.end()) {
        stringcolourmap[descr]=colours[ii];
        ii++;
        if (ii==ncolours)
          ii=0;
      }
    }
    return;
  }
  //
  //int, first find which vector of double to use, dbfDoubleDescr
  int nddn =dbfDoubleName.size();
  for (int iddn=0; iddn < nddn; iddn++) {
    if (dbfDoubleName[iddn]==fname) {
      dbfDoubleDescr=dbfDoubleDesc[iddn];
      doublecolourmapMade=true;
    }
  }
  if (doublecolourmapMade) {
    for (int i=0; i<n; i++) {
      double descr=dbfDoubleDescr[shapes[i]->nShapeId];
      if (doublecolourmap.find(descr)==doublecolourmap.end()) {
        doublecolourmap[descr]=colours[ii];
        ii++;
        if (ii==ncolours)
          ii=0;
      }
    }
    return;
  }
  //
  int ndin =dbfIntName.size();
  for (int idin=0; idin < ndin; idin++) {
    if (dbfIntName[idin]==fname) {
      dbfIntDescr=dbfIntDesc[idin];
      intcolourmapMade=true;
    }
  }
  if (intcolourmapMade) {
    for (int i=0; i<n; i++) {
      int descr=dbfIntDescr[shapes[i]->nShapeId];
      if (intcolourmap.find(descr)==intcolourmap.end()) {
        intcolourmap[descr]=colours[ii];
        ii++;
        if (ii==ncolours)
          ii=0;
      }
    }
    return;
  }
}

int ShapeObject::getXYZsize()
{
  int size=0;
  int n=shapes.size();
  for (int i=0; i<n; i++)
    size+=shapes[i]->nVertices;
  return size;
}

vector<float> ShapeObject::getX()
{
  vector<float> x;
  int n=shapes.size();
  for (int i=0; i<n; i++) {
    for (int j = 0; j < shapes[i]->nVertices; j++)
      x.push_back(shapes[i]->padfX[j]);
  }
  return x;
}

vector<float> ShapeObject::getY()
{
  vector<float> y;
  int n=shapes.size();
  for (int i=0; i<n; i++) {
    for (int j = 0; j < shapes[i]->nVertices; j++)
      y.push_back(shapes[i]->padfY[j]);
  }
  return y;
}

void ShapeObject::setXY(vector<float> x, vector <float> y)
{
  int n=shapes.size();
  int m=0;
  for (int i=0; i<n; i++) {
    for (int j = 0; j < shapes[i]->nVertices; j++) {
      shapes[i]->padfX[j]=x[m];
      shapes[i]->padfY[j]=y[m];
      m++;
    }
  }
}
//#define DEBUGPRINT 1

int ShapeObject::readDBFfile(const std::string& filename,
    vector<std::string>& dbfIntName, vector< vector<int> >& dbfIntDesc,
    vector<std::string>& dbfDoubleName, vector< vector<double> >& dbfDoubleDesc,
    vector<std::string>& dbfStringName, vector< vector<std::string> >& dbfStringDesc)
{
  DBFHandle hDBF;
  int i, iRecord;
  size_t n;
  int nWidth, nDecimals;
  char szTitle[12];

  //int indexTema1= -1;
  //int indexTema2= -1;
  //int indexLength= -1;
  int nFieldCount, nRecordCount;

  vector<int> indexInt, indexDouble, indexString;

  vector<int> dummyint;
  vector<double> dummydouble;
  vector<std::string> dummystring;
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("readDBFfile: "<<filename);
#endif
  /* -------------------------------------------------------------------- */
  /*      Open the file.                                                  */
  /* -------------------------------------------------------------------- */
  hDBF = DBFOpen(filename.c_str(), "rb");
  if (hDBF == NULL) {
    METLIBS_LOG_ERROR("DBFOpen "<<filename<<" failed");
    return 2;
  }

  /* -------------------------------------------------------------------- */
  /*	If there is no data in this file let the user know.		*/
  /* -------------------------------------------------------------------- */
  if (DBFGetFieldCount(hDBF) == 0) {
    METLIBS_LOG_ERROR("There are no fields in this table!");
    return 3;
  }

  nFieldCount = DBFGetFieldCount(hDBF);
  nRecordCount = DBFGetRecordCount(hDBF);

  /* -------------------------------------------------------------------- */
  /*	Check header definitions.					*/
  /* -------------------------------------------------------------------- */
  for (i = 0; i < nFieldCount; i++) {
    DBFFieldType eType;

    eType = DBFGetFieldInfo(hDBF, i, szTitle, &nWidth, &nDecimals);
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("---> "<<szTitle);
#endif
    std::string name= miutil::to_upper(szTitle);

    if (eType == FTInteger) {
      //          if (name=="LTEMA" || name=="FTEMA" || name=="VANNBR")  {
      indexInt.push_back(i);
      dbfIntName.push_back(name);
      dbfIntDesc.push_back(dummyint);
      //          }
    } else if (eType == FTDouble) {
      //          if (name == "LENGTH" ) {
      indexDouble.push_back(i);
      dbfDoubleName.push_back(name);
      dbfDoubleDesc.push_back(dummydouble);
      //	    }
    } else if (eType == FTString) {
      //          if (name == "VEGTYPE" ) {
      indexString.push_back(i);
      dbfStringName.push_back(name);
      dbfStringDesc.push_back(dummystring);
      //	    }
    }
  }
#ifdef DEBUGPRINT
  for ( int n=0; n<dbfIntName.size(); n++)
    METLIBS_LOG_DEBUG("Int    description:  "<<indexInt[n]<<"  "<<dbfIntName[n]);
  for (int n=0; n<dbfDoubleName.size(); n++)
    METLIBS_LOG_DEBUG("Double description:  "<<indexDouble[n]<<"  "<<dbfDoubleName[n]);
  for (int n=0; n<dbfStringName.size(); n++)
    METLIBS_LOG_DEBUG("String description:  "<<indexString[n]<<"  "<<dbfStringName[n]);
#endif
  /* -------------------------------------------------------------------- */
  /*	Read all the records 						*/
  /* -------------------------------------------------------------------- */

  for (n=0; n<dbfIntName.size(); n++) {
    i= indexInt[n];
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("Int    description:  "<<indexInt[n]<<"  "<<dbfIntName[n]);
#endif
	for (iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (DBFIsAttributeNULL(hDBF, iRecord, i) )
        dbfIntDesc[n].push_back(0);
	  else {
        dbfIntDesc[n].push_back(DBFReadIntegerAttribute(hDBF, iRecord, i) );
#ifdef DEBUGPRINT
        METLIBS_LOG_DEBUG("DBFReadIntegerAttribute(hDBF, iRecord, i)"
              << DBFReadIntegerAttribute(hDBF, iRecord, i));
#endif
	  }
    }
  }

  for (n=0; n<dbfDoubleName.size(); n++) {
    i= indexDouble[n];
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("Double description:  "<<indexDouble[n]<<"  "<<dbfDoubleName[n]);
#endif    
	for (iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (DBFIsAttributeNULL(hDBF, iRecord, i) )
        dbfDoubleDesc[n].push_back(0.0);
      else {
        dbfDoubleDesc[n].push_back(DBFReadDoubleAttribute(hDBF, iRecord, i) );
#ifdef DEBUGPRINT
          METLIBS_LOG_DEBUG("DBFReadDoubleAttribute( hDBF, iRecord, i )"
              << DBFReadDoubleAttribute(hDBF, iRecord, i));
#endif
      }
    }
  }

  for (n=0; n<dbfStringName.size(); n++) {
    i= indexString[n];
    vector<std::string> tempStr;
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("String description:  "<<indexString[n]<<"  "<<dbfStringName[n]);
#endif
    for (iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (DBFIsAttributeNULL(hDBF, iRecord, i) )
        dbfStringDesc[n].push_back("-");
      else {
        dbfStringDesc[n].push_back(std::string(DBFReadStringAttribute(hDBF,
            iRecord, i) ) );
        tempStr.push_back(std::string(DBFReadStringAttribute(hDBF,
            iRecord, i) ) );
#ifdef DEBUGPRINT
          METLIBS_LOG_DEBUG("DBFReadStringAttribute( hDBF, iRecord, i )"
              << DBFReadStringAttribute(hDBF, iRecord, i) << "**temp= " << tempStr[iRecord]);
#endif
      }
    }
    dbfPlotDesc[dbfStringName[n]]=tempStr;
    tempStr.clear();

    

  }
 /* map <std::string, vector<std::string> >::iterator it=dbfPlotDesc.begin();
    for (; it!=dbfPlotDesc.end(); it++) {
      vector<std::string> temp=it->second;
        METLIBS_LOG_DEBUG("*** ID_temp " << it->first);
      for (int ar=0; ar<temp.size(); ar++) {
        METLIBS_LOG_DEBUG("***temp [" << ar <<"] =  " << temp[ar]);
      }
    }
*/






  DBFClose(hDBF);

  double minlength=0.0;
  double maxlength=0.0;
  double avglength=0.0;
  int n1=0, n10=0, n100=0, n1000=0;

  unsigned int m= 0;
  while (m<dbfDoubleName.size() && dbfDoubleName[m]!="LENGTH")
    m++;

  if (m<dbfDoubleName.size()) {
    n= dbfDoubleDesc[m].size();
    if (n>0) {
      minlength= maxlength= dbfDoubleDesc[m][0];
      for (i=0; i<int(n); i++) {
        if (minlength>dbfDoubleDesc[m][i])
          minlength= dbfDoubleDesc[m][i];
        if (maxlength<dbfDoubleDesc[m][i])
          maxlength= dbfDoubleDesc[m][i];
        avglength+=dbfDoubleDesc[m][i];
        if (dbfDoubleDesc[m][i]<1.0)
          n1++;
        if (dbfDoubleDesc[m][i]<10.0)
          n10++;
        if (dbfDoubleDesc[m][i]<100.0)
          n100++;
        if (dbfDoubleDesc[m][i]<1000.0)
          n1000++;
      }
      avglength/=double(n);
    }
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("nFieldCount=      "<<nFieldCount);
  METLIBS_LOG_DEBUG("nRecordCount=     "<<nRecordCount);

  for (n=0; n<dbfIntName.size(); n++)
    METLIBS_LOG_DEBUG("Int    description, size,name:  "<<dbfIntDesc[n].size()<<"  "
        <<dbfIntName[n]);
  for (n=0; n<dbfDoubleName.size(); n++)
    METLIBS_LOG_DEBUG("Double description, size,name:  "<<dbfDoubleDesc[n].size()<<"  "
        <<dbfDoubleName[n]);
  for (n=0; n<dbfStringName.size(); n++)
    METLIBS_LOG_DEBUG("String description, size,name:  "<<dbfStringDesc[n].size()<<"  "
        <<dbfStringName[n]);
#endif
  return 0;
}

void ShapeObject::writeCoordinates()
{
  METLIBS_LOG_WARN("ShapeObject:writeCoordinates NOT IMPLEMENTED");
  /*****************************************************************************
   METLIBS_LOG_DEBUG("ShapeObject:writeCoordinates");
   // open filestream
   ofstream dbfile("shapelocations.txt");
   if (!dbfile){
   METLIBS_LOG_DEBUG("ERROR OPEN (WRITE) ");
   return;
   }

   int n=shapes.size();
   for (int i=0;i<n;i++){
   if (shapes[i]->nSHPType!=5)
   continue;
   int nr=650+i;
   dbfile << nr << "|" << "shape " << i << "|county|1|";
   int nparts=shapes[i]->nParts;
   METLIBS_LOG_DEBUG("number of parts " << nparts);
   int nv= shapes[i]->nVertices;
   int j=0;
   for (int jpart=0;jpart<nparts;jpart++){
   int nstop,ncount=0;
   int nstart=shapes[i]->panPartStart[jpart];
   if (jpart==nparts-1)
   nstop=nv;
   else
   nstop=shapes[i]->panPartStart[jpart+1];
   if (shapes[i]->padfX[0] == shapes[i]->padfX[nstop-1] &&
   shapes[i]->padfY[0] == shapes[i]->padfY[nstop-1])
   nstop--;
   for(int k = nstart; k < nstop; k++ ){
   METLIBS_LOG_DEBUG("x=" <<  shapes[i]->padfX[k]);
   METLIBS_LOG_DEBUG("  y =" << shapes[i]->padfY[k]);
   miCoordinates newcor(float(shapes[i]->padfX[k]),float(shapes[i]->padfY[k]));
   METLIBS_LOG_DEBUG(newcor.str());
   dbfile << newcor.iLon() << " " << newcor.iLat();
   //dbfile << shapes[i]->padfX[k] << "   " << shapes[i]->padfY[k];
   if (k!=nstop-1)
   dbfile << ":";

   }
   }
   dbfile << "|NULL|NULL|" << endl;
   }
   dbfile.close();
   *****************************************************************************/
}
