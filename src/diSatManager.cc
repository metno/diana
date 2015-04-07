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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <diSatManager.h>

#include "diPlotModule.h"
#include "diUtilities.h"

#include <puCtools/stat.h>
#include <puTools/miSetupParser.h>

#include <diMItiff.h>
#ifdef HDF5FILE
#include <diHDF5.h>
#endif
#ifdef GEOTIFF
#include <diGEOtiff.h>
#endif

#include <GL/gl.h>
#include <fstream>
#include <set>

#define MILOGGER_CATEGORY "diana.SatManager"
#include <miLogger/miLogging.h>

using namespace miutil;

static const std::vector<SatFileInfo> emptyfile;

//#define DEBUGPRINT
SatManager::SatManager()
{
  //Max time between filelist updates in seconds
  updateFreq = 300;
  //new satellite files read
  fileListChanged = false;
  //zero time = 00:00:00 UTC Jan 1 1970
  ztime = miTime(1970, 1, 1, 0, 0, 0);
  useArchive=false;
}

void SatManager::prepareSat(const std::vector<std::string>& inp)
{
  METLIBS_LOG_SCOPE();

  diutil::was_enabled plotenabled;
  for (unsigned int i = 0; i < vsp.size(); i++)
    plotenabled.save(vsp[i], vsp[i]->getPlotInfo(4));

  if (!init(inp))
    METLIBS_LOG_WARN("init returned false");

  for (unsigned int i = 0; i < vsp.size(); i++)
    plotenabled.restore(vsp[i], vsp[i]->getPlotInfo(4));
}

bool SatManager::init(const std::vector<std::string>& pinfo)
{
  //     PURPOSE:   Decode PlotInfo &pinfo
  //                - make a new SatPlot for each SAT entry in pinfo
  //                - if similar plot already exists, just make a copy of the
  //                  old one (satellite, filetype and channel the same)
  METLIBS_LOG_SCOPE();

  // init inuse array
  std::vector<bool> inuse(vsp.size(), false);
  SatPlot_xv new_vsp;

  bool reuseCommonColourStretch=false;

  // loop through all PlotInfo's
  for (size_t ip=0; ip<pinfo.size(); ip++) {
    // make a new SatPlot with a new Sat
    Sat* satdata = new Sat(pinfo[ip]);

    bool isok= false;
    if (not vsp.empty()) { // if satplots exists
      for (size_t j=0; j<vsp.size(); j++) {
        if (!inuse[j] && vsp[j]!=0) { // not already taken
          Sat *sdp = vsp[j]->satdata;
          if (sdp->satellite != satdata->satellite || sdp->filetype
              != satdata->filetype || sdp->formatType != satdata->formatType
              || sdp->metadata != satdata->metadata || sdp->proj_string != satdata->proj_string || sdp->channelInfo
              != satdata->channelInfo || sdp->paletteinfo
              != satdata->paletteinfo || sdp->hdf5type != satdata->hdf5type
              || sdp->plotChannels != satdata->plotChannels || sdp->mosaic
              !=satdata->mosaic || sdp->maxDiff!=satdata->maxDiff
              || sdp->filename !=satdata->filename)
            continue;
          // this satplot equal enough
          // reset parameter change-flags
          sdp->channelschanged= false;
          sdp->rgboperchanged= false;
          sdp->alphaoperchanged= false;
          sdp->mosaicchanged=false;
          // check rgb operation parameters
          if (sdp->cut != satdata->cut || satdata->cut == -0.5
              || (sdp->commonColourStretch && ip>0)) {
            sdp->cut= satdata->cut;
            sdp->rgboperchanged= true;
            sdp->commonColourStretch=false;
          } else if (sdp->commonColourStretch) {
            reuseCommonColourStretch=true;
          }
          // check alpha operation parameters
          if ((sdp->alphacut != satdata->alphacut) || (sdp->alpha
              != satdata->alpha)) {
            sdp->alphacut= satdata->alphacut;
            sdp->alpha= satdata->alpha;
            sdp->alphaoperchanged= true;
          }
          // new satdata is not needed, but keep filename and classtable
          // variables from PlotInfo before deleting it
          sdp->filename= satdata->filename;
          sdp->formatType= satdata->formatType;
          sdp->metadata = satdata->metadata;
          sdp->proj_string = satdata->proj_string;
          sdp->channelInfo = satdata->channelInfo;
          sdp->paletteinfo = satdata->paletteinfo;
          sdp->hdf5type = satdata->hdf5type;

#ifdef DEBUGPRINT
          METLIBS_LOG_DEBUG(LOGVAL(sdp->formatType) << LOGVAL(sdp->metadata));
#endif

          sdp->classtable= satdata->classtable;
          sdp->maxDiff = satdata->maxDiff;
          sdp->autoFile = satdata->autoFile;
          sdp->hideColour=satdata->hideColour;
          delete satdata; //deleting the new sat created
          // add a new satplot, which is a copy of the old one,
          // and contains a pointer to a sat(sdp), to the end of vector
          // rgoperchanged and alphaoperchanged indicates if
          // rgb and alpha cuts must be redone
          isok= true;
          inuse[j]= true;
          vsp[j]->setPlotInfo(pinfo[ip]);
          new_vsp.push_back(vsp[j]);
          break;
        }
      }
    }
    if (!isok) { // make new satplot
      SatPlot *sp = new SatPlot;
      sp->setData(satdata); //new sat, with no images
      sp->setPlotInfo(pinfo[ip]);
      new_vsp.push_back(sp);
    }
  } // end loop PlotInfo's

  // delete unwanted satplots  (all plots not in use)
  for (size_t i=0; i<vsp.size(); i++) {
    if (!inuse[i]) {
      delete vsp[i];
    }
  }
  vsp = new_vsp;

  //reset colourStretchInfo
  if (!reuseCommonColourStretch) {
    colourStretchInfo.channels.clear();
  }

  return true;
}

void SatManager::addPlotElements(std::vector<PlotElement>& pel)
{
  for (size_t j = 0; j < vsp.size(); j++) {
    std::string str = vsp[j]->getPlotName() + "# " + miutil::from_number(int(j));
    bool enabled = vsp[j]->isEnabled();
    pel.push_back(PlotElement("RASTER", str, "RASTER", enabled));
  }
}

void SatManager::enablePlotElement(const PlotElement& pe)
{
  if (pe.type != "RASTER")
    return;
  for (unsigned int i = 0; i < vsp.size(); i++) {
    std::string str = vsp[i]->getPlotName() + "# " + miutil::from_number(int(i));
    if (str == pe.str) {
      vsp[i]->setEnabled(pe.enabled);
      break;
    }
  }
}

void SatManager::addSatAnnotations(std::vector<AnnotationPlot::Annotation>& annotations)
{
  AnnotationPlot::Annotation ann;
  for (size_t j = 0; j < vsp.size(); j++) {
    if (!vsp[j]->isEnabled())
      continue;
    vsp[j]->getSatAnnotation(ann.str, ann.col);
    annotations.push_back(ann);
  }
}

void SatManager::getSatAnnotations(std::vector<std::string>& anno)
{
  for (size_t j = 0; j < vsp.size(); j++)
    vsp[j]->getAnnotations(anno);
}

void SatManager::plot(Plot::PlotOrder porder)
{
  for (size_t i = 0; i < vsp.size(); i++)
    vsp[i]->plot(porder);
}

void SatManager::clear()
{
  diutil::delete_all_and_clear(vsp);
}

bool SatManager::getGridResolution(float& rx, float& ry)
{
  if (vsp.empty())
    return false;
  rx = vsp[0]->getGridResolutionX();
  ry = vsp[0]->getGridResolutionY();
  return true;
}

bool SatManager::setData()
{
  METLIBS_LOG_SCOPE();
  bool allok = true;
  for (size_t i = 0; i < vsp.size(); i++) {
    if (not setData(vsp[i]))
      allok = false;
  }
  return allok;
}

bool SatManager::setData(SatPlot *satp)
{
  //  PURPOSE:s   Read data from file, and init. SatPlot
  METLIBS_LOG_SCOPE();

  Sat* satdata = satp->satdata;

  //not yet approved for plotting
  satdata->approved= false;
//  sp=satp;

  int index;
  if (!satdata->filename.empty()) { //requested a specific filename
    index=getFileName(satdata, satdata->filename);
  } else {
    if (!satdata->autoFile) //keep the same filename
      index=getFileName(satdata, satdata->actualfile);
    else
      //find filename from time
      index=getFileName(satdata, PlotModule::instance()->getStaticPlot()->getTime());
  }

  if (index <0) {
    return false;
  }

  //Read header if not opened
  if (!Prod[satdata->satellite][satdata->filetype].file[index].opened) {
    readHeader(Prod[satdata->satellite][satdata->filetype].file[index],
        Prod[satdata->satellite][satdata->filetype].channel);
    Prod[satdata->satellite][satdata->filetype].file[index].opened = true;
  }

  SatFileInfo & fInfo = Prod[satdata->satellite][satdata->filetype].file[index];

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
    satp->clearData();
    //find out which channels to read (satdata->index), total no
    if ( !parseChannels(satdata, fInfo) ) {
      METLIBS_LOG_ERROR("Failed parseChannels");
      return false;
    }
    satdata->cleanup();
    if (!readSatFile(satdata)) {
      METLIBS_LOG_ERROR("Failed readSatFile");
      return false;
    }
    satdata->setArea();
    satdata->setCalibration();
    satdata->setAnnotation();
    // ADC
    satdata->setPlotName();
    satp->setPlotName(satdata->plotname);
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
bool SatManager::parseChannels(Sat* satdata, SatFileInfo &fInfo)
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

bool SatManager::readSatFile(Sat* satdata)
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
    getMosaicfiles(satdata);
    addMosaicfiles(satdata);
  }

  return true;
}

/***********************************************************************/
void SatManager::setPalette(Sat* satdata, SatFileInfo &fInfo)
{
  //  PURPOSE:   uses palette to put data from image into satdata.image
  METLIBS_LOG_SCOPE(miTime::nowTime());

  int nx=satdata->nx;
  int ny=satdata->ny;
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

  //clean up fInfo
  fInfo.col.clear();
  //colours to return to sat dialog
  int ncolours = satdata->paletteInfo.noofcl;
  for (int i=0; i<=ncolours; i++)
    fInfo.col.push_back(Colour(colmap[0][i], colmap[1][i], colmap[2][i]));

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
  //   * PURPOSE:   put data from 3 images into satdata.image
  METLIBS_LOG_SCOPE(satdata->filetype);

  int i, j, k;
  int nx=satdata->nx;
  int ny=satdata->ny;
  int size =nx*ny;

  if (size==0)
    return;

  unsigned char *color[3];//contains the three rgb channels ofraw image
  if (satdata->formatType == "geotiff" ){
    // IMAGE is in UINT32 RGBA buffer
    color[0]= 0;
    color[1]= 0;
    color[2]= 0;
  }
  else {
    color[0]= satdata->rawimage[satdata->rgbindex[0]];
    color[1]= satdata->rawimage[satdata->rgbindex[1]];
    color[2]= satdata->rawimage[satdata->rgbindex[2]];
  }

  bool dorgb= (satdata->noimages() || satdata->rgboperchanged);
  bool doalpha= (satdata->noimages() || satdata->alphaoperchanged || dorgb);

  // METLIBS_LOG_DEBUG(dorgb << " " << doalpha);
  if (dorgb) {
    // index -> RGB
    const int colmapsize=256;
    unsigned char colmap[3][colmapsize];

    //Improve color
    if (satdata->cut > -1) {
      int shift;
      float factor;

      if (satdata->formatType == "geotiff" ) {
        if (satdata->rawimage[0]!=NULL) {
          int index[6];
          if (satdata->cut==-0.5 && //reuse stretch from first image
              satdata->plotChannels == colourStretchInfo.channels) {
            for (k=0; k<3; k++) {
              index[0+k*2] = colourStretchInfo.index1[k];
              index[1+k*2] = colourStretchInfo.index2[k];
            }
          } else { //find stretch from this image
            cutImageRGBA(satdata, satdata->rawimage[0], satdata->cut, index);
          }
          //remember stretch from first image
          if (colourStretchInfo.channels.empty() || satdata->commonColourStretch) {
            for (k=0; k<3; k++) {
              colourStretchInfo.index1[k]= index[0+k*2];
              colourStretchInfo.index2[k]= index[1+k*2];
            }
            colourStretchInfo.channels = satdata->plotChannels;
            satdata->commonColourStretch=true;
          }

          for (k=0; k<3; k++) {
            factor=(float)(colmapsize-1)/(index[1+k*2]-index[0+k*2]+1);
            shift=index[0+k*2]-1;
            int idx;
            colmap[k][0] = (unsigned char) (0);
            for (i=1; i<colmapsize; i++) {
              idx=(int)(float(i-shift)*factor + 0.5);
              if (idx<1)
                idx=1;
              if (idx>255)
                idx=255;
              colmap[k][i] = (unsigned char) (idx);
            }
          }
        }
      } else {
        for (k=0; k<3; k++)
          if (color[k]!=NULL && (k==0 || color[k]!=color[k-1])) {

            int index1, index2;
            if (satdata->cut==-0.5 && //reuse stretch from first image
                satdata->plotChannels == colourStretchInfo.channels) {
              index1 = colourStretchInfo.index1[k];
              index2 = colourStretchInfo.index2[k];
            } else { //find stretch from this image
              cutImage(satdata, color[k], satdata->cut, index1, index2);
            }
            //remember stretch from first image
            if (colourStretchInfo.channels.empty() || satdata->commonColourStretch) {
              colourStretchInfo.index1[k]= index1;
              colourStretchInfo.index2[k]= index2;
              colourStretchInfo.channels = satdata->plotChannels;
              satdata->commonColourStretch=true;
            }
            factor=(float)(colmapsize-1)/(index2-index1+1);
            shift=index1-1;
            int index;

            colmap[k][0] = (unsigned char) (0);
            for (i=1; i<colmapsize; i++) {
              index=(int)(float(i-shift)*factor + 0.5);
              if (index<1)
                index=1;
              if (index>255)
                index=255;
              colmap[k][i] = (unsigned char) (index);
            }
          }
      }
    } else { // set colourStretchInfo even if cut is off
      if (colourStretchInfo.channels.empty()) {
        for (k=0; k<3; k++) {
          colourStretchInfo.index1[k]= 0;
          colourStretchInfo.index2[k]= 255;
          colourStretchInfo.channels = satdata->plotChannels;
          satdata->commonColourStretch=true;
        }
      }
    }
    // Put 1,2 or 3 different channels into satdata->image(RGBA).
    // Start in lower left corner instead of upper left.

    if (satdata->formatType == "geotiff" ){
      // UINT32 RGBA version

      if (satdata->image)
        delete[] satdata->image;
      satdata->image= new unsigned char[size*4];

      if (satdata->rawimage[0] == NULL) {
        for (k=0; k<3; k++)
          for (i=0; i<size; i++)
            satdata->image[i*4+k] = 0;
      } else if (satdata->cut > -1 ) {
        for (i=0; i<size; i++) {
          satdata->image[i*4+0] = colmap[0][(unsigned int)(satdata->rawimage[0][i*4+0])];
          satdata->image[i*4+1] = colmap[1][(unsigned int)(satdata->rawimage[0][i*4+1])];
          satdata->image[i*4+2] = colmap[2][(unsigned int)(satdata->rawimage[0][i*4+2])];
        }
      } else {
        satdata->image= satdata->rawimage[0];
        satdata->rawimage[0] = NULL;
      }
    } else {
      if (satdata->image)
        delete[] satdata->image;
      satdata->image= new unsigned char[size*4];

      for (k=0; k<3; k++)
        if (color[k]==NULL) {
          for (i=0; i<size; i++)
            satdata->image[i*4+k] = 0;
        } else if (satdata->cut > -1 && (k==0 || color[k]!=color[k-1])) {
          for (j=0; j<ny; j++)
            for (i=0; i<nx; i++)
              satdata->image[(i+(ny-j-1)*nx)*4+k] = colmap[k][int(color[k][j*nx+i])];
        } else if (satdata->cut > -1) {
          for (i=0; i<size; i++)
            satdata->image[i*4+k] = satdata->image[i*4+k-1];
        } else {
          for (j=0; j<ny; j++)
            for (i=0; i<nx; i++)
              satdata->image[(i+(ny-j-1)*nx)*4+k] = color[k][j*nx+i];
        }
    }
  }

  if (doalpha) {
    //set alpha values according to wanted blending
    if (satdata->alphacut > 0) {
      for (i=0; i<size; i++) {
        //cut on pixelvalue
        if ( (int) satdata->image[i*4] < satdata->alphacut)
          satdata->image[i*4+3] = (unsigned char) 0;
        else
          //set alpha value to default or the one chosen in dialog
          satdata->image[i*4+3] = (unsigned char) satdata->alpha;
      }

    } else {
      for (i=0; i<size; i++) {
        //set alpha value to default or the one chosen in dialog
        satdata->image[i*4+3] = (unsigned char) satdata->alpha;
        //remove black pixels
        /*if (satdata->formatType != "geotiff") { */
        if (satdata->image[i*4] == 0 && satdata->image[i*4+1]== 0
            && satdata->image[i*4+2] == 0)
          satdata->image[i*4+3]=0;
        //	}

      }
    }
  }

}

void SatManager::cutImage(Sat* satdata, unsigned char *image, float cut, int &index1,
    int &index2)
{

  //  * PURPOSE:   (1-cut)*#pixels should have a value between index1 and index2

  int i;
  int nindex[256];
  int size=satdata->nx*satdata->ny;

  for (i=0; i<256; i++)
    nindex[i]=0;

  for (i=0; i<size; i++) {
    nindex[(int)image[i]]++;
  }

  float npixel=size-nindex[0]; //number of pixels, drop index=0
  int ncut=(int) (npixel*cut); //number of pixels to cut   +1?
  index1=1;
  index2=255;
  int nsum=0; //number of pixels dropped

  while (nsum<ncut && index1<index2) {
    if (nindex[index1] < nindex[index2]) {
      nsum += nindex[index1];
      index1++;
    } else {
      nsum += nindex[index2];
      index2--;
    }
  }

  //Strengt tatt skulle den index som sist ble flyttet, blitt flyttet
  //tilbake.
}

int SatManager::getFileName(Sat* satdata, std::string &name)
{
  METLIBS_LOG_SCOPE(name);

  std::vector<SatFileInfo> &ft = Prod[satdata->satellite][satdata->filetype].file;

  int fileno=-1;
  int n=ft.size();

  for (int i=0; i<n; i++) {
    if (name == ft[i].name) {
      fileno=i;
      break;
    }
  }

  if (fileno < 0 ) {
    METLIBS_LOG_WARN("Could not find "<<name<<" in inventory");
  }

  return fileno;
}

int SatManager::getFileName(Sat* satdata, const miTime &time)
{
  METLIBS_LOG_SCOPE(time);

  int fileno=-1;
  int diff= satdata->maxDiff+1;
  int d;

  subProdInfo &subp =Prod[satdata->satellite][satdata->filetype];

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG(LOGVAL(satdata->satellite) << LOGVAL(satdata->filetype));
#endif

  miTime now = miTime::nowTime();
  //time between now and last update ?
  int updiff= miTime::secDiff(now, ztime)-subp.updateTime;

  if (subp.file.size() == 0 || subp.updated ==false || (!subp.archiveFiles
      && updiff > updateFreq)) {
    listFiles(subp);
    subp.updated = true;
  } else
    fileListChanged = false;

  std::vector<SatFileInfo> &ft=subp.file;
  int n=ft.size();
  for (int i=0; i<n; i++) {
    d = abs(miTime::minDiff(ft[i].time, time));
    if (d<diff) {
      diff=d;
      fileno=i;
    }
  }

#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG(LOGVAL(fileno) /* << LOGVAL(fileListChanged)*/);
#endif

  if (fileno < 0 ) {
    METLIBS_LOG_WARN("Could not find data from "<<time.isoTime("T")<<" in inventory");
  }
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

    unsigned char* rawimage[Sat::maxch]; // raw images
    for (int j=0; j<Sat::maxch; j++)
      rawimage[j]= 0;
    Sat sd;
    for (int j=0; j<Sat::maxch; j++)
      sd.index[j] = satdata->index[j];
    sd.no = satdata->no;

    bool ok = MItiff::readMItiff(mosaicfiles[i].name, sd);

    if (!ok)
      continue;

    int size =sd.nx*sd.ny;
    if (sd.Ax!=satdata->Ax || sd.Ay!=satdata->Ay || sd.Bx!=satdata->Bx || sd.By
        !=satdata->By) {
      METLIBS_LOG_WARN("File "<<mosaicfiles[i].name <<" not added to mosaic, area not ok");
      continue;
    }

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

    for (int i=0; i<Sat::maxch; i++) {
      if (rawimage[i]!=0)
        delete[] rawimage[i];
      rawimage[i]= 0;
    }

  }//end loop over mosaic files

  //first and last filetimes to use in annotations
  satdata->firstMosaicFileTime=firstFileTime;
  satdata->lastMosaicFileTime=lastFileTime;

}

void SatManager::getMosaicfiles(Sat* satdata)
{
  METLIBS_LOG_SCOPE();

  int satdiff, plotdiff, diff= satdata->maxDiff+1;
  miTime plottime, sattime=satdata->time;
  if (satdata->autoFile)
    plottime=PlotModule::instance()->getStaticPlot()->getTime();
  else
    plottime=sattime;

  subProdInfo &subp =Prod[satdata->satellite][satdata->filetype];

  mosaicfiles. clear();
  std::vector<int> vdiff;

  std::vector<SatFileInfo>::iterator p = subp.file.begin();
  while (p!=subp.file.end()) {
    satdiff = abs(miTime::minDiff(p->time, sattime)); //diff from current sat time
    plotdiff = abs(miTime::minDiff(p->time, plottime)); //diff from current plottime
    if (plotdiff<diff && satdiff<diff && satdiff!=0) {
      std::vector<SatFileInfo>::iterator q = mosaicfiles.begin();
      std::vector<int>::iterator i=vdiff.begin();
      while (q!=mosaicfiles.end() && i!=vdiff.end() && satdiff>=*i) {
        q++;
        i++;
      }
      mosaicfiles.insert(q, *p);
      vdiff.insert(i, satdiff);
    }
    p++;
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
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("readHDF5Header");
#endif
    HDF5::readHDF5Header(file);
  }
#endif

#ifdef GEOTIFF
  if (file.formattype=="geotiff") {
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("reading geotiff "<<file.name);
#endif
    GEOtiff::readGEOtiffHeader(file);
#ifdef DEBUGPRINT
    METLIBS_LOG_DEBUG("finished reading geotiff "<<file.name);
#endif
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
  if (index<0 || index>=int (Prod[satellite][file].file.size()))
    return Prod[satellite][file].channel;

  if (Prod[satellite][file].file[index].opened)
    return Prod[satellite][file].file[index].channel;
  if (readHeader(Prod[satellite][file].file[index],
      Prod[satellite][file].channel)) {
    Prod[satellite][file].file[index].opened=true;
    return Prod[satellite][file].file[index].channel;
  }

  return Prod[satellite][file].channel;
}

void SatManager::listFiles(subProdInfo &subp)
{
  METLIBS_LOG_SCOPE();

  miTime now = miTime::nowTime();
  fileListChanged = false;

  //if not in archiveMode and archive files are included, clear list
  if (!useArchive && subp.archiveFiles)
    subp.file.clear();

  for (unsigned int j=0; j<subp.pattern.size() ;j++) {
    //skip archive files if not in archive mode
    if (subp.archive[j] && !useArchive)
      continue;

    const diutil::string_v matches = diutil::glob(subp.pattern[j]);
    if (matches.empty()) {
      METLIBS_LOG_ERROR("No files found! " << subp.pattern[j]);
    }
    for (diutil::string_v::const_reverse_iterator it = matches.rbegin(); it != matches.rend(); ++it) {
      //remember that archive files are read
      if (subp.archive[j])
        subp.archiveFiles=true;
      SatFileInfo ft;
      ft.name = *it;
      ft.formattype= subp.formattype;
      ft.metadata = subp.metadata;
      ft.proj4string = subp.proj4string;
      ft.channelinfo = subp.channelinfo;
      ft.paletteinfo = subp.paletteinfo;
      ft.hdf5type = subp.hdf5type;

      bool newfile = true;

      //HK ??? forandret kode for at oppdatering skal virke
      std::vector<SatFileInfo>::iterator p = subp.file.begin();
      for (; p!=subp.file.end(); p++) {
        if (ft.name == p->name) {
          newfile=false;
          //find out when this file was last updated
          unsigned long modtime = _modtime(ft.name);
          if (modtime >= subp.updateTime) {
            //special case - file has been changed since last updated
            //but read header
            //	     METLIBS_LOG_DEBUG("SPECIAL CASE"<<ft.name);
            ft.formattype= subp.formattype;
            ft.metadata = subp.metadata;
            ft.proj4string = subp.proj4string;
            ft.channelinfo = subp.channelinfo;
            ft.paletteinfo = subp.paletteinfo;
            ft.hdf5type = subp.hdf5type;
            readHeader(ft, subp.channel);
            ft.opened = true;
            //has time changed in header since last update ?
            if (ft.time != p->time) {
              //erase file, then put back in list
              subp.file.erase(p);
              newfile=true;
            }
          }
          break;
        }
      }

      if (newfile) {
        fileListChanged = true;

        //try to find time from filename
        METLIBS_LOG_DEBUG(ft.name << " " << ft.time);
        if (subp.filter[j].getTime(ft.name, ft.time) || true) {
          METLIBS_LOG_DEBUG(ft.name << " Failed");

          ft.opened = false;
        } else {
          //Open file if time not found from filename
          ft.formattype= subp.formattype;
          ft.metadata = subp.metadata;
          ft.proj4string = subp.proj4string;
          ft.channelinfo = subp.channelinfo;
          ft.paletteinfo = subp.paletteinfo;
          ft.hdf5type = subp.hdf5type;
          readHeader(ft, subp.channel);
          ft.opened=true;
        }
        METLIBS_LOG_DEBUG(ft.time << " time");

        //put it in the sorted list
        //Check if filelist is empty
        if (subp.file.empty())
          subp.file.push_back(ft);
        else {
          std::vector<SatFileInfo>::iterator p = subp.file.begin();
          while (p!=subp.file.end() && p->time>ft.time)
            p++;
          //skip archive files which are already in list
          if (!subp.archive[j] || p->time != ft.time)
            subp.file.insert(p, ft);
        }
      }
    }
  }

  //save time of last update
  subp.updateTime = miTime::secDiff(now, ztime);

  if (subp.formattype == "mitiff") {
    //update Prod[satellite][file].colours
    //Asumes that all files have same palette
    int n=subp.file.size();
    //check max 3 files,
    int i=0;
    while (i<n && i<3 && !MItiff::readMItiffPalette(subp.file[i].name.c_str(),
        subp.colours))
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
      GEOtiff::readGEOtiffPalette(subp.file[0].name.c_str(), subp.colours);
    }
  }
#endif
  //METLIBS_LOG_DEBUG(fileListChanged);
}

void SatManager::cutImageRGBA(Sat* satdata, unsigned char *image, float cut, int *index)
{
  //  * PURPOSE:   (1-cut)*#pixels should have a value between index[0] and index[1]

  int i;
  int nindexR[256], nindexG[256], nindexB[256];
  int size=satdata->nx*satdata->ny;

  for (i=0; i<256; i++) {
    nindexR[i]=0;
    nindexG[i]=0;
    nindexB[i]=0;
  }

  for (i=0; i<size; i++) {
    nindexR[(int)image[4*i]]++;
    nindexG[(int)image[4*i+1]]++;
    nindexB[(int)image[4*i+2]]++;
  }

  float npixelR=size-nindexR[0]; //number of pixels, drop index=0
  float npixelG=size-nindexG[0]; //number of pixels, drop index=0
  float npixelB=size-nindexB[0]; //number of pixels, drop index=0

  int ncutR=(int) (npixelR*cut); //number of pixels to cut   +1?
  int ncutG=(int) (npixelG*cut); //number of pixels to cut   +1?
  int ncutB=(int) (npixelB*cut); //number of pixels to cut   +1?
  int nsum; //number of pixels dropped

  // RED cut
  index[0]=1;
  index[1]=255;
  nsum=0;
  while (nsum<ncutR && index[0]<index[1]) {
    if (nindexR[index[0]] < nindexR[index[1]]) {
      nsum += nindexR[index[0]];
      index[0]++;
    } else {
      nsum += nindexR[index[1]];
      index[1]--;
    }
  }

  // GREEN cut
  index[2]=1;
  index[3]=255;
  nsum=0;
  while (nsum<ncutG && index[2]<index[3]) {
    if (nindexG[index[2]] < nindexG[index[3]]) {
      nsum += nindexG[index[2]];
      index[2]++;
    } else {
      nsum += nindexG[index[3]];
      index[3]--;
    }
  }

  // BLUE cut
  index[4]=1;
  index[5]=255;
  nsum=0;
  while (nsum<ncutB && index[4]<index[5]) {
    if (nindexB[index[4]] < nindexB[index[5]]) {
      nsum += nindexB[index[4]];
      index[4]++;
    } else {
      nsum += nindexB[index[5]];
      index[5]--;
    }
  }

  //Strengt tatt skulle den index som sist ble flyttet, blitt flyttet
  //tilbake.
}

const std::vector<SatFileInfo> &SatManager::getFiles(const std::string &satellite,
    const std::string & file, bool update)
{
  METLIBS_LOG_SCOPE();
  //check if satellite exists, (name occurs in prod)
  if (Prod.find(satellite)==Prod.end()) {
    METLIBS_LOG_ERROR("Product doesn't exist:"<<satellite);
    return emptyfile;
  }
  //check if filetype exist...
  if (Prod[satellite].find(file)==Prod[satellite].end()) {
    METLIBS_LOG_ERROR("Subproduct doesn't exist:"<<file);
    return emptyfile;
  }

  //define new struct SubprodInfo
  subProdInfo &subp = Prod[satellite][file];
  // reset flag only if update
  if (update) fileListChanged = false;
  if (update) {
    int updiff;
    //don't update whith updateFreq in archive mode
    if (useArchive) {
      updiff = updateFreq -1;
    } else {
      miTime now = miTime::nowTime();
      //time between now and last update
      updiff = miTime::secDiff(now, ztime)-subp.updateTime;
    }
    if (subp.file.size()==0 || subp.updated ==false || updiff > updateFreq) {
      //update filelist
      listFiles(subp);
      subp.updated = true;

    }
  }

  // METLIBS_LOG_DEBUG("SatManager----> getFiles:  " << fileListChanged);
  return Prod[satellite][file].file;

}

const std::vector<Colour> & SatManager::getColours(const std::string &satellite,
    const std::string & file)
{
  //Returns colour palette for this subproduct.
  return Prod[satellite][file].colours;
}

bool SatManager::isMosaic(const std::string &satellite, const std::string & file)
{
  return Prod[satellite][file].mosaic;
}

std::vector<std::string> SatManager::getCalibChannels()
{
  std::vector<std::string> channels;
  for (size_t i = 0; i < vsp.size(); i++) {
    if (vsp[i]->isEnabled())
      vsp[i]->getCalibChannels(channels); //add channels
  }
  return channels;
}

std::vector<SatValues> SatManager::showValues(float x, float y)
{
  std::vector<SatValues> satval;
  for (size_t i = 0; i < vsp.size(); i++) {
    if (vsp[i]->isEnabled()) {
      vsp[i]->values(x, y, satval);
    }
  }
  return satval;
}

std::vector<std::string> SatManager::getSatnames()
{
  std::vector<std::string> satnames;
  for (size_t j = 0; j < vsp.size(); j++) {
    std::string str;
    vsp[j]->getSatName(str);
    if (!str.empty())
      satnames.push_back(str);
  }
  return satnames;
}

void SatManager::setSatAuto(bool autoFile, const std::string& satellite,
    const std::string& file)
{
  for (size_t j = 0; j < vsp.size(); j++)
    vsp[j]->setSatAuto(autoFile, satellite, file);
}

std::vector<miTime> SatManager::getSatTimes(bool updateFileList, bool openFiles)
{
  //  * PURPOSE:   return times for list of PlotInfo's
  METLIBS_LOG_SCOPE();

  std::vector<std::string> pinfos;
  for (size_t i = 0; i < vsp.size(); i++)
    pinfos.push_back(vsp[i]->getPlotInfo());

  std::set<miTime> timeset;
  std::vector< miTime> timevec;
  int m, nn= pinfos.size();
  std::string satellite, file;

  for(int i=0; i<nn; i++) {
    const std::vector<std::string> tokens = miutil::split_protected(pinfos[i], '"', '"');
    m= tokens.size();
    if (m<3)
      continue;
    satellite= tokens[1];
    file = tokens[2];

    if (Prod.find(satellite)==Prod.end()) {
      METLIBS_LOG_ERROR("Product doesn't exist:"<<satellite);
      continue;
    }

    if (Prod[satellite].find(file)==Prod[satellite].end()) {
      METLIBS_LOG_ERROR("Subproduct doesn't exist:"<<file);
      continue;
    }

    subProdInfo &subp = Prod[satellite][file];
    if (updateFileList || subp.file.size()==0) {
      listFiles(subp);
      subp.updated = true;
    } else {
      fileListChanged = false;
    }

    if (openFiles) {
      int n=subp.file.size();
      for (int i=0; i<n; i++)
        if (!subp.file[i].opened) {
          readHeader(subp.file[i], subp.channel);
          subp.file[i].opened=true;
        }
    }

    int nf= subp.file.size();
    for (int j=0; j<nf; j++) {
      timeset.insert(subp.file[j].time);
    }

  }

  m= timeset.size();
  if (m>0) {
    std::set<miTime>::iterator p= timeset.begin();
    for (; p!=timeset.end(); p++)
      timevec.push_back(*p);
  }
  //METLIBS_LOG_DEBUG(fileListChanged);
  return timevec;
}

void SatManager::getCapabilitiesTime(std::vector<miTime>& normalTimes,
    int& timediff, const std::string& pinfo)
{
  //Finding times from pinfo
  //If pinfo contains "file=", return constTime

  timediff=0;

  std::vector<std::string> tokens= miutil::split_protected(pinfo, '"', '"');
  int m= tokens.size();
  if (m<3)
    return;

  std::string satellite= tokens[1];
  std::string file = tokens[2];
  std::string filename;

  for (unsigned int j=0; j<tokens.size(); j++) {
    std::vector<std::string> stokens= miutil::split(tokens[j], "=");
    if (stokens.size()==2 && miutil::to_lower(stokens[0])=="file") {
      filename = stokens[1];
    }
    if (stokens.size()==2 && miutil::to_lower(stokens[0])=="timediff") {
      timediff=miutil::to_int(stokens[1]);
    }
  }

  //Product with prog times
  if (filename.empty()) {
    std::vector<SatFileInfo> finfo = getFiles(satellite, file, true);
    int nfinfo=finfo.size();
    for (int k=0; k<nfinfo; k++) {
      normalTimes.push_back(finfo[k].time);
    }
  }
}


/**
 * Sets flag to update filelists for all satellites
 */
void SatManager::updateFiles()
{
  //  * PURPOSE: sets flag to update filelists for all satellites
  METLIBS_LOG_SCOPE();

  //loop over all satellites and filetypes

  std::map<std::string, std::map<std::string, subProdInfo> >::iterator p = Prod.begin();
  while (p !=Prod.end()) {
    std::map<std::string,subProdInfo>::iterator q;
    q= p->second.begin();
    while (q !=p->second.end()) {
      Prod[p->first][q->first].updated=false;
      q++;
    }
    p++;
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
      TimeFilter tf;
      // init time filter and replace yyyy etc. with *
      tf.initFilter(value);
      Prod[prod][subprod].filter.push_back(tf);
      Prod[prod][subprod].pattern.push_back(value);
      Prod[prod][subprod].archive.push_back(key == "archivefile");
      Prod[prod][subprod].channel = channels;
      Prod[prod][subprod].mosaic = mosaic;
      Prod[prod][subprod].updated = false;
      Prod[prod][subprod].archiveFiles = false;
      Prod[prod][subprod].updateTime = 0;
      Prod[prod][subprod].formattype = formattype;
      Prod[prod][subprod].metadata = metadata;
      Prod[prod][subprod].proj4string = proj4string;
      Prod[prod][subprod].channelinfo = channelinfo;
      Prod[prod][subprod].paletteinfo = paletteinfo;

      Prod[prod][subprod].hdf5type = hdf5type;
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

  // Workaround for SMHI images
  if(formattype=="hdf5")
    Dialog.cut.value=0;

  //default values in dialog to be used in batch
  Sat::setDefaultValues(Dialog);

  //read UFFDA classes
  const std::string section = "UFFDA";
  std::vector<std::string> vstr;

  if (!SetupParser::getSection(section, vstr)) {
    uffdaEnabled=false;
  } else {
    uffdaEnabled=true;
  }
  int i, n, nv, nvstr=vstr.size();
  for (nv=0; nv<nvstr; nv++) {
    std::vector<std::string> tokens = miutil::split_protected(vstr[nv], '\"', '\"', " ", true);
    n=tokens.size();
    for (i=0; i<n; i++) {
      std::vector<std::string> stokens = miutil::split_protected(tokens[i], '\"', '\"', "=", true);
      if (stokens.size()==2) {
        key=stokens[0];
        value=stokens[1];
        miutil::trim(key);
        miutil::trim(value);
        if (value[0]=='"')
          value= value.substr(1, value.length()-2);
        if (key=="class")
          vUffdaClass.push_back(value);
        else if (key=="desc")
          vUffdaClassTip.push_back(value);
        else if (key=="mailto")
          uffdaMailAddress=value;
      }
    }
  }
  return true;
}

/*********************************************************************/
bool SatManager::_isafile(const std::string name)
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

/*********************************************************************/
unsigned long SatManager::_modtime(const std::string fname)
{
  pu_struct_stat filestat;
  // first check if fname is a proper file
  int result = _filestat(fname, filestat);
  if (!result) {
    return (unsigned long)filestat.st_mtime;
  } else
    return 1;
}

/*********************************************************************/
int SatManager::_filestat(const std::string fname, pu_struct_stat& filestat)
{
  return pu_stat(fname.c_str(), &filestat);
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

const std::map<std::string, std::map<std::string,SatManager::subProdInfo> >& SatManager::getProductsInfo() const
{
  return Prod;
}
