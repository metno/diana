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
#include <sys/types.h>
#include <sys/stat.h>

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
    vector<miString> pattern;
    vector<bool> archive;
    vector<TimeFilter> filter;
    vector<SatFileInfo> file;
    vector<miString> channel;
    vector<Colour> colours;
    // HK variable to tell whether this list has been updated since
    //last time we clicked "refresh"
    bool updated;
    //time of last update in seconds since Jan 1, 1970. 00:00:00
    int updateTime;
    //archive files read, files must be removed if useArchive=false
    bool archiveFiles;
    bool mosaic; //ok to make mosaic
  };

  map<miString, map<miString,subProdInfo> > Prod;
  SatDialogInfo Dialog;
  map<miString,miString> channelmap; // ex: name:1+2+3 -> channelmap[name]=1+2+3

  // needed for getFiles error return
  const vector<SatFileInfo> emptyfile;
  bool useArchive; //read archive files too.

/************************************************************************/

//current sat image and plot
  Sat *satdata;
  SatPlot * sp; 
  vector <Sat *> vsatdata; 
  
  int updateFreq;   //Max time between filelist updates in seconds
  miTime ztime;     //zero time = 00:00:00 UTC Jan 1 1970
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
  bool readHeader(SatFileInfo &, vector<miString> &);

  bool _isafile(const miString name);
  unsigned long _modtime(const miString fname);
  void _filestat(const miString fname, struct stat& filestat);
  bool parseChannels(SatFileInfo &info);
  bool readSatFile();

  void init_rgbindex(Sat& sd);
  void init_rgbindex_Meteosat(Sat& sd);

  //cut index from first picture,can be reused in other pictures
  struct ColourStretchInfo{
    miString channels;
    int index1[3];
    int index2[3];
  };
  ColourStretchInfo colourStretchInfo;

public:
  // Constructors
  SatManager();

  bool init(vector<SatPlot*>&, const vector<miString>&);
  bool setData(SatPlot *);
  vector<miTime> getSatTimes(const vector<miString>&);

  ///returns times where time - sat/obs-time < mindiff
  vector<miTime> timeIntersection(const vector<miString>& pinfos,
				  const vector<miTime>& times);

  const vector<SatFileInfo> & getFiles(const miString &, 
				       const miString &, 
				       bool =false);
  const vector<Colour> & getColours(const miString &, 
				       const miString &);

  const vector<miString>& getChannels(const miString &satellite, 
				      const miString & file,
				      int index=-1);
  bool isMosaic(const miString &satellite, const miString & file);

  int getFileName(miString &);
  int getFileName(const miTime&);

  SatDialogInfo initDialog(void){ return Dialog;}
  bool parseSetup(SetupParser &);

  //  Sat * findSatdata(const miString & filename);//search vsatdata
  void updateFiles();
  void setSatAuto(bool,const miString &, const miString &); 

  void archiveMode( bool on ){useArchive=on; updateFiles();}

  vector <miString> vUffdaClass;
  vector <miString> vUffdaClassTip;
  miString uffdaMailAddress;
  bool uffdaEnabled;

  bool fileListChanged;

};

#endif




