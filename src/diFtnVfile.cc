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
/*
   DESCRIPTION:    Reading fortran Vprof and Vcross files (bput/bget)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diFtnVfile.h"

#include <cmath>
#include <cstdio>
#include <sstream>

#define MILOGGER_CATEGORY "diana.FtnVfile"
#include <miLogger/miLogging.h>

FtnVfile::FtnVfile(const std::string& filename, int bufferlength) :
  bufferLength(bufferlength), fileName(filename),
  firstRead(true), swapFile(false), index(bufferlength), pfile(0)
{
  METLIBS_LOG_SCOPE();
  buffer= new short int[bufferlength];
}


FtnVfile::~FtnVfile()
{
  METLIBS_LOG_SCOPE();
  if (pfile)
    fclose(pfile);
  delete[] buffer;
}


void FtnVfile::init()
{
  METLIBS_LOG_SCOPE();

  pfile = fopen(fileName.c_str(), "rb");

  if (!pfile) {
    METLIBS_LOG_WARN("FtnVfile::init error open " << fileName);
    throw VfileError();
  }

  try {
    readBuffer();
  }
  catch (...) {
    METLIBS_LOG_WARN("FtnVfile::init error reading " << fileName);
    throw;
  }
}


void FtnVfile::readBuffer()
{
  METLIBS_LOG_SCOPE();

  if (int (fread(buffer, 2, bufferLength, pfile)) != bufferLength) {
    METLIBS_LOG_WARN("FtnVfile::readBuffer error reading " << fileName);
    throw VfileError();
  }

  if (firstRead) {
    // DNMI Vertical profile data:     first word = 201
    // DNMI Vertical crossection data: first word = 121
    // DNMI Wave spectrum data:        first word = 251
    swapFile= false;
    if (buffer[0]!=201 && buffer[0]!=121 && buffer[0]!=251) {
      short int word= buffer[0];
      byteSwap(&word, 1);
      if (word==201 || word==121 || word==251) swapFile= true;
    }
    firstRead= false;
  }

  if (swapFile) byteSwap(buffer, bufferLength);

  index= 0;
}


bool FtnVfile::setFilePosition(int record, int word)
{
  METLIBS_LOG_SCOPE();

  long offset = (record-1) * bufferLength * 2;
  if(offset<0){
    METLIBS_LOG_ERROR("FtnVfile::setFilePosition ERROR " << fileName);
    return false;
  }

  if (fseek(pfile, offset, SEEK_SET) != 0) {
    METLIBS_LOG_ERROR("FtnVfile::setFilePosition ERROR " << fileName);
    fclose(pfile);
    throw VfileError();
  }

  try {
    readBuffer();
  }
  catch (...) {
    METLIBS_LOG_ERROR("FtnVfile::setFilePosition error reading " << fileName);
    throw;
  }

  index= word - 1;
  return true;
}


int FtnVfile::getInt()
{
  METLIBS_LOG_SCOPE();

  if (index>=bufferLength)
    readBuffer();

  return buffer[index++];
}


int* FtnVfile::getInt(int length)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  int* idata= new int[length];

  int n, j, i=0;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    for (j=0; j<n; ++j)
      idata[i++]= buffer[index++];
  }
  return idata;
}


int* FtnVfile::getIntDuo(int length)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  int* idata= new int[length];

  int n, j, i=0, ilo, ihi;

  if (index % 2) index++;
  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<(bufferLength-index)/2) ? length-i : (bufferLength-index)/2;
    for (j=0; j<n; ++j) {
      ilo= buffer[index++]; if (ilo<0) ilo += 65536;
      ihi= buffer[index++];
      idata[i++]= (ihi<<16) | ilo;
    }
  }
  return idata;
}


std::vector<int> FtnVfile::getIntVector(int length)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  std::vector<int> idata(length);

  int n, j, i=0;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    for (j=0; j<n; ++j)
      idata[i++]= buffer[index++];
  }
  return idata;
}


short int* FtnVfile::getShortInt(int length)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  short int* idata= new short int[length];

  int n, j, i=0;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    for (j=0; j<n; ++j)
      idata[i++]= buffer[index++];
  }
  return idata;
}


float FtnVfile::getFloat(int iscale, int iundef)
{
  METLIBS_LOG_SCOPE();

  if (index>=bufferLength) readBuffer();

  float fdata;
  float scale= powf(10.,iscale);

  if (iundef<1 || buffer[index]!=intUndef) {
    fdata= float(buffer[index++]) * scale;
  } else {
    fdata= floatUndef;
    index++;
  }
  return fdata;
}


float* FtnVfile::getFloat(int length, int iscale, int iundef)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  float* fdata= new float[length];

  int n, j, i=0;
  float scale= powf(10.,iscale);

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    if (iundef<1) {
      for (j=0; j<n; ++j)
        fdata[i++]= float(buffer[index++]) * scale;
    } else {
      for (j=0; j<n; ++j) {
        if (buffer[index]!=intUndef) {
          fdata[i++]= float(buffer[index++]) * scale;
        } else {
          fdata[i++]= floatUndef;
          index++;
        }
      }
    }
  }
  return fdata;
}


void FtnVfile::getFloat(float *fdata, int length, int iscale, int iundef)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  int n, j, i=0;
  float scale= powf(10.,iscale);

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    if (iundef<1) {
      for (j=0; j<n; ++j)
        fdata[i++]= float(buffer[index++]) * scale;
    } else {
      for (j=0; j<n; ++j) {
        if (buffer[index]!=intUndef) {
          fdata[i++]= float(buffer[index++]) * scale;
        } else {
          fdata[i++]= floatUndef;
          index++;
        }
      }
    }
  }
}


std::vector<float> FtnVfile::getFloatVector(int length, int iscale, int iundef)
{
  METLIBS_LOG_SCOPE();
  METLIBS_LOG_DEBUG(LOGVAL(length));

  std::vector<float> fdata(length);

  int n, j, i=0;
  float scale= powf(10.,iscale);

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    if (iundef<1) {
      for (j=0; j<n; ++j)
        fdata[i++]= float(buffer[index++]) * scale;
    } else {
      for (j=0; j<n; ++j) {
        if (buffer[index]!=intUndef) {
          fdata[i++]= float(buffer[index++]) * scale;
        } else {
          fdata[i++]= floatUndef;
          index++;
        }
      }
    }
  }
  return fdata;
}


std::string FtnVfile::getString(int length)
{
  METLIBS_LOG_SCOPE(LOGVAL(length));

  int lengthStr= length;

  length= (length+1)/2; // always stored N*2 characters

  int c1, c2, n, j, i=0;

  std::ostringstream ostr;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    for (j=0; j<n; ++j) {
      c1= buffer[index]>>8;
      c2= buffer[index] & 255;
      ostr << char(c1) << char(c2);
      index++;
    }
    i+=n;
  }
  std::string str= ostr.str().substr(0,lengthStr);

  // files handled here often contains some strange norwegian characters
  n= str.length();
  for (i=0; i<n; i++) {
    if      (str[i]=='#') str[i]='\xC6'; // was 'Æ'
    else if (str[i]=='@') str[i]='\xD8'; // was 'Ø'
    else if (str[i]=='$') str[i]='\xC5'; // was 'Å'
  }

  return str;
}


void FtnVfile::skipData(int length)
{
  METLIBS_LOG_SCOPE(LOGVAL(length));

  int n, i=0;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    i+=n;
    index+=n;
  }
}
