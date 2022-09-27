/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2022 met.no

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

#include "diSatManagerBase.h"

#include "diSatTypes.h"

#include <puTools/TimeFilter.h>

#include <map>

/**
  \brief Managing satellite and radar images

  - parse setup
  - decode plot info strings
  - managing file/time info
  - read data
  - making rgb images, mosaic of images
*/
class SatManager : public SatManagerBase
{

public:
  struct subProdInfo {
    std::vector<std::string> pattern;
    std::vector<miutil::TimeFilter> filter;
    std::string formattype; //holds mitiff or hdf5
    std::string metadata;
    Projection projection;
    std::string channelinfo;
    std::string paletteinfo;
    int hdf5type;
    std::vector<SatFileInfo> file;
    std::vector<std::string> channel;
    std::vector<Colour> colours;
    bool mosaic; //ok to make mosaic
    bool archive;
  };

  typedef std::map<std::string, subProdInfo> SubProd_t;
  typedef std::map<std::string, SubProd_t> Prod_t;

private:
  Prod_t Prod;
  SatImage_v Dialog;

  typedef std::map<std::string,std::string> channelmap_t;
  channelmap_t channelmap; // ex: name:1+2+3 -> channelmap[name]=1+2+3

  bool useArchive; //read archive files too.

  std::vector<SatFileInfo> getMosaicfiles(Sat* satdata, const miutil::miTime& t);
  void addMosaicfiles(Sat* satdata, const std::vector<SatFileInfo>& mosaicfiles);

  void setRGB(Sat* satdata);
  void calcRGBstrech(unsigned char *image, const int& size, const float& cut);
  void setPalette(Sat* satdata);
  void listFiles(subProdInfo &subp);
  void readHeader(SatFileInfo&, const std::vector<std::string>&);

  bool parseChannels(Sat* satdata, const SatFileInfo& info);
  bool readSatFile(Sat* satdata, const miutil::miTime& t);
  bool doReadSatFile(const std::string& filename, Sat* satdata);

  void init_rgbindex(Sat& sd);
  void init_rgbindex_Meteosat(Sat& sd);

  //cut index from first picture,can be reused in other pictures
  int colourStretchInfo[6];

  int getFileName(subProdInfo* subp, const std::string&);
  int getFileName(subProdInfo* subp, const miutil::miTime&, int maxDiff);

  /*! Find product, \returns nullptr if not found. */
  subProdInfo* findProduct(const SatImageAndSubType& sist);

public:
  SatManager();

  SatPlotBase* createPlot(SatPlotCommand_cp cmd) override;

  //! Try to reuse plot for showing cmd
  //! return true iff the plot has been reused
  bool reusePlot(SatPlotBase* plot, SatPlotCommand_cp cmd, bool first) override;

  plottimes_t getSatTimes(const SatImageAndSubType& sist) override;

  SatFile_v getFiles(const SatImageAndSubType& sist, bool update) override;

  //! Returns colour palette for this subproduct.
  const std::vector<Colour>& getColours(const SatImageAndSubType& sist) override;

  const std::vector<std::string>& getChannels(const SatImageAndSubType& sist, int index = -1) override;

  const SatImage_v& initDialog() override { return Dialog; }

  bool parseSetup() override;

  void archiveMode(bool on) override { useArchive = on; }

  void setData(Sat* satdata, const miutil::miTime& satptime);
};

#endif
