/*
  Diana - A Free Meteorological Visualisation Tool

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

#include "diana_config.h"

#include "diSpectrumFile.h"
#include "diSpectrumPlot.h"
#include "diFtnVfile.h"

#include <puCtools/stat.h>
#include <puTools/miStringFunctions.h>

#include <map>

#define MILOGGER_CATEGORY "diana.SpectrumFile"
#include <miLogger/miLogging.h>

static const float DEG_TO_RAD = M_PI / 180;

using namespace miutil;

SpectrumFile::SpectrumFile(const std::string& filename, const std::string& modelname)
  : fileName(filename), modelName(modelname), vfile(0), modificationtime(0),
    numPos(0), numTime(0), numDirec(0), numFreq(0), numExtra(0),
    dataAddress(0)
{
  METLIBS_LOG_SCOPE();
}

SpectrumFile::~SpectrumFile()
{
  METLIBS_LOG_SCOPE();
  cleanup();
}

void SpectrumFile::cleanup()
{
  METLIBS_LOG_SCOPE(LOGVAL(fileName));

  delete[] dataAddress;
  delete vfile;
  dataAddress= 0;
  vfile= 0;
  modificationtime= 0;
}

bool SpectrumFile::update()
{
  METLIBS_LOG_SCOPE(LOGVAL(fileName));

  bool ok= true;

  pu_struct_stat statbuf;

  if (pu_stat(fileName.c_str(),&statbuf)==0) {
    if (modificationtime != statbuf.st_ctime) {
      cleanup();
      if (readFileHeader())
        modificationtime= statbuf.st_ctime;
      else
        ok= false;
    }
  } else {
    ok= false;
  }

  if (!ok)
    cleanup();

  return ok;
}


bool SpectrumFile::readFileHeader()
{
  METLIBS_LOG_SCOPE(LOGVAL(fileName));

  // reading and storing information, not data

  int bufferlength= 1024;

  vfile= new FtnVfile(fileName, bufferlength);

  int n, i, length, iscale, iundef, ltext;
  int *head, *spinfo;
  int *tmp;

  iundef= 0;

  numPos= numTime= numDirec= numFreq= 0;

  try {

    vfile->init();
    length= 4;
    head= vfile->getInt(length);
    if (head[0]!=251 || head[1]!=1 || head[2]!=bufferlength)
      throw VfileError();
    length= head[3];
    if (length<6)
      throw VfileError();
    spinfo= vfile->getInt(length);

    numPos=   spinfo[ 0];
    numTime=  spinfo[ 1];
    numDirec= spinfo[ 2];
    numFreq=  spinfo[ 3];
    numExtra= spinfo[ 4];
    ltext=    spinfo[ 5];

    delete[] spinfo;

    if (numPos<1 || numTime<1 ||
        numDirec<4 || numFreq<2 || numExtra<6+2)
      throw VfileError();

    // model name or some text
    modelName2= vfile->getString(ltext);

    iundef= 0;

    iscale= vfile->getInt();
    posLatitude= vfile->getFloatVector(numPos,iscale,iundef);

    iscale= vfile->getInt();
    posLongitude= vfile->getFloatVector(numPos,iscale,iundef);

    // position names, first length of each name
    tmp= vfile->getInt(numPos);
    std::map<std::string, int> namemap;
    for (n=0; n<numPos; n++) {
      std::string str= vfile->getString(tmp[n]);
      // may have one space at the end (n*2 characters stored in file)
      miutil::trim(str, false, true);
      if(!namemap.count(str)){
        namemap[str]=0;
      } else {
        namemap[str]++;
      }
      if(namemap[str]>0) str = str + "(" + miutil::from_number(namemap[str]) + ")";
      posName.push_back(str);
    }
    delete[] tmp;

    // time
    tmp= vfile->getInt(numTime*6);
    i=0;
    for (n=0; n<numTime; n++) {
      int year  = tmp[i++];
      int month = tmp[i++];
      int day   = tmp[i++];
      int hour  = tmp[i++];
      int minute= tmp[i++];
      int fchour= tmp[i++];
      miTime t= miTime(year,month,day,hour,minute,0);
      validTime.push_back(t);
      forecastHour.push_back(fchour);
    }
    delete[] tmp;

    iscale= vfile->getInt();
    directions= vfile->getFloatVector(numDirec,iscale,iundef);

    iscale= vfile->getInt();
    frequences= vfile->getFloatVector(numFreq,iscale,iundef);

    std::vector<int> decscale = vfile->getIntVector(numExtra);
    extraScale.clear();
    for (int i=0; i<numExtra; i++)
      extraScale.push_back(powf(10.0,decscale[i]));

    // pointer to data for each position and time
    // (pointer as fortran record and word no.)
    dataAddress= vfile->getInt(2*numPos*numTime);

  }  // end of try

  catch (...) {
    METLIBS_LOG_ERROR("Bad Spectrum file: " << fileName);
    delete vfile;
    vfile= 0;
    return false;
  }

  return true;
}


SpectrumPlot* SpectrumFile::getData(const std::string& name, const miTime& time)
{
  METLIBS_LOG_SCOPE(LOGVAL(name) << LOGVAL(time));

  SpectrumPlot *spp= 0;

  if (!vfile) return spp;

  int iPos=0;
  while (iPos<numPos && posName[iPos]!=name)
    iPos++;

  if (iPos==numPos)
    return spp;

  int iTime= 0;
  while (iTime<numTime && validTime[iTime]!=time)
    iTime++;
  if (iTime==numTime)
    return spp;

  spp= new SpectrumPlot();

  spp->prognostic= true;

  spp->modelName= modelName2;
  spp->posName= posName[iPos];

  spp->numDirec= numDirec;
  spp->numFreq=  numFreq;

  spp->validTime= validTime[iTime];
  spp->forecastHour= forecastHour[iTime];

  spp->directions= directions;
  spp->frequences= frequences;

  float *spec= 0;

  try {

    // set start position in file ("fortran" record and word)
    int record= dataAddress[iTime*numPos*2+iPos*2];
    int word=   dataAddress[iTime*numPos*2+iPos*2+1];
    vfile->setFilePosition(record,word);

    int *tmp= vfile->getInt(numExtra);
    spp->northRotation= tmp[0]*extraScale[0];
    spp->wspeed=        tmp[1]*extraScale[1];
    spp->wdir=          tmp[2]*extraScale[2] + 180;
    spp->hmo=           tmp[3]*extraScale[3];
    spp->tPeak=         tmp[4]*extraScale[4];
    spp->ddPeak=        tmp[5]*extraScale[5] + 180;

    int iundef= tmp[numExtra-2];
    int iscale= tmp[numExtra-1];

    spec= vfile->getFloat(numDirec*numFreq,iscale,iundef);
  }  // end of try

  catch (...) {
    METLIBS_LOG_ERROR("Bad Spectrum file: " << fileName);
    delete vfile;
    vfile= 0;
    return spp;
  }

  int n= numDirec+1;
  int m= (numDirec+1)*numFreq;
  float *sdata= new float[m];
  float *xdata= new float[m];
  float *ydata= new float[m];

  float radstep= fabsf(directions[0]-directions[1])*DEG_TO_RAD;

  // extend direction size, for graphics
  for (int j=0; j<numFreq; j++) {
    for (int i=0; i<numDirec; i++)
      sdata[j*n+i]= spec[j*numDirec+i];
    sdata[j*n+numDirec]= spec[j*numDirec+0];
  }

  float smax=0.0;
  m= numDirec*numFreq;
  for (int i=0; i<m; i++)
    if (smax<spec[i]) smax= spec[i];
  spp->specMax= smax;

  // total for all directions
  spp->eTotal.clear();
  float etotmax=0.0;
  for (int j=0; j<numFreq; j++) {
    float etot= 0.0;
    for (int i=0; i<numDirec; i++)
      etot += spec[j*numDirec+i]*radstep;
    spp->eTotal.push_back(etot);
    if(etotmax<etot) etotmax= etot;
  }

  // ok until more than one spectrum shown on one diagram !!!
  SpectrumPlot::eTotalMax= etotmax;

  float *cosdir= new float[n];
  float *sindir= new float[n];
  float rotation= spp->northRotation;
  float angle;
  for (int i=0; i<numDirec; i++) {
    angle= (90.-directions[i]-rotation)*DEG_TO_RAD;
    cosdir[i]= cos(angle);
    sindir[i]= sin(angle);
  }
  cosdir[n-1]= cosdir[0];
  sindir[n-1]= sindir[0];

  for (int j=0; j<numFreq; j++) {
    for (int i=0; i<n; i++) {
      xdata[j*n+i]= frequences[j] * cosdir[i];
      ydata[j*n+i]= frequences[j] * sindir[i];
    }
  }

  delete[] spp->sdata;
  delete[] spp->xdata;
  delete[] spp->ydata;

  spp->sdata= sdata;
  spp->xdata= xdata;
  spp->ydata= ydata;

  delete[] spec;
  delete[] cosdir;
  delete[] sindir;

  return spp;
}
