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

#include "diLocalSetupParser.h"

#include "diColourShading.h"
#include "diLinetype.h"
#include "diPattern.h"
#include "miSetupParser.h"

#include <puTools/miStringFunctions.h>
#include <puCtools/mkdir.h>
#include <puCtools/stat.h>

#include <fstream>
#include <list>

#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define MILOGGER_CATEGORY "diana.LocalSetupParser"
#include <miLogger/miLogging.h>


/// PVERSION is defined due to debian packing using the metno-debuild tool
#ifndef PVERSION
#define PVERSION
#endif

const std::string SectColours=     "COLOURS";
const std::string SectPalettes=    "PALETTES";
const std::string SectFillPatterns="FILLPATTERNS";
const std::string SectLineTypes=   "LINETYPES";
const std::string SectQuickMenus=  "QUICKMENUS";
const std::string SectBasics=      "BASIC";
const std::string SectInfoFiles=   "TEXT_INFORMATION_FILES";

// static members
std::string LocalSetupParser::setupFilename;
std::vector<QuickMenuDefs>      LocalSetupParser::quickmenudefs;
std::map<std::string,std::string>     LocalSetupParser::basic_values;
std::map<std::string,InfoFile>     LocalSetupParser::infoFiles;
std::vector<std::string>           LocalSetupParser::langPaths;


bool LocalSetupParser::makeDirectory(const std::string& filename, std::string & error)
{
  pu_struct_stat buff;
  if (pu_stat(filename.c_str(), &buff) != -1){
    if ( S_ISDIR(buff.st_mode) ){
      return true;
    }
  }

  if (pu_mkdir(filename.c_str(), 0755) != 0) {
    error = strerror(errno);
    return false;
  }
  return true;
}

bool LocalSetupParser::parse(std::string& mainfilename)
{
  METLIBS_LOG_INFO("LocalSetupParser::parse:" << mainfilename);

  //find $HOME, and make homedir
  std::string homedir=miutil::from_c_str(getenv("HOME"));
  homedir += "/.diana";
  std::string error;
  if (makeDirectory(homedir,error)) {
    std::string workdir = homedir + "/work";
    if (makeDirectory(workdir,error))
      basic_values["workdir"]    = workdir;

    std::string cachedir = homedir + "/cache";
    if (makeDirectory(cachedir,error))
      basic_values["cachedir"] = cachedir;
  } else {
    homedir=".";
  }
  basic_values["homedir"]    = homedir;

  //if no setupfile specified, use previus setupfile
  if (not mainfilename.empty())
    setupFilename = mainfilename;

  //if no setupfile, use default
  if (setupFilename.empty()) {
    setupFilename = "diana.setup";
    std::string filename_str = setupFilename;
    METLIBS_LOG_INFO("filename:" << setupFilename);
    std::ifstream file(setupFilename.c_str());
    if (!file) {
      setupFilename = homedir + "/diana.setup";
      filename_str += " or ";
      filename_str += setupFilename;
      METLIBS_LOG_INFO("filename:" << setupFilename);
      std::ifstream file2(setupFilename.c_str());
      if (!file2) {
        setupFilename = "/etc/diana/" PVERSION "/diana.setup-COMMON";
        filename_str += " or ";
        filename_str += setupFilename;
        METLIBS_LOG_INFO("filename:" << setupFilename);
        std::ifstream file3(setupFilename.c_str());
        if (!file3) {
          METLIBS_LOG_ERROR("LocalSetupParser::readSetup. cannot open default setupfile "
          << filename_str);
          METLIBS_LOG_ERROR("Try diana-" PVERSION ".bin -s setupfile");
          return false;
        }
      }
    }
  }

  if (! miutil::SetupParser::parse( setupFilename ) )
    return false;

  if (!parseBasics(SectBasics)) return false;
  if (!parseColours(SectColours)) return false;
  if (!parsePalettes(SectPalettes)) return false;
  if (!parseFillPatterns(SectFillPatterns)) return false;
  if (!parseLineTypes(SectLineTypes)) return false;
  if (!parseQuickMenus(SectQuickMenus)) return false;
  if (!parseTextInfoFiles(SectInfoFiles)) return false;

  // return the setupFilename
  mainfilename=setupFilename;

  return true;
}



// parse basics-section
bool LocalSetupParser::parseBasics(const std::string& sectname){

  const std::string key_fontpath= "fontpath";
  const std::string key_docpath=  "docpath";
  const std::string key_obspath=  "obsplotfilepath";
  const std::string key_qserver=  "qserver";
  const std::string key_imagepath="imagepath";
  const std::string key_langpaths="languagepaths";
  const std::string key_language= "language";
  const std::string key_setenv= "setenv";

  // default values
  const std::string SHAREDIR = DATAROOTDIR "/diana/" PVERSION;
  std::string langpaths = SHAREDIR + "/lang:${QTDIR}/translations";
  std::string language = "en";
  basic_values[key_fontpath] = SHAREDIR + "/fonts";
  basic_values[key_docpath] = "share/doc/diana-" PVERSION;
  basic_values[key_obspath] = SHAREDIR;
  basic_values[key_qserver] = "/usr/bin/coserver";
  basic_values[key_imagepath] = SHAREDIR + "/images";
  basic_values[key_language] = language;

  std::vector<std::string> list,tokens;
  std::string key,value;
  int i,n;

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    miutil::SetupParser::splitKeyValue(list[i],key,value);

    // everything into basic_values map
    basic_values[key] = value;

    if (key==key_langpaths){
      langpaths= value;
    } else if (key==key_setenv){
      std::vector<std::string> part = miutil::split(value, ",");
      if(part.size()==3){
#ifdef __WIN32__
        //TODO: This is broken, disregards third argument (replace option)
	std::string envst = part[0] + "=" + part[1];
	putenv(envst.c_str());
#else
        setenv(part[0].c_str(), part[1].c_str(), miutil::to_int(part[2]));
#endif
      }
    }
  }

  // fix language paths
  //   checkEnvironment(langpaths);
  langPaths = miutil::split(langpaths, ":");

  return true;
}


// parse text-information-files
bool LocalSetupParser::parseTextInfoFiles(const std::string& sectname)
{
  infoFiles.clear();

  const std::string key_name= "name";
  const std::string key_file= "file";
  const std::string key_type= "type";
  const std::string key_font= "font";

  const std::string def_type= "auto";
  const std::string def_font= "auto";

  std::vector<std::string> list;
  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  for (const std::string line : list) {
    std::string name,filename, type = def_type, font = def_font;
    const std::vector<std::string> tokens2 = miutil::split(line, " ");
    for (unsigned int j=0; j<tokens2.size(); j++){
      std::string key,value;
      miutil::SetupParser::splitKeyValue(tokens2[j], key, value);
      if (key==key_name)
        name= value;
      else if (key==key_file)
        filename= value;
      else if (key==key_type)
        type= value;
      else if (key==key_font)
        font= value;
    }
    miutil::trim(name);
    miutil::trim(filename);
    miutil::trim(type);
    miutil::trim(font);
    if (not name.empty() && not filename.empty()){
      infoFiles[name].name= name;
      infoFiles[name].filename= filename;
      infoFiles[name].doctype= type;
      infoFiles[name].fonttype= font;
    }
  }

  return true;
}


// parse section containing colour definitions
bool LocalSetupParser::parseColours(const std::string& sectname){
  // default colours
  const int numcols= 21;
  const std::string colnames[numcols]=
  {"black","white","red","green","blue","yellow",
      "dark_green","dark_yellow","dark_red","dark_blue",
      "brown","orange","cyan","magenta",
      "purple","lightblue","dnmi_green","dnmi_blue",
      "grey25","grey50","grey90"};
  const unsigned char cols[numcols][3]=
  { {0,0,0},{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},
      {0,127,127},{178,178,0},{178,0,0},{0,0,178},
      {178,51,0},{255,89,0},{0,255,255},{255,0,255},
      {77,0,77},{51,51,255},{43,120,36},{0,54,125},
      {64,64,64},{127,127,127},{230,230,230}};


  std::vector<std::string> list,tokens,stokens;
  std::string key,value,value2;
  int i,n;
  Colour c;
  unsigned char r,g,b,a;
  Colour::ColourInfo cinfo;

  // -- default colours
  for (i=0; i<numcols; i++){
    Colour::define(colnames[i],cols[i][0],cols[i][1],cols[i][2]);
    cinfo.name=colnames[i];
    cinfo.rgb[0]= cols[i][0];
    cinfo.rgb[1]= cols[i][1];
    cinfo.rgb[2]= cols[i][2];
    Colour::addColourInfo(cinfo);
  }

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    miutil::SetupParser::splitKeyValue(list[i], key, stokens);
    if (stokens.size()>2){
      r= atoi(stokens[0].c_str());
      g= atoi(stokens[1].c_str());
      b= atoi(stokens[2].c_str());
      if (stokens.size()>3) a= atoi(stokens[3].c_str());
      else a= 255;
      Colour::define(key,r,g,b,a);
      cinfo.rgb[0]= r;
      cinfo.rgb[1]= g;
      cinfo.rgb[2]= b;
      cinfo.name= miutil::to_lower(key);
      Colour::addColourInfo(cinfo);
    }
  }
  return true;
}



// parse section containing colour-palette definitions
bool LocalSetupParser::parsePalettes(const std::string& sectname){

  // first define default types/values
  ColourShading::ColourShadingInfo csinfo;
  const int nbaseRGB=5;
  const float baseRGB[nbaseRGB][3]=
  { {0.0,0.0,0.5}, {0.0,1.0,1.0}, {0.0,0.5,0.0},
      {1.0,1.0,0.0}, {1.0,0.0,0.0} };

  //colour shading
  const int divRGB= 3;
  const int mRGBtab= (nbaseRGB-1)*divRGB + 1;
  float RGBtab[mRGBtab][3];

  RGBtab[0][0]= baseRGB[0][0];
  RGBtab[0][1]= baseRGB[0][1];
  RGBtab[0][2]= baseRGB[0][2];
  int nRGBtab=1;

  for (int j=1; j<nbaseRGB; j++) {
    float rstep= (baseRGB[j][0]-baseRGB[j-1][0])/float(divRGB);
    float gstep= (baseRGB[j][1]-baseRGB[j-1][1])/float(divRGB);
    float bstep= (baseRGB[j][2]-baseRGB[j-1][2])/float(divRGB);
    for (int i=1; i<divRGB; i++) {
      RGBtab[nRGBtab][0]= baseRGB[j-1][0] + rstep*float(i);
      RGBtab[nRGBtab][1]= baseRGB[j-1][1] + gstep*float(i);
      RGBtab[nRGBtab][2]= baseRGB[j-1][2] + bstep*float(i);
      nRGBtab++;
    }
    RGBtab[nRGBtab][0]= baseRGB[j][0];
    RGBtab[nRGBtab][1]= baseRGB[j][1];
    RGBtab[nRGBtab][2]= baseRGB[j][2];
    nRGBtab++;
  }
  int numrm=2;
  nRGBtab-=numrm;
  for (int j=0; j<nRGBtab; j++) {
    RGBtab[j][0]= RGBtab[j+numrm][0];
    RGBtab[j][1]= RGBtab[j+numrm][1];
    RGBtab[j][2]= RGBtab[j+numrm][2];
  }

#ifdef DEBUGPRINT1
  METLIBS_LOG_DEBUG("nRGBtab,mRGBtab: "<<nRGBtab<<" "<<mRGBtab);
  for (int i=0; i<nRGBtab; i++) {
    METLIBS_LOG_DEBUG(std::setw(3)<<i<<":  "
    <<std::setw(3)<<int(RGBtab[i][0]*255.+0.5)<<"  "
    <<std::setw(3)<<int(RGBtab[i][1]*255.+0.5)<<"  "
    <<std::setw(3)<<int(RGBtab[i][2]*255.+0.5));
  }
#endif

  for (int j=0; j<nRGBtab; j++) {
    std::string name= "tmp_contour_fill_" + miutil::from_number(j);
    int red=   int(RGBtab[j][0]*255);
    int green= int(RGBtab[j][1]*255);
    int blue=  int(RGBtab[j][2]*255);
    int alpha= 180;
    Colour::define(name,red,green,blue,alpha);
    csinfo.colour.push_back(Colour(name));
  }

  csinfo.name="standard";
  ColourShading::addColourShadingInfo(csinfo);
  ColourShading::define("standard",csinfo.colour);

  std::vector<std::string> list,tokens,stokens;
  std::string key,value,value2;

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  int n= list.size();
  for (int i=0; i<n; i++){
    miutil::SetupParser::splitKeyValue(list[i], key, stokens);
    int m=stokens.size();
    if ( m == 0 )
      continue;
    csinfo.colour.clear();
    for(int j=0;j<m;j++)
      csinfo.colour.push_back(Colour(stokens[j]));
    csinfo.name=key;
    ColourShading::addColourShadingInfo(csinfo);
    ColourShading::define(key,csinfo.colour);
  }
  return true;
}




// parse section containing fill pattern definitions
bool LocalSetupParser::parseFillPatterns(const std::string& sectname){

  std::vector<std::string> list;
  std::string key;
  int i,n;

  // Default pattern
  Pattern::PatternInfo pinfo;
  pinfo.name="def_pattern";
  pinfo.pattern.push_back("diag");
  pinfo.pattern.push_back("horizontal");
  pinfo.pattern.push_back("vertical");
  pinfo.pattern.push_back("dots");
  Pattern::addPatternInfo(pinfo);

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    miutil::SetupParser::splitKeyValue(list[i], key, pinfo.pattern);
    if ( pinfo.pattern.empty() )
      continue;
    pinfo.name=key;
    Pattern::addPatternInfo(pinfo);
  }
  return true;
}



// parse section containing linetype definitions
bool LocalSetupParser::parseLineTypes(const std::string& sectname)
{
  // linetype bits and bitmask

  // first define default types/values
  Linetype::init();

  std::vector<std::string> list;
  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  const unsigned int numbits= 16;
  std::vector<std::string> stokens;
  std::string key,value;

  for (const std::string& line : list) {
    miutil::SetupParser::splitKeyValue(line, key, value);
    if (value.empty())
      continue;
    unsigned short int bm = 0;
    int factor= 1;
    stokens= miutil::split(value, ":");
    if (stokens.size()>1){
      value = stokens[0];
      const std::string& value2 = stokens[1];
      if (miutil::is_int(value2))
        factor= atoi(value2.c_str());
    }
    if (value.length()==numbits){
      for (unsigned int j=0, m=(1<<(numbits-1)); j<numbits; j++, m >>= 1) {
        if (value[j]=='1')
          bm |= m;
      }
    } else { //0x00FF
      bm = strtol(value.c_str(),NULL,0);
    }
    Linetype::define(key,bm,factor);
  }

  return true;
}



// parse section containing definitions of quickmenus
bool LocalSetupParser::parseQuickMenus(const std::string& sectname){

  const std::string key_file= "file";

  std::vector<std::string> list,tokens,stokens;
  std::string key,value,file;
  QuickMenuDefs qmenu;
  int i,j,m,n;

  quickmenudefs.clear();

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    file= "";

    tokens= miutil::split(list[i], " ");
    m= tokens.size();
    for (j=0; j<m; j++){
      miutil::SetupParser::splitKeyValue(tokens[j],key,value);
      if (key==key_file && not value.empty()){
        file= value;
      }
    }
    if (not file.empty()){
      qmenu.filename= file;
      quickmenudefs.push_back(qmenu);
    } else {
      miutil::SetupParser::errorMsg(sectname,i,"Incomplete quickmenu-specification");
      return false;
    }
  }

  return true;
}


bool LocalSetupParser::getQuickMenus(std::vector<QuickMenuDefs>& qm)
{
  qm= quickmenudefs;
  return true;
}
