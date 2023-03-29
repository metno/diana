/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diPrintOptions.h"
#include "diLocalSetupParser.h"
#include "miSetupParser.h"

#include <puTools/miStringFunctions.h>

#include <iostream>
#include <fstream>

#define MILOGGER_CATEGORY "diana.PrintOptions"
#include <miLogger/miLogging.h>

using namespace d_print;
using namespace::miutil;

std::vector<printerManager::printerExtra> printerManager::printers;
std::map<std::string, d_print::PageSize> printerManager::pages;
std::map<d_print::PageSize, d_print::PaperSize> printerManager::pagesizes;
std::string printerManager::pcommand;

/*
  MANUAL POSTSCRIPT-COMMANDS
  -----------------------------------------------
  Syntax for printer definition file:
  -----------------------------------------------
  # comments starts with '#'
  # -- New Printer --
  printer=fou3, pagesize=A3
  COMMENT={{
  << commands to insert in comment-section >>
  }}
  PROLOG={{
  << commands to insert at start of prolog-section >>
  }}
  PAGE={{
  << commands to insert at start of each page >>
  }}
  TRAILER={{
  << commands to insert before %%EOF >>
  }}
  # -- New Printer --
  printer=farge
  PROLOG={{
  << .... >>
  }}
  -----------------------------------------------
  OBS: Write more specialized keys first!
  With:
     printer=fou3
     PROLOG={{
     ...........
     }}
     #
     printer=fou3, pagesize=A4
     PROLOG={{
     ...........
     }}
  the first entry will always be used for printer fou3!!!
*/
void printOptions::printPrintOptions()
{
  std::cerr << "printOptions: " << std::endl;
  std::cerr << fname << " name of output file" << std::endl;
  std::cerr << printer << " name of printer" << std::endl;
  std::cerr << orientation << " paper-orientation" << std::endl;
  std::cerr << colop << " use of colour" << std::endl;
  std::cerr << pagesize << " pagesize in standard notation" << std::endl;
  std::cerr << papersize.hsize << ", " << papersize.vsize << " size of paper in mm" << std::endl;
  std::cerr << usecustomsize << " use papersize instead of pagesize" << std::endl;
}

printerManager::printerManager()
{
}

//static
void printerManager::initialize()
{
  if (pages.size()==0){
    pages["A4"]= A4;
    pages["B5"]= B5;
    pages["LETTER"]= Letter;
    pages["LEGAL"]= Legal;
    pages["EXECUTIVE"]= Executive;
    pages["A0"]= A0;
    pages["A1"]= A1;
    pages["A2"]= A2;
    pages["A3"]= A3;
    pages["A5"]= A5;
    pages["A6"]= A6;
    pages["A7"]= A7;
    pages["A8"]= A8;
    pages["A9"]= A9;
    pages["B0"]= B0;
    pages["B1"]= B1;
    pages["B10"]= B10;
    pages["B2"]= B2;
    pages["B3"]= B3;
    pages["B4"]= B4;
    pages["B6"]= B6;
    pages["B7"]= B7;
    pages["B8"]= B8;
    pages["B9"]= B9;
    pages["C5E"]= C5E;
    pages["COMM10E"]=Comm10E;
    pages["DLE"]= DLE;
    pages["FOLIO"]= Folio;
    pages["LEDGER"]= Ledger;
    pages["TABLOID"]= Tabloid;
    pages["NPAGESIZE"]= NPageSize;
  }

  if (pagesizes.size()==0){
    pagesizes[A4]=       PaperSize(210,297);
    pagesizes[B5]=       PaperSize(182,257);
    pagesizes[Letter]=   PaperSize(216,279);
    pagesizes[Legal]=    PaperSize(216,356);
    pagesizes[Executive]=PaperSize(191,254);
    pagesizes[A0]=       PaperSize(841,1189);
    pagesizes[A1]=       PaperSize(594,841);
    pagesizes[A2]=       PaperSize(420,594);
    pagesizes[A3]=       PaperSize(297,420);
    pagesizes[A5]=       PaperSize(148,210);
    pagesizes[A6]=       PaperSize(105,148);
    pagesizes[A7]=       PaperSize(74,105);
    pagesizes[A8]=       PaperSize(52,74);
    pagesizes[A9]=       PaperSize(37,52);
    pagesizes[B0]=       PaperSize(1030,1456);
    pagesizes[B1]=       PaperSize(728,1030);
    pagesizes[B10]=      PaperSize(32,45 );
    pagesizes[B2]=       PaperSize(515,728 );
    pagesizes[B3]=       PaperSize(364,515);
    pagesizes[B4]=       PaperSize(257,364 );
    pagesizes[B6]=       PaperSize(128,182);
    pagesizes[B7]=       PaperSize(91,128);
    pagesizes[B8]=       PaperSize(64,91);
    pagesizes[B9]=       PaperSize(45,64);
    pagesizes[C5E]=      PaperSize(163,229);
    pagesizes[Comm10E]=  PaperSize(105,241);
    pagesizes[DLE]=      PaperSize(110,220);
    pagesizes[Folio]=    PaperSize(210,330);
    pagesizes[Ledger]=   PaperSize(432,279);
    pagesizes[Tabloid]=  PaperSize(279,432 );
    pagesizes[NPageSize]=PaperSize(1,1); // OBS
  }
}

// expand variables in pcommand
bool printerManager::expandCommand(std::string& com, const printOptions& po)
{
  if (com.find("{numcopies}") != std::string::npos)
    METLIBS_LOG_WARN("print command with {numcopies}, which is always 1: '" << com << "'");

  miutil::replace(com, "{printer}", po.printer);
  miutil::replace(com, "{filename}", po.fname);

  miutil::replace(com, "{hash}", "#"); // setupParser is not fond of #'s
  miutil::replace(com, "{numcopies}", "1");

  return true;
}


bool printerManager::readPrinterInfo(const std::string fname)
{
  //   METLIBS_LOG_DEBUG("Reading printerdef from:" << fname);
  printers.clear();

  if (fname.empty())
    return false;

  // open filestream
  std::ifstream file(fname);
  if (!file) {
    METLIBS_LOG_ERROR("Cannot open printer definition file '" << fname << "'");
    return false;
  }

  int j=-1;
  std::string s;
  std::string comkey, com;
  std::vector<std::string> vs, vvs;
  bool incom= false;

  while (getline(file,s)){
    miutil::trim(s);
    if (s.empty() || s[0]=='#')
      continue;

    if (incom) { // reading command-lines
      if (miutil::contains(s, "}}")){ // end of command
	if (j>=0) printers[j].commands[comkey]= com;
	incom= false;
	continue;
      }
      com += (s + "\n");

    } else {
      if (miutil::contains(s, "{{")){ // start of command
	vs= miutil::split(s, "=");
	if (vs.size()>1){
	  comkey= miutil::to_upper(vs[0]);
	  com= "";
	  incom= true;
	}
      } else if (miutil::contains(s, "=")){ // start new printer
	printers.push_back(printerExtra());
	j++;
	vs= miutil::split(s, ",");
	int n= vs.size();
	for (int i=0; i<n; i++){
	  if (miutil::contains(vs[i], "=")){
	    vvs= miutil::split(vs[i], "=");
	    if (vvs.size() > 1)
	      printers[j].keys[miutil::to_upper(vvs[0])]= vvs[1];
	  }
	}
      }
    }
  }

  return true;
}

PageSize  printerManager::getPage(const std::string s) // page from string
{
  PageSize ps = A4;

  std::string us= miutil::to_upper(s);
  miutil::trim(us);

  initialize();
  if (pages.count(us)>0)
    ps= pages[us];

  return ps;
}

PaperSize printerManager::getSize(const PageSize ps)// size from page
{
  PaperSize prs;

  initialize();
  if (pagesizes.count(ps)>0)
    prs= pagesizes[ps];

  return prs;
}

bool printerManager::checkSpecial(const printOptions& po, std::map<std::string, std::string>& mc)
{
  if (!printers.size()) return false;
  mc.clear();

  for (unsigned int i=0; i<printers.size(); i++){
    if (printers[i].keys["PRINTER"]==po.printer){
      // check rest of the keys
      if (printers[i].keys.count("PAGESIZE")>0)
	if (getPage(printers[i].keys["PAGESIZE"])!=po.pagesize)
	  continue;

      // we have a match
      mc= printers[i].commands;
      return true;
    }
  }

  return false;
}


bool printerManager::parseSetup() {

  std::string section="PRINTING";
  std::vector<std::string> vstr;

  const std::string key_command=   "printcommand";
  const std::string key_manualcom= "manualcommands";
  // default key-values
  pcommand= "lp -c -d {printer} {filename}";
  std::string printerfile= "";

  if (!SetupParser::getSection(section,vstr)){
    METLIBS_LOG_WARN("No " << section << " section in setupfile, ok.");
    return true;
  }

  std::string key,value,error;
  int i,n,nv,nvstr=vstr.size();

  for (nv=0; nv<nvstr; nv++) {
    std::vector<std::string> tokens = miutil::split(vstr[nv], ",", true);
    n= tokens.size();
    std::string name;

    for (i=0; i<n; i++) {
      SetupParser::splitKeyValue(tokens[i],key,value);
      if (key==key_command)
	pcommand= value;
      else if (key==key_manualcom)
	printerfile= value;
    }
  }

  if (not printerfile.empty())
    readPrinterInfo(printerfile);
  return true;
}

