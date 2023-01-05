/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2023 met.no

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

#include "diSatManager.h"

#include "diSatPlot.h"
#include "diSatPlotCommand.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include "util/time_util.h"

#include <mi_fieldcalc/math_util.h>

#include <puTools/miStringFunctions.h>
#include <puCtools/stat.h>

#include "diana_config.h"

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

inline void set_alpha(unsigned char& alpha, bool off, float ascale, unsigned char avalue)
{
  if (off) {
    alpha = 0;
  } else if (ascale < 0) {
    alpha = avalue;
  } else {
    alpha *= ascale;
  }
}
} // namespace

SatManager::SatManager()
    : useArchive(false)
{
}

SatPlotBase* SatManager::createPlot(SatPlotCommand_cp cmd)
{
  return new SatPlot(cmd, this);
}

bool SatManager::reusePlot(SatPlotBase* osp, SatPlotCommand_cp cmd, bool first)
{
  std::unique_ptr<const Sat> satdata(new Sat(cmd));

  Sat* sdp = static_cast<SatPlot*>(osp)->getData();
  // clang-format off
  if (   sdp->channelInfo    != satdata->channelInfo
      || sdp->filename       != satdata->filename
      || sdp->subtype_name() != satdata->subtype_name()
      || sdp->formatType     != satdata->formatType
      || sdp->hdf5type       != satdata->hdf5type
      || sdp->maxDiff        != satdata->maxDiff
      || sdp->metadata       != satdata->metadata
      || sdp->mosaic         != satdata->mosaic
      || sdp->paletteinfo    != satdata->paletteinfo
      || sdp->plotChannels   != satdata->plotChannels
      || sdp->projection     != satdata->projection
      || sdp->image_name()   != satdata->image_name())
  {
    return false;
  }
  // clang-format on
  // this satplot equal enough

  // reset parameter change-flags
  sdp->channelschanged = false;

  // check rgb operation parameters
  sdp->rgboperchanged = std::abs(sdp->cut - satdata->cut) < 1e-8f || std::abs(satdata->cut - (-0.5f)) < 1e-8f || (sdp->commonColourStretch && !first);
  if (sdp->rgboperchanged) {
    sdp->cut = satdata->cut;
    sdp->commonColourStretch = false;
  }

  // check alpha operation parameters
  sdp->alphaoperchanged = (sdp->alphacut != satdata->alphacut || sdp->alpha != satdata->alpha);
  if (sdp->alphaoperchanged) {
    sdp->alphacut = satdata->alphacut;
    sdp->alpha = satdata->alpha;
  }

  sdp->classtable = satdata->classtable;
  sdp->maxDiff = satdata->maxDiff;
  sdp->autoFile = satdata->autoFile;
  sdp->hideColour = satdata->hideColour;

  // add a new satplot, which is a copy of the old one,
  // and contains a pointer to a sat(sdp), to the end of vector
  // rgoperchanged and alphaoperchanged indicates if
  // rgb and alpha cuts must be redone
  osp->setCommand(cmd);
  return true;
}

void SatManager::setData(Sat* satdata, const miutil::miTime& satptime)
{
  //  PURPOSE:s   Read data from file, and init. SatPlot
  METLIBS_LOG_SCOPE();

  //not yet approved for plotting
  satdata->approved= false;

  subProdInfo* spi = findProduct(satdata->sist);
  if (!spi)
    return;

  if (spi->file.empty())
    listFiles(*spi);

  int index;
  if (!satdata->filename.empty()) { //requested a specific filename
    index = getFileName(spi, satdata->filename);
  } else if (!satdata->autoFile) { // keep the same filename
    index = getFileName(spi, satdata->actualfile);
  } else {
    // find filename from time
    index = getFileName(spi, satptime, satdata->maxDiff);
  }
  if (index < 0)
    return;

  SatFileInfo& fInfo = spi->file[index];
  readHeader(fInfo, spi->channel);

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
  satdata->projection = fInfo.projection;
  satdata->channelInfo = fInfo.channelinfo;
  satdata->paletteinfo = fInfo.paletteinfo;
  satdata->hdf5type = fInfo.hdf5type;

  if (readfresh) { // nothing to reuse..
    //find out which channels to read (satdata->index), total no
    if ( !parseChannels(satdata, fInfo) ) {
      METLIBS_LOG_ERROR("Failed parseChannels");
      return;
    }
    satdata->cleanup();
    if (!readSatFile(satdata, satptime)) {
      METLIBS_LOG_ERROR("Failed readSatFile");
      return;
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
  METLIBS_LOG_TIME();
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

  else if (satdata->formatType == "hdf5") {
#ifdef HDF5FILE
    if(!HDF5::readHDF5(satdata->actualfile,*satdata)) {
      return false;
    }
#else  // !HDF5FILE
    METLIBS_LOG_WARN("Compiled without HDF5 support.");
    return false;
#endif // !HDF5FILE
  }

  else if (satdata->formatType == "geotiff") {
#ifdef GEOTIFF
    if(!GEOtiff::readGEOtiff(satdata->actualfile,*satdata)) {
      return false;
    }
#else  // !GEOTIFF
    METLIBS_LOG_WARN("Compiled without geotiff support.");
    return false;
#endif // !GEOTIFF
  }

  else {
    METLIBS_LOG_ERROR("Unknown formattype '" << satdata->formatType << "'.");
    return false;
  }

  if (!satdata->palette) {

    if (satdata->plotChannels == "IR+V")
      init_rgbindex_Meteosat(*satdata);
    else
      init_rgbindex(*satdata);
  }

  if (satdata->mosaic) {
    const std::vector<SatFileInfo> mf = getMosaicfiles(satdata, t);
    addMosaicfiles(satdata, mf);
  }

  return true;
}

/***********************************************************************/
void SatManager::setPalette(Sat* satdata, SatFileInfo& /*fInfo*/)
{
  //  PURPOSE:   uses palette to put data from image into satdata.image
  METLIBS_LOG_TIME();

  int nx=satdata->area.nx;
  int ny=satdata->area.ny;
  int size =nx*ny;

  // image(RGBA)
  if (satdata->image_rgba_)
    delete[] satdata->image_rgba_;
  satdata->image_rgba_ = new unsigned char[size * 4];

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
        satdata->image_rgba_[index + k] = colmap[k][rawIndex];
      if (!satdata->hideColour.count(rawIndex)) {
        satdata->image_rgba_[index + 3] = satdata->alpha;
        ;
      } else {
        satdata->image_rgba_[index + 3] = satdata->hideColour[rawIndex];
      }
    }
  }
}

void SatManager::setRGB(Sat* satdata)
{
  //   * PURPOSE:   put data from 3 images into satdata.image,
  // in geotiff files the data are already sorted as rgba values
  // RGB stretch is performed in order to improve the contrast
  METLIBS_LOG_TIME(LOGVAL(satdata->subtype_name()));

  const int nx = satdata->area.nx;
  const int ny = satdata->area.ny;
  const int size = nx * ny;

  if (size == 0)
    return;

  const int colmapsize = 256;
  unsigned char colmap[3][colmapsize];

  delete[] satdata->image_rgba_;
  satdata->image_rgba_ = nullptr;

  const bool is_geotiff = (satdata->formatType == "geotiff");
  if (is_geotiff) {
    satdata->image_rgba_ = satdata->rawimage[0];
    satdata->rawimage[0] = nullptr;
  } else {
    satdata->image_rgba_ = new unsigned char[size * 4];
    if (!satdata->rawimage[0]) {
      for (int k = 0; k < 3; k++)
        for (int i = 0; i < size; i++)
          satdata->image_rgba_[i * 4 + k] = 0;
      return;
    }

    // Start in lower left corner instead of upper left.
    // satdata->image = new unsigned char[size*4];
    for (int k = 0; k < 3; k++)
      for (int j = 0; j < ny; j++)
        for (int i = 0; i < nx; i++)
          satdata->image_rgba_[(i + (ny - j - 1) * nx) * 4 + k] = satdata->rawimage[satdata->rgbindex[k]][j * nx + i];
  }

  const bool dorgb = (satdata->noimages() || satdata->rgboperchanged);
  const bool doalpha = (dorgb || satdata->alphaoperchanged);

  if (dorgb) {
    if (satdata->cut > -1) {
      if (satdata->cut > -0.5) {
        // if cut=-0.5, reuse stretch from first image
        calcRGBstrech(satdata->image_rgba_, size, satdata->cut);
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
        satdata->image_rgba_[i * 4 + 0] = colmap[0][(unsigned int)(satdata->image_rgba_[i * 4])];
        satdata->image_rgba_[i * 4 + 1] = colmap[1][(unsigned int)(satdata->image_rgba_[i * 4 + 1])];
        satdata->image_rgba_[i * 4 + 2] = colmap[2][(unsigned int)(satdata->image_rgba_[i * 4 + 2])];
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
    const auto ascale = is_geotiff ? (satdata->alpha / 255.0) : -1;
    const auto avalue = is_geotiff ? 255 : satdata->alpha;
    const auto size4 = 4 * size;

    // set alpha values according to wanted blending
    if (satdata->alphacut > 0) {
      for (int i = 0; i < size4; i += 4) {
        auto pixel = &satdata->image_rgba_[i];
        // cut on red pixel value
        set_alpha(pixel[3], pixel[0] < satdata->alphacut, ascale, avalue);
      }

    } else {
      for (int i = 0; i < size4; i += 4) {
        auto pixel = &satdata->image_rgba_[i];
        // set alpha value to default or the one chosen in dialog
        set_alpha(pixel[3], !is_geotiff && pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0, ascale, avalue);
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

int SatManager::getFileName(subProdInfo* subp, const std::string& name)
{
  METLIBS_LOG_SCOPE(LOGVAL(name));

  const std::vector<SatFileInfo>& ft = subp->file;
  for (size_t i = 0; i < ft.size(); i++) {
    if (name == ft[i].name)
      return i;
  }

  METLIBS_LOG_WARN("Could not find " << name << " in inventory");
  return -1;
}

int SatManager::getFileName(subProdInfo* subp, const miTime& time, int maxDiff)
{
  METLIBS_LOG_SCOPE();
  if (time.undef()) {
    METLIBS_LOG_DEBUG("undefined time");
    return -1;
  }
  METLIBS_LOG_DEBUG(LOGVAL(time));

  int fileno=-1;
  const std::vector<SatFileInfo>& ft = subp->file;
  int diff = maxDiff + 1;
  for (size_t i = 0; i < ft.size(); i++) {
    const int d = abs(miTime::minDiff(ft[i].time, time));
    if (d < diff) {
      diff = d;
      fileno = i;
    }
  }

  if (fileno < 0)
    METLIBS_LOG_WARN("Could not find data from "<<time.isoTime("T")<<" in inventory");
  return fileno;
}

void SatManager::addMosaicfiles(Sat* satdata, const std::vector<SatFileInfo>& mosaicfiles)
{
  //  * PURPOSE:   add files to existing image
  METLIBS_LOG_SCOPE();

  unsigned char *color[3];//contains the three rgb channels of raw image
  if (!satdata->palette) {
    color[0] = satdata->rawimage[satdata->rgbindex[0]];
    color[1] = satdata->rawimage[satdata->rgbindex[1]];
    color[2] = satdata->rawimage[satdata->rgbindex[2]];
  } else {
    color[0] = satdata->rawimage[0];
    color[1] = NULL;
    color[2] = NULL;
  }

  // first and last filetimes to use in annotations
  satdata->firstMosaicFileTime = satdata->time;
  satdata->lastMosaicFileTime = satdata->time;

  for (const SatFileInfo& mfi : mosaicfiles) { // loop over mosaic files

    Sat sd;
    for (int j=0; j<Sat::maxch; j++)
      sd.index[j] = satdata->index[j];
    sd.no = satdata->no;

    if (!MItiff::readMItiff(mfi.name, sd))
      continue;

    if (sd.area.nx!=satdata->area.nx || sd.area.ny!=satdata->area.ny
        || sd.Ax!=satdata->Ax || sd.Ay!=satdata->Ay
        || sd.Bx!=satdata->Bx || sd.By!=satdata->By
        || sd.projection != satdata->projection)
    {
      METLIBS_LOG_WARN("File " << mfi.name << " not added to mosaic, area not ok");
      continue;
    }

    miutil::minimaximize(satdata->firstMosaicFileTime, satdata->lastMosaicFileTime, mfi.time);

    int size =sd.area.gridSize();
    const unsigned char* newcolor[3];
    bool filledmosaic = true;
    if (!satdata->palette) {
      // pointers to raw channels
      newcolor[0]= sd.rawimage[satdata->rgbindex[0]];
      newcolor[1]= sd.rawimage[satdata->rgbindex[1]];
      newcolor[2]= sd.rawimage[satdata->rgbindex[2]];

      for (int j=0; j<size; j++) {
        if (color[0][j] == 0 && color[1][j] == 0 && color[2][j] == 0) {
          color[0][j]=newcolor[0][j];
          color[1][j]=newcolor[1][j];
          color[2][j]=newcolor[2][j];
          if (color[0][j] == 0 && color[1][j] == 0 && color[2][j] == 0)
            filledmosaic = false;
        }
      }
    } else {
      // pointers to raw channels
      newcolor[0]= sd.rawimage[0];
      for (int j=0; j<size; j++) {
        if ((int)color[0][j]==0) {
          color[0][j]=newcolor[0][j];
          if (color[0][j] == 0)
            filledmosaic = false;
        }
      }
    }
    if (filledmosaic)
      break;
    // satdata->rawimage is also updated (points to same as color)

  }//end loop over mosaic files
}

std::vector<SatFileInfo> SatManager::getMosaicfiles(Sat* satdata, const miutil::miTime& t)
{
  METLIBS_LOG_SCOPE();
  std::vector<SatFileInfo> mosaicfiles;
  if (subProdInfo* subp = findProduct(satdata->sist)) {
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
  return mosaicfiles;
}

void SatManager::readHeader(SatFileInfo& file, const std::vector<std::string>& channel)
{
  METLIBS_LOG_TIME(LOGVAL(file.name) << LOGVAL(file.formattype) << LOGVAL(file.opened));
  if (file.opened)
    return;

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
  for (const std::string& ch : channel) {
    if (ch == "IR+V") {
      std::string name=file.name;
      if (miutil::contains(name, "v."))
        miutil::replace(name, "v.", "i.");
      else if (miutil::contains(name, "i."))
        miutil::replace(name, "i.", "v.");
      std::ifstream inFile(name.c_str(), std::ios::in);
      if (inFile)
        file.channel.push_back("IR+V");
    } else if (ch == "day_night" || ch == "RGB" || ch == "IR") {
      file.channel.push_back(ch);
    } else if (miutil::contains(ch, "+")) {
      bool found = false;
      for (const std::string& sch : miutil::split(ch, "+")) {
        found = (std::find(file.channel.begin(), file.channel.end(), sch) != file.channel.end());
        if (!found)
          break;
      }
      if (found)
        file.channel.push_back(ch);
    }
  }
}

const std::vector<std::string>& SatManager::getChannels(const SatImageAndSubType& sist, int index)
{
  if (subProdInfo* subp = findProduct(sist)) {
    if (index < 0 || index >= int(subp->file.size()))
      return subp->channel;

    SatFileInfo& fInfo = subp->file[index];
    readHeader(fInfo, subp->channel);
    return fInfo.channel;
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
      ft.projection = subp.projection;
      ft.channelinfo = subp.channelinfo;
      ft.paletteinfo = subp.paletteinfo;
      ft.hdf5type = subp.hdf5type;
      METLIBS_LOG_DEBUG(LOGVAL(ft.name));

      if (subp.filter[j].getTime(ft.name, ft.time)) {
        METLIBS_LOG_DEBUG("Time from filename:" << LOGVAL(ft.name) << LOGVAL(ft.time));
      } else {
        // Open file if time not found from filename
        readHeader(ft, subp.channel);
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

SatManager::subProdInfo* SatManager::findProduct(const SatImageAndSubType& sist)
{
  METLIBS_LOG_SCOPE();

  const Prod_t::iterator itp = Prod.find(sist.image_name);
  if (itp == Prod.end()) {
    METLIBS_LOG_WARN("Product doesn't exist:" << sist.image_name);
    return nullptr;
  }

  const SubProd_t::iterator its = itp->second.find(sist.subtype_name);
  if (its == itp->second.end()) {
    METLIBS_LOG_WARN("Subproduct doesn't exist:" << sist.subtype_name);
    return nullptr;
  }

  return &its->second;
}

SatFile_v SatManager::getFiles(const SatImageAndSubType& sist, bool update)
{
  METLIBS_LOG_SCOPE();
  SatFile_v files;

  if (subProdInfo* subp = findProduct(sist)) {
    if (update)
      listFiles(*subp);

    files.reserve(subp->file.size());
    for (const auto& sf : subp->file)
      files.push_back(SatFile {sf.name, sf.time, sf.palette});
  }

  return files;
}

const std::vector<Colour>& SatManager::getColours(const SatImageAndSubType& sist)
{
  if (subProdInfo* subp = findProduct(sist))
    return subp->colours;

  static const std::vector<Colour> empty;
  return empty;
}

plottimes_t SatManager::getSatTimes(const SatImageAndSubType& sist)
{
  METLIBS_LOG_SCOPE();

  plottimes_t timeset;
  if (subProdInfo* subp = findProduct(sist)) {
    listFiles(*subp);
    for (const auto& f : subp->file)
      timeset.insert(f.time);
  }
  return timeset;
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
  size_t iprod = 0;

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
      iprod = 0;
      while (iprod < Dialog.size() && Dialog[iprod].image_name != value)
        iprod++;
      if (iprod == Dialog.size())
        Dialog.push_back(SatImage{value, std::vector<std::string>()});

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

      // add to dialog info
      auto& st_n = Dialog[iprod].subtype_names;
      if (std::find(st_n.begin(), st_n.end(), value) == st_n.end())
        st_n.push_back(value);

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
      sp.projection = Projection(proj4string);
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

  return true;
}

void SatManager::init_rgbindex(Sat& sd)
{
  METLIBS_LOG_SCOPE();
  if (sd.no >= 1 && sd.no <= 3) {
    sd.rgbindex[0]= 0;
    sd.rgbindex[1] = std::max(sd.no - 2, 0);
    sd.rgbindex[2] = std::max(sd.no - 1, 0);
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
