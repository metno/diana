/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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

#include "diField.h"
#include "diGridConverter.h"

#define MILOGGER_CATEGORY "diField.Field"
#include "miLogger/miLogging.h"

using namespace std;
using namespace miutil;

Field::Field()
  : data(0),
allDefined(true),level(0), idnum(0),
forecastHour(0), aHybrid(-1.),bHybrid(-1.), discontinuous(false),
numSmoothed(0), gridChanged(false), difference(false),
turnWaveDirection(false), isPartOfCache(false)
{
  METLIBS_LOG_SCOPE();
  lastAccessed = miTime::nowTime();
}

Field::Field(const Field &rhs)
  : data(0)
{
  METLIBS_LOG_SCOPE();
  memberCopy(rhs);
}

Field::~Field()
{
  METLIBS_LOG_SCOPE();
  if (isPartOfCache)
    METLIBS_LOG_ERROR("Somebody tries to delete a Field owned by Cache!");

  cleanup();
}

Field& Field::operator=(const Field &rhs)
{
  METLIBS_LOG_SCOPE();
  if (this != &rhs)
    memberCopy(rhs);
  return *this;
}

bool Field::operator==(const Field &rhs) const
{
  return area == rhs.area;
}

bool Field::operator!=(const Field &rhs) const
{
  return !(*this == rhs);
}

void Field::shallowMemberCopy(const Field& rhs)
{
  METLIBS_LOG_SCOPE();

  // the isPartOfCache variable is always false in copy actions.
  // the Cache is (as a friend) the only instance to set this
  // marker to true... copying a field outside the cache is possible
  // sets this marker to false
  isPartOfCache=false;
  lastAccessed=miTime::nowTime();

  area= rhs.area;
  allDefined= rhs.allDefined;
  level= rhs.level;
  idnum= rhs.idnum;
  forecastHour= rhs.forecastHour;
  validFieldTime= rhs.validFieldTime;
  analysisTime= rhs.analysisTime;

  aHybrid= rhs.aHybrid;
  bHybrid= rhs.bHybrid;
  discontinuous= rhs.discontinuous;
  palette= rhs.palette;
  unit = rhs.unit;

  numSmoothed= rhs.numSmoothed;
  gridChanged= rhs.gridChanged;
  validFieldTime= rhs.validFieldTime;
  turnWaveDirection= rhs.turnWaveDirection;

  name= rhs.name;
  text= rhs.text;
  fulltext= rhs.fulltext;
  modelName= rhs.modelName;
  paramName= rhs.paramName;
  fieldText= rhs.fieldText;
  leveltext= rhs.leveltext;
  idnumtext= rhs.idnumtext;
  progtext= rhs.progtext;
  timetext= rhs.timetext;
}

void Field::memberCopy(const Field& rhs)
{
  METLIBS_LOG_SCOPE();

  // the isPartOfCache variable is always false in copy actions.
  // the Cache is (as a friend) the only instance to set this
  // marker to true... copying a field outside the cache is possible
  // sets this marker to false
  isPartOfCache=false;
  lastAccessed=miTime::nowTime();

  if (area.nx != rhs.area.nx || area.ny != rhs.area.ny) {
    delete[] data;
    data= 0;
  }

  const int fsize = rhs.area.gridSize();
  if (!data && fsize)
    data= new float[fsize];

  for (int i=0; i<fsize; i++)
    data[i] = rhs.data[i];

  area=           rhs.area;
  allDefined=     rhs.allDefined;
  level=          rhs.level;
  idnum=          rhs.idnum;
  forecastHour=   rhs.forecastHour;
  validFieldTime= rhs.validFieldTime;
  analysisTime=   rhs.analysisTime;

  aHybrid=        rhs.aHybrid;
  bHybrid=        rhs.bHybrid;
  unit =          rhs.unit;
  discontinuous =rhs.discontinuous;
  palette       =rhs.palette;

  numSmoothed=    rhs.numSmoothed;
  gridChanged=    rhs.gridChanged;
  validFieldTime= rhs.validFieldTime;
  turnWaveDirection= rhs.turnWaveDirection;

  name=           rhs.name;
  text=           rhs.text;
  fulltext=       rhs.fulltext;
  modelName=      rhs.modelName;
  paramName=      rhs.paramName;
  fieldText=      rhs.fieldText;
  leveltext=      rhs.leveltext;
  idnumtext=      rhs.idnumtext;
  progtext=       rhs.progtext;
  timetext=       rhs.timetext;
}

// estimate the size of this particular field in bytes

long Field::bytesize()
{
  long bytes=0;

  // data
  bytes+= area.gridSize()*sizeof(float);

  // numSmoothed;forecastHour
  bytes+= 2*sizeof(int);

  // aHybrid;bHybrid
  bytes+= 2*sizeof(float);

  // allDefined;gridChanged;difference;
  bytes+= 3*sizeof(bool);

  // validFieldTime;analysisTime;
  bytes+= 2*sizeof(miTime);

  // all strings
  bytes+= (name.length()+text.length()+fulltext.length()+modelName.length()+paramName.length()
      +fieldText.length()+leveltext.length()+idnumtext.length()
      +progtext.length()+timetext.length()) * sizeof(char);

  // the area
  bytes+= sizeof(Area);

  return bytes;
}

void Field::reserve(int xdim, int ydim)
{
  cleanup();

  area.nx = xdim;
  area.ny = ydim;

  if (area.nx>0 && area.ny>0)
    data= new float[area.gridSize()];
}

void Field::cleanup()
{
  METLIBS_LOG_SCOPE();
  area.nx  = 0;
  area.ny  = 0;
  delete[] data;
  data= 0;

  allDefined= true;
  numSmoothed= 0;
}

bool Field::subtract(const Field &rhs)
{
  if (area != rhs.area)
    return false;

  const int fsize = area.gridSize();
  const bool ad = (allDefined && rhs.allDefined);
  for (int i=0; i<fsize; i++) {
    if (ad || (data[i] != fieldUndef && rhs.data[i] != fieldUndef)) {
      data[i] -= rhs.data[i];
    } else {
      data[i] = fieldUndef;
      allDefined = false;
    }
  }
  return true;
}

#if 0
namespace {
typedef void (*operation_t)(float& d, float rhs_d);
} // namespace

bool Field::performOperation(const Field &rhs, operation_t op)
{
  if (area != rhs.area)
    return false;

  const int fsize = area.gridSize();
  const bool ad = (allDefined && rhs.allDefined);
  for (int i=0; i<fsize; i++)
    if (ad || (data[i] != fieldUndef && rhs.data[i] != fieldUndef)) {
      op(data[i], rhs.data[i]);
    } else {
      data[i] = fieldUndef;
      allDefined = false;
    }
  }
  return true;
}

bool Field::add(const Field &rhs)
{
  if (area != rhs.area)
    return false;

  const int fsize = area.gridSize();
  const bool ad = (allDefined && rhs.allDefined);
  for (int i=0; i<fsize; i++)
    if (ad || (data[i] != fieldUndef && rhs.data[i] != fieldUndef)) {
      data[i] += rhs.data[i];
    } else {
      data[i] = fieldUndef;
      allDefined = false;
    }
  }
  return true;
}

// Multiply by a field
bool Field::multiply(const Field &rhs)
{
  if (area != rhs.area)
    return false;

  int fsize = area.gridSize();
  if (allDefined && rhs.allDefined) {
    for (int i=0; i<fsize; i++)
      data[i]*=rhs.data[i];
  } else {
    for (int i=0; i<fsize; i++)
      if (data[i]!=fieldUndef && rhs.data[i]!=fieldUndef)
        data[i]*=rhs.data[i];
      else
        data[i]= fieldUndef;
  }
  return true;
}


// Divide by a field (division by 0 = fieldUndef)
bool Field::divide(const Field &rhs)
{
  if (area != rhs.area)
    return false;

  int fsize = area.gridSize();
  if (allDefined && rhs.allDefined) {
    for (int i=0; i<fsize; i++)
      if (rhs.data[i]!=0.0)
        data[i]/=rhs.data[i];
      else
        data[i]= fieldUndef;
  } else {
    for (int i=0; i<fsize; i++)
      if (data[i]!=fieldUndef && rhs.data[i]!=fieldUndef
          && rhs.data[i]!=0.0)
        data[i]/=rhs.data[i];
      else
        data[i]= fieldUndef;
  }
  return true;
}


// Difference between another field and this
bool Field::differ(const Field &rhs)
{
  if (area != rhs.area)
    return false;

  int fsize = area.gridSize();
  if (allDefined && rhs.allDefined) {
    for (int i=0; i<fsize; i++)
      data[i]= rhs.data[i] - data[i];
  } else {
    for (int i=0; i<fsize; i++)
      if (data[i]!=fieldUndef && rhs.data[i]!=fieldUndef)
        data[i]= rhs.data[i] - data[i];
      else
        data[i]= fieldUndef;
  }
  return true;
}


// Negate
void Field::negate()
{
  int fsize = area.gridSize();
  if (allDefined) {
    for (int i=0; i<fsize; i++)
      data[i]= -data[i];
  } else {
    for (int i=0; i<fsize; i++)
      if (data[i]!=fieldUndef)
        data[i]= -data[i];
  }
}


// Set constant value
void Field::setValue(float value)
{
  int fsize = area.gridSize();
  if (allDefined) {
    for (int i=0; i<fsize; i++)
      data[i]= value;
  } else {
    for (int i=0; i<fsize; i++)
      if (data[i]!=fieldUndef)
        data[i]= value;
  }
}

// Set all values undefined
void Field::setUndefined()
{
  int fsize = area.gridSize();
  allDefined = false;
  for (int i=0; i<fsize; i++) {
    data[i]= fieldUndef;
  }
}
#endif

bool Field::smooth(int nsmooth)
{
  if (nsmooth<1)
    return true;

  float *work = new float[area.gridSize()];
  float *worku1 = 0;
  float *worku2 = 0;

  if (!allDefined) {
    worku1= new float[area.gridSize()];
    worku2= new float[area.gridSize()];
  }

  bool res= smooth(nsmooth, work, worku1, worku2);

  delete[] work;
  delete[] worku1;
  delete[] worku2;

  return res;
}


bool Field::smooth(int nsmooth, float *work,
    float *worku1, float *worku2)
{
  //  Low-bandpass filter, removing short wavelengths
  //  (not a 2nd or 4th order Shapiro filter)
  //
  //  G.J.Haltiner, Numerical Weather Prediction,
  //                   Objective Analysis,
  //                       Smoothing and filtering
  //
  //  input:   nsmooth       - no. of iterations (1,2,3,...),
  //                           nsmooth<0 => '-nsmooth' iterations and
  //                           using input work as mask
  //           work[nx*ny]   - a work matrix (size as the field),
  //                           or input mask if nsmooth<0
  //                           (0.0=not smooth, 1.0=full smooth)
  //           worku1[nx*ny] - a work matrix (only used if !allDefined or nsmooth<0)
  //           worku2[nx*ny] - a work matrix (only used if !allDefined or nsmooth<0)

  const float s = 0.25;
  int   size = area.gridSize();
  int   i, j, n, i1, i2;

  if (area.nx<3 || area.ny<3)
    return false;
  if (nsmooth==0)
    return true;

  if (allDefined && nsmooth>0) {

    for (n=0; n<nsmooth; n++) {

      // loop extended, reset below
      for (i=1; i<size-1; ++i)
        work[i] = data[i] + s * (data[i-1] + data[i+1] - 2.*data[i]);

      i1 = 0;
      i2 = area.nx - 1;
      for (j=0; j<area.ny; ++j, i1+=area.nx, i2+=area.nx) {
        work[i1] = data[i1];
        work[i2] = data[i2];
      }

      // loop extended, reset below
      for (i=area.nx; i<size-area.nx; ++i)
        data[i] = work[i] + s * (work[i-area.nx] + work[i+area.nx] - 2.*work[i]);

      i2 = size - area.nx;
      for (i1=0; i1<area.nx; ++i1, ++i2) {
        data[i1] = work[i1];
        data[i2] = work[i2];
      }

    }

  } else {

    if (!allDefined) {
      // loops extended, no problem
      for (i=1; i<size-1; ++i)
        worku1[i] = (data[i-1]!=fieldUndef && data[i]  !=fieldUndef
            && data[i+1]!=fieldUndef ) ? s : 0.;
      for (i=area.nx; i<size-area.nx; ++i)
        worku2[i] = (data[i-area.nx]!=fieldUndef && data[i]   !=fieldUndef
            && data[i+area.nx]!=fieldUndef ) ? s : 0.;
      if (nsmooth<0) {
        for (i=1;  i<size-1;  ++i) worku1[i] = worku1[i]*work[i];
        for (i=area.nx; i<size-area.nx; ++i) worku2[i] = worku2[i]*work[i];
      }
    } else {
      for (i=1;  i<size-1;  ++i) worku1[i] = s*work[i];
      for (i=area.nx; i<size-area.nx; ++i) worku2[i] = s*work[i];
    }

    if (nsmooth<0) nsmooth= -nsmooth;

    for (n=0; n<nsmooth; n++) {

      // loop extended, reset below
      for (i=1; i<size-1; ++i)
        work[i] = data[i] + worku1[i] * (data[i-1] + data[i+1] - 2.*data[i]);

      i1 = 0;
      i2 = area.nx - 1;
      for (j=0; j<area.ny; ++j, i1+=area.nx, i2+=area.nx) {
        work[i1] = data[i1];
        work[i2] = data[i2];
      }

      // loop extended, reset below
      for (i=area.nx; i<size-area.nx; ++i)
        data[i] = work[i] + worku2[i] * (work[i-area.nx] + work[i+area.nx] - 2.*work[i]);

      i2 = size - area.nx;
      for (i1=0; i1<area.nx; ++i1, ++i2) {
        data[i1] = work[i1];
        data[i2] = work[i2];
      }
    }
  }

  numSmoothed+= nsmooth;

  return true;
}


bool Field::interpolate(int npos, const float *xpos, const float *ypos,
    float *zpos, InterpolationType itype) const
{
  /*
   * PURPOSE:    interpoaltion in field data
   * ALGORITHM:
   * ARGUMENTS:
   *    interpoltype=  0: bilinear interpolation (2x2 points used)
   *    interpoltype=  1: bessel   interpolation (4x4 points used)
   *                      (2x2 points used near borders and undefined points)
   *    interpoltype=  2: nearest gridpoint
   *    interpoltype=100: as type 0 with extrapolation at boundaries
   *    interpoltype=101: as type 1 with extrapolation at boundaries
   *    interpoltype=102: as type 2 with extrapolation at boundaries
   */
  METLIBS_LOG_SCOPE();
  if (!data)
    return false;

  int   n,i,j,ij;
  float x,y,x1,x2,x3,x4,y1,y2,y3,y4,a,b,c,d,at,bt,ct,dt,t1,t2,t3,t4;

  float xmin= -0.03;
  float xmax= area.nx-1+0.03;
  float ymin= -0.03;
  float ymax= area.ny-1+0.03;

  int interpoltype = itype;
  if (interpoltype >= I_BILINEAR_EX && interpoltype != I_NEAREST_EX) {
    interpoltype -= 100;
    xmin= -1.e+35;
    xmax=  1.e+35;
    ymin= -1.e+35;
    ymax=  1.e+35;
  }

  if (interpoltype == I_NEAREST) {
    for (n=0; n<npos; ++n) {
      x=xpos[n];
      y=ypos[n];
      if (x>xmin && x<xmax && y>ymin && y<ymax) {
        i=int(x+1.5)-1;
        j=int(y+1.5)-1;
        if (i<0)    i=0;
        if (i>area.nx-1) i=area.nx-1;
        if (j<0)    j=0;
        if (j>area.ny-1) j=area.ny-1;
        zpos[n]= data[j*area.nx+i];
      } else {
        zpos[n]= fieldUndef;
      }
    }

  } else if (interpoltype == I_NEAREST_EX) {
    for (n=0; n<npos; ++n) {
      i=int(xpos[n]+1.5)-1;
      j=int(ypos[n]+1.5)-1;
      if (i<0)    i=0;
      if (i>area.nx-1) i=area.nx-1;
      if (j<0)    j=0;
      if (j>area.ny-1) j=area.ny-1;
      zpos[n]= data[j*area.nx+i];
    }
  } else if (allDefined) {
    if (interpoltype == I_BILINEAR) {
      for (n=0; n<npos; ++n) {
        x=xpos[n];
        y=ypos[n];
        i=int(x+1.)-1;
        j=int(y+1.)-1;
        if (i>-1 && i<area.nx-1 && j>-1 && j<area.ny-1) {
          // bilinear interpolation (2x2 points)
          x1=x-float(i);
          y1=y-float(j);
          a=(1.-y1)*(1.-x1);
          b=(1.-y1)*x1;
          c=y1*(1.-x1);
          d=y1*x1;
          ij=j*area.nx+i;
          zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
        } else if (x>xmin && x<xmax && y>ymin && y<ymax) {
          if (i<0)    i=0;
          if (i>area.nx-2) i=area.nx-2;
          if (j<0)    j=0;
          if (j>area.ny-2) j=area.ny-2;
          x1=x-float(i);
          y1=y-float(j);
          a=(1.-y1)*(1.-x1);
          b=(1.-y1)*x1;
          c=y1*(1.-x1);
          d=y1*x1;
          ij=j*area.nx+i;
          zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
        } else {
          zpos[n]= fieldUndef;
        }
      }

    } else {           // interpoltype == I_BESSEL
      for (n=0; n<npos; ++n) {
        x=xpos[n];
        y=ypos[n];
        i=int(x+1.)-1;
        j=int(y+1.)-1;
        if (i>0 && i<area.nx-2 && j>0 && j<area.ny-2) {
          // bessel interpolation (4x4 points)
          x1=x-float(i);
          x2=1.-x1;
          x3=-0.25*x1*x2;
          x4=-0.1666667*x1*x2*(x1-0.5);
          a=x3-x4;
          b=x2-x3+3.*x4;
          c=x1-x3-3.*x4;
          d=x3+x4;
          y1=y-float(j);
          y2=1.-y1;
          y3=-0.25*y1*y2;
          y4=-0.1666667*y1*y2*(y1-0.5);
          at=y3-y4;
          bt=y2-y3+3.*y4;
          ct=y1-y3-3.*y4;
          dt=y3+y4;
          ij=j*area.nx+i;
          t1=a*data[ij-area.nx-1]  +b*data[ij-area.nx]  +c*data[ij-area.nx+1]  +d*data[ij-area.nx+2]  ;
          t2=a*data[ij-1]     +b*data[ij]     +c*data[ij+1]     +d*data[ij+2]     ;
          t3=a*data[ij+area.nx-1]  +b*data[ij+area.nx]  +c*data[ij+area.nx+1]  +d*data[ij+area.nx+2]  ;
          t4=a*data[ij+area.nx*2-1]+b*data[ij+area.nx*2]+c*data[ij+area.nx*2+1]+d*data[ij+area.nx*2+2];
          zpos[n]=at*t1+bt*t2+ct*t3+dt*t4;
        } else if (i>-1 && i<area.nx-1 && j>-1 && j<area.ny-1) {
          // bilinear interpolation (2x2 points)
          x1=x-float(i);
          y1=y-float(j);
          a=(1.-y1)*(1.-x1);
          b=(1.-y1)*x1;
          c=y1*(1.-x1);
          d=y1*x1;
          ij=j*area.nx+i;
          zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
        } else if (x>xmin && x<xmax && y>ymin && y<ymax) {
          // extrapolation at boundaries
          if (i<0)    i=0;
          if (i>area.nx-2) i=area.nx-2;
          if (j<0)    j=0;
          if (j>area.ny-2) j=area.ny-2;
          x1=x-float(i);
          y1=y-float(j);
          a=(1.-y1)*(1.-x1);
          b=(1.-y1)*x1;
          c=y1*(1.-x1);
          d=y1*x1;
          ij=j*area.nx+i;
          zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
        } else {
          zpos[n]= fieldUndef;
        }
      }
    }

  } else {

    if (interpoltype == I_BILINEAR) {

      for (n=0; n<npos; ++n) {
        zpos[n]= fieldUndef;
        x=xpos[n];
        y=ypos[n];
        i=int(x+1.)-1;
        j=int(y+1.)-1;
        if (i>-1 && i<area.nx-1 && j>-1 && j<area.ny-1) {
          ij=j*area.nx+i;
          if (data[ij]   !=fieldUndef && data[ij+1]   !=fieldUndef &&
              data[ij+area.nx]!=fieldUndef && data[ij+area.nx+1]!=fieldUndef) {
            // bilinear interpolation (2x2 points)
            x1=x-float(i);
            y1=y-float(j);
            a=(1.-y1)*(1.-x1);
            b=(1.-y1)*x1;
            c=y1*(1.-x1);
            d=y1*x1;
            zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
          }
        } else if (x>xmin && x<xmax && y>ymin && y<ymax) {
          if (i<0)    i=0;
          if (i>area.nx-2) i=area.nx-2;
          if (j<0)    j=0;
          if (j>area.ny-2) j=area.ny-2;
          ij=j*area.nx+i;
          if (data[ij]   !=fieldUndef && data[ij+1]   !=fieldUndef &&
              data[ij+area.nx]!=fieldUndef && data[ij+area.nx+1]!=fieldUndef) {
            // bilinear interpolation (2x2 points)
            x1=x-float(i);
            y1=y-float(j);
            a=(1.-y1)*(1.-x1);
            b=(1.-y1)*x1;
            c=y1*(1.-x1);
            d=y1*x1;
            zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
          }
        }
      }

    } else { // interpoltype == I_BESSEL
      bool bilin;
      for (n=0; n<npos; ++n) {
        zpos[n]=fieldUndef;
        x=xpos[n];
        y=ypos[n];
        i=int(x+1.)-1;
        j=int(y+1.)-1;
        bilin= true;
        if (i>0 && i<area.nx-2 && j>0 && j<area.ny-2) {
          ij=j*area.nx+i;
          if (data[ij-area.nx-1]  !=fieldUndef && data[ij-area.nx]    !=fieldUndef &&
              data[ij-area.nx+1]  !=fieldUndef && data[ij-area.nx+2]  !=fieldUndef &&
              data[ij-1]     !=fieldUndef && data[ij]       !=fieldUndef &&
              data[ij+1]     !=fieldUndef && data[ij+2]     !=fieldUndef &&
              data[ij+area.nx-1]  !=fieldUndef && data[ij+area.nx]    !=fieldUndef &&
              data[ij+area.nx+1]  !=fieldUndef && data[ij+area.nx+2]  !=fieldUndef &&
              data[ij+area.nx*2-1]!=fieldUndef && data[ij+area.nx*2]  !=fieldUndef &&
              data[ij+area.nx*2+1]!=fieldUndef && data[ij+area.nx*2+2]!=fieldUndef) {
            bilin= false;
            // bessel interpolation (4x4 points)
            x1=x-float(i);
            x2=1.-x1;
            x3=-0.25*x1*x2;
            x4=-0.1666667*x1*x2*(x1-0.5);
            a=x3-x4;
            b=x2-x3+3.*x4;
            c=x1-x3-3.*x4;
            d=x3+x4;
            y1=y-float(j);
            y2=1.-y1;
            y3=-0.25*y1*y2;
            y4=-0.1666667*y1*y2*(y1-0.5);
            at=y3-y4;
            bt=y2-y3+3.*y4;
            ct=y1-y3-3.*y4;
            dt=y3+y4;
            t1=a*data[ij-area.nx-1]  +b*data[ij-area.nx]  +c*data[ij-area.nx+1]  +d*data[ij-area.nx+2]  ;
            t2=a*data[ij-1]     +b*data[ij]     +c*data[ij+1]     +d*data[ij+2]     ;
            t3=a*data[ij+area.nx-1]  +b*data[ij+area.nx]  +c*data[ij+area.nx+1]  +d*data[ij+area.nx+2]  ;
            t4=a*data[ij+area.nx*2-1]+b*data[ij+area.nx*2]+c*data[ij+area.nx*2+1]+d*data[ij+area.nx*2+2];
            zpos[n]=at*t1+bt*t2+ct*t3+dt*t4;
          }
        }
        if (bilin) {
          if (i>-1 && i<area.nx-1 && j>-1 && j<area.ny-1) {
            ij=j*area.nx+i;
            if (data[ij]   !=fieldUndef && data[ij+1]   !=fieldUndef &&
                data[ij+area.nx]!=fieldUndef && data[ij+area.nx+1]!=fieldUndef) {
              // bilinear interpolation (2x2 points)
              x1=x-float(i);
              y1=y-float(j);
              a=(1.-y1)*(1.-x1);
              b=(1.-y1)*x1;
              c=y1*(1.-x1);
              d=y1*x1;
              zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
            }
          } else if (x>xmin && x<xmax && y>ymin && y<ymax) {
            // extrapolation at boundaries
            if (i<0)    i=0;
            if (i>area.nx-2) i=area.nx-2;
            if (j<0)    j=0;
            if (j>area.ny-2) j=area.ny-2;
            ij=j*area.nx+i;
            if (data[ij]   !=fieldUndef && data[ij+1]   !=fieldUndef &&
                data[ij+area.nx]!=fieldUndef && data[ij+area.nx+1]!=fieldUndef) {
              x1=x-float(i);
              y1=y-float(j);
              a=(1.-y1)*(1.-x1);
              b=(1.-y1)*x1;
              c=y1*(1.-x1);
              d=y1*x1;
              zpos[n]= a*data[ij]+b*data[ij+1]+c*data[ij+area.nx]+d*data[ij+area.nx+1];
            }
          }
        }
      }
    }
  }
  return true;
}


bool Field::changeGrid(const GridArea& anew, const std::string& demands)
{
  /*
   * PURPOSE:    change the grid by interplation in original field
   * ALGORITHM:
   * ARGUMENTS:
   *    demands= "fine.interpolation"   : bessel interp. (default)
   *    demands= "simple.interpolation" : bilinear interp.
   */
  METLIBS_LOG_SCOPE();
  if (!data)
    return false;

  if (area == anew)
    return true;

  size_t l= demands.length();
  InterpolationType interpoltype = I_BESSEL;
  if (demands.find("simple.interpolation",0)<l)
    interpoltype = I_BILINEAR;

  int nxnew= int(anew.R().width() /anew.resolutionX+0.5)+1;
  int nynew= int(anew.R().height()/anew.resolutionY+0.5)+1;

  if (nxnew<2 || nynew<2)
    return false;

  float *newdata= new float[nxnew*nynew];

  float *x, *y;
  GridConverter gc;
  gc.getGridPoints(anew, area, false, &x, &y);

  int ii = 0;
  for (int iy = 0; iy < anew.ny; iy++) {
    for (int ix = 0; ix < anew.nx; ix++) {
      x[ii] /= area.resolutionX;
      y[ii] /= area.resolutionY;
      ii++;
    }
  }

  if (!interpolate(anew.gridSize(), x, y, newdata, interpoltype)) {
    delete[] newdata;
    METLIBS_LOG_ERROR("Interpolation failure");
    return false;
  }

  delete[] data;
  data = newdata;
  area = anew;

  int i=0;
  while (i<area.gridSize() && data[i]!=fieldUndef)
    i++;
  allDefined= (i==area.gridSize());

  gridChanged= true;

  return true;
}

void Field::convertToGrid(int npos, float* xpos, float* ypos) const
{
  for (int i = 0; i < npos; i++) {
    xpos[i] /= area.resolutionX;
    ypos[i] /= area.resolutionY;
  }
}

void Field::convertFromGrid(int npos, float* xpos, float* ypos) const
{
  for (int i = 0; i < npos; i++) {
    xpos[i] *= area.resolutionX;
    ypos[i] *= area.resolutionY;
  }
}

ostream& operator<<(ostream& output, const Field& f)
{
  return output
      << "Name="      << f.name << " "
      << "modelName=" << f.modelName << " "
      << "paramName=" << f.paramName << endl

      << "level="     << f.level << " "
      << "idnum="     << f.idnum << endl

      << "forcasthour="    << f.forecastHour << " "
      << "validfieldtime=" << f.validFieldTime << " "
      << "analysisTime="   << f.analysisTime << endl

      << "datapointer=" << (long)f.data << " "
      << "area.nx,area.ny=" << f.area.nx << "," << f.area.ny << " "
      << "gridResolutionX,gridResolutionY=" << f.area.resolutionX << "," << f.area.resolutionY;
}
