/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

#include "diSatManager.h"

#include "diSatPlot.h"
#include "diSatPlotCommand.h"
#include "diStaticPlot.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/time_util.h"
#include "util/was_enabled.h"

#include <puTools/miStringFunctions.h>
#include <puCtools/stat.h>

#include <diMItiff.h>
#ifdef HDF5FILE
#include <diHDF5.h>
#endif
#ifdef GEOTIFF
#include <diGEOtiff.h>
#endif

#include <fstream>
#include <set>

#define MILOGGER_CATEGORY "diana.SatManager"
#include <miLogger/miLogging.h>

using namespace miutil;

namespace {

int _filestat(const std::string fname, pu_struct_stat& filestat)
{
  return pu_stat(fname.c_str(), &filestat);
}

bool _isafile(const std::string& name)
{
  pu_struct_stat filestat;
  // first check if fname is a proper file
  int result = _filestat(name, filestat);
  if (!result) {
    return true;
  }
  else
  {
    //stat() is not able to get the file attributes,
    //so the file obviously does not exist or
    //more capabilities is required
    return false;
  }
}
} // namespace

SatManager::SatManager()
    : useArchive(false)
{
}

bool SatManager::setData(Sat* satdata, const miutil::miTime& satptime)
{
  //  PURPOSE:s   Read data from file, and init. SatPlot
  METLIBS_LOG_SCOPE();

  //not yet approved for plotting
  satdata->approved= false;

  int index = -1;
  if (!satdata->filename.empty()) { //requested a specific filename
    index=getFileName(satdata, satdata->filename);
  } else if (!satdata->autoFile) { // keep the same filename
    index = getFileName(satdata, satdata->actualfile);
  } else {
    // find filename from time
    index = getFileName(satdata, satptime);
  }
  if (index < 0)
    return false;

  //Read header if not opened

  subProdInfo* spi = findProduct(satdata->satellite, satdata->filetype);
  if (!spi)
    return false;

  SatFileInfo& fInfo = spi->file[index];
  if (!fInfo.opened) {
    readHeader(fInfo, spi->channel);
    fInfo.opened = true;
  }

  //read new file if :
  // 1) satdata contains no images
  // 2) different channels requested
  // 3) requested file different from the actual file
  bool readfresh= (satdata->noimages() || satdata->channelschanged
      || fInfo.name != satdata->actualfile);

  // remember filename for later checks
  satdata->actualfile= fInfo.name;
  satdata->time = fInfo.time;
  satdata->formatType = fInfo.formattype;
  satdata->metadata = fInfo.metadata;
  satdata->proj_string = fInfo.proj4string;
  satdata->channelInfo = fInfo.channelinfo;
  satdata->paletteinfo = fInfo.paletteinfo;
  satdata->hdf5type = fInfo.hdf5type;

  if (readfresh) { // nothing to reuse..
    //find out which channels to read (satdata->index), total no
    if ( !parseChannels(satdata, fInfo) ) {
      METLIBS_LOG_ERROR("Failed parseChannels");
      return false;
    }
    satdata->cleanup();
    if (!readSatFile(satdata, satptime)) {
      METLIBS_LOG_ERROR("Failed readSatFile");
      return false;
    }
    satdata->setArea();
    satdata->setCalibration();
    satdata->setAnnotation();
    // ADC
    satdata->setPlotName();
  } // end of data read (readfresh)

  // approved for plotting
  satdata->approved= true;

  //************* Convert to RGB ***************************************

  if (satdata->palette && satdata->formatType != "geotiff") {
    // at the moment no controllable rgb or alpha operations here
    setPalette(satdata, fInfo);
  } else {
    // GEOTIFF ALWAYS RGBA
    setRGB(satdata);
  }

  //delete filename selected in dialog
  satdata->filename.erase();

  return true;
}

/***********************************************************************/
bool SatManager::parseChannels(Sat* satdata, const SatFileInfo &fInfo)
{
  //returns in satdata->index the channels to be plotted

  //decide which channels in file (fInfo.name) to plot. from
  // the string satdata.channel
  //NOAA files contains (up to) 5 different channels per file
  //   1,2 are in the visual spectrum, 3-5 infrared
  // METEOSAT 1 channel per file
  //   either VIS_RAW(visual) or IR_CAL(infrared)
  METLIBS_LOG_SCOPE();
  std::string channels;
  { const channelmap_t::const_iterator itc = channelmap.find(satdata->plotChannels);
    if (itc != channelmap.end())
      channels = itc->second;
    else
      channels = satdata->plotChannels;
  }

  if (channels=="day_night" && !MItiff::day_night(fInfo, channels)) {
    METLIBS_LOG_ERROR("day_night");
    return false;
  }
  //name of channels selected

  satdata->vch = miutil::split(channels, "+");

  //no - is the number of channels to plot
  //index[i] 0,1,2,3,4 -> channel 1,2,3,4,5 NOAA
  int no=0;
  int n = satdata->vch.size();
  for (int i=0; (i<n && no<3); i++) {
    for (unsigned int j=0; j<fInfo.channel.size(); j++) {
      if (fInfo.channel[j] == satdata->vch[i]) {
        satdata->index[no]=j;
        no++;
        break;
      }
    }
  }

  //For METEOSAT it is possible to choose IR+V (infrared+visual)
  //then two different files must be read
  if (channels=="IR+V") {
    satdata->index[0]=0;
    no=1;
  }

  if (no==0) {
    METLIBS_LOG_ERROR("Channel(s):"<<satdata->plotChannels<<" doesn't exist");
    return false;
  }

  satdata->no = no;

  return true;
}

/***********************************************************************/

bool SatManager::readSatFile(Sat* satdata, const miutil::miTime& t)
{
  //read the file with name satdata->actualfile, channels given in
  //satdata->index. Result in satdata->rawimage
  METLIBS_LOG_SCOPE();
  if(!_isafile(satdata->actualfile)) {
    //stat() is not able to get the file attributes,
    //so the file obviously does not exist or
    //more capabilities is required
    METLIBS_LOG_ERROR("filename:" << satdata->actualfile << " does not exist or is not readable");
    return false;
  }

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG(LOGVAL(satdata->formatType));
#endif

  if (satdata->formatType == "mitiff") {
    if (!MItiff::readMItiff(satdata->actualfile, *satdata))
      return false;
  }

#ifdef HDF5FILE
  if (satdata->formatType == "hdf5") {
    if(!HDF5::readHDF5(satdata->actualfile,*satdata)) {
      return false;
    }
  }
#endif
#ifdef GEOTIFF
  if (satdata->formatType == "geotiff") {
    if(!GEOtiff::readGEOtiff(satdata->actualfile,*satdata)) {
      return false;
    }
  }
#endif
  if (!satdata->palette) {

    if (satdata->plotChannels == "IR+V")
      init_rgbindex_Meteosat(*satdata);
    else
      init_rgbindex(*satdata);
  }

  if (satdata->mosaic) {
    getMosaicfiles(satdata, t);
    addMosaicfiles(satdata);
  }

  return true;
}

/***********************************************************************/
void SatManager::setPalette(Sat* satdata, SatFileInfo& /*fInfo*/)
{
  //  PURPOSE:   uses palette to put data from image into satdata.image
  METLIBS_LOG_SCOPE(miTime::nowTime());

  int nx=satdata->area.nx;
  int ny=satdata->area.ny;
  int size =nx*ny;

  // image(RGBA)
  if (satdata->image)
    delete[] satdata->image;
  satdata->image= new unsigned char[size*4];

  // index -> RGB

  const int colmapsize=256;
  unsigned char colmap[3][colmapsize];

  //convert colormap
  for (int k=0; k<3; k++)
    for (int i=0; i<colmapsize; i++)
      colmap[k][i]= satdata->paletteInfo.cmap[k][i];

  //convert image from palette to RGBA
  for (int j=0; j<ny; j++) {
    for (int i=0; i<nx; i++) {
      int rawIndex = (int)satdata->rawimage[0][j*nx+i]; //raw image index
      int index = (i+(ny-j-1)*nx)*4;//image index
      for (int k=0; k<3; k++)
        satdata->image[index+k] = colmap[k][rawIndex];
      if (!satdata->hideColour.count(rawIndex)) {
        satdata->image[index+3] = satdata->alpha;;
      } else {
        satdata->image[index+3] = satdata->hideColour[rawIndex];
      }
    }
  }
}

void SatManager::setRGB(Sat* satdata)
{
  //   * PURPOSE:   put data from 3 images into satdata.image,
  // in geotiff files the data are already sorted as rgba values
  // RGB stretch is performed in order to improve the contrast
  METLIBS_LOG_SCOPE(LOGVAL(satdata->filetype));

  const int nx = satdata->area.nx;
  const int ny = satdata->area.ny;
  const int size = nx * ny;

  if (size == 0)
    return;

  const int colmapsize = 256;
  unsigned char colmap[3][colmapsize];

  delete[] satdata->image;
  satdata->image = nullptr;

  if (satdata->formatType == "geotiff") {
    satdata->image = satdata->rawimage[0];
    satdata->rawimage[0] = nullptr;
  } else {
    satdata->image = new unsigned char[size * 4];
    if (!satdata->rawimage[0]) {
      for (int k = 0; k < 3; k++)
        for (int i = 0; i < size; i++)
          satdata->image[i * 4 + k] = 0;
      return;
    }

    // Start in lower left corner instead of upper left.
    // satdata->image = new unsigned char[size*4];
    for (int k = 0; k < 3; k++)
      for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++)
          satdata->image[(i + (ny - j - 1) * nx) * 4 + k] = satdata->rawimage[satdata->rgbindex[k]][j * nx + i];
  }

  const bool dorgb = (satdata->noimages() || satdata->rgboperchanged);
  const bool doalpha = (dorgb || satdata->alphaoperchanged);

  if (dorgb) {
    if (satdata->cut > -1) {
      if (satdata->cut > -0.5) {
        // if cut=-0.5, reuse stretch from first image
        calcRGBstrech(satdata->image, size, satdata->cut);
      }
      // calc. colour map
      for (int k = 0; k < 3; k++) {
        const float factor = (float)(colmapsize - 1) / (colourStretchInfo[1 + k * 2] - colourStretchInfo[0 + k * 2] + 1);
        const int shift = colourStretchInfo[0 + k * 2] - 1;
        colmap[k][0] = (unsigned char)(0);
        for (int i = 1; i < colmapsize; i++) {
          int idx = (int)(float(i - shift) * factor + 0.5);
          if (idx < 1)
            idx = 1;
          if (idx > 255)
            idx = 255;
          colmap[k][i] = (unsigned char)(idx);
        }
      }
      // Put 1,2 or 3 different channels into satdata->image(RGBA).
      for (int i = 0; i < size; i++) {
        satdata->image[i * 4 + 0] = colmap[0][(unsigned int)(satdata->image[i * 4])];
        satdata->image[i * 4 + 1] = colmap[1][(unsigned int)(satdata->image[i * 4 + 1])];
        satdata->image[i * 4 + 2] = colmap[2][(unsigned int)(satdata->image[i * 4 + 2])];
      }
    } else { // set colourStretchInfo even if cut is off
      for (int k = 0; k < 3; k++) {
        colourStretchInfo[k * 2] = 0;
        colourStretchInfo[k * 2 + 1] = 255;
        satdata->commonColourStretch = true;
      }
    }
  }

  if (doalpha) {
    // set alpha values according to wanted blending
    if (satdata->alphacut > 0) {
      for (int i = 0; i < size; i++) {
        // cut on pixelvalue
        if ((int)satdata->image[i * 4] < satdata->alphacut)
          satdata->image[i * 4 + 3] = (unsigned char)0;
        else
          // set alpha value to default or the one chosen in dialog
          satdata->image[i * 4 + 3] = (unsigned char)satdata->alpha;
      }

    } else {
      for (int i = 0; i < size; i++) {
        // set alpha value to default or the one chosen in dialog
        satdata->image[i * 4 + 3] = (unsigned char)satdata->alpha;
        // remove black pixels
        if (satdata->image[i * 4] == 0 && satdata->image[i * 4 + 1] == 0 && satdata->image[i * 4 + 2] == 0)
          satdata->image[i * 4 + 3] = 0;
      }
    }
  }
}

void SatManager::calcRGBstrech(unsigned char *image, const int& size, const float& cut)
{
  //  * PURPOSE:   (1-cut)*#pixels should have a value between index[0] and index[1]

  int nindex[3][256];

  for (int k=0; k<3; k++) {
    for (int i=0; i<256; i++) {
      nindex[k][i]=0;
    }
  }

  for (int i=0; i<size; i++) {
    for (int k=0; k<3; k++) {
      nindex[k][(int)image[4*i+k]]++;
    }
  }

  for (int k=0; k<3; k++) {
    float npixel=size-nindex[k][0]; //number of pixels, drop index=0

    int ncut=(int) (npixel*cut); //number of pixels to cut   +1?


    colourStretchInfo[2*k]=1;
    colourStretchInfo[2*k+1]=255;
    int nsum=0; //number of pixels dropped
    while (nsum<ncut && colourStretchInfo[2*k]<colourStretchInfo[2*k+1]) {
      if (nindex[k][colourStretchInfo[2*k]] < nindex[k][colourStretchInfo[2*k+1]]) {
        nsum += nindex[k][colourStretchInfo[2*k]];
        colourStretchInfo[2*k]++;
      } else {
        nsum += nindex[k][colourStretchInfo[2*k+1]];
        colourStretchInfo[2*k+1]--;
      }
    }
  }
}

int SatManager::getFileName(Sat* satdata, std::string &name)
{
  METLIBS_LOG_SCOPE(name);

  if (subProdInfo* subp = findProduct(satdata->satellite, satdata->filetype)) {
    const std::vector<SatFileInfo>& ft = subp->file;
    if (ft.empty())
      listFiles(*subp);

    for (size_t i = 0; i < ft.size(); i++) {
      if (name == ft[i].name)
        return i;
    }
  }

  METLIBS_LOG_WARN("Could not find " << name << " in inventory");
  return -1;
}

int SatManager::getFileName(Sat* satdata, const miTime &time)
{
  METLIBS_LOG_SCOPE();
  if (time.undef()) {
    METLIBS_LOG_DEBUG("undefined time");
    return -1;
  }
  METLIBS_LOG_DEBUG(LOGVAL(time));

  int fileno=-1;
  if (subProdInfo* subp = findProduct(satdata->satellite, satdata->filetype)) {
    const std::vector<SatFileInfo>& ft = subp->file;
    if (ft.empty())
      listFiles(*subp);

    int diff = satdata->maxDiff + 1;
    for (size_t i = 0; i < ft.size(); i++) {
      const int d = abs(miTime::minDiff(ft[i].time, time));
      if (d < diff) {
        diff = d;
        fileno = i;
      }
    }
  }

  if (fileno < 0)
    METLIBS_LOG_WARN("Could not find data from "<<time.isoTime("T")<<" in inventory");
  return fileno;
}

void SatManager::addMosaicfiles(Sat* satdata)
{
  //  * PURPOSE:   add files to existing image
  METLIBS_LOG_SCOPE();

  unsigned char *color[3];//contains the three rgb channels of raw image
  if (!satdata->palette) {
    color[0] = satdata->rawimage[satdata->rgbindex[0]];
    color[1] = satdata->rawimage[satdata ->rgbindex[1]];
    color[2] = satdata->rawimage[satdata->rgbindex[2]];
  } else {
    color[0] = satdata->rawimage[0];
    color[1] = NULL;
    color[2] = NULL;
  }

  int n=mosaicfiles.size();
  bool filledmosaic=false;
  miTime firstFileTime=satdata->time;
  miTime lastFileTime=satdata->time;

  for (int i=0; i<n && !filledmosaic; i++) { //loop over mosaic files
    if (mosaicfiles[i].time < firstFileTime)
      firstFileTime=mosaicfiles[i].time;
    if (mosaicfiles[i].time > lastFileTime)
      lastFileTime=mosaicfiles[i].time;

    Sat sd;
    for (int j=0; j<Sat::maxch; j++)
      sd.index[j] = satdata->index[j];
    sd.no = satdata->no;

    bool ok = MItiff::readMItiff(mosaicfiles[i].name, sd);

    if (!ok)
      continue;

    if (sd.area.nx!=satdata->area.nx || sd.area.ny!=satdata->area.ny
        || sd.Ax!=satdata->Ax || sd.Ay!=satdata->Ay
        || sd.Bx!=satdata->Bx || sd.By!=satdata->By
        || sd.proj_string != satdata->proj_string)
    {
      METLIBS_LOG_WARN("File "<<mosaicfiles[i].name <<" not added to mosaic, area not ok");
      continue;
    }

    int size =sd.area.gridSize();
    unsigned char *newcolor[3];
    if (!satdata->palette) {
      // pointers to raw channels
      newcolor[0]= sd.rawimage[satdata->rgbindex[0]];
      newcolor[1]= sd.rawimage[satdata->rgbindex[1]];
      newcolor[2]= sd.rawimage[satdata->rgbindex[2]];

      filledmosaic=true;
      for (int j=0; j<size; j++) {
        if ((int)color[0][j]==0 && (int)color[1][j]==0 && (int)color[2][j]==0) {
          filledmosaic=false;
          color[0][j]=newcolor[0][j];
          color[1][j]=newcolor[1][j];
          color[2][j]=newcolor[2][j];
        }
      }
    } else {
      // pointers to raw channels
      newcolor[0]= sd.rawimage[0];
      filledmosaic=true;
      for (int j=0; j<size; j++) {
        if ((int)color[0][j]==0) {
          filledmosaic=false;
          color[0][j]=newcolor[0][j];
        }
      }
    }
    // satdata->rawimage is also updated (points to same as color)

  }//end loop over mosaic files

  //first and last filetimes to use in annotations
  satdata->firstMosaicFileTime=firstFileTime;
  satdata->lastMosaicFileTime=lastFileTime;

}

void SatManager::getMosaicfiles(Sat* satdata, const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE();
  mosaicfiles. clear();
  if (subProdInfo* subp = findProduct(satdata->satellite, satdata->filetype)) {
    const miutil::miTime& plottime = (satdata->autoFile) ? t : satdata->time;
    std::vector<int> vdiff;
    const int diff = satdata->maxDiff + 1;
    for (const SatFileInfo& si : subp->file) {
      int satdiff = abs(miTime::minDiff(si.time, satdata->time)); // diff from current sat time
      int plotdiff = abs(miTime::minDiff(si.time, plottime));     // diff from current plottime
      if (plotdiff < diff && satdiff < diff && satdiff != 0) {
        std::vector<SatFileInfo>::iterator q = mosaicfiles.begin();
        std::vector<int>::iterator i = vdiff.begin();
        while (q != mosaicfiles.end() && i != vdiff.end() && satdiff >= *i) {
          q++;
          i++;
        }
        mosaicfiles.insert(q, si);
        vdiff.insert(i, satdiff);
      }
    }
  }
}

bool SatManager::readHeader(SatFileInfo &file, std::vector<std::string> &channel)
{
  METLIBS_LOG_SCOPE(LOGVAL(file.name) << LOGVAL(file.formattype));

  if (file.formattype=="mitiff") {
    MItiff::readMItiffHeader(file);
  }

#ifdef HDF5FILE
  if (file.formattype=="hdf5" || file.formattype=="hdf5-standalone") {
    HDF5::readHDF5Header(file);
  }
#endif

#ifdef GEOTIFF
  if (file.formattype=="geotiff") {
    GEOtiff::readGEOtiffHeader(file);
  }
#endif

  //compare channels from setup and channels from file
  for (unsigned int k=0; k<channel.size(); k++) {
    if (channel[k]=="IR+V") {
      std::string name=file.name;
      if (miutil::contains(name, "v."))
        miutil::replace(name, "v.", "i.");
      else if (miutil::contains(name, "i."))
        miutil::replace(name, "i.", "v.");
      std::ifstream inFile(name.c_str(), std::ios::in);
      if (inFile)
        file.channel.push_back("IR+V");
    }

    else if (channel[k]=="day_night") {
      file.channel.push_back("day_night");
    }
    else if (channel[k]=="RGB") {
      file.channel.push_back("RGB");
    }
    else if (channel[k]=="IR") {
      file.channel.push_back("IR");
    }
    else if (miutil::contains(channel[k], "+") ) {
      std::vector<std::string> ch = miutil::split(channel[k], "+");
      bool found =false;
      for (unsigned int l=0; l<ch.size(); l++) {
        found =false;
        for (unsigned int m=0; m<file.channel.size(); m++)
          if (ch[l]==file.channel[m]) {
            found=true;
            break;
          }
        if (!found)
          break;
      }
      if (found)
        file.channel.push_back(channel[k]);
    }

  }

  return true;
}

const std::vector<std::string>& SatManager::getChannels(const std::string &satellite,
    const std::string & file, int index)
{
  if (subProdInfo* subp = findProduct(satellite, file)) {
    if (index < 0 || index >= int(subp->file.size()))
      return subp->channel;

    if (subp->file[index].opened)
      return subp->file[index].channel;
    if (readHeader(subp->file[index], subp->channel)) {
      subp->file[index].opened = true;
      return subp->file[index].channel;
    }
  }

  static const std::vector<std::string> empty;
  return empty;
}

void SatManager::listFiles(subProdInfo &subp)
{
  METLIBS_LOG_SCOPE();

  if (subp.archive && !useArchive)
    return;

  if (subp.archive && !subp.file.empty())
    return;

  METLIBS_LOG_DEBUG("updating file lists");

  subp.file.clear();

  for (unsigned int j=0; j<subp.pattern.size() ;j++) {

    const diutil::string_v matches = diutil::glob(subp.pattern[j]);
    if (matches.empty()) {
      METLIBS_LOG_WARN("No files found! " << subp.pattern[j]);
    }
    for (diutil::string_v::const_reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
      SatFileInfo ft;
      ft.name = *it;
      ft.formattype= subp.formattype;
      ft.metadata = subp.metadata;
      ft.proj4string = subp.proj4string;
      ft.channelinfo = subp.channelinfo;
      ft.paletteinfo = subp.paletteinfo;
      ft.hdf5type = subp.hdf5type;
      METLIBS_LOG_DEBUG(LOGVAL(ft.name));

      if (subp.filter[j].getTime(ft.name, ft.time)) {
        METLIBS_LOG_DEBUG("Time from filename:" << LOGVAL(ft.name) << LOGVAL(ft.time));
        ft.opened = false;
      } else {
        // Open file if time not found from filename
        readHeader(ft, subp.channel);
        ft.opened = true;
        METLIBS_LOG_DEBUG("Time from File:" LOGVAL(ft.time));
      }

      // put it in the sorted list
      std::vector<SatFileInfo>::iterator p =
          std::lower_bound(subp.file.begin(), subp.file.end(), ft.time, [](const SatFileInfo& i, const miutil::miTime& t) { return t < i.time; });
      subp.file.insert(p, ft);
    }
  }

  if (subp.formattype == "mitiff") {
    //update Prod[satellite][file].colours
    //Asumes that all files have same palette
    int n=subp.file.size();
    //check max 3 files,
    int i=0;
    while (i < n && i < 3 && !MItiff::readMItiffPalette(subp.file[i].name, subp.colours))
      i++;
  }
#ifdef HDF5FILE
  if(subp.formattype == "hdf5" || subp.formattype=="hdf5-standalone") {
    if (subp.file.size() > 0 && subp.file[0].name != "") {
      HDF5::readHDF5Palette(subp.file[0],subp.colours);
    }
  }
#endif

#ifdef GEOTIFF
  else if (subp.formattype == "geotiff") {
    //update Prod[satellite][file].colours
    //Asumes that all files have same palette
    if (subp.file.size() > 0 && subp.file[0].name != "") {
      GEOtiff::readGEOtiffPalette(subp.file[0].name, subp.colours);
    }
  }
#endif
}

SatManager::subProdInfo* SatManager::findProduct(const std::string& image, const std::string& subtype)
{
  METLIBS_LOG_SCOPE();

  const Prod_t::iterator itp = Prod.find(image);
  if (itp == Prod.end()) {
    METLIBS_LOG_WARN("Product doesn't exist:" << image);
    return nullptr;
  }

  const SubProd_t::iterator its = itp->second.find(subtype);
  if (its == itp->second.end()) {
    METLIBS_LOG_WARN("Subproduct doesn't exist:" << subtype);
    return nullptr;
  }

  return &its->second;
}

const std::vector<SatFileInfo>& SatManager::getFiles(const std::string& image, const std::string& subtype, bool update)
{
  METLIBS_LOG_SCOPE();
  if (subProdInfo* subp = findProduct(image, subtype)) {
    if (update)
      listFiles(*subp);

    return subp->file;
  }

  static const std::vector<SatFileInfo> empty;
  return empty;
}

const std::vector<Colour> & SatManager::getColours(const std::string &satellite,
    const std::string & file)
{
  if (subProdInfo* subp = findProduct(satellite, file))
    return subp->colours;

  static const std::vector<Colour> empty;
  return empty;
}

plottimes_t SatManager::getSatTimes(const std::string& satellite, const std::string& filetype)
{
  METLIBS_LOG_SCOPE();

  plottimes_t timeset;
  const Prod_t::iterator itS = Prod.find(satellite);
  if (itS != Prod.end()) {
    const SubProd_t::iterator itF = itS->second.find(filetype);
    if (itF != itS->second.end()) {
      subProdInfo& subp = itF->second;
      listFiles(subp);
      for (const auto& f : subp.file)
        timeset.insert(f.time);
    }
  }

  return timeset;
}

void SatManager::getCapabilitiesTime(plottimes_t& normalTimes, int& timediff, const PlotCommand_cp& pinfo)
{
  //Finding times from pinfo
  //If pinfo contains "file=", return constTime

  timediff=0;

  SatPlotCommand_cp cmd = std::dynamic_pointer_cast<const SatPlotCommand>(pinfo);
  if (!cmd)
    return;

  timediff = cmd->timediff;

  //Product with prog times
  if (!cmd->hasFileName()) {
    for (const SatFileInfo& fi : getFiles(cmd->satellite, cmd->filetype, true))
      normalTimes.insert(fi.time);
  }
}

/**
 * Read config from setup file
 */
bool SatManager::parseSetup()
{
  //  * PURPOSE:   Info to fro setup
  METLIBS_LOG_SCOPE();

  //remove old setup info
  Prod.clear();

  const std::string sat_name = "IMAGE";
  std::vector<std::string> sect_sat;

  if (!SetupParser::getSection(sat_name, sect_sat)) {
    METLIBS_LOG_DEBUG("Missing section " << sat_name << " in setupfile.");
    return true;
  }

  std::string prod;
  std::string subprod;
  std::string file;
  std::string formattype = "mitiff";
  std::string metadata = "";
  std::string proj4string = "";
  std::string channelinfo = "";
  std::string paletteinfo = "";
  int hdf5type = 0;
  std::vector<std::string> channels;
  std::string key, value;
  bool mosaic=true;
  int iprod = 0;

  for (unsigned int i=0; i<sect_sat.size(); i++) {
    std::vector<std::string> token = miutil::split(sect_sat[i],1, "=");
    if (token.size() != 2) {
      std::string errmsg="Line must contain '='";
      SetupParser::errorMsg(sat_name, i, errmsg);
      return false;
    }
    key = miutil::to_lower(token[0]);
    value = token[1];

    if (key == "channels") {
      std::vector<std::string> chStr=miutil::split(value, " ");
      int nch=chStr.size();
      channels.clear();
      for (int j=0; j<nch; j++) {
        std::vector<std::string> vstr=miutil::split(chStr[j], ":");
        if (vstr.size()==2)
          channelmap[vstr[0]] = vstr[1];
        channels.push_back(vstr[0]);
      }
    } else if (key=="mosaic") {
      //METLIBS_LOG_DEBUG("mosaic " << value);
      mosaic=(value=="yes") ? true : false;
    } else if (key == "image") {
      prod=value;
      subprod.clear();
      //SatDialogInfo
      iprod=0;
      int n= Dialog.image.size();
      while (iprod<n && Dialog.image[iprod].name != value)
        iprod++;
      if (iprod==n) {
        SatDialogInfo::Image image;
        image.name = value;
        Dialog.image.push_back(image);
      }

    } else if (key == "formattype") {
      if (prod.empty()) {
        std::string errmsg="You must give image and sub.type before formattype";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      formattype = value;

    } else if (key == "channelinfo") {
      if (prod.empty()) {
        std::string errmsg="You must give image and sub.type before formattype";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      channelinfo = value;

    } else if (key == "paletteinfo") {
      if (prod.empty()) {
        std::string errmsg="You must give image and sub.type before palette";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      paletteinfo = value;

    } else if (key == "metadata") {
      if (prod.empty()) {
        std::string errmsg="You must give image and sub.type before formattype";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      metadata = value;

    }
    else if (key == "proj4string") {
      if (prod.empty()) {

        std::string errmsg="You must give image and sub.type before proj4string";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      proj4string = value;
    }
    else if (key == "hdf5type") {
      if (prod.empty()) {
        std::string errmsg="You must give image and sub.type before type";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      if (miutil::to_lower(value) == "radar") {
        hdf5type = 0;
      } else if (miutil::to_lower(value) == "noaa") {
        hdf5type = 1;
      } else if (miutil::to_lower(value) == "msg") {
        hdf5type = 2;
      } else if (miutil::to_lower(value) == "saf") {
        hdf5type = 3;
      }
    } else if (key == "sub.type") {
      if (prod.empty()) {
        std::string errmsg="You must give image before sub.type";
        SetupParser::errorMsg(sat_name, i, errmsg);
        continue;
      }
      subprod=value;
      //SatDialogInfo
      int j=0;
      int n=Dialog.image[iprod].file.size();
      while (j<n && Dialog.image[iprod].file[j].name != value)
        j++;
      if (j==n) {
        SatDialogInfo::File file;
        file.name = value;
        file.channel = channels;
        Dialog.image[iprod].file.push_back(file);
      }
    } else if (key == "file" || key == "archivefile") {
      if (subprod.empty() ) {
        std::string errmsg="You must give image and sub.type before file";
        SetupParser::errorMsg(sat_name, i, errmsg);
        return false;
      }
      subProdInfo& sp = Prod[prod][subprod]; // insert if not yet defined
      // init time filter and replace yyyy etc. with *
      const miutil::TimeFilter tf(value); // modifies value!
      sp.filter.push_back(tf);
      sp.pattern.push_back(value);
      sp.channel = channels;
      sp.mosaic = mosaic;
      sp.archive = (key == "archivefile");
      sp.formattype = formattype;
      sp.metadata = metadata;
      sp.proj4string = proj4string;
      sp.channelinfo = channelinfo;
      sp.paletteinfo = paletteinfo;
      sp.hdf5type = hdf5type;
      hdf5type = 0; //> reset type
      metadata = ""; //> reset metadata
      proj4string = ""; //> reset proj4string
      paletteinfo = ""; //> reset palette
      channelinfo = ""; //> reset channelinfo
    }
  }

  Dialog.cut.minValue=0;
  Dialog.cut.maxValue=5;
  Dialog.cut.value=2;
  Dialog.cut.scale=0.01;
  Dialog.alphacut.minValue=0;
  Dialog.alphacut.maxValue=10;
  Dialog.alphacut.value=0;
  Dialog.alphacut.scale=0.1;
  Dialog.alpha.minValue=0;
  Dialog.alpha.maxValue=10;
  Dialog.alpha.value=10;
  Dialog.alpha.scale=0.1;
  Dialog.timediff.minValue=0;
  Dialog.timediff.maxValue=96;
  Dialog.timediff.value=4;
  Dialog.timediff.scale=15;

  return true;
}

void SatManager::init_rgbindex(Sat& sd)
{
  METLIBS_LOG_SCOPE();
  if (sd.no==1) {
    sd.rgbindex[0]= 0;
    sd.rgbindex[1]= 0;
    sd.rgbindex[2]= 0;

  } else if (sd.no==2) {
    sd.rgbindex[0]= 0;
    sd.rgbindex[1]= 0;
    sd.rgbindex[2]= 1;

  } else if (sd.no ==3) {
    sd.rgbindex[0]= 0;
    sd.rgbindex[1]= 1;
    sd.rgbindex[2]= 2;

  } else {
    METLIBS_LOG_INFO("number of channels: " << sd.no);
  }
}

void SatManager::init_rgbindex_Meteosat(Sat& sd)
{
  //METEOSAT; if channel = IR+V, we have to read another file.
  //Filenames for visual channel end by "v", infrared names
  //end by "i". In order to get both channels, the filename are changed
  // the other channel is read from another file
  //  This image is put in last rawimage slot

  const int tmpidx= Sat::maxch-1;

  std::string name=sd.actualfile;
  Sat sd2;
  if (miutil::contains(name, "v.")) {
    miutil::replace(name, "v.", "i.");
    std::ifstream inFile(name.c_str(), std::ios::in);
    std::string cal=sd.cal_vis;

    if (inFile && MItiff::readMItiff(name, sd, tmpidx)) {
      sd.cal_ir=sd2.cal_ir;
      sd.rgbindex[0]= 0;
      sd.rgbindex[1]= 0;
      sd.rgbindex[2]= tmpidx;
    } else {
      METLIBS_LOG_WARN("No IR channel available");
      sd.rgbindex[0]= 0;
      sd.rgbindex[1]= 0;
      sd.rgbindex[2]= 0;
      sd.plotChannels = "VIS_RAW"; //for annotations
    }
    inFile.close();

  } else if (miutil::contains(name, "i.")) {
    miutil::replace(name, "i.", "v.");
    std::string cal=sd.cal_ir;
    std::ifstream inFile(name.c_str(), std::ios::in);

    if (inFile && MItiff::readMItiff(name, sd, tmpidx)) {
      sd.cal_ir=cal;
      sd.rgbindex[0]= tmpidx;
      sd.rgbindex[1]= tmpidx;
      sd.rgbindex[2]= 0;
    } else {
      METLIBS_LOG_WARN("No visual channel available");
      sd.rgbindex[0]= 0;
      sd.rgbindex[1]= 0;
      sd.rgbindex[2]= 0;
      sd.plotChannels = "IR_CAL"; //for annotations
    }
    inFile.close();
  }
}
