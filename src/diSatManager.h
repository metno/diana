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

#include <diAnnotationPlot.h>
#include <diSat.h>
#include <diSatPlot.h>
#include <diCommonTypes.h>

#include <puCtools/stat.h>
#include <diField/TimeFilter.h>

#include <map>
#include <set>
#include <vector>

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
    std::vector<std::string> pattern;
    std::vector<bool> archive;
    std::vector<TimeFilter> filter;
    std::string formattype; //holds mitiff or hdf5
    std::string metadata;
    std::string proj4string;
    std::string channelinfo;
    std::string paletteinfo;
    int hdf5type;
    std::vector<SatFileInfo> file;
    std::vector<std::string> channel;
    std::vector<Colour> colours;
    // HK variable to tell whether this list has been updated since
    //last time we clicked "refresh"
    bool updated;
    //time of last update in seconds since Jan 1, 1970. 00:00:00
    unsigned long updateTime;
    //archive files read, files must be removed if useArchive=false
    bool archiveFiles;
    bool mosaic; //ok to make mosaic
  };

  typedef std::map<std::string, subProdInfo> SubProd_t;
  typedef std::map<std::string, SubProd_t> Prod_t;

private:
  Prod_t Prod;
  SatDialogInfo Dialog;

  typedef std::map<std::string,std::string> channelmap_t;
  channelmap_t channelmap; // ex: name:1+2+3 -> channelmap[name]=1+2+3

  bool useArchive; //read archive files too.

/************************************************************************/

  int updateFreq;   //Max time between filelist updates in seconds
  miutil::miTime ztime;     //zero time = 00:00:00 UTC Jan 1 1970

  void getMosaicfiles(Sat* satdata);
  void addMosaicfiles(Sat* satdata);
  std::vector<SatFileInfo> mosaicfiles;

  void cutImage(Sat* satdata, unsigned char*, float, int&, int&);
  void setRGB(Sat* satdata);
  void setPalette(Sat* satdata, SatFileInfo &);
  void listFiles(subProdInfo &subp);
  bool readHeader(SatFileInfo &, std::vector<std::string> &);

  bool _isafile(const std::string name);
  unsigned long _modtime(const std::string fname);
  int _filestat(const std::string fname, pu_struct_stat& filestat);
  bool parseChannels(Sat* satdata, SatFileInfo &info);
  bool readSatFile(Sat* satdata);

  bool init(const std::vector<std::string>&);
  void init_rgbindex(Sat& sd);
  void init_rgbindex_Meteosat(Sat& sd);

  //cut index from first picture,can be reused in other pictures
  struct ColourStretchInfo{
    std::string channels;
    int index1[3];
    int index2[3];
  };
  ColourStretchInfo colourStretchInfo;

  typedef std::vector<SatPlot*> SatPlot_xv;
  SatPlot_xv vsp;   // vector of satellite plots

  bool fileListChanged;

  bool setData(SatPlot *satp);
  void cutImageRGBA(Sat* satdata, unsigned char *image, float cut, int *index);
  int getFileName(Sat* satdata, std::string &);
  int getFileName(Sat* satdata, const miutil::miTime&);

public:
  SatManager();

  /// handles images plot info strings
  void prepareSat(const std::vector<std::string>& inp);

  void addPlotElements(std::vector<PlotElement>& pel);
  void enablePlotElement(const PlotElement& pe);
  void addSatAnnotations(std::vector<AnnotationPlot::Annotation>& annotations);
  void getSatAnnotations(std::vector<std::string>& anno);
  void plot(Plot::PlotOrder porder);
  void clear();
  bool getGridResolution(float& rx, float& ry);

  bool setData();
  bool getSatArea(Area& a) const
    { if (vsp.empty()) return false; a = vsp.front()->getSatArea(); return true; }

  bool isFileListChanged() const
    { return fileListChanged; }
  void setFileListChanged(bool flc)
    { fileListChanged = flc; }

  std::vector<miutil::miTime> getSatTimes(bool updateFileList=false, bool openFiles=false);

  /// get name++ of current channels (with calibration)
  std::vector<std::string> getCalibChannels();
  ///show pixel values in status bar
  std::vector<SatValues> showValues(float x, float y);
  ///get satellite name from all SatPlots
  std::vector <std::string> getSatnames();
  ///satellite follows main plot time
  void setSatAuto(bool, const std::string&, const std::string&);

  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(std::vector<miutil::miTime>& progTimes,
			   int& timediff,
			   const std::string& pinfo);

  const std::vector<SatFileInfo> & getFiles(const std::string &,
				       const std::string &,
				       bool =false);
  const std::vector<Colour> & getColours(const std::string &,
				       const std::string &);

  const std::vector<std::string>& getChannels(const std::string &satellite,
				      const std::string & file,
				      int index=-1);
  bool isMosaic(const std::string &satellite, const std::string & file);

  SatDialogInfo initDialog()
    { return Dialog; }
  bool parseSetup();

  //  Sat * findSatdata(const std::string & filename);//search vsatdata
  void updateFiles();

  void archiveMode(bool on)
    { useArchive = on; updateFiles(); }

  std::vector <std::string> vUffdaClass;
  std::vector <std::string> vUffdaClassTip;
  std::string uffdaMailAddress;
  bool uffdaEnabled;

  const Prod_t& getProductsInfo() const;

  const std::vector<SatPlot*>& getSatellitePlots() const
    { return vsp; }
};

#endif
