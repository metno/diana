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
#include <miCommandLine.h>
#include <miString.h>
#include <iostream>

#ifndef NOLOG4CXX
#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/level.h>
#else
#include <miLogger/logger.h>
#endif

#include <diVersion.h>

using namespace std;

const miString helpstr="\
----------------------------------------------------------\n\
Meteorologisk visningsverktøy, produksjonssystem for digital analyse \n\
FoU/met.no, første versjon 1999        \n\
----------------------------------------------------------\n\
Kommandolinje-argumenter:                                 \n\
  -h            :  skriver ut denne teksten og avslutter  \n\
  -s <filnavn>  :  navn på setupfil (def. diana.setup)    \n\
  -l <språk>    :  systemspråk (def. no)                  \n\
  -L <logger>   :  loggerFil for debugging                \n\
  -p <profet>   :  profet test version                    \n\
----------------------------------------------------------\n\
";

int main(int argc, char **argv)
{
  cerr << argv[0] << " : DIANA version " << version_string << endl;
  // parsing commandline-arguments

  vector<miCommandLine::option> opt;

  opt.push_back(miCommandLine::option('s',"setup",  1 ));
  opt.push_back(miCommandLine::option('h',"help",   0 ));
  opt.push_back(miCommandLine::option('l',"lang",   1 ));
  opt.push_back(miCommandLine::option('L',"logger", 1 ));
  opt.push_back(miCommandLine::option('p',"profet", 0 ));


  miCommandLine cl(opt,argc,argv);

  

  
  if (cl.hasFlag('h')){
    cerr << helpstr << endl;
    return 0;
  }

  bool profetEnabled=cl.hasFlag('p');

  
  if(cl.hasFlag('L')) {
    miString logfilename=cl.arg('L')[0];
#ifndef NOLOG4CXX
    log4cxx::PropertyConfigurator::configure(logfilename.cStr());
#endif
  }
  else {
#ifndef NOLOG4CXX
    log4cxx::BasicConfigurator::configure();
    log4cxx::Logger::getRootLogger()->setLevel(log4cxx::Level::WARN);
#endif
  }



  miString ver_str= version_string;
  miString lang;
  miString setupfile;

  if (cl.hasFlag('s')) setupfile= cl.arg('s')[0];
  else setupfile= "diana.setup";

  SetupParser sp;
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

  // language from setup
  if(sp.basicValue("language").exists())
    lang=sp.basicValue("language");

  // language from logfile!!!!

  // language from command line
  if(cl.hasFlag('l'))
    lang=cl.arg('l')[0];

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

    for(int i=0;i<langpaths.size(); i++ )
      if( qt.load(    qtlang.cStr(),langpaths[i].cStr()))
	break;

    for(int i=0;i<langpaths.size(); i++ )
      if( myapp.load( dilang.cStr(),langpaths[i].cStr()))
	break;

    for(int i=0;i<langpaths.size(); i++ )
      if( qutil.load( qulang.cStr(),langpaths[i].cStr()))
	break;

    a.installTranslator( &qt    );
    a.installTranslator( &myapp );
    a.installTranslator( &qutil );
  }

  DianaMainWindow * mw = new DianaMainWindow(&contr, ver_str,profetEnabled);

  mw->start();

  a.setMainWidget(mw);
  mw->show();

  // news ?
  mw->checkNews();

  return a.exec();

}
