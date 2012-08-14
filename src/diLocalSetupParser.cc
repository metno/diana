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

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <list>

#include <diLocalSetupParser.h>
#include <diColourShading.h>
#include <diPattern.h>
#include <diImageGallery.h>
#include <puCtools/mkdir.h>
#include <puCtools/stat.h>


const miutil::miString SectColours=     "COLOURS";
const miutil::miString SectPalettes=    "PALETTES";
const miutil::miString SectFillPatterns="FILLPATTERNS";
const miutil::miString SectLineTypes=   "LINETYPES";
const miutil::miString SectQuickMenus=  "QUICKMENUS";
const miutil::miString SectBasics=      "BASIC";
const miutil::miString SectInfoFiles=   "TEXT_INFORMATION_FILES";

// static members
vector<QuickMenuDefs>      LocalSetupParser::quickmenudefs;
map<miutil::miString,miutil::miString>     LocalSetupParser::basic_values;
map<miutil::miString,InfoFile>     LocalSetupParser::infoFiles;
vector<miutil::miString>           LocalSetupParser::langPaths;


bool LocalSetupParser::makeDirectory(const miutil::miString& filename, miutil::miString & error)
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

bool LocalSetupParser::parse(miutil::miString & mainfilename){

  cerr << "LocalSetupParser::parse:" << mainfilename << endl;
  miutil::miString filename=mainfilename;

  //find $HOME, and make homedir
  miutil::miString homedir=getenv("HOME");
  homedir += "/.diana";
  miutil::miString error;
  if (makeDirectory(homedir,error)) {
    miutil::miString workdir = homedir + "/work";
    makeDirectory(workdir,error);
  } else {
    homedir=".";
  }
  basic_values["homedir"]    = homedir;

  //if no setupfile, use default
  if (!filename.exists()) {
    filename = "diana.setup";
    miutil::miString filename_str = filename;
    cerr << "filename:" << filename << endl;
    ifstream file(filename.cStr());
    if (!file) {
      filename = homedir + "/diana.setup";
      filename_str += " or ";
      filename_str += filename;
      cerr << "filename:" << filename << endl;
      ifstream file2(filename.cStr());
      if (!file2) {
        filename = "/etc/diana/" PVERSION "/diana.setup-COMMON";
        filename_str += " or ";
        filename_str += filename;
        cerr << "filename:" << filename << endl;
        ifstream file3(filename.cStr());
        if (!file3) {
          cerr << "LocalSetupParser::readSetup. cannot open default setupfile "
          << filename_str << endl;
          cerr << "Try diana-" PVERSION ".bin -s setupfile" << endl;
          return false;
        }
      }
    }
  }

  if (! miutil::SetupParser::parse( filename ) )
    return false;

  if (!parseBasics(SectBasics)) return false;
  if (!parseColours(SectColours)) return false;
  if (!parsePalettes(SectPalettes)) return false;
  if (!parseFillPatterns(SectFillPatterns)) return false;
  if (!parseLineTypes(SectLineTypes)) return false;
  if (!parseQuickMenus(SectQuickMenus)) return false;
  if (!parseTextInfoFiles(SectInfoFiles)) return false;

  // return the filename (for profet setup parser)
  mainfilename=filename;

  return true;
}



// parse basics-section
bool LocalSetupParser::parseBasics(const miutil::miString& sectname){

  const miutil::miString key_fontpath= "fontpath";
  const miutil::miString key_docpath=  "docpath";
  const miutil::miString key_obspath=  "obsplotfilepath";
  const miutil::miString key_qserver=  "qserver";
  const miutil::miString key_imagepath="imagepath";
  const miutil::miString key_langpaths="languagepaths";
  const miutil::miString key_language= "language";
  const miutil::miString key_setenv= "setenv";

  // default values
  miutil::miString langpaths="lang:/metno/local/translations:${QTDIR}/translations";
  miutil::miString language="en";
  basic_values[key_fontpath]   = "share/diana/" PVERSION "/fonts";
  basic_values[key_docpath]    = "share/doc/diana-" PVERSION;
  basic_values[key_obspath]    = "share/diana/" PVERSION;
  basic_values[key_qserver]    = "/usr/bin/coserver";
  basic_values[key_imagepath]  = "share/diana/" PVERSION "/images";
  basic_values[key_language]   = language;

  vector<miutil::miString> list,tokens;
  miutil::miString key,value;
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
      vector<miutil::miString> part = value.split(",");
      if(part.size()==3){
#ifdef __WIN32__
        //TODO: This is broken, disregards third argument (replace option)
	miutil::miString envst = part[0] + "=" + part[1];
	putenv(envst.cStr());
#else
        setenv(part[0].cStr(),part[1].cStr(),part[2].toInt());
#endif
      }
    }
  }

  // fix language paths
  //   checkEnvironment(langpaths);
  langPaths= langpaths.split(":");

  return true;
}


// parse text-information-files
bool LocalSetupParser::parseTextInfoFiles(const miutil::miString& sectname)
{
  infoFiles.clear();

  const miutil::miString key_name= "name";
  const miutil::miString key_file= "file";
  const miutil::miString key_type= "type";
  const miutil::miString key_font= "font";

  const miutil::miString def_type= "auto";
  const miutil::miString def_font= "auto";

  vector<miutil::miString> list,tokens,tokens2;
  miutil::miString key,value;
  miutil::miString name,filename, type, font;

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  int n= list.size();
  for (int i=0; i<n; i++){
    type= def_type; font= def_font;
    tokens2= list[i].split(' ');
    for (unsigned int j=0; j<tokens2.size(); j++){
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
    name.trim();
    filename.trim();
    type.trim();
    font.trim();
    if (name.exists() && filename.exists()){
      infoFiles[name].name= name;
      infoFiles[name].filename= filename;
      infoFiles[name].doctype= type;
      infoFiles[name].fonttype= font;
    }
  }

  return true;
}


// parse section containing colour definitions
bool LocalSetupParser::parseColours(const miutil::miString& sectname){
  // default colours
  const int numcols= 21;
  const miutil::miString colnames[numcols]=
  {"black","white","red","green","blue","yellow",
      "dark_green","dark_yellow","dark_red","dark_blue",
      "brown","orange","cyan","magenta",
      "purple","lightblue","dnmi_green","dnmi_blue",
      "grey25","grey50","grey90"};
  const uchar_t cols[numcols][3]=
  { {0,0,0},{255,255,255},{255,0,0},{0,255,0},{0,0,255},{255,255,0},
      {0,127,127},{178,178,0},{178,0,0},{0,0,178},
      {178,51,0},{255,89,0},{0,255,255},{255,0,255},
      {77,0,77},{51,51,255},{43,120,36},{0,54,125},
      {64,64,64},{127,127,127},{230,230,230}};


  vector<miutil::miString> list,tokens,stokens;
  miutil::miString key,value,value2;
  int i,n;
  Colour c;
  uchar_t r,g,b,a;
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
      r= atoi(stokens[0].cStr());
      g= atoi(stokens[1].cStr());
      b= atoi(stokens[2].cStr());
      if (stokens.size()>3) a= atoi(stokens[3].cStr());
      else a= 255;
      Colour::define(key,r,g,b,a);
      cinfo.rgb[0]= r;
      cinfo.rgb[1]= g;
      cinfo.rgb[2]= b;
      cinfo.name= key.downcase();
      Colour::addColourInfo(cinfo);
    }
  }
  return true;
}



// parse section containing colour-palette definitions
bool LocalSetupParser::parsePalettes(const miutil::miString& sectname){

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
  cerr<<"nRGBtab,mRGBtab: "<<nRGBtab<<" "<<mRGBtab<<endl;
  for (int i=0; i<nRGBtab; i++) {
    cerr<<setw(3)<<i<<":  "
    <<setw(3)<<int(RGBtab[i][0]*255.+0.5)<<"  "
    <<setw(3)<<int(RGBtab[i][1]*255.+0.5)<<"  "
    <<setw(3)<<int(RGBtab[i][2]*255.+0.5)<<endl;
  }
#endif

  for (int j=0; j<nRGBtab; j++) {
    miutil::miString name= "tmp_contour_fill_" + miutil::miString(j);
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

  vector<miutil::miString> list,tokens,stokens;
  miutil::miString key,value,value2;

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
bool LocalSetupParser::parseFillPatterns(const miutil::miString& sectname){

  vector<miutil::miString> list;
  miutil::miString key;
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
bool LocalSetupParser::parseLineTypes(const miutil::miString& sectname){

  // linetype bits and bitmask
  const unsigned int numbits= 16;
  const uint16 bmask[numbits]=
  {32768,16384,8192,4096,2048,1024,512,256,128,64,32,16,8,4,2,1};

  vector<miutil::miString> list,tokens,stokens;
  miutil::miString key,value,value2;
  int i,n;
  uint16 bm;
  int factor;

  // first define default types/values
  Linetype::init();

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    miutil::SetupParser::splitKeyValue(list[i], key, value);
    if ( !value.exists() )
      continue;
    bm= 0;
    factor= 1;
    stokens= value.split(':');
    if (stokens.size()>1){
      value = stokens[0];
      value2= stokens[1];
      if (value2.isInt())
        factor= atoi(value2.cStr());
    }
    if (value.length()==numbits){
      for (unsigned int j=0; j<numbits; j++){
        if (value[j]=='1') bm |= bmask[j];
      }
      Linetype::define(key,bm,factor);
    } else { //0x00FF
      bm = strtol(value.c_str(),NULL,0);
      Linetype::define(key,bm,factor);
    }
  }

  return true;
}



// parse section containing definitions of quickmenus
bool LocalSetupParser::parseQuickMenus(const miutil::miString& sectname){

  const miutil::miString key_file= "file";

  vector<miutil::miString> list,tokens,stokens;
  miutil::miString key,value,file;
  QuickMenuDefs qmenu;
  int i,j,m,n;

  quickmenudefs.clear();

  if (!miutil::SetupParser::getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    file= "";

    tokens= list[i].split(' ');
    m= tokens.size();
    for (j=0; j<m; j++){
      miutil::SetupParser::splitKeyValue(tokens[j],key,value);
      if (key==key_file && value.exists()){
        file= value;
      }
    }
    if (file.exists()){
      qmenu.filename= file;
      quickmenudefs.push_back(qmenu);
    } else {
      miutil::SetupParser::errorMsg(sectname,i,"Incomplete quickmenu-specification");
      return false;
    }
  }

  return true;
}


bool LocalSetupParser::getQuickMenus(vector<QuickMenuDefs>& qm)
{
  qm= quickmenudefs;
  return true;
}

vector< vector<Colour::ColourInfo> > LocalSetupParser::getMultiColourInfo(int multiNum)
{
  // Combine multiNum (1, 2 or 3) colours with most contrast

  vector< vector<Colour::ColourInfo> > vmc;

  vector<Colour::ColourInfo> colours = Colour::getColourInfo();
  int ncol= colours.size();

  if (multiNum==1) {

    // 1 is hardly multi, just returning all defined colours

    for (int i=0; i<ncol; i++) {
      vector<Colour::ColourInfo> mc;
      mc.push_back(colours[i]);
      vmc.push_back(mc);
    }

  } else if (multiNum==2) {

    // combinations of 2 colours, using the 'clearest' colours

    vector<int> cind;
    int i,j;
    bool ok;

    for (i=0; i<ncol; i++) {
      ok= true;
      for (j=0; j<3; j++)
        if (colours[i].rgb[j]!=0 && colours[i].rgb[j]!=255) ok= false;
      if (ok) cind.push_back(i);
    }

    int n1,n2,nc= cind.size();
    vector<Colour::ColourInfo> mc(2);

    for (n1=0; n1<nc; n1++) {
      for (n2=n1+1; n2<nc; n2++) {
        mc[0]= colours[cind[n1]];
        mc[1]= colours[cind[n2]];
        vmc.push_back(mc);
        mc[0]= colours[cind[n2]];
        mc[1]= colours[cind[n1]];
        vmc.push_back(mc);
      }
    }

  } else if (multiNum==3) {

    // combinations of 3 colours, using the 'clearest' colours

    vector<int> cind;
    int i,j;
    bool ok;

    for (i=0; i<ncol; i++) {
      ok= true;
      for (j=0; j<3; j++)
        if (colours[i].rgb[j]!=0 && colours[i].rgb[j]!=255) ok= false;
      if (ok) cind.push_back(i);
    }

    const int s[6][3]= { {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0} };
    int nn[3];
    int n1,n2,n3,nc= cind.size();
    vector<Colour::ColourInfo> mc(3);

    for (n1=0; n1<nc; n1++) {
      for (n2=n1+1; n2<nc; n2++) {
        for (n3=n2+1; n3<nc; n3++) {
          nn[0]= cind[n1];
          nn[1]= cind[n2];
          nn[2]= cind[n3];
          for (j=0; j<6; j++) {
            mc[0]= colours[nn[s[j][0]]];
            mc[1]= colours[nn[s[j][1]]];
            mc[2]= colours[nn[s[j][2]]];
            vmc.push_back(mc);
          }
        }
      }
    }

  }

  return vmc;
}


