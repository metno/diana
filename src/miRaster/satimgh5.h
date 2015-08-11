/*
 libdiHDF5 - SMHI HDF5 interface

 Copyright (C) 2006-2013 met.no

 Contact information:
 Norwegian Meteorological Institute
 Box 43 Blindern
 0313 OSLO
 NORWAY
 email: diana-smhi@met.no

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

/*
 * PURPOSE:
 * Header file for module reading from HDF5 file data and computing
 * satellite geometry.
 */

#ifndef _AUSATH5_H
#define _AUSATH5_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <hdf5.h>
#include <map>
#include <puTools/miStringFunctions.h>
#include "satimg.h"
#include "ImageCache.h"

using namespace std;
using namespace satimg;

namespace metno {
/**
    \brief The HDF5 reader support class

    Reads metadata and image data from HDF5 files.

 */

class satimgh5 {
  //private:
  //  static ImageCache* mImageCache;

public:

/**
 * Enumerator used to specifiy specifics for the different satellites and radar types.
 */

 enum hdf5type {
  radar = 0,
  noaa = 1,
  msg = 2,
  saf = 3
};


/*!
 Used to extract strings for HDF5 files.
 */

struct myString {
  char str[1024];
};

struct outval_name {
  char str[129];
};

/*!
 Used to extract float arrays from HDF5 files.
 */

struct arrFloat {
  double f[1024];
};


/**
 * Used to calculate if the image is taken during the day or night.
 * This information is used to figure out if a visual image should be shown or not.
 */
static int day_night(dihead sinfo);

/**
 * Reads the imagedata of the HDF5 file with the help of metadata read in HDF5_head_diana
 * @param infile - name of HDF5 file to read from
 * @param image - char array to be shown as image in diana
 * @param orgimage - float data array with temperature values to be shown per pixel in diana
 */
static int HDF5_read_diana(const string& infile, unsigned char *image[], float *orgimage[], int nchan, int chan[], dihead& ginfo);

/**
 * Reads the metadata of the HDF5 file and fills the dihead with data
 * @param infile - name of HDF5 file to read from
 * @param ginfo - internal diana structure to store metadata for sat and radar images
 */
static int HDF5_head_diana(const string& infile, dihead &ginfo);

/**
 * Checks if the metadata has been read for the specific file. If filename and metadata are already in the map,
 * the data is untouched and no futher reading of the HDF5 file is done. If filename or metadata differs from the
 * data in the map the data is discarded and read from the HDF5 file again.
 * @param filename - filename of the current HDF5 file
 * @param metadata - metadata form configuration file
 */
static bool checkMetadata(string filename,string metadata);

private:
/**
 * If an attribute is found in the current block it is written to the internal map structure.
 * @param dataset - group (or dataset) to process
 * @param index - internal HDF5 file index of the group (or dataset)
 * @param path - path to the group (or dataset)
 */
  static  int getAttributeFromGroup(hid_t& dataset, int index, string path);

/**
 * If a group is found in the current block it is opened by this function that calls itself recursively to traverse subgroups.
 * Stores the data in hdf5map with path and value.
 * @param group - group (or dataset) to process
 * @param groupname - name of the group (or dataset) to process
 * @param path - path to the group (or dataset) to process
 * @param metadata - metadata from configuration file
 */
  static  int openGroup(hid_t group, string groupname, string path, string metadata);

/**
 * If a dataset is found the metadata part of the dataset is read. Support for compound datasets is included.
 * @param root - node to be processed
 * @param dataset - name of the dataset
 * @param path - path to the dataset in the file
 * @param metadata - metadata from configuration file
 */
  static  int openDataset(hid_t root, string dataset, string path, string metadata);

/**
 * Simple insert to the internal map structure.
 * @param fullpath - full path to the value in the file.
 * @param value - value of the attribute
 */
  static  int insertIntoValueMap(string fullpath, string value);

/**
 * Dump the values of the internal map structures to console. Used ony in development and debugging.
 */
  static  int getAllValuesFromMap();

/**
 * Function to calculate how much light the image has. Used to determine whether to show a visual satellite channel.
 */
  static  short selalg(const dto& d, const ucs& upos, const float& hmax, const float& hmin);

/**
 * Calculates the Julian day of the year.
 * @param yy - year
 * @param mm - month
 * @param dd - day
 */
  static  int JulianDay(usi yy, usi mm, usi dd);

/**
 * Fills the internal structure dihead with metadata from the HDF5 metadata.
 * @param inputStr - metadata= in diana.setup
 */
  static herr_t fill_head_diana(string inputStr, int chan);

/**
 * Get the channel name that the channel has in the HDF5 file.
 *
 * @param inputStr - data from configuration file
 * @param data - channelname placed here if successfull
 *
 */
  static  herr_t getDataForChannel(string& inputStr, string& data);

/**
 * Creates a palette based on infomation from the configuration file.
 * @param ginfo - the ginfo object
 * @param unit - unit for the palette (dbz, mm, ...)
 * @param backcolour - greyscale background color for palette.
 * @param colorvector - steps in colormap for selected pallete colors.
 * @param manualcolors - True if paletteinfo in setupfile is specified with value:color.
 */
  static  bool makePalette(dihead& ginfo, string unit, int backcolour,
      std::vector<string> colorvector, bool manualcolors);

/**
 * Validates the format of the string from configuration file.
 * @param inputStr - string to be validated.
 *
 * TODO: Implement this function.
 *
 */
  static  bool validateChannelString(string& inputStr);

/**
 * Parses the inputStr and places information in the output parameters
 *
 * @param inputStr from configuration file
 * @param chan - channel number
 * @param chpath - path to the channel in the HDF5 file
 * @param chname - name of the datatable where the data is stored in the HDF5 file
 * @param chinvert - True if the channel should be inverted or not (*-1)
 * @param subtract - True if the channel is to be part of substraction with another channel
 * @param subchpath - path to the channel to be subtracted
 * @param subchname - name of the channel to be subtracted
 * @param subinvert - True if the channel to be subtracted should be inverted
 * @param ch4co2corr - True if channel (1) is channel 4 and should be co2 corrected.
 * @param subch4co2corr - True if subchannel (2) is channel 4 and should be co2 corrected.
 *
 */
  static herr_t getDataForChannel(string inputStr, int chan, string& chpath, string& chname, bool& chinvert,
      bool& subtract, string& subchpath, string& subchname, bool& subchinvert, bool& ch4co2corr, bool& subch4co2corr);

/**
 * Read the data part of the channel in the HDF5 file.
 * The data is processed and put in the int_data[] array.
 * orgimage is filled with unprocessed data.
 *
 * @param ginfo - metadata
 * @param source - the data source (dataset)
 * @param path - path to the dataset
 * @param name - name of the dataset
 * @param invert - True if invert should be done
 * @param float_data - the float array holding the data
 * @param chan - internal channelnumber of the image array
 * @param orgImage - the original image data, used for cloudTopTemperature.
 * @param cloudTopTemperature - True if orgImage should be filled.
 * @param haveCachedImage - True if the image is already cached, will only
 * fill orgImage (if cloudTopTemperature is True)
 *
 */
  static int readDataFromDataset(dihead& ginfo, hid_t source, string path, string name,
      bool invert, float *float_data[], int chan, float *orgImage[],
      bool cloudTopTemperature, bool haveCachedImage);

/**
 * Without original image for radar images
 * @param ginfo - metadata
 * @param source - the data source (dataset)
 * @param path - path to the dataset
 * @param name - name of the dataset
 * @param invert - True if invert should be done
 * @param float_data - the float array holding the data
 * @param chan - internal channelnumber of the image array
 *
 */
  static int readDataFromDataset(dihead& ginfo, hid_t source, string path, string name,
      bool invert, float *float_data[], int chan);

/**
 * Process data from int_data and places it in channel 0 of the image array
 * @param image - the image array to be returned to Diana
 * @param float_data - the data
 */
  static int makeImage(unsigned char *image[], float* float_data[]);

/**
 * Process data from int_data and places it in channel chan of float_data
 * @param image - the image array to be returned to Diana
 * @param float_data - the data
 * @param chan - channel number, determines where in image[] float_data
 * should be written (0 for R, 1 for G and 2 for B).
 */
  static int makeImage(unsigned char *image[], float* float_data[], int chan);

/**
 * Places unprocessed data from int_data to the orgimage channel chan
 * @param orgimage - array to hold original (Kelvin) values.
 * @param float_data - the data array
 * @param chan - channel number (0=R, 1=G, 2=B).
 */
  static int makeOrgImage(float* orgimage[], float* float_data[], int chan, string name);

/**
 * Subtracts all the values from int_data with the values from int_data_sub.
 * The result is placed in float_data
 * @param float_data - channel to be subtracted
 * @param float_data_sub - channel to subtract
 */
  static int subtractChannels(float* float_data[], float* float_data_sub[]);

/**
 * Help function to check the type of a data. If it is a group of a dataset etc.
 * @param dataset - dataset (or group) to be checked.
 * @param name - name of the dataset (or group)
 */
  static hid_t checkType(hid_t dataset, string name);

/**
 * Returns the palettestep for the specified value.
 * @param value - the value to be translated into a palettestep
 * @returns - the palettestep
 */
  static int getPaletteStep(float value);

/**
 * Converts datestrings with alpha months (JAN,FEB,...)
 * @param date -date with the format <day>-<month>-<year> where month is a string. The
 * date is converted to <year>-<month>-<day> where month is alphamumeric.
 */
  static string convertAlphaDate(string date);

/**
 * CO2 correction of the MSG 3.9 um channel:
 *
 * T4_CO2corr = (BT(IR3.9)^4 + Rcorr)^0.25
 * Rcorr = BT(IR10.8)^4 - (BT(IR10.8)-dt_CO2)^4
 * dt_CO2 = (BT(IR10.8)-BT(IR13.4))/4.0
 * @param ginfo - the ginfo object
 * @param source - pointing to the open HDF5 file
 * @param ch4r - array to hold the result
 * @param invert - true if the result should be inverted (4ri)
 * @param chan - the channel number
 *
 */
  static int co2corr_bt39(dihead& ginfo, hid_t source, float* ch4r[], bool invert, int chan);


/**
 * Internal map structures.
 */
  static std::map<string, string> hdf5map;
  static std::map<float, int> paletteMap;
  static std::map<int, string> paletteStringMap;
  static std::vector<int> RPalette;
  static std::vector<int> GPalette;
  static std::vector<int> BPalette;
  static std::map <string, std::vector<float> > calibrationTable;
  static std::map <string, string> metadataMap;
};
}
#endif /* _AUSATH5_H */
