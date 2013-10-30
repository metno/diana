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

#include <sys/types.h>

#include <map>
#include <set>

#include <puCtools/stat.h>

#include <diPlot.h>
#include <diSat.h>
#include <diSatPlot.h>
#include <diCommonTypes.h>
#include <diField/TimeFilter.h>

#include <miLogger/logger.h>
#include <miLogger/LogHandler.h>

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

public:
    struct subProdInfo {
    vector<std::string> pattern;
    vector<bool> archive;
    vector<TimeFilter> filter;
    std::string formattype; //holds mitiff or hdf5
    std::string metadata;
    std::string channelinfo;
    std::string paletteinfo;
    int hdf5type;
    vector<SatFileInfo> file;
    vector<std::string> channel;
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

private:
  map<std::string, map<std::string,subProdInfo> > Prod;
  SatDialogInfo Dialog;
  map<std::string,std::string> channelmap; // ex: name:1+2+3 -> channelmap[name]=1+2+3

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
  bool readHeader(SatFileInfo &, vector<std::string> &);

  bool _isafile(const std::string name);
  unsigned long _modtime(const std::string fname);
  int _filestat(const std::string fname, pu_struct_stat& filestat);
  bool parseChannels(SatFileInfo &info);
  bool readSatFile();

  void init_rgbindex(Sat& sd);
  void init_rgbindex_Meteosat(Sat& sd);

  //cut index from first picture,can be reused in other pictures
  struct ColourStretchInfo{
    std::string channels;
    int index1[3];
    int index2[3];
  };
  ColourStretchInfo colourStretchInfo;

public:
  // Constructors
  SatManager();

  bool init(vector<SatPlot*>&, const std::vector<std::string>&);
  bool setData(SatPlot *);
  vector<miutil::miTime> getSatTimes(const std::vector<std::string>&, bool updateFileList=false, bool openFiles=false);

  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miutil::miTime>& progTimes,
			   miutil::miTime& constTime,
			   int& timediff,
			   const std::string& pinfo);

  const vector<SatFileInfo> & getFiles(const std::string &,
				       const std::string &,
				       bool =false);
  const vector<Colour> & getColours(const std::string &,
				       const std::string &);

  const vector<std::string>& getChannels(const std::string &satellite,
				      const std::string & file,
				      int index=-1);
  bool isMosaic(const std::string &satellite, const std::string & file);

  void cutImageRGBA(unsigned char *image, float cut, int *index);
  int getFileName(std::string &);
  int getFileName(const miutil::miTime&);

  SatDialogInfo initDialog(void){ return Dialog;}
  bool parseSetup();

  //  Sat * findSatdata(const std::string & filename);//search vsatdata
  void updateFiles();
  void setSatAuto(bool,const std::string &, const std::string &);

  void archiveMode( bool on ){useArchive=on; updateFiles();}

  vector <std::string> vUffdaClass;
  vector <std::string> vUffdaClassTip;
  std::string uffdaMailAddress;
  bool uffdaEnabled;

  bool fileListChanged;

  // Radar echo mesure
  map<float,float> radarecho;

  map<std::string, map<std::string,subProdInfo> > getProductsInfo() const;
};

#endif
