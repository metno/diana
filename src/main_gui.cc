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
//
// Diana main program
//

#include <qapplication.h>
#include <qtranslator.h>

#include <diSetupParser.h>
#include <diPrintOptions.h>
#include <diController.h>
#include <qtMainWindow.h>
#include <puTools/miCommandLine.h>
#include <puTools/miString.h>
#include <iostream>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/level.h>
#else
#include <miLogger/logger.h>
#endif

#ifdef PROFET
#include <profet/ProfetController.h>
#endif

#include <diBuild.h>
#include <config.h>

using namespace std; using namespace miutil;


void printUsage(){

  cout << "----------------------------------------------------------" << endl
       << "Diana - a 2D presentation system for meteorological data, " <<endl
       << "including fields, observations, satellite- and radarimages, "<<endl
       << "vertical profiles and cross sections. Diana has tools for  " <<endl
       << "on-screen fieldediting and drawing of objects (fronts, areas, "<<endl
       << "symbols etc."<<endl
      << " Copyright (C) 2006 met.no" << endl
      << "----------------------------------------------------------" << endl
      << "Command line arguments:                                 "  << endl
      << "  -h            :  Show help  "                            <<  endl
      << "  -v            :  Version "                                << endl
      << "  -s <filnavn>  :  name of setupfile (def. diana.setup)   " << endl
      << "  -l <språk>    :  language                               " << endl
      << "  -L <logger>   :  loggerFile for debugging               " << endl
      << "  -p <profet>   :  profet test version                    " << endl
      << "  -S <server>   :  profet server host                     " << endl
      << "  -T <title>    :  Change Main window title "               << endl
       << "----------------------------------------------------------"<<endl;

}


int main(int argc, char **argv)
{
  cout << argv[0] << " : DIANA version: " << VERSION << "  build: "
      << build_string << endl;

  miString logfilename;
  miString ver_str= VERSION;
  miString build_str= build_string;
  miString cl_lang;
  bool profetEnabled= false;
  miString diana_title="Diana";
  miString profetServer;
  miString setupfile;
  miString lang;
  map<miString,miString> user_variables;

    // parsing command line arguments
  int ac= 1;
  while (ac < argc){
    miString sarg= argv[ac];

    if (sarg == "-h" || sarg == "--help"){
      printUsage();
      return 0;

    } else if (sarg=="-s" || sarg=="--setup") {
      ac++;
      if (ac >= argc) printUsage();
      setupfile= argv[ac];

    } else if (sarg=="-l" || sarg=="--lang") {
      ac++;
      if (ac >= argc) printUsage();
      cl_lang= argv[ac];

    } else if (sarg=="-L" || sarg=="--logger") {
      ac++;
      if (ac >= argc) printUsage();
      logfilename= argv[ac];

    } else if (sarg=="-v" || sarg=="--version") {
      //cerr << argv[0] << " : DIANA version: " << version_string << "  build: "<<build_string<<endl;
      return 0;

    } else if (sarg=="-p" || sarg=="--profet") {
      profetEnabled = true;

    } else if (sarg=="-S" || sarg=="--server") {
      ac++;
      if (ac >= argc) printUsage();
      profetServer= argv[ac];
    } else if (sarg=="-T" || sarg=="--title") {
        ac++;
        if (ac >= argc) printUsage();
       diana_title = miString(argv[ac]);

    } else {
      vector<miString> ks= sarg.split("=");
      if (ks.size()==2) {
        user_variables[ks[0].upcase()] = ks[1];
      } else {
        cerr << "WARNING, unknown argument on commandline:" << sarg << endl;
      }
    }
    ac++;
  } // command line parameters


  // Fix logger
  if(logfilename.exists()) {
#ifndef NOLOG4CXX
    log4cxx::PropertyConfigurator::configure(logfilename.cStr());
#endif
  }
  else {
#ifndef NOLOG4CXX
    log4cxx::BasicConfigurator::configure();
    //log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::WARN_INT);
    log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::getWarn());
#endif
  }

  SetupParser sp;
  sp.setUserVariables(user_variables);
  if (!sp.parse(setupfile)){
    cerr << "An error occured while reading setup: " << setupfile << endl;
    return 99;
  }
  printerManager printman;
  if (!printman.parseSetup(sp)){
    cerr << "An error occured while reading setup: "
	 << setupfile << endl;
    return 99;
  }

  Controller contr;

  // read setup
  if (!contr.parseSetup()){
    cerr << "An error occured while reading setup: " << setupfile << endl;
    return 99;
  }
#ifdef PROFET
  Profet::ProfetController::SETUP_FILE = setupfile;
  if (profetEnabled && profetServer.exists()) {
    Profet::ProfetController::SERVER_HOST = profetServer;
  }
#endif

  // language from setup
  if(sp.basicValue("language").exists())
    lang=sp.basicValue("language");

  // language from command line
  if(cl_lang.exists())
    lang=cl_lang;

  miTime x; x.setDefaultLanguage(lang);

  // gui init
  QApplication a( argc, argv );

  QTranslator qutil( 0 );
  QTranslator myapp( 0 );
  QTranslator qt( 0 );

  if(lang.exists()) {

    cout << "SYSTEM LANGUAGE: " << lang << endl;

    miString qtlang   = "qt_" +lang;
    miString dilang   = "diana_"+lang;
    miString qulang   = "qUtilities_"+lang;

    // translation files for application strings
    vector<miString> langpaths = sp.languagePaths();

    for(unsigned int i=0;i<langpaths.size(); i++ )
      if( qt.load(    qtlang.cStr(),langpaths[i].cStr()))
	break;

    for(unsigned int i=0;i<langpaths.size(); i++ )
      if( myapp.load( dilang.cStr(),langpaths[i].cStr()))
	break;

    for(unsigned int i=0;i<langpaths.size(); i++ )
      if( qutil.load( qulang.cStr(),langpaths[i].cStr()))
	break;

    a.installTranslator( &qt    );
    a.installTranslator( &myapp );
    a.installTranslator( &qutil );
  }

  DianaMainWindow * mw = new DianaMainWindow(&contr, ver_str,build_str,diana_title, profetEnabled);

  mw->start();

  a.setMainWidget(mw);
  mw->show();

  // news ?
  mw->checkNews();

  return a.exec();

}
