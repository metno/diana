/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2013 met.no

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <QApplication>
#include <QTranslator>
#include <QMessageBox>

// required to tell fimex to use log4cpp
#include <fimex/Logger.h>

#include "diLocalSetupParser.h"
#include "diPrintOptions.h"
#include "diController.h"
#include "diEditItemManager.h"

#include "qtMainWindow.h"

#include <puTools/miSetupParser.h>
#include <iostream>

#include <diBuild.h>

#define MILOGGER_CATEGORY "diana.main_gui"
#include <miLogger/miLogging.h>

using namespace std;
using namespace miutil;

void printUsage()
{
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
      << "  -s <filename> :  name of setupfile (def. diana.setup)   " << endl
      << "  -l <language> :  language used in dialogs                               " << endl
      << "  -L <logger>   :  loggerFile for debugging               " << endl
      << "  -T <title>    :  Change Main window title "               << endl
       << "----------------------------------------------------------"<<endl;
}

int main(int argc, char **argv)
{
  cout << argv[0] << " : DIANA version: " << VERSION << "  build: "
      << build_string << endl;

#if defined(Q_WS_QWS)
  QApplication a(argc, argv, QApplication::GuiServer);
#else
  #if defined(USE_PAINTGL) // either QPA or X11 without OpenGL
    QApplication a(argc, argv);
  #else
  #if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
    QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
  #endif
    QApplication a( argc, argv );
  #endif
#endif

  string logfilename;
  string ver_str= VERSION;
  string build_str= build_string;
  string cl_lang;
  string diana_title="diana";
  string setupfile;
  string lang;
  map<std::string, std::string> user_variables;

  user_variables["PVERSION"]= PVERSION;
  user_variables["SYSCONFDIR"]= SYSCONFDIR;
    // parsing command line arguments
  int ac= 1;
  while (ac < argc){
    const string sarg = argv[ac];

    if (sarg == "-h" || sarg == "--help"){
      printUsage();
      return 0;

    } else if (sarg=="-s" || sarg=="--setup") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 99;
      }
      setupfile= argv[ac];

    } else if (sarg=="-l" || sarg=="--lang") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 99;
      }
      cl_lang= argv[ac];

    } else if (sarg=="-L" || sarg=="--logger") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 99;
      }
      logfilename= argv[ac];

    } else if (sarg=="-v" || sarg=="--version") {
      //METLIBS_LOG_DEBUG(argv[0] << " : DIANA version: " << version_string << "  build: "<<build_string);
      return 0;

    } else if (sarg=="-T" || sarg=="--title") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 99;
      }
      diana_title = argv[ac];

    } else {
      vector<string> ks= miutil::split(sarg, "=");
      if (ks.size()==2) {
        user_variables[miutil::to_upper(ks[0])] = ks[1];
      } else {
        METLIBS_LOG_WARN("WARNING, unknown argument on commandline:" << sarg);
      }
    }
    ac++;
  } // command line parameters

  if ( logfilename.empty() ){
    logfilename = "/etc/diana/";
    logfilename += PVERSION;
    logfilename += "/log4cpp.properties";
  }

  // tell fimex to use log4cpp
  MetNoFimex::Logger::setClass(MetNoFimex::Logger::LOG4CPP);
  milogger::LoggingConfig log4cpp(logfilename);

  SetupParser::setUserVariables(user_variables);
  if (!LocalSetupParser::parse(setupfile)){
    if (setupfile.empty()) {
      METLIBS_LOG_ERROR("No setup file specified.");
      QMessageBox::critical(0, QString("Diana %1").arg(VERSION),
        QString("No setup file specified."));
    } else {
      METLIBS_LOG_ERROR("An error occurred while reading setup: " << setupfile);
      QMessageBox::critical(0, QString("Diana %1").arg(VERSION),
        QString("An error occurred while reading setup: %1").arg(QString::fromStdString(setupfile)));
    }
    return 99;
  }
  printerManager printman;
  if (!printman.parseSetup()) {
    METLIBS_LOG_ERROR("An error occurred while reading print setup: " << setupfile);
    QMessageBox::critical(0, QString("Diana %1").arg(VERSION),
      QString("An error occurred while reading print setup: %1").arg(QString::fromStdString(setupfile)));
    return 99;
  }

  Controller contr;
  contr.addManager("EDITDRAWING", EditItemManager::instance());

  // read setup
  if (!contr.parseSetup()){
    METLIBS_LOG_ERROR("An error occurred while reading setup: " << setupfile);
    QMessageBox::critical(0, QString("Diana %1").arg(VERSION),
      QString("An error occurred while reading setup: %1").arg(QString::fromStdString(setupfile)));
    return 99;
  }

  // language from setup
  if (not LocalSetupParser::basicValue("language").empty())
    lang = LocalSetupParser::basicValue("language");

  // language from command line
  if (not cl_lang.empty())
    lang = cl_lang;

  { miTime x; x.setDefaultLanguage(lang.c_str()); }

  QTranslator qutil( 0 );
  QTranslator myapp( 0 );
  QTranslator qt( 0 );

  if (not lang.empty()) {

    METLIBS_LOG_INFO("SYSTEM LANGUAGE: " << lang);

    string qtlang   = "qt_" +lang;
    string dilang   = "diana_"+lang;
    string qulang   = "qUtilities_"+lang;

    // translation files for application strings
    vector<string> langpaths = LocalSetupParser::languagePaths();

    for(unsigned int i=0;i<langpaths.size(); i++ )
      if( qt.load(    qtlang.c_str(),langpaths[i].c_str()))
	break;

    for(unsigned int i=0;i<langpaths.size(); i++ )
      if( myapp.load( dilang.c_str(),langpaths[i].c_str()))
	break;

    for(unsigned int i=0;i<langpaths.size(); i++ )
      if( qutil.load( qulang.c_str(),langpaths[i].c_str()))
	break;

    a.installTranslator( &qt    );
    a.installTranslator( &myapp );
    a.installTranslator( &qutil );
  }

  DianaMainWindow * mw = new DianaMainWindow(&contr, ver_str,build_str,diana_title);
  mw->start();

//  a.setMainWidget(mw);
#if defined(Q_WS_QWS)
  mw->showFullScreen();
#else
  mw->show();
  mw->resize(mw->width()-1, mw->height()-1);
  mw->resize(mw->width()+1, mw->height()+1);
#endif

  // news ?
  mw->checkNews();

  return a.exec();
}
