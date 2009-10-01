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
#include <fstream>
#include <iostream>
#include <diColourShading.h>
#include <diTesselation.h>
#include <diShapeObject.h>
#include <miCoordinates.h>

/* Created at Wed Oct 12 15:31:16 2005 */

using namespace std;

ShapeObject::ShapeObject() :
  ObjectPlot()
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
  for (int i=0; i<n; i++) {
    SHPDestroyObject(shapes[i]);
  }
}

ShapeObject::ShapeObject(const ShapeObject &rhs) :
  ObjectPlot(rhs)
{
}

ShapeObject& ShapeObject::operator=(const ShapeObject &rhs) {
  if (this == &rhs) return *this;
  // elementwise copy
  memberCopy(rhs);

  return *this;
}

void ShapeObject::memberCopy(const ShapeObject& rhs)
{
  int n=shapes.size();
  for (int i=0; i<n; i++) {
    SHPDestroyObject(shapes[i]);
  }

  colours = rhs.colours;
  fname = rhs.fname;
  dbfIntName = rhs.dbfIntName;
  dbfDoubleName = rhs.dbfDoubleName;
  dbfStringName = rhs.dbfStringName;

  // Copy shapes
  if (n) {
    for (int i=0; i<n; i++) {
      shapes.push_back(rhs.shapes[i]);
    }
  }
}

bool ShapeObject::changeProj(Area fromArea)
{
#ifdef DEBUGPRINT
  cerr << "ShapeObject::changeproj(): ";
#endif
  int nEntities = shapes.size();
  bool success = false;
  for (int i=0; i<nEntities; i++) {
    int nVertices=shapes[i]->nVertices;
    float *tx;
    float *ty;
    tx = new float[nVertices];
    ty = new float[nVertices];

    for (int j=0; j<nVertices; j++) {
      tx[j] = shapes[i]->padfX[j];
      ty[j] = shapes[i]->padfY[j];
    }
    success = gc.getPoints(fromArea, area, nVertices, tx, ty);
    for (int k=0; k<nVertices; k++) {
      shapes[i]->padfX[k] = tx[k];
      shapes[i]->padfY[k] = ty[k];
    }
    delete[] tx;
    delete[] ty;
  }
#ifdef DEBUGPRINT
  cerr << "done!" << endl;
#endif
  return success;
}

bool ShapeObject::read(miString filename)
{
  read(filename, false);
  return true;
}

bool ShapeObject::read(miString filename, bool convertFromGeo)
{
#ifdef DEBUGPRINT
  cerr << "ShapeObject::read(" << filename << "," << convertFromGeo << endl;
#endif
  // shape reading
  SHPHandle hSHP;
  int nShapeType, nEntities, i;
  double adfMinBound[4], adfMaxBound[4];
  hSHP = SHPOpen(filename.cStr(), "rb");

  if (hSHP == NULL) {
    cerr<<"Unable to open: "<<filename<<endl;
    return 1;
  }

  SHPGetInfo(hSHP, &nEntities, &nShapeType, adfMinBound, adfMaxBound);

  for (i = 0; i < nEntities; i++) {
    SHPObject *psShape;
    psShape = SHPReadObject(hSHP, i);
    if (convertFromGeo) {
      float *tx;
      float *ty;
      int nVertices = psShape->nVertices;
      tx = new float[nVertices];
      ty = new float[nVertices];
      for (int j=0; j<nVertices; j++) {
        tx[j] = psShape->padfX[j];
        ty[j] = psShape->padfY[j];
      }
      gc.geo2xy(area, nVertices, tx, ty);
      for (int j=0; j<nVertices; j++) {
        psShape->padfX[j] = tx[j];
        psShape->padfY[j] = ty[j];
      }
      delete[] tx;
      delete[] ty;
    }
    shapes.push_back(psShape);
  }

  SHPClose(hSHP);
  //


  //read dbf file

  readDBFfile(filename, dbfIntName, dbfIntDesc, dbfDoubleName,
      dbfDoubleDesc, dbfStringName, dbfStringDesc);

  colourmapMade=false;
  stringcolourmapMade=false;
  doublecolourmapMade=false;
  intcolourmapMade=false;

  //writeCoordinates writes a file with coordinates to teddb
  //writeCoordinates();

  return true;

}

bool ShapeObject::plot()
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
      miString descr=dbfStringDescr[shapes[i]->nShapeId];
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

  return true;

}

bool ShapeObject::plot(Area area, // current area
    double gcd, // size of plotarea in m
    bool land, // plot triangles
    bool cont, // plot contour-lines
    bool keepcont, // keep contourlines for later
    GLushort linetype, // contour line type
    float linewidth, // contour linewidth
    const uchar_t* lcolour, // contour linecolour
    const uchar_t* fcolour, // triangles fill colour
    const uchar_t* bcolour)
{
  float x1, y1, x2, y2;
  x1= area.R().x1;
  x2= area.R().x2;
  y1= area.R().y1;
  y2= area.R().y2;
  int divider=int(x2 - x1);
  int kstep=1;
  if (divider<12)
    divider=12;
  if (divider>200)
    divider=200;
#ifdef DEBUGPRINT
  cerr << "x1=" << x1 << endl;
  cerr << "x2=" << x2 << endl;
  cerr << "y1=" << y1 << endl;
  cerr << "y2=" << y2 << endl;
  cerr << "divider=" << divider << endl;
#endif
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  int n=shapes.size();
  for (int i=0; i<n; i++) {
    if (shapes[i]->nSHPType!=5)
      continue;
    int nparts=shapes[i]->nParts;
    if(nparts<=1) {
      kstep=1;
    } else {
      kstep=divider;
    }
    int *countpos= new int[nparts];//# of positions for each part
    int nv= shapes[i]->nVertices;
    GLdouble *gldata= new GLdouble[nv*3];
    int j=0;
    bool visible = false;
    for (int jpart=0; jpart<nparts; jpart++) {
      visible = false;
      int nstop, ncount=0;
      int nstart=shapes[i]->panPartStart[jpart];
      if (jpart==nparts-1)
        nstop=nv;
      else
        nstop=shapes[i]->panPartStart[jpart+1];
      if (shapes[i]->padfX[0] == shapes[i]->padfX[nstop-1]
          && shapes[i]->padfY[0] == shapes[i]->padfY[nstop-1])
        nstop--;
      if (cont) {
        glLineWidth(linewidth);
        if (linetype!=0xFFFF) {
          glLineStipple(1, linetype);
          glEnable(GL_LINE_STIPPLE);
        }
        glColor4ubv(lcolour);
        glBegin(GL_LINE_STRIP);
      }
      for (int k = nstart; k < nstop; k=k+kstep) {
        // check if point is inside area but only if shape is more than one polygon
         if ((shapes[i]->padfX[k] > x2+10.0) || (shapes[i]->padfX[k] < x1-10.0)
              || (shapes[i]->padfY[k] > y2+10.0) || (shapes[i]->padfY[k] < y1-10.0)) {
            continue;
          }
        if (cont) {
          glVertex2f(shapes[i]->padfX[k], shapes[i]->padfY[k]);
        }
        if (land) {
          gldata[j] = shapes[i]->padfX[k];
          gldata[j+1]= shapes[i]->padfY[k];
          gldata[j+2]= 0.0;
          j+=3;
          ncount++;
        }
      }
      // Insert last point to avoid leaking polygons
      if(land && (nstop/kstep != 0)) {
        gldata[j] = shapes[i]->padfX[nstop-1];
        gldata[j+1]= shapes[i]->padfY[nstop-1];
        gldata[j+2]= 0.0;
        j+=3;
        ncount++;
      }
      if(cont && (nstop/kstep != 0)) {
        glVertex2f(shapes[i]->padfX[nstop-1], shapes[i]->padfY[nstop-1]);
      }
      if(cont) {
        glEnd();
        glDisable(GL_LINE_STIPPLE);
      }
      if(ncount > 1) {
        visible = true;
      }
      countpos[jpart]=ncount;
    }
    if (visible) {
      glColor4ubv(fcolour);
      glLineWidth(linewidth);
      beginTesselation();
      tesselation(gldata, nparts, countpos);
      endTesselation();
    }
    delete[] gldata;
  }
  return true;
}

bool ShapeObject::getAnnoTable(miString & str)
{
  makeColourmap();
  str="table=\""+ fname;
  if (stringcolourmapMade) {
    map <miString,Colour>::iterator q=stringcolourmap.begin();
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
  if (poptions.fname.exists())
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
      miString descr=dbfStringDescr[shapes[i]->nShapeId];
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

int ShapeObject::readDBFfile(const miString& filename,
    vector<miString>& dbfIntName, vector< vector<int> >& dbfIntDesc,
    vector<miString>& dbfDoubleName, vector< vector<double> >& dbfDoubleDesc,
    vector<miString>& dbfStringName, vector< vector<miString> >& dbfStringDesc)
{
  DBFHandle hDBF;
  unsigned int i, n, iRecord;
  int nWidth, nDecimals;
  char szTitle[12];

  unsigned int nFieldCount, nRecordCount;

  vector<int> indexInt, indexDouble, indexString;

  vector<int> dummyint;
  vector<double> dummydouble;
  vector<miString> dummystring;
#ifdef DEBUGPRINT
  cerr<<"readDBFfile: "<<filename<<endl;
#endif
  /* -------------------------------------------------------------------- */
  /*      Open the file.                                                  */
  /* -------------------------------------------------------------------- */
  hDBF = DBFOpen(filename.cStr(), "rb");
  if (hDBF == NULL) {
    cerr<<"DBFOpen "<<filename<<" failed"<<endl;
    return 2;
  }

  /* -------------------------------------------------------------------- */
  /*	If there is no data in this file let the user know.		*/
  /* -------------------------------------------------------------------- */
  if (DBFGetFieldCount(hDBF) == 0) {
    cerr<<"There are no fields in this table!"<<endl;
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
    cerr<<"---> "<<szTitle<<endl;
#endif
    miString name= miString(szTitle).upcase();

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
  for (n=0; n<dbfIntName.size(); n++)
    cerr<<"Int    description:  "<<indexInt[n]<<"  "<<dbfIntName[n]<<endl;
  for (n=0; n<dbfDoubleName.size(); n++)
    cerr<<"Double description:  "<<indexDouble[n]<<"  "<<dbfDoubleName[n]
        <<endl;
  for (n=0; n<dbfStringName.size(); n++)
    cerr<<"String description:  "<<indexString[n]<<"  "<<dbfStringName[n]
        <<endl;
#endif
  /* -------------------------------------------------------------------- */
  /*	Read all the records 						*/
  /* -------------------------------------------------------------------- */

  for (n=0; n<dbfIntName.size(); n++) {
    i= indexInt[n];
    for (iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (DBFIsAttributeNULL(hDBF, iRecord, i) )
        dbfIntDesc[n].push_back(0);
      else
        dbfIntDesc[n].push_back(DBFReadIntegerAttribute(hDBF, iRecord, i) );
    }
  }

  for (n=0; n<dbfDoubleName.size(); n++) {
    i= indexDouble[n];
    for (iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (DBFIsAttributeNULL(hDBF, iRecord, i) )
        dbfDoubleDesc[n].push_back(0.0);
      else {
        dbfDoubleDesc[n].push_back(DBFReadDoubleAttribute(hDBF, iRecord, i) );
#ifdef DEBUGPRINT
        if (iRecord < 10)
          cerr << "DBFReadDoubleAttribute( hDBF, iRecord, i )"
              << DBFReadDoubleAttribute(hDBF, iRecord, i) << endl;
#endif
      }
    }
  }

  for (n=0; n<dbfStringName.size(); n++) {
    i= indexString[n];
    for (iRecord=0; iRecord<nRecordCount; iRecord++) {
      if (DBFIsAttributeNULL(hDBF, iRecord, i) )
        dbfStringDesc[n].push_back("-");
      else {
        dbfStringDesc[n].push_back(miString(DBFReadStringAttribute(hDBF,
            iRecord, i) ) );
#ifdef DEBUGPRINT
        if (iRecord < 10)
          cerr << "DBFReadStringAttribute( hDBF, iRecord, i )"
              << DBFReadStringAttribute(hDBF, iRecord, i) << endl;
#endif
      }
    }
  }

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
      for (i=0; i<n; i++) {
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
  cerr<<"nFieldCount=      "<<nFieldCount<<endl;
  cerr<<"nRecordCount=     "<<nRecordCount<<endl;

  for (n=0; n<dbfIntName.size(); n++)
    cerr<<"Int    description, size,name:  "<<dbfIntDesc[n].size()<<"  "
        <<dbfIntName[n]<<endl;
  for (n=0; n<dbfDoubleName.size(); n++)
    cerr<<"Double description, size,name:  "<<dbfDoubleDesc[n].size()<<"  "
        <<dbfDoubleName[n]<<endl;
  for (n=0; n<dbfStringName.size(); n++)
    cerr<<"String description, size,name:  "<<dbfStringDesc[n].size()<<"  "
        <<dbfStringName[n]<<endl;
#endif
  return 0;
}

void ShapeObject::writeCoordinates()
{
  cerr << "ShapeObject:writeCoordinates NOT IMPLEMENTED" << endl;
  /*****************************************************************************
   cerr << "ShapeObject:writeCoordinates" << endl;
   // open filestream
   ofstream dbfile("shapelocations.txt");
   if (!dbfile){
   cerr << "ERROR OPEN (WRITE) " << endl;
   return;
   }

   int n=shapes.size();
   for (int i=0;i<n;i++){
   if (shapes[i]->nSHPType!=5)
   continue;
   int nr=650+i;
   dbfile << nr << "|" << "shape " << i << "|county|1|";
   int nparts=shapes[i]->nParts;
   cerr << "number of parts " << nparts << endl;
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
   cerr << "x=" <<  shapes[i]->padfX[k];
   cerr << "  y =" << shapes[i]->padfY[k] << endl;
   miCoordinates newcor(float(shapes[i]->padfX[k]),float(shapes[i]->padfY[k]));
   cerr << newcor.str() << endl;
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
