/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2018 met.no and SMHI

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

#include "diObsRoad.h"

#include "diLabelPlotCommand.h"
#include "diUtilities.h"

// from kvroadapi
#ifdef NEWARK_INC
#include <newarkAPI/diParam.h>
#include <newarkAPI/diRoaddata.h>
#include <newarkAPI/diStation.h>
#else
#include <roadAPI/diParam.h>
#include <roadAPI/diRoaddata.h>
#include <roadAPI/diStation.h>
#endif

#include <mi_fieldcalc/MetConstants.h>

#include <puTools/miStringFunctions.h>

#include <algorithm>
#include <fstream>
#include <vector>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <regex>

#define MILOGGER_CATEGORY "diana.ObsRoad"
#include <miLogger/miLogging.h>

//#define DEBUGPRINT 1
//#define DEBUG_OBSDATA 1

#define _undef -32767.0
// headerfilename, dianaparameter name, headerfile items corresponding to lines
// first line in map is the header
std::map<std::string, std::map<std::string, std::vector<std::string>>> ObsRoad::parameterMap;
std::map<std::string, ObsDataVector_p> ObsRoad::obsDataCache;
std::map<std::string, time_t> ObsRoad::obsDataTimeStamp;

//remove more than one " ". 
const std::regex sanitizeSpaces("\\s\\s+");

using namespace road;
using namespace miutil;

ObsRoad::ObsRoad(const std::string& filename, const std::string& databasefile, const std::string& stationfile, const std::string& headerfile,
                 const miTime& filetime, ObsDataRequest_cp request, bool breadData)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif
  // init members  
  vObsData=ObsDataVector_p(new ObsDataVector());
  headerRead = false;
  filename_ = filename;
  databasefile_ = databasefile;
  stationfile_ = stationfile;
  headerfile_ = headerfile;
  filetime_ = filetime;
  if (!breadData) {
    readHeader(request);
    headerRead = true;
  } else
    readData(vObsData,request);
}

void ObsRoad::readHeader(ObsDataRequest_cp request)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("headerfile: " << headerfile_);
#endif
  int n, i;
  std::vector<std::string> vstr, pstr;
  std::string str;
  plotTime = filetime_;
  timeDiff = 60;
  if (request) {
    plotTime = request->obstime;
    timeDiff = request->timeDiff;
  }
  fileTime = filetime_;
  // Check if old format
  bool old_format = true;
  std::vector<std::string> filename_part = miutil::split(headerfile_, ".");
  if (filename_part.size() >= 2)
	if (filename_part[filename_part.size()-1] == "csv"){
		old_format = false;
	}
  if (old_format) {
    int theresult = diParam::initParameters(headerfile_);
    if (theresult) {
      // take a local copy
      std::vector<diParam>* params = NULL;
      std::map<std::string, std::vector<diParam>*>::iterator itp = diParam::params_map.find(headerfile_);
      if (itp != diParam::params_map.end()) {
        params = itp->second;
      }
      /* this should not happen if configured properly */
      if (params == NULL) {
        // oplot->roadobsHeader = false;
        METLIBS_LOG_ERROR(" ObsRoad::readHeader() error, parameterfile: " << headerfile_);
#ifdef DEBUGPRINT
        METLIBS_LOG_DEBUG("++ ObsRoad::readHeader() done, error finding parameters ++");
#endif
      }

      // Construct a header line like this
      //[NAME UALF_Lyn]
      //[COLUMNS
      //Ver:f:"UALF versjons nummer" Year:year:"aar" Month:month:"Maaned" Day:day:"Dag" Hour:hour:"Time" Min:min:"Min" Sec:sec:"Sekund" Ns:f:"Nano sekunder"
      //Lat:lat:"Breddegrad" Lon:lon:"Lengdegrad" Pk:f:"Beregnet maksstroem i kA" Fd:f:"Multiplisitet for <<flash>> data(1 - 99) eller 0 for <<strokes>>" No_sens:f:"Antall sensorer med bidrag til beregningen" Df:f:"Antall frihetsgrader" Ea:r:"Ellipse vinkel fra 0 grader Nord" Major:r:"Lengste hovedakse i km" Minor:r:"Minste hovedakse i km" Chi:r:"Chi-kvadrat fra posisjons beregningen, 0-3.0 bra, 3.0-10.0 akseptabelt, >10.0 daarlig" Rise:r:"Stigetid for boelgeformen i mikrosekunder" Pz:r:"Tiden fra maks- til 0-stroem for boelgeformen i mikrosekunder" Mrr:r:"Maks stigningsforhold til boelgeformen i kA/us" Ci:f:"1 Sky-Sky, 0 sky-bakke" Ai:f:"1 vinkel data benyttet til aa beregne posisjonen, 0 ellers" Sig_i:f:"1 hvis sensor signal data er brukt til aa beregne posisjonen. 0 ellers" Ti:f:"1 hvis timing data er brukt til aa beregne posisjonen, 0 elles."
      //DeltaTime:deltatime:"Delta Time"]
      //[DATA]
      // Just in case ...
      lines.clear();
      std::string line;
      line = "[NAME MORA_OBSERVATIONS]";
      lines.push_back(line);
      line = "[COLUMNS";
      lines.push_back(line);
      // The fixed part..
      line = "StationName:s: Name:id: Date:d: Time:t: Lat:lat: Lon:lon: ";
      // the dynamic part
      // the data value parameters
      for (i = 0; i < params->size(); i++) {
        std::string name = (*params)[i].diananame();
        miutil::trim(name);
        line = line + name + ":r:" + (*params)[i].unit() + " ";
      }
      // The encloseing bracket
      line += "]";
      // cerr << line << endl;
      lines.push_back(line);
      // The fixed data line tells decode header to stop parsing
      line = "[DATA]";
      lines.push_back(line);
      // Now we should be ready to decode it...
      separator.clear();
      decodeHeader();
    } // end if theresult
  }   // end if true
  else {
    // Check if headerfile has been read before.
	// std::map<std::string, std::map<std::string, std::vector<std::string>>> ObsRoad::parameterMap;
	std::map<std::string, std::map<std::string, std::vector<std::string>>>::iterator its=ObsRoad::parameterMap.begin();
	// search
	its = ObsRoad::parameterMap.find(headerfile_);
	if (its == ObsRoad::parameterMap.end()) {
	  size_t i = 0;
	  size_t heading_size = 0;
	  std::ifstream ifs(headerfile_.c_str(), std::ios::in);
	  char buf[1024];
	  if (ifs.is_open()) {
		while (ifs.good()) {
		  ifs.getline(buf, 1024);
		  std::string str(buf);
		  //miutil::trim(str);
		  // Append every non empty line to lines
		  // Empty lines allowed
		  if (!str.empty()) {
		    // decode the headerfile and put it in map
			std::vector<std::string> pstr = miutil::split(str, ";");
			if (i == 0)
				heading_size=pstr.size();
			// Check if format is incorrect
			// Each lite must be equal in length
			if (pstr.size() != heading_size) {
				METLIBS_LOG_ERROR(" ObsRoad::readHeader() error in parameterfile: " << headerfile_ << " line: " << str);
				return;
			}
			// Remove '"'
			for (size_t i = 0; i < pstr.size(); i++)
			{
				miutil::remove(pstr[i], '"');
			}
			// The diana name is always column 0
			parameterlines[pstr[0]] = pstr;
			i++;
		  }
		}
		ifs.close();
	  }
	  ObsRoad::parameterMap[headerfile_]=parameterlines;
	 } else {
	   parameterlines=ObsRoad::parameterMap[headerfile_];	
	 }
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ ObsRoad::readHeader()  done ++");
#endif
}

void ObsRoad::getStationList(std::vector<stationInfo>& stations)
{
  // This creates the stationlist
  // We must also init the connect string to mora db
  Roaddata::initRoaddata(databasefile_);
  diStation::initStations(stationfile_);
  // get the pointer to the actual station vector
  std::map<std::string, std::vector<diStation>*>::iterator its = diStation::station_map.find(stationfile_);
  if (its == diStation::station_map.end()) {
    METLIBS_LOG_ERROR("Unable to find stationlist: '" << stationfile_ << "'");
    return;
  }
  const std::vector<diStation>& stationlist = *its->second;

  if (stationlist.empty()) {
    METLIBS_LOG_WARN("Empty stationlist: '" << stationfile_ << "'");
    return;
  }
  for (size_t i = 0; i < stationlist.size(); i++) {
    stationInfo st("", 0, 0);
    if (stationlist[i].station_type() == "WMO")
      st.name = stationlist[i].name();
    else if (stationlist[i].station_type() == "ICAO")
      st.name = stationlist[i].ICAOID();
    else
      st.name = stationlist[i].call_sign();
    st.lat = stationlist[i].lat();
    st.lon = stationlist[i].lon();
    stations.push_back(st);
  }
}

void ObsRoad::initData(ObsDataRequest_cp request)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_
                                 << " filetime= " << filetime_.isoTime());
#endif
  // read the headerfile if needed
  readHeader(request);
  initRoadData(request);
}

void ObsRoad::initRoadData(ObsDataRequest_cp request)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_
                                 << " filetime= " << filetime_.isoTime());
#endif
// Does not work, why ?
// if (!oplot->isallObs())
//	oplot->clear(); // ???
// oplot->clearVisibleStations();
#if 0
  oplot->setLabels(labels);
  oplot->columnName = m_columnName;
#endif
  plotTime = filetime_;
  timeDiff = 60;
  if (request) {
    plotTime = request->obstime;
    timeDiff = request->timeDiff;
  }
  fileTime = filetime_;

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, filetime_);
  std::map<std::string, std::vector<diStation>*>::iterator its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end()) {
    stationlist = its->second;
  }

  std::map<int, std::string> lines_map;
  // get av vector of paramater names
  std::vector<std::string> paramnames;
  int theresult = diParam::initParameters(headerfile_);
  if (theresult) {
    // take a local copy
    std::vector<diParam>* params = NULL;
    std::map<std::string, std::vector<diParam>*>::iterator itp = diParam::params_map.find(headerfile_);
    if (itp != diParam::params_map.end()) {
      params = itp->second;
    }
    /* this should not happen if configured properly */
    if (params == NULL) {
      // oplot->roadobsHeader = false;
      METLIBS_LOG_ERROR("parameterfile: " << headerfile_);
#ifdef DEBUGPRINT
      METLIBS_LOG_DEBUG("error finding parameters");
#endif
    } else {
      // the dynamic part
      // the data value parameters
      for (int i = 0; i < params->size(); i++) {
        std::string name = (*params)[i].diananame();
        miutil::trim(name);
        paramnames.push_back(name);
      }
    }
  }
  road.initData(paramnames, lines_map);
  // decode the data...
  int stnid;
  std::string str;
  std::map<int, std::string>::iterator it = lines_map.begin();
  for (; it != lines_map.end(); it++) {
    // FIXME: Stnid !!!!
    // INDEX in station list
    stnid = it->first;
    str = it->second;
    miutil::trim(str);
    // Append every line to
    lines.push_back(str);
  }

  separator = "|";

  decodeData();
// FIXME, no longer use of dummy data  
#if 0
  oplot->addObsData(vObsData);

  // Force setting of dummy data
  oplot->setData();
// clear plot positions
// oplot->clearVisibleStations();
// make a dummy plot to compute a list of stations to be plotted
   oplot->preparePlot();
#endif
}

void ObsRoad::readData(ObsDataVector_p & obsdata, ObsDataRequest_cp request)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_
                                 << " filetime= " << filetime_.isoTime());
#endif
  // read the headerfile if needed
  struct stat buf;
  if (obsDataTimeStamp.count(filename_) == 0) {
	readHeader(request);
	readRoadData(request);
	obsDataCache[filename_] = vObsData;
	if(!stat(filename_.c_str(), &buf))
	{
		obsDataTimeStamp[filename_] = buf.st_mtime;
	} else {
		// Something went wrong, set to zero...
		obsDataTimeStamp[filename_] = 0;
	}
	obsdata=vObsData;
  } else {
	if(!stat(filename_.c_str(), &buf))
	{
		// file have changed on disk
		if (buf.st_mtime > obsDataTimeStamp[filename_])
		{
			readHeader(request);
			readRoadData(request);
			obsDataCache[filename_] = vObsData;
			obsDataTimeStamp[filename_] = buf.st_mtime;
			obsdata=vObsData;
		} else {
			obsdata = obsDataCache[filename_];
		}
	} else {
		// file removed from disk, remove from both caches
		obsDataTimeStamp.erase(filename_);
		obsDataCache.erase(filename_);
		obsdata.get()->clear();
	}
  }
}

void ObsRoad::readRoadData(ObsDataRequest_cp request)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE("filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_
                                 << " filetime= " << filetime_.isoTime());
#endif

#if 0  
  oplot->setLabels(labels);
  oplot->columnName = m_columnName;
#endif
  bool new_format = false;
  datatype_ = "";
  std::vector<std::string> filename_part = miutil::split(filename_, ".");
  if (filename_part.size() >= 2)
	if (filename_part[filename_part.size()-1] == "csv") {
		new_format = true;
		std::vector<std::string> filename_base_part = miutil::split(filename_, "/");
		if (filename_base_part.size() >= 1) {
			std::string tmp_datatype=filename_base_part[filename_base_part.size()-1];
			std::vector<std::string> filename_datatype = miutil::split(tmp_datatype, "_");
			// There must always be one _ in filename
			if (filename_datatype.size() >=2) {
				datatype_ = filename_datatype[0];
			}
		}
	}
  plotTime = filetime_;
  timeDiff = 60;
  if (request) {
    plotTime = request->obstime;
    timeDiff = request->timeDiff;
  }
  fileTime = filetime_;

  lines.clear();
  struct stat stat_buf;
  stat(filename_.c_str(), &stat_buf);
  int fsize = stat_buf.st_size;
  std::ifstream ifs(filename_, std::ios::in | std::ios::binary | std::ios::ate);
  if (ifs.is_open()) {
    ifs.seekg(std::ios::beg);
    std::string inResultStr(fsize,0);
    ifs.read(&inResultStr[0], fsize);
    ifs.close();
    if (new_format) {
      inResultStr.erase(std::remove(inResultStr.begin(), inResultStr.end(), '"'), inResultStr.end());      
    }
    lines=miutil::split(inResultStr, "\n", false);
    if (!new_format) {
      for (size_t i = 0; i < lines.size(); i++)
      {
          miutil::trim(lines[i]);
      }
    }
  }
  // decode the header part and data...
  if (new_format) {
  
	decodeNewHeader();

	separator = ";";

	// remove the fake data....
	vObsData.get()->clear();
	mObsData.clear();

	decodeNewData();
  } else {
	decodeHeader();

	separator = "|";

	// remove the fake data....
	vObsData.get()->clear();
	mObsData.clear();

	decodeData();
  }
  // Dont set data here, the obsReaderRoad will do that...
  // HERE, we should be finished!
#if 0
  oplot->replaceObsData(vObsData);

  oplot->clearVisibleStations();
#endif
  
}

// from ObsAscii

// FIXME: Not used for now.
void ObsRoad::readDecodeData()
{
  METLIBS_LOG_SCOPE();
  if (m_needDataRead)
    readData(m_filename);
  decodeData();
}

// FIXME: Not used for now..
void ObsRoad::readData(const std::string& filename)
{
  METLIBS_LOG_SCOPE();
  if (not diutil::getFromAny(filename, lines))
    METLIBS_LOG_WARN("could not read '" << filename << "'");
}

static void erase_comment(std::string& line, const std::string& comment = "#")
{
  const size_t cpos = line.find(comment);
  if (cpos != std::string::npos)
    line.erase(cpos);
}

bool ObsRoad::bracketContents(std::vector<std::string>& in_out)
{
  METLIBS_LOG_SCOPE();
  if (in_out.empty())
    return true;

  std::string joined = in_out[0];
  for (size_t i = 1; i < in_out.size(); ++i)
    joined += " " + in_out[i];
  METLIBS_LOG_DEBUG(LOGVAL(joined));

  in_out.clear();

  size_t start = 0;
  while (start < joined.length()) {
    size_t p_open = joined.find('[', start);
    if (p_open == std::string::npos)
      break;
    size_t p_close = joined.find(']', p_open + 1);
    if (p_close == std::string::npos)
      return false;
    in_out.push_back(joined.substr(p_open + 1, p_close - p_open - 1));
    METLIBS_LOG_DEBUG(LOGVAL(in_out.back()));
    start = p_close + 1;
  }
  return true;
}

void ObsRoad::parseHeaderBrackets(const std::string& str)
{
  METLIBS_LOG_SCOPE(LOGVAL(str));
  std::vector<std::string> pstr = miutil::split_protected(str, '"', '"');
  if (pstr.size() <= 1)
    return;

  if (pstr[0] == "COLUMNS") {
    if (not separator.empty())
      pstr = miutil::split(str, separator);
    for (size_t j = 1; j < pstr.size(); j++) {
      miutil::remove(pstr[j], '"');
      const std::vector<std::string> vs = miutil::split(pstr[j], ":");
      if (vs.size() > 1) {
        m_columnName.push_back(vs[0]);
        m_columnType.push_back(miutil::to_lower(vs[1]));
        if (vs.size() > 2) {
          m_columnTooltip.push_back(vs[2]);
        } else {
          m_columnTooltip.push_back("");
        }
      }
    }
  } else if (pstr[0] == "UNDEFINED") {
    const std::vector<std::string> vs = miutil::split(pstr[1], ",");
    asciiColumnUndefined.insert(vs.begin(), vs.end());
  } else if (pstr[0] == "SKIP_DATA_LINES" && pstr.size() > 1) {
    asciiSkipDataLines = miutil::to_int(pstr[1]);
  } else if (pstr[0] == "LABEL") {
    labels.push_back(LabelPlotCommand::fromString(str.substr(5)));
  } else if (pstr[0] == "SEPARATOR") {
    separator = pstr[1];
  }
}

void ObsRoad::parseNewHeaderFormat(const std::string& str)
{
  METLIBS_LOG_SCOPE(LOGVAL(str));
  std::vector<std::string> pstr = miutil::split(str, ";");
  // Check if format is incorrect
  if (pstr.size() <= 1)
    return;
  
  for (size_t j = 0; j < pstr.size(); j++)
  {
	  m_columnName.push_back(pstr[j]);
	  // Make index to headeer items and save this.
	  asciiColumn[pstr[j]] = j;
	  // For now, this will be enough
	  // Specify this in setup file
	  m_columnType.push_back("");
	  m_columnTooltip.push_back("");
  }
}

void ObsRoad::decodeHeader()
{
  METLIBS_LOG_SCOPE();

  std::vector<std::string> vstr;
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string& line = lines[i]; // must be reference here, used later in decodeData
    erase_comment(line);
    miutil::trim(line);
    if (line.empty())
      return;

    if (line == "[DATA]")
      // end of header, start data
      break;
    METLIBS_LOG_DEBUG(LOGVAL(line));
    vstr.push_back(line);
  }

  asciiColumn.clear();
  asciiSkipDataLines = 0;

  m_columnType.clear();
  m_columnName.clear();
  m_columnTooltip.clear();
  asciiColumnUndefined.clear();

  // parse header

  if (not bracketContents(vstr)) {
    METLIBS_LOG_ERROR("bad header, cannot find closing ']'");
    fileOK = false;
    return;
  }
  for (size_t i = 0; i < vstr.size(); ++i)
    parseHeaderBrackets(vstr[i]);

  METLIBS_LOG_DEBUG("#columns: " << m_columnType.size() << LOGVAL(asciiSkipDataLines));

  knots = false;
  for (size_t i = 0; i < m_columnType.size(); i++) {
    METLIBS_LOG_DEBUG("column " << i << " : " << m_columnName[i] << "  " << m_columnType[i]);

    const std::string& ct = m_columnType[i];
    const std::string ct_lower = miutil::to_lower(ct);
    const std::string cn_lower = miutil::to_lower(m_columnName[i]);

    if (ct == "d")
      asciiColumn["date"] = i;
    else if (ct == "t")
      asciiColumn["time"] = i;
    else if (ct == "year")
      asciiColumn["year"] = i;
    else if (ct == "month")
      asciiColumn["month"] = i;
    else if (ct == "day")
      asciiColumn["day"] = i;
    else if (ct == "hour")
      asciiColumn["hour"] = i;
    else if (ct == "min")
      asciiColumn["min"] = i;
    else if (ct == "sec")
      asciiColumn["sec"] = i;
    else if (ct_lower == "lon")
      asciiColumn["x"] = i;
    else if (ct_lower == "lat")
      asciiColumn["y"] = i;
    else if (ct_lower == "dd")
      asciiColumn["dd"] = i;
    else if (ct_lower == "ff") // Wind speed in m/s
      asciiColumn["ff"] = i;
    else if (ct_lower == "ffk") // Wind speed in knots
      asciiColumn["ff"] = i;
    else if (ct_lower == "image")
      asciiColumn["image"] = i;
    else if (cn_lower == "lon" && ct == "r") // Obsolete
      asciiColumn["x"] = i;
    else if (cn_lower == "lat" && ct == "r") // Obsolete
      asciiColumn["y"] = i;
    else if (cn_lower == "dd" && ct == "r") // Obsolete
      asciiColumn["dd"] = i;
    else if (cn_lower == "ff" && ct == "r") // Obsolete
      asciiColumn["ff"] = i;
    else if (cn_lower == "ffk" && ct == "r") // Obsolete
      asciiColumn["ff"] = i;
    else if (cn_lower == "image" && ct == "s") // Obsolete
      asciiColumn["image"] = i;
    else if (cn_lower == "name" || ct == "id")
      asciiColumn["Name"] = i;

    if (ct_lower == "ffk" || cn_lower == "ffk")
      knots = true;
  }

  fileOK = true;
  return;
}

void ObsRoad::decodeNewHeader()
{
  METLIBS_LOG_SCOPE();
  
  // New header format: Time;Offset;StationId;StationName;StationType;Parameter;StatisticsFormula;SamplingTime;Longitud;Latitud;Height;Presentation Value;Database Value;Quality;Nordklim;Restriction;Value Origin;Level Parameter;Level from;Level to;ClimateStationId;ClimateCtationName;NationalStationId;NationalStationName;WmoStationId;WmoStationName;IcaoStationId;IcaoStationName;OperatingMode;
  // Always the same 

  std::vector<std::string> vstr;
  for (size_t i = 0; i < lines.size(); ++i) {
    std::string& line = lines[i]; // must be reference here, used later in decodeData
	// Check for invalid format in data file.
    if (line.empty())
      return;
    // In the new format, heading is always line no 0.
    if (i == 1)
      // end of header, start data
      break;
    METLIBS_LOG_DEBUG(LOGVAL(line));
    vstr.push_back(line);
  }

  asciiColumn.clear();
  asciiSkipDataLines = 0;
  parameterColumn.clear();

  m_columnType.clear();
  m_columnName.clear();
  m_columnTooltip.clear();
  asciiColumnUndefined.clear();
  
  m_parameterColumnName.clear();

  // parse header
  // Makes column index for asciiColumn
 
  for (size_t i = 0; i < vstr.size(); ++i)
    parseNewHeaderFormat(vstr[i]);

  METLIBS_LOG_DEBUG("#columns: " << m_columnName.size() << LOGVAL(asciiSkipDataLines));
  
  // std::map<std::string, std::vector<std::string>> parameterlines;
  // get the heading and index it!  
  std::vector<std::string> pstr = parameterlines["DianaParameter"];
  for (size_t j = 0; j < pstr.size(); j++)
  {
	  m_parameterColumnName.push_back(pstr[j]);
	  // Make index to headeer items and save this.
	  parameterColumn[pstr[j]] = j;
  }
  fileOK = true;
  return;
}

ObsRoad::string_size_m::const_iterator ObsRoad::getColumn(const std::string& cn, const std::vector<std::string>& cv) const
{
  string_size_m::const_iterator it = asciiColumn.find(cn);
  if (it == asciiColumn.end() or it->second >= cv.size() or asciiColumnUndefined.count(cv[it->second]))
    return asciiColumn.end();
  else
    return it;
}

bool ObsRoad::getColumnValue(const std::string& cn, const diutil::string_v& cv, float& value) const
{
  string_size_m::const_iterator it = getColumn(cn, cv);
  if (it == asciiColumn.end())
    return false;

  value = miutil::to_float(cv[it->second]);
  return true;
}

bool ObsRoad::getColumnValue(const std::string& cn, const diutil::string_v& cv, int& value) const
{
  string_size_m::const_iterator it = getColumn(cn, cv);
  if (it == asciiColumn.end())
    return false;

  value = miutil::to_int(cv[it->second]);
  return true;
}

bool ObsRoad::getColumnValue(const std::string& cn, const diutil::string_v& cv, std::string& value) const
{
  string_size_m::const_iterator it = getColumn(cn, cv);
  if (it == asciiColumn.end())
    return false;

  value = cv[it->second];
  return true;
}

std::string ObsRoad::getParameterColumn(const std::string& cn, const std::vector<std::string>& cv)
{
  // Get parameter description from parameterline map
  std::string result;
  std::map<std::string, std::vector<std::string>>::const_iterator it = parameterlines.find(cn);
  if (it == parameterlines.end()) {
    return result;
  }
  std::vector<std::string> pstr = it->second;
  // Map the parameter description with the current data line (cv)
  // Check for Parameter;StatisticsFormula;SamplingTime;LevelParameter;LevelFrom;LevelTo;
  if (pstr[parameterColumn["Parameter"]] != cv[asciiColumn["Parameter"]]) {
	return result;
  }
  if (pstr[parameterColumn["StatisticsFormula"]] != cv[asciiColumn["StatisticsFormula"]]) {
	return result;
  }
  // Check for wildcard
  if (pstr[parameterColumn["SamplingTime"]] != "*")
	if (pstr[parameterColumn["SamplingTime"]] != cv[asciiColumn["SamplingTime"]]) {
	  return result;
	}	  
  if (pstr[parameterColumn["Level Parameter"]] != cv[asciiColumn["Level Parameter"]])
	return result;
  // Check for wildcard
  if (pstr[parameterColumn["Level from"]] != "*")
	if (pstr[parameterColumn["Level from"]] != cv[asciiColumn["Level from"]])
	  return result;
  // Check for wildcard
  if (pstr[parameterColumn["Level to"]] != "*")
	if (pstr[parameterColumn["Level to"]] != cv[asciiColumn["Level to"]])
	  return result; 	  
  // Now, we have the parameter and the value is in Presentation Value
  result = cv[asciiColumn["Presentation Value"]];
  return result;
}

bool ObsRoad::getParameterColumnValue(const std::string& cn, const diutil::string_v& cv, float& value)
{
  std::string result = getParameterColumn(cn, cv);
  if (result.empty())
    return false;
  //std::cerr << cn << ", " << result << std::endl;
  value = miutil::to_float(result);
  return true;
}

bool ObsRoad::getParameterColumnValue(const std::string& cn, const diutil::string_v& cv, int& value)
{
  std::string result = getParameterColumn(cn, cv);
  if (result.empty())
    return false;
  //std::cerr << cn << ", " << result << std::endl;
  value = miutil::to_int(result);
  return true;
}

bool ObsRoad::getParameterColumnValue(const std::string& cn, const diutil::string_v& cv, std::string& value)
{
  std::string result = getParameterColumn(cn, cv);
  if (result.empty())
    return false;
  //std::cerr << cn << ", " << result << std::endl;
  value = result;
  return true;
}

// FIXME: The automation code ?
// FIXME: float or string value
void ObsRoad::cloud_type_string(ObsData& d, double v)
{
  int type = int(v) / 10;
  float value = float(int(v) % 10);

  if (value < 1)
    return;

  if (type == 1)
    d.put_string("Ch", miutil::from_number(value));
  if (type == 2)
    d.put_string("Cm", miutil::from_number(value));
  if (type == 3)
    d.put_string("Cl", miutil::from_number(value));
}

std::string ObsRoad::height_of_clouds_string(double height)
{
  METLIBS_LOG_SCOPE(height);
  height = (height * 3.2808399) / 100.0;
  return miutil::from_number(height);
}

// Some methods from the ObsBufr class
void ObsRoad::cloud_type(ObsData& d, double v)
{
  int type = int(v) / 10;
  float value = float(int(v) % 10);

  if (value < 1)
    return;

  if (type == 1)
    d.put_float("Ch", value);
  if (type == 2)
    d.put_float("Cm", value);
  if (type == 3)
    d.put_float("Cl", value);
}

float ObsRoad::height_of_clouds(double height)
{
  METLIBS_LOG_SCOPE(height);

  if (height < 50)
    return 0.0;
  if (height < 100)
    return 1.0;
  if (height < 200)
    return 2.0;
  if (height < 300)
    return 3.0;
  if (height < 600)
    return 4.0;
  if (height < 1000)
    return 5.0;
  if (height < 1500)
    return 6.0;
  if (height < 2000)
    return 7.0;
  if (height < 2500)
    return 8.0;
  return 9.0;
}

float ObsRoad::convert2hft(double height)
{
  METLIBS_LOG_SCOPE(height);

  // return (height*3.2808399)/100.0;
  return (height * miutil::constants::ft_per_m) / 100.0;
}

float ObsRoad::ms2code4451(float v)
{
  METLIBS_LOG_SCOPE(v);
  if (v < 1852.0 / 3600.0)
    return 0.0;
  if (v < 5 * 1852.0 / 3600.0)
    return 1.0;
  if (v < 10 * 1852.0 / 3600.0)
    return 2.0;
  if (v < 15 * 1852.0 / 3600.0)
    return 3.0;
  if (v < 20 * 1852.0 / 3600.0)
    return 4.0;
  if (v < 25 * 1852.0 / 3600.0)
    return 5.0;
  if (v < 30 * 1852.0 / 3600.0)
    return 6.0;
  if (v < 35 * 1852.0 / 3600.0)
    return 7.0;
  if (v < 40 * 1852.0 / 3600.0)
    return 8.0;
  if (v < 25 * 1852.0 / 3600.0)
    return 9.0;
  return 9.0;
}

float ObsRoad::percent2oktas(float v)
{
  METLIBS_LOG_SCOPE(v);
  if (v == 113) {
    return 9.0;
  } else {
    return v / 12.5; //% -> oktas
  }
}
float ObsRoad::convertWW(float ww)
{
  if (ww == 508.0)
    ww = 0.0;
  // Check if new BUFR code, not supported yet!
  // FIXME, see OBS_200+.xlsx
  if (ww > 199.0)
    return 0.0;
  
  const int auto2man[100] = {
      /*100-199*/ 0, 1, 2, 3, 5, 5, 0, 0, 0, 0,
      10, 76, 13, 0, 0, 0, 0, 0, 18, 0,
      28, 21, 20, 21, 22, 24, 29, 38, 38, 39,
      45, 41, 43, 45, 47, 49, 0, 0, 0, 0,
      63, 63, 65, 63, 65, 73, 75, 66, 67, 0,
      53, 51, 53, 55, 56, 57, 57, 58, 59, 0,
      63, 61, 63, 65, 66, 67, 67, 68, 69, 0,
      73, 71, 73, 75, 79, 79, 79, 0, 0, 0,
      81,80, 81, 81, 82, 85, 86, 86, 0, 0,
      17, 17, 95, 96, 17, 97, 99, 0, 0, 19};
      
  if (ww > 99.0)  {
    ww = auto2man[(int)(ww-100.0)];
  }
  return ww;
}

void ObsRoad::amountOfClouds_1(ObsData& dta, bool metar)
{
  float Nh = _undef;
  float h = _undef;
  const float * ppar = nullptr;
  
  if ((ppar = dta.get_float("Nh")) != nullptr)
    Nh = *ppar;
  if ((ppar = dta.get_float("h")) != nullptr)
    h = *ppar;
  
  if (Nh != _undef || h != _undef) {
    QString ost;
    if (Nh > -1)
      if (metar) {
        if (Nh == 8)
          ost = "O";
        else if (Nh == 11)
          ost = "S";
        else if (Nh == 12)
          ost = "B";
        else if (Nh == 13)
          ost = "F";
        else
          ost.setNum(Nh);
      } else
        ost.setNum(Nh);
    else
      ost = "x";

    ost += "/";

    if (h > -1)
      ost += QString::number(h);
    else
      ost += "x";
    dta.metar().cloud.push_back(ost.toStdString());
  }
}

bool ObsRoad::isAuto(const ObsData& obs)
{
  const float* pauto = obs.get_float("auto");
  return pauto && (*pauto == 0);
}

void ObsRoad::decodeMetarCloudAmount(const int & Nsx, QString & ost)
{
  if (Nsx == 8)
	ost = "O";
  else if ((Nsx <=4) && (Nsx>=3))
	ost = "S";
  else if ((Nsx <= 7) && (Nsx >=5))
	ost = "B";
  else if ((Nsx <= 2) && (Nsx >=1))
	ost = "F";
  else
	ost.setNum(Nsx);
}


void ObsRoad::amountOfClouds_1_4(ObsData& dta, bool metar)
{
  // This metod puts detailed cloud parameter in a cloud string
  // and puts it in the vector cloud in ObsData.
  int Ns1 = _undef;
  int hs1 = _undef;
  int Ns2 = _undef;
  int hs2 = _undef;
  int Ns3 = _undef;
  int hs3 = _undef;
  int Ns4 = _undef;
  int hs4 = _undef;
  const float * ppar = nullptr;
  // manned / automated station
    if (isAuto(dta)&&!metar) {
      // automated station
      if ((ppar = dta.get_float("NS_A1")) != nullptr)
          Ns1 = *ppar;
      if ((ppar = dta.get_float("NS_A2")) != nullptr)
          Ns2 = *ppar;
      if ((ppar = dta.get_float("NS_A3")) != nullptr)
          Ns3 = *ppar;
      if ((ppar = dta.get_float("NS_A4")) != nullptr)
          Ns4 = *ppar;
      if ((ppar = dta.get_float("HS_A1")) != nullptr)
          hs1 = *ppar;
      if ((ppar = dta.get_float("HS_A2")) != nullptr)
          hs2 = *ppar;
      if ((ppar = dta.get_float("HS_A3")) != nullptr)
          hs3 = *ppar;
      if ((ppar = dta.get_float("HS_A4")) != nullptr)
          hs4 = *ppar;
        
  } else {
      // manual station
      if ((ppar = dta.get_float("NS1")) != nullptr)
          Ns1 = *ppar;
      if ((ppar = dta.get_float("NS2")) != nullptr)
          Ns2 = *ppar;
      if ((ppar = dta.get_float("NS3")) != nullptr)
          Ns3 = *ppar;
      if ((ppar = dta.get_float("NS4")) != nullptr)
          Ns4 = *ppar;
      if ((ppar = dta.get_float("HS1")) != nullptr)
          hs1 = (int)(*ppar+.5);
      if ((ppar = dta.get_float("HS2")) != nullptr)
          hs2 = (int)(*ppar+.5);
      if ((ppar = dta.get_float("HS3")) != nullptr)
          hs3 = (int)(*ppar+.5);
      if ((ppar = dta.get_float("HS4")) != nullptr)
          hs4 = (int)(*ppar+.5);
  }
  // if metar station do not report Ns1 ... Ns4 and hs1 .. hs4, nothing to do
  if (metar) {
    if (Ns1 == _undef && hs1 == _undef && Ns2 == _undef && hs2 == _undef && Ns3 == _undef && hs3 == _undef && Ns4 == _undef && hs4 == _undef) {
      return;
    }
  }

  if (Ns4 != _undef && Ns4 > 0 && hs4 != _undef && hs4 > 0) {
    QString ost;
    if (metar) {
	  decodeMetarCloudAmount(Ns4,ost);
    } else
      ost.setNum(Ns4);
    ost += "-";
    ost += QString::asprintf("%02i",hs4);
    dta.metar().cloud.push_back(ost.toStdString());
  }
  if (Ns3 != _undef && Ns3 > 0 && hs3 != _undef && hs3 > 0) {
    QString ost;
    if (metar) {
      decodeMetarCloudAmount(Ns3,ost);
    } else
      ost.setNum(Ns3);
    ost += "-";
    ost += QString::asprintf("%02i",hs3);
    dta.metar().cloud.push_back(ost.toStdString());
  }
  if (Ns2 != _undef && Ns2 > 0 && hs2 != _undef && hs2 > 0) {
    QString ost;
    if (metar) {
      decodeMetarCloudAmount(Ns2,ost);
    } else
      ost.setNum(Ns2);
    ost += "-";
    ost += QString::asprintf("%02i",hs2);
    dta.metar().cloud.push_back(ost.toStdString());
  }
  if (Ns1 != _undef && Ns1 > 0 && hs1 != _undef && hs1 > 0) {
    QString ost;
    if (metar) {
      decodeMetarCloudAmount(Ns1,ost);
    } else
      ost.setNum(Ns1);
    ost += "-";
    ost += QString::asprintf("%02i",hs1);
    dta.metar().cloud.push_back(ost.toStdString());
  }
}

void ObsRoad::decodeData()
{
  METLIBS_LOG_SCOPE();

  const bool isoTime = asciiColumn.count("time");
  const bool useTime = isoTime || asciiColumn.count("hour");
  const bool date_ymd = asciiColumn.count("year") && asciiColumn.count("month") && asciiColumn.count("day");
  const bool date_column = asciiColumn.count("date");
  const bool allTime = useTime && (date_column || date_ymd);
  const bool isoDate = useTime && date_column;

  miTime obstime;

  miDate filedate = fileTime.date();

  // skip header; this relies on decodeHeader having trimmed the header lines
  size_t ii = 0;
  while (ii < lines.size() && not miutil::contains(lines[ii], "[DATA]")) {
    METLIBS_LOG_DEBUG("skip '" << lines[ii] << "'");
    ++ii;
  }
  ii += 1; // skip [DATA] line too

  for (int i = 0; ii < lines.size() and i < asciiSkipDataLines; ++i) {
    METLIBS_LOG_DEBUG("skip data start '" << lines[ii++] << "'");
    miutil::trim(lines[ii]);
    if (lines[ii].empty() or lines[ii][0] == '#')
      continue;
    ii += 1;
  }

  for (; ii < lines.size(); ++ii) {
    METLIBS_LOG_DEBUG("read '" << lines[ii] << "'");
    miutil::trim(lines[ii]);
    if (lines[ii].empty() or lines[ii][0] == '#')
      continue;

    std::vector<std::string> pstr;
    if (not separator.empty())
      pstr = miutil::split(lines[ii], separator, false);
    else
      pstr = miutil::split_protected(lines[ii], '"', '"');

    ObsData obsData;
    const size_t tmp_nColumn = std::min(pstr.size(), m_columnType.size());
    // We must get obshour for TxTn and RRR.
    int obshour = 0;
    float value;
    std::string text;
	obsData.metar().CAVOK=false;
    if (getColumnValue("time", pstr, text)) {
      METLIBS_LOG_DEBUG("time: " << text);
      std::vector<std::string> tpart = miutil::split(text, ":");
      obshour = miutil::to_int(tpart[0]);
    }
    // fdata note Cl, Cm, Ch, adjustment
    for (size_t i = 0; i < tmp_nColumn; i++) {
      if (not asciiColumnUndefined.count(pstr[i])) {
        // Set metadata for station...
        if (m_columnName[i] == "data_type") {
          if (pstr[i] != undef_string)
            obsData.put_string(m_columnName[i], pstr[i]);
          if (pstr[i] == "SHIP") {
            obsData.metar().ship_buoy = true;
          }
        } else if (m_columnName[i] == "TxTxTx") {
          if (pstr[i] != undef_string) {
            obsData.put_float(m_columnName[i], miutil::to_float(pstr[i]));
            if (obshour == 18)
              obsData.put_float("TxTn",miutil::to_float(pstr[i]));
          }
        } else if (m_columnName[i] == "TnTnTn") {
          if (pstr[i] != undef_string) {
            obsData.put_float(m_columnName[i], miutil::to_float(pstr[i]));
            if (obshour == 6)
              obsData.put_float("TxTn", miutil::to_float(pstr[i]));
          }
        } else if (m_columnName[i] == "GWI") {
          if (pstr[i] != undef_string) {
            // Convert to float
            value = miutil::to_float(pstr[i]);
            if (value == 2.) {
              obsData.put_string(m_columnName[i], "OK");
			  // Fixme, needed ?
			  //obsData.CAVOK=true;
			  obsData.metar().CAVOK=false;
            } else if (value == 1.) { // Clouds
              obsData.put_string(m_columnName[i], "NSC");
            } else if (value == 3.) { // Clouds
              obsData.put_string(m_columnName[i], "SKC");
            } else { // FIXME, translate to string if needed.
              obsData.put_string(m_columnName[i], pstr[i]);
            }
          }
        } else if (m_columnName[i] == "auto") {
          if (pstr[i] != undef_string)
            obsData.put_float(m_columnName[i], miutil::to_float(pstr[i]));
        } else if (m_columnName[i] == "isdata") {
          if (pstr[i] != undef_string)
            obsData.put_float(m_columnName[i], miutil::to_float(pstr[i]));
          // End of metadata
        } else if (m_columnName[i] == "Cl" || m_columnName[i] == "Cm" || m_columnName[i] == "Ch") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i])) {
              // Convert to the symbol dataspace
              cloud_type(obsData, miutil::to_float(pstr[i]));
            }
        } else if (m_columnName[i] == "h") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              // Convert to clouds dataspace
              obsData.put_float(m_columnName[i], height_of_clouds(miutil::to_float(pstr[i])));
        } else if (m_columnName[i] == "N") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              // Convert to clouds dataspace
              obsData.put_float(m_columnName[i], percent2oktas(miutil::to_float(pstr[i])));
        } else if (m_columnName[i] == "vs") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              // Convert to vs dataspace
              obsData.put_float(m_columnName[i], ms2code4451(miutil::to_float(pstr[i])));
        } else if (m_columnName[i] == "ww") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              // Convert to malual synop dataspace
              obsData.put_float(m_columnName[i], convertWW(miutil::to_float(pstr[i])));
        } else if ((m_columnName[i] == "HS_A1") || (m_columnName[i] == "HS_A2") || (m_columnName[i] == "HS_A3") || (m_columnName[i] == "HS_A4") ||
                   (m_columnName[i] == "HS1") || (m_columnName[i] == "HS2") || (m_columnName[i] == "HS3") || (m_columnName[i] == "HS4")) {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              // Convert to malual synop dataspace
              obsData.put_float(m_columnName[i], convert2hft(miutil::to_float(pstr[i])));
        } else if (m_columnName[i] == "fmfmk") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              // Backward compatibility
              obsData.put_float("fmfm", miutil::to_float(pstr[i]));
        } else {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              obsData.put_float(m_columnName[i], miutil::to_float(pstr[i]));
        }
      }
    }
    // Format the cloud vector
    bool metar = false;
    if (*obsData.get_string("data_type") == "ICAO")
      metar = true;
    amountOfClouds_1_4(obsData, metar);

    if (getColumnValue("x", pstr, value) || getColumnValue("Lon", pstr, value))
      if (value != _undef)
        obsData.basic().xpos = value;
    if (getColumnValue("y", pstr, value) || getColumnValue("Lat", pstr, value))
      if (value != _undef)
        obsData.basic().ypos = value;
    if (getColumnValue("Name", pstr, text)) {
      obsData.basic().id = text;
	  obsData.metar().metarId = text;
	  obsData.put_string("Name", text);
	}
	if (getColumnValue("StationName", pstr, text))
      obsData.put_string("StationName", text);
    if (getColumnValue("ff", pstr, value))
      if (value != _undef)
        obsData.put_float("ff", knots ? miutil::knots2ms(value) : value);
    if (getColumnValue("dd", pstr, value))
      if (value != _undef)
        obsData.put_float("dd", value);
    if (getColumnValue("image", pstr, text))
      obsData.put_string("image", text);

    if (useTime) {
      miClock clock;
      miDate date;
      int hour = 0, min = 0, sec = 0;
      if (isoTime) {
        if (getColumnValue("time", pstr, text)) {
          METLIBS_LOG_DEBUG("time: " << text);
          std::vector<std::string> tpart = miutil::split(text, ":");
          hour = miutil::to_int(tpart[0]);
          if (tpart.size() > 1)
            min = miutil::to_int(tpart[1]);
          if (tpart.size() > 2)
            sec = miutil::to_int(tpart[2]);
          clock = miClock(hour, min, sec);
        } else {
          METLIBS_LOG_WARN("time column missing");
          continue;
        }
      } else {
        if (getColumnValue("hour", pstr, hour)) {
          getColumnValue("min", pstr, min); // no problem if missing, assume min = sec = 00
          getColumnValue("sec", pstr, sec);
          clock = miClock(hour, min, sec);
        } else {
          METLIBS_LOG_WARN("hour column missing");
          continue;
        }
      }

      if (isoDate) {
        if (getColumnValue("date", pstr, text)) {
          METLIBS_LOG_DEBUG("date: " << text);
          date = miDate(text);
        } else {
          METLIBS_LOG_WARN("date column missing");
          continue;
        }
      } else if (allTime) {
        int year = 0, month = 0, day = 0;
        if (getColumnValue("year", pstr, year) and getColumnValue("month", pstr, month) and getColumnValue("day", pstr, day)) {
          date = miDate(year, month, day);
        } else {
          METLIBS_LOG_WARN("year/month/day column missing");
          continue;
        }
      } else {
        date = filedate;
      }
      obstime = miTime(date, clock);

      if (not allTime) {
        int mdiff = miTime::minDiff(obstime, fileTime);
        if (mdiff < -12 * 60)
          obstime.addHour(24);
        else if (mdiff > 12 * 60)
          obstime.addHour(-24);
      }

      METLIBS_LOG_DEBUG(LOGVAL(obstime) << LOGVAL(plotTime) << LOGVAL(timeDiff));
      if (timeDiff < 0 || abs(miTime::minDiff(obstime, plotTime)) < timeDiff)
        obsData.basic().obsTime = obstime;
      else
        continue;
    }
#ifdef DEBUG_OBSDATA
    // Produces a lot of output...
    METLIBS_LOG_INFO("obsData.id: "
                     << ", " << obsData.basic().id);
    METLIBS_LOG_INFO("obsData.xpos: "
                     << ", " << obsData.basic().xpos);
    METLIBS_LOG_INFO("obsData.ypos: "
                     << ", " << obsData.basic().ypos);
    METLIBS_LOG_INFO("fdata");
	std::map<std::string, float>::iterator itf = obsData.fdata.begin();
	for (; itf != obsData.fdata.end(); itf++) {
		METLIBS_LOG_INFO(itf->first << ", " << itf->second);
	}
#endif
    vObsData.get()->add(obsData);
    mObsData[obsData.basic().id] = obsData;
  }
}

void ObsRoad::decodeOneLine(ObsData * obsData, std::vector<std::string> & pstr, const int & obshour)
{
	METLIBS_LOG_SCOPE();
	// In the new format, there are only one parameter at each line
	float value;
	std::string text;
	if (getParameterColumnValue("TTT", pstr, value)) {
	    obsData->put_float("TTT",value);
	} else if (getParameterColumnValue("RoadSurfaceTemperature", pstr, value)) {
		obsData->put_float("RoadSurfaceTemperature",value);
	} else if (getParameterColumnValue("TxTxTx", pstr, value)) {
		obsData->put_float("TxTxTx",value);
		if (obshour == 18)
            obsData->put_float("TxTn",value);
	} else if (getParameterColumnValue("TnTnTn", pstr, value)) {
	    obsData->put_float("TnTnTn",value);
		if (obshour == 6)
            obsData->put_float("TxTn",value);	
	} else if (getParameterColumnValue("TdTdTd", pstr, value)) {
		obsData->put_float("TdTdTd",value);
	} else if (getParameterColumnValue("UU", pstr, value)) {
		obsData->put_float("UU",value);
	} else if (getParameterColumnValue("RRR_1", pstr, value)) {
		obsData->put_float("RRR_1",value);
	} else if (getParameterColumnValue("RRR_3", pstr, value)) {
		obsData->put_float("RRR_3",value);
	} else if (getParameterColumnValue("RRR_6", pstr, value)) {
		obsData->put_float("RRR_6",value);
	} else if (getParameterColumnValue("RRR_12", pstr, value)) {
		obsData->put_float("RRR_12",value);
	} else if (getParameterColumnValue("RRR_24", pstr, value)) {
		obsData->put_float("RRR_24",value);
	} else if (getParameterColumnValue("sss", pstr, value)) {
		// Convert to cm from m.
		obsData->put_float("sss",100.0*value);
	} else if (getParameterColumnValue("PPPP", pstr, value)) {
		obsData->put_float("PPPP",value);
	} else if (getParameterColumnValue("PHPHPHPH", pstr, value)) {
		// convert to hPa
		obsData->put_float("PHPHPHPH",value/100.0);
	} else if (getParameterColumnValue("a", pstr, value)) {
		obsData->put_float("a",value);
	} else if (getParameterColumnValue("ppp", pstr, value)) {
		value = round(value);
		obsData->put_float("ppp",value);
	} else if (getParameterColumnValue("Cl", pstr, value) || getParameterColumnValue("Cm", pstr, value) || getParameterColumnValue("Ch", pstr, value)) {
          // Convert to the symbol dataspace
          cloud_type(*obsData, value);
	} else if (getParameterColumnValue("h", pstr, value)) {
          // Convert to clouds dataspace
          obsData->put_float("h", height_of_clouds(value));
	} else if (getParameterColumnValue("N", pstr, value)) {
          // Convert to clouds dataspace
          obsData->put_float("N", percent2oktas(value));
	} else if (getParameterColumnValue("vs", pstr, value)) {
          // Convert to vs dataspace
          obsData->put_float("vs", ms2code4451(value));
	}  else if (getParameterColumnValue("ds", pstr, value)) {
          obsData->put_float("ds", value);
	} else if (getParameterColumnValue("ww", pstr, value)) {
		  // Convert to malual synop dataspace
          obsData->put_float("ww", convertWW(value));
	} else if (getParameterColumnValue("W1", pstr, value)) {
		  // Convert to malual synop dataspace ?
          obsData->put_float("W1", value);
	} else if (getParameterColumnValue("W2", pstr, value)) {
		  // Convert to malual synop dataspace ?
          obsData->put_float("W2", value);
	} else if (getParameterColumnValue("VV", pstr, value)) {
		  // Convert to malual synop dataspace ?
          obsData->put_float("VV", value);
	} else if (getParameterColumnValue("GWI", pstr, value)) {
		if (value == 2.) {
		  obsData->put_string("GWI", "OK");
		  obsData->metar().CAVOK=false;
		} else if (value == 1.) { // Clouds
		  obsData->put_string("GWI", "NSC");
		} else if (value == 3.) { // Clouds
		  obsData->put_string("GWI", "SKC");
		} else { // FIXME, translate to string if needed.
		  getParameterColumnValue("GWI", pstr, text);
		  obsData->put_string("GWI", text);
		}
    } else if (getParameterColumnValue("HS_A1",pstr,value)) {
		   obsData->put_float("HS_A1", convert2hft(value));
	} else if (getParameterColumnValue("HS_A2",pstr,value)) {
		   obsData->put_float("HS_A2", convert2hft(value));
	} else if (getParameterColumnValue("HS_A3",pstr,value)) {
		   obsData->put_float("HS_A3", convert2hft(value));
	} else if (getParameterColumnValue("HS_A4",pstr,value)) {
		   obsData->put_float("HS_A4", convert2hft(value));
	} else if (getParameterColumnValue("NS_A1",pstr,value)) {
		   obsData->put_float("NS_A1", value);
	} else if (getParameterColumnValue("NS_A2",pstr,value)) {
		   obsData->put_float("NS_A2", value);
	} else if (getParameterColumnValue("NS_A3",pstr,value)) {
		   obsData->put_float("NS_A3", value);
	} else if (getParameterColumnValue("NS_A4",pstr,value)) {
		   obsData->put_float("NS_A4", value);
	} else if (getParameterColumnValue("HS1",pstr,value)) {
		   obsData->put_float("HS1", convert2hft(value));
	} else if (getParameterColumnValue("HS2",pstr,value)) {
		   obsData->put_float("HS2", convert2hft(value));
	} else if (getParameterColumnValue("HS3",pstr,value)) {
		   obsData->put_float("HS3", convert2hft(value));
	} else if (getParameterColumnValue("HS4",pstr,value)) {
		   obsData->put_float("HS4", convert2hft(value));
    } else if (getParameterColumnValue("NS1",pstr,value)) {
		   obsData->put_float("NS1", value);
	} else if (getParameterColumnValue("NS2",pstr,value)) {
		   obsData->put_float("NS2", value);
	} else if (getParameterColumnValue("NS3",pstr,value)) {
		   obsData->put_float("NS3", value);
	} else if (getParameterColumnValue("NS4",pstr,value)) {
		   obsData->put_float("NS4", value);
    }else if (getParameterColumnValue("CC1",pstr,value)) {
		   obsData->put_float("CC1", value);
	} else if (getParameterColumnValue("CC2",pstr,value)) {
		   obsData->put_float("CC2", value);
	} else if (getParameterColumnValue("CC3",pstr,value)) {
		   obsData->put_float("CC3", value);
	} else if (getParameterColumnValue("CC4",pstr,value)) {
		   obsData->put_float("CC4", value);
    } else if (getParameterColumnValue("fmfmk", pstr, value) || getParameterColumnValue("fmfm", pstr, value)){
        // Backward compatibility
        obsData->put_float("fmfm", value);
    } else if (getParameterColumnValue("911ff", pstr, value)) {
		obsData->put_float("911ff", value);
    } else if (getParameterColumnValue("ff", pstr, value)) {
	    // Wind speed always m/s from database.
		obsData->put_float("ff", value);
    } else if (getParameterColumnValue("dd", pstr, value)) {
        obsData->put_float("dd", value);
	} else if (getParameterColumnValue("dxdxdx", pstr, value)) {
        obsData->put_float("dxdxdx", value);
	} else if (getParameterColumnValue("dndndn", pstr, value)) {
        obsData->put_float("dndndn", value);
	} else if (getParameterColumnValue("image", pstr, text)) {
		obsData->put_string("image", text);
	} 
	// This must be done for every parameter
	if (getColumnValue("Quality", pstr, text)) {
	  // decode and set the total quality of observation
	  float quality_value = 0;
	  if (text == "G")
		quality_value = 4;
	  else if (text == "Y")
	    quality_value = 2;
	  const float * pVal = 0;
	  pVal = obsData->get_float("quality");
	  if (pVal != 0) {
		if (quality_value < *pVal) {
			obsData->put_float("quality",quality_value);
		}
	  } else {
		obsData->put_float("quality",quality_value);
	  }
	}
}

std::string ObsRoad::convertWmoStationId(std::string & text)
{
	//station id 426,0,2
	std::string result;
	std::vector<std::string> wmo_parts = miutil::split(text, ",");
	if (wmo_parts.size() == 3)
	{ 
		result = wmo_parts[1] + wmo_parts[2] + wmo_parts[0];
	} else {
		// Nothing to do, just return text
		result=text;
	}
	return result;
}

float ObsRoad::convertOperatingMode(std::string & text)
{
	// default auto ?
	if (text.empty()) return 0.0;
	// CODE TABLE 2001
	if (text == "Automatic") {
		return 0.0;
	} else if (text == "Manned") {
		return 1.0;
	} else if (text == "Hybrid") {
		return 2.0;
	} else {
		// Missing ?
		return 3.0;
	}
}

int ObsRoad::decodeOffset(std::string & text)
{
	if (text == "PT0S")
		return 0;
	miutil::trim(text, true, true, "PTM");
	return -miutil::to_int(text);
}

void ObsRoad::decodeNewData()
{
  METLIBS_LOG_SCOPE();

  const bool isoTime = 1;/*asciiColumn.count("Time")*/;
  const bool useTime = isoTime || asciiColumn.count("hour");
  const bool date_ymd = asciiColumn.count("year") && asciiColumn.count("month") && asciiColumn.count("day");
  const bool date_column = asciiColumn.count("date");
  const bool allTime = useTime && (date_column || date_ymd);
  const bool isoDate = useTime && date_column;
  

  miTime obstime;

  miDate filedate = fileTime.date();
  // we must parese the file two times, first to construct a station list and second to fill
  // the stations with data.
  // skip header; header is always line number 0.
  size_t ii = 1;
  
  size_t index = -1;
  
  ObsData * obsData = 0;
  bool new_object = true;
  for (; ii < lines.size(); ++ii) {
    //METLIBS_LOG_DEBUG("read '" << lines[ii] << "'");
    // This should not happen
    if (lines[ii].empty() or lines[ii][0] == '#')
      continue;
    std::vector<std::string> pstr;
    if (not separator.empty())
      pstr = miutil::split(lines[ii], separator, false);
    else
      pstr = miutil::split_protected(lines[ii], '"', '"');
	// The data file contaions data from all station types, we must select the correct type of station
	//ClimateStationId;ClimateStationName;NationalStationId;NationalStationName;WmoStationId;WmoStationName;IcaoStationId;IcaoStationName;OperatingMode;
	// Check all types...
    // We must get obshour for TxTn and RRR.
    int obshour = 0;
	int offset = 0;
    float value;
    std::string text;
	// New Time format, 2021-11-16 06:00
	// An ugly fix
	text = pstr[0];
	if (!text.empty()) {
    //if (getColumnValue("Time", pstr, text)) {
      METLIBS_LOG_DEBUG("time: " << text);
	  miutil::miTime time = miutil::miTime(text);
	  if (getColumnValue("Offset", pstr, text)) {
		//decode offset, offset is always negative and for now, in minutes.
		offset=decodeOffset(text);
		if (offset != 0)
			time.addMin(offset);
	  }
	  obshour=time.hour();
    }
	// Get station meta data
	//ClimateStationId;ClimateStationName;NationalStationId;NationalStationName;WmoStationId;WmoStationName;IcaoStationId;IcaoStationName;OperatingMode;
	if (datatype_ == "wmo") {
		if (pstr[asciiColumn["WmoStationId"]].empty()) {
			continue;
		}
		if (getColumnValue("WmoStationId", pstr, text)) {
			text = convertWmoStationId(text);
			if (mObsData.count(text)) {
				new_object = false;
				decodeOneLine(obsData, pstr, obshour);
			} else {
				if (obsData != 0) {
					 // Format the cloud vector
					bool metar = false;
					amountOfClouds_1_4(*obsData, metar);
					vObsData.get()->add(*obsData);
					mObsData[obsData->basic().id] = *obsData;
#ifdef DEBUG_OBSDATA
					// Produces a lot of output...
					METLIBS_LOG_INFO("obsData.id: "
						 << ", " << obsData->basic().id);
					METLIBS_LOG_INFO("obsData.xpos: "
						 << ", " << obsData->basic().xpos);
					METLIBS_LOG_INFO("obsData.ypos: "
						 << ", " << obsData->basic().ypos);
						 
					METLIBS_LOG_INFO("fdata");
					std::map<std::string, float>::iterator itf = obsData->fdata.begin();
					for (; itf != obsData->fdata.end(); itf++) {
						METLIBS_LOG_INFO(itf->first << ", " << itf->second);
					}
#endif
					delete obsData;
					obsData = 0;
				}
				new_object = true;
				obsData = new ObsData();
				obsData->clear_data();
				obsData->basic().id = text;
				obsData->metar().metarId = text;
				obsData->metar().ship_buoy = false;
				obsData->put_string("Name", text);
				text.clear();
				// Take care or station without StationName, should not happen
				getColumnValue("WmoStationName", pstr, text);
				obsData->put_string("StationName", text);
				obsData->put_float("isdata", 1.0);
				obsData->basic().dataType=datatype_;
				obsData->put_string("data_type", datatype_);
				obsData->metar().CAVOK=false;
				if (getColumnValue("Longitud", pstr, value))
					if (value != _undef)
						obsData->basic().xpos = value;
				if (getColumnValue("Latitud", pstr, value))
					if (value != _undef)
						obsData->basic().ypos = value;
				getColumnValue("OperatingMode", pstr, text);
				value = convertOperatingMode(text);
				obsData->put_float("auto", value);
				decodeOneLine(obsData, pstr, obshour);
				mObsData[obsData->basic().id] = *obsData;
			}			
		}
	}
	if (datatype_ == "icao") {
		if (pstr[asciiColumn["IcaoStationId"]].empty()) {
			continue;
		}
		if (getColumnValue("IcaoStationId", pstr, text)) {
			if (mObsData.count(text)) {
				new_object = false;
				decodeOneLine(obsData, pstr, obshour);
			} else {
				if (obsData != 0) {
					 // Format the cloud vector
					bool metar = true;
					amountOfClouds_1_4(*obsData, metar);
					vObsData.get()->add(*obsData);
					mObsData[obsData->basic().id] = *obsData;
#ifdef DEBUG_OBSDATA
					// Produces a lot of output...
					METLIBS_LOG_INFO("obsData.id: "
						 << ", " << obsData->basic().id);
					METLIBS_LOG_INFO("obsData.xpos: "
						 << ", " << obsData->basic().xpos);
					METLIBS_LOG_INFO("obsData.ypos: "
						 << ", " << obsData->basic().ypos);
						 
					METLIBS_LOG_INFO("fdata");
					std::map<std::string, float>::iterator itf = obsData->fdata.begin();
					for (; itf != obsData->fdata.end(); itf++) {
						METLIBS_LOG_INFO(itf->first << ", " << itf->second);
					}
#endif
					delete obsData;
					obsData = 0;
				}
				new_object = true;
				obsData = new ObsData();
				obsData->clear_data();
				obsData->basic().id = text;
				obsData->metar().metarId = text;
				obsData->metar().ship_buoy = false;
				obsData->put_string("Name", text);
				text.clear();
				// Take care or station without StationName, should not happen
				getColumnValue("IcaoStationName", pstr, text);
				obsData->put_string("StationName", text);
				obsData->put_float("isdata", 1.0);
				obsData->basic().dataType=datatype_;
				obsData->put_string("data_type", datatype_);
				obsData->metar().CAVOK=false;
				if (getColumnValue("Longitud", pstr, value))
					if (value != _undef)
						obsData->basic().xpos = value;
				if (getColumnValue("Latitud", pstr, value))
					if (value != _undef)
						obsData->basic().ypos = value;
				getColumnValue("OperatingMode", pstr, text);
				value = convertOperatingMode(text);
				obsData->put_float("auto", value);
				decodeOneLine(obsData, pstr, obshour);
				mObsData[obsData->basic().id] = *obsData;
			}			
		}
	}
	if (datatype_ == "nat") {
		if (pstr[asciiColumn["NationalStationId"]].empty()) {
			continue;
		}
		if (getColumnValue("NationalStationId", pstr, text)) {
			if (mObsData.count(text)) {
				new_object = false;
				decodeOneLine(obsData, pstr, obshour);
			} else {
				if (obsData != 0) {
					 // Format the cloud vector
					bool metar = false;
					amountOfClouds_1_4(*obsData, metar);
					vObsData.get()->add(*obsData);
					mObsData[obsData->basic().id] = *obsData;
#ifdef DEBUG_OBSDATA
					// Produces a lot of output...
					METLIBS_LOG_INFO("obsData.id: "
						 << ", " << obsData->basic().id);
					METLIBS_LOG_INFO("obsData.xpos: "
						 << ", " << obsData->basic().xpos);
					METLIBS_LOG_INFO("obsData.ypos: "
						 << ", " << obsData->basic().ypos);
						 
					METLIBS_LOG_INFO("fdata");
					std::map<std::string, float>::iterator itf = obsData->fdata.begin();
					for (; itf != obsData->fdata.end(); itf++) {
						METLIBS_LOG_INFO(itf->first << ", " << itf->second);
					}
#endif
					delete obsData;
					obsData = 0;
				}
				new_object = true;
				obsData = new ObsData();
				obsData->clear_data();
				obsData->basic().id = text;
				obsData->metar().metarId = text;
				obsData->metar().ship_buoy = false;
				obsData->put_string("Name", text);
				text.clear();
				// Take care or station without StationName, should not happen
				getColumnValue("NationalStationName", pstr, text);
				obsData->put_string("StationName", text);
				obsData->put_float("isdata", 1.0);
				obsData->basic().dataType=datatype_;
				obsData->put_string("data_type", datatype_);
				obsData->metar().CAVOK=false;
				if (getColumnValue("Longitud", pstr, value))
					if (value != _undef)
						obsData->basic().xpos = value;
				if (getColumnValue("Latitud", pstr, value))
					if (value != _undef)
						obsData->basic().ypos = value;
				getColumnValue("OperatingMode", pstr, text);
				value = convertOperatingMode(text);
				obsData->put_float("auto", value);
				decodeOneLine(obsData, pstr, obshour);
				mObsData[obsData->basic().id] = *obsData;
			}			
		}
	}
	if (datatype_ == "clim") {
		if (pstr[asciiColumn["ClimateStationId"]].empty()) {
			continue;
		}
		if (getColumnValue("ClimateStationId", pstr, text)) {
			if (mObsData.count(text)) {
				new_object = false;
				decodeOneLine(obsData, pstr, obshour);
			} else {
				if (obsData != 0) {
					 // Format the cloud vector
					bool metar = false;
					amountOfClouds_1_4(*obsData, metar);
					vObsData.get()->add(*obsData);
					mObsData[obsData->basic().id] = *obsData;
#ifdef DEBUG_OBSDATA
					// Produces a lot of output...
					METLIBS_LOG_INFO("obsData.id: "
						 << ", " << obsData->basic().id);
					METLIBS_LOG_INFO("obsData.xpos: "
						 << ", " << obsData->basic().xpos);
					METLIBS_LOG_INFO("obsData.ypos: "
						 << ", " << obsData->basic().ypos);
						 
					METLIBS_LOG_INFO("fdata");
					std::map<std::string, float>::iterator itf = obsData->fdata.begin();
					for (; itf != obsData->fdata.end(); itf++) {
						METLIBS_LOG_INFO(itf->first << ", " << itf->second);
					}
#endif
					delete obsData;
					obsData = 0;
				}
				new_object = true;
				obsData = new ObsData();
				obsData->clear_data();
				obsData->basic().id = text;
				obsData->metar().metarId = text;
				obsData->metar().ship_buoy = false;
				obsData->put_string("Name", text);
				text.clear();
				// Take care or station without StationName, should not happen
				getColumnValue("ClimateStationName", pstr, text);
				obsData->put_string("StationName", text);
				obsData->put_float("isdata", 1.0);
				obsData->basic().dataType=datatype_;
				obsData->put_string("data_type", datatype_);
				obsData->metar().CAVOK=false;
				if (getColumnValue("Longitud", pstr, value))
					if (value != _undef)
						obsData->basic().xpos = value;
				if (getColumnValue("Latitud", pstr, value))
					if (value != _undef)
						obsData->basic().ypos = value;
				getColumnValue("OperatingMode", pstr, text);
				value = convertOperatingMode(text);
				obsData->put_float("auto", value);
				decodeOneLine(obsData, pstr, obshour);
				mObsData[obsData->basic().id] = *obsData;
			}			
		}
	}

    if (useTime && new_object) {
      miClock clock;
      miDate date;
      int hour = 0, min = 0, sec = 0;
      if (isoTime) {
		text=pstr[0];
		if (!text.empty()) {
        //if (getColumnValue("Time", pstr, text)) {
          METLIBS_LOG_DEBUG("time: " << text);
		  miutil::miTime time = miutil::miTime(text);
		  if (getColumnValue("Offset", pstr, text)) {
			//decode offset, offset is always negative and for now, in minutes.
			offset=decodeOffset(text);
			if (offset != 0)
				time.addMin(offset);
		  }
          hour = time.hour();
          min = time.min();
          sec = time.sec();
          clock = miClock(hour, min, sec);
        } else {
          METLIBS_LOG_WARN("time column missing");
          continue;
        }
      } else {
        if (getColumnValue("hour", pstr, hour)) {
          getColumnValue("min", pstr, min); // no problem if missing, assume min = sec = 00
          getColumnValue("sec", pstr, sec);
          clock = miClock(hour, min, sec);
        } else {
          METLIBS_LOG_WARN("hour column missing");
          continue;
        }
      }

      if (isoDate) {
        if (getColumnValue("date", pstr, text)) {
          METLIBS_LOG_DEBUG("date: " << text);
          date = miDate(text);
        } else {
          METLIBS_LOG_WARN("date column missing");
          continue;
        }
      } else if (allTime) {
        int year = 0, month = 0, day = 0;
        if (getColumnValue("year", pstr, year) and getColumnValue("month", pstr, month) and getColumnValue("day", pstr, day)) {
          date = miDate(year, month, day);
        } else {
          METLIBS_LOG_WARN("year/month/day column missing");
          continue;
        }
      } else {
        date = filedate;
      }
      obstime = miTime(date, clock);

      if (not allTime) {
        int mdiff = miTime::minDiff(obstime, fileTime);
        if (mdiff < -12 * 60)
          obstime.addHour(24);
        else if (mdiff > 12 * 60)
          obstime.addHour(-24);
      }

      METLIBS_LOG_DEBUG(LOGVAL(obstime) << LOGVAL(plotTime) << LOGVAL(timeDiff));
      if (timeDiff < 0 || abs(miTime::minDiff(obstime, plotTime)) < timeDiff)
		if (obsData != 0) {
			obsData->basic().obsTime = obstime;
		}
      else
        continue;
    }
  }
  // save the last object after loop is done
  if (obsData != 0) {
	  vObsData.get()->add(*obsData);
	  mObsData[obsData->basic().id] = *obsData;
#ifdef DEBUG_OBSDATA
		// Produces a lot of output...
		METLIBS_LOG_INFO("obsData.id: "
						 << ", " << obsData->basic().id);
		METLIBS_LOG_INFO("obsData.xpos: "
						 << ", " << obsData->basic().xpos);
		METLIBS_LOG_INFO("obsData.ypos: "
						 << ", " << obsData->basic().ypos);
		METLIBS_LOG_INFO("fdata");
		std::map<std::string, float>::iterator itf = obsData->fdata.begin();
		for (; itf != obsData->fdata.end(); itf++) {
			METLIBS_LOG_INFO(itf->first << ", " << itf->second);
		}
#endif
	  delete obsData;
	  obsData = 0;
  }
}


VprofValues_p ObsRoad::getVprofPlot(const std::string& modelName, const std::string& station, const miutil::miTime& time)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_SCOPE();
#endif

  vertical_axis_ = PRESSURE;
  const std::string zUnit = (vertical_axis_ == PRESSURE) ? "hPa" : "m";
  vp = std::make_shared<VprofSimpleValues>();
  vp->add(temperature = std::make_shared<VprofSimpleData>(vprof::VP_AIR_TEMPERATURE, zUnit, "degree_Celsius"));
  vp->add(dewpoint_temperature = std::make_shared<VprofSimpleData>(vprof::VP_DEW_POINT_TEMPERATURE, zUnit, "degree_Celsius"));
  vp->add(wind_dd = std::make_shared<VprofSimpleData>(vprof::VP_WIND_DD, zUnit, vprof::VP_UNIT_COMPASS_DEGREES));
  vp->add(wind_ff = std::make_shared<VprofSimpleData>(vprof::VP_WIND_FF, zUnit, "m/s"));
  vp->add(wind_sig = std::make_shared<VprofSimpleData>(vprof::VP_WIND_SIG, zUnit, ""));

  if (modelName == "AMDAR" || modelName == "PILOT") {
    METLIBS_LOG_WARN("This model is not implemented yet: " << modelName);
    return vp;
  }
  // This creates the stationlist
  // We must also init the connect string to mora db
  Roaddata::initRoaddata(databasefile_);
  diStation::initStations(stationfile_);
  // get the pointer to the actual station vector
  std::vector<diStation>* stations = 0;
  std::map<std::string, std::vector<diStation>*>::iterator its = diStation::station_map.find(stationfile_);
  if (its == diStation::station_map.end()) {
    METLIBS_LOG_ERROR("Unable to find stationlist: " << stationfile_);
    return vp;
  }
  stations = its->second;
  if (stations == 0) {
    METLIBS_LOG_ERROR("Empty stationlist: " << stationfile_);
    return vp;
  }
  int nStations = stations->size();
  // Return if empty station list
  if (nStations < 1)
    return 0;

  int n = 0;
  while (n < nStations && (*stations)[n].name() != station)
    n++;
  // Return if station not found in station list
  if (n == nStations) {
    METLIBS_LOG_ERROR("Unable to find station: " << station << " in stationlist!");
    return vp;
  }
  const diStation& station_n = (*stations)[n];

  vp->text.index = -1;
  vp->text.modelName = modelName;
  vp->text.posName = miutil::trimmed(station_n.name());
  vp->text.prognostic = false;
  vp->text.forecastHour = 0;
  vp->text.validTime = time;
  vp->text.latitude = station_n.lat();
  vp->text.longitude = station_n.lon();
  vp->text.kindexFound = false;
  vp->text.realization = -1;

  /* HERE we should get the data from road */

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, time);
  road.open();

  std::vector<diStation> stations_to_plot;
  /* only get data for one station */
  stations_to_plot.push_back(station_n);
  /* get the data */
  std::map<int, std::vector<RDKCOMBINEDROW_2>> raw_data_map;
  road.getRadiosondeData(stations_to_plot, raw_data_map);

  road.close();
  std::vector<RDKCOMBINEDROW_2> raw_data;
  std::map<int, std::vector<RDKCOMBINEDROW_2>>::iterator itd = raw_data_map.find(station_n.stationID());
  if (itd != raw_data_map.end()) {
    raw_data = itd->second;
  }
  /* Sort the data */
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Lines returned from road.getData(): " << raw_data.size());
#endif
  // For now, we dont use the surface data!
  std::vector<diParam>* params = 0;
  std::map<std::string, std::vector<diParam>*>::iterator itp = diParam::params_map.find(headerfile_);
  if (itp == diParam::params_map.end()) {
    // the code below would try to use 'params'
    return vp;
  }
  params = itp->second;
  if (params == 0)
    return vp;
  /* map the data and sort them */
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>> data_map;
  int no_of_data_rows = raw_data.size();
  for (size_t k = 0; k < params->size(); k++) {
    std::map<float, RDKCOMBINEDROW_2> tmpresult;
    for (int i = 0; i < no_of_data_rows; i++) {
      if ((*params)[k].isMapped(raw_data[i])) {
        // Data must be keyed with pressure
        if (raw_data[i].srid - 1000 == 4)
          tmpresult[raw_data[i].altitudefrom] = raw_data[i];
      }
    }
    if (tmpresult.size() != 0) {
      data_map[(*params)[k].diananame()] = tmpresult;
    }
  }
  /* data is now sorted */

#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("Data is now sorted!");
#endif

  /* Use TTT + 1 to detrmine no of levels */

  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator ittt = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator ittd = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itdd = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itff = data_map.begin();
  /* Siginifcant wind levels */
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_1 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_4 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_5 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_6 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_7 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_12 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_13 = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itsig_14 = data_map.begin();

  /* the surface values */
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator ittts = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator ittds = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itdds = data_map.begin();
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itffs = data_map.begin();
  /* the ground pressure */
  std::map<std::string, std::map<float, RDKCOMBINEDROW_2>>::iterator itpps = data_map.begin();

  std::map<float, RDKCOMBINEDROW_2>::iterator ittp;

  ittts = data_map.find("TTTs");
  float TTTs_value = -32767.0;
  if (ittts != data_map.end()) {
    ittp = ittts->second.begin();
    for (; ittp != ittts->second.end(); ittp++) {
      TTTs_value = ittp->second.floatvalue;
    }
  }

  ittds = data_map.find("TdTdTds");
  float TdTdTds_value = -32767.0;

  if (ittds != data_map.end()) {
    ittp = ittds->second.begin();
    for (; ittp != ittds->second.end(); ittp++) {
      TdTdTds_value = ittp->second.floatvalue;
    }
  }

  itdds = data_map.find("dds");
  float dds_value = -32767.0;
  if (itdds != data_map.end()) {
    ittp = itdds->second.begin();
    for (; ittp != itdds->second.end(); ittp++) {
      dds_value = ittp->second.floatvalue;
    }
  }

  itffs = data_map.find("ffs");
  float ffs_value = -32767.0;
  if (itffs != data_map.end()) {
    ittp = itffs->second.begin();
    for (; ittp != itffs->second.end(); ittp++) {
      ffs_value = ittp->second.floatvalue;
    }
  }

  itpps = data_map.find("PPPP");
  float PPPPs_value = -32767.0;
  if (itpps != data_map.end()) {
    ittp = itpps->second.begin();
    for (; ittp != itpps->second.end(); ittp++) {
      PPPPs_value = ittp->second.floatvalue;
    }
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("surface data TTTs: " << TTTs_value << " TdTdTds: " << TdTdTds_value << " dds: " << dds_value << " ffs: " << ffs_value
                                          << " PPPP: " << PPPPs_value);
#endif

  ittt = data_map.find("TTT");
  /* Only surface observations */

  if (ittt == data_map.end()) {
    vp->prognostic = false;
    return vp;
  }

  ittd = data_map.find("TdTdTd");
  /* Only surface observations */

  if (ittd == data_map.end()) {
    vp->prognostic = false;
    return vp;
  }

  itdd = data_map.find("dd");
  /* Only surface observations */

  if (itdd == data_map.end()) {
    vp->prognostic = false;
    return vp;
  }

  itff = data_map.find("ff");
  /* Only surface observations */

  if (itff == data_map.end()) {
    vp->prognostic = false;
    return vp;
  }

  /* Significant vind levels */
  /* These may be or may be not in telegram */
  itsig_1 = data_map.find("sig_1");
  itsig_4 = data_map.find("sig_4");
  itsig_7 = data_map.find("sig_7");
  itsig_12 = data_map.find("sig_12");
  itsig_13 = data_map.find("sig_13");
  itsig_14 = data_map.find("sig_14");
  /* allocate temporary maps */
  std::map<float, RDKCOMBINEDROW_2> sig_1;
  std::map<float, RDKCOMBINEDROW_2> sig_4;
  std::map<float, RDKCOMBINEDROW_2> sig_7;
  std::map<float, RDKCOMBINEDROW_2> sig_12;
  std::map<float, RDKCOMBINEDROW_2> sig_13;
  std::map<float, RDKCOMBINEDROW_2> sig_14;

  /* fill them with data if data present */
  if (itsig_1 != data_map.end())
    sig_1 = itsig_1->second;
  if (itsig_4 != data_map.end())
    sig_4 = itsig_4->second;
  if (itsig_7 != data_map.end())
    sig_7 = itsig_7->second;
  if (itsig_12 != data_map.end())
    sig_12 = itsig_12->second;
  if (itsig_13 != data_map.end())
    sig_13 = itsig_13->second;
  if (itsig_14 != data_map.end())
    sig_14 = itsig_14->second;

  float p, tt, td, fff, ddd;
  int dd, ff, bpart;
  int ffmax = -1, kmax = -1;
  /* Can we trust that all parameters, has the same number of levels ? */
  ittp = ittt->second.begin();
  // Here should we sort!
  std::set<float> keys;
  /* Check if PPPP is in telegram */
  if (PPPPs_value != -32767.0)
    keys.insert(PPPPs_value * 100.0);
  for (; ittp != ittt->second.end(); ittp++) {
    // insert altitudefrom in the set
    keys.insert(ittp->second.altitudefrom);
  }
  int siglevels = sig_1.size() + sig_4.size() + sig_7.size() + sig_12.size() + sig_13.size() + sig_14.size();
  // Iterate over the sorted set */
  int d = 0;
  std::set<float>::iterator it = keys.begin();
  for (; it != keys.end(); it++) {
    float key = *it;
    p = key * 0.01;
    // check with ground level pressure !
    if ((p > PPPPs_value) && (PPPPs_value != -32767.0))
      continue;
    if (p > 0. && p < 1300.) {
      if (key == (PPPPs_value * 100.0)) {
        tt = TTTs_value;
        td = TdTdTds_value;
      } else {
        tt = -30000.;
        if (ittt->second.count(key))
          tt = ittt->second[key].floatvalue;
        td = -30000.;
        if (ittd->second.count(key))
          td = ittd->second[key].floatvalue;
      }
      if (tt > -30000.) {
        temperature->add(p, tt);
        if (td > -30000.) {
          dewpoint_temperature->add(p, td);
        }
      }
      if (key == (PPPPs_value * 100.0)) {
        dd = dds_value;
        ff = ffs_value;
      } else {
        dd = -1;
        if (itdd->second.count(key))
          dd = int(itdd->second[key].floatvalue);
        ff = -1;
        if (itff->second.count(key))
          ff = int(itff->second[key].floatvalue);
      }
      if (dd >= 0 && dd <= 360 && ff >= 0) {
        // Only plot the significant winds
        bpart = 0;
        // SHOULD it always be 1 ?
        if (sig_1.count(key) || sig_4.count(key) || sig_7.count(key) || sig_12.count(key) || sig_13.count(key) || sig_14.count(key))
          bpart = 1;
        // Plot winds at significant levels and at standard pressure levels.
        if (bpart > 0 || p == 1000 || p == 925 || p == 850 || p == 800 || p == 700 || p == 500 || p == 400 || p == 300 || p == 200 || p == 100 || p == 50) {
          wind_dd->add(p, dd);
          wind_ff->add(p, ff);
          wind_sig->add(p, bpart);
          if (ff > ffmax) {
            ffmax = ff;
            kmax = wind_sig->length() - 1;
          }
        }
      }
    }
  } /* End for */
  if (kmax >= 0)
    wind_sig->setX(kmax, 3);
  vp->prognostic = false;
  vp->calculate();
  return vp;
}
