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

#include "qtDianaApplication.h"
#include <QTranslator>
#include <QMessageBox>

#include "diBuild.h"
#include "diLocalSetupParser.h"
#include "diPrintOptions.h"
#include "diController.h"
#include "diEditItemManager.h"
#include "miSetupParser.h"
#include "util/fimex_logging.h"

#include "qtMainWindow.h"

#include <puTools/miStringFunctions.h>

#include <QDir>
#include <QInputDialog>
#include <QStringList>
#include <QString>

#include <iostream>

#define MILOGGER_CATEGORY "diana.main_gui"
#include <miLogger/miLogging.h>


using namespace std;
using namespace miutil;

namespace {

void printUsage()
{
  cout << "----------------------------------------------------------"    << endl
       << "Diana - a 2D presentation system for meteorological data,"     << endl
       << "including fields, observations, satellite- and radarimages,"   << endl
       << "vertical profiles and cross sections. Diana has tools for"     << endl
       << "on-screen fieldediting and drawing of objects (fronts, areas," << endl
       << "symbols etc." << endl
       << "Copyright (C) 2006-2015 met.no" << endl
       << "----------------------------------------------------------" << endl
       << "Command line arguments:"                                    << endl
       << "  -h            :  Show help"                               << endl
       << "  -v            :  Version"                                 << endl
       << "  -s <filename> :  name of setupfile (def. diana.setup)"    << endl
       << "  -qm <menu> <item> : apply a quickmenu at start"           << endl
       << "  -l <language> :  language used in dialogs"                << endl
       << "  -L <logger>   :  loggerFile for debugging"                << endl
       << "  -I <name>     :  change name of instance for coserver"    << endl
       << "                   (use ? to choose at startup)"            << endl
       << "----------------------------------------------------------" << endl;
}

class LanguageInstaller {
public:
  LanguageInstaller(QCoreApplication* a, const QString& f);
  ~LanguageInstaller();
  void load(const QString& path);

private:
  QCoreApplication* app;
  QTranslator* translator;
  QString filename;
  bool loaded;
};

LanguageInstaller::LanguageInstaller(QCoreApplication* a, const QString& f)
  : app(a)
  , translator(new QTranslator(app))
  , filename(f)
  , loaded(false)
{
}

LanguageInstaller::~LanguageInstaller()
{
  if (loaded)
    app->installTranslator(translator);
  else
    delete translator;

}

void LanguageInstaller::load(const QString& dir)
{
  if (!loaded)
    loaded = translator->load(filename, dir);
}

void setupQtLanguage(QCoreApplication* app, const QString& lang)
{
  if (lang.isEmpty())
    return;

  METLIBS_LOG_INFO("SYSTEM LANGUAGE: " << lang.toStdString());
  LanguageInstaller liQt(app, "qt_"+lang), liDiana(app, "diana_"+lang), liQUtilities(app, "qUtilities_"+lang);

  // translation files for application strings
  const vector<string> langpaths = LocalSetupParser::languagePaths();
  for (vector<string>::const_iterator it = langpaths.begin(); it != langpaths.end(); ++it) {
    const QString dir = QString::fromStdString(*it);
    liQt.load(dir);
    liDiana.load(dir);
    liQUtilities.load(dir);
  }
}

void setupLanguage(QCoreApplication* app, QString lang)
{
  if (lang.isEmpty())
    // language from setup
    lang = QString::fromStdString(LocalSetupParser::basicValue("language"));

  miTime x;
  x.setDefaultLanguage(lang.toStdString().c_str());

  setupQtLanguage(app, lang);
}

} // namespace

// ========================================================================

int main(int argc, char **argv)
{
  cout << argv[0] << " : DIANA version: " << VERSION
       << "  build: " << diana_build_string
       << "  commit: " << diana_build_commit
       << endl;

  // if LC_NUMERIC this is not "C" or something with '.' as decimal
  // separator, udunits will not be able to read unit specifications, which
  // leads to errors when reading netcdf files with fimex

  // this seems to be necessary to prevent kde image plugins / libkdecore
  // from resetting LC_NUMERIC from the environment; image plugins might,
  // e.g., be loaded when the clipboard is accessed
  setenv("LC_NUMERIC", "C", 1);

#if defined(Q_WS_QWS)
  QApplication a(argc, argv, QApplication::GuiServer);
#else
#if (QT_VERSION >= QT_VERSION_CHECK(4, 8, 0))
  QCoreApplication::setAttribute(Qt::AA_X11InitThreads);
#endif
  DianaApplication a( argc, argv );
#endif

  // setlocale must be called after initializing QApplication,
  // see http://doc.qt.io/qt-4.8/qcoreapplication.html#locale-settings
  setlocale(LC_NUMERIC, "C");
  setenv("LC_NUMERIC", "C", 1);

  string logfilename = SYSCONFDIR "/" PACKAGE_NAME "/" PVERSION "/log4cpp.properties";
  QString lang;
  bool have_diana_title = false;
  QString diana_instancename, qm_menu, qm_item;
  string setupfile;
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
        return 0;
      }
      setupfile= argv[ac];

    } else if (sarg=="-l" || sarg=="--lang") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 0;
      }
      lang = argv[ac];

    } else if (sarg=="-L" || sarg=="--logger") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 0;
      }
      logfilename= argv[ac];

    } else if (sarg=="-v" || sarg=="--version") {
      return 0;

    } else if (sarg=="-T" || sarg=="--title") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 0;
      }
      have_diana_title = true;

    } else if (sarg=="-I" || sarg=="--instancename") {
      ac++;
      if (ac >= argc) {
        printUsage();
        return 0;
      }
      diana_instancename = argv[ac];

    } else if (sarg=="-qm") {
      if (ac+2 >= argc) {
        printUsage();
        return 0;
      }
      qm_menu = argv[++ac];
      qm_item = argv[++ac];

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

  FimexLoggingAdapter fla;
  milogger::LoggingConfig log4cpp(logfilename);

  if (have_diana_title) {
    METLIBS_LOG_INFO("the -T/--title option is ignored");
  }

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
    return 0;
  }

  setupLanguage(&a, lang);

  if (diana_instancename == "?") {
    QDir logdir(QString::fromStdString(DianaMainWindow::getLogFileDir()));
    const QString ext = QString::fromStdString(DianaMainWindow::getLogFileExt());
    logdir.setNameFilters(QStringList("*" + ext));
    logdir.setFilter(QDir::Files | QDir::Readable);
    logdir.setSorting(QDir::Time);
    const QStringList logfilenames = logdir.entryList();
    QStringList instancenames;
    for (int i=0; i<logfilenames.count(); ++i) {
      const QString& lfn = logfilenames.at(i);
      const QString inn = lfn.left(lfn.size() - ext.size());
      if (DianaMainWindow::allowedInstanceName(inn)) {
        instancenames << inn;
        METLIBS_LOG_INFO("allowed" << LOGVAL(inn.toStdString()));
      } else {
        METLIBS_LOG_INFO("forbidden" << LOGVAL(inn.toStdString()));
      }
    }
    if (instancenames.size() == 1) {
      diana_instancename = instancenames.first();
    } else if (instancenames.size() >= 1) {
      bool ok;
      const QString item = QInputDialog::getItem(0,
          QCoreApplication::translate("DianaMainWindow", "Select name"),
          QCoreApplication::translate("DianaMainWindow", "Select diana name:"),
          instancenames, 0, false, &ok);
      if (ok && !item.isEmpty())
        diana_instancename = item;
    }
  }

  printerManager printman;
  if (!printman.parseSetup()) {
    METLIBS_LOG_ERROR("An error occurred while reading print setup: " << setupfile);
    QMessageBox::critical(0, QString("Diana %1").arg(VERSION),
      QString("An error occurred while reading print setup: %1").arg(QString::fromStdString(setupfile)));
    return 0;
  }

  Controller contr;
  contr.addManager("EDITDRAWING", EditItemManager::instance());

  // read setup
  if (!contr.parseSetup()){
    METLIBS_LOG_ERROR("An error occurred while reading setup: " << setupfile);
    QMessageBox::critical(0, QString("Diana %1").arg(VERSION),
      QString("An error occurred while reading setup: %1").arg(QString::fromStdString(setupfile)));
    return 0;
  }

  DianaMainWindow * mw = new DianaMainWindow(&contr, diana_instancename);
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

  if (!qm_menu.isEmpty() && !qm_item.isEmpty()) {
    mw->applyQuickMenu(qm_menu, qm_item);
  }

  return a.exec();
}
