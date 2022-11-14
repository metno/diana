/*!
 libmiRaster - met.no hdf5 interface

 Copyright (C) 2006-2022 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana@met.no

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*! Originally satimg.{cc,h} changed into satimgh5.{cc,h} in 2008
 * by Ariunaa Bertelsen, Erik Josefsson and Johan Karlsteen (SMHI)
 * RETURN VALUES:
 *-1 - Something is rotten
 * 0 - Normal and correct ending - no palette
 * 1 - Only header read
 * 2 - Normal and correct ending - palette-color image
 *
 * HDF5_head_diana reads only image head
 */

/*!
 * PURPOSE:
 * Read and decode HDF image files with satellite data on the
 * multichannel format and radar data.
 *
 * RETURN VALUES:
 * 0 - Normal and correct ending - no palette
 * 2 - Normal and correct ending - palette-color image
 *
 * NOTES:
 * Tested with MEOS MSG satellite files, NOAA satellite files
 * and radar files.
 *
 * AUTHOR: SMHI, 2008-2009
 */

#include "satimgh5.h"

#include "ImageCache.h"

#include <puTools/miStringFunctions.h>

#include <sstream>

using namespace miutil;
using namespace satimg;

// #define DEBUGPRINT
#define MILOGGER_CATEGORY "metno.satimgh5"
#include "miLogger/miLogging.h"

std::map<std::string, std::string> metno::satimgh5::hdf5map;
std::map<float, int> metno::satimgh5::paletteMap;
std::map<std::string, std::vector<float>> metno::satimgh5::calibrationTable;
std::vector<int> metno::satimgh5::RPalette;
std::vector<int> metno::satimgh5::GPalette;
std::vector<int> metno::satimgh5::BPalette;
std::map<std::string, std::string> metno::satimgh5::metadataMap;
std::map<int, std::string> metno::satimgh5::paletteStringMap;

/*!
 TODO: Should check the metadata std::string and verify the format.
 */
bool metno::satimgh5::validateChannelString(std::string& inputStr)
{
  return 1;
}

/*!
 Extracts channel information from channelinfo. The data is used to validate channel combinations.

 EXAMPLE: input:
 inputStr: 0-3A-channel3b,1-4i-channel4,2-5-channel5
 output:
 data: 3A-4i-5

 */

herr_t metno::satimgh5::getDataForChannel(std::string& inputStr, std::string& data)
{
  METLIBS_LOG_SCOPE(LOGVAL(inputStr));

  std::vector<std::string> channels, channelParts;

  if (validateChannelString(inputStr)) {
    if (inputStr.find(",") != std::string::npos) {
      channels = split(inputStr, ",", true);
    } else {
      channels.push_back(inputStr);
    }

    for (unsigned int i = 0; i < channels.size(); i++) {
      METLIBS_LOG_DEBUG(channels[i]);
      if (channels[i].find("-") != std::string::npos) {
        channelParts = miutil::split_protected(channels[i], '(', ')', "-", true);
        if (channelParts.size() == 3) {
          remove(channelParts[1], '(');
          remove(channelParts[1], ')');
          data += channelParts[1] + " ";
        }
      }
    }
  }
  return 0;
}
/*

 0-(5i-4i)-(channel5-channel4), \
    1-(4i-3Bi)-(channel4-channel3b),\
    2-4i-channel4

 0-(5i-4i)-(image5:channel5-image4:channel4), \
    1-(4i-3Bi)-(image4:channel4-image3:channel3b),\
    2-4i-image4:channel4


 0-3Bi-image3:image_data, \
 1-4i-image4:image_data, \
    2-5i-image5:image_data

 0-3Bi-image3, \
 1-4i-image4, \
    2-5i-image5


 */

herr_t metno::satimgh5::getDataForChannel(std::string inputStr, int chan, std::string& chpath, std::string& chname, bool& chinvert, bool& subtract, std::string& subchpath,
                                          std::string& subchname, bool& subchinvert, bool& ch4co2corr, bool& subch4co2corr)
{

  std::vector<std::string> channels, channelParts, channelSplit, channelSplitParts, channelNameParts, subChannelNameParts, nameSplit, nameSplitParts, subNameSplitParts;

  chinvert = false;

  replace(inputStr, " ", "");

  if (inputStr.find(",") != std::string::npos) {
    channels = split(inputStr, ",", true);
  } else {
    channels.push_back(inputStr);
  }

  for (unsigned int i = 0; i < channels.size(); i++) {
    if (channels[i].find("-") != std::string::npos) {
      channelParts = split_protected(channels[i], '(', ')', "-", true);
      if (to_int(channelParts[0], 0) == chan) {
        // check if subtract
        if (channelParts[1].find("(") != std::string::npos && channelParts[1].find(")") != std::string::npos) {
          subtract = true;
          replace(channelParts[1], "(", "");
          replace(channelParts[1], ")", "");
          channelSplit = split(channelParts[1], "-", true);

          // check invert
          if (channelSplit[0].find("i") != std::string::npos) {
            chinvert = true;
          } else {
            chinvert = false;
          }

          if (channelSplit[1].find("i") != std::string::npos) {
            subchinvert = true;
          } else {
            subchinvert = false;
          }

          // Check co2corr
          if (channelSplit[0].find("4r") != std::string::npos) {
            ch4co2corr = true;
          } else {
            ch4co2corr = false;
          }

          if (channelSplit[1].find("4r") != std::string::npos) {
            subch4co2corr = true;
          } else {
            subch4co2corr = false;
          }

        } else {
          // No substract
          subtract = false;
          subchinvert = false;

          // Check invert
          if (channelParts[1].find("i") != std::string::npos) {
            chinvert = true;
          } else {
            chinvert = false;
          }

          // Check co2corr
          if (channelParts[1].find("4r") != std::string::npos) {
            ch4co2corr = true;
          } else {
            ch4co2corr = false;
          }
        }

        // extract path and name
        if (channelParts[2].find("(") != std::string::npos && channelParts[2].find(")") != std::string::npos && subtract == true) {
          replace(channelParts[2], "(", "");
          replace(channelParts[2], ")", "");
          nameSplit = split(channelParts[2], "-", true);
          nameSplitParts = split(nameSplit[0], ":", true);
          subNameSplitParts = split(nameSplit[1], ":", true);
          chpath = nameSplitParts[0];
          chname = nameSplitParts[1];
          subchpath = subNameSplitParts[0];
          subchname = subNameSplitParts[1];

        } else if ((channelParts[2].find(":") != std::string::npos) && (subtract == false)) {
          nameSplit = split(channelParts[2], ":", true);
          // We may have more than one level of groups
          // The chpath is construted as <group1>:<group2>:...<groupn>
          // the last is the chname
          for (size_t i = 0; i < nameSplit.size(); i++) {
            if (i == nameSplit.size() - 1) {
              chname = nameSplit[i];
            } else if (i == nameSplit.size() - 1) {
              chpath += nameSplit[i];
            } else {
              chpath += nameSplit[i] + ":";
            }
          } // end for

        } else {
          chpath = "";
          chname = channelParts[2];
          subchpath = "";
          subchname = "";
        }
      }
    }
  }
  return 0;
}

/**
 * Check the type of a dataset
 */
hid_t metno::satimgh5::checkType(hid_t dataset, std::string name)
{
  hid_t dset = H5Dopen2(dataset, name.c_str(), H5P_DEFAULT);
  hid_t dtype = H5Dget_type(dset);
  hid_t result = H5Tget_class(dtype);

  H5Tclose(dtype);
  H5Dclose(dset);

  return result;
}

/**
 * Reads the imagedata of the HDF5 file with the help of metadata read in HDF5_head_diana
 */
int metno::satimgh5::HDF5_read_diana(const std::string& infile, unsigned char* image[], float* orgimage[], int nchan, int chan[], dihead& ginfo)
{
  METLIBS_LOG_TIME();

  int pal = 0;
  std::string chpath = "";
  std::string chname = "";
  bool chinvert = false;
  bool subtract = false;
  bool subch4co2corr = false;
  bool ch4co2corr = false;
  std::string subchpath = "";
  std::string subchname = "";
  bool subchinvert = false;
  H5G_stat_t statbuf;
  hid_t group;
  hid_t subgroup;
  hid_t file;
  float** float_data;
  float** float_data_sub;

  // If no channels, return
  if (nchan == 0) {
    return 1;
  }

  /* Read head and set pal
   * pal == 2 => image has palette
   * pal == 0 => image has no palette
   */
  pal = HDF5_head_diana(infile.c_str(), ginfo);

  if (pal == -1)
    return -1;

  /*
   * Memory allocated for image data in this function (*image) is freed
   * in function main process.
   */
  for (int i = 0; i < nchan; i++) {
    image[i] = new unsigned char[ginfo.xsize * ginfo.ysize];
    for (unsigned int j = 0; j < ginfo.xsize * ginfo.ysize; j++) {
      image[i][j] = 0;
    }
  }

  for (int i = 0; i < nchan; i++) {
    orgimage[i] = new float[ginfo.xsize * ginfo.ysize];
    for (unsigned int j = 0; j < ginfo.xsize * ginfo.ysize; j++) {
      orgimage[i][j] = -32000.0;
    }
  }

  /* Check if cacheFilePath is set in metadata
   * Example:
   * /tmp-cacheFilePath
   * will put temporary files in /tmp
   */
  std::string tmpFilePath = "";
  if (metadataMap.count("cacheFilePath"))
    tmpFilePath = metadataMap["cacheFilePath"];
  else {
    if (getenv("DIANA_TMP") != NULL) {
      tmpFilePath = getenv("DIANA_TMP");
    }
  }

  bool haveCachedImage = false;

  /* Check if metadata contains
   * true-cloudTopTemperature
   * in this case the cloudTopTemperature will be used and
   * orgImage will be generated
   */
  bool cloudTopTemperature = false;
  if (metadataMap.count("cloudTopTemperature"))
    cloudTopTemperature = (metadataMap["cloudTopTemperature"] == "true");

  std::string file_name = infile.substr(infile.rfind("/") + 1);
  // A hdf5 image may have more than one channel
  std::string channelinfo = ginfo.channelinfo;
  // Replace : with _
  miutil::replace(channelinfo, ':', '_');
  file_name += channelinfo;
  ImageCache* mImageCache = ImageCache::getInstance();
  /* TBD: Don't cache if orgimage, problem writing/reading float array to/from diask */
  if (ginfo.hdf5type == radar || (!cloudTopTemperature)) {
    if (mImageCache->getFromCache(file_name, (uint8_t*)image[0])) {
      haveCachedImage = true;
      /* Return if the file is a radar file or if we
       * don't need the cloud top temperature
       */
      return pal;
    }
  }

  file = H5Fopen(infile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

  // Loop through all channels
  for (int q = 0; q < nchan; q++) {
    getDataForChannel(ginfo.channelinfo, chan[q], chpath, chname, chinvert, subtract, subchpath, subchname, subchinvert, ch4co2corr, subch4co2corr);

    METLIBS_LOG_DEBUG("ginfo.channelinfo: " << ginfo.channelinfo);
    METLIBS_LOG_DEBUG("chan[q]: " << chan[q]);
    METLIBS_LOG_DEBUG("chpath: " << chpath);
    METLIBS_LOG_DEBUG("chname: " << chname);
    METLIBS_LOG_DEBUG("chinvert: " << chinvert);
    METLIBS_LOG_DEBUG("subtract: " << subtract);
    METLIBS_LOG_DEBUG("subchpath: " << subchpath);
    METLIBS_LOG_DEBUG("subchname: " << subchname);
    METLIBS_LOG_DEBUG("subchinvert: " << subchinvert);
    METLIBS_LOG_DEBUG("ginfo.xsize : " << ginfo.xsize);
    METLIBS_LOG_DEBUG("ginfo.ysize : " << ginfo.ysize);
    METLIBS_LOG_DEBUG("ch4co2corr : " << ch4co2corr);
    METLIBS_LOG_DEBUG("subch4co2corr : " << subch4co2corr);
    METLIBS_LOG_DEBUG("chpath.c_str()" << chpath);
    METLIBS_LOG_DEBUG("chname.c_str()" << chname);

    // first, we must check if channel path contains ":"
    std::vector<std::string> chpathSplit;
    if (chpath.find(":") != std::string::npos) {
      chpathSplit = split(chpath, ":", true);
    } else {
      // One level of groups
      chpathSplit.push_back(chpath);
    }
    if (chpathSplit[0].length() > 0) {
      H5Gget_objinfo(file, chpathSplit[0].c_str(), false, &statbuf);
    } else {
      H5Gget_objinfo(file, chname.c_str(), false, &statbuf);
    }

    if (statbuf.type == H5G_GROUP) {
      METLIBS_LOG_DEBUG("READ FOUND GROUP");
      group = H5Gopen2(file, chpathSplit[0].c_str(), H5P_DEFAULT);

      if (group >= 0) {
        // Check if group in groups
        if (chpathSplit.size() > 1) {
          // This should be a subgroup
          size_t j = 1;
          while ((statbuf.type == H5G_GROUP) && (j < chpathSplit.size())) {
            chpath = chpathSplit[j];
            METLIBS_LOG_DEBUG("chpath: " << chpath);
            H5Gget_objinfo(group, chpath.c_str(), false, &statbuf);
            if (statbuf.type == H5G_GROUP) {
              subgroup = H5Gopen2(group, chpath.c_str(), H5P_DEFAULT);
              if (subgroup >= 0) {
                H5Gclose(group);
                group = subgroup;
              }
              j++;
            }
          }
          // Here we should have a data set
          H5Gget_objinfo(group, chname.c_str(), false, &statbuf);
          METLIBS_LOG_DEBUG("READ FOUND DATASET INSIDE SUBGROUP");
        } else {
          H5Gget_objinfo(group, chname.c_str(), false, &statbuf);
        }

        if (statbuf.type == H5G_DATASET) {
          METLIBS_LOG_DEBUG("READ FOUND DATASET INSIDE GROUP");
          if (checkType(group, chname) == H5T_INTEGER) {
            METLIBS_LOG_DEBUG("H5T_INTEGER FOUND");
            METLIBS_LOG_DEBUG("READ FOUND DATASET INSIDE GROUP OF TYPE INTEGER");
            // Initialize array for data retrieval
            float_data = new float*[ginfo.xsize];
            float_data[0] = new float[ginfo.xsize * ginfo.ysize];

            for (unsigned int i = 1; i < ginfo.xsize; i++)
              float_data[i] = float_data[0] + i * ginfo.ysize;

            // If channel is 4r, then co2 correct it else just read it
            if (ch4co2corr)
              co2corr_bt39(ginfo, group, float_data, chinvert, q);
            else
              readDataFromDataset(ginfo, group, chpath, chname, chinvert, float_data, q, orgimage, cloudTopTemperature, haveCachedImage);

            /* If the channelinfo contains:
             * 0-(10-9)-(image10:image_data-image9:image_data)
             * 10-9 - will subtract 9 from 10.
             */
            if (subtract) {
              int firstmin = 0;
              if (hdf5map.count(std::string("min_") + from_number(q)))
                firstmin = to_int(hdf5map[std::string("min_") + from_number(q)]);
              int firstmax = 0;
              if (hdf5map.count(std::string("max_") + from_number(q)))
                firstmax = to_int(hdf5map[std::string("max_") + from_number(q)]);

              /* Save the channel path as chan_q
               * chan_q is used to extract channel specific settings from
               * metadata in makeImage.
               * Example:
               * channelinfo=0-(10-9)-(image10:image_data-image9:image_data) =>
               * image10_image_data_image9_image_data
               */
              hdf5map[std::string("chan_") + from_number(q)] = chpath + std::string("_") + chname + std::string("_") + subchpath + std::string("_") + subchname;
              H5Gclose(group);
              group = H5Gopen2(file, subchpath.c_str(), H5P_DEFAULT);
              float_data_sub = new float*[ginfo.xsize];
              float_data_sub[0] = new float[ginfo.xsize * ginfo.ysize];

              // Initialize array for second channel
              for (unsigned int i = 1; i < ginfo.xsize; i++)
                float_data_sub[i] = float_data_sub[0] + i * ginfo.ysize;

              // Same procedure as for the first channel
              if (subch4co2corr)
                co2corr_bt39(ginfo, group, float_data_sub, subchinvert, q);
              else
                readDataFromDataset(ginfo, group, subchpath, subchname, subchinvert, float_data_sub, q, orgimage, cloudTopTemperature, haveCachedImage);
              int secondmin = 0;
              if (hdf5map.count(std::string("min_") + from_number(q)))
                secondmin = to_int(hdf5map[std::string("min_") + from_number(q)]);
              int secondmax = 0;
              if (hdf5map.count(std::string("max_") + from_number(q)))
                secondmax = to_int(hdf5map[std::string("max_") + from_number(q)]);

              // Compute min/max for the subtracted picture
              hdf5map[std::string("min_") + from_number(q)] = from_number(firstmin - secondmax);
              hdf5map[std::string("max_") + from_number(q)] = from_number(firstmax - secondmin);

              // Subtract the channel, the result goes into float_data
              subtractChannels(float_data, float_data_sub);

              delete[] float_data_sub[0];
              delete[] float_data_sub;
            } else {
              /*
               * Same as above, example:
               * channelinfo=0-1-image1:image_data =>
               * hdf5map[chan_0]=image1_image_data
               */
              hdf5map[std::string("chan_") + from_number(q)] = chpath + std::string("_") + chname;
            }

            // Return the already cached image
            if (haveCachedImage) {
              METLIBS_LOG_DEBUG("Image is already cached");
            } else if (ginfo.hdf5type == radar) {
              METLIBS_LOG_DEBUG("RADAR IMAGE");
              makeImage(image, float_data);
            } else {
              METLIBS_LOG_DEBUG("SATELLITE IMAGE");
              makeImage(image, float_data, q);
            }
            delete[] float_data[0];
            delete[] float_data;

          } else if (checkType(group, chname) == H5T_FLOAT) {
            METLIBS_LOG_DEBUG("READ FOUND DATASET INSIDE GROUP OF TYPE FLOAT");
            // Initialize array
            float_data = new float*[ginfo.xsize];
            float_data[0] = new float[ginfo.xsize * ginfo.ysize];

            for (unsigned int i = 1; i < ginfo.xsize; i++)
              float_data[i] = float_data[0] + i * ginfo.ysize;

            // Read dataset, radar style
            readDataFromDataset(ginfo, group, chpath, chname, chinvert, float_data, q);

            makeImage(image, float_data);

            delete[] float_data[0];
            delete[] float_data;

          } else {
            METLIBS_LOG_WARN("UNKNOWN dataset class");
          }
        }
      }
      H5Gclose(group);
    } else if (statbuf.type == H5G_DATASET) {
      METLIBS_LOG_DEBUG("READ FOUND DATASET");
      float_data = new float*[ginfo.xsize];
      float_data[0] = new float[ginfo.xsize * ginfo.ysize];

      for (unsigned int i = 1; i < ginfo.xsize; i++)
        float_data[i] = float_data[0] + i * ginfo.ysize;

      readDataFromDataset(ginfo, file, "", chname, chinvert, float_data, q, orgimage, cloudTopTemperature, haveCachedImage);

      // Do not overwrite the image if it was cached
      if (!haveCachedImage)
        makeImage(image, float_data, q);

      delete[] float_data[0];
      delete[] float_data;
    }
  }

  H5Fclose(file);
  /* TBD: Don't cache if orgimage, problem writing/reading float array to/from diask */
  if (ginfo.hdf5type == radar || (!cloudTopTemperature)) {
    mImageCache->putInCache(file_name, (uint8_t*)image[0], ginfo.xsize * ginfo.ysize * nchan);
  }
  return pal;
}

/**
 * Subtracts all the values from int_data with the values from int_data_sub.
 * The result is placed in float_data
 */
int metno::satimgh5::subtractChannels(float* float_data[], float* float_data_sub[])
{
  int xsize = 0;
  if (hdf5map.count("xsize"))
    xsize = to_int(hdf5map["xsize"], 0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
    ysize = to_int(hdf5map["ysize"], 0);
  float nodata = -32000.0;
  if (hdf5map.count("nodata"))
    nodata = to_float(hdf5map["nodata"]);
  /*float min = 32000.0;
   float max = -32000.0;*/

  for (int i = 0; i < xsize; i++) {
    for (int j = 0; j < ysize; j++) {
      if ((float_data[i][j] == nodata) || (float_data_sub[i][j] == nodata)) {
        float_data[i][j] = -32000.0;
      } else {
        float_data[i][j] = float_data[i][j] - float_data_sub[i][j];
        /*if (float_data[i][j] < min)
         min = float_data[i][j];
         else if (float_data[i][j] > max)
         max = float_data[i][j];*/
      }
    }
  }

  /*#ifdef DEBUGPRINT
   cerr << "Submin: " << min << " Submax: " << max << endl;
   #endif
   hdf5map[std::string("submin")] = std::string(min);
   hdf5map[std::string("submax")] = std::string(max);*/

  return 0;
}

/**
 * Returns the palettestep for the specified value.
 */
int metno::satimgh5::getPaletteStep(float value)
{
  int step = 0;
  for (std::map<float, int>::iterator p = paletteMap.begin(); p != paletteMap.end(); p++) {
    if (value < p->first) {
      break;
    }
    step++;
  }
  return step;
}

/**
 * Process data from int_data and places it in channel 0 of the image array
 */
int metno::satimgh5::makeImage(unsigned char* image[], float* float_data[])
{
  METLIBS_LOG_TIME();

  int k = 0;
  //  int intToChar = 65535/255;
  int xsize = 0;
  if (hdf5map.count("xsize"))
    xsize = to_int(hdf5map["xsize"], 0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
    ysize = to_int(hdf5map["ysize"], 0);
  float gain = 1.0;
  if (hdf5map.count("gain"))
    gain = to_float(hdf5map["gain"]);
  float offset = 0;
  if (hdf5map.count("offset"))
    offset = to_float(hdf5map["offset"]);
  int nodata = 255;
  if (hdf5map.count("nodata"))
    nodata = to_int(hdf5map["nodata"], 255);
  int noofcl = 255;
  if (hdf5map.count("noofcl"))
    noofcl = to_int(hdf5map["noofcl"], 255);
  bool isBorder = false;
  if (hdf5map.count("isBorder"))
    isBorder = (hdf5map["isBorder"] == "true");
  bool isPalette = false;
  if (hdf5map.count("palette"))
    isPalette = (hdf5map["palette"] == "true");
  int borderColour = 255;
  if (hdf5map.count("border"))
    borderColour = to_int(hdf5map["border"], 255);

  if (isBorder)
    METLIBS_LOG_DEBUG("border with colour " << borderColour);
  if (isPalette)
    METLIBS_LOG_DEBUG("palette");
  METLIBS_LOG_DEBUG("making radar image: ");

  for (int i = 0; i < xsize; i++) {
    for (int j = 0; j < ysize; j++) {
      if (float_data[i][j] == nodata) {
        image[0][k] = noofcl;
      } else {
        // If the picture is a border, set the borderColour
        if (isBorder)
          image[0][k] = (float_data[i][j] > 0) ? borderColour : 0;
        // If it has palette, get the palettestep
        else if (isPalette)
          image[0][k] = getPaletteStep(((float_data[i][j]) * gain) + offset);
        // Else just set the value
        else
          image[0][k] = (int)float_data[i][j];
      }
      k++;
    }
  }
  METLIBS_LOG_DEBUG(" done!");
  return 0;
}

/*
 * CO2 correction of the MSG 3.9 um channel:
 *
 * T4_CO2corr = (BT(IR3.9)^4 + Rcorr)^0.25
 * Rcorr = BT(IR10.8)^4 - (BT(IR10.8)-dt_CO2)^4
 * dt_CO2 = (BT(IR10.8)-BT(IR13.4))/4.0
 */
int metno::satimgh5::co2corr_bt39(dihead& ginfo, hid_t source, float* ch4r[], bool chinvert, int chan)
{
  float epsilon = 0.001;
  int xsize = 0;
  if (hdf5map.count("xsize"))
    xsize = to_int(hdf5map["xsize"], 0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
    ysize = to_int(hdf5map["ysize"], 0);
  float** dt_co2;
  float** a;
  float** b;
  float** Rcorr;
  float** x;
  float** ch4;
  float** ch9;
  float** ch11;

  dt_co2 = new float*[xsize];
  a = new float*[xsize];
  b = new float*[xsize];
  Rcorr = new float*[xsize];
  x = new float*[xsize];
  ch4 = new float*[xsize];
  ch9 = new float*[xsize];
  ch11 = new float*[xsize];

  dt_co2[0] = new float[xsize * ysize];
  a[0] = new float[xsize * ysize];
  b[0] = new float[xsize * ysize];
  Rcorr[0] = new float[xsize * ysize];
  x[0] = new float[xsize * ysize];
  ch4[0] = new float[xsize * ysize];
  ch9[0] = new float[xsize * ysize];
  ch11[0] = new float[xsize * ysize];

  for (int i = 1; i < xsize; i++) {
    dt_co2[i] = dt_co2[0] + i * ysize;
    a[i] = a[0] + i * ysize;
    b[i] = b[0] + i * ysize;
    Rcorr[i] = Rcorr[0] + i * ysize;
    x[i] = x[0] + i * ysize;
    ch4[i] = ch4[0] + i * ysize;
    ch9[i] = ch9[0] + i * ysize;
    ch11[i] = ch11[0] + i * ysize;
  }

  // Hardcoded values for the channels, not good but works for now
  readDataFromDataset(ginfo, source, "image4", "image_data", false, ch4, chan, ch4, false, false);
  readDataFromDataset(ginfo, source, "image9", "image_data", false, ch9, chan, ch9, false, false);
  readDataFromDataset(ginfo, source, "image11", "image_data", false, ch11, chan, ch11, false, false);

  float min = 32000.0;
  float max = -32000.0;
  for (int i = 0; i < xsize; i++) {
    for (int j = 0; j < ysize; j++) {
      if (ch9[i][j] > 0.0) {
        dt_co2[i][j] = (ch9[i][j] - ch11[i][j]) / 4.0;
        a[i][j] = ch9[i][j] * ch9[i][j] * ch9[i][j] * ch9[i][j];
        b[i][j] = (ch9[i][j] - dt_co2[i][j]) * (ch9[i][j] - dt_co2[i][j]) * (ch9[i][j] - dt_co2[i][j]) * (ch9[i][j] - dt_co2[i][j]);
        Rcorr[i][j] = a[i][j] - b[i][j];
        a[i][j] = ch4[i][j] * ch4[i][j] * ch4[i][j] * ch4[i][j];
        if (a[i][j] + Rcorr[i][j] > 0.0)
          x[i][j] = a[i][j] + Rcorr[i][j];
        else
          x[i][j] = epsilon;
        ch4r[i][j] = pow(x[i][j], 0.25);
        if (chinvert)
          ch4r[i][j] *= -1;
        if (ch4r[i][j] > max)
          max = ch4r[i][j];
        else if (ch4r[i][j] < min)
          min = ch4r[i][j];
      } else {
        ch4r[i][j] = -32000.0;
      }
    }
  }
  hdf5map[std::string("max_") + from_number(chan)] = from_number(max);
  hdf5map[std::string("min_") + from_number(chan)] = from_number(min);

  return 0;
}

/**
 * Copies the values from float_data to orgimage
 * If there is a calibrationTable, use it for conversion.
 */
int metno::satimgh5::makeOrgImage(float* orgimage[], float* float_data[], int chan, std::string name)
{
  METLIBS_LOG_TIME();

  int k = 0;
  int xsize = 0;
  if (hdf5map.count("xsize"))
    xsize = to_int(hdf5map["xsize"], 0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
    ysize = to_int(hdf5map["ysize"], 0);
  float nodata = 0.0;
  if (hdf5map.count("nodata"))
    nodata = to_float(hdf5map["nodata"], 0.0);
  float offset = 0;
  if (hdf5map.count("offset_" + name))
    offset = to_float(hdf5map["offset_" + name], 0);
  float gain = 1;
  if (hdf5map.count("gain_" + name))
    gain = to_float(hdf5map["gain_" + name], 1);
  bool haveCalibrationTable = false;
  if (calibrationTable.count("calibration_table_" + name))
    haveCalibrationTable = (calibrationTable["calibration_table_" + name].size() > 0);
  std::vector<float> calibrationVector;
  std::string description = "";
  if (hdf5map.count("description"))
    description = hdf5map["description"];
  std::string cloudTopUnit = "";
  if (metadataMap.count("cloudTopUnit"))
    cloudTopUnit = metadataMap["cloudTopUnit"];

  METLIBS_LOG_DEBUG("making org image: " << name);
  METLIBS_LOG_DEBUG("haveCalibrationTable: " << haveCalibrationTable);
  METLIBS_LOG_DEBUG("offset: " << offset << " gain: " << gain);
  METLIBS_LOG_DEBUG("description: " << description);
  METLIBS_LOG_DEBUG("cloudTopUnit: " << cloudTopUnit);

  float add = 0.0;
  float mul = 1.0;

  // Convert if necessary
  if (description != "" && cloudTopUnit != "") {
    if (description.empty() || (description.find("eight (m)") != std::string::npos && cloudTopUnit == "ft")) {
      mul = 100 / 30.52;
    } else if ((description.find("eight (m)") != std::string::npos && cloudTopUnit == "hft")) {
      mul = 100 / 30.52 / 100.0;
    } else if (cloudTopUnit == "force_ft") {
      mul = 100 / 30.52;
    } else if (cloudTopUnit == "force_hft") {
      mul = 100 / 30.52 / 100.0;
    } else if (description.find("temperature (K)") != std::string::npos && cloudTopUnit == "C")
      add = -275.15;
  }

  if (haveCalibrationTable)
    calibrationVector = calibrationTable["calibration_table_" + name];
  for (int i = 0; i < xsize; i++) {
    for (int j = 0; j < ysize; j++) {
      if (float_data[i][j] == nodata)
        orgimage[chan][k] = -32000.0;
      else if (!haveCalibrationTable)
        orgimage[chan][k] = mul * (gain * float_data[i][j] + offset) + add;
      else
        orgimage[chan][k] = calibrationVector[(int)float_data[i][j]];
      k++;
    }
  }
  METLIBS_LOG_DEBUG(" done!");

  return 0;
}

/**
 * Process data from int_data and places it in channel chan of float_data
 */
int metno::satimgh5::makeImage(unsigned char* image[], float* float_data[], int chan)
{
  METLIBS_LOG_TIME();

  int k = 0;
  int xsize = 0;
  if (hdf5map.count("xsize"))
    xsize = to_int(hdf5map["xsize"], 0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
    ysize = to_int(hdf5map["ysize"], 0);
  float nodata = -32000.0;
  /*if (hdf5map.count("nodata"))
          nodata = to_float(hdf5map["nodata"]);*/
  bool isPalette = false;
  if (hdf5map.count("palette"))
    isPalette = (hdf5map["palette"] == "true");
  float gain = 1.0;
  if (hdf5map.count("gain"))
    gain = to_float(hdf5map["gain"]);
  float offset = 0.0;
  if (hdf5map.count("offset"))
    offset = to_float(hdf5map["offset"]);
  /* Get the channelname from hdf5map
   * This was generated earlier and looks like this:
   * image9_image_data
   * or
   * image5_image_data_image6_image_data
   */
  std::string channelName = "";
  if (hdf5map.count("chan_" + from_number(chan)))
    channelName = hdf5map["chan_" + from_number(chan)];
  /**
   * Use the channelname to extract min, max and gamma.
   * Example:
   * metadata=243-image9_image_data_min, \
   *          283-image9_image_data_max, \
   *          1.6-image9_image_data_gamma
   *
   * will give:
   * minString = 243
   * maxString = 283
   * gammaValue = 1.6
   */
  std::string minString = "";
  if (metadataMap.count(channelName + std::string("_min")))
    minString = metadataMap[channelName + std::string("_min")];
  std::string maxString = "";
  if (metadataMap.count(channelName + std::string("_max")))
    maxString = metadataMap[channelName + std::string("_max")];
  float gammaValue = 1.0;
  if (metadataMap.count(channelName + std::string("_gamma")))
    gammaValue = to_float(metadataMap[channelName + std::string("_gamma")], 1.0);

  // Replace m with - in {min,max}String.
  replace(minString, "m", "-");
  replace(maxString, "m", "-");

  // If min- or maxString contains p then count those values as percent
  bool minPercent = false;
  bool maxPercent = false;
  if (minString.find("p") != std::string::npos) {
    minPercent = true;
    replace(minString, "p", "");
  }
  if (maxString.find("p") != std::string::npos) {
    maxPercent = true;
    replace(maxString, "p", "");
  }

  // Set the lowest and highest values in the channel using
  // values from metadata or values from readDataFromDataset
  float chanMin = 0.0;
  if (hdf5map.count(std::string("min_") + from_number(chan)))
    chanMin = to_float(hdf5map[std::string("min_") + from_number(chan)]);
  float rangeMin = to_float(minString, chanMin);
  float chanMax = 0.0;
  if (hdf5map.count(std::string("max_") + from_number(chan)))
    chanMax = to_float(hdf5map[std::string("max_") + from_number(chan)]);
  float rangeMax = to_float(maxString, chanMax);
  int tmpVal;

  // Set upper/lower limit in percent
  if (minPercent)
    rangeMin = chanMin * (rangeMin * 0.01);
  if (maxPercent)
    rangeMax = chanMax * (rangeMax * 0.01);

  // stretch is used to stretch the range to 255
  float stretch = 255.0 / (rangeMax - rangeMin);

  METLIBS_LOG_DEBUG("Making channel: " << channelName);
  METLIBS_LOG_DEBUG("minPercent: " << minPercent << " maxPercent: " << maxPercent);
  METLIBS_LOG_DEBUG("chanMin: " << chanMin << " chanMax: " << chanMax);
  METLIBS_LOG_DEBUG("rangeMin: " << rangeMin << " rangeMax: " << rangeMax);
  METLIBS_LOG_DEBUG("nodata: " << nodata);
  METLIBS_LOG_DEBUG("gamma: " << gammaValue);
  METLIBS_LOG_DEBUG("stretch: " << stretch);

  // Upper/lower bound in array
  float arrmin = 32000.0;
  float arrmax = -32000.0;

  /* Loop through the array
   * stretch each pixel with this formula:
   * pixel = (oldpixel-lowerBound)*stretch
   * Then make sure all values are between 0.0001 and 1
   * (divide by 255 but set values under 0 to 0.0001)
   * Compute gamma if gammaValue != 1.0
   * Save highest and lowest values in arrmax and arrmin
   */
  if (!isPalette) {
    for (int i = 0; i < xsize; i++) {
      for (int j = 0; j < ysize; j++) {
        if (float_data[i][j] != nodata) {
          float_data[i][j] = (float_data[i][j] - rangeMin) * stretch;
          if (float_data[i][j] > 255.0)
            float_data[i][j] = 1.0;
          else if (float_data[i][j] <= 0)
            float_data[i][j] = 0.0001;
          else
            float_data[i][j] /= 255.0;
          if (gammaValue != 1.0)
            float_data[i][j] = exp(1.0 / gammaValue * log(float_data[i][j]));
          if (float_data[i][j] < arrmin)
            arrmin = float_data[i][j];
          else if (float_data[i][j] > arrmax)
            arrmax = float_data[i][j];
        }
      }
    }
    METLIBS_LOG_DEBUG("Arrmin: " << arrmin << " Arrmax: " << arrmax);
    /*
     * Loop through the array once more
     * if pixel == nodata || arrmax-arrmin<=0.0001
     *  zero out the array
     * else
     *  pixel = 255*(pixel-arrmin)/(arrmax-arrmin)
     *  make sure values are between 1 and 255 (0 is transparent)
     * put the pixel in image (Dianas picture) at chan
     */
    for (int i = 0; i < xsize; i++) {
      for (int j = 0; j < ysize; j++) {
        if (float_data[i][j] == nodata) {
          tmpVal = 0;
        } else if (isPalette) {
          tmpVal = float_data[i][j];
        } else if (arrmax - arrmin <= 0.001) {
          tmpVal = 0;
        } else {
          float_data[i][j] = (float_data[i][j] - arrmin) / (arrmax - arrmin);
          float_data[i][j] *= 255.0;
          tmpVal = (int)float_data[i][j];
          if (tmpVal > 255)
            tmpVal = 255;
          else if (tmpVal < 1)
            tmpVal = 1;
        }
        image[chan][k] = tmpVal;
        k++;
      }
    }
  } else {
    for (int i = 0; i < xsize; i++) {
      for (int j = 0; j < ysize; j++) {
        if (float_data[i][j] == nodata)
          image[chan][k++] = 0;
        else
          image[chan][k++] = float_data[i][j];
      }
    }
  }
  METLIBS_LOG_DEBUG(" done!");
  return 0;
}

/**
 * Read the data part of the channel in the HDF5 file.
 * The data is processed and put in the int_data[] array.
 * orgimage is filled with unprocessed data.
 */
int metno::satimgh5::readDataFromDataset(dihead& ginfo, hid_t source, std::string path, std::string name, bool invert, float** float_data, int chan, float* orgImage[],
                                         bool cloudTopTemperature, bool haveCachedImage)
{
  METLIBS_LOG_TIME();

  float min = -32768.0;
  float max = 32768.0;

  hid_t dset;
  int daynight = day_night(ginfo);
  bool skip = (name.find("1") != std::string::npos || name.find("2") != std::string::npos);
  std::string pathname;
  float nodata = 0.0;
  if (hdf5map.count("nodata"))
    nodata = to_float(hdf5map["nodata"], 0.0);
  // Is this correct...
  float novalue = -32001.0;
  if (hdf5map.count("novalue"))
    novalue = to_float(hdf5map["novalue"], -32001.0);
  if (hdf5map.count("undetect"))
    novalue = to_float(hdf5map["undetect"], -32001.0);

  if (ginfo.hdf5type == radar)
    min = 0;
  if (path == "") {
    pathname = name;
  } else {
    pathname = path;
  }
  int invertValue = 1;
  if (invert)
    invertValue = -1;

  /* If a calibration table has been read, example:
   * metadata=calibration_table_image7-image7:calibration:calibration_table
   * This will read image7:calibration:calibration_table into
   * calibrationTable[calibration_table_image7]
   * This vectior is used to convert values to Kelvin
   */
  bool haveCalibrationTable = false;
  if (calibrationTable.count("calibration_table_" + pathname))
    haveCalibrationTable = (calibrationTable["calibration_table_" + pathname].size() > 0);
  std::vector<float> calibrationVector;
  if (haveCalibrationTable)
    calibrationVector = calibrationTable["calibration_table_" + pathname];

  /* If a color range has been read, example:
   * metadata=color_range_image7-visualization7:color_range
   * This will read image7:calibration:calibration_table into
   * calibrationTable[color_range_image7]
   * This vectior is used to convert values to rgb values
   */
  bool haveColorRange = false;
  if (calibrationTable.count("color_range_" + pathname))
    haveColorRange = (calibrationTable["color_range_" + pathname].size() > 0);
  std::vector<float> color_range;
  if (haveColorRange)
    color_range = calibrationTable["color_range_" + pathname];

  /* If a color range has been read, example:
   * metadata=color_range_image7-visualization7:color_range
   * This will read image7:calibration:calibration_table into
   * calibrationTable[color_range_image7]
   * This vectior is used to convert values to rgb values
   */
  bool havePalette = ((ginfo.metadata.find("RGBPalette-") != std::string::npos) && (paletteStringMap.size() == 0));
  std::vector<int> palette;
  if (havePalette) {
    if (chan == 0)
      palette = RPalette;
    else if (chan == 1)
      palette = GPalette;
    else
      palette = BPalette;
  }

  /*
   * Get statmin_ and statmax_ from file, if set:
   * statmin_image9-image9:statistics:stat_min_value, \
   * statmax_image9-image9:statistics:stat_max_value
   * MEOS MSG specific
   */
  if (hdf5map.count(std::string("statmin_") + pathname)) {
    if (hdf5map[std::string("statmin_") + pathname].length() > 0) {
      if (invert) {
        max = -1 * to_int(hdf5map[std::string("statmin_") + pathname]);
      } else {
        min = to_int(hdf5map[std::string("statmin_") + pathname]);
      }
    }
  }
  if (hdf5map.count(std::string("statmax_") + pathname)) {
    if (hdf5map[std::string("statmax_") + pathname].length() > 0) {
      if (invert) {
        min = -1 * to_int(hdf5map[std::string("statmax_") + pathname]);
      } else {
        max = to_int(hdf5map[std::string("statmax_") + pathname]);
      }
    }
  }

  METLIBS_LOG_DEBUG("INTEGER");
  METLIBS_LOG_DEBUG("ginfo.xsize: " << ginfo.xsize);
  METLIBS_LOG_DEBUG("ginfo.ysize: " << ginfo.ysize);
  METLIBS_LOG_DEBUG("map xsize: " << to_int(hdf5map["xsize"], 0));
  METLIBS_LOG_DEBUG("map ysize: " << to_int(hdf5map["ysize"], 0));
  METLIBS_LOG_DEBUG("daynight: " << daynight);
  METLIBS_LOG_DEBUG("skip: " << skip);
  METLIBS_LOG_DEBUG("name: " << name);
  METLIBS_LOG_DEBUG("cloudTopTemperature: " << cloudTopTemperature);
  METLIBS_LOG_DEBUG("pathname: " << pathname);
  METLIBS_LOG_DEBUG("haveColorRange: " << haveColorRange);
  METLIBS_LOG_DEBUG("haveCalibrationTable: " << haveCalibrationTable);
  METLIBS_LOG_DEBUG("havePalette: " << havePalette);
  METLIBS_LOG_DEBUG("haveCachedImage: " << haveCachedImage);
  METLIBS_LOG_DEBUG("max: " << max);
  METLIBS_LOG_DEBUG("min: " << min);
  METLIBS_LOG_DEBUG("reading array: ");

  // Open the dataset
  dset = H5Dopen2(source, name.c_str(), H5P_DEFAULT);
  int** int_data;
  int_data = new int*[ginfo.xsize];
  int_data[0] = new int[ginfo.xsize * ginfo.ysize];

  for (size_t i = 1; i < ginfo.xsize; i++)
    int_data[i] = int_data[0] + i * ginfo.ysize;

  // Extract the data from the HDF5 file
  H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, int_data[0]);

  // Move the data to a float array for precision
  for (size_t i = 0; i < ginfo.xsize; i++) {
    for (size_t j = 0; j < ginfo.ysize; j++) {
      float_data[i][j] = int_data[i][j];
    }
  }

  delete[] int_data[0];
  delete[] int_data;

  // Save the original data to show cloud top temperature
  if (cloudTopTemperature)
    makeOrgImage(orgImage, float_data, chan, pathname);

  // If the image is already cached, return
  if (haveCachedImage) {
    H5Dclose(dset);
    return 0;
  }

  METLIBS_LOG_DEBUG("computing: ");

  /*
   * Fill the holes in the color range to have a complete
   * lookup table to convert value in HDF5 file to the RGB
   * value. This is MEOS MSG specific.
   */
  std::vector<float> lookupTable;
  if (haveColorRange) {
    float value = color_range[0];
    for (size_t i = 0; i < color_range.size(); i++) {
      if (color_range[i] > 6000)
        break;
      lookupTable.push_back(color_range[i]);
      for (; value < color_range[i]; value++)
        lookupTable.push_back(color_range[i]);
    }
    /*for(int i=0;i<lookupTable.size();i++) {
     cerr << "key: " << i<< " value: " << lookupTable[i] << endl;
     }*/
  }

  float msgMax = -32000.0;
  float msgMin = 32000.0;
  for (size_t i = 0; i < ginfo.xsize; i++) {
    for (size_t j = 0; j < ginfo.ysize; j++) {
      if (max - min < 1.0) {
        float_data[i][j] = -32000.0;
        continue;
      }
      if ((ginfo.hdf5type != radar) &&
          ((float_data[i][j] == nodata) || (float_data[i][j] == novalue) || ((ginfo.hdf5type == noaa) && (daynight == 1) && skip))) {
        float_data[i][j] = -32000.0;
        continue;
      } else if (haveColorRange) {
        if (invert)
          float_data[i][j] = 255 - lookupTable[(int)float_data[i][j]];
        else
          float_data[i][j] = lookupTable[(int)float_data[i][j]];
      } else if (haveCalibrationTable) {
        float_data[i][j] = calibrationVector[(int)float_data[i][j]] * invertValue;
      } else if (havePalette) {
        if (invert)
          float_data[i][j] = 255 - palette[(int)float_data[i][j]];
        else
          float_data[i][j] = palette[(int)float_data[i][j]];
      } else if (ginfo.hdf5type == saf) {
        if (invert)
          float_data[i][j] = 255 - float_data[i][j];
        else
          float_data[i][j] = float_data[i][j];
      } else if (float_data[i][j] > nodata) {
        float_data[i][j] *= invertValue;
      }
      if (float_data[i][j] > msgMax)
        msgMax = float_data[i][j];
      else if (float_data[i][j] < msgMin)
        msgMin = float_data[i][j];
    }
  }

  /*
   * For MSG type, compute new min/max for array
   */
  if (ginfo.hdf5type == msg && (!haveColorRange)) {
    // At night this happens to VIS channels
    if ((msgMax - msgMin) < 1.0) {
      min = 0.0;
      max = 0.0;
    } else {
      min = msgMin;
      max = msgMax;
    }

    METLIBS_LOG_DEBUG("msgMax: " << msgMax);
    METLIBS_LOG_DEBUG("msgMin: " << msgMin);

  } else if (haveColorRange || havePalette || ginfo.hdf5type == saf) {
    min = 0.0;
    max = 255.0;
  }

  // Put min/max for array in hdf5map.
  // These are used in makeImage
  hdf5map[std::string("max_") + from_number(chan)] = from_number(max);
  hdf5map[std::string("min_") + from_number(chan)] = from_number(min);

  H5Dclose(dset);
  return 0;
}

/**
 * Without original image for radar images
 */
int metno::satimgh5::readDataFromDataset(dihead& ginfo, hid_t source, std::string path, std::string name, bool invert, float** float_data, int chan)
{
  METLIBS_LOG_SCOPE();

  hid_t dset;
  herr_t status;

  dset = H5Dopen2(source, name.c_str(), H5P_DEFAULT);
  METLIBS_LOG_DEBUG("reading float array: ");

  status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, float_data[chan]);

  METLIBS_LOG_DEBUG("status: " << status);

  status = H5Dclose(dset);
  return 0;
}

/**
 * If an attribute is found in the current block it is written to the internal map structure.
 */
int metno::satimgh5::getAttributeFromGroup(hid_t& dataset, int index, std::string path)
{

  METLIBS_LOG_SCOPE();

  hid_t native_type, memb_id, space;
  hsize_t dims[1];
  H5T_class_t memb_cls;
  hid_t memtype;
  size_t size;
  hid_t palette_type;
  outval_name* palette;
  int nmembs;
  hid_t attr, atype, aspace;
  char memb[1024];
  // NOTE: sizeof(memb) -1)
  hsize_t buf = sizeof(memb) - 1;
  herr_t status = 0;
  float* float_array;
  int* int_array;
  size_t npoints = 0;
  herr_t ret = 0;
  std::string value = "";
  H5T_class_t dclass;
  // NOTE: Must be large, H5Aread used and no control of overflow!
  char string_value[10240];
  char* tmpdataset;
  std::vector<std::string> splitString;

  attr = H5Aopen_idx(dataset, index);
  atype = H5Aget_type(attr);
  aspace = H5Aget_space(attr);
  status = H5Aget_name(attr, buf, memb);
  METLIBS_LOG_DEBUG("status: " << status);
  dclass = H5Tget_class(atype);
  npoints = H5Sget_simple_extent_npoints(aspace);

  if (path != "")
    path += ":";

  if (status != 0)
    switch (dclass) {
    case H5T_INTEGER:
      int_array = new int[(int)npoints];
      ret = H5Aread(attr, H5T_NATIVE_INT, int_array);

      if (!ret) {
        for (int i = 0; i < (int)npoints; i++) {
          value += from_number(int_array[i]);
        }
      } else {
        METLIBS_LOG_DEBUG("ret: " << ret);
      }
      delete[] int_array;
      break;

    case H5T_STRING:
      ret = H5Aread(attr, atype, string_value);

      if (!ret) {
        value = std::string(string_value);
      }
      break;

    case H5T_FLOAT:
      float_array = new float[(int)npoints];
      ret = H5Aread(attr, H5T_NATIVE_FLOAT, float_array);

      if (!ret) {
        if (npoints > 1) {
          for (int i = 0; i < (int)npoints; i++) {
            value += from_number(float_array[i]);
            insertIntoValueMap(path + std::string(memb) + "[" + from_number(i) + "]", from_number(float_array[i]));
          }
        } else {
          value = from_number(float_array[0]);
        }
      } else {
        METLIBS_LOG_DEBUG("ret: " << ret);
      }

      delete[] float_array;
      break;

    case H5T_ARRAY:
      METLIBS_LOG_DEBUG("ARRAY FOUND IN GROUP");
      break;

    case H5T_COMPOUND:
      METLIBS_LOG_DEBUG("COMPOUND FOUND IN GROUP");
      status = H5Aget_name(attr, buf, memb);
      METLIBS_LOG_DEBUG("status: " << status);
      native_type = H5Tget_native_type(atype, H5T_DIR_DEFAULT);
      nmembs = H5Tget_nmembers(native_type);
      // Memory leak...
      H5Aclose(attr);
      attr = H5Aopen_name(dataset, memb);
      for (int j = 0; j < nmembs; j++) {
        memb_cls = H5Tget_member_class(native_type, j);
        tmpdataset = H5Tget_member_name(native_type, j);
        memb_id = H5Tget_member_type(native_type, j);

        if (memb_cls == H5T_STRING) {
          /*
           * Get dataspace and allocate memory for read buffer.  This is a
           * three dimensional attribute when the array datatype is included
           * so the dynamic allocation must be done in steps.
           */
          size = H5Tget_size(memb_id);
          space = H5Aget_space(attr);
          /*ndims=*/H5Sget_simple_extent_dims(space, dims, NULL);

          size++; /* Make room for null terminator */

          /*
           * Create the memory datatype.
           */
          memtype = H5Tcopy(H5T_C_S1);
          status = H5Tset_size(memtype, size);

          /*
           * Read the data.
           */
          // We MUST be sure that only the "correct" palette strings is read
          // cerr << "path: " << path << endl;

          palette_type = H5Tcreate(H5T_COMPOUND, sizeof(outval_name));
          status = H5Tinsert(palette_type, tmpdataset, HOFFSET(outval_name, str), memtype);

          palette = new outval_name[dims[0]];

          /*for (i=0; i<dims[0]; i++)
          for (j=0; j<1024; j++)
          palette[i].str[j] = (char)0;*/

          status = H5Aread(attr, palette_type, palette);

          bool addPalette = false;
          for (std::map<std::string, std::string>::iterator p = metadataMap.begin(); p != metadataMap.end(); p++) {
            if (!p->first.empty()) {
              if (p->first.find(path) != std::string::npos) {
                addPalette = true;
              }
            }
          }

          // If the image contains more than one palette, add only the wanted palette

          if (addPalette) {
            for (size_t i = 0; i < dims[0]; i++) {
              splitString = split(std::string(palette[i].str), ":", true);
              if (splitString.size() == 2) {
                paletteStringMap[to_int(splitString[0], 0)] = splitString[1];
                METLIBS_LOG_DEBUG("splitString.size()==2 "
                                  << "i =  " << i << " " << splitString[0] << " " << splitString[1]);
              } else if (splitString.size() == 3) {
                trim(splitString[1]);
                if (paletteStringMap.count(to_int(splitString[1], 0)) == 0)
                  paletteStringMap[to_int(splitString[1], 0)] = splitString[2];
                METLIBS_LOG_DEBUG("splitString.size()==3. "
                                  << "i =  " << i << " " << splitString[1] << " " << splitString[2]);
              }
            }
          }
          delete[] palette;
        }

        free(tmpdataset);
      }

      break;

    default:
      METLIBS_LOG_DEBUG("metno::satimgh5::getAttributeFromGroup UNKNOWN found. ");
      METLIBS_LOG_DEBUG("dclass: " << dclass);
      break;
    }

  insertIntoValueMap(path + std::string(memb), value);

  H5Sclose(aspace);
  H5Tclose(atype);
  H5Aclose(attr);
  return 0;
}

/**
 * If a dataset is found the metadata part of the dataset is read. Support for compound datasets is included.
 */
int metno::satimgh5::openDataset(hid_t root, std::string dataset, std::string path, std::string metadata)
{
  METLIBS_LOG_SCOPE("dataset: " << dataset << " path: " << path);
  hid_t dset;
  hid_t dtype;
  H5T_class_t dclass;
  hid_t native_type;
  int nmembs;
  H5T_class_t memb_cls;
  char* memb;
  hid_t comp;
  herr_t status;
  float fary[10];
  int iary[10];
  int nrAttrs = 0;
  hid_t memb_id;
  hid_t s3_tid;
  myString s3[1024];
  hid_t stype;
  size_t size;
  hid_t stid;
  hsize_t dims[25];
  hid_t memb_super_id;
  hid_t space;
  arrFloat* rdata;
  hid_t floattype;

  dset = H5Dopen2(root, dataset.c_str(), H5P_DEFAULT);
  dtype = H5Dget_type(dset);
  dclass = H5Tget_class(dtype);
  native_type = H5Tget_native_type(dtype, H5T_DIR_DEFAULT);
  std::vector<std::string> sPath = split(path, ":", true);
  std::string dianaPath = metadataMap[path];

  switch (dclass) {
  case H5T_COMPOUND:
    METLIBS_LOG_DEBUG("dataset: " << dataset << " is H5T_COMPOUND");
    if (path != "")
      path += ":";
    nmembs = H5Tget_nmembers(native_type);
    for (int j = 0; j < nmembs; j++) {
      memb_cls = H5Tget_member_class(native_type, j);
      memb = H5Tget_member_name(native_type, j);
      memb_id = H5Tget_member_type(native_type, j);

      switch (memb_cls) {
      case H5T_ARRAY:
        memb_super_id = H5Tget_super(memb_id);
        METLIBS_LOG_DEBUG("ARRAY FOUND");

        if ((H5Tequal(memb_super_id, H5T_IEEE_F64LE) > 0) || (H5Tequal(memb_super_id, H5T_IEEE_F32LE) > 0)) {
          METLIBS_LOG_DEBUG("FLOAT ARRAY FOUND");
          // if (H5Tequal(memb_super_id, H5T_FLOAT) > 0) {
          floattype = H5Tcreate(H5T_COMPOUND, sizeof(arrFloat));
          status = H5Tinsert(floattype, memb, HOFFSET(arrFloat, f), memb_id);
          space = H5Dget_space(dset);
          rdata = new arrFloat[1024];
          status = H5Dread(dset, floattype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rdata);
          H5Tget_array_dims2(memb_id, dims);

          for (size_t j = 0; j < dims[0]; j++) {
            insertIntoValueMap("compound:" + path + "ARRAY[" + miutil::from_number(int(j)) + "]:" + std::string(memb), from_number(rdata[0].f[j], 20));
          }

          status = H5Dvlen_reclaim(floattype, space, H5P_DEFAULT, rdata);

          delete[] rdata;
          H5Sclose(space);
          H5Tclose(floattype);
        }
        break;

      case H5T_INTEGER:
        METLIBS_LOG_DEBUG("array is H5T_INTEGER");
        comp = H5Tcreate(H5T_COMPOUND, sizeof(int));
        status = H5Tinsert(comp, memb, 0, H5T_NATIVE_INT);
        status = H5Dread(dset, comp, H5S_ALL, H5S_ALL, H5P_DEFAULT, iary);
        status = H5Tclose(comp);
        insertIntoValueMap("compound:" + path + std::string(memb), from_number(iary[0]));
        break;

      case H5T_FLOAT:
        comp = H5Tcreate(H5T_COMPOUND, sizeof(float));
        status = H5Tinsert(comp, memb, 0, H5T_NATIVE_FLOAT);
        status = H5Dread(dset, comp, H5S_ALL, H5S_ALL, H5P_DEFAULT, fary);
        status = H5Tclose(comp);
        insertIntoValueMap("compound:" + path + std::string(memb), from_number(fary[0]));
        break;

      default:
        stid = H5Tcopy(H5T_C_S1);
        size = H5Tget_size(memb_id);
        status = H5Tset_size(stid, size);
        stype = H5Tcopy(H5T_C_S1);
        status = H5Tset_size(stype, 1024);
        s3_tid = H5Tcreate(H5T_COMPOUND, sizeof(myString));
        status = H5Tinsert(s3_tid, memb, HOFFSET(myString, str), stype);
        status = H5Dread(dset, s3_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, s3);

        status = H5Tclose(s3_tid);
        status = H5Tclose(stype);
        status = H5Tclose(stid);

        insertIntoValueMap("compound:" + path + std::string(memb), std::string(s3[0].str));
        break;
      }
      free(memb);
    }
    break;

  case H5T_FLOAT: {
    hid_t space = H5Dget_space(dset);
    int ndims = H5Sget_simple_extent_dims(space, dims, NULL);
    if (ndims == 2) {
      float* float_data;
      float_data = new float[dims[0] * dims[1]];
      for (size_t j = 0; j < dims[0] * dims[1]; j++) {
        float_data[j] = 0;
      }
      for (size_t i = 1; i < dims[0]; i++)
        float_data[i] = float_data[0] + i * dims[1];
      status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, float_data);
      for (size_t i = 0; i < dims[0] * dims[1]; i = i + 2)
        calibrationTable[dianaPath].push_back(float_data[i + 1]);
      delete[] float_data;
    }
    nrAttrs = H5Aget_num_attrs(dset);

    for (int k = 0; k < nrAttrs; k++) {
      getAttributeFromGroup(dset, k, path);
    }
  } break;

  case H5T_INTEGER: {
    hid_t space = H5Dget_space(dset);
    int ndims = H5Sget_simple_extent_dims(space, dims, NULL);
    if (ndims == 2) {
      int* int_data;
      int_data = new int[dims[0] * dims[1]];
      for (size_t j = 0; j < dims[0] * dims[1]; j++) {
        int_data[j] = 0;
      }
      for (size_t i = 1; i < dims[0]; i++)
        int_data[i] = int_data[0] + i * dims[1];
      status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, int_data);
      if (dims[1] == 2)
        for (size_t i = 0; i < dims[0] * dims[1]; i = i + 2)
          calibrationTable[dianaPath].push_back((float)int_data[i + 1]);
      else if (dims[1] == 3) {
        // Add only to palette if path in metadata
        // cerr << "addPalette: " << path << endl;
        bool addPalette = false;
        for (std::map<std::string, std::string>::iterator p = metadataMap.begin(); p != metadataMap.end(); p++) {
          if (!p->first.empty()) {
            // cerr << "key: " << p->first << " value: " << p-> second << endl;
            if (p->first.find(path) != std::string::npos) {
              if (p->second == "RGBPalette") {
                addPalette = true;
              }
            }
          }
        }

        // If the image contains more than one palette, add only the wanted palette

        if (addPalette) {
          // cerr << "dims[0]: " << dims[0] << " dims[1]: " << dims[1] << endl;
          for (size_t i = 0; i < dims[0] * dims[1]; i = i + 3) {
            // Put the values in the palettemaps
            // cerr << int_data[i] << "," << int_data[i+1] << "," << int_data[i+2] << endl;
            RPalette.push_back(int_data[i]);
            GPalette.push_back(int_data[i + 1]);
            BPalette.push_back(int_data[i + 2]);
          }
        }
      }
      delete[] int_data;
    } else if (ndims == 1) {
      int* int_data;
      int_data = new int[dims[0]];
      for (size_t j = 0; j < dims[0]; j++) {
        int_data[j] = j;
      }
      status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, int_data);
      for (size_t i = 0; i < dims[0]; i++)
        calibrationTable[dianaPath].push_back(int_data[i]);
      delete[] int_data;
    }
    nrAttrs = H5Aget_num_attrs(dset);

    for (int k = 0; k < nrAttrs; k++) {
      getAttributeFromGroup(dset, k, path);
    }
  } break;
  }

  status = H5Tclose(native_type);
  status = H5Tclose(dtype);
  status = H5Dclose(dset);
  return 0;
}

/**
 * If a group is found in the current block it is opened by this function that calls itself recursively to traverse subgroups.
 * This is the starting point for traversing a HDF5 file
 */
int metno::satimgh5::openGroup(hid_t group, std::string groupname, std::string path, std::string metadata)
{

  METLIBS_LOG_SCOPE("group: " << group << " groupname: " << groupname << " path: " << path << " metadata: " << metadata);

  char tmpdataset[80];
  hsize_t nrObj = 0;
  int nrAttrs = 0;
  int obj_type = 0;
  H5G_stat_t statbuf;
  std::string delimiter;

  H5Gget_objinfo(group, groupname.c_str(), false, &statbuf);
  if (statbuf.type == H5G_GROUP) {
    METLIBS_LOG_DEBUG("group: " << groupname << " is H5G_GROUP");
    hid_t root = H5Gopen2(group, groupname.c_str(), H5P_DEFAULT);
    H5Gget_num_objs(root, &nrObj);
    nrAttrs = H5Aget_num_attrs(root);
    METLIBS_LOG_DEBUG("group: " << groupname << " nrObj " << nrObj << " nrAttrs " << nrAttrs);
    for (int k = 0; k < nrAttrs; k++) {
      getAttributeFromGroup(root, k, path);
    }

    for (int i = 0; i < (int)nrObj; i++) {
      obj_type = H5Gget_objtype_by_idx(root, (hsize_t)i);
      if (obj_type == H5G_GROUP) {
        H5Gget_objname_by_idx(root, (hsize_t)i, tmpdataset, 80);
        if (path.length() > 0)
          delimiter = ":";
        else
          delimiter = "";
        openGroup(root, tmpdataset, path + delimiter + std::string(tmpdataset), metadata);
      } else if (obj_type == H5G_DATASET) {
        H5Gget_objname_by_idx(root, (hsize_t)i, tmpdataset, 80);
        if (path.length() > 0)
          delimiter = ":";
        else
          delimiter = "";
        openDataset(root, tmpdataset, path + delimiter + std::string(tmpdataset), metadata);
      }
    }
    H5Gclose(root);
  } else if (statbuf.type == H5G_DATASET) {
    METLIBS_LOG_DEBUG("group: " << groupname << " is H5G_DATASET");
    openDataset(group, groupname, path, metadata);
  }
  return 0;
}

/**
 * Simple insert to the internal map structure.
 */
int metno::satimgh5::insertIntoValueMap(std::string fullpath, std::string value)
{
  METLIBS_LOG_SCOPE("fullpath: " << fullpath << " value: " << value);
  // Special case for product
  if (fullpath.find("product") != std::string::npos)
    replace(value, " ", "_");
  hdf5map[fullpath] = value;
  return 0;
}

/**
 * Dump the values of the internal map structures to console.
 */
int metno::satimgh5::getAllValuesFromMap()
{
  METLIBS_LOG_SCOPE();
  for (std::map<std::string, std::string>::iterator p = hdf5map.begin(); p != hdf5map.end(); p++) {
    METLIBS_LOG_ERROR("Path: " << p->first << " Value: " << p->second);
  }
  if (calibrationTable.size() > 0) {
    for (std::map<std::string, std::vector<float>>::iterator p = calibrationTable.begin(); p != calibrationTable.end(); p++) {
      if (!p->first.empty()) {
        for (size_t i = 0; i < p->second.size(); i++) {
          METLIBS_LOG_ERROR("Path: " << p->first << " Value: " << i << " Value: " << p->second[i]);
        }
      }
    }
  }
  for (size_t i = 0; i < RPalette.size(); i++) {
    if (RPalette[i] != 0)
      METLIBS_LOG_ERROR("RPalette[" << i << "]: " << RPalette[i]);
  }
  for (size_t i = 0; i < GPalette.size(); i++) {
    if (GPalette[i] != 0)
      METLIBS_LOG_ERROR("GPalette[" << i << "]: " << GPalette[i]);
  }
  for (size_t i = 0; i < BPalette.size(); i++) {
    if (BPalette[i] != 0)
      METLIBS_LOG_ERROR("BPalette[" << i << "]: " << BPalette[i]);
  }
  return 0;
}

/**
 * Returns true if the metadata in the file is read, false otherwise
 *
 */
bool metno::satimgh5::checkMetadata(std::string filename, std::string metadata)
{
  if (!(hdf5map["filename"] == filename) || !(hdf5map["metadata"] == metadata)) {
    hdf5map.clear();
    calibrationTable.clear();
    paletteStringMap.clear();
    RPalette.clear();
    GPalette.clear();
    BPalette.clear();
    metadataMap.clear();
    hdf5map["filename"] = filename;
    hdf5map["metadata"] = metadata;
    const std::vector<std::string> metadataVector = split(metadata, ",", true);
    for (size_t i = 0; i < metadataVector.size(); i++) {
      std::vector<std::string> metadataRows = split(metadataVector[i], "-", true);
      if (metadataRows.size() == 2)
        metadataMap[metadataRows[1]] = metadataRows[0];
    }
    return false;
  }
  return true;
}

/**
 * Fills the internal structure dihead with metadata from the HDF5 metadata.
 * metadata in diana.setup is used as well
 */
herr_t metno::satimgh5::fill_head_diana(std::string inputStr, int chan)
{
  METLIBS_LOG_SCOPE("inputStr:" << inputStr << " chan: " << chan);

  /*
   From configfile:
   diana_xscale-compound:region:xscale,diana_yscale-compound:region:yscale,diana_xsize-compound:region:xsize,diana_ysize-compound:region:ysize

   In map:
   compound:region:xscale 1000
   compound:region:yscale 1000
   compound:region:xsize 1000
   compound:region:ysize 1000
   */

  std::vector<std::string> input;
  std::vector<std::string> inputPart;
  std::string value;
  /*  bool startDateSet = false;
  bool startTimeSet = false;
  bool endDateSet = false;
  bool endTimeSet = false;
  bool dateSet = false;
  bool timeSet = false;
  int ret = 0;
  */

  // Break metadata into pieces and insert it into hdf5map
  replace(inputStr, " ", "");
  input = split(inputStr, ",", true);

  for (size_t i = 0; i < input.size(); i++) {
    inputPart = split(input[i], "-", true);
    if (inputPart.size() > 0) {
      hdf5map[inputPart[0]] = hdf5map[inputPart[1]];
    }
  }

  if ((hdf5map["date"].length() > 0) && (hdf5map["time"].length() > 0)) {
    hdf5map["dateTime"] = miTime(hdf5map["date"] + hdf5map["time"]).isoTime();
  } else if (hdf5map["utime"].length() > 0) {
    time_t utime = to_int(hdf5map["utime"]);
    hdf5map["dateTime"] = miTime(utime).isoTime();
  } else if (hdf5map["date_alpha_month"].length() > 0) {
    hdf5map["dateTime"] = miTime(convertAlphaDate(hdf5map["date_alpha_month"])).isoTime();
  } else if (hdf5map["filename"].length() > 0) {
    // hdf5map["dateTime"] = miTime(hdf5map["filename"].split("_",true)[1]).isoTime();
  }
  METLIBS_LOG_DEBUG(" Datetime: " << hdf5map["dateTime"]);

  // extract proj
  if (hdf5map["projdef"].length() > 0) {
    std::string projdef = hdf5map["projdef"];

    replace(projdef, "+", "");
    replace(projdef, ",", " ");
    hdf5map["projdef"] = projdef;

    std::vector<std::string> proj = split(projdef, " ", true);

    for (unsigned int i = 0; i < proj.size(); i++) {
      std::vector<std::string> projParts = split(proj[i], "=", true);
      hdf5map[projParts[0]] = projParts[1];
    }
  }

  if (hdf5map["projdef"].find("+") == std::string::npos) {
    hdf5map["projdef"] = std::string("+") + hdf5map["projdef"];
    replace(hdf5map["projdef"], " ", " +");
  }

  return 1;
}

/**
 * Converts datestrings with alpha months (JAN,FEB,...)
 * this is necessary since MEOS MSG files use JAN instead of 01 etc
 */
std::string metno::satimgh5::convertAlphaDate(std::string date)
{

  if (date.find("JAN") != std::string::npos)
    replace(date, "JAN", "01");
  else if (date.find("FEB") != std::string::npos)
    replace(date, "FEB", "02");
  else if (date.find("MAR") != std::string::npos)
    replace(date, "MAR", "03");
  else if (date.find("APR") != std::string::npos)
    replace(date, "APR", "04");
  else if (date.find("MAY") != std::string::npos)
    replace(date, "MAY", "05");
  else if (date.find("JUN") != std::string::npos)
    replace(date, "JUN", "06");
  else if (date.find("JUL") != std::string::npos)
    replace(date, "JUL", "07");
  else if (date.find("AUG") != std::string::npos)
    replace(date, "AUG", "08");
  else if (date.find("SEP") != std::string::npos)
    replace(date, "SEP", "09");
  else if (date.find("OCT") != std::string::npos)
    replace(date, "OCT", "10");
  else if (date.find("NOV") != std::string::npos)
    replace(date, "NOV", "11");
  else if (date.find("DEC") != std::string::npos)
    replace(date, "DEC", "12");

  std::vector<std::string> tmpdate = split(date, " ", true);

  if (tmpdate.size() == 2) {
    std::vector<std::string> tmpdateparts = split(tmpdate[0], "-", true);
    date = tmpdateparts[2] + "-" + tmpdateparts[1] + "-" + tmpdateparts[0] + " " + tmpdate[1];
  }

  return date;
}

/**
 * Reads the metadata of the HDF5 file and fills the dihead with data
 */
int metno::satimgh5::HDF5_head_diana(const std::string& infile, dihead& ginfo)
{
  METLIBS_LOG_TIME();
  hid_t file;
  //  char string_value[80];
  //  int npoints = 0;
  bool havePalette = false;
  std::string channelName;
  // TODO: Fix this
  int chan = 1;

  METLIBS_LOG_DEBUG("infile: " << infile);
  METLIBS_LOG_DEBUG("channel: " << ginfo.channel);
  METLIBS_LOG_DEBUG("satellite: " << ginfo.satellite);

  if (checkMetadata(infile, ginfo.metadata) == false) {
    file = H5Fopen(infile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    openGroup(file, "/", "", ginfo.metadata);
    if (H5Fclose(file) < 0) {
      return -1;
    }
  }

  fill_head_diana(ginfo.metadata, chan);
  // Check valid dateTime
  if (hdf5map.count("dateTime")) {
    if (hdf5map["dateTime"].find("1970-01-01") != std::string::npos || hdf5map["dateTime"].length() == 0) {
      hdf5map["dateTime"] = "";
    }
  } else {
    hdf5map["dateTime"] = "";
  }

#ifdef DEBUGPRINT
  getAllValuesFromMap();
#endif

  // Return channel names (ie 1 2 9i) to Diana for display purposes.
  // Is not read from HDF5 file
  getDataForChannel(ginfo.channelinfo, channelName);

  METLIBS_LOG_DEBUG("channelinfo: " << ginfo.channelinfo);
  METLIBS_LOG_DEBUG("channelName: " << channelName);
  if (hdf5map.count("product")) {
    METLIBS_LOG_DEBUG("hdf5map[product]: " << hdf5map["product"]);
    if (hdf5map["product"].length() > 0) {
      ginfo.channel = hdf5map["product"];
      ginfo.name = hdf5map["product"];
    } else {
      ginfo.channel = channelName;
      ginfo.name = channelName;
    }
  } else {
    ginfo.channel = channelName;
    ginfo.name = channelName;
  }
  if (hdf5map.count("nodata")) {
    ginfo.nodata = to_float(hdf5map["nodata"], 0.0);
  } else {
    ginfo.nodata = 0.0;
  }

  METLIBS_LOG_DEBUG("ginfo.channel: " << ginfo.channel);

  // Map from gHead to gInfo.
  if (hdf5map.count("place"))
    ginfo.satellite = hdf5map["place"];
  else
    ginfo.satellite = "";
  // Calibration for sat images
  bool cloudTopTemperature = false;
  if (metadataMap.count("cloudTopTemperature"))
    cloudTopTemperature = (metadataMap["cloudTopTemperature"] == "true");
  if (cloudTopTemperature && ginfo.hdf5type == msg) {
    ginfo.cal_ir = "T=(0)+(1)*C";
    ginfo.cal_vis = "T=(0)+(1)*C";
  } else if (cloudTopTemperature && ginfo.hdf5type == noaa) {
    ginfo.cal_ir = "T=(0)+(1)*C";
    ginfo.cal_vis = "T=(0)+(1)*C";
  } else if (cloudTopTemperature && ginfo.hdf5type == saf) {
    if (metadataMap.count("cloudTopUnit")) {
      ginfo.cal_ir = "T=(0)+(1)*" + metadataMap["cloudTopUnit"];
      ginfo.cal_vis = "T=(0)+(1)*" + metadataMap["cloudTopUnit"];
    } else {
      ginfo.cal_ir = "T=(0)+(1)*C";
      ginfo.cal_vis = "T=(0)+(1)*C";
    }
  }

  METLIBS_LOG_DEBUG("ginfo.paletteinfo: " << ginfo.paletteinfo);

  paletteMap.clear();
  std::vector<std::string> paletteInfo = split(ginfo.paletteinfo, ",", true);
  std::vector<std::string> paletteSteps;       // values in palette
  std::vector<std::string> paletteColorVector; // steps in colormap for selected colors
                                     // This values are defined in paletteinfo from setupfile
  std::vector<std::string> paletteInfoRow;
  // True if paletteinfo in setupfile contains value:color
  bool manualColors = false;
  for (unsigned int ari = 0; ari < paletteInfo.size(); ari++) {
    paletteInfoRow = split(paletteInfo[ari], ":", true);
    for (unsigned int k = 0; k < paletteInfoRow.size(); k++) {
      if (k == 0)
        paletteSteps.push_back(paletteInfoRow[k]);
      if (k == 1) {
        paletteColorVector.push_back(paletteInfoRow[k]);
        manualColors = true;
      }
    }
    paletteInfoRow.clear();
  }

#ifdef DEBUGPRINT
  for (unsigned int i = 0; i < paletteSteps.size(); i++) {
    METLIBS_LOG_DEBUG("paletteSteps[" << i << "] = " << paletteSteps[i]);
  }
  for (unsigned int i = 0; i < paletteColorVector.size(); i++) {
    METLIBS_LOG_DEBUG("paletteColorVector[" << i << "] = " << paletteColorVector[i]);
  }
#endif
  ginfo.noofcl = 255;
  hdf5map["palette"] = "false";
  hdf5map["isBorder"] = "false";
  if (paletteSteps.size() > 0 || paletteStringMap.size() > 0) {
    std::string unit = "";
    if (paletteSteps.size())
      unit = paletteSteps[0];
    if ((unit == "border") && (paletteSteps.size() == 2)) {
      METLIBS_LOG_DEBUG("Drawing a border with colour: " << paletteSteps[1]);
      hdf5map["isBorder"] = "true";
      hdf5map["border"] = paletteSteps[1];
    } else {
      if (paletteStringMap.size() > 0) {
        ginfo.noofcl = paletteStringMap.size() - 1;
        for (size_t i = 1; i < paletteStringMap.size(); i++) {
          paletteMap[i] = i;
          METLIBS_LOG_DEBUG("Getting paletteMap1: " << i << "," << paletteMap[i]);
        }
      } else {
        METLIBS_LOG_DEBUG("Getting paletteMap2: ");
        ginfo.noofcl = paletteSteps.size() - 1;
        for (size_t i = 1; i < paletteSteps.size() - 1; i++) {
          paletteMap[to_float(paletteSteps[i])] = i - 1;
        }
      }

      hdf5map["palette"] = "true";
      METLIBS_LOG_DEBUG("palette will be made");
      METLIBS_LOG_DEBUG("unit: " << unit);
      METLIBS_LOG_DEBUG("ginfo.noofcl: " << ginfo.noofcl);
      hdf5map["noofcl"] = from_number(ginfo.noofcl);
#ifdef DEBUGPRINT
      for (std::map<float, int>::iterator p = paletteMap.begin(); p != paletteMap.end(); p++)
        METLIBS_LOG_DEBUG("paletteMap[" << p->first << "]: " << p->second);
#endif
      int backcolour = 0;
      if (paletteSteps.size())
        backcolour = to_int(paletteSteps[paletteSteps.size() - 1], 255);
      havePalette = makePalette(ginfo, unit, backcolour, paletteColorVector, manualColors);
    }
  }

  METLIBS_LOG_DEBUG("ginfo.noofcl: " << ginfo.noofcl);
  if (hdf5map.count("projdef"))
    METLIBS_LOG_DEBUG("hdf5map[\"projdef\"]: " << hdf5map["projdef"]);

  float denominator = 1.0;
  if (hdf5map.count("pixelscale")) {
    if (!(hdf5map["pixelscale"].find("KM") != std::string::npos)) {
      denominator = 1000.0;
    }
  } else if (ginfo.hdf5type == radar) {
    if (hdf5map.count("projdef")) {
      if (hdf5map["projdef"].find("+units=m ") != std::string::npos) {
        denominator = 1.0;
      } else
        denominator = 1000.0;
    } else
      denominator = 1000.0;
  } else {
    denominator = 1000.0;
  }
  if (hdf5map.count("xscale"))
    ginfo.Ax = (float)(to_float(hdf5map["xscale"]) / denominator);
  else
    ginfo.Ax = 0;
  // TODO: Fix SAF products for MSG, metadata is incorrect
  if (ginfo.Ax == 0)
    ginfo.Ax = 4;
  if (hdf5map.count("yscale"))
    ginfo.Ay = (float)(to_float(hdf5map["yscale"]) / denominator);
  else
    ginfo.Ay = 0;

  // TODO: Fix SAF products for MSG, metadata is incorrect
  if (ginfo.Ay == 0)
    ginfo.Ay = 4;
  if (hdf5map.count("xsize"))
    ginfo.xsize = to_int(hdf5map["xsize"]);
  else
    ginfo.xsize = 0;
  if (hdf5map.count("ysize"))
    ginfo.ysize = to_int(hdf5map["ysize"]);
  else
    ginfo.ysize = 0;

  if (hdf5map.count("dateTime")) {
    METLIBS_LOG_DEBUG("Datetime" << hdf5map["dateTime"]);
    if (hdf5map["dateTime"].size() > 0) {
      ginfo.time = miTime(miTime(hdf5map["dateTime"]).format("%Y%m%d%H%M00", "", true));
    }
  }
  ginfo.zsize = 0;
  // TODO: defaults correct ?
  float gridRot = 60, trueLat = 14;
  if (hdf5map.count("lon_0"))
    gridRot = to_float(hdf5map["lon_0"]);
  if (hdf5map.count("lat_ts"))
    trueLat = to_float(hdf5map["lat_ts"]);

  if (hdf5map.count("projdef")) {
    METLIBS_LOG_DEBUG("projdef: " << hdf5map["projdef"]);
    ginfo.projection.setProj4Definition(hdf5map["projdef"]);
  } else {
    // FIXME this does not work, +units=km +x_0=.. +y_0=.. will be
    // added below to an otherwise empty proj4 std::string
    ginfo.projection = Projection();
  }

  if (ginfo.hdf5type == radar) {
    if (!ginfo.projection.isDefined()) {
      METLIBS_LOG_ERROR("Bad projection '" << ginfo.projection << "'");
      return -1;
    }

    float lon_x = 0;
    float lat_y = 0;
    if (hdf5map.count("LL_lat"))
      lat_y = to_float(hdf5map["LL_lat"]);
    if (hdf5map.count("LL_lon"))
      lon_x = to_float(hdf5map["LL_lon"]);
    if (!ginfo.projection.convertFromGeographic(1, &lon_x, &lat_y)) {
      METLIBS_LOG_ERROR("data conversion error");
    }
    ginfo.Bx = lon_x / denominator;
    ginfo.By = lat_y / denominator + ginfo.ysize * ginfo.Ay;

  } else if (ginfo.hdf5type == msg) {
    if (!ginfo.projection.isDefined()) {
      return -1;
    }
    float lon_x = 0;
    float lat_y = 0;
    if (hdf5map.count("center_lon"))
      lon_x = to_float(hdf5map["center_lon"], 0.0);
    if (hdf5map.count("center_lat"))
      lat_y = to_float(hdf5map["center_lat"], 0.0);
    if (!ginfo.projection.convertFromGeographic(1, &lon_x, &lat_y)) {
      METLIBS_LOG_ERROR("data conversion error");
    }
    ginfo.Bx = lon_x - (ginfo.Ax * ginfo.xsize / 2.0);
    ginfo.By = lat_y - (ginfo.Ay * ginfo.ysize / 2.0) + ginfo.ysize * ginfo.Ay;
  } else {
    if (hdf5map.count("LL_lon"))
      ginfo.Bx = to_float(hdf5map["LL_lon"]) / denominator;
    else
      ginfo.Bx = 0;
    if (hdf5map.count("LL_lat"))
      ginfo.By = to_float(hdf5map["LL_lat"]) / denominator + ginfo.ysize * ginfo.Ay;
    else
      ginfo.By = 0;
  }

  ginfo.AIr = 0;
  ginfo.BIr = 0;

  // Dont add x_0 or y_0 if they are already there!
  if (ginfo.projection.getProj4Definition().find("+x_0=") == std::string::npos) {
    std::ostringstream tmp_proj_string;
    tmp_proj_string << ginfo.projection.getProj4Definition();
    if (denominator == 1000.0)
      tmp_proj_string << " +units=km";
    else
      tmp_proj_string << " +units=m";
    tmp_proj_string << " +x_0=" << ginfo.Bx * -denominator;
    tmp_proj_string << " +y_0=" << (ginfo.Ay * ginfo.ysize - ginfo.By) * denominator + 15; // FIXME why add 15 ???

    ginfo.projection.setProj4Definition(tmp_proj_string.str());
  }

  METLIBS_LOG_DEBUG(LOGVAL(ginfo.Ax) << LOGVAL(ginfo.Ay) << LOGVAL(ginfo.Bx) << LOGVAL(ginfo.By) << LOGVAL(trueLat) << LOGVAL(gridRot)
                                     << LOGVAL(ginfo.projection.getProj4Definition()));

  if (havePalette) // Return color palette
    return 2;
  else
    return 0;
}

bool metno::satimgh5::makePalette(dihead& ginfo, std::string unit, int backcolour, std::vector<std::string> colorvector, bool manualcolors)
{
  /*if (ginfo.noofcl == 255) {
    return false;
  }*/
  int k = 0;
  std::string* pal_name = new std::string[ginfo.noofcl];
  if (paletteStringMap.size()) {
    // cerr << "paletteStringMap.size(): " << paletteStringMap.size() << endl;
    int pSize = paletteStringMap.size();
    for (int i = 1; i < pSize; i++) {
      // cerr << "i-1: " << i-1 << " i: " << i << " " << paletteStringMap[i] << endl;
      pal_name[i - 1] = paletteStringMap[i];
    }
  } else {
    for (std::map<float, int>::iterator p = paletteMap.begin(); p != paletteMap.end(); p++) {
      pal_name[p->second] = from_number(p->first);
      if (p->second > 0)
        pal_name[p->second - 1] += std::string("-") + from_number(p->first) + std::string(" ") + unit;
    }
    pal_name[ginfo.noofcl - 2] = std::string(">") + pal_name[ginfo.noofcl - 2] + std::string(" ") + unit;
    pal_name[ginfo.noofcl - 1] = "No Value";
  }

  for (int i = 0; i < ginfo.noofcl - 1; i++) {
    METLIBS_LOG_DEBUG("pal_name[" << i << "]" << pal_name[i]);
  }

  for (int i = 0; i < ginfo.noofcl; i++) {
    ginfo.clname.push_back(pal_name[i]);
  }
  delete[] pal_name;
  unsigned short int blue[256];
  unsigned short int red[256];
  unsigned short int green[256];
  bool haveHDFPalette = (ginfo.metadata.find("RGBPalette-") != std::string::npos);
  if (haveHDFPalette) {
    for (int i = 0; i < ginfo.noofcl; i++) {
      ginfo.cmap[0][i] = RPalette[i];
      ginfo.cmap[1][i] = GPalette[i];
      ginfo.cmap[2][i] = BPalette[i];
    }

    ginfo.cmap[0][0] = 0;
    ginfo.cmap[1][0] = 0;
    ginfo.cmap[2][0] = 0;

  } else {
    int phase = 0;
    for (int i = 0; i < 255; i++) {
      phase = i / 51;
      switch (phase) {
      case 0:
        red[i] = 0;
        green[i] = i * 5;
        blue[i] = 255;
        break;
      case 1:
        red[i] = 0;
        green[i] = 255;
        blue[i] = (255 - ((i - 51) * i));
        break;
      case 2:
        red[i] = (i - 102) * 5;
        green[i] = 255;
        blue[i] = 0;
        break;
      case 3:
        red[i] = 255;
        green[i] = (255 - ((i - 153) * 5));
        blue[i] = 0;
        break;
      case 4:
        red[i] = 255;
        green[i] = 0;
        blue[i] = ((i - 204) * 5);

        break;
      }
    }

    // Backcolor from setup file
    ginfo.cmap[0][0] = backcolour;
    ginfo.cmap[1][0] = backcolour;
    ginfo.cmap[2][0] = backcolour;

    // Grey
    ginfo.cmap[0][ginfo.noofcl] = 53970 / 255.0;
    ginfo.cmap[1][ginfo.noofcl] = 53970 / 255.0;
    ginfo.cmap[2][ginfo.noofcl] = 53970 / 255.0;

    // Take nice values from the palette and put them in the colormap
    for (int i = 1; i < ginfo.noofcl; i++) {
      if (manualcolors) {
        if (colorvector[i].find("0x") != std::string::npos) {
          unsigned int rgb = strtoul(colorvector[i].c_str(), NULL, 16);
          METLIBS_LOG_DEBUG("RGB: " << rgb);
          ginfo.cmap[0][i] = (rgb >> 16) & 0xFF;
          ginfo.cmap[1][i] = (rgb >> 8) & 0xFF;
          ginfo.cmap[2][i] = (rgb >> 0) & 0xFF;
        } else {
          if (!is_number(colorvector[i])) {
            METLIBS_LOG_ERROR("Error: " << colorvector[i] << " is not a number");
          } else {
            k = to_int(colorvector[i]);
            ginfo.cmap[0][i] = red[k];
            ginfo.cmap[1][i] = green[k];
            ginfo.cmap[2][i] = blue[k];
          }
        }
      } else {
        ginfo.cmap[0][i] = red[i * (255 / ginfo.noofcl)];
        ginfo.cmap[1][i] = green[i * (255 / ginfo.noofcl)];
        ginfo.cmap[2][i] = blue[i * (255 / ginfo.noofcl)];
      }
      METLIBS_LOG_DEBUG("-----nice values for radar palette");
      METLIBS_LOG_DEBUG("i =" << i << "   k =  " << k << "   red = " << ginfo.cmap[0][i] << "   green = " << ginfo.cmap[1][i]
                              << "    blue = " << ginfo.cmap[2][i]);
    }
  }

  return true;
}
