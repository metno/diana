/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2016 met.no and SMHI

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
//#ifndef ROADOBS
//#define ROADOBS 1
//#endif
//#ifdef ROADOBS

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "diObsRoad.h"
#include "diLabelPlotCommand.h"
#include "diUtilities.h"

// from kvroadapi
#ifdef NEWARK_INC
#include <newarkAPI/diParam.h>
#include <newarkAPI/diStation.h>
#include <newarkAPI/diRoaddata.h>
#else
#include <roadAPI/diParam.h>
#include <roadAPI/diStation.h>
#include <roadAPI/diRoaddata.h>
#endif
#include <puTools/miStringFunctions.h>

#include <boost/bind/bind.hpp>

#include <algorithm>
#include <fstream>
#include <vector>

#define MILOGGER_CATEGORY "diana.ObsRoad"
#include <miLogger/miLogging.h>

//#define DEBUGPRINT 1

using namespace road;
using namespace miutil;
using namespace std;

ObsRoad::ObsRoad(const std::string &filename, const std::string &databasefile, const std::string &stationfile, const std::string &headerfile,
		   const miTime &filetime, RoadObsPlot *oplot, bool breadData)
{
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::ObsRoad() ++");
#endif
	// clear members

	headerRead = false;
  filename_ = filename;
  databasefile_ = databasefile;
  stationfile_ = stationfile;
  headerfile_ = headerfile;
  filetime_ = filetime;
	if (!breadData) {
		readHeader(oplot);
		headerRead = true;
	}
	else
		readData(oplot);
}

void ObsRoad::readHeader(RoadObsPlot *oplot)
{
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ ObsRoad::readHeader( headerfile: " << headerfile_ << " ) ++");
#endif
  int n,i;
  vector<std::string> vstr,pstr;
  std::string str;
  if (oplot) {
    plotTime = oplot->getObsTime();
    timeDiff= oplot->getTimeDiff();
  }
  fileTime = filetime_;
	// Dont tamper with the plot object...
	if (true) {
	int theresult = diParam::initParameters(headerfile_);
    if (theresult)
	{
		// take a local copy
		vector<diParam> * params = NULL;
		map<std::string, vector<diParam> * >::iterator itp = diParam::params_map.find(headerfile_);
		if (itp != diParam::params_map.end())
		{
			params = itp->second;
		}
		/* this should not happen if configured properly */
		if (params == NULL)
		{
				//oplot->roadobsHeader = false;
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
			line = "Name:id: Date:d: Time:t: Lat:lat: Lon:lon: ";
			// the dynamic part
			// the data value parameters
		for (i = 0; i < params->size(); i++)
		{
			std::string name = (*params)[i].diananame();
				miutil::trim(name);
				line = line + name + ":r:" + (*params)[i].unit() + " ";
  }
			// The encloseing bracket
			line += "]";
			//cerr << line << endl;
			lines.push_back(line);
			// The fixed data line tells decode header to stop parsing
			line = "[DATA]";
			lines.push_back(line);
			// Now we should be ready to decode it...
			separator.clear();
			decodeHeader();
		} // end if theresult
	} // end if true
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("++ ObsRoad::readHeader()  done ++");
#endif
}

void ObsRoad::getStationList(vector<stationInfo> & stations)
{
  // This creates the stationlist
  // We must also init the connect string to mora db
  Roaddata::initRoaddata(databasefile_);
  diStation::initStations(stationfile_);
  // get the pointer to the actual station vector
  map<std::string, vector<diStation> * >::iterator its = diStation::station_map.find(stationfile_);
  if (its == diStation::station_map.end()) {
    METLIBS_LOG_ERROR("Unable to find stationlist: '" <<stationfile_ << "'");
    return;
  }
  const std::vector<diStation>& stationlist = *its->second;

  if (stationlist.empty()) {
    METLIBS_LOG_WARN("Empty stationlist: '" <<stationfile_ << "'");
    return;
  }
  for (size_t i = 0; i < stationlist.size(); i++) {
    stationInfo st("",0,0);
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

void ObsRoad::initData(RoadObsPlot *oplot)
{
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::initData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl);
#endif
	// read the headerfile if needed
		readHeader(oplot);
	initRoadData(oplot);
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::initData()done ++ ");
#endif
}

void ObsRoad::initRoadData(RoadObsPlot *oplot)
{
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::initRoadData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl);
#endif
	// Does not work, why ?
	//if (!oplot->isallObs())
	//	oplot->clear(); // ???
	//oplot->clearVisibleStations();
	oplot->setLabels(labels);
  oplot->columnName = m_columnName;

  plotTime = oplot->getObsTime();
  timeDiff= oplot->getTimeDiff();
  fileTime = filetime_;

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, filetime_); 
  map<std::string, vector<diStation> * >::iterator its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end())
  {
		stationlist = its->second;
  }

	map<int, std::string> lines_map;
	// get av vector of paramater names
	std::vector<std::string> paramnames;
	int theresult = diParam::initParameters(headerfile_);
	if (theresult)
			  {
		// take a local copy
		vector<diParam> * params = NULL;
		map<std::string, vector<diParam> * >::iterator itp = diParam::params_map.find(headerfile_);
		if (itp != diParam::params_map.end())
			  {
			params = itp->second;
	  }
		/* this should not happen if configured properly */
		if (params == NULL)
		{
			//oplot->roadobsHeader = false;
			METLIBS_LOG_ERROR(" ObsRoad::initData() error, parameterfile: " << headerfile_);
#ifdef DEBUGPRINT
			METLIBS_LOG_DEBUG("++ ObsRoad::initData() done, error finding parameters ++");
#endif
		}
		else
		{
			// the dynamic part
			// the data value parameters
			for (int i = 0; i < params->size(); i++)
			{
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
	map<int, std::string>::iterator it=lines_map.begin();	
  for(;it!=lines_map.end(); it++) {
		//FIXME: Stnid !!!!
		//INDEX in station list
		stnid = it->first;
		str = it->second;
		miutil::trim(str);
		// Append every line to 
		lines.push_back(str);
  }

	separator = "|";
  
	decodeData();

	oplot->addObsData(vObsData);

  // Force setting of dummy data
  oplot->setData();
  // clear plot positions
  //oplot->clearVisibleStations();
  // make a dummy plot to compute a list of stations to be plotted
  oplot->preparePlot();

#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::initRoadData()done ++ ");
#endif
}

void ObsRoad::readData(RoadObsPlot *oplot)
{
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::readData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl);
#endif
	// read the headerfile if needed
		readHeader(oplot);
	readRoadData(oplot);
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::readData()done ++ ");
#endif
}

void ObsRoad::readRoadData(RoadObsPlot *oplot)
{
#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::readRoadData( filename= " << filename_ << " databasefile= " << databasefile_ << " stationfile= " << stationfile_ << " headerfile= " << headerfile_ << " filetime= " << filetime_.isoTime() << " )++ " << endl);
#endif

  
	oplot->setLabels(labels);
  oplot->columnName = m_columnName;
   
  plotTime = oplot->getObsTime();
  timeDiff= oplot->getTimeDiff();
  fileTime = filetime_;

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, filetime_); 
  map<std::string, vector<diStation> * >::iterator its = diStation::station_map.find(stationfile_);
  if (its != diStation::station_map.end())
  {
	  stationlist = its->second;
  }
  // FIXME: For now, does nothing...
  road.open();
  
  //vector<std::string> lines;
	// FIXME: I think we must keep stations_to_plot!
  map<int, std::string> lines_map;
  // Members in the global stationlist that is not in the stations_to_plot list are not read from road
  // This should improve performancestd::cerr << "Nextplot.size() " << nextplot.size() << std::endl;
  const std::vector<int> & plot_stations = oplot->getStationsToPlot();
  METLIBS_LOG_DEBUG("plot_stations.size() " << plot_stations.size());
  road.getData(plot_stations, lines_map);
	// FIXME: For now, does nothing...
	road.close();

	// decode the data...
  int stnid;
	std::string str;
	map<int, std::string>::iterator it=lines_map.begin();	
  for(;it!=lines_map.end(); it++) {
		//FIXME: Stnid !!!!
		//INDEX in station list
	stnid = it->first;
	str = it->second;
	miutil::trim(str);
		// Append every line to 
		lines.push_back(str);
	}
	separator = "|";

	// remove the fake data....
	vObsData.clear();
	mObsData.clear();

	decodeData();

	oplot->replaceObsData(vObsData);

	oplot->clearVisibleStations();

	// Dont set data here, the manager will do that...
	// HERE, we should be finished!


#ifdef DEBUGPRINT
	METLIBS_LOG_DEBUG("++ ObsRoad::readRoadData()done ++ ");
#endif
}

// from ObsAscii
// FIXME: Not used for now...
void ObsRoad::yoyoPlot(const miTime &filetime, RoadObsPlot *oplot)
{
  METLIBS_LOG_SCOPE();
  oplot->setLabels(labels);
  oplot->columnName = m_columnName;

  plotTime = oplot->getObsTime();
  timeDiff= oplot->getTimeDiff();
  fileTime = filetime;

  readDecodeData();

  oplot->addObsData(vObsData);
}
// FIXME: Should be used ?
void ObsRoad::yoyoMetadata(ObsMetaData *metaData)
{
  METLIBS_LOG_SCOPE();
  readDecodeData();
  metaData->setMetaData(mObsData);
}

//####################################################################
// FIXME: Not used for now.
void ObsRoad::readHeaderInfo(const string& filename, const string& headerfile,
    const vector<string>& headerinfo)
{
  METLIBS_LOG_SCOPE(LOGVAL(filename) << LOGVAL(headerfile) << LOGVAL(headerinfo.size()));

  m_needDataRead = true;
  if (not headerinfo.empty()) {
    lines = headerinfo;
  } else if (not headerfile.empty()) {
    readData(headerfile);
  } else {
    m_needDataRead = false;
    readData(filename);
  }
  if (m_needDataRead)
    m_filename = filename;
}
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
  for (size_t i=1; i<in_out.size(); ++i)
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
    in_out.push_back(joined.substr(p_open+1, p_close - p_open - 1));
    METLIBS_LOG_DEBUG(LOGVAL(in_out.back()));
    start = p_close + 1;
        }
  return true;
}

void ObsRoad::parseHeaderBrackets(const std::string& str)
{
  METLIBS_LOG_SCOPE(LOGVAL(str));
  vector<string> pstr = miutil::split_protected(str, '"', '"');
  if (pstr.size() <= 1)
    return;

  if (pstr[0] == "COLUMNS") {
    if (not separator.empty())
      pstr= miutil::split(str, separator);
    for (size_t j=1; j<pstr.size(); j++) {
	      miutil::remove(pstr[j], '"');
      const vector<std::string> vs = miutil::split(pstr[j], ":");
	      if (vs.size()>1) {
        m_columnName.push_back(vs[0]);
        m_columnType.push_back(miutil::to_lower(vs[1]));
		if (vs.size()>2) {
          m_columnTooltip.push_back(vs[2]);
        } else {
          m_columnTooltip.push_back("");
          }
        }
      }
  } else if (pstr[0] == "UNDEFINED") {
    const std::vector<std::string> vs= miutil::split(pstr[1], ",");
    asciiColumnUndefined.insert(vs.begin(), vs.end());
  } else if (pstr[0] == "SKIP_DATA_LINES" && pstr.size()>1) {
    asciiSkipDataLines = miutil::to_int(pstr[1]);
  } else if (pstr[0] == "LABEL") {
    labels.push_back(std::make_shared<LabelPlotCommand>(str.substr(5)));
  } else if (pstr[0] == "SEPARATOR") {
    separator = pstr[1];
    }
}

void ObsRoad::decodeHeader()
{
  METLIBS_LOG_SCOPE();

  vector<string> vstr;
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
  asciiSkipDataLines= 0;

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
  for (size_t i=0; i<vstr.size(); ++i)
    parseHeaderBrackets(vstr[i]);

  METLIBS_LOG_DEBUG("#columns: " << m_columnType.size() << LOGVAL(asciiSkipDataLines));

  knots=false;
  for (size_t i=0; i<m_columnType.size(); i++) {
    METLIBS_LOG_DEBUG("column " << i << " : " << m_columnName[i] << "  " << m_columnType[i]);

    const std::string& ct = m_columnType[i];
    const std::string ct_lower = miutil::to_lower(ct);
    const std::string cn_lower = miutil::to_lower(m_columnName[i]);

    if      (ct=="d")
      asciiColumn["date"]= i;
    else if (ct=="t")
      asciiColumn["time"]= i;
    else if (ct=="year")
      asciiColumn["year"] = i;
    else if (ct=="month")
      asciiColumn["month"] = i;
    else if (ct=="day")
      asciiColumn["day"] = i;
    else if (ct=="hour")
      asciiColumn["hour"] = i;
    else if (ct=="min")
      asciiColumn["min"] = i;
    else if (ct=="sec")
      asciiColumn["sec"] = i;
    else if (ct_lower=="lon")
      asciiColumn["x"]= i;
    else if (ct_lower=="lat")
      asciiColumn["y"]= i;
    else if (ct_lower=="dd")
      asciiColumn["dd"]= i;
    else if (ct_lower=="ff") //Wind speed in m/s
      asciiColumn["ff"]= i;
    else if (ct_lower=="ffk") //Wind speed in knots
      asciiColumn["ff"]= i;
    else if (ct_lower=="image")
      asciiColumn["image"]= i;
    else if (cn_lower=="lon" && ct=="r") //Obsolete
      asciiColumn["x"]= i;
    else if (cn_lower=="lat" && ct=="r") //Obsolete
      asciiColumn["y"]= i;
    else if (cn_lower=="dd" && ct=="r") //Obsolete
      asciiColumn["dd"]= i;
    else if (cn_lower=="ff" && ct=="r") //Obsolete
      asciiColumn["ff"]= i;
    else if (cn_lower=="ffk" && ct=="r") //Obsolete
      asciiColumn["ff"]= i;
    else if (cn_lower=="image" && ct=="s") //Obsolete
      asciiColumn["image"]= i;
    else if (cn_lower=="name" || ct=="id")
      asciiColumn["Name"]= i;

    if (ct_lower=="ffk" || cn_lower=="ffk")
      knots=true;
    }

  fileOK= true;
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
// FIXME: The automation code ?
// FIXME: float or string value
void ObsRoad::cloud_type_string(ObsData& d, double v)
{
  int type = int(v) / 10;
  float value = float(int(v) % 10);

  if (value < 1)
    return;

  if (type == 1)
    d.stringdata["Ch"] = miutil::from_number(value);
  if (type == 2)
    d.stringdata["Cm"] = miutil::from_number(value);
  if (type == 3)
    d.stringdata["Cl"] = miutil::from_number(value);

}

string ObsRoad::height_of_clouds_string(double height)
{
  METLIBS_LOG_SCOPE(height);
  height = (height*3.2808399)/100.0;
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
    d.fdata["Ch"] = value;
  if (type == 2)
    d.fdata["Cm"] = value;
  if (type == 3)
    d.fdata["Cl"] = value;

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
    return v/12.5; //% -> oktas
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

  miDate filedate= fileTime.date();

  // skip header; this relies on decodeHeader having trimmed the header lines
  size_t ii = 0;
  while (ii < lines.size() && not miutil::contains(lines[ii], "[DATA]")) {
    METLIBS_LOG_DEBUG("skip '" << lines[ii] << "'");
		//cerr << "skip '" << lines[ii] << "'" << endl;
    ++ii;
  }
  ii += 1; //skip [DATA] line too

  for (int i=0; ii < lines.size() and i < asciiSkipDataLines; ++i) {
    METLIBS_LOG_DEBUG("skip data start '" << lines[ii++] << "'");
		//cerr << "skip data start '" << lines[ii++] << "'" << endl;
    miutil::trim(lines[ii]);
    if (lines[ii].empty() or lines[ii][0]=='#')
      continue;
    ii += 1;
  }

  int index = 0;
  for (; ii < lines.size(); ++ii) {
    METLIBS_LOG_DEBUG("read '" << lines[ii] << "'");
		//cerr << "read '" << lines[ii] << "'" << endl;
    miutil::trim(lines[ii]);
    if (lines[ii].empty() or lines[ii][0]=='#')
      continue;

    vector<string> pstr;
    if (not separator.empty())
      pstr = miutil::split(lines[ii], separator, false);
    else
      pstr = miutil::split_protected(lines[ii], '"', '"');

    ObsData  obsData;
		// Set metadata for station...
		obsData.stringdata["data_type"] = (*stationlist)[index].station_type();
		obsData.fdata["auto"] = (*stationlist)[index].environmentid();
		obsData.fdata["isdata"] = (*stationlist)[index].data();
    index ++;
    const size_t tmp_nColumn = std::min(pstr.size(), m_columnType.size());
    // fill both stringdata and fdata
    // stringdata
    for (size_t i=0; i<tmp_nColumn; i++) {
      if (not asciiColumnUndefined.count(pstr[i])) {
        if (m_columnName[i] == "Cl" || m_columnName[i] == "Cm" ||  m_columnName[i] == "Ch") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i])) {
              // Convert to the symbol dataspace
              cloud_type_string(obsData, miutil::to_float(pstr[i]));
            }
        } else if (m_columnName[i] == "h") {
            if (pstr[i] != undef_string)
              if (miutil::is_number(pstr[i]))
                // Convert to clouds dataspace
                obsData.stringdata[m_columnName[i]] = height_of_clouds_string(miutil::to_float(pstr[i]));
        } else if (m_columnName[i] == "N") {
            if (pstr[i] != undef_string)
              if (miutil::is_number(pstr[i]))
                // Convert to clouds dataspace
                obsData.stringdata[m_columnName[i]] = miutil::from_number(percent2oktas(miutil::to_float(pstr[i])));
        } else if (m_columnName[i] == "vs") {
            if (pstr[i] != undef_string)
              if (miutil::is_number(pstr[i]))
                // Convert to clouds dataspace
                obsData.stringdata[m_columnName[i]] = miutil::from_number(ms2code4451(miutil::to_float(pstr[i])));
        } else {        
          if (pstr[i] != undef_string)
            obsData.stringdata[m_columnName[i]] = pstr[i];
        }
      }
    }
    // fdata note Cl, Cm, Ch, adjustment
    for (size_t i=0; i<tmp_nColumn; i++) {
      if (not asciiColumnUndefined.count(pstr[i])) {
        if (m_columnName[i] == "Cl" || m_columnName[i] == "Cm" ||  m_columnName[i] == "Ch") {
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i])) {
              // Convert to the symbol dataspace
              cloud_type(obsData, miutil::to_float(pstr[i]));
            }
        } else if (m_columnName[i] == "h") {
            if (pstr[i] != undef_string)
              if (miutil::is_number(pstr[i]))
                // Convert to clouds dataspace
                obsData.fdata[m_columnName[i]] = height_of_clouds(miutil::to_float(pstr[i]));
        } else if (m_columnName[i] == "N") {
            if (pstr[i] != undef_string)
              if (miutil::is_number(pstr[i]))
                // Convert to clouds dataspace
                obsData.fdata[m_columnName[i]] = percent2oktas(miutil::to_float(pstr[i]));
        } else if (m_columnName[i] == "vs") {
            if (pstr[i] != undef_string)
              if (miutil::is_number(pstr[i]))
                // Convert to vs dataspace
                obsData.fdata[m_columnName[i]] = ms2code4451(miutil::to_float(pstr[i]));  
        } else {        
          if (pstr[i] != undef_string)
            if (miutil::is_number(pstr[i]))
              obsData.fdata[m_columnName[i]] = miutil::to_float(pstr[i]);
        }
      }
    }
    float value;
    std::string text;
    if (getColumnValue("x", pstr, value)||getColumnValue("Lon", pstr, value))
      if (value != undef)
        obsData.xpos = value;
    if (getColumnValue("y", pstr, value)||getColumnValue("Lat", pstr, value))
      if (value != undef)
        obsData.ypos = value;
    if (getColumnValue("Name", pstr, text))
      obsData.id = text;
    if (getColumnValue("ff", pstr, value))
      if (value != undef)  
        obsData.fdata["ff"] = knots ? diutil::knots2ms(value) : value;
    if (getColumnValue("dd", pstr, value))
      if (value != undef)
        obsData.fdata["dd"] = value;
    if (getColumnValue("image", pstr, text))
      obsData.stringdata["image"] = text;

	if (useTime) {
      miClock clock;
      miDate date;
      if (isoTime) {
        if (getColumnValue("time", pstr, text)) {
          clock = miClock(text);
	  } else {
          METLIBS_LOG_WARN("time column missing");
          continue;
	  }
	  } else {
        int hour=0, min=0, sec=0;
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
          date = miDate(text);
	} else {
          METLIBS_LOG_WARN("date column missing");
          continue;
	}
      } else if (allTime) {
        int year=0,month=0,day=0;
        if (getColumnValue("year", pstr, year) and getColumnValue("month", pstr, month) and getColumnValue("day", pstr, day)) {
          date = miDate(year, month, day);
        } else {
          METLIBS_LOG_WARN("year/month/day column missing");
          continue;
      }
      } else  {
        date = filedate;
    }
      obstime = miTime(date,clock);
      
      if (not allTime) {
        int mdiff= miTime::minDiff(obstime,fileTime);
        if      (mdiff<-12*60)
          obstime.addHour(24);
        else if (mdiff> 12*60)
          obstime.addHour(-24);
      }
      
      METLIBS_LOG_DEBUG(LOGVAL(obstime) << LOGVAL(plotTime) << LOGVAL(timeDiff));

      if (timeDiff < 0 || abs(miTime::minDiff(obstime, plotTime)) < timeDiff || (timeDiff == 0 && abs(miTime::minDiff(obstime, plotTime)) == 0))
        obsData.obsTime = obstime;
      else
        continue;
    }
#ifdef DEBUGPRINT
    //Produces a lot of output...
    std::map<std::string,float>::iterator itf = obsData.fdata.begin();
    METLIBS_LOG_INFO("fdata");
    for (; itf != obsData.fdata.end(); itf++)
    {
      METLIBS_LOG_INFO(itf->first << ", " << itf->second);
    }

    std::map<std::string,std::string>::iterator its = obsData.stringdata.begin();
    METLIBS_LOG_INFO("stringdata");
    for (; its != obsData.stringdata.end(); its++)
    {
      METLIBS_LOG_INFO(its->first << ", " << its->second);
    }
#endif
		//cerr << "obsdata added: " << obsData.obsTime << endl;
    vObsData.push_back(obsData);
    mObsData[obsData.id] = obsData;
  }
}

VprofPlot* ObsRoad::getVprofPlot(const std::string& modelName,
    const std::string& station, const miutil::miTime& time)
{
  if (modelName=="AMDAR" || modelName=="PILOT") {
    METLIBS_LOG_WARN("This model is not implemented yet: " << modelName);
    return 0;
  }
  // This creates the stationlist
  // We must also init the connect string to mora db
  Roaddata::initRoaddata(databasefile_);
  diStation::initStations(stationfile_);
  // get the pointer to the actual station vector
  vector<diStation> * stations = 0;
  map<std::string, vector<diStation> * >::iterator its = diStation::station_map.find(stationfile_);
  if (its == diStation::station_map.end()) {
    METLIBS_LOG_ERROR("Unable to find stationlist: " <<stationfile_);
    return 0;
  }
  stations = its->second;
  if (stations == 0) {
    METLIBS_LOG_ERROR("Empty stationlist: " <<stationfile_);
    return 0;
  }
  int nStations = stations->size();
  // Return if empty station list
  if (nStations<1) return 0;
  
  int n= 0;
  while (n<nStations && (*stations)[n].name()!=station) n++;
  // Return if station not found in station list
  if (n==nStations)
  {
	  METLIBS_LOG_ERROR("Unable to find station: " << station << " in stationlist!");
	  return 0;
  }
  const diStation& station_n = (*stations)[n];

  std::unique_ptr<VprofPlot> vp(new VprofPlot());

  vp->text.index= -1;
  vp->text.modelName = modelName;
//vp->text.posName= station;
//####  if (contents[n].stationID.substr(0,2)=="99")
//####    vp->text.posName=
//####	contents[n].stationID.substr(2,contents[n].stationID.length()-2);
//####  else
//####    vp->text.posName= contents[n].stationID;
  vp->text.posName= miutil::trimmed(station_n.name());
  vp->text.prognostic= false;
  vp->text.forecastHour= 0;
  vp->text.validTime= time;
  vp->text.latitude=  station_n.lat();
  vp->text.longitude= station_n.lon();
  vp->text.kindexFound= false;


  /* HERE we should get the data from road */

  Roaddata road = Roaddata(databasefile_, stationfile_, headerfile_, time);
  road.open();

  vector<diStation> stations_to_plot;
  /* only get data for one station */
  stations_to_plot.push_back(station_n);
  /* get the data */
  map<int, vector<RDKCOMBINEDROW_2 > > raw_data_map;
  road.getData(stations_to_plot, raw_data_map);

  road.close();
  vector<RDKCOMBINEDROW_2 > raw_data;
  map<int, vector<RDKCOMBINEDROW_2 > >::iterator itd = raw_data_map.find(station_n.stationID());
  if (itd != raw_data_map.end()) {
    raw_data = itd->second;
  }
  /* Sort the data */
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofRTemp::getStation: Lines returned from road.getData(): " << raw_data.size());
#endif
  // For now, we dont use the surface data!
  vector <diParam> * params = 0;
  map<std::string, vector<diParam> * >::iterator itp = diParam::params_map.find(headerfile_);
  if (itp == diParam::params_map.end()) {
    // the code below would try to use 'params'
    return 0;
  }
  params = itp->second;
  if (params == 0)
    return 0;
  /* map the data and sort them */
  map < std::string, map<float, RDKCOMBINEDROW_2 > > data_map;
  int no_of_data_rows = raw_data.size();
  for (size_t k = 0; k < params->size(); k++) {
    map<float, RDKCOMBINEDROW_2 > tmpresult;
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
  METLIBS_LOG_DEBUG("VprofRTemp::getStation: Data is now sorted!");
#endif

  /* Use TTT + 1 to detrmine no of levels */

  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittt = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittd = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itdd = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itff = data_map.begin();
  /* Siginifcant wind levels */
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_1 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_4 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_5 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_6 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_7 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_12 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_13 = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itsig_14 = data_map.begin();

  /* the surface values */
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittts = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator ittds = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itdds = data_map.begin();
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itffs = data_map.begin();
  /* the ground pressure */
  map< std::string, map<float, RDKCOMBINEDROW_2 > >::iterator itpps = data_map.begin();

  map<float, RDKCOMBINEDROW_2 >::iterator ittp;

  ittts = data_map.find("TTTs");
  float TTTs_value = -32767.0;
  if (ittts != data_map.end())
  { 
	  ittp = ittts->second.begin();
	  for (; ittp != ittts->second.end(); ittp++) {
		TTTs_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("TTTs_value: " << TTTs_value);
      }
  }

  ittds = data_map.find("TdTdTds");
  float TdTdTds_value = -32767.0;

  if (ittds != data_map.end())
  {
	  ittp = ittds->second.begin();
	  for (; ittp != ittds->second.end(); ittp++) {
		TdTdTds_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("TdTdTds_value: " << TdTdTds_value);
      }
  }

  itdds = data_map.find("dds");
  float dds_value = -32767.0;
  if (itdds != data_map.end())
  {
	  ittp = itdds->second.begin();
	  for (; ittp != itdds->second.end(); ittp++) {
		dds_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("dds_value: " << dds_value);
	  }
  }

  itffs = data_map.find("ffs");
  float ffs_value = -32767.0;
  if (itffs != data_map.end())
  {
	  ittp = itffs->second.begin();
	  for (; ittp != itffs->second.end(); ittp++) {  
		ffs_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("ffs_value: " << ffs_value);
	  }	
  }

  itpps = data_map.find("PPPP");
  float PPPPs_value = -32767.0;
  if (itpps != data_map.end())
  {
	  ittp = itpps->second.begin();
	  for (; ittp != itpps->second.end(); ittp++) {  
		PPPPs_value = ittp->second.floatvalue;
		//METLIBS_LOG_DEBUG("PPPPs_value: " << PPPPs_value);
	  }	
  }
#ifdef DEBUGPRINT
  METLIBS_LOG_DEBUG("VprofRTemp::getStation: surface data TTTs: " << TTTs_value << " TdTdTds: " << TdTdTds_value << " dds: " << dds_value << " ffs: " << ffs_value << " PPPP: " << PPPPs_value);
#endif
  
  ittt = data_map.find("TTT");
  /* Only surface observations */

  if (ittt == data_map.end())
  {
    vp->prognostic= false;
    vp->maxLevels= 0;
    return vp.release();
  }

  ittd = data_map.find("TdTdTd");
  /* Only surface observations */

  if (ittd == data_map.end())
  {
    vp->prognostic= false;
    vp->maxLevels= 0;
    return vp.release();
  }

  itdd = data_map.find("dd");
  /* Only surface observations */

  if (itdd == data_map.end())
  {
    vp->prognostic= false;
    vp->maxLevels= 0;
    return vp.release();
  }

  itff = data_map.find("ff");
  /* Only surface observations */

  if (itff == data_map.end())
  {
    vp->prognostic= false;
    vp->maxLevels= 0;
    return vp.release();
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
  map<float, RDKCOMBINEDROW_2 > sig_1;
  map<float, RDKCOMBINEDROW_2 > sig_4;
  map<float, RDKCOMBINEDROW_2 > sig_7;
  map<float, RDKCOMBINEDROW_2 > sig_12;
  map<float, RDKCOMBINEDROW_2 > sig_13;
  map<float, RDKCOMBINEDROW_2 > sig_14;

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


  float p,tt,td,fff,ddd;
  int   dd,ff,bpart;
  int   ffmax= -1, kmax= -1;

//####################################################################
//  if (station=="03005" || station=="03953") {
//    METLIBS_LOG_DEBUG("-------------------------------------------------");
//    METLIBS_LOG_DEBUG(station<<"   "<<time);
//    for (int k=0; k<nLevel; k++) {
//      METLIBS_LOG_DEBUG("  p="<<contents[n].data.PPPP[k]
//          <<"  dd="<<contents[n].data.dd[k]
//          <<"  ff="<<contents[n].data.ff[k]
//          <<"  t="<<contents[n].data.TTT[k]
//          <<"  td="<<contents[n].data.TdTdTd[k]
//          <<"  flags1="<<contents[n].data.flags1[k]);
//    }
//    METLIBS_LOG_DEBUG("-------------------------------------------------");
//  }
//####################################################################
/* Can we trust that all parameters, has the same number of levels ? */
  ittp = ittt->second.begin();
  // Here should we sort!
  std::set <float> keys;
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
  std::set<float>::iterator it=keys.begin();
  for (; it != keys.end(); it++)
  {
	  float key = *it;
    p = key *0.01;
	  //p= ittp->second[key].floatvalue * 0.01;
	  // check with ground level pressure !
	  if ((p > PPPPs_value) && (PPPPs_value != -32767.0))
		  continue;
	  if (p>0. && p<1300.) {
		  if (key == (PPPPs_value * 100.0))
		  {
			  tt = TTTs_value;
			  td = TdTdTds_value;
		  }
		  else
		  {
        tt = -30000.;
        if (ittt->second.count(key))
          tt= ittt->second[key].floatvalue;
        td = -30000.;
        if (ittd->second.count(key))
          td= ittd->second[key].floatvalue;
		  }
		  if (tt>-30000.) {
			  vp->ptt.push_back(p);
			  vp->tt.push_back(tt);
			  if (td>-30000.) {
				  vp->ptd.push_back(p);
				  vp->td.push_back(td);
				  vp->pcom.push_back(p);
				  vp->tcom.push_back(tt);
				  vp->tdcom.push_back(td);
			  }
		  }
		  if (key == (PPPPs_value * 100.0))
		  {
			  dd = dds_value;
			  ff = ffs_value;
		  }
		  else
		  {
			  dd = -1;
        if (itdd->second.count(key))
          dd= int(itdd->second[key].floatvalue);
			  ff = -1;
        if (itff->second.count(key))
          ff= int(itff->second[key].floatvalue);
		  }
		  /* Wind should always be plotted in knots,
		  convert from m/s as they are stored in road */
		  ff = diutil::ms2knots(ff);
		  if (dd>=0 && dd<=360 && ff>=0) {
			  // Only plot the significant winds
			  bpart = 0;
			  // SHOULD it always be 1 ?
        if (sig_1.count(key) || sig_4.count(key) || sig_7.count(key) || sig_12.count(key) || sig_13.count(key) || sig_14.count(key))
          bpart = 1;
			  // Plot winds at significant levels and at standard pressure levels.
				if (bpart > 0 || p == 1000 || p == 925 || p == 850 || p == 800 || p == 700 || p == 500 || p == 400 || p == 300 || p == 200 || p == 100 || p == 50)
				{
					  vp->sigwind.push_back(bpart);
					  vp->puv.push_back(p);
					  vp->dd.push_back(dd);
					  vp->ff.push_back(ff);
					  // convert to east/west and north/south component
					  fff= float(ff);
					  ddd= (float(dd)+90.)*DEG_TO_RAD;
					  vp->uu.push_back( fff*cosf(ddd));
					  vp->vv.push_back(-fff*sinf(ddd));

					  if (ff>ffmax) {
						  ffmax= ff;
						  kmax = vp->sigwind.size() - 1;
					  }
			  }
		  }
	  }
  } /* End for */
  if (kmax>=0)
    vp->sigwind[kmax]= 3;
  vp->prognostic= false;
  vp->maxLevels = std::max(vp->ptt.size(), vp->puv.size());
  return vp.release();
}

//#endif //ROADOBS
