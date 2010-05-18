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
/*
   DESCRIPTION:    Reading fortran Vprof and Vcross files (bput/bget)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include <math.h>
#include <diFtnVfile.h>

using namespace::miutil;

// Default constructor
FtnVfile::FtnVfile(miString filename, int bufferlength) :
  bufferLength(bufferlength), fileName(filename),
  firstRead(true), swapFile(false), index(bufferlength), pfile(0)

  {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::Default Constructor" << endl;
#endif
  buffer= new short int[bufferlength];
  }


// Destructor
FtnVfile::~FtnVfile() {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::Destructor" << endl;
#endif
  if (pfile) fclose(pfile);
  delete[] buffer;
}


void FtnVfile::init(){
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::init" << endl;
#endif

  pfile = fopen(fileName.c_str(), "rb");

  if (!pfile) {
    cerr << "FtnVfile::init error open " << fileName << endl;
    throw VfileError();
  }

  try {
    readBuffer();
  }
  catch (...) {
    cerr << "FtnVfile::init error reading " << fileName << endl;
    throw;
  }
}


void FtnVfile::readBuffer() {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::readBuffer" << endl;
#endif

  if (int (fread(buffer, 2, bufferLength, pfile)) != bufferLength) {
    cerr << "FtnVfile::readBuffer error reading " << fileName << endl;
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


bool FtnVfile::setFilePosition(int record, int word) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::setFilePosition" << endl;
#endif
  long offset = (record-1) * bufferLength * 2;

  if(offset<0){
    cerr << "FtnVfile::setFilePosition ERROR " << fileName << endl;
    return false;
  }

  if (fseek(pfile, offset, SEEK_SET) != 0) {
    cerr << "FtnVfile::setFilePosition ERROR " << fileName << endl;
    fclose(pfile);
    throw VfileError();
  }

  try {
    readBuffer();
  }
  catch (...) {
    cerr << "FtnVfile::setFilePosition error reading " << fileName << endl;
    throw;
  }

  index= word - 1;
  return true;
}


int FtnVfile::getInt() {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getInt" << endl;
#endif

  if (index>=bufferLength) readBuffer();

  int idata= buffer[index++];

  return idata;
}


int* FtnVfile::getInt(int length) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getInt  length= " << length << endl;
#endif

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


int* FtnVfile::getIntDuo(int length) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getIntDuo  length= " << length << endl;
#endif

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


vector<int> FtnVfile::getIntVector(int length) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getIntVector length= " << length << endl;
#endif

  vector<int> idata(length);

  int n, j, i=0;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    for (j=0; j<n; ++j)
      idata[i++]= buffer[index++];
  }
  return idata;
}


short int* FtnVfile::getShortInt(int length) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getShortInt  length= " << length << endl;
#endif

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


float FtnVfile::getFloat(int iscale, int iundef) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getFloat" << endl;
#endif

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


float* FtnVfile::getFloat(int length, int iscale, int iundef) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getFloat length= " << length << endl;
#endif

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


void FtnVfile::getFloat(float *fdata, int length,
    int iscale, int iundef) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getFloat length= " << length << endl;
#endif

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


vector<float> FtnVfile::getFloatVector(int length, int iscale, int iundef) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getFloatVector length= " << length << endl;
#endif

  vector<float> fdata(length);

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


miString FtnVfile::getString(int length) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::getString  length= " << length << endl;
#endif

  int lengthStr= length;

  length= (length+1)/2; // always stored N*2 characters

  int c1, c2, n, j, i=0;

  ostringstream ostr;

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
  miString str= ostr.str().substr(0,lengthStr);

  // files handled here often contains some strange norwegian characters
  n= str.length();
  for (i=0; i<n; i++) {
    if      (str[i]=='#') str[i]='Æ';
    else if (str[i]=='@') str[i]='Ø';
    else if (str[i]=='$') str[i]='Å';
  }

  //###############################################################
  //cerr<<"FtnVfile::getString >>> " << str << " <<<" << endl;
  //###############################################################
  return str;
}


void FtnVfile::skipData(int length) {
#ifdef DEBUGPRINT
  cerr << "++ FtnVfile::skipData  length= " << length << endl;
#endif

  int n, i=0;

  while (i<length) {
    if (index>=bufferLength) readBuffer();
    n= (length-i<bufferLength-index) ? length-i : bufferLength-index;
    i+=n;
    index+=n;
  }
}

