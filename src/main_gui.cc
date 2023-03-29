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
//
// Diana main program
//

#include "qtDianaApplication.h"
#include <QTranslator>
#include <QMessageBox>

#include "diCommandlineOptions.h"
#include "diController.h"
#include "diEditItemManager.h"
#include "diLocalSetupParser.h"
#include "diPrintOptions.h"
#include "miSetupParser.h"
#include "util/fimex_logging.h"

#include "qtMainWindow.h"

#include <QDir>
#include <QInputDialog>
#include <QStringList>
#include <QString>

#include <iostream>

#define MILOGGER_CATEGORY "diana.main_gui"
#include <miLogger/miLogging.h>


using namespace miutil;

namespace {

namespace po = miutil::program_options;

const po::option op_version = std::move(po::option("version", "show version info").set_narg(0));
const po::option op_lang = std::move(po::option("lang", "select language used in dialogs").set_shortkey("l").set_overwriting());
const po::option op_instancename = std::move(po::option("instancename", "instance name for coserver ('?' to choose at startup)").set_shortkey("I"));
const po::option op_quickmenu = std::move(
    po::option("quickmenu", "apply a quickmenu at start; has two arguments, menu name and item, like \"my menu\" \"my item\"").set_shortkey("qm").set_narg(2));

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
  const std::vector<std::string> langpaths = LocalSetupParser::languagePaths();
  for (std::vector<std::string>::const_iterator it = langpaths.begin(); it != langpaths.end(); ++it) {
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

  miTime::setDefaultLanguage(lang.toStdString());

  setupQtLanguage(app, lang);
}

int criticalStartupError(const QString& message)
{
  METLIBS_LOG_ERROR(message.toStdString());
  QMessageBox::critical(0, diutil::titleDianaVersion, message);
  return 1;
}

} // namespace

// ========================================================================

int main(int argc, char **argv)
{
  diutil::printVersion(std::cout, argv[0]);

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

  po::option_set cmdline_options;
  cmdline_options
      << diutil::op_help
      << diutil::op_setup
      << diutil::op_logger
      << op_lang
      << op_version
      << op_instancename
      << op_quickmenu;

  std::vector<std::string> positional;
  po::value_set vm;
  try {
    vm = parse_command_line(argc, argv, cmdline_options, positional);
  } catch (po::option_error& e) {
    std::cerr << "ERROR while parsing commandline options: " << e.what() << std::endl;
    diutil::printUsage(std::cout, cmdline_options);
    return 1;
  }

  if (vm.is_set(op_version)) {
    return 0;
  } else if (vm.is_set(diutil::op_help)) {
    diutil::printUsage(std::cout, cmdline_options);
    return 0;
  }

  FimexLoggingAdapter fla;
  milogger::LoggingConfig log4cpp(vm.value(diutil::op_logger));

  const std::map<std::string, std::string> user_variables = diutil::parse_user_variables(positional);
  SetupParser::setUserVariables(user_variables);
  std::string setupfile = vm.value(diutil::op_setup);
  if (!LocalSetupParser::parse(setupfile)){
    const QString message = setupfile.empty()
                          ? ("No setup file specified.")
                          : QString("An error occurred while reading setup: %1").arg(QString::fromStdString(setupfile));
    return criticalStartupError(message);
  }

  QString lang;
  diutil::value_if_set(vm, op_lang, lang);
  setupLanguage(&a, lang);

  QString diana_instancename;
  diutil::value_if_set(vm, op_instancename, diana_instancename);
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
    return criticalStartupError(QString("An error occurred while reading print setup: %1").arg(QString::fromStdString(setupfile)));
  }

  Controller contr;
  contr.addManager("EDITDRAWING", EditItemManager::instance());

  // read setup
  if (!contr.parseSetup()){
    return criticalStartupError(QString("An error occurred while applying setup: %1").arg(QString::fromStdString(setupfile)));
  }

  DianaMainWindow * mw = new DianaMainWindow(&contr, diana_instancename);
  mw->start();

#if defined(Q_WS_QWS)
  mw->showFullScreen();
#else
  mw->show();
  mw->resize(mw->width()-1, mw->height()-1);
  mw->resize(mw->width()+1, mw->height()+1);
#endif

  // news ?
  mw->checkNews();

  if (vm.is_set(op_quickmenu)) {
    const auto& qm_values = vm.values(op_quickmenu);
    if (qm_values.size() == 2) {
      const QString qm_menu = QString::fromStdString(qm_values[0]);
      const QString qm_item = QString::fromStdString(qm_values[1]);
      if (!qm_menu.isEmpty() && !qm_item.isEmpty())
        mw->applyQuickMenu(qm_menu, qm_item);
    }
  }

  return a.exec();
}
