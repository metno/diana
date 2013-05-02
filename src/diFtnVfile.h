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
#ifndef diFtnVfile_h
#define diFtnVfile_h

#include <string>
#include <vector>

const float   floatUndef= 1.e+35;
const short int intUndef= -32767;

/**
   \brief Exception handling during FtnVfile reading

*/
class VfileError {};

/**
   \brief Reading some proprietary met.no files

   Read (fortran made) files containg preprocessed prognostic
   Vertical Profile, Vertical Crossection and Wave Spectrum data.
   Misc position, time and contents info in file header, incl. data pointers.
   Byteswapping performed automatically if needed.
   No fortran code used here.
*/
class FtnVfile {

public:

  FtnVfile(const std::string& filename, int bufferlength);
  ~FtnVfile();
  void init();
  bool setFilePosition(int record, int word);
  int        getInt     ();
  int*       getInt     (int length);
  int*       getIntDuo  (int length);
  std::vector<int> getIntVector(int length);
  short int* getShortInt(int length);
  float      getFloat   (int iscale, int iundef);
  float*     getFloat   (int length, int iscale, int iundef);
  void       getFloat   (float *fdata, int length,
			 int iscale, int iundef);
  std::vector<float> getFloatVector(int length, int iscale, int iundef);
  std::string   getString  (int length);
  void       skipData   (int length);

private:
  void readBuffer();

  int bufferLength;
  short int *buffer;

  std::string fileName;
  bool firstRead;
  bool swapFile;
  int  index;
  FILE *pfile;

  inline static void byteSwap(short int *data, int ndata) {
    short int iswap;
    for (int i=0; i<ndata; ++i) {
      iswap= data[i];
      data[i]= (iswap << 8) | ((iswap >> 8) & 255);
    }
  }
};

#endif
