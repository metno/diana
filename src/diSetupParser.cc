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

#include <fstream>
#include <diSetupParser.h>
#include <diColourShading.h>
#include <diPattern.h>
#include <diImageGallery.h>
#include <list>

const miString SectColours=     "COLOURS";
const miString SectPalettes=    "PALETTES";
const miString SectFillPatterns="FILLPATTERNS";
const miString SectLineTypes=   "LINETYPES";
const miString SectQuickMenus=  "QUICKMENUS";
const miString SectBasics=      "BASIC";
const miString SectInfoFiles=   "TEXT_INFORMATION_FILES";

// static members
map<miString,miString>     SetupParser::substitutions;
vector<miString>           SetupParser::sfilename;
map<miString,SetupSection> SetupParser::sectionm;
map<miString,Filltype>     SetupParser::filltypes;
vector<QuickMenuDefs>      SetupParser::quickmenudefs;
map<miString,miString>     SetupParser::basic_values;
vector<InfoFile>           SetupParser::infoFiles;
vector<miString>           SetupParser::langPaths;


bool SetupParser::checkSubstitutions(miString& t)
{
  if(!t.contains("$("))
    return false;

  int start,stop;

  start = t.find("$(",0) + 2;
  stop  = t.find(")",start);

  if(stop < start ) {
    return false;
  }

  miString s = t.substr(start, stop-start);
  miString r = miString("$(") + s + ")";
  miString n;
  s = s.upcase();

  if( substitutions.count(s) > 0 )
    n = substitutions[s];

  t.replace(r,n);
  return true;
}


bool SetupParser::checkEnvironment(miString& t)
{
  if(!t.contains("${"))
    return false;

  int start,stop;

  start = t.find("${",0) + 2;
  stop  = t.find("}",start);

  if(stop < start ) {
    return false;
  }

  miString s = t.substr(start, stop-start);
  miString r = miString("${") + s + "}";

  s = s.upcase();

  miString n = getenv(s.cStr());

  t.replace(r,n);
  return true;
}



void SetupParser::cleanstr(miString& s){
  int p;
  if ((p=s.find("#"))!=string::npos)
    s.erase(p);

  // substitute environment/shell variables
  checkEnvironment(s);

  // substitute local/setupfile variables
  checkSubstitutions(s);

  s.remove('\n');
  s.trim();

  // prepare strings for easy split on '=' and ' '
  // : remove leading and trailing " " for each '='
  if (s.contains("=") && s.contains(" ")){
    p=0;
    while((p=s.find_first_of("=",p))!=string::npos){
      // check for "" - do not clean out blanks inside these
      vector<int> sf1,sf2;
      int f1=0,f2;
      while ((f1=s.find_first_of("\"",f1  ))!=string::npos &&
	     (f2=s.find_first_of("\"",f1+1))!=string::npos){
	sf1.push_back(f1);
	sf2.push_back(f2);
	f1=f2+1;
      }
      bool dropit=false;
      for (int i=0; i<sf1.size(); i++){
	f1= sf1[i];
	f2= sf2[i];
	if ( f1 > p ){
	  // no chance of "" pairs around this..
	  break;
	}
	if ( f1 < p && f2 > p ){
	  p=f2+1;
	  // '=' is inside a "" pair, drop cleaning
	  dropit=true;
	  break;
	}
      }
      if (dropit) continue;

      while (p>0 && s[p-1]==' '){
	s.erase(p-1,1);
	p--;
      }
      while (p<s.length()-1 && s[p+1]==' '){
	s.erase(p+1,1);
      }
      p++;
    }
  }

}

void SetupParser::splitKeyValue(const miString& s,
				miString& key, miString& value) {
  vector<miString> vs= s.split(2,'=',true);
  if (vs.size()==2) {
    key=vs[0].downcase(); // always converting keyword to lowercase !
    value=vs[1];
    // structures of type: A=B || C means A=B for existing B, otherwise C
    if ( value.contains("||") ){
      int j = value.find("||");
      miString a1 = value.substr(0,j);
      miString a2 = value.substr(j+2,value.length()-j);
      a1.trim(); a2.trim();
      value = ( a1.length()>0 ? a1 : a2 );
    }
    // remove "" from value
    if (value[0]=='"' && value[value.length()-1]=='"')
      value= value.substr(1,value.length()-2);
  } else if (vs.size()>2){
    key=vs[0].downcase(); // always converting keyword to lowercase !
    int n=vs.size();
    value.clear();
    for (int i=1;i<n;i++){
      value+=vs[i];
      if (i<n-1) value+="=";
    }
    if (value[0]=='"' && value[value.length()-1]=='"')
      value= value.substr(1,value.length()-2);
  } else{
    key= s.downcase(); // assuming pure keyword (without value)
    value= "";
  }
}


void SetupParser::splitKeyValue(const miString& s,
				miString& key, vector<miString>& value) {
  value.clear();
  vector<miString> vs= s.split(2,'=',true);
  if (vs.size()==2) {
    key=vs[0].downcase(); // always converting keyword to lowercase !
    vector<miString> vv= vs[1].split(',',true);
    int n= vv.size();
    for (int i=0; i<n; i++) {
      if (vv[i][0]=='"' && vv[i][vv[i].length()-1]=='"')
        value.push_back(vv[i].substr(1,vv[i].length()-2));
      else
        value.push_back(vv[i]);
    }
  } else {
    key= s.downcase(); // assuming pure keyword (without value)
  }
}



// parse one setupfile
bool SetupParser::parseFile(const miString& filename, // name of file
			    const miString& section,  // inherited section
			    int level)                // recursive level
{
  // list of filenames, index to them
  sfilename.push_back(filename);
  int activefile= sfilename.size()-1;

  // ====== just output
  level++;
  miString dummy= " ";
  for ( int i=0; i<=level; i++)
    dummy += ".";
  cerr << dummy << " reading \t[" << filename << "] " << endl;
  // ===================

  const miString undefsect= "_UNDEF_";
  miString origsect = ( section.exists() ? section : undefsect );
  miString sectname = origsect;
  list<miString> sectstack;

  miString str;
  int n,ln=0,linenum;

  // open filestream
  ifstream file(filename.cStr());
  if (!file){
    cerr << "SetupParser::readSetup. cannot open setupfile " <<
      filename << endl;
    return false;
  }

  /*
    - skip blank lines,
    - strip lines for comments and left/right whitespace
    - merge lines ending with \
    - accumulate strings for each section
  */
  miString tmpstr;
  int tmpln;
  bool merge= false, newmerge;

  while (getline(file,str)){
    ln++;
    str.trim();
    n= str.length();
    if (n == 0) continue;

    /*
      check for linemerging
    */
    newmerge= false;
    if (str[n-1] == '\\'){
      newmerge= true;
      str= str.substr(0,str.length()-1);
    }
    if (merge){           // this is at least the second merge-line..
      tmpstr+= str;
      if (newmerge) continue; // and there is more, go to next line
      str    = tmpstr;        // We are finished: go to checkout
      linenum= tmpln;
      merge= false;

    } else if (newmerge){ // This is the start of a merge
      tmpln= ln;
      tmpstr= str;
      merge=true;
      continue;             // go to next line

    } else {              // no merge at all
      linenum = ln;
    }

    /*
      Remove preceding and trailing blanks.
      Remove comments
      Remove blanks around '='
      Do variable substitutions
    */
    cleanstr(str);
    n= str.length();
    if ( n == 0 )
      continue;

    /*
      Check each line..
    */
    if (n>1 && str[0]=='<' && str[n-1]=='>'){
      // start or end of section
      if (str[1]=='/'){ // end of current section
	if ( sectstack.size() > 0 ){
	  // retreat to previous section
	  sectname = sectstack.back();
	  sectstack.pop_back();
	} else
	  sectname= undefsect;

      } else { // start of new section
	// push current section onto stack
	sectstack.push_back(sectname);
	sectname= str.substr(1,n-2);
      }

    } else if ( str.substr(0,8) == "%include" ){
      /*
	include another setupfile
      */
      if ( n < 10 ){
	miString error = "Missing filename for include";
	internalErrorMsg(filename,linenum,error);
	return false;
      }
      miString nextfile = str.substr(8,n);
      nextfile.trim();
      if ( !parseFile( nextfile,sectname,level ) )
	return false;

    } else if ( str.upcase() == "CLEAR" ){
      /*
	Clear all strings for this section
	Only valid inside a section
      */
      if ( sectname == undefsect ){
	miString error = "CLEAR only valid within a section";
	internalErrorMsg(filename,linenum,error);
	continue;
      }
      if ( sectionm.count(sectname) == 0 )
	continue;
      sectionm[sectname].strlist.clear();
      sectionm[sectname].linenum.clear();
      sectionm[sectname].filenum.clear();
//       // also add clear command to section
//       sectionm[sectname].strlist.push_back(str);
//       sectionm[sectname].linenum.push_back(linenum);
//       sectionm[sectname].filenum.push_back(activefile);

    } else {
      /*
	Add string to section.
	If undefined section, check instead for variable declaration
      */
      if ( sectname == undefsect ){
	miString key, value;
	splitKeyValue( str, key, value );
	if ( value.exists() ){
	  substitutions[ key.upcase() ] = value;
	} else {
	  cerr << "** setupfile WARNING, line " << linenum
	       << " in file " << filename << " is no variabledefinition, and is outside all sections:"
	       << str << endl;
	}
	continue;
      }
      // add strings to appropriate section
      sectionm[sectname].strlist.push_back(str);
      sectionm[sectname].linenum.push_back(linenum);
      sectionm[sectname].filenum.push_back(activefile);
    }
  }

  file.close();

  // File should start and end in same section
  if ( sectname != origsect ){
    miString error = "File started in section " + origsect
      + " and ended in section " + sectname;
    internalErrorMsg(filename,linenum,error);
    return false;
  }
  return true;
}


bool SetupParser::parse(const miString& mainfilename){

  sfilename.clear();

  if (! parseFile( mainfilename, "", -1 ) )
    return false;

  if (!parseBasics(SectBasics)) return false;
  if (!parseColours(SectColours)) return false;
  if (!parsePalettes(SectPalettes)) return false;
  if (!parseFillPatterns(SectFillPatterns)) return false;
  if (!parseLineTypes(SectLineTypes)) return false;
  if (!parseQuickMenus(SectQuickMenus)) return false;
  if (!parseTextInfoFiles(SectInfoFiles)) return false;

  return true;
}


// report an error with filename and linenumber
void SetupParser::internalErrorMsg(const miString& filename,
				   const int linenum,
				   const miString& error)
{
  cerr << "================================================" << endl;
  cerr << "Error in setupfile " << filename << endl
       << "The error occured in line " << linenum << endl
       << "Message: " << error << endl;
  cerr << "================================================" << endl;
}

// report an error with line# and sectionname
void SetupParser::errorMsg(const miString& sectname,
			   const int linenum,
			   const miString& error)
{
  map<miString,SetupSection>::iterator p;
  if ((p=sectionm.find(sectname))!=sectionm.end()){
    int n= p->second.linenum.size();
    int lnum= ( linenum >= 0 && linenum <n ) ?
      p->second.linenum[linenum] : 9999;
    int m= p->second.filenum.size();
    int fnum= ( linenum >= 0 && linenum <m ) ?
      p->second.filenum[linenum] : 0;

    cerr << "================================================" << endl;
    cerr << "Error in setupfile " << sfilename[fnum] << endl
	 << "The error occured in section " << sectname << ", line " << lnum << endl
	 << "Line   : " << p->second.strlist[linenum] << endl
	 << "Message: " << error << endl;
    cerr << "================================================" << endl;
  } else {
    cerr << "================================================" << endl;
    cerr << "Internal SetupParser error." << endl
	 << "An error was reported for unknown section " << sectname << endl
	 << "The message is: " << error << endl;
    cerr << "================================================" << endl;
  }
}

// give a warning with line# and sectionname
void SetupParser::warningMsg(const miString& sectname,
			     const int linenum,
			     const miString& warning)
{
  map<miString,SetupSection>::iterator p;
  if ((p=sectionm.find(sectname))!=sectionm.end()){
    int n= p->second.linenum.size();
    int lnum= ( linenum >= 0 && linenum <n ) ?
      p->second.linenum[linenum] : 9999;
    int m= p->second.filenum.size();
    int fnum= ( linenum >= 0 && linenum <m ) ?
      p->second.filenum[linenum] : 0;

    cerr << "================================================" << endl;
    cerr << "Warning for setupfile " << sfilename[fnum] << endl
	 << "Section " << sectname << ", line " << lnum << endl
	 << "Line   : " << p->second.strlist[linenum] << endl
	 << "Message: " << warning << endl;
    cerr << "================================================" << endl;
  } else {
    cerr << "================================================" << endl;
    cerr << "Internal SetupParser warning." << endl
	 << "A warning was reported for unknown section " << sectname << endl
	 << "The message is: " << warning << endl;
    cerr << "================================================" << endl;
  }
}

bool SetupParser::getSection(const miString& sectname,
			     vector<miString>& setuplines)
{
  map<miString,SetupSection>::iterator p;
  if ((p=sectionm.find(sectname))!=sectionm.end()){
    setuplines= p->second.strlist;
    return true;
  }
#ifdef DEBUGPRINT1
  cerr << "++SetupParser::getSection for unknown section: " <<
    sectname << endl;
#endif
  setuplines.clear();
  return false;
}

// parse basics-section
bool SetupParser::parseBasics(const miString& sectname){

  const miString key_fontpath= "fontpath";
  const miString key_docpath=  "docpath";
  const miString key_obspath=  "obsplotfilepath";
  const miString key_qserver=  "qserver";
  const miString key_imagepath="imagepath";
  const miString key_langpaths="languagepaths";
  const miString key_language= "language";

  // default values
  miString langpaths="lang:/metno/local/translations:${QTDIR}/translations";
  miString language="no";
  basic_values[key_fontpath]   = "../fonts";
  basic_values[key_docpath]    = "../doc";
  basic_values[key_obspath]    = "../etc";
  basic_values[key_qserver]    = "/metno/local/bin/coserver";
  basic_values[key_imagepath]  = "../images";
  basic_values[key_language]   = language;

  vector<miString> list,tokens;
  miString key,value;
  int i,m,n;

  if (!getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    splitKeyValue(list[i],key,value);

    // everything into basic_values map
    basic_values[key] = value;

    if (key==key_langpaths)
      langpaths= value;
  }

  // fix language paths
  //   checkEnvironment(langpaths);
  langPaths= langpaths.split(":");

  return true;
}


// parse text-information-files
bool SetupParser::parseTextInfoFiles(const miString& sectname)
{
  infoFiles.clear();

  const miString key_name= "name";
  const miString key_file= "file";
  const miString key_type= "type";
  const miString key_font= "font";

  const miString def_type= "auto";
  const miString def_font= "auto";

  vector<miString> list,tokens,tokens2;
  miString key,value;
  miString name,filename, type, font;
  int m,n, numfiles=0;

  if (!getSection(sectname,list))
    return true;

  n= list.size();
  for (int i=0; i<n; i++){
    type= def_type; font= def_font;
    tokens2= list[i].split(' ');
    for (int j=0; j<tokens2.size(); j++){
      splitKeyValue(tokens2[j], key, value);
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
      infoFiles.push_back(InfoFile());
      infoFiles[numfiles].name= name;
      infoFiles[numfiles].filename= filename;
      infoFiles[numfiles].doctype= type;
      infoFiles[numfiles].fonttype= font;
      numfiles++;
    }
  }

  return true;
}


// parse section containing colour definitions
bool SetupParser::parseColours(const miString& sectname){
  // default colours
  const int numcols= 21;
  const miString colnames[numcols]=
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


  vector<miString> list,tokens,stokens;
  miString key,value,value2;
  int i,j,n,m;
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

  if (!getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    splitKeyValue(list[i], key, stokens);
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
bool SetupParser::parsePalettes(const miString& sectname){

  // first define default types/values
  ColourShading::ColourShadingInfo csinfo;
  const int nbaseRGB=5;
  const float baseRGB[nbaseRGB][3]=
    { 0.0,0.0,0.5, 0.0,1.0,1.0, 0.0,0.5,0.0,
      1.0,1.0,0.0, 1.0,0.0,0.0 };

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

  int ncolours= nRGBtab;
  for (int j=0; j<nRGBtab; j++) {
    miString name= "tmp_contour_fill_" + miString(j);
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

  vector<miString> list,tokens,stokens;
  miString key,value,value2;

  if (!getSection(sectname,list))
    return true;

  int n= list.size();
  for (int i=0; i<n; i++){
    splitKeyValue(list[i], key, stokens);
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
bool SetupParser::parseFillPatterns(const miString& sectname){

  vector<miString> list;
  miString key;
  int i,n;

  // Default pattern
  Pattern::PatternInfo pinfo;
  pinfo.name="def_pattern";
  pinfo.pattern.push_back("diag");
  pinfo.pattern.push_back("horizontal");
  pinfo.pattern.push_back("vertical");
  pinfo.pattern.push_back("dots");
  Pattern::addPatternInfo(pinfo);

  if (!getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    splitKeyValue(list[i], key, pinfo.pattern);
    if ( pinfo.pattern.empty() )
      continue;
    pinfo.name=key;
    Pattern::addPatternInfo(pinfo);
  }
  return true;
}



// parse section containing linetype definitions
bool SetupParser::parseLineTypes(const miString& sectname){

  // linetype bits and bitmask
  const int numbits= 16;
  const uint16 bmask[numbits]=
  {32768,16384,8192,4096,2048,1024,512,256,128,64,32,16,8,4,2,1};

  vector<miString> list,tokens,stokens;
  miString key,value,value2;
  int i,j,n,m;
  uint16 bm;
  int factor;

  // first define default types/values
  Linetype::init();

  if (!getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    splitKeyValue(list[i], key, value);
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
      for (j=0; j<numbits; j++){
	if (value[j]=='1') bm |= bmask[j];
      }
      Linetype::define(key,bm,factor);
    }
  }
  return true;
}



// parse section containing definitions of quickmenus
bool SetupParser::parseQuickMenus(const miString& sectname){

  const miString key_file= "file";

  vector<miString> list,tokens,stokens;
  miString key,value,file;
  QuickMenuDefs qmenu;
  int i,j,m,n,o;

  quickmenudefs.clear();

  if (!getSection(sectname,list))
    return true;

  n= list.size();
  for (i=0; i<n; i++){
    file= "";

    tokens= list[i].split(' ');
    m= tokens.size();
    for (j=0; j<m; j++){
      splitKeyValue(tokens[j],key,value);
      if (key==key_file && value.exists()){
	file= value;
      }
    }
    if (file.exists()){
      qmenu.filename= file;
      quickmenudefs.push_back(qmenu);
    } else {
      errorMsg(sectname,i,"Incomplete quickmenu-specification");
      return false;
    }
  }

  return true;
}


bool SetupParser::getQuickMenus(vector<QuickMenuDefs>& qm)
{
  qm= quickmenudefs;
  return true;
}

vector< vector<Colour::ColourInfo> > SetupParser::getMultiColourInfo(int multiNum)
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

    const int s[6][3]= { 0,1,2, 0,2,1, 1,0,2, 1,2,0, 2,0,1, 2,1,0 };
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
