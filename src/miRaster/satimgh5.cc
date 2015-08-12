/*!
 libmiRaster - met.no hdf5 interface

 Copyright (C) 2006-2013 met.no

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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "satimgh5.h"
#include <tiffio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <projects.h>
#include <proj_api.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <errno.h>

// #define M_TIME 1

#include <puCtools/stat.h>

#include <miLogger/logger.h>
#include <miLogger/LogHandler.h>

using namespace miutil;
using namespace milogger;
using namespace satimg;
using namespace std;

//#define DEBUGPRINT

map<string, string> metno::satimgh5::hdf5map;
map<float, int> metno::satimgh5::paletteMap;
map <string,vector<float> > metno::satimgh5::calibrationTable;
vector<int> metno::satimgh5::RPalette;
vector<int> metno::satimgh5::GPalette;
vector<int> metno::satimgh5::BPalette;
map <string,string> metno::satimgh5::metadataMap;
map<int, string> metno::satimgh5::paletteStringMap;


/*!
 TODO: Should check the metadata string and verify the format.
 */
bool metno::satimgh5::validateChannelString(string& inputStr)
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

herr_t metno::satimgh5::getDataForChannel(string& inputStr, string& data)
{

#ifdef DEBUGPRINT
  cerr << "getChannel::inputStr: " << inputStr << endl;
#endif

  vector<string> channels, channelParts;

  if (validateChannelString(inputStr)) {
    if (inputStr.find(",")!= string::npos) {
      channels = split(inputStr,",", true);
    } else {
      channels.push_back(inputStr);
    }

    for (unsigned int i = 0; i < channels.size(); i++) {
#ifdef DEBUGPRINT
      cerr << "getChannel::channel: " << channels[i] << endl;
#endif
      if (channels[i].find("-") != string::npos) {
        channelParts = split_protected(channels[i],'(', ')', "-", true);
        if (channelParts.size() == 3) {
          remove(channelParts[1],'(');
          remove(channelParts[1],')');
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

herr_t metno::satimgh5::getDataForChannel(string inputStr, int chan,
    string& chpath, string& chname, bool& chinvert, bool& subtract,
    string& subchpath, string& subchname, bool& subchinvert,
    bool& ch4co2corr, bool& subch4co2corr)
{

  vector<string> channels, channelParts, channelSplit, channelSplitParts,
  channelNameParts, subChannelNameParts, nameSplit, nameSplitParts,
  subNameSplitParts;

  chinvert = false;

  replace(inputStr, " ", "");

  if (inputStr.find(",") != string::npos) {
    channels = split(inputStr, ",", true);
  } else {
    channels.push_back(inputStr);
  }

  for (unsigned int i = 0; i < channels.size(); i++) {
    if (channels[i].find("-") != string::npos) {
      channelParts = split_protected(channels[i],'(', ')', "-", true);
      if (to_int(channelParts[0],0) == chan) {
        // check if subtract
        if (channelParts[1].find("(") != string::npos && channelParts[1].find(")") != string::npos) {
          subtract = true;
          replace(channelParts[1],"(", "");
          replace(channelParts[1],")", "");
          channelSplit = split(channelParts[1],"-", true);

          // check invert
          if (channelSplit[0].find("i") != string::npos) {
            chinvert = true;
          } else {
            chinvert = false;
          }

          if (channelSplit[1].find("i") != string::npos) {
            subchinvert = true;
          } else {
            subchinvert = false;
          }

          // Check co2corr
          if (channelSplit[0].find("4r") != string::npos) {
            ch4co2corr = true;
          } else {
            ch4co2corr = false;
          }

          if (channelSplit[1].find("4r") != string::npos) {
            subch4co2corr = true;
          } else {
            subch4co2corr = false;
          }

        } else {
          // No substract
          subtract = false;
          subchinvert = false;

          // Check invert
          if (channelParts[1].find("i") != string::npos) {
            chinvert = true;
          } else {
            chinvert = false;
          }

          // Check co2corr
          if (channelParts[1].find("4r") != string::npos) {
            ch4co2corr = true;
          } else {
            ch4co2corr = false;
          }
        }

        // extract path and name
        if (channelParts[2].find("(") != string::npos && channelParts[2].find(")") != string::npos
            && subtract == true) {
          replace(channelParts[2],"(", "");
          replace(channelParts[2],")", "");
          nameSplit = split(channelParts[2],"-", true);
          nameSplitParts = split(nameSplit[0], ":", true);
          subNameSplitParts = split(nameSplit[1],":", true);
          chpath = nameSplitParts[0];
          chname = nameSplitParts[1];
          subchpath = subNameSplitParts[0];
          subchname = subNameSplitParts[1];

        } else if ((channelParts[2].find(":") != string::npos) && (subtract == false)) {
          nameSplit = split(channelParts[2],":", true);
		  // We may have more than one level of groups
		  // The chpath is construted as <group1>:<group2>:...<groupn>
		  // the last is the chname
		  for (int i = 0; i < nameSplit.size(); i++)
		  {
			  if (i == nameSplit.size() - 1)
			  {
				  chname = nameSplit[i];
			  }
			  else if (i == nameSplit.size() - 1)
			  {
				  chpath += nameSplit[i];
			  }
			  else
			  {
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
hid_t metno::satimgh5::checkType(hid_t dataset, string name)
{
  hid_t dset;
  hid_t dtype;
  hid_t dclass;
  herr_t status;
  hid_t result;

  dset = H5Dopen2(dataset, name.c_str(),H5P_DEFAULT);
  dtype = H5Dget_type(dset);
  dclass = H5Tget_class(dtype);

  result = dclass;

  status = H5Tclose(dtype);
  status = H5Dclose(dset);

  return result;
}

/**
 * Reads the imagedata of the HDF5 file with the help of metadata read in HDF5_head_diana
 */
int metno::satimgh5::HDF5_read_diana(const string& infile,
    unsigned char *image[], float *orgimage[], int nchan, int chan[],
    dihead &ginfo)
{
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::HDF5_read_diana" << endl;
#endif

  int pal = 0;
  string chpath = "";
  string chname = "";
  bool chinvert = false;
  bool subtract = false;
  bool subch4co2corr = false;
  bool ch4co2corr = false;
  string subchpath = "";
  string subchname = "";
  bool subchinvert = false;
  H5G_stat_t statbuf;
  hid_t group;
  hid_t subgroup;
  hid_t file;
  herr_t res = 0;
  herr_t status = 0;
  float **float_data;
  float **float_data_sub;
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif

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
    image[i] = new unsigned char[ginfo.xsize*ginfo.ysize];
    for (unsigned int j = 0; j < ginfo.xsize*ginfo.ysize; j++) {
      image[i][j] = 0;
    }
  }

  for (int i = 0; i < nchan; i++) {
    orgimage[i] = new float[ginfo.xsize*ginfo.ysize];
    for (unsigned int j = 0; j < ginfo.xsize*ginfo.ysize; j++) {
      orgimage[i][j] = -32000.0;
    }
  }

  /* Check if cacheFilePath is set in metadata
   * Example:
   * /tmp-cacheFilePath
   * will put temporary files in /tmp
   */
  string tmpFilePath = "";
  if (metadataMap.count("cacheFilePath"))
	tmpFilePath = metadataMap["cacheFilePath"];
  string filePath = tmpFilePath;
  // TODO, check if correct!!!
  string cacheFileName = from_number(nchan) + "_" + from_number(ginfo.hdf5type) +
    ginfo.channel + infile.substr(infile.rfind("/") + 1,infile.length() - (infile.rfind("/") + 1));
  bool haveCachedImage = false;

  /* Check if metadata contains
   * true-cloudTopTemperature
   * in this case the cloudTopTemperature will be used and
   * orgImage will be generated
   */
  bool cloudTopTemperature = false;
  if (metadataMap.count("cloudTopTemperature"))
	cloudTopTemperature = (metadataMap["cloudTopTemperature"] == "true");

  tmpFilePath += string("/") + from_number(ginfo.hdf5type) + ginfo.channel
  + ginfo.time.isoTime();
  for (int j=0; j<nchan; j++)
    tmpFilePath += from_number(chan[j]);

  remove(tmpFilePath,' ');
  remove(tmpFilePath,':');
  remove(tmpFilePath,'-');


  ImageCache* mImageCache = ImageCache::getInstance();

#if 0
  if (!mImageCache->getFromCache(cacheFileName, (uint8_t*)image)) {
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  cerr << "metno::satimgh5::HDF5_read_diana, Cached image read in: " << s << endl;
#endif
      haveCachedImage = true;
      /* Return if the file is a radar file or if we
       * don't need the cloud top temperature
       */
      if (ginfo.hdf5type == radar || (!cloudTopTemperature))
        return pal;
  }
#endif

#if 1
  // If cacheFilePath is set, look for a cached copy of the image
  if (tmpFilePath != "") {
    /* Remove cached files older than a certain time
     * specified in metadataMap["cacheFileKeepTime"]
     */
    pu_struct_stat st;

    unsigned char isFile =0x8;
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(filePath.c_str())) == NULL) {
        //cerr << "Cannot open: " << filePath << endl;
    } else {
      string fullPath;
      // Get keeptime, if not found set it to 1 day
      int keepTime = to_int(metadataMap["cacheFileKeepTime"],3600*24);
      while ((dirp = readdir(dp)) != NULL) {
        //cerr << dirp->d_name << endl;
        if (1) {
          fullPath = filePath + string("/") + string(dirp->d_name);
          if (pu_stat(fullPath.c_str(), &st) == 0) {
            //cerr << fullPath << " atim: " << st.st_atime << " mtime: " << st.st_mtime <<" ctime: " << st.st_ctime<< endl;
            if(time(NULL) - st.st_atime > keepTime) {
              cerr << "file: " << fullPath << " is older than "
              << keepTime << " seconds, removing" << endl;
              if( remove(fullPath.c_str()) != 0 )
                cerr << "Error deleting file" << endl;
            }
          } else
            if(errno == EACCES)
              cerr <<"EACCES"<< endl;
            else if (errno == EBADF)
              cerr <<"EBADF"<< endl;
            else if (errno == EFAULT)
              cerr <<"EFAULT"<< endl;
            else if (errno == ENOENT)
              cerr <<"ENOENT"<< endl;
            else if (errno == ENOTDIR)
              cerr <<"ENOTDIR"<< endl;
            else
              cerr <<"Unknown"<< endl;
        }
      }
      closedir(dp);
    }

    /* Add the tmpFileName to the tmpFilePath
     * Example file name:
     * /tmp/2329i20090225113009012
     * Broken down filename:
     * 2 - hdf5type
     * 329i - channels in product
     * 20090225113009 - date
     * 012 - channels in file
     */
    tmpFilePath += string("/") + from_number(ginfo.hdf5type) + string(ginfo.channel)
    + ginfo.time.isoTime();
    for (int j=0; j<nchan; j++)
      tmpFilePath += from_number(chan[j]);

    remove(tmpFilePath,' ');
    remove(tmpFilePath,':');
    remove(tmpFilePath,'-');

    ifstream in(tmpFilePath.c_str(), ios::in | ios::binary);

    if (!in) {
#ifdef DEBUGPRINT
      cerr << "Cannot open input file: " << tmpFilePath << endl;
#endif
    } else {
#ifdef DEBUGPRINT
      cerr << "File: " << tmpFilePath << endl;
#endif
      try
      {
        double num;
        int length;

        in.seekg(0,ios::end);
        length = in.tellg();
        in.seekg(0, ios::beg);
#ifdef DEBUGPRINT
        cerr << "length of data in file: " << length << endl;
#endif
        for(int i=0;i<nchan;i++)
          in.read((char*)image[i], length/nchan);
      }
      catch (exception& e)
      {
        cerr << "exception caught: " << e.what() << endl;
      }
      in.close();
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.HDF5_read_diana");
  COMMON_LOG::getInstance("common").infoStream() << "Cached image read in: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
      haveCachedImage = true;

      /* Return if the file is a radar file or if we
       * don't need the cloud top temperature
       */
      if (ginfo.hdf5type == radar || (!cloudTopTemperature))
        return pal;
    }
  }

#endif

  file = H5Fopen(infile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);

  // Loop through all channels
  for (int q = 0; q < nchan; q++) {
    res = getDataForChannel(ginfo.channelinfo, chan[q], chpath, chname,
        chinvert, subtract, subchpath, subchname, subchinvert, ch4co2corr,
        subch4co2corr);

#ifdef DEBUGPRINT
    cerr << "metno::satimgh5::HDF5_read_diana: ginfo.channelinfo: " << ginfo.channelinfo << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: chan[q]: " << chan[q] << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: chpath: " << chpath << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: chname: " << chname << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: chinvert: " << chinvert << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: subtract: " << subtract << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: subchpath: " << subchpath << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: subchname: " << subchname << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: subchinvert: " << subchinvert << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: ginfo.xsize : " << ginfo.xsize << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: ginfo.ysize : " << ginfo.ysize << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: ch4co2corr : " << ch4co2corr << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: subch4co2corr : " << subch4co2corr << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: chpath.c_str()" << chpath << endl;
    cerr << "metno::satimgh5::HDF5_read_diana: chname.c_str()" << chname << endl;
#endif

    // first, we must check if channel path contains ":"
	vector<string> chpathSplit;
	if(chpath.find(":") != string::npos)
	{
		chpathSplit = split(chpath,":", true);
	}
	else
	{
		// One level of groups
		chpathSplit.push_back(chpath);
	}
	if (chpathSplit[0].length() > 0) {
		H5Gget_objinfo(file, chpathSplit[0].c_str(), FALSE, &statbuf);
	} else {
		H5Gget_objinfo(file, chname.c_str(), FALSE, &statbuf);
	}

    if (statbuf.type == H5G_GROUP) {
#ifdef DEBUGPRINT
      cerr << "READ FOUND GROUP" << endl;
#endif
      group = H5Gopen2(file, chpathSplit[0].c_str(),H5P_DEFAULT);

      if (group >= 0) {
		  // Check if group in groups
		  if (chpathSplit.size() > 1)
		  {
			  // This should be a subgroup
			  int j = 1;
			  while ((statbuf.type == H5G_GROUP) && (j < chpathSplit.size()))
			  {
				chpath = chpathSplit[j];
#ifdef DEBUGPRINT
				cerr << "chpath: " << chpath << endl;
#endif
				H5Gget_objinfo(group, chpath.c_str(), FALSE, &statbuf);
				if (statbuf.type == H5G_GROUP)
				{
					subgroup = H5Gopen2(group, chpath.c_str(),H5P_DEFAULT);
					if (subgroup >= 0) {
						status = H5Gclose(group);
						group = subgroup;
					}
					j++;
				}
			  }
			  // Here we should have a data set
			  H5Gget_objinfo(group, chname.c_str(), FALSE, &statbuf);
#ifdef DEBUGPRINT
				cerr << "READ FOUND DATASET INSIDE SUBGROUP" << endl;
#endif
		  }
		  else
		  {
			  H5Gget_objinfo(group, chname.c_str(), FALSE, &statbuf);
		  }

        if (statbuf.type == H5G_DATASET) {
#ifdef DEBUGPRINT
          cerr << "READ FOUND DATASET INSIDE GROUP" << endl;
#endif
          if (checkType(group, chname) == H5T_INTEGER) {
#ifdef DEBUGPRINT
            cerr << "H5T_INTEGER FOUND" << endl;
            cerr << "READ FOUND DATASET INSIDE GROUP OF TYPE INTEGER" << endl;
#endif
            // Initialize array for data retrieval
            float_data = new float*[ginfo.xsize];
            float_data[0] = new float[ginfo.xsize * ginfo.ysize];

            for (unsigned int i=1; i<ginfo.xsize; i++)
              float_data[i] = float_data[0] + i * ginfo.ysize;

            // If channel is 4r, then co2 correct it else just read it
            if (ch4co2corr)
              co2corr_bt39(ginfo, group, float_data, chinvert, q);
            else
              readDataFromDataset(ginfo, group, chpath, chname, chinvert,
                  float_data, q, orgimage, cloudTopTemperature, haveCachedImage);

            /* If the channelinfo contains:
             * 0-(10-9)-(image10:image_data-image9:image_data)
             * 10-9 - will subtract 9 from 10.
             */
            if (subtract) {
			  int firstmin = 0;
			  if (hdf5map.count(string("min_") + from_number(q)))
				firstmin = to_int(hdf5map[string("min_") + from_number(q)]);
			  int firstmax = 0;
			  if (hdf5map.count(string("max_") + from_number(q)))
				firstmax = to_int(hdf5map[string("max_") + from_number(q)]);

              /* Save the channel path as chan_q
               * chan_q is used to extract channel specific settings from
               * metadata in makeImage.
               * Example:
               * channelinfo=0-(10-9)-(image10:image_data-image9:image_data) =>
               * image10_image_data_image9_image_data
               */
              hdf5map[string("chan_") + from_number(q)] = chpath + string("_") + chname + string("_")
              + subchpath + string("_") + subchname;
              status = H5Gclose(group);
              group = H5Gopen2(file, subchpath.c_str(),H5P_DEFAULT);
              float_data_sub = new float*[ginfo.xsize];
              float_data_sub[0] = new float[ginfo.xsize * ginfo.ysize];

              // Initialize array for second channel
              for (unsigned int i=1; i<ginfo.xsize; i++)
                float_data_sub[i] = float_data_sub[0] + i * ginfo.ysize;

              // Same procedure as for the first channel
              if (subch4co2corr)
                co2corr_bt39(ginfo, group, float_data_sub, subchinvert, q);
              else
                readDataFromDataset(ginfo, group, subchpath, subchname,
                    subchinvert, float_data_sub, q, orgimage,
                    cloudTopTemperature, haveCachedImage);
			  int secondmin = 0;
			  if (hdf5map.count(string("min_") + from_number(q)))
				secondmin = to_int(hdf5map[string("min_") + from_number(q)]);
			  int secondmax = 0;
			  if (hdf5map.count(string("max_") + from_number(q)))
				secondmax = to_int(hdf5map[string("max_") + from_number(q)]);

              // Compute min/max for the subtracted picture
              hdf5map[string("min_") + from_number(q)] = from_number(firstmin - secondmax);
              hdf5map[string("max_") + from_number(q)] = from_number(firstmax - secondmin);

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
              hdf5map[string("chan_") + from_number(q)] = chpath + string("_") + chname;
            }

            // Return the already cached image
            if (haveCachedImage) {
#ifdef DEBUGPRINT
              cerr << "Image is already cached" << endl;
#endif
            } else if (ginfo.hdf5type == radar) {
#ifdef DEBUGPRINT
              cerr << "RADAR IMAGE" << endl;
#endif
              makeImage(image, float_data);
            } else {
#ifdef DEBUGPRINT
              cerr << "SATELLITE IMAGE" << endl;
#endif
              makeImage(image, float_data, q);
            }
            delete[] float_data[0];
            delete[] float_data;

          } else if (checkType(group, chname) == H5T_FLOAT) {
#ifdef DEBUGPRINT
            cerr << "READ FOUND DATASET INSIDE GROUP OF TYPE FLOAT" << endl;
#endif
            // Initialize array
            float_data = new float*[ginfo.xsize];
            float_data[0] = new float[ginfo.xsize * ginfo.ysize];

            for (unsigned int i=1; i<ginfo.xsize; i++)
              float_data[i] = float_data[0] + i * ginfo.ysize;

            // Read dataset, radar style
            readDataFromDataset(ginfo, group, chpath, chname, chinvert,
                float_data, q);

            makeImage(image, float_data);

            delete[] float_data[0];
            delete[] float_data;

          } else {
#ifdef DEBUGPRINT
            cerr << "int metno::satimgh5::HDF5_read_diana UNKNOWN dataset class" << endl;
#endif
          }

        }
      }
      status = H5Gclose(group);
    } else if (statbuf.type == H5G_DATASET) {
#ifdef DEBUGPRINT
      cerr << "READ FOUND DATASET" << endl;
#endif
      float_data = new float*[ginfo.xsize];
      float_data[0] = new float[ginfo.xsize * ginfo.ysize];

      for (unsigned int i=1; i<ginfo.xsize; i++)
        float_data[i] = float_data[0] + i * ginfo.ysize;

      readDataFromDataset(ginfo, file, "", chname, chinvert, float_data, q,
          orgimage, cloudTopTemperature, haveCachedImage);

      // Do not overwrite the image if it was cached
      if (!haveCachedImage)
        makeImage(image, float_data, q);

      delete[] float_data[0];
      delete[] float_data;
    }
  }

  status = H5Fclose(file);
  // If cacheFilePath is set, save a copy of the image

  // << tmpFilePath << endl;
  if (!mImageCache->getFromCache(cacheFileName, (uint8_t*)image)) {
    //cerr << "Faile to put in cache" << endl;
  }
  //  mImageCache.putInCache(file, (uint8_t*)image[0], ginfo.xsize*ginfo.ysize*nchan);
#if 1
if (tmpFilePath != "") {
    // Set everything created to 0666
    umask (0);
    pu_struct_stat st;
    if(pu_stat(filePath.c_str(),&st) != 0) {
      // Create the temporary directory if it's not there
      vector<string> filePathParts = split(filePath,"/",true);
      string realFilePath;
      for(unsigned int j=0;j<filePathParts.size();j++) {
        realFilePath += "/" + filePathParts[j];
        if(pu_stat(realFilePath.c_str(),&st) != 0) {
          cerr << "Creating directory: " << realFilePath;
#ifdef __MINGW32__
          if(mkdir(realFilePath.c_str()) != 0)
#else
          if(mkdir(realFilePath.c_str(), 0777) != 0)
#endif
            cerr << " ERROR" << endl;
          else
            cerr << " SUCCESS" << endl;
        }
      }
    }
    ofstream out(tmpFilePath.c_str(), ios::out | ios::binary);

    if (!out) {
#ifdef DEBUGPRINT
      cerr << "Cannot open file.\n";
#endif
    } else {
      for (int i=0; i<nchan; i++)
        for (unsigned int j=0; j<ginfo.xsize*ginfo.ysize; j++)
          out.put(image[i][j]);
      out.close();
    }
  }
#endif

#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.HDF5_read_diana");
  COMMON_LOG::getInstance("common").infoStream() << "Image read in: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
  return pal;
}

/**
 * Subtracts all the values from int_data with the values from int_data_sub.
 * The result is placed in float_data
 */
int metno::satimgh5::subtractChannels(float* float_data[],
    float* float_data_sub[])
{
  int xsize = 0;
  if (hdf5map.count("xsize"))
	  xsize= to_int(hdf5map["xsize"],0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
	ysize = to_int(hdf5map["ysize"],0);
  float nodata = -32000.0;
  if (hdf5map.count("nodata"))
	  nodata = to_float(hdf5map["nodata"]);
  /*float min = 32000.0;
   float max = -32000.0;*/

  for (int i=0; i < xsize; i++) {
    for (int j=0; j < ysize; j++) {
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
   hdf5map[string("submin")] = string(min);
   hdf5map[string("submax")] = string(max);*/

  return 0;
}

/**
 * Returns the palettestep for the specified value.
 */
int metno::satimgh5::getPaletteStep(float value)
{
  int step=0;
  for (map<float,int>::iterator p = paletteMap.begin(); p != paletteMap.end(); p++) {
    if (value < p->first) {
      break;
    }
#ifdef DEBUGPRINT
//  cerr <<  " step: " << step << endl;
#endif
    step++;
  }
  return step;
}

/**
 * Process data from int_data and places it in channel 0 of the image array
 */
int metno::satimgh5::makeImage(unsigned char *image[], float* float_data[])
{
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
  int k = 0;
  //  int intToChar = 65535/255;
  int xsize = 0;
  if (hdf5map.count("xsize"))
	  xsize = to_int(hdf5map["xsize"],0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
	ysize = to_int(hdf5map["ysize"],0);
  float gain=1.0;
  if (hdf5map.count("gain"))
	gain = to_float(hdf5map["gain"]);
  float offset=0;
  if (hdf5map.count("offset"))
	offset = to_float(hdf5map["offset"]);
  int nodata=255;
  if (hdf5map.count("nodata"))
	nodata = to_int(hdf5map["nodata"],255);
  int noofcl=255;
  if (hdf5map.count("noofcl"))
	noofcl=to_int(hdf5map["noofcl"],255);
  bool isBorder=false;
  if (hdf5map.count("isBorder"))
	isBorder = (hdf5map["isBorder"] == "true");
  bool isPalette=false;
  if (hdf5map.count("palette"))
	isPalette = (hdf5map["palette"] == "true");
  int borderColour=255;
  if (hdf5map.count("border"))
	borderColour=to_int(hdf5map["border"],255);
#ifdef DEBUGPRINT
  if(isBorder)
    cerr << "border with colour " << borderColour << endl;
  if(isPalette)
    cerr << "palette" << endl;
  cerr << "making radar image: ";
#endif
  for (int i=0; i < xsize; i++) {
    for (int j=0; j < ysize; j++) {
      if (float_data[i][j] == nodata) {
        image[0][k] = noofcl;
      } else {
        // If the picture is a border, set the borderColour
        if (isBorder)
          image[0][k] = (float_data[i][j]>0) ? borderColour : 0;
        // If it has palette, get the palettestep
        else if (isPalette)
          image[0][k] = getPaletteStep(((float_data[i][j])*gain)+offset);
        // Else just set the value
        else
          image[0][k] = (int)float_data[i][j];
      }
      k++;
    }
  }
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.makeImage");
  COMMON_LOG::getInstance("common").infoStream() << "Make image took: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
#ifdef DEBUGPRINT
  cerr << " done!" << endl;
#endif
  return 0;
}

/*
 * CO2 correction of the MSG 3.9 um channel:
 *
 * T4_CO2corr = (BT(IR3.9)^4 + Rcorr)^0.25
 * Rcorr = BT(IR10.8)^4 - (BT(IR10.8)-dt_CO2)^4
 * dt_CO2 = (BT(IR10.8)-BT(IR13.4))/4.0
 */
int metno::satimgh5::co2corr_bt39(dihead& ginfo, hid_t source, float* ch4r[],
    bool chinvert, int chan)
{
  float epsilon = 0.001;
  int xsize = 0;
  if (hdf5map.count("xsize"))
	xsize = to_int(hdf5map["xsize"],0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
	ysize = to_int(hdf5map["ysize"],0);
  float **dt_co2;
  float **a;
  float **b;
  float **Rcorr;
  float **x;
  float **ch4;
  float **ch9;
  float **ch11;

  dt_co2 = new float*[xsize];
  a = new float*[xsize];
  b = new float*[xsize];
  Rcorr = new float*[xsize];
  x = new float*[xsize];
  ch4 = new float*[xsize];
  ch9 = new float*[xsize];
  ch11 = new float*[xsize];

  dt_co2[0] = new float[xsize*ysize];
  a[0] = new float[xsize*ysize];
  b[0] = new float[xsize*ysize];
  Rcorr[0] = new float[xsize*ysize];
  x[0] = new float[xsize*ysize];
  ch4[0] = new float[xsize*ysize];
  ch9[0] = new float[xsize*ysize];
  ch11[0] = new float[xsize*ysize];

  for (int i=1; i<xsize; i++) {
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
  readDataFromDataset(ginfo, source, "image4", "image_data", false, ch4, chan,
      ch4, false, false);
  readDataFromDataset(ginfo, source, "image9", "image_data", false, ch9, chan,
      ch9, false, false);
  readDataFromDataset(ginfo, source, "image11", "image_data", false, ch11,
      chan, ch11, false, false);

  float min = 32000.0;
  float max = -32000.0;
  for (int i=0; i < xsize; i++) {
    for (int j=0; j < ysize; j++) {
      if (ch9[i][j] > 0.0) {
        dt_co2[i][j] = (ch9[i][j] - ch11[i][j])/4.0;
        a[i][j] = ch9[i][j]*ch9[i][j]*ch9[i][j]*ch9[i][j];
        b[i][j] = (ch9[i][j]-dt_co2[i][j])*(ch9[i][j]-dt_co2[i][j])*(ch9[i][j]
                                                                            -dt_co2[i][j])*(ch9[i][j]-dt_co2[i][j]);
        Rcorr[i][j] = a[i][j] - b[i][j];
        a[i][j] = ch4[i][j]*ch4[i][j]*ch4[i][j]*ch4[i][j];
        if (a[i][j]+Rcorr[i][j]>0.0)
          x[i][j] = a[i][j]+Rcorr[i][j];
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
  hdf5map[string("max_") + from_number(chan)] = from_number(max);
  hdf5map[string("min_") + from_number(chan)] = from_number(min);

  return 0;
}

/**
 * Copies the values from float_data to orgimage
 * If there is a calibrationTable, use it for conversion.
 */
int metno::satimgh5::makeOrgImage(float* orgimage[], float* float_data[],
    int chan, string name)
{
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
  int k = 0;
  int xsize = 0;
  if (hdf5map.count("xsize"))
	xsize = to_int(hdf5map["xsize"],0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
	ysize = to_int(hdf5map["ysize"],0);
  float nodata = 0.0;
  if (hdf5map.count("nodata"))
	nodata=to_float(hdf5map["nodata"],0.0);
  float offset = 0;
  if (hdf5map.count("offset_" + name))
	offset=to_float(hdf5map["offset_" + name],0);
  float gain = 1;
  if (hdf5map.count("gain_" + name))
	gain = to_float(hdf5map["gain_" + name],1);
  bool haveCalibrationTable = false;
  if (calibrationTable.count("calibration_table_" + name))
    haveCalibrationTable = (calibrationTable["calibration_table_" + name].size() > 0);
  vector<float> calibrationVector;
  string description = "";
  if (hdf5map.count("description"))
	description = hdf5map["description"];
  string cloudTopUnit = "";
  if (metadataMap.count("cloudTopUnit"))
	cloudTopUnit = metadataMap["cloudTopUnit"];

#ifdef DEBUGPRINT
  cerr << "making org image: " << name << endl;
  cerr << "haveCalibrationTable: " << haveCalibrationTable << endl;
  cerr << "offset: " << offset << " gain: " << gain << endl;
  cerr << "description: " << description << endl;
  cerr << "cloudTopUnit: " << cloudTopUnit << endl;
#endif

  float add=0.0;
  float mul=1.0;

  // Convert if necessary
  if(description != "" && cloudTopUnit != "") {
    if (description.empty() || description.find("eight (m)") != string::npos && cloudTopUnit == "ft") {
      mul=100/30.52;
    }
    else if (description.empty() || description.find("eight (m)") != string::npos && cloudTopUnit == "hft") {
      mul=100/30.52/100.0;
    }
    else if (cloudTopUnit == "force_ft") {
      mul=100/30.52;
    }
    else if (cloudTopUnit == "force_hft") {
      mul=100/30.52/100.0;
    }
    else if(description.empty() || description.find("temperature (K)") != string::npos && cloudTopUnit == "C")
      add=-275.15;
  }

  if (haveCalibrationTable)
    calibrationVector = calibrationTable["calibration_table_" + name];
  for (int i=0; i < xsize; i++) {
    for (int j=0; j < ysize; j++) {
      if (float_data[i][j] == nodata)
        orgimage[chan][k] = -32000.0;
      else if (!haveCalibrationTable)
        orgimage[chan][k] = mul*(gain*float_data[i][j] + offset)+add;
      else
        orgimage[chan][k] = calibrationVector[(int)float_data[i][j]];
      k++;
    }
  }
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.makeOrgImage");
  COMMON_LOG::getInstance("common").infoStream() << "Make original image took: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
#ifdef DEBUGPRINT
  cerr << " done!" << endl;
#endif

  return 0;
}

/**
 * Process data from int_data and places it in channel chan of float_data
 */
int metno::satimgh5::makeImage(unsigned char *image[], float* float_data[],
    int chan)
{
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
  int k = 0;
  int xsize = 0;
  if (hdf5map.count("xsize"))
	  xsize = to_int(hdf5map["xsize"],0);
  int ysize = 0;
  if (hdf5map.count("ysize"))
	  ysize = to_int(hdf5map["ysize"],0);
  float nodata = -32000.0;
  /*if (hdf5map.count("nodata"))
	  nodata = to_float(hdf5map["nodata"]);*/
  bool isPalette = false;
  if (hdf5map.count("palette"))
	isPalette=(hdf5map["palette"] == "true");
  float gain = 1.0;
  if (hdf5map.count("gain"))
	gain=to_float(hdf5map["gain"]);
  float offset = 0.0;
  if (hdf5map.count("offset"))
	offset=to_float(hdf5map["offset"]);
  /* Get the channelname from hdf5map
   * This was generated earlier and looks like this:
   * image9_image_data
   * or
   * image5_image_data_image6_image_data
   */
  string channelName = "";
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
  string minString = "";
  if (metadataMap.count(channelName + string("_min")))
	minString = metadataMap[channelName + string("_min")];
  string maxString = "";
  if (metadataMap.count(channelName + string("_max")))
	maxString = metadataMap[channelName + string("_max")];
  float gammaValue = 1.0;
  if (metadataMap.count(channelName + string("_gamma")))
	gammaValue = to_float(metadataMap[channelName + string("_gamma")],1.0);

  // Replace m with - in {min,max}String.
  replace(minString, "m", "-");
  replace(maxString, "m", "-");

  // If min- or maxString contains p then count those values as percent
  bool minPercent=false;
  bool maxPercent=false;
  if (minString.find("p") != string::npos) {
    minPercent=true;
    replace(minString, "p", "");
  }
  if (maxString.find("p") != string::npos) {
    maxPercent=true;
    replace(maxString, "p", "");
  }

  // Set the lowest and highest values in the channel using
  // values from metadata or values from readDataFromDataset
  float chanMin = 0.0;
  if (hdf5map.count(string("min_") + from_number(chan)))
	chanMin = to_float(hdf5map[string("min_") + from_number(chan)]);
  float rangeMin = to_float(minString,chanMin);
  float chanMax = 0.0;
  if (hdf5map.count(string("max_") + from_number(chan)))
	chanMax = to_float(hdf5map[string("max_") + from_number(chan)]);
  float rangeMax = to_float(maxString,chanMax);
  int tmpVal;

  // Set upper/lower limit in percent
  if (minPercent)
    rangeMin = chanMin * (rangeMin*0.01);
  if (maxPercent)
    rangeMax = chanMax * (rangeMax*0.01);

  // stretch is used to stretch the range to 255
  float stretch=255.0/(rangeMax-rangeMin);

#ifdef DEBUGPRINT
  cerr << "Making channel: " << channelName << endl;
  cerr << "minPercent: " << minPercent << " maxPercent: " << maxPercent << endl;
  cerr << "chanMin: " << chanMin << " chanMax: " << chanMax << endl;
  cerr << "rangeMin: " << rangeMin << " rangeMax: " << rangeMax << endl;
  cerr << "nodata: " << nodata << endl;
  cerr << "gamma: " << gammaValue << endl;
  cerr << "stretch: " << stretch << endl;
#endif

  // Upper/lower bound in array
  float arrmin=32000.0;
  float arrmax=-32000.0;

  /* Loop through the array
   * stretch each pixel with this formula:
   * pixel = (oldpixel-lowerBound)*stretch
   * Then make sure all values are between 0.0001 and 1
   * (divide by 255 but set values under 0 to 0.0001)
   * Compute gamma if gammaValue != 1.0
   * Save highest and lowest values in arrmax and arrmin
   */
  if(!isPalette) {
  for (int i=0; i < xsize; i++) {
    for (int j=0; j < ysize; j++) {
      if (float_data[i][j] != nodata) {
        float_data[i][j] = (float_data[i][j]-rangeMin)*stretch;
        if (float_data[i][j] > 255.0)
          float_data[i][j] = 1.0;
        else if (float_data[i][j] <= 0)
          float_data[i][j] = 0.0001;
        else
          float_data[i][j] /= 255.0;
        if (gammaValue != 1.0)
          float_data[i][j] = exp(1.0/gammaValue*log(float_data[i][j]));
        if (float_data[i][j] < arrmin)
          arrmin = float_data[i][j];
        else if (float_data[i][j] > arrmax)
          arrmax = float_data[i][j];
      }
    }
  }
#ifdef DEBUGPRINT
  cerr << "Arrmin: " << arrmin << " Arrmax: " << arrmax << endl;
#endif
  /*
   * Loop through the array once more
   * if pixel == nodata || arrmax-arrmin<=0.0001
   *  zero out the array
   * else
   *  pixel = 255*(pixel-arrmin)/(arrmax-arrmin)
   *  make sure values are between 1 and 255 (0 is transparent)
   * put the pixel in image (Dianas picture) at chan
   */
  for (int i=0; i < xsize; i++) {
    for (int j=0; j < ysize; j++) {
      if (float_data[i][j] == nodata) {
        tmpVal = 0;
      } else if (isPalette) {
        tmpVal = float_data[i][j];
      } else if (arrmax-arrmin <= 0.001) {
        tmpVal = 0;
      } else {
        float_data[i][j] = (float_data[i][j]-arrmin)/(arrmax-arrmin);
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
    for (int i=0; i < xsize; i++) {
      for (int j=0; j < ysize; j++) {
        if (float_data[i][j] == nodata)
          image[chan][k++] = 0;
        else
          image[chan][k++] = float_data[i][j];
      }
    }
  }
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.makeImage");
  COMMON_LOG::getInstance("common").infoStream() << "Make image took: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
#ifdef DEBUGPRINT
  cerr << " done!" << endl;
#endif
  return 0;
}

/**
 * Read the data part of the channel in the HDF5 file.
 * The data is processed and put in the int_data[] array.
 * orgimage is filled with unprocessed data.
 */
int metno::satimgh5::readDataFromDataset(dihead& ginfo, hid_t source,
    string path, string name, bool invert, float **float_data, int chan,
    float *orgImage[], bool cloudTopTemperature, bool haveCachedImage)
{
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::readDataFromDataset" << endl;
#endif
  int k = 0;
  float min = -32768.0;
  float max = 32768.0;

  hid_t dset;
  herr_t status;
  int daynight = day_night(ginfo);
  bool skip = (name.find("1") != string::npos || name.find("2") != string::npos);
  string pathname;
  float nodata = 0.0;
  if (hdf5map.count("nodata"))
	nodata = to_float(hdf5map["nodata"],0.0);
  // Is this correct...
  float novalue = -32001.0;
  if (hdf5map.count("novalue"))
	novalue = to_float(hdf5map["novalue"],-32001.0);
  if (hdf5map.count("undetect"))
	novalue = to_float(hdf5map["undetect"],-32001.0);
  
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
  vector<float> calibrationVector;
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
  vector<float> color_range;
  if (haveColorRange)
    color_range = calibrationTable["color_range_" + pathname];

  /* If a color range has been read, example:
   * metadata=color_range_image7-visualization7:color_range
   * This will read image7:calibration:calibration_table into
   * calibrationTable[color_range_image7]
   * This vectior is used to convert values to rgb values
   */
  bool havePalette = ((ginfo.metadata.find("RGBPalette-") != string::npos) && (paletteStringMap.size() == 0));
  vector<int> palette;
  if(havePalette){
    if(chan == 0)
      palette = RPalette;
    else if(chan == 1)
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
  if (hdf5map.count(string("statmin_") + pathname))
  {
	  if (hdf5map[string("statmin_") + pathname].length() > 0)
		  if (invert)
			  max = -1*to_int(hdf5map[string("statmin_") + pathname]);
		  else
			  min = to_int(hdf5map[string("statmin_") + pathname]);
  }
  if (hdf5map.count(string("statmax_") + pathname))
  {
	  if (hdf5map[string("statmax_") + pathname].length() > 0)
		  if (invert)
			  min = -1*to_int(hdf5map[string("statmax_") + pathname]);
		  else
			  max = to_int(hdf5map[string("statmax_") + pathname]);
  }

#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::readDataFromDataset INTEGER" << endl;
  cerr << "metno::satimgh5::readDataFromDataset ginfo.xsize: " << ginfo.xsize
  << endl;
  cerr << "metno::satimgh5::readDataFromDataset ginfo.ysize: " << ginfo.ysize
  << endl;
  cerr << "metno::satimgh5::readDataFromDataset map xsize: "
  << to_int(hdf5map["xsize"],0) << endl;
  cerr << "metno::satimgh5::readDataFromDataset map ysize: "
  << to_int(hdf5map["ysize"],0) << endl;
  cerr << "metno::satimgh5::readDataFromDataset daynight: " << daynight << endl;
  cerr << "metno::satimgh5::readDataFromDataset skip: " << skip << endl;
  cerr << "metno::satimgh5::readDataFromDataset name: " << name << endl;
  cerr << "metno::satimgh5::readDataFromDataset cloudTopTemperature: "
  << cloudTopTemperature << endl;
  cerr << "metno::satimgh5::readDataFromDataset pathname: " << pathname << endl;
  cerr << "metno::satimgh5::readDataFromDataset haveColorRange: "
  << haveColorRange << endl;
  cerr << "metno::satimgh5::readDataFromDataset haveCalibrationTable: "
  << haveCalibrationTable << endl;
  cerr << "metno::satimgh5::readDataFromDataset havePalette: "
  << havePalette << endl;
  cerr << "metno::satimgh5::readDataFromDataset haveCachedImage: "
  << haveCachedImage << endl;
  cerr << "max: " << max << endl;
  cerr << "min: " << min << endl;
  cerr << "reading array: ";
#endif

  // Open the dataset
  dset = H5Dopen2(source, name.c_str(),H5P_DEFAULT);
  int** int_data;
  int_data = new int*[ginfo.xsize];
  int_data[0] = new int[ginfo.xsize * ginfo.ysize];

  for (int i=1; i<ginfo.xsize; i++)
    int_data[i] = int_data[0] + i * ginfo.ysize;

#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
  // Extract the data from the HDF5 file
  status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
      int_data[0]);
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno::satimgh5::readDataFromDataset");
  COMMON_LOG::getInstance("common").infoStream() << "H5Dread took: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif

  // Move the data to a float array for precision
  for (int i=0; i<ginfo.xsize; i++) {
    for (int j=0; j<ginfo.ysize; j++) {
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
    status = H5Dclose(dset);
    return 0;
  }

#ifdef DEBUGPRINT
  cerr << "computing: ";
#endif
#ifdef M_TIME
  gettimeofday(&pre, NULL);
#endif

  /*
   * Fill the holes in the color range to have a complete
   * lookup table to convert value in HDF5 file to the RGB
   * value. This is MEOS MSG specific.
   */
  vector<float> lookupTable;
  float *colorRange;
  if (haveColorRange) {
    float value = color_range[0];
    float key = 0;
    for (int i=0; i<color_range.size(); i++) {
      if (color_range[i] > 6000)
        break;
      key=i;
      lookupTable.push_back(color_range[i]);
      for (; value<color_range[i]; value++)
        lookupTable.push_back(color_range[i]);
    }
    /*for(int i=0;i<lookupTable.size();i++) {
     cerr << "key: " << i<< " value: " << lookupTable[i] << endl;
     }*/
  }

  float msgMax = -32000.0;
  float msgMin = 32000.0;
  for (int i=0; i<ginfo.xsize; i++) {
    for (int j=0; j<ginfo.ysize; j++) {
      if (max - min < 1.0) {
        float_data[i][j] = -32000.0;
        continue;
      }
      if ((ginfo.hdf5type != radar) && ((float_data[i][j] == nodata)
          || (float_data[i][j] == novalue) || ((ginfo.hdf5type == noaa)
              && (daynight == 1) && skip))) {
        float_data[i][j] = -32000.0;
        continue;
      } else if (haveColorRange) {
        if (invert)
          float_data[i][j] = 255-lookupTable[(int)float_data[i][j]];
        else
          float_data[i][j] = lookupTable[(int)float_data[i][j]];
      } else if (haveCalibrationTable) {
        float_data[i][j] = calibrationVector[(int)float_data[i][j]]*invertValue;
      } else if(havePalette) {
        if(invert)
          float_data[i][j] = 255-palette[(int)float_data[i][j]];
        else
          float_data[i][j] = palette[(int)float_data[i][j]];
      } else if(ginfo.hdf5type == saf) {
        if(invert)
          float_data[i][j] = 255-float_data[i][j];
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
#ifdef DEBUGPRINT
    cerr << "msgMax: " << msgMax << endl;
    cerr << "msgMin: " << msgMin << endl;
#endif
  } else if (haveColorRange || havePalette || ginfo.hdf5type == saf) {
    min = 0.0;
    max = 255.0;
  }

  // Put min/max for array in hdf5map.
  // These are used in makeImage
  hdf5map[string("max_") + from_number(chan)] = from_number(max);
  hdf5map[string("min_") + from_number(chan)] = from_number(min);

#ifdef M_TIME
  gettimeofday(&post, NULL);
  s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno::satimgh5::readDataFromDataset");
  COMMON_LOG::getInstance("common").infoStream() << "channel converted in: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
  status = H5Dclose(dset);
  return 0;
}

/**
 * Without original image for radar images
 */
int metno::satimgh5::readDataFromDataset(dihead& ginfo, hid_t source,
    string path, string name, bool invert, float **float_data, int chan)
{
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::readDataFromDataset(): " << endl;
#endif
  hid_t dset;
  herr_t status;

  dset = H5Dopen2(source, name.c_str(),H5P_DEFAULT);
#ifdef DEBUGPRINT
  cerr << "reading float array: ";
#endif

  status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
      float_data[chan]);

#ifdef DEBUGPRINT
  cerr << "status: " << status << endl;
#endif

  status = H5Dclose(dset);
  return 0;
}

/**
 * If an attribute is found in the current block it is written to the internal map structure.
 */
int metno::satimgh5::getAttributeFromGroup(hid_t &dataset, int index,
    string path)
{

#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::getAttributeFromGroup path:" << path << endl;
#endif

  hid_t native_type,memb_id,space;
  hsize_t dims[1],adims[2];
  H5T_class_t memb_cls;
  hid_t memtype;
  size_t size;
  hid_t stid;
  hid_t palette_type;
  outval_name *palette;
  int ndims,i,j,nmembs;
  hid_t attr, atype, aspace;
  char memb[1024];
  hsize_t buf = 1024;
  herr_t status = 0;
  float* float_array;
  int* int_array;
  char** char_array;
  size_t npoints = 0;
  herr_t ret = 0;
  string value = "";
  H5T_class_t dclass;
  char string_value[1024];
  char* tmpdataset;
  vector<string> splitString;

  attr = H5Aopen_idx(dataset, index);
  atype = H5Aget_type(attr);
  aspace = H5Aget_space(attr);
  status = H5Aget_name(attr, buf, memb);
  dclass = H5Tget_class(atype);
  npoints = H5Sget_simple_extent_npoints(aspace);

  if (path != "")
	  path += ":";

  if (!status==0)
	  switch (dclass) {
	  case H5T_INTEGER:
		  int_array = new int[(int)npoints];
		  ret = H5Aread(attr, H5T_NATIVE_INT, int_array);

		  if (!ret) {
			  for (int i = 0; i < (int)npoints; i++) {
				  value += from_number(int_array[i]);
			  }
		  } else {
#ifdef DEBUGPRINT
			  cerr << "metno::satimgh5::getAttributeFromGroup ret: " << ret << endl;
#endif
		  }
		  delete[] int_array;
		  break;

	  case H5T_STRING:
		  ret = H5Aread(attr, atype, string_value);

		  if (!ret) {
			  value = string(string_value);
		  }
		  break;

	  case H5T_FLOAT:
		  float_array = new float[(int)npoints];
		  ret = H5Aread(attr, H5T_NATIVE_FLOAT, float_array);

		  if (!ret) {
			  if (npoints > 1) {
				  for (int i = 0; i < (int)npoints; i++) {
					  value += from_number(float_array[i]);
					  insertIntoValueMap(path + string(memb) + "[" + from_number(i) +"]",
						  from_number(float_array[i]));
				  }
			  } else {
				  value = from_number(float_array[0]);
			  }
		  } else {
#ifdef DEBUGPRINT
			  cerr << "metno::satimgh5::getAttributeFromGroup ret: " << ret << endl;
#endif
		  }

		  delete[] float_array;
		  break;

	  case H5T_ARRAY:
#ifdef DEBUGPRINT
		  cerr << "ARRAY FOUND IN GROUP" << endl;
#endif
		  break;

	  case H5T_COMPOUND:
#ifdef DEBUGPRINT
		  cerr << "COMPOUND FOUND IN GROUP" << endl;
#endif
		  status = H5Aget_name(attr, buf, memb);
		  native_type=H5Tget_native_type(atype, H5T_DIR_DEFAULT);
		  nmembs = H5Tget_nmembers(native_type);
		  // Memory leak...
		  H5Aclose(attr);
		  attr = H5Aopen_name(dataset, memb);
		  for (int j = 0; j < nmembs; j++) {
			  memb_cls = H5Tget_member_class(native_type, j);
			  tmpdataset = H5Tget_member_name(native_type, j);
			  memb_id = H5Tget_member_type(native_type, j);

			  if(memb_cls == H5T_STRING) {
				  /*
				  * Get dataspace and allocate memory for read buffer.  This is a
				  * three dimensional attribute when the array datatype is included
				  * so the dynamic allocation must be done in steps.
				  */
				  size = H5Tget_size(memb_id);
				  space = H5Aget_space (attr);
				  ndims = H5Sget_simple_extent_dims (space, dims, NULL);

				  size++;                         /* Make room for null terminator */

				  /*
				  * Create the memory datatype.
				  */
				  memtype = H5Tcopy (H5T_C_S1);
				  status = H5Tset_size (memtype, size);

				  /*
				  * Read the data.
				  */
				  // We MUST be sure that only the "correct" palette strings is read
				  //cerr << "path: " << path << endl;
				  
				  palette_type = H5Tcreate(H5T_COMPOUND, sizeof(outval_name));
				  status = H5Tinsert(palette_type, tmpdataset, HOFFSET(outval_name,str), memtype);

				  palette = new outval_name[dims[0]];

				  /*for (i=0; i<dims[0]; i++)
				  for (j=0; j<1024; j++)
				  palette[i].str[j] = (char)0;*/

				  status = H5Aread(attr, palette_type, palette);

				  bool addPalette = false;
				  for (map <string,string>::iterator p = metadataMap.begin(); p
					  != metadataMap.end(); p++) {
						  if (!p->first.empty())
						  {
							  if (p->first.find(path) != string::npos)
							  {
								addPalette = true;
							  }
						  }
				  }

				  // If the image contains more than one palette, add only the wanted palette
				 
				  if (addPalette)
				  {
					  for (i=0; i<dims[0]; i++) {
						  splitString = split(string(palette[i].str),":",true);
						  if(splitString.size()==2) {
							  paletteStringMap[to_int(splitString[0],0)] = splitString[1];
#ifdef DEBUGPRINT
							  cerr << "splitString.size()==2 " << "i =  "<< i<< " " << splitString[0] << " " << splitString[1] << endl;
#endif
						  } else if(splitString.size()==3){
							  trim(splitString[1]);
							  if (paletteStringMap.count(to_int(splitString[1],0)) == 0)
								  paletteStringMap[to_int(splitString[1],0)] = splitString[2];
#ifdef DEBUGPRINT
							  cerr << "splitString.size()==3. " << "i =  "<< i<< " " << splitString[1] << " " << splitString[2] << endl;
#endif
						  }
					  }
				  }
				  delete[] palette;

			  }

			  free(tmpdataset);
		  }

		  break;

	  default:
#ifdef DEBUGPRINT
		  cerr << "metno::satimgh5::getAttributeFromGroup UNKNOWN found. " << endl;
		  cerr << "dclass: " << dclass << endl;
#endif
		  break;
  }

  insertIntoValueMap(path + string(memb), value);

  H5Sclose(aspace);
  H5Tclose(atype);
  H5Aclose(attr);
  return 0;
}

/**
 * If a dataset is found the metadata part of the dataset is read. Support for compound datasets is included.
 */
int metno::satimgh5::openDataset(hid_t root, string dataset, string path,
    string metadata)
{
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::openDataset dataset: " << dataset << " path: " << path << endl;
#endif
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
  myString myStr;
  size_t size;
  hid_t stid;
  hsize_t dims[25];
  hid_t memb_super_id;
  hid_t space;
  arrFloat *rdata;
  hid_t floattype;

  dset = H5Dopen2(root, dataset.c_str(),H5P_DEFAULT);
  dtype = H5Dget_type(dset);
  dclass= H5Tget_class(dtype);
  native_type=H5Tget_native_type(dtype, H5T_DIR_DEFAULT);
  vector<string> sPath = split(path,":", true);
  string dianaPath = metadataMap[path];

  switch (dclass) {
  case H5T_COMPOUND:
#ifdef DEBUGPRINT
    cerr << "dataset: " << dataset << " is H5T_COMPOUND" << endl;
#endif
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

#ifdef DEBUGPRINT
        cerr << "ARRAY FOUND" << endl;
#endif

        if ((H5Tequal(memb_super_id, H5T_IEEE_F64LE) > 0) || (H5Tequal(
            memb_super_id, H5T_IEEE_F32LE) > 0)) {
#ifdef DEBUGPRINT
          cerr << "FLOAT ARRAY FOUND" << endl;
#endif
          //if (H5Tequal(memb_super_id, H5T_FLOAT) > 0) {
          floattype = H5Tcreate(H5T_COMPOUND, sizeof(arrFloat));
          status = H5Tinsert(floattype, memb, HOFFSET(arrFloat, f), memb_id);
          space = H5Dget_space(dset);
          rdata = new arrFloat[1024];
          status = H5Dread(dset, floattype, H5S_ALL, H5S_ALL, H5P_DEFAULT,
              rdata);
          H5Tget_array_dims2(memb_id, dims);

          for (int j=0; j<dims[0]; j++) {
            insertIntoValueMap("compound:" + path + "ARRAY[" + from_number(j)
                + "]:" + string(memb), from_number(rdata[0].f[j], 20));
          }

          status = H5Dvlen_reclaim(floattype, space, H5P_DEFAULT, rdata);

          delete[] rdata;
          H5Sclose(space);
          H5Tclose(floattype);
        }
        break;

      case H5T_INTEGER:
#ifdef DEBUGPRINT
        cerr << "array is H5T_INTEGER" << endl;
#endif
        comp = H5Tcreate(H5T_COMPOUND, sizeof(int));
        status = H5Tinsert(comp, memb, 0, H5T_NATIVE_INT);
        status = H5Dread(dset, comp, H5S_ALL, H5S_ALL, H5P_DEFAULT, iary);
        status = H5Tclose(comp);
        insertIntoValueMap("compound:" + path + string(memb),
            from_number(iary[0]));
        break;

      case H5T_FLOAT:
        comp = H5Tcreate(H5T_COMPOUND, sizeof(float));
        status = H5Tinsert(comp, memb, 0, H5T_NATIVE_FLOAT);
        status = H5Dread(dset, comp, H5S_ALL, H5S_ALL, H5P_DEFAULT, fary);
        status = H5Tclose(comp);
        insertIntoValueMap("compound:" + path + string(memb),
            from_number(fary[0]));
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

        insertIntoValueMap("compound:" + path + string(memb),
            string(s3[0].str));
        break;
      }
      free(memb);
    }
    break;

  case H5T_FLOAT:
    {
      hid_t space = H5Dget_space(dset);
      int ndims = H5Sget_simple_extent_dims(space, dims, NULL);
      if (ndims == 2) {
        float *float_data;
        float_data = new float[dims[0]*dims[1]];
        for (int j = 0; j < dims[0]*dims[1]; j++) {
          float_data[j] = 0;
        }
        for (int i=1; i<dims[0]; i++)
          float_data[i] = float_data[0] + i * dims[1];
        status = H5Dread(dset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
            float_data);
        for (int i=0; i<dims[0]*dims[1]; i=i+2)
          calibrationTable[dianaPath].push_back(float_data[i+1]);
        delete[] float_data;
      }
      nrAttrs = H5Aget_num_attrs(dset);

      for (int k = 0; k < nrAttrs; k++) {
        getAttributeFromGroup(dset, k, path);
      }
    }
    break;

  case H5T_INTEGER:
    {
      hid_t space = H5Dget_space(dset);
      int ndims = H5Sget_simple_extent_dims(space, dims, NULL);
      if (ndims == 2) {
        int *int_data;
        int_data = new int[dims[0]*dims[1]];
        for (int j = 0; j < dims[0]*dims[1]; j++) {
          int_data[j] = 0;
        }
        for (int i=1; i<dims[0]; i++)
          int_data[i] = int_data[0] + i * dims[1];
        status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
            int_data);
        if(dims[1]==2)
          for (int i=0; i<dims[0]*dims[1]; i=i+2)
            calibrationTable[dianaPath].push_back((float)int_data[i+1]);
		else if(dims[1]==3) {
			// Add only to palette if path in metadata
			//cerr << "addPalette: " << path << endl;
			bool addPalette = false;
			for (map <string,string>::iterator p = metadataMap.begin(); p!= metadataMap.end(); p++) {
					if (!p->first.empty())
					{
						//cerr << "key: " << p->first << " value: " << p-> second << endl;
						if (p->first.find(path) != string::npos)
						{
							if (p->second == "RGBPalette")
							{
								addPalette = true;
							}
						}
					}
			}

			// If the image contains more than one palette, add only the wanted palette

			if (addPalette)
			{
				//cerr << "dims[0]: " << dims[0] << " dims[1]: " << dims[1] << endl;
				for (int i=0; i<dims[0]*dims[1]; i=i+3) {
					// Put the values in the palettemaps
					//cerr << int_data[i] << "," << int_data[i+1] << "," << int_data[i+2] << endl;
					RPalette.push_back(int_data[i]);
					GPalette.push_back(int_data[i+1]);
					BPalette.push_back(int_data[i+2]);
				}
			}
		}
        delete[] int_data;
      } else if (ndims == 1) {
        int *int_data;
        int_data = new int[dims[0]];
        for (int j = 0; j < dims[0]; j++) {
          int_data[j] = j;
        }
        status = H5Dread(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
            int_data);
        for (int i=0; i<dims[0]; i++)
          calibrationTable[dianaPath].push_back(int_data[i]);
        delete[] int_data;
      }
    nrAttrs = H5Aget_num_attrs(dset);

      for (int k = 0; k < nrAttrs; k++) {
        getAttributeFromGroup(dset, k, path);
      }
    }
    break;
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
int metno::satimgh5::openGroup(hid_t group, string groupname, string path,
    string metadata)
{
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::openGroup group: " << group << " groupname: " << groupname << " path: " << path << " metadata: " << metadata << endl;
#endif
  char tmpdataset[80];
  hsize_t nrObj = 0;
  int nrAttrs = 0;
  int obj_type = 0;
  H5G_stat_t statbuf;
  string delimiter;

  H5Gget_objinfo(group, groupname.c_str(), FALSE, &statbuf);
  if (statbuf.type == H5G_GROUP) {
#ifdef DEBUGPRINT
    cerr << "group: " << groupname << " is H5G_GROUP" << endl;
#endif
    hid_t root = H5Gopen2(group, groupname.c_str(),H5P_DEFAULT);
    H5Gget_num_objs(root, &nrObj);
    nrAttrs = H5Aget_num_attrs(root);
#ifdef DEBUGPRINT
    cerr << "group: " << groupname << " nrObj " << nrObj << " nrAttrs " << nrAttrs << endl;
#endif
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
        openGroup(root, tmpdataset, path + delimiter + string(tmpdataset),
            metadata);
      } else if (obj_type == H5G_DATASET) {
          H5Gget_objname_by_idx(root, (hsize_t)i, tmpdataset, 80);
          if (path.length() > 0)
            delimiter = ":";
          else
            delimiter = "";
		  openDataset(root, tmpdataset, path + delimiter + string(tmpdataset),
              metadata);
      }
    }
    H5Gclose(root);
  } else if (statbuf.type == H5G_DATASET) {
#ifdef DEBUGPRINT
    cerr << "group: " << groupname << " is H5G_DATASET" << endl;
#endif
      openDataset(group, groupname, path, metadata);
  }
  return 0;
}

/**
 * Simple insert to the internal map structure.
 */
int metno::satimgh5::insertIntoValueMap(string fullpath, string value)
{
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::insertIntoValueMap fullpath: " << fullpath << " value: " << value << endl;
#endif
  // Special case for product
  if (fullpath.find("product") != string::npos)
	  replace(value, " ", "_");
  hdf5map[fullpath] = value;
  return 0;
}

/**
 * Dump the values of the internal map structures to console.
 */
int metno::satimgh5::getAllValuesFromMap()
{
  cerr << "metno:satimgh5::getAllValuesFromMap()" << endl;
  for (map<string,string>::iterator p = hdf5map.begin(); p != hdf5map.end(); p++) {
    cerr << "Path: " << p->first << " Value: " << p->second << endl;
  }
  typedef map<float,float> mapType;
  if (calibrationTable.size() > 0)
  {
	  for (map<string,vector<float> >::iterator p = calibrationTable.begin(); p
		  != calibrationTable.end(); p++) {
			  if (!p->first.empty())
			  {
				  for (int i=0; i<p->second.size() ; i++) {
					  cerr << "Path: " << p->first << " Value: " << i << " Value: "
						  << p->second[i] << endl;
				  }
			  }
	  }
  }
  for (int i=0;i<RPalette.size();i++) {
    if(RPalette[i]!=0)
      cerr << "RPalette["<<i<<"]: " << RPalette[i] << endl;
  }
  for (int i=0;i<GPalette.size();i++) {
    if(GPalette[i]!=0)
      cerr << "GPalette["<<i<<"]: " << GPalette[i] << endl;
  }
  for (int i=0;i<BPalette.size();i++) {
    if(BPalette[i]!=0)
    cerr << "BPalette["<<i<<"]: " << BPalette[i] << endl;
  }
  return 0;
}

/**
 * Returns true if the metadata in the file is read, false otherwise
 *
 */
bool metno::satimgh5::checkMetadata(string filename, string metadata)
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
    vector<string> metadataVector = split(metadata, ",", true);
    for (int i=0; i<metadataVector.size(); i++) {
      vector<string> metadataRows = split(metadataVector[i], "-", true);
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
herr_t metno::satimgh5::fill_head_diana(string inputStr, int chan)
{
  /*
   From configfile:
   diana_xscale-compound:region:xscale,diana_yscale-compound:region:yscale,diana_xsize-compound:region:xsize,diana_ysize-compound:region:ysize

   In map:
   compound:region:xscale 1000
   compound:region:yscale 1000
   compound:region:xsize 1000
   compound:region:ysize 1000
   */

  vector<string> input;
  vector<string> inputPart;
  string value;
  /*  bool startDateSet = false;
  bool startTimeSet = false;
  bool endDateSet = false;
  bool endTimeSet = false;
  bool dateSet = false;
  bool timeSet = false;
  int ret = 0;
  */
#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::fill_head_diana: " << inputStr << endl;
#endif

  // Break metadata into pieces and insert it into hdf5map
  replace(inputStr, " ", "");
  input = split(inputStr, ",", true);

  for (int i = 0; i < input.size(); i++) {
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
    //hdf5map["dateTime"] = miTime(hdf5map["filename"].split("_",true)[1]).isoTime();
  }
#ifdef DEBUGPRINT
  cerr << __LINE__ << " Datetime: " << hdf5map["dateTime"] << endl;
#endif

  // extract proj
  if (hdf5map["projdef"].length() > 0) {
    string projdef = hdf5map["projdef"];

    replace(projdef,"+", "");
    replace(projdef,",", " ");
    hdf5map["projdef"] = projdef;

    vector<string> proj = split(projdef, " ", true);

    for (unsigned int i = 0; i < proj.size(); i++) {
      vector<string> projParts = split(proj[i], "=", true);
      hdf5map[projParts[0]] = projParts[1];
    }
  }

  if (!hdf5map["projdef"].find("+") != string::npos) {
    hdf5map["projdef"] = string("+") + hdf5map["projdef"];
    replace(hdf5map["projdef"], " ", " +");
  }

  return 1;
}

/**
 * Converts datestrings with alpha months (JAN,FEB,...)
 * this is necessary since MEOS MSG files use JAN instead of 01 etc
 */
string metno::satimgh5::convertAlphaDate(string date)
{

  if (date.find("JAN") != string::npos)
    replace(date,"JAN", "01");
  else if (date.find("FEB") != string::npos)
    replace(date,"FEB", "02");
  else if (date.find("MAR") != string::npos)
    replace(date,"MAR", "03");
  else if (date.find("APR") != string::npos)
    replace(date,"APR", "04");
  else if (date.find("MAY") != string::npos)
    replace(date,"MAY", "05");
  else if (date.find("JUN") != string::npos)
    replace(date,"JUN", "06");
  else if (date.find("JUL") != string::npos)
    replace(date,"JUL", "07");
  else if (date.find("AUG") != string::npos)
    replace(date,"AUG", "08");
  else if (date.find("SEP") != string::npos)
    replace(date,"SEP", "09");
  else if (date.find("OCT") != string::npos)
    replace(date,"OCT", "10");
  else if (date.find("NOV") != string::npos)
    replace(date,"NOV", "11");
  else if (date.find("DEC") != string::npos)
    replace(date,"DEC", "12");

  vector<string> tmpdate = split(date, " ", true);

  if (tmpdate.size() == 2) {
    vector<string> tmpdateparts = split(tmpdate[0],"-", true);
    date = tmpdateparts[2] + "-" + tmpdateparts[1] + "-" + tmpdateparts[0]
                                                                        + " " + tmpdate[1];
  }

  return date;
}

/**
 * Reads the metadata of the HDF5 file and fills the dihead with data
 */
int metno::satimgh5::HDF5_head_diana(const string& infile, dihead &ginfo)
{
  hid_t file;
  //  char string_value[80];
  //  int npoints = 0;
  bool havePalette=false;
  herr_t ret;
  string channelName;
#ifdef M_TIME
  struct timeval pre;
  struct timeval post;
  gettimeofday(&pre, NULL);
#endif
  // TODO: Fix this
  int chan = 1;

#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::HDF5_head_diana channel: " << ginfo.channel << endl;
  cerr << "metno::satimgh5::HDF5_head_diana satellite: " << ginfo.satellite << endl;
#endif

  if (checkMetadata(infile, ginfo.metadata) == false) {
    file=H5Fopen(infile.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    openGroup(file, "/", "", ginfo.metadata);
    if (H5Fclose(file)<0)
	{
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.HDF5_head_diana");
  COMMON_LOG::getInstance("common").infoStream() << "error closing file: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
		return -1;
	}
  }

  ret = fill_head_diana(ginfo.metadata, chan);
  // Check valid dateTime
  if (hdf5map.count("dateTime"))
  {
	  if (hdf5map["dateTime"].find("1970-01-01") != string::npos || hdf5map["dateTime"].length() == 0) {
		  hdf5map["dateTime"] = "";
	  }
  }
  else
  {
	  hdf5map["dateTime"] = "";
  }


#ifdef DEBUGPRINT
  getAllValuesFromMap();
#endif

  // Return channel names (ie 1 2 9i) to Diana for display purposes.
  // Is not read from HDF5 file
  ret = getDataForChannel(ginfo.channelinfo, channelName);

#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::HDF5_head_diana channelinfo: " << ginfo.channelinfo << endl;
  cerr << "metno::satimgh5::HDF5_head_diana channelName: " << channelName << endl;
  if (hdf5map.count("product"))
  {
	cerr << "hdf5map[product]: " << hdf5map["product"] << endl;
  }
#endif
  if (hdf5map.count("product"))
  {
	  if (hdf5map["product"].length() > 0) {
		  ginfo.channel = hdf5map["product"];
		  ginfo.name = hdf5map["product"];
	  } else {
		  ginfo.channel = channelName;
		  ginfo.name = channelName;
	  }
  }
  else
  {
	  ginfo.channel = channelName;
	  ginfo.name = channelName;
  }
  if (hdf5map.count("nodata"))
  {
	ginfo.nodata = to_float(hdf5map["nodata"],0.0);
  }
  else
  {
	ginfo.nodata = 0.0;
  }

#ifdef DEBUGPRINT
  cerr << "ginfo.channel: " << ginfo.channel << endl;
#endif


  projUV data;
  int i;
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
	  if (metadataMap.count("cloudTopUnit"))
	  {
		  ginfo.cal_ir = "T=(0)+(1)*" + metadataMap["cloudTopUnit"];
		  ginfo.cal_vis = "T=(0)+(1)*" + metadataMap["cloudTopUnit"];
	  }
	  else
	  {
		  ginfo.cal_ir = "T=(0)+(1)*C";
		  ginfo.cal_vis = "T=(0)+(1)*C";
	  }
  }

#ifdef DEBUGPRINT
  cerr << "ginfo.paletteinfo: " << ginfo.paletteinfo << endl;
#endif
  paletteMap.clear();
  vector<string> paletteInfo = split(ginfo.paletteinfo, ",", true);
  vector<string> paletteSteps;            //values in palette 
  vector<string> paletteColorVector;      //steps in colormap for selected colors 
                                            //This values are defined in paletteinfo from setupfile    
  vector<string> paletteInfoRow;
  // True if paletteinfo in setupfile contains value:color
  bool manualColors = false;
  for (unsigned int ari=0; ari<paletteInfo.size(); ari++){
     paletteInfoRow=split(paletteInfo[ari], ":",true);
     for (unsigned int k=0; k<paletteInfoRow.size(); k++){
          if (k==0) paletteSteps.push_back(paletteInfoRow[k]);
          if (k==1) {
              paletteColorVector.push_back(paletteInfoRow[k]);
              manualColors = true;
          }
     }
     paletteInfoRow.clear();
  }

#ifdef DEBUGPRINT
     cerr << "Checkpoint 5" << endl;
 for (unsigned int i = 0; i < paletteSteps.size(); i++ ) {
   cerr << "paletteSteps["<<i<< "] = " << paletteSteps[i] << endl;
 }
     cerr << "Checkpoint 6" << endl;

 for(unsigned int i = 0; i < paletteColorVector.size(); i++ ) {
     cerr << "paletteColorVector["<<i<< "] = " << paletteColorVector[i];
 }



#endif
  ginfo.noofcl = 255;
  hdf5map["palette"] = "false";
  hdf5map["isBorder"] = "false";
  if (paletteSteps.size() > 0 || paletteStringMap.size() > 0) {
    string unit="";
    if(paletteSteps.size())
      unit = paletteSteps[0];
    if ((unit == "border") && (paletteSteps.size() == 2)) {
#ifdef DEBUGPRINT
      cerr << "Drawing a border with colour: " << paletteSteps[1] << endl;
#endif
      hdf5map["isBorder"] = "true";
      hdf5map["border"] = paletteSteps[1];
    } else {
      if(paletteStringMap.size() > 0) {
        ginfo.noofcl=paletteStringMap.size() - 1;
        for (i=1; i<paletteStringMap.size(); i++) {
          paletteMap[i] = i;
#ifdef DEBUGPRINT
      cerr << "Getting paletteMap1: " << i << "," << paletteMap[i] << endl;
#endif
        }
      } else {
#ifdef DEBUGPRINT
      cerr << "Getting paletteMap2: "  << endl;
#endif
        ginfo.noofcl=paletteSteps.size()-1;
        for (i=1; i<paletteSteps.size()-1; i++) {
          paletteMap[to_float(paletteSteps[i])] = i-1;
        }
      }

      hdf5map["palette"] = "true";
#ifdef DEBUGPRINT
      cerr << "palette will be made" << endl;
      cerr << "unit: " << unit << endl;
      cerr << "ginfo.noofcl: " << ginfo.noofcl << endl;
#endif
      hdf5map["noofcl"] = from_number(ginfo.noofcl);
#ifdef DEBUGPRINT
      for (map<float,int>::iterator p = paletteMap.begin(); p != paletteMap.end(); p++)
        cerr << "paletteMap[" << p->first << "]: "<< p->second << endl;
#endif
      int backcolour = 0;
      if(paletteSteps.size())
        backcolour = to_int(paletteSteps[paletteSteps.size()-1],255);
      havePalette=makePalette(ginfo, unit, backcolour,paletteColorVector, manualColors);
    }
  }

#ifdef DEBUGPRINT
  cerr << "metno::satimgh5::HDFS_head_diana ginfo.noofcl: " << ginfo.noofcl << endl;
  if (hdf5map.count("projdef"))
	cerr << "metno::satimgh5::hdf5map[\"projdef\"]: " << hdf5map["projdef"] << endl;
#endif

  float denominator = 1.0;
  if (hdf5map.count("pixelscale"))
  {
	  if (!(hdf5map["pixelscale"].find("KM") != string::npos)) {
		  denominator = 1000.0;
	  }
  }else if (ginfo.hdf5type == radar) {
	  if (hdf5map.count("projdef"))
	  {
		if(hdf5map["projdef"].find("+units=m ") != string::npos)
		{
			denominator = 1.0;
		}
		else
			denominator = 1000.0;
	  }
	  else
		denominator = 1000.0;
  } else {
	  denominator = 1000.0;
  }
  if (hdf5map.count("xscale"))
	ginfo.Ax = (float)(to_float(hdf5map["xscale"]) / denominator);
  else
	ginfo.Ax = 0;
  // TODO: Fix SAF products for MSG, metadata is incorrect
  if(ginfo.Ax == 0)
    ginfo.Ax = 4;
  if (hdf5map.count("yscale"))
	ginfo.Ay = (float)(to_float(hdf5map["yscale"]) / denominator);
  else
	ginfo.Ay = 0;

  // TODO: Fix SAF products for MSG, metadata is incorrect
  if(ginfo.Ay == 0)
      ginfo.Ay = 4;
  if (hdf5map.count("xsize"))
	ginfo.xsize = to_int(hdf5map["xsize"]);
  else
	ginfo.xsize = 0;
  if (hdf5map.count("ysize"))
	ginfo.ysize = to_int(hdf5map["ysize"]);
  else
	ginfo.ysize = 0;

#ifdef DEBUGPRINT
  if (hdf5map.count("dateTime"))
	cerr << "Datetime" << hdf5map["dateTime"] << endl;
#endif
  if (hdf5map.count("dateTime"))
  {
	  if (hdf5map["dateTime"].size() > 0) {
		  ginfo.time = miTime(miTime(hdf5map["dateTime"]).format("%Y%m%d%H%M00"));
	  }
  }
  ginfo.zsize = 0;
  // TODO: defaults correct ?
  if (hdf5map.count("lon_0"))
	ginfo.gridRot = to_float(hdf5map["lon_0"]);
  else
	ginfo.gridRot = 14.0;
  if (hdf5map.count("lat_ts"))
	ginfo.trueLat = to_float(hdf5map["lat_ts"]);
  else
	ginfo.trueLat = 60.0;
#ifdef DEBUGPRINT
  if (hdf5map.count("projdef"))
	cerr << "projdef: " <<  hdf5map["projdef"] << endl;
#endif
  if (hdf5map.count("projdef"))
	ginfo.projection = hdf5map["projdef"];
  else
	ginfo.projection = "";

  ginfo.proj_string = ginfo.projection;

  if (ginfo.hdf5type == radar) {
    PJ *ref;

    if ( ! (ref = pj_init_plus(ginfo.projection.c_str()))) {
    	cerr << "Bad proj string: " << ginfo.projection << endl;
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.HDF5_head_diana");
  COMMON_LOG::getInstance("common").infoStream() << "error in projdef: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
      return -1;
    }

    if (hdf5map.count("LL_lat"))
		data.v = to_float(hdf5map["LL_lat"]);
	else
		data.v = 0;
	if (hdf5map.count("LL_lon"))
		data.u = to_float(hdf5map["LL_lon"]);
	else
		data.u = 0;
    data.u *= DEG_TO_RAD;
    data.v *= DEG_TO_RAD;
    data = pj_fwd(data, ref);

    if (data.u == HUGE_VAL) {
      cerr << "data conversion error" << endl;
    }
    ginfo.Bx = data.u / denominator;
    ginfo.By = data.v / denominator + ginfo.ysize * ginfo.Ay;

    pj_free(ref);

  } else if (ginfo.hdf5type == msg) {
#ifdef DEBUGPRINT
	if (hdf5map.count("projdef"))
		cerr << "projdef:" << hdf5map["projdef"] << endl;
#endif
    PJ *ref;
	// Projection must be defined..
	if (!hdf5map.count("projdef"))
	  return -1;
    if ( ! (ref = pj_init_plus(hdf5map["projdef"].c_str()))) {
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.HDF5_head_diana");
  COMMON_LOG::getInstance("common").infoStream() << "error in projdef: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
      return -1;
    }
	if (hdf5map.count("center_lon"))
		data.u = to_float(hdf5map["center_lon"],0.0);
	else
		data.u = 0;
	if (hdf5map.count("center_lat"))
		data.v = to_float(hdf5map["center_lat"],0.0);
	else
		data.v = 0;
    data.u *= DEG_TO_RAD;
    data.v *= DEG_TO_RAD;
    data = pj_fwd(data, ref);

    if (data.u == HUGE_VAL) {
      cerr << "data conversion error" << endl;
    }
    ginfo.Bx = data.u-(ginfo.Ax*ginfo.xsize/2.0);
    ginfo.By = data.v-(ginfo.Ay*ginfo.ysize/2.0) + ginfo.ysize * ginfo.Ay;

    pj_free(ref);
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

  ginfo.AVis = 0;
  ginfo.BVis = 0;
  ginfo.AIr = 0;
  ginfo.BIr = 0;

  // Dont add x_0 or y_0 if they are already there!
  if (ginfo.proj_string.find("+x_0=") == string::npos)
  {
	  std::stringstream tmp_proj_string;
	  tmp_proj_string <<  ginfo.proj_string;
	  if (denominator == 1000.0)
		tmp_proj_string << " +units=km";
	  else
		tmp_proj_string << " +units=m";
	  tmp_proj_string << " +x_0=" << ginfo.Bx * -denominator;
	  tmp_proj_string << " +y_0=" << (ginfo.By*-denominator)+(ginfo.Ay*ginfo.ysize*denominator) + 15;

	  ginfo.proj_string=tmp_proj_string.str();
  }
 

#ifdef DEBUGPRINT
  cerr << "++ metno::satimgh5::HDF5_head_diana ++ ginfo.Ax: " << ginfo.Ax << endl;
  cerr << "++ metno::satimgh5::HDF5_head_diana ++ ginfo.Ay: " << ginfo.Ay << endl;
  cerr << "++ metno::satimgh5::HDF5_head_diana ++ ginfo.Bx: " << ginfo.Bx << endl;
  cerr << "++ metno::satimgh5::HDF5_head_diana ++ ginfo.By: " << ginfo.By << endl;
  cerr << "++ metno::satimgh5::HDF5_head_diana ++ ginfo.trueLat: " << ginfo.trueLat
  << endl;
  cerr << "++ metno::satimgh5::HDF5_head_diana++ ginfo.gridRot: " << ginfo.gridRot
  << endl;
  cerr << "++ metno::satimgh5::HDF5_head_diana++ ginfo.proj_string: " << ginfo.proj_string
  << endl;
#endif
#ifdef M_TIME
  gettimeofday(&post, NULL);
  double s = (((double)post.tv_sec*1000000.0 + (double)post.tv_usec)-((double)pre.tv_sec*1000000.0 + (double)pre.tv_usec))/1000000.0;
  LogHandler::getInstance()->setObjectName("metno.satimgh5.HDF5_head_diana");
  COMMON_LOG::getInstance("common").infoStream() << "HDF5_head_diana took: " << s << " s";
  COMMON_LOG::getInstance("common").infoStream().flush();
#endif
  if (havePalette) // Return color palette
    return 2;
  else
    return 0;
}

bool metno::satimgh5::makePalette(dihead& ginfo, string unit, int backcolour, vector<string> colorvector,bool manualcolors)
{
  /*if (ginfo.noofcl == 255) {
    return false;
  }*/
  int k=0;
  string *pal_name = new string[ginfo.noofcl];
  if(paletteStringMap.size()) {
	//cerr << "paletteStringMap.size(): " << paletteStringMap.size() << endl;
	int pSize = paletteStringMap.size();
    for(int i=1;i<pSize;i++) {
	  //cerr << "i-1: " << i-1 << " i: " << i << " " << paletteStringMap[i] << endl;
      pal_name[i-1] = paletteStringMap[i];
    }
  } else {
    for (map<float,int>::iterator p = paletteMap.begin(); p != paletteMap.end(); p++) {
      pal_name[p->second] = from_number(p->first);
      if (p->second > 0)
        pal_name[p->second-1] += string("-") + from_number(p->first)
        + string(" ") + unit;
    }
    pal_name[ginfo.noofcl-2] = string(">") + pal_name[ginfo.noofcl-2]
                                                        + string(" ") + unit;
    pal_name[ginfo.noofcl-1] = "No Value";
  }

#ifdef DEBUGPRINT
  for (int i=0; i<ginfo.noofcl-1; i++) {
    cerr << "pal_name[" <<i<<"]" << pal_name[i] << endl;
  }
#endif

  for (int i=0; i<ginfo.noofcl; i++) {
    ginfo.clname.push_back(pal_name[i]);
  }
  delete[] pal_name;
  unsigned short int blue[256];
  unsigned short int red[256];
  unsigned short int green[256];
  bool haveHDFPalette = (ginfo.metadata.find("RGBPalette-") != string::npos);
  if(haveHDFPalette) {
    for (int i=0; i<ginfo.noofcl; i++) {
      ginfo.cmap[0][i] = RPalette[i];
      ginfo.cmap[1][i] = GPalette[i];
      ginfo.cmap[2][i] = BPalette[i];
    }

    ginfo.cmap[0][0] = 0;
    ginfo.cmap[1][0] = 0;
    ginfo.cmap[2][0] = 0;

  } else {
    int phase=0;
    for (int i=0; i<255; i++) {
      phase=i/51;
      switch (phase) {
      case 0:
        red[i] = 0;
        green[i] = i*5;
        blue[i] = 255;
        break;
      case 1:
        red[i] = 0;
        green[i] = 255;
        blue[i] = (255-((i-51)*i));
        break;
      case 2:
        red[i] = (i-102)*5;
        green[i] = 255;
        blue[i] = 0;
        break;
      case 3:
        red[i] = 255;
        green[i] = (255-((i-153)*5));
        blue[i] = 0;
        break;
      case 4:
        red[i] = 255;
        green[i] = 0;
        blue[i] = ((i-204)*5);

        break;
      }
    }

    // Backcolor from setup file
    ginfo.cmap[0][0] = backcolour;
    ginfo.cmap[1][0] = backcolour;
    ginfo.cmap[2][0] = backcolour;

    // Grey
    ginfo.cmap[0][ginfo.noofcl] = 53970/255.0;
    ginfo.cmap[1][ginfo.noofcl] = 53970/255.0;
    ginfo.cmap[2][ginfo.noofcl] = 53970/255.0;

    // Take nice values from the palette and put them in the colormap
    for (int i=1; i<ginfo.noofcl; i++) {
      if (manualcolors) {
	if (colorvector[i].find("0x") != string::npos) {
	  unsigned int rgb = strtoul(colorvector[i].c_str(), NULL, 16);
#ifdef DEBUGPRINT
	  cerr << "RGB: " << rgb <<
	    endl;
#endif
	  ginfo.cmap[0][i] = (rgb >> 16) & 0xFF;
	  ginfo.cmap[1][i] = (rgb >> 8) & 0xFF;
	  ginfo.cmap[2][i] = (rgb >> 0) & 0xFF;
	}
	else {
	  if (!is_number(colorvector[i])) {
	    cerr << "Error: " << colorvector[i] << " is not a number" << endl;
	  }
	  else {
	    k= to_int(colorvector[i]);
	    ginfo.cmap[0][i] = red[k];
	    ginfo.cmap[1][i] = green[k];
	    ginfo.cmap[2][i] = blue[k];
	  }
	}
      }
      else {
         ginfo.cmap[0][i] = red[i*(255/ginfo.noofcl)];
         ginfo.cmap[1][i] = green[i*(255/ginfo.noofcl)];
         ginfo.cmap[2][i] = blue[i*(255/ginfo.noofcl)];
      }

#ifdef DEBUGPRINT
    cerr << "-----nice values for radar pallete" << endl;
    cerr << "i ="<< i << "   k =  "<<k << "   red = " <<  ginfo.cmap[0][i]<< "   green = "<<ginfo.cmap[1][i]<<"    blue = " << ginfo.cmap[2][i]  << endl;
#endif

    }
  }

  return true;
}

int metno::satimgh5::day_night(dihead sinfo)
{
  struct ucs upos;
  struct dto d;
  upos.Ax = sinfo.Ax;
  upos.Ay = sinfo.Ay;
  upos.Bx = sinfo.Bx;
  upos.By = sinfo.By;
  upos.iw = sinfo.xsize;
  upos.ih = sinfo.ysize;
  d.ho = sinfo.time.hour();
  d.mi = sinfo.time.min();
  d.dd = sinfo.time.day();
  d.mm = sinfo.time.month();
  d.yy = sinfo.time.year();

  int aa = selalg(d, upos, 5., -2.); //Why 5 and -2? From satsplit.c

  return aa;
}

/*
 * FUNCTION:
 * JulianDay
 *
 * PURPOSE:
 * Computes Julian day number (day of the year).
 *
 * RETURN VALUES:
 * Returns the Julian Day.
 */

int metno::satimgh5::JulianDay(usi yy, usi mm, usi dd)
{
  static unsigned short int daytab[2][13]= { { 0, 31, 28, 31, 30, 31, 30, 31,
      31, 30, 31, 30, 31 },
      { 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 } };
  unsigned short int i, leap;
  int dn;

  leap = 0;
  if ((yy%4 == 0 && yy%100 != 0) || (yy%400 == 0))
    leap=1;

  dn = (int) dd;
  for (i=1; i<mm; i++) {
    dn += (int) daytab[leap][i];
  }

  return (dn);
}

/*
 * NAME:
 * selalg
 *
 * PURPOSE:
 * This file contains functions that are used for selecting the correct
 * algoritm according to the available daylight in a satellite scene.
 * The algoritm uses only corner values to chose from.
 *
 * AUTHOR:
 * Oystein Godoy, met.no/FOU, 23/07/1998
 * MODIFIED:
 * Oystein Godoy, met.no/FOU, 06/10/1998
 * Selection conditions changed. Error in nighttime test removed.
 */

short metno::satimgh5::selalg(const dto& d, const ucs& upos, const float& hmax,
    const float& hmin)
{

  int i, overcompensated[2];
  unsigned int countx, county;
  int size;
  float inclination, hourangle, coszenith, sunh, xval, yval;
  float max = 0., min = 0.;
  float northings, eastings, latitude, longitude;
  //  float RadPerDay = 0.01721420632;
  float DistPolEkv, daynr, gmttime;
  float Pi = 3.141592654;
  float TrueScaleLat = 60.;
  float CentralMer = 0.;
  //  float tempvar;
  float theta0, lat;
  double radian, Rp, TrueLatRad;

  radian = Pi/180.;
  TrueLatRad = TrueScaleLat*radian;
  DistPolEkv = 6378.*(1.+sin(TrueLatRad));
  size = upos.iw*upos.ih;

  /*
   * Decode day and time information for use in formulas.
   */
  daynr = (float) JulianDay((int) d.yy, (int) d.mm, (int) d.dd);
  gmttime = (float) d.ho+((float) d.mi/60.);

  theta0 = (2*Pi*daynr)/365;
  inclination = 0.006918-(0.399912*cos(theta0))+(0.070257*sin(theta0))
  -(0.006758*cos(2*theta0))+(0.000907*sin(2*theta0)) -(0.002697*cos(3
      *theta0))+(0.001480*sin(3*theta0));

  for (i = 0; i < 2; i++) {
    overcompensated[i] = 0;
  }

  /*
   Estimates latitude and longitude for the corner pixels.
   */
  countx = 0;
  county = 0;
  for (i = 0; i < 4; i++) {
    xval = upos.Bx + upos.Ax*((float) countx + 0.5);
    yval = upos.By - fabsf(upos.Ay)*((float) county + 0.5);

    countx += upos.iw;
    if (countx > upos.iw) {
      countx = 0;
      county += upos.ih;
    }
    northings = yval;
    eastings = xval;

    Rp = pow(double(eastings*eastings + northings*northings), 0.5);

    latitude = 90.-(1./radian)*atan(Rp/DistPolEkv)*2.;
    longitude = CentralMer+(1./radian)*atan2(eastings, -northings);

    latitude=latitude*radian;
    longitude=longitude*radian;

    /*
     * Estimates zenith angle in the pixel.
     /read*/
    lat = gmttime+((longitude/radian)*0.0667);
    hourangle = fabsf(lat-12.)*0.2618;

    coszenith = (cos(latitude)*cos(hourangle)*cos(inclination))
    + (sin(latitude)*sin(inclination));
    sunh = 90.-(acosf(coszenith)/radian);

    if (sunh < min) {
      min = sunh;
    } else if (sunh > max) {
      max = sunh;
    }

  }

  /*
   hmax and hmin are input variables to the function determining
   the maximum and minimum sunheights. A twilight scene is defined
   as being in between these limits. During daytime scenes all corners
   of the image have sunheights larger than hmax, during nighttime
   all corners have sunheights below hmin (usually negative values).

   Return values,
   0= no algorithm chosen (twilight)
   1= nighttime algorithm
   2= daytime algorithm.
   */
  if (max > hmax && fabs(max) > (fabs(min)+hmax)) {
    return (2);
  } else if (min<hmin && fabs(min) >(fabs(max)+hmin)) {
    return (1);
  } else {
    return (0);
  }
  return (0);
}
