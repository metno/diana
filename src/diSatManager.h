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
#ifndef diSatManager_h
#define diSatManager_h

#include <diPlot.h>
#include <diSat.h>
#include <diSatPlot.h>
#include <diCommonTypes.h>
#include <diTimeFilter.h>
#include <map>
#include <set>
#include <sys/types.h>
#include <sys/stat.h>



//#ifndef HDF5_INC
//#define HDF5_INC
//#endif

//#ifndef DEBUGPRINT
//#define DEBUGPRINT
//#endif


//#ifdef HDF5_INC

//#endif

/**

  \brief Managing satellite and radar images

  - parse setup
  - decode plot info strings
  - managing file/time info
  - read data
  - making rgb images, mosaic of images

*/
class SatManager {
private:

  struct subProdInfo {
    vector<miutil::miString> pattern;
    vector<bool> archive;
    vector<TimeFilter> filter;
    miutil::miString formattype; //holds mitiff or hdf5
    miutil::miString metadata;
    miutil::miString channelinfo;
    miutil::miString paletteinfo;
    int hdf5type;
    vector<SatFileInfo> file;
    vector<miutil::miString> channel;
    vector<Colour> colours;
    // HK variable to tell whether this list has been updated since
    //last time we clicked "refresh"
    bool updated;
    //time of last update in seconds since Jan 1, 1970. 00:00:00
    unsigned long updateTime;
    //archive files read, files must be removed if useArchive=false
    bool archiveFiles;
    bool mosaic; //ok to make mosaic
  };


  map<miutil::miString, map<miutil::miString,subProdInfo> > Prod;
  SatDialogInfo Dialog;
  map<miutil::miString,miutil::miString> channelmap; // ex: name:1+2+3 -> channelmap[name]=1+2+3

  // needed for getFiles error return
  const vector<SatFileInfo> emptyfile;
  bool useArchive; //read archive files too.

/************************************************************************/

//current sat image and plot
  Sat *satdata;
  SatPlot * sp;

  int updateFreq;   //Max time between filelist updates in seconds
  miutil::miTime ztime;     //zero time = 00:00:00 UTC Jan 1 1970
  int timeDiff;

  // Copy members
  void memberCopy(const SatManager& rhs);
  void getMosaicfiles();
  void addMosaicfiles();
  vector<SatFileInfo> mosaicfiles;

  void cutImage(unsigned char*, float, int&, int&);
  void setRGB();
  void setPalette(SatFileInfo &);
  void listFiles(subProdInfo &subp);
  bool readHeader(SatFileInfo &, vector<miutil::miString> &);

  bool _isafile(const miutil::miString name);
  unsigned long _modtime(const miutil::miString fname);
  void _filestat(const miutil::miString fname, struct stat& filestat);
  bool parseChannels(SatFileInfo &info);
  bool readSatFile();

  void init_rgbindex(Sat& sd);
  void init_rgbindex_Meteosat(Sat& sd);

  //cut index from first picture,can be reused in other pictures
  struct ColourStretchInfo{
    miutil::miString channels;
    int index1[3];
    int index2[3];
  };
  ColourStretchInfo colourStretchInfo;

public:
  // Constructors
  SatManager();

  bool init(vector<SatPlot*>&, const vector<miutil::miString>&);
  bool setData(SatPlot *);
  vector<miutil::miTime> getSatTimes(const vector<miutil::miString>&);

  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miutil::miTime>& progTimes,
			   miutil::miTime& constTime,
			   int& timediff,
			   const miutil::miString& pinfo);

  const vector<SatFileInfo> & getFiles(const miutil::miString &,
				       const miutil::miString &,
				       bool =false);
  const vector<Colour> & getColours(const miutil::miString &,
				       const miutil::miString &);

  const vector<miutil::miString>& getChannels(const miutil::miString &satellite,
				      const miutil::miString & file,
				      int index=-1);
  bool isMosaic(const miutil::miString &satellite, const miutil::miString & file);

  int getFileName(miutil::miString &);
  int getFileName(const miutil::miTime&);

  SatDialogInfo initDialog(void){ return Dialog;}
  bool parseSetup(SetupParser &);

  //  Sat * findSatdata(const miutil::miString & filename);//search vsatdata
  void updateFiles();
  void setSatAuto(bool,const miutil::miString &, const miutil::miString &);

  void archiveMode( bool on ){useArchive=on; updateFiles();}

  vector <miutil::miString> vUffdaClass;
  vector <miutil::miString> vUffdaClassTip;
  miutil::miString uffdaMailAddress;
  bool uffdaEnabled;

  bool fileListChanged;

  // Radar echo mesure
  map<float,float> radarecho;

};

#endif




