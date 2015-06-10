/*-*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-*/
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bdiana_capi.h"

#include <qglobal.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <fstream>
#include <iostream>

#include <QtCore>
#include <QtGui>
#include <QtSvg>
#include "diPaintGLPainter.h"

#include <diAnnotationPlot.h>
#include <diController.h>
#include <diFieldPlot.h>
#include <diObsManager.h>
#include <diObsPlot.h>
#include <diPlotModule.h>
#include <diSatManager.h>
#include <diSatPlot.h>

#include <puCtools/sleep.h>
#include <puTools/miTime.h>
#include <diLocalSetupParser.h>
#include <diPrintOptions.h>
#include <diImageIO.h>
#include "diUtilities.h"
#include <puTools/miSetupParser.h>
#include <puTools/mi_boost_compatibility.hh>

#include "vcross_v2/VcrossQtManager.h"
#include "vcross_v2/VcrossQuickmenues.h"

#include <diVprofManager.h>
#include <diVprofOptions.h>

#include <diField/diFieldManager.h>
#include <diField/diRectangle.h>

#include <diSpectrumManager.h>
#include <diSpectrumOptions.h>

#include <boost/algorithm/string/join.hpp>

#include <QApplication>

#ifdef VIDEO_EXPORT
# include <MovieMaker.h>
#endif

#include <signalhelper.h>

// required to tell fimex to use log4cpp
#include <fimex/Logger.h>

#define MILOGGER_CATEGORY "diana.bdiana"
#include <miLogger/miLogging.h>

#include <diOrderBook.h>

/* Created at Wed May 23 15:28:41 2001 */

using namespace std;
using namespace miutil;

bool verbose = false;

// command-strings
const std::string com_liststart = "list.";
const std::string com_listend = "list.end";

const std::string com_loop = "loop";
const std::string com_endloop = "endloop";
const std::string com_loopend = "loop.end";

const std::string com_vprof_opt = "vprof.options";
const std::string com_vprof_opt_end = "vprof.options.end";
const std::string com_vcross_opt = "vcross.options";
const std::string com_vcross_opt_end = "vcross.options.end";
const std::string com_spectrum_opt = "spectrum.options";
const std::string com_spectrum_opt_end = "spectrum.options.end";

const std::string com_plot = "plot";
const std::string com_vcross_plot = "vcross.plot";
const std::string com_vprof_plot = "vprof.plot";
const std::string com_spectrum_plot = "spectrum.plot";
const std::string com_endplot = "endplot";
const std::string com_plotend = "plot.end";

const std::string com_print_document = "print_document";

const std::string com_wait_for_commands = "wait_for_commands";
const std::string com_wait_end = "wait.end";
const std::string com_fifo_name = "fifo";

const std::string com_setupfile = "setupfile";
const std::string com_command_path = "command_path";
const std::string com_buffersize = "buffersize";

const std::string com_papersize = "papersize";
const std::string com_filename = "filename";
const std::string com_toprinter = "toprinter";
const std::string com_printer = "printer";
const std::string com_output = "output";
const std::string com_colour = "colour";
const std::string com_drawbackground = "drawbackground";
const std::string com_orientation = "orientation";
const std::string com_antialiasing = "antialiasing";

const std::string com_settime = "settime";
const std::string com_addhour = "addhour";
const std::string com_addminute = "addminute";
const std::string com_archive = "archive";
const std::string com_keepplotarea = "keepplotarea";
const std::string com_plotannotationsonly = "plotannotationsonly";

const std::string com_fail_on_missing_data="failonmissingdata";
const std::string com_multiple_plots = "multiple.plots";
const std::string com_plotcell = "plotcell";

const std::string com_trajectory = "trajectory";
const std::string com_trajectory_opt = "trajectory_options";
const std::string com_trajectory_print = "trajectory_print";
const std::string com_setup_field_info = "setup_field_info";

const std::string com_time_opt = "time_options";
const std::string com_time_format = "time_format";
const std::string com_time = "time";
const std::string com_time_vprof = "time.vprof";
const std::string com_time_spectrum = "time.spectrum";
const std::string com_endtime = "endtime";
const std::string com_level = "level";
const std::string com_endlevel = "endlevel";

const std::string com_field_files = "<field_files>";
const std::string com_field_files_end = "</field_files>";

const std::string com_describe = "describe";
const std::string com_describe_spectrum = "describe.spectrum";
const std::string com_describe_end = "enddescribe";

enum canvas {
  qt_qimage
};

enum image_type {
  image_rgb, image_png, image_avi, image_unknown
};

// types of plot
enum plot_type {
  plot_none = 0,
  plot_standard = 1, // standard diana map-plot
  plot_vcross = 2, // vertical cross-section
  plot_vprof = 3, // profiles
  plot_spectrum = 4
// wave spectrum
};

/*
 key/value pairs from commandline-parameters
 */
struct keyvalue {
  std::string key;
  std::string value;
};

// one list of strings with name
struct stringlist {
  std::string name;
  vector<std::string> l;
};

plot_type plottype = plot_none;// current plot_type
plot_type prevplottype = plot_none;// previous plottype
plot_type multiple_plottype = plot_none;//

bool hardcopy_started[5]; // has startHardcopy been called

// the Controller and Managers
Controller* main_controller = 0;
VprofManager* vprofmanager = 0;
vcross::QtManager_p vcrossmanager;
SpectrumManager* spectrummanager = 0;

QApplication * application = 0; // The Qt Application object
QPainter painter;
QPrinter *printer = 0;
QPainter pagePainter;
static DiPaintGLCanvas* glcanvas = 0;
static DiPaintGLPainter* glpainter = 0;
QPicture picture;
QImage image;
map<std::string, map<std::string,std::string> > outputTextMaps; // output text for cases where output data is XML/JSON
vector<std::string> outputTextMapOrder;                   // order of legends in output text

int xsize; // total pixmap width
int ysize; // total pixmap height
bool multiple_plots = false; // multiple plots per page
int numcols, numrows; // for multiple plots
int plotcol, plotrow; // current plotcell for multiple plots
int deltax, deltay; // width and height of plotcells
int margin, spacing; // margin and spacing for multiple plots
bool multiple_newpage = false; // start new page for multiple plots

bool use_nowtime = false;
bool use_firsttime = false;
bool antialias = false;
bool failOnMissingData=false;

// replaceable values for plot-commands
vector<keyvalue> keys;

miTime thetime, ptime, fixedtime;

std::string batchinput;
// diana setup file
std::string setupfile = "diana.setup";
bool setupfilegiven = false;
std::string command_path;

bool keeparea = false;
bool useArchive = false;
bool toprinter = false;
bool raster = false; // false means postscript
bool shape = false; // false means postscript
bool postscript = false;
bool svg = false;
bool pdf = false;
bool json = false;
int raster_type = image_png; // see enum image_type above

bool plotAnnotationsOnly = false;
vector<Rectangle> annotationRectangles;
QTransform annotationTransform;

/*
 more...
 */
vector<std::string> vs, vvs, vvvs;
bool setupread = false;
bool buffermade = false;
vector<std::string> lines, tmplines;
vector<int> linenumbers, tmplinenumbers;

bool plot_trajectory = false;
bool trajectory_started = false;

std::string trajectory_options;

std::string time_options;
std::string time_format = "$time";

#ifdef VIDEO_EXPORT
MovieMaker *movieMaker = 0;
#endif

// list of lists..
vector<stringlist> lists;

printerManager * printman;
printOptions priop;

bool wait_for_signals = false;
bool wait_for_input = false; // if running as lib

std::string fifo_name;

std::string logfilename;

/*
 clean an input-string: remove preceding and trailing blanks,
 remove comments
 */
void cleanstr(std::string& s)
{
  std::string::size_type p;
  if ((p = s.find("#")) != string::npos)
    s.erase(p);

  miutil::remove(s, '\n');
  miutil::trim(s);
}

/*
 Recursively unpack one (or several nested) LOOP-section(s)
 1) convert a LOOP-section to multiple copies of original text
 with VARIABLES set from ARGUMENTS
 2) ARGUMENTS may be a previously defined LIST

 Syntax for LOOPS:
 LOOP VAR1 [ | VAR2 ... ] = ARG1 [ | ARG2 ... ] , ARG1 [ | ARG2 ... ] , ..
 <contents, all VAR1,VAR2,.. replaced by ARG1,ARG2,.. for each iteration>
 ENDLOOP or LOOP.END
 */
void unpackloop(vector<std::string>& orig, // original strings..
    vector<int>& origlines, // ..with corresponding line-numbers
    unsigned int& index, // original string-counter to update
    vector<std::string>& part, // final strings from loop-unpacking..
    vector<int>& partlines) // ..with corresponding line-numbers
{
  unsigned int start = index;

  std::string loops = orig[index];
  loops = loops.substr(4, loops.length() - 4);

  vector<std::string> vs, vs2;

  vs = miutil::split(loops, 0, "=");
  if (vs.size() < 2) {
    METLIBS_LOG_ERROR("ERROR, missing \'=\' in loop-statement at line:"
        << origlines[start]);
    exit(1);
  }

  std::string keys = vs[0]; // key-part
  vector<std::string> vkeys = miutil::split(keys, 0, "|");
  unsigned int nkeys = vkeys.size();

  std::string argu = vs[1]; // argument-part
  unsigned int nargu;
  vector<vector<std::string> > arguments;

  /* Check if argument is name of list
   Lists are recognized with preceding '@' */
  if (argu.length() > 1 && argu.substr(0, 1) == "@") {
    std::string name = argu.substr(1, argu.length() - 1);
    // search for list..
    unsigned int k;
    for (k = 0; k < lists.size(); k++) {
      if (lists[k].name == name)
        break;
    }
    if (k == lists.size()) {
      // list not found
      METLIBS_LOG_ERROR("ERROR, reference to unknown list at line:" << origlines[start]);
      exit(1);
    }
    nargu = lists[k].l.size();
    // split listentries into separate arguments for loop
    for (unsigned int j = 0; j < nargu; j++) {
      vs = miutil::split(lists[k].l[j], 0, "|");
      // check if correct number of arguments
      if (vs.size() != nkeys) {
        METLIBS_LOG_ERROR("ERROR, number of arguments in loop at: '" << lists[k].l[j]
            << "' line:" << origlines[start] << " does not match key: '" << keys << "'");
        exit(1);
      }
      arguments.push_back(vs);
    }

  } else {
    // ordinary arguments to loop: comma-separated
    vs2 = miutil::split(argu, 0, ",");
    nargu = vs2.size();
    for (unsigned int k = 0; k < nargu; k++) {
      vs = miutil::split(vs2[k], 0, "|");
      // check if correct number of arguments
      if (vs.size() != nkeys) {
        METLIBS_LOG_ERROR("ERROR, number of arguments in loop at:'" << vs2[k]
            << "' line:" << origlines[start] << " does not match key: '" << keys << "'");
        exit(1);
      }
      arguments.push_back(vs);
    }
  }

  // temporary storage of loop-contents
  vector<std::string> tmppart;
  vector<int> tmppartlines;

  // go to next line
  index++;

  // start unpacking loop
  for (; index < orig.size(); index++) {
    if (miutil::to_lower(orig[index]) == com_endloop || miutil::to_lower(orig[index]) ==
        com_loopend) { // reached end
      // we have the loop-contents
      for (unsigned int i = 0; i < nargu; i++) { // loop over arguments
        for (unsigned int j = 0; j < tmppart.size(); j++) { // loop over lines
          std::string l = tmppart[j];
          for (unsigned int k = 0; k < nkeys; k++) { // loop over keywords
            // replace all variables
            miutil::replace(l, vkeys[k], arguments[i][k]);
          }
          part.push_back(l);
          partlines.push_back(tmppartlines[j]);
        }
      }
      break;

    } else if (miutil::to_lower(orig[index].substr(0, 4)) == com_loop) {
      // start of new loop
      unpackloop(orig, origlines, index, tmppart, tmppartlines);

    } else { // fill loop-contents to temporary vector
      tmppart.push_back(orig[index]);
      tmppartlines.push_back(origlines[index]);
    }
  }
  if (index == orig.size()) {
    METLIBS_LOG_ERROR("ERROR, missing \'LOOP.END\' for loop at line:" << origlines[start]);
    exit(1);
  }
}

/*
 Prepare input-lines
 1. unpack loops
 2. recognize and store lists

 Syntax for list with name: <listname>
 LIST.<listname>
 <entry>
 <entry>
 ...
 LIST.END
 */
void unpackinput(vector<std::string>& orig, // original setup
    vector<int>& origlines, // original list of linenumbers
    vector<std::string>& final, // final setup
    vector<int>& finallines) // final list of linenumbers
{
  unsigned int i;
  for (i = 0; i < orig.size(); i++) {
    if (miutil::to_lower(orig[i].substr(0, 4)) == com_loop) {
      // found start of loop - unpack it
      unpackloop(orig, origlines, i, final, finallines);
    } else if (miutil::to_lower(orig[i].substr(0, 5)) == com_liststart) {
      // save a list
      stringlist li;
      if (orig[i].length() < 6) {
        METLIBS_LOG_ERROR("ERROR, missing name for LIST at line:" << origlines[i]);
        exit(1);
      }
      li.name = orig[i].substr(5, orig[i].length() - 5);
      unsigned int start = i;
      i++;
      for (; i < orig.size() && miutil::to_lower(orig[i]) != com_listend; i++)
        li.l.push_back(orig[i]);
      if (i == orig.size() || miutil::to_lower(orig[i]) != com_listend) {
        METLIBS_LOG_ERROR("ERROR, missing LIST.END for list starting at line:"
               << origlines[start]);
        exit(1);
      }
      // push it..
      lists.push_back(li);
    } else {
      // plain input-line -- push it on the final list
      final.push_back(orig[i]);
      finallines.push_back(origlines[i]);
    }
  }
}

int prepareInput(istream &is)
{
  unsigned int linenum = 0;
  //   if ( tmplinenumbers.size() > 0 )
  //     linenum = linenumbers[ linenumbers.size() - 1 ];

  tmplines.clear();
  tmplinenumbers.clear();
  lines.clear();
  linenumbers.clear();

  std::string s;
  int n;
  bool merge = false, newmerge;

  /*
   read inputfile
   - skip blank lines
   - strip lines for comments and left/right whitespace
   - merge lines (ending with \)
   */

  while (getline(is, s)) {
    linenum++;
    cleanstr(s);
    n = s.length();
    if (n > 0) {
      newmerge = false;
      if (s[n - 1] == '\\') {
        newmerge = true;
        s = s.substr(0, s.length() - 1);
      }
      if (merge) {
        tmplines[tmplines.size() - 1] += s;
      } else {
        tmplines.push_back(s);
        tmplinenumbers.push_back(linenum);
      }
      merge = newmerge;
    }
  }
  // unpack loops and lists
  unpackinput(tmplines, tmplinenumbers, lines, linenumbers);

  linenum = lines.size();

  // substitute environment variables and key-values
  unsigned int nkeys = keys.size();
  for (unsigned int k = 0; k < linenum; k++) {
    SetupParser::checkEnvironment(lines[k]);
    for (unsigned int m = 0; m < nkeys; m++)
      miutil::replace(lines[k], "$" + keys[m].key, keys[m].value);
  }
  return 0;
}

#ifdef VIDEO_EXPORT
void startVideo(const printOptions priop)
{
  const std::string format = "avi";
  const std::string output = priop.fname;

  if (movieMaker &&
      !movieMaker->outputFormat().compare(format) &&
      !movieMaker->outputFile().compare(output))
  {
    return;
  }

  delete movieMaker;
  METLIBS_LOG_INFO("opening video stream |-->");
  movieMaker = new MovieMaker(output, format, 0.2f, QSize(xsize, ysize));
}

bool addVideoFrame(const QImage &img)
{
  if (!movieMaker)
    return false;

  return movieMaker->addImage(img);
}

void endVideo()
{
  METLIBS_LOG_INFO("-->| video stream closed");
  delete movieMaker;
  movieMaker = 0;
}
#endif // VIDEO_EXPORT

void startHardcopy(const plot_type pt, const printOptions priop)
{
  if (!printer) {
    printer = new QPrinter();
    printer->setOutputFileName(QString::fromStdString(priop.fname));
    if (pdf)
      printer->setOutputFormat(QPrinter::PdfFormat);
    else
      printer->setOutputFormat(QPrinter::PostScriptFormat);

    if (priop.usecustomsize) {
      printer->setPaperSize(QSizeF(priop.papersize.hsize, priop.papersize.vsize), QPrinter::Millimeter);

      // According to QTBUG-23868, orientation and custom paper sizes do not
      // play well together. Always use portrait.
      printer->setOrientation(QPrinter::Portrait);
    } else {
      // The pagesize option maps directly to QPrinter's PaperSize enum.
      printer->setPaperSize(QPrinter::PaperSize(priop.pagesize));

      if (priop.orientation == d_print::ori_landscape)
        printer->setOrientation(QPrinter::Landscape);
      else
        printer->setOrientation(QPrinter::Portrait);
    }

    printer->setFullPage(true);

    QSizeF size = printer->paperSize(QPrinter::DevicePixel);

    double xscale = size.width()/xsize;
    double yscale = size.height()/ysize;
    double scale = qMin(qMin(xscale, yscale), 1.0);
    pagePainter.begin(printer);
    pagePainter.translate(size.width()/2.0, size.height()/2.0);
    if (scale != 1.0)
      pagePainter.scale(scale, scale);
    pagePainter.translate(-xsize/2.0, -ysize/2.0);
    pagePainter.setClipRect(QRectF(0, 0, xsize, ysize));
  } else
      printer->newPage();
  hardcopy_started[pt] = true;
}

static void ensureNewContext();
static void printPage(int ox, int oy);

void endHardcopy(const plot_type pt)
{
  // Guard against this function being called before printing occurs
  // or in cases where it is unnecessary.
  if (!painter.isActive() || shape)
    return;

  // If we have printed then we can no longer be making multiple plots.
  multiple_plots = false;

  ensureNewContext();

  if (pdf || postscript)
    printPage(0, 0);
  hardcopy_started[pt] = false;
}

// VPROF-options with parser
std::vector<std::string> vprof_stations;
vector<string> vprof_models, vprof_options;
bool vprof_plotobs = true;
bool vprof_optionschanged;

void parse_vprof_options(const vector<string>& opts)
{
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    std::string line = opts[i];
    miutil::trim(line);
    if (line.empty())
      continue;
    std::string upline = miutil::to_upper(line);

    if (upline == "OBSERVATION.ON")
      vprof_plotobs = true;
    else if (upline == "OBSERVATION.OFF")
      vprof_plotobs = false;
    else if (miutil::contains(upline, "MODELS=") || miutil::contains(upline, "MODEL=")
        || miutil::contains(upline, "STATION=")) {
      vector<std::string> vs = miutil::split(line, "=");
      if (vs.size() > 1) {
        std::string key = miutil::to_upper(vs[0]);
        std::string value = vs[1];
        if (key == "STATION") {
          if (miutil::contains(value, "\""))
            miutil::remove(value, '\"');
          vprof_stations  = miutil::split(value, ",");
        } else if (key == "MODELS" || key == "MODEL") {
          vprof_models = miutil::split(value, 0, ",");
        }
      }
    } else {
      // assume plot-options
      vprof_options.push_back(line);
      vprof_optionschanged = true;
    }
  }
}


// SPECTRUM-options with parser
std::string spectrum_station;
vector<string> spectrum_models,spectrum_options;
bool spectrum_optionschanged;

static void parse_spectrum_options(const vector<string>& opts)
{
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    std::string line = opts[i];
    miutil::trim(line);
    if (line.empty())
      continue;
    std::string upline = miutil::to_upper(line);

    if (miutil::contains(upline, "MODELS=") || miutil::contains(upline, "MODEL=")
        || miutil::contains(upline, "STATION=")) {
      vector<std::string> vs = miutil::split(line, "=");
      if (vs.size() > 1) {
        std::string key = miutil::to_upper(vs[0]);
        std::string value = vs[1];
        if (key == "STATION") {
          if (miutil::contains(value, "\""))
            miutil::remove(value, '\"');
          spectrum_station = value;
        } else if (key == "MODELS" || key == "MODEL") {
          spectrum_models = miutil::split(value, 0, ",");
        }
      }
    } else {
      // assume plot-options
      spectrum_options.push_back(line);
      spectrum_optionschanged = true;
    }
  }
}

/*
 parse setupfile
 perform other initialisations based on setup information
 */
static bool readSetup(const std::string& constSetupfile, printerManager& printmanager)
{
  std::string setupfile = constSetupfile;
  METLIBS_LOG_INFO("Reading setupfile: '" << setupfile << "'");

  if (!LocalSetupParser::parse(setupfile)) {
    METLIBS_LOG_ERROR("ERROR, an error occured while reading setup: '" << setupfile << "'");
    return false;
  }
  if (!printmanager.parseSetup()) {
    METLIBS_LOG_ERROR("ERROR, an error occured while reading setup: '" << setupfile << "'");
    return false;
  }

  // language from setup
  if (not LocalSetupParser::basicValue("language").empty()) {
    std::string lang = LocalSetupParser::basicValue("language");
    { miTime x; x.setDefaultLanguage(lang.c_str()); }
  }

  return true;
}

/*
 * public C-version of above readSetup
 */
int diana_readSetupFile(const char* setupFilename) {

    using namespace std; using namespace miutil;
    printman = new printerManager();
    std::string setupfile(setupFilename);
    setupread = readSetup(setupfile, *printman);
    if (!setupread) {
      METLIBS_LOG_ERROR("ERROR, unable to read setup: '" << setupfile << "'");
      return DIANA_ERROR;
    }
    return DIANA_OK;
}


/*
 Output Help-message:  call-syntax and optionally an example
 input-file for bdiana
 */
static void printUsage(bool showexample)
{
  const std::string help =
      "***************************************************             \n"
        " DIANA batch version:" + std::string(VERSION) + "                  \n"
        " plot products in batch                                         \n"
        "***************************************************             \n"
        " Available products:                                            \n"
        " - All standard map-products from DIANA                         \n"
        " - Vertical cross sections                                      \n"
        " - Vertical profiles                                            \n"
        " - WaveSpectrum plots                                           \n"
        " Available output-formats:                                      \n"
        " - as PostScript (to file and printer)                          \n"
        " - as EPS (Encapsulated PostScript)                             \n"
        " - as PNG (raster-format)                                       \n"
#ifdef VIDEO_EXPORT
        " - as AVI (MS MPEG4-v2 video format)                            \n"
#endif
        " - using qimage: all available raster formats in Qt             \n"
        "***************************************************             \n"
        "                                                                \n"
        "Usage: bdiana -i <job-filename>"
        " [-s <setup-filename>]"
        " [-v]"
        " [-display xhost:display]"
        " [-example]"
        " ["
        " -use_qimage ]"
        " [key=value key=value] \n"
        "                                                                        \n"
        "-i                : job-control file. See example below                 \n"
        "-s                : setupfile for diana                                 \n"
        "-v                : (verbose) for more job-output                       \n"
        "-address=addr[:port]                                                    \n"
        "                  : production triggered by TCP connection              \n"
        "                    addr is a hostname or IP address                    \n"
        "                    port is an optional port number, default is 3190    \n" // diOrderListener::DEFAULT_PORT
        "-signal           : production triggered by SIGUSR1 signal (see example)\n"
        "-libinput         : using bdiana_capi as library                        \n"
        "-example          : list example input-file and exit                    \n"
        "-use_qimage       : use QImage as drawing medium                        \n"
        "                                                                        \n"
        "special key/value pairs:                                                \n"
        " - TIME=\"YYYY-MM-DD hh:mm:ss\"      plot-time                          \n"
        "                                                                        \n";

  const std::string
      example =
          "#--------------------------------------------------------------   \n"
            "# inputfile for bdiana                                            \n"
            "# - '#' marks start of comment.                                   \n"
            "# - you may split long lines by adding '\\' at the end.           \n"
            "#--------------------------------------------------------------   \n"
            "                                                                  \n"
            "#- Mandatory:                                                     \n"
            "buffersize=1696x1200     # plotbuffer (WIDTHxHEIGHT)              \n"
            "                         # For output=RASTER: size of plot.       \n"
            "                         # For output=POSTSCRIPT: size of buffer  \n"
            "                         #  affects output-quality. TIP: make     \n"
            "                         #  sure width/height ratio = width/height\n"
            "                         #  ratio of MAP-area (defined under PLOT)\n"
            "                                                                  \n"
            "#- Optional: values for each option below are default-values      \n"
            "setupfile=diana.setup    # use a standard setup-file              \n"
            "output=POSTSCRIPT        # POSTSCRIPT/EPS/PNG/RASTER/AVI/SHP      \n"
            "                         #  RASTER: format from filename-suffix   \n"
            "                         #  PDF/SVG/JSON (only with -use_qimage)  \n"
            "                         #  JSON (only for annotations)           \n"
            "colour=COLOUR            # GREYSCALE/COLOUR                       \n"
            "filename=tmp_diana.ps    # output filename                        \n"
            "keepPlotArea=NO          # YES=try to keep plotarea for several   \n"
            "                         # plots                                  \n"
            "plotAnnotationsOnly=NO   # YES=only plot annotations/legends      \n"
            "                         # (only available with -use_qimage)      \n"
            "antialiasing=NO          # only available with -use_qimage        \n"
            "                                                                  \n"
            "# the following options for output=POSTSCRIPT or EPS only         \n"
            "toprinter=NO             # send output to printer (postscript)    \n"
            "                         # obsolete command! use PRINT_DOCUMENT instead\n"
            "printer=fou3             # name of printer        (postscript)    \n"
            "                         # (see PRINT_DOCUMENT command below)     \n"
            "papersize=297x420,A4     # size of paper in mm,   (postscript)    \n"
            "                         # papertype (A4 etc) or both.            \n"
            "drawbackground=NO        # plot background colour (postscript)    \n"
            "orientation=LANDSCAPE    # PORTRAIT/LANDSCAPE     (postscript)    \n"
            "                         # (default here is really 'automatic'    \n"
            "                         # which sets orientation according to    \n"
            "                         # width/height-ratio of buffersize)      \n"
            "                                                                  \n"
            "# Extra fields: including field sources not in the setup file     \n"
            "<FIELD_FILES>                                                     \n"
            "m=Hirlam12 t=fimex f=hirlam12/hirlam12ml.nc format=netcdf         \n"
            "</FIELD_FILES>                                                    \n"
            "                                                                  \n"
            "#--------------------------------------------------------------   \n"
            "# Product-examples:                                               \n"
            "# Products are made by one or more PLOT-sections seen below,      \n"
            "# in between two PLOT-sections you may change any of the options   \n"
            "# described above.                                                \n"
            "# The data-time will be set from the TIME=\"isotime-string\"      \n"
            "# commandline parameter.                                          \n"
            "# Output filename may contain data-time, format see man date      \n"
            "# Example: filename=diana_%Y%M%dT%H.ps                            \n"
            "#--------------------------------------------------------------   \n"
            "# STANDARD MAP-PRODUCT SECTION:                                   \n"
            "PLOT                     # start of plot-command for map-product  \n"
            "# paste in commands from quick-menues (one line for each element) \n"
            "                                                                  \n"
            "FIELD HIRLAM.00 DZ(500-850) colour=yellow linetype=solid \\\n"
            " linewidth=1 line.interval=40 extreme.type=None extreme.size=1 \\\n"
            " extreme.radius=1 line.smooth=0 value.label=1 label.size=1  \\\n"
            " field.smooth=0 grid.lines=0 undef.masking=0 undef.colour=white \\\n"
            " undef.linewidth=1 undef.linetype=solid\n"
            "FIELD DNMI.ANA MSLP colour=blue linetype=dash linewidth=2 \\\n"
            " line.interval=1 extreme.type=None extreme.size=1 \\\n"
            " extreme.radius=1 line.smooth=0 value.label=1 label.size=1 \\\n"
            " field.smooth=0 grid.lines=0 undef.masking=0 undef.colour=white \\\n"
            " undef.linewidth=1 undef.linetype=solid\n"
            "OBS plot=Synop data=Synop parameter=Vind,TTT,TdTdTd,PPPP,ppp,a,h,\\\n"
            " VV,N,RRR,ww,W1,W2,Nh,Cl,Cm,Ch,vs,ds,TwTwTw,PwaHwa,Dw1Dw1,Pw1Hw1,\\\n"
            " TxTn,sss,911ff,s,fxfx,Kjtegn  tempprecision=true density=1 scale=1 \\\n"
            " timediff=180 colour=black font=BITMAPFONT face=normal\n"
            "OBJECTS NAME=\"DNMI Bakkeanalyse\" types=front,symbol,area \\\n"
            " timediff=60\n"
            "MAP area=Norge backcolour=white map=Gshhs-AUTO contour=on \\\n"
            " cont.colour=black cont.linewidth=1 cont.linetype=solid cont.zorder=1 \\\n"
            " land=on land.colour=landgul land.zorder=0 latlon=off frame=off\n"
            "LABEL data font=BITMAPFONT\n"
            "LABEL text=\"$day $date $auto UTC\" tcolour=red bcolour=black \\\n"
            " fcolour=white:200 polystyle=both halign=left valign=top \\\n"
            " font=BITMAPFONT fontsize=12\n"

            "                                                                  \n"
            "ENDPLOT                  # End of plot-command                    \n"
            "#--------------------------------------------------------------   \n"
            "# VERTICAL CROSSECTION SECTION:                                   \n"
            "filename=vcross.ps                                                \n"
            "                                                                  \n"
            "# detailed options for plot                                       \n"
            "#VCROSS.OPTIONS                                                   \n"
            "#text=on textColour=black                                         \n"
            "#frame=on frameColour=black frameLinetype=solid frameLinewidth=1  \n"
            "#etc...  (see DIANA documentation or diana.log)                   \n"
            "#VCROSS.OPTIONS.END                                               \n"
            "                                                                  \n"
            "VCROSS.PLOT              # start of vertical crossection plot     \n"
            "                                                                  \n"

            "VCROSS model=HIRLAM.00 field=Temp(C) colour=black linetype=solid \\\n"
            " linewidth=1 line.interval=4 line.smooth=0 value.label=1 \\       \n"
            " label.size=1                                                     \n"
            "VCROSS model=HIRLAM.00 field=Temp(C) colour=red linetype=solid \\ \n"
            " linewidth=1 line.interval=4 line.smooth=0 value.label=1 \\       \n"
            " label.size=1                                                     \n"
            "                                                                  \n"
            "CROSSECTION=A.(70N,30W)-(50N,30W) # name of crossection           \n"
            "                                                                  \n"
            "ENDPLOT                  # End of plot-command                    \n"
            "#--------------------------------------------------------------   \n"
            "# VERTICAL PROFILE SECTION:                                       \n"
            "filename=vprof.ps                                                 \n"
            "                                                                  \n"
            "# detailed options for plot                                       \n"
            "#VPROF.OPTIONS                                                    \n"
            "#tttt=on                                                          \n"
            "#tdtd=on                                                          \n"
            "#etc...  (see DIANA documentation or diana.log)                   \n"
            "#VPROF.OPTIONS.END                                                \n"
            "                                                                  \n"
            "VPROF.PLOT               # start of vertical profile plot         \n"
            "                                                                  \n"
            "OBSERVATION.ON           # plot observation: OBSERVATION.ON/OFF   \n"
            "MODELS=AROME-MetCoOp.00, HIRLAM.12KM.00  # comma-separated list of models \n"
            "STATION=KIRKENES         # station-name                           \n"
            "                                                                  \n"
            "ENDPLOT                  # End of plot-command                    \n"
            "#--------------------------------------------------------------   \n"
            "# SPECTRUM SECTION:                                               \n"
            "filename=spectrum.ps                                              \n"
            "                                                                  \n"
            "# detailed options for plot                                       \n"
            "#SPECTRUM.OPTIONS                                                 \n"
            "#freqMax=0.3                                                      \n"
            "#backgroundColour=white                                           \n"
            "#etc...  (see DIANA documentation or diana.log)                   \n"
            "#SPECTRUM.OPTIONS.END                                             \n"
            "                                                                  \n"
            "SPECTRUM.PLOT            # start of spectrum plot                 \n"
            "                                                                  \n"
            "MODEL=WAM.50KM.00        # model                                  \n"
            "STATION=\"60.1N 5.3W\"   # station-name                           \n"
            "                                                                  \n"
            "ENDPLOT                  # End of plot-command                    \n"
            "#--------------------------------------------------------------   \n"
            "# ADDITIONAL:                                                     \n"
            "#--------------------------------------------------------------   \n"
            "#- You can add LOOPS with one or more variables:                  \n"
            "#  LOOP [X]|[Y] = X_value1 | Y_value1 , X_value2 | Y_value2       \n"
            "#   <any other input lines, all \"[X]\" and \"[Y]\" will be       \n"
            "#   replaced by the values after '=' for each iteration>          \n"
            "#  LOOP.END                                                       \n"
            "#  The example shows a loop with two variables ([X] and [Y],      \n"
            "#  separated by '|') and two iterations (separated with ',')      \n"
            "#  Loops kan be nested                                            \n"
            "#--------------------------------------------------------------   \n"
            "#- Make a LIST for use in loops:                                  \n"
            "#  LIST.stations           # A new list with name=stations        \n"
            "#  OSLO                    # May contain any strings..            \n"
            "#  KIRKENES                #                                      \n"
            "#  LIST.END                # Marks End of list                    \n"
            "#                                                                 \n"
            "#  To use in a loop:                                              \n"
            "#  LOOP [VAR]=@stations    # The key here is the \'@\' with the   \n"
            "#                          # list-name.                           \n"
            "#  LOOP.END                # This will loop over all list-entries \n"
            "#                                                                 \n"
            "#  NOTES:                                                         \n"
            "#  - To make a list with multiple variables, convenient for       \n"
            "#    multiple-variable loops, just add \'|\'s in the list-strings.\n"
            "#    Example:                                                     \n"
            "#    LIST.name             # new list                             \n"
            "#    OSLO | blue           # two variables for each entry         \n"
            "#    KIRKENES | red        #                                      \n"
            "#    LIST.END                                                     \n"
            "#                                                                 \n"
            "#    LOOP [POS] | [COL] = @name # Loop using two variables in list\n"
            "#    LOOP.END                                                     \n"
            "#  - Lists must be defined OUTSIDE all loop statements            \n"
            "#--------------------------------------------------------------   \n"
            "#- alternative to TIME=.. commandline option:                     \n"
            "#  (default time is the last available time)                      \n"
            "#  use settime=YYYY-MM-DD hh:mm:ss                                \n"
            "#  use settime=currenttime / nowtime / firsttime                  \n"
            "#- use addhour=<value> or addminute=<value> to increment datatime \n"
            "#  (offset from TIME=\"\" variable). Useful in loops              \n"
            "#--------------------------------------------------------------   \n"
            "#- \"key=value\" pairs given on the commandline controls variables\n"
            "#  in the inputfile: Any \"$key\" found in the text will be       \n"
            "#  substituted by \"value\".                                      \n"
            "#--------------------------------------------------------------   \n"
            "#- toggle archive mode (for observations)                         \n"
            "#  archive=ON/OFF  (default=OFF)                                  \n"
            "#--------------------------------------------------------------   \n"
            "#- Making TRAJECTORIES                                            \n"
            "#  to create and plot trajectories from any vector-field use:     \n"
            "#  TRAJECTORY_OPTIONS=<options>    # see available options below  \n"
            "#  TRAJECTORY=ON                   # Set this before PLOT command \n"
            "#  TRAJECTORY=OFF                  # Turn off trajectories when finished\n"
            "#  Valid options:                                                 \n"
            "#  latitudelongitude=<lat>,<lng>   # start position, repeat if necessary\n"
            "#  field=\"<full fieldname>\"        # Example: \"HIRLAM.00 VIND10.M\" \n"
            "#  colour=<colourname>             # Colour of trajectory lines   \n"
            "#  line=<linewidth>                # width of trajectory lines    \n"
            "#  numpos=1,5 or 9                 # number of sub-positions      \n"
            "#  radius=<spreadth in km>         # if numpos>1, spreadth in km  \n"
            "#  timemarker=<minutes>            # marker for each specified minute\n"
            "# \n"
            "#  Example: \n"
            "#    TRAJECTORY_OPTIONS=latitudelongitude=58.0,11.0 latitudelongitude=60.0,9.0 \\ \n"
            "#    latitudelongitude=66.0,0.0 field=\"HIRLAM.00 VIND.10M\" colour=red \\ \n"
            "#    line=2 numpos=5 radius=50 timemarker=0 \n"
            "#  specifies 3 start positions, each with 5 sub-pos., using a HIRLAM \n"
            "#  wind-field as input (must also be specified in PLOT section)      \n"
            "#  Prepare for trajectories \n"
            "#    TRAJECTORY=ON  \n"
            "#  followed by a series of PLOT sections where the above field must be \n"
            "#  specified, running through the desired timesteps: \n"
            "#    LOOP [HOUR]=0,3,6,9 \n"
            "#    ADDHOUR=[HOUR] \n"
            "#    FILENAME=traj_[HOUR].png \n"
            "#    PLOT \n"
            "#     FIELD HIRLAM.00 VIND.10M ........ etc. \n"
            "#    ENDPLOT \n"
            "#    LOOP.END \n"
            "#  Finally, turn off trajectories with: \n"
            "#    TRAJECTORY=OFF \n"
            "#--------------------------------------------------------------   \n"
            "#* PostScript output * \n"
            "#- Send current postscript-file to printer (immediate command):   \n"
            "#  PRINT_DOCUMENT                         \n"
            "#\n"
            "#- MULTIPLE PLOTS PER PAGE                  \n"
            "#  You can put several plots in one postscript page by using the \'multiple.plots\'\n"
            "#  and \'plotcell\' commands. Start with describing the layout you want:\n"
            "# \n"
            "#  MULTIPLE.PLOTS=<rows>,<columns> # set the number of rows and columns\n"
            "# \n"
            "#  In the same command, you can specify the spacing between plots and \n"
            "#  the page-margin (given as percent of page width/height [0-100]): \n"
            "#  MULTIPLE.PLOTS=<rows>,<columns>,<spacing>,<margin> \n"
            "#  \n"
            "#  Then, for each separate plot use the plotcell command to place plot on page:\n"
            "#  PLOTCELL=<row>,<column>         # the row and column, starting with 0\n"
            "#  \n"
            "#  Finally, end the page with: \n"
            "#  MULTIPLE.PLOTS=OFF \n"
            "#  \n"
            "#- To produce multi-page postscript files: Just make several plots \n"
            "#  to the same filename (can be combined with MULTIPLE.PLOTS). \n"
            "#  You can not mix map plots, cross sections or soundings in one file\n"
            "#  \n"
            "#- Use of alpha-channel blending is not supported in postscript \n"
            "#--------------------------------------------------------------   \n"
            "#* Interactive batch-plotting *                  \n"
            "#  To make plots on-demand, you will find this feature useful.  \n"
            "#  Put this in the input-file:  \n"
            "#    command_path=<some_path> \n"
            "#    wait_for_commands \n"
            "#  and bdiana will wait at the \'wait_for_commands\' line, periodically\n"
            "#  checking \'<some_path>\' for a matching filename. When found, the \n"
            "#  file(s) are read and the instructions within executed. If the line:\n"
            "#    wait.end \n"
            "#  is encountered, bdiana will stop waiting for commands. \n"
            "#  The \'command_path\' must specify a complete filename - optionally \n"
            "#  with wildcards. Example: \'/tmp/*.txt\' \n"
            "#  NB: all files found are deleted immediately after reading.  \n"
            "#--------------------------------------------------------------   \n"
            "#* Batch-plotting triggered by Signals *                  \n"
            "#  Alternative on-demand production (developed for use in WMS) \n"
            "#  Start bdiana with the -signal commandline option. \n"
            "#  After the initial input/setup have been read, bdiana will write its \n"
            "#  pid-number to file bdiana.pid, and wait for a SIGUSR1-signal from an \n"
            "#  external process. After a signal is received, the command_path is scanned\n"
            "#  for commandfile(s), and these will be processed in the same fashion as\n"
            "#  in the wait_for_commands section above.\n"
            "#  If a confirmative answer is required, add the command fifo=<name>, and\n"
            "#  bdiana will write the single character \'r\' to the named FIFO. NB: The \n"
            "#  FIFO must exist! \n"
            "#--------------------------------------------------------------        \n"
            "#* Get Capabilities *                                                  \n"
            "#  Developed for use in WMS                                            \n"
            "#  The syntax of the TIME-, LEVEL- and DESCRIBE-sections are equal to  \n"
            "#  the PLOT-sections. The plot options will be ignored.                \n"
            "#  The TIME-sections give available times,                             \n"
            "#  both normal and constant times.                                     \n"
            "#  The LEVEL-sections give available levels.                           \n"
            "#  The DESCRIBE-sections give information about the files read.        \n"
            "#  Valid options:                                                      \n"
            "#  Normal times common to all products and  all constant times:        \n"
            "#  time_options = intersection                                         \n"
            "#  All normal times from all products:                                 \n"
            "#  time_options = union                                                \n"
            "#  Time format in outputfile                                           \n"
            "#  time_format=%Y-%m-%dT%H:%M:%S                                       \n"
            "#  TIME                                                                \n"
            "#  FIELD HIRLAM.00 DZ(500-850) colour=yellow linetype=solid \\         \n"
            "#  linewidth=1 line.interval=40 extreme.type=None extreme.size=1 \\   \n"
            "#  extreme.radius=1 line.smooth=0 value.label=1 label.size=1  \\       \n"
            "#  field.smooth=0 grid.lines=0 undef.masking=0 undef.colour=white \\   \n"
            "#  undef.linewidth=1 undef.linetype=solid                              \n"
            "#  FIELD DNMI.ANA MSLP                                                 \n"
            "#  OBS plot=Synop data=Synop parameter=Vind,TTT,TdTdTd,PPPP,ppp,a,h,\\ \n"
            "#  VV,N,RRR,ww,W1,W2,Nh,Cl,Cm,Ch,vs,ds,TwTwTw,PwaHwa,Dw1Dw1,Pw1Hw1,\\  \n"
            "#  TxTn,sss,911ff,s,fxfx,Kjtegn  tempprecision=true density=1 \\       \n"
            "#  scale=1 timediff=180 colour=black font=BITMAPFONT face=normal        \n"
            "#  OBJECTS NAME=\"DNMI Bakkeanalyse\" types=front,symbol,area          \n"
            "#  ENDTIME                                                             \n"
            "#  LEVEL                                                               \n"
            "#  FIELD HIRLAM.20KM.00 Z                                              \n"
            "#  ENDLEVEL                                                            \n"
            "#--------------------------------------------------------------      \n"
            "\n";

  if (!showexample)
    cout << help << endl;
  else
    cout << example << endl;

  exit(1);
}

static miutil::miTime selectNowTime(vector<miutil::miTime>& fieldtimes,
                                    vector<miutil::miTime>& sattimes,
                                    vector<miutil::miTime>& obstimes,
                                    vector<miutil::miTime>& objtimes,
                                    vector<miutil::miTime>& ptimes)
{
  const miTime now = miTime::nowTime();

  if (fieldtimes.empty()) {
    if (!sattimes.empty())
      return sattimes.back();
    else if (!obstimes.empty())
      return obstimes.back();
    else if (!objtimes.empty())
      return objtimes.back();
    else if (!ptimes.empty())
      return ptimes.back();
    else
      return now;
  }

  // select closest to now without overstepping
  const int n = fieldtimes.size();
  for (int i = 0; i < n; i++) {
    if (fieldtimes[i] >= now) {
      return i > 0 ? fieldtimes[i - 1] : fieldtimes[i];
    }
  }

  return fieldtimes.back();
}

static miutil::miTime selectTime()
{
  map<string,vector<miTime> > times;
  main_controller->getPlotTimes(times, false);
  miTime thetime;

  // Check for times in a certain order, initialising a time vector as
  // empty vectors if the getPlotTimes function did not return one for
  // a given data type.
  if (ptime.undef()) {
    if (use_nowtime) {
      thetime = selectNowTime(times["fields"], times["satellites"],
          times["observations"], times["objects"],
          times["products"]);
    } else if ( use_firsttime ) {
      if (times["fields"].size() > 0)
        thetime = times.at("fields").front();
      else if (times["satellites"].size() > 0)
        thetime = times.at("satellites").front();
      else if (times["observations"].size() > 0)
        thetime = times.at("observations").front();
      else if (times["objects"].size() > 0)
        thetime = times.at("objects").front();
      else if (times["products"].size() > 0)
        thetime = times.at("products").front();
    } else {
      if (times["fields"].size() > 0)
        thetime = times.at("fields").back();
      else if (times["satellites"].size() > 0)
        thetime = times.at("satellites").back();
      else if (times["observations"].size() > 0)
        thetime = times.at("observations").back();
      else if (times["objects"].size() > 0)
        thetime = times.at("objects").back();
      else if (times["products"].size() > 0)
        thetime = times.at("products").back();
    }
    fixedtime = ptime = thetime;
  } else
    thetime = ptime;

  return thetime;
}

/*
 * Returns the area covered in the requested annotation in the ox, oy, xsize and ysize
 * variables passed as arguments. If the annotation number is -1 then the area covered
 * by all annotations will be returned.
*/
static void getAnnotationsArea(int& ox, int& oy, int& xsize, int& ysize, int number = -1)
{
  QRectF cutout;
  int i = 0;

  vector<Rectangle>::iterator it;
  for (it = annotationRectangles.begin(); it != annotationRectangles.end(); ++it) {
    QRectF r = annotationTransform.mapRect(QRectF(it->x1, it->y1, it->width(), it->height()));
    if (cutout.isNull() && (i == number || number == -1))
      cutout = r;
    else if (!r.isNull() && (i == number || number == -1))
      cutout = cutout.united(r);

    i += 1;
  }

  if (!cutout.isNull()) {
    ox = -cutout.x();
    oy = -cutout.y();
    xsize = cutout.width();
    ysize = cutout.height();
  }
}

void createJsonAnnotation()
{
  vector<AnnotationPlot*> annotationPlots = main_controller->getAnnotations();
  for (vector<AnnotationPlot*>::iterator it = annotationPlots.begin(); it != annotationPlots.end(); ++it) {

    vector<AnnotationPlot::Annotation> annotations = (*it)->getAnnotations();
    for (vector<AnnotationPlot::Annotation>::iterator iti = annotations.begin(); iti != annotations.end(); ++iti) {

      for (vector<string>::iterator itj = iti->vstr.begin(); itj != iti->vstr.end(); ++itj) {

        // Find the table description in the string.
        std::string legend = (*itj);
        size_t at = legend.find("table=\"");

        if (at != string::npos) {
          // Find the trailing quote.
          at += 7;
          size_t end = legend.find("\"", at);
          if (end == string::npos)
              end = legend.size();

          // Remove leading and trailing quotes.
          legend = legend.substr(at, end - at);

          map<std::string,std::string> textMap;

          std::string title;
          vector<std::string> colors;
          vector<std::string> labels;

          bool first = true;
          vector<std::string> line;
          std::string current;
          unsigned int i = 0;

          while (i < legend.size()) {

            bool publish;

            if (legend[i] == ';') {
              // Add the remaining characters in the current piece to the list.
              line.push_back(current);
              current.clear();
              if (i < legend.size() - 1) {
                if (legend[i + 1] == ';') {
                  // End of a piece, so skip the next character.
                  publish = false;
                  i += 1;
                } else {
                  // End of a line.
                  publish = true;
                }
              } else {
                // End of the legend follows the semicolon.
                publish = true;
              }
            } else {
              current += legend[i];
              if (i == legend.size() - 1) {
                // End of the legend.
                line.push_back(current);
                publish = true;
              } else
                publish = false;
            }

            ++i;

            if (publish) {
              if (first) {
                title = line[0];
                textMap["title"] = "\"" + title + "\"";
                first = false;
              } else {
                Colour color = Colour(line[0]);
                stringstream cs;
                cs.flags(ios::hex);
                cs.width(6);
                cs.fill('0');
                cs << ((int(color.R()) << 16) | (int(color.G()) << 8) | int(color.B()));
                colors.push_back("\"" + cs.str() + "\"");
                std::string label = line[1];
                miutil::trim(label);
                labels.push_back("\"" + label + "\"");
              }
              line.clear();
            }
          }
          textMap["colors"] = "[" + boost::algorithm::join(colors, ", ") + "]";
          textMap["labels"] = "[" + boost::algorithm::join(labels, ", ") + "]";

          if (!title.empty()) {
            outputTextMaps[title] = textMap;
            outputTextMapOrder.push_back(title);
          }
        }
      }
    }
  }

  // Add a metadata entry to describe the request that corresponds to this reply.
  // The wms clients did not accept metadata, temporarily removed until clients are fixed
  //  map<std::string,std::string> metaDataMap;
//
//  std::string thetime;
//  main_controller->getPlotTime(thetime);
//  if ( !ptime.undef() ) {
//    metaDataMap["request time"] = std::string("\"") + ptime.isoTime() + std::string("\"");
//  }
//  metaDataMap["time used"] = std::string("\"") + thetime + std::string("\"");
//  outputTextMaps["metadata"] = metaDataMap;
//  outputTextMapOrder.push_back("metadata");
}

static void ensureNewContext()
{
  if (!glpainter)
    return;

  bool was_printing = glpainter->printing;

  if (!multiple_plots) {
    if (glpainter->isPainting())
      glpainter->end();
    if (painter.isActive())
      painter.end();
  }

  glpainter->printing = was_printing;
}

static void printPage(int ox, int oy)
{
  pagePainter.drawPicture(ox, oy, picture);
}

void createPaintDevice()
{
  METLIBS_LOG_SCOPE();
  ensureNewContext();

  delete glpainter;
  delete glcanvas;

  bool printing = false;
  if (raster) {
    image = QImage(xsize, ysize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    glcanvas = new DiPaintGLCanvas(&image);

  } else if (pdf || svg || json) {
    picture = QPicture();
    picture.setBoundingRect(QRect(0, 0, xsize, ysize));
    glcanvas = new DiPaintGLCanvas(&picture);
    if (pdf)
      printing = true;

  } else { // Postscript

    picture = QPicture();
    picture.setBoundingRect(QRect(0, 0, xsize, ysize));
    glcanvas = new DiPaintGLCanvas(&picture);
    printing = true;
  }

  glpainter = new DiPaintGLPainter(glcanvas);
  glpainter->ShadeModel(DiGLPainter::gl_FLAT);
  glpainter->Viewport(0, 0, xsize, ysize);

  painter.begin(glcanvas->device());
  glpainter->begin(&painter);
  glpainter->printing = printing;
}

void subplot(int margin, int plotcol, int plotrow, int deltax, int deltay, int spacing)
{
  glpainter->Viewport(margin + plotcol * (deltax + spacing), ysize - (margin + (plotrow + 1) * (deltay + spacing)),
      deltax, deltay);
}

#define FIND_END_COMMAND(COMMAND) \
  for (int i = k + 1; i < linenum && miutil::to_lower(lines[i]) != COMMAND; i++, k++) \
    pcom.push_back(lines[i]); \
  k++;

static bool MAKE_CONTROLLER()
{
  if (main_controller)
    return true;

  main_controller = new Controller;

  const bool ps = main_controller->parseSetup();
  if (not ps) {
    METLIBS_LOG_ERROR("ERROR, an error occured while main_controller parsed setup: '" << setupfile << "'");
    return false;
  }

  main_controller->setCanvas(glcanvas);
  return true;
}


static int parseAndProcess(istream &is)
{
  ensureNewContext();

  // unpack loops, make lists, merge lines etc.
  int res = prepareInput(is);
  if (res != 0)
    return res;

  vector<std::string> extra_field_lines;

  int linenum = lines.size();

  // parse input - and perform plots
  for (int k = 0; k < linenum; k++) {// input-line loop
    // start parsing...
    if (miutil::to_lower(lines[k]) == com_vprof_opt) {
      vector<string> pcom;
      for (int i = k + 1; i < linenum && miutil::to_lower(lines[i])
          != com_vprof_opt_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;
      parse_vprof_options(pcom);
      continue;

    } else if (miutil::to_lower(lines[k]) == com_vcross_opt) {
      vector<string> pcom;
      for (int i = k + 1; i < linenum && miutil::to_lower(lines[i])
          != com_vcross_opt_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;

      if (!vcrossmanager)
        vcrossmanager = miutil::make_shared<vcross::QtManager>();
      vcross::VcrossQuickmenues::parse(vcrossmanager, pcom);

      continue;

    } else if (miutil::to_lower(lines[k]) == com_spectrum_opt) {
      vector<string> pcom;
      for (int i = k + 1; i < linenum && miutil::to_lower(lines[i])
          != com_spectrum_opt_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;
      parse_spectrum_options(pcom);
      continue;

    } else if (miutil::to_lower(lines[k]) == com_plot
        || miutil::to_lower(lines[k]) == com_vcross_plot
        || miutil::to_lower(lines[k]) == com_vprof_plot
        || miutil::to_lower(lines[k]) == com_spectrum_plot)
    {
      // --- START PLOT ---
      if (miutil::to_lower(lines[k]) == com_plot) {
        plottype = plot_standard;
        if (verbose)
          METLIBS_LOG_INFO("Preparing new standard-plot");

      } else if (miutil::to_lower(lines[k]) == com_vcross_plot) {
        plottype = plot_vcross;
        if (verbose)
          METLIBS_LOG_INFO("Preparing new vcross-plot");

      } else if (miutil::to_lower(lines[k]) == com_vprof_plot) {
        plottype = plot_vprof;
        if (verbose)
          METLIBS_LOG_INFO("Preparing new vprof-plot");

      } else if (miutil::to_lower(lines[k]) == com_spectrum_plot) {
        plottype = plot_spectrum;
        if (verbose)
          METLIBS_LOG_INFO("Preparing new spectrum-plot");
      }

      // if new plottype: make sure previous postscript-session is stopped
      if (prevplottype != plot_none && plottype != prevplottype) {
        endHardcopy(prevplottype);
      }
      prevplottype = plottype;

      if (multiple_plots && multiple_plottype != plot_none
          && hardcopy_started[multiple_plottype] && multiple_plottype
          != plottype) {
        METLIBS_LOG_ERROR("ERROR, you can not mix STANDARD/VCROSS/VPROF/SPECTRUM in multiple plots "
            << "..Exiting..");
        return 1;
      }
      multiple_plottype = plottype;

      if (multiple_plots && shape ) {
        METLIBS_LOG_ERROR("ERROR, you can not use shape option for multiple plots "
            << "..Exiting..");
        return 1;
      }
      if ( !(plottype == plot_none || plottype == plot_standard)  && shape ) {
        METLIBS_LOG_ERROR("ERROR, you can only use plottype STANDARD when using shape option"
            << "..Exiting..");
        return 1;
      }

      if (!buffermade) {
        METLIBS_LOG_ERROR("ERROR, no buffersize set..exiting");
        return 1;
      }
      if (!setupread) {
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          METLIBS_LOG_ERROR("ERROR, no setupinformation..exiting");
          return 99;
        }
      }

      vector<string> pcom;
      for (int i = k + 1; i < linenum && miutil::to_lower(lines[i]) != com_endplot
          && miutil::to_lower(lines[i]) != com_plotend; i++, k++) {
        if (shape ) {
                if ( (miutil::contains(lines[i], "OBS") || miutil::contains(lines[i], "SAT") || miutil::contains(lines[i], "OBJECTS") ||
                        miutil::contains(lines[i], "EDITFIELD") || miutil::contains(lines[i], "TRAJECTORY"))) {
                        METLIBS_LOG_ERROR("Error, Shape option cannot be used for OBS/OBJECTS/SAT/TRAJECTORY/EDITFIELD.. exiting");
                        return 1;
                }
                if ( miutil::contains(lines[i], "FIELD") ) {
                        std::string shapeFileName=priop.fname;
                        lines[i]+= " shapefilename=" + shapeFileName;
                        if ( miutil::contains(lines[i], "shapefile=0") )
                                miutil::replace(lines[i], "shapefile=0","shapefile=1");
                        else if (not miutil::contains(lines[i], "shapefile="))
                                lines[i]+=" shapefile=1";
                }
        }
        pcom.push_back(lines[i]);
      }
      k++;

      if (plottype == plot_standard) {
        // -- normal plot

        if (!multiple_plots)
          createPaintDevice();
        if (not MAKE_CONTROLLER())
          return 99;
        else {
          vector<std::string> field_errors;
          if (!main_controller->getFieldManager()->updateFileSetup(extra_field_lines, field_errors)) {
            METLIBS_LOG_ERROR("ERROR, an error occurred while adding new fields:");
            for (unsigned int kk = 0; kk < field_errors.size(); ++kk)
              METLIBS_LOG_ERROR(field_errors[kk]);
          }
          extra_field_lines.clear();
        }

        // turn on/off archive-mode (observations)
        main_controller->archiveMode(useArchive);

        if (verbose)
          METLIBS_LOG_INFO("- setPlotWindow");
        if (!multiple_plots)
          main_controller->setPlotWindow(xsize, ysize);
        else
          main_controller->setPlotWindow(deltax, deltay);

        // keeparea= false: use selected area or field/sat-area
        // keeparea= true : keep previous used area (if possible)
        main_controller->keepCurrentArea(keeparea);

        // necessary to set time before plotCommands()..?
        thetime = miTime::nowTime();
        main_controller->setPlotTime(thetime);

        if (verbose)
          METLIBS_LOG_INFO("- sending plotCommands");
        main_controller->plotCommands(pcom);

        thetime = selectTime();

        if (verbose)
          METLIBS_LOG_INFO("- plotting for time:" << thetime);
        main_controller->setPlotTime(thetime);

        //expand filename
        if (miutil::contains(priop.fname, "%")) {
          priop.fname = thetime.format(priop.fname);
        }

        if (verbose)
          METLIBS_LOG_INFO("- updatePlots");
        if (!main_controller->updatePlots() && failOnMissingData) {
            METLIBS_LOG_WARN("Failed to update plots.");
            ensureNewContext();
            return 99;
        }
        METLIBS_LOG_INFO("map area = " << main_controller->getMapArea());

        if (!raster && !shape && !json && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_standard, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
            startVideo(priop);
#endif
        }

        if (multiple_plots)
          subplot(margin, plotcol, plotrow, deltax, deltay, spacing);

        if (plot_trajectory && !trajectory_started) {
          vector<string> vstr;
          vstr.push_back("clear");
          vstr.push_back("delete");
          vstr.push_back(trajectory_options);
          main_controller->trajPos(vstr);
          //main_controller->trajTimeMarker(trajectory_timeMarker);
          main_controller->startTrajectoryComputation();
          trajectory_started = true;
        } else if (!plot_trajectory && trajectory_started) {
          vector<string> vstr;
          vstr.push_back("clear");
          vstr.push_back("delete");
          main_controller->trajPos(vstr);
          trajectory_started = false;
        }

        if (raster && antialias)
          glpainter->Enable(DiGLPainter::gl_MULTISAMPLE);

        if (verbose)
          METLIBS_LOG_INFO("- plot");

        if (plotAnnotationsOnly) {
          // Plotting annotations only for the purpose of returning legends to a WMS
          // server front end.
          if (raster) {
            annotationRectangles = main_controller->plotAnnotations(glpainter);
            annotationTransform = glpainter->transform;
          }
        } else
        {
          if (shape)
            main_controller->plot(glpainter, true, false);
          else if (!json)
            main_controller->plot(glpainter, true, true);
        }

        // Create JSON annotations irrespective of the value of plotAnnotationsOnly.
        if (json)
          createJsonAnnotation();

        // --------------------------------------------------------
      } else if (plottype == plot_vcross) {

        if (!multiple_plots)
          createPaintDevice();
        if (not MAKE_CONTROLLER())
          return 99;

        // -- vcross plot
        if (!vcrossmanager)
          vcrossmanager = miutil::make_shared<vcross::QtManager>();
        else
          vcrossmanager->cleanup(); // needed to clear zoom history

        // set size of plotwindow
        if (!multiple_plots)
          vcrossmanager->setPlotWindow(xsize, ysize);
        else
          vcrossmanager->setPlotWindow(deltax, deltay);

        if (verbose)
          METLIBS_LOG_INFO("- sending vcross plot commands");
        vcross::VcrossQuickmenues::parse(vcrossmanager, pcom);

        if (ptime.undef()) {
          thetime = vcrossmanager->getTimeValue();
          if (verbose)
            METLIBS_LOG_INFO("VCROSS has default time:" << thetime);
        } else
          thetime = ptime;
        if (verbose)
          METLIBS_LOG_INFO("- plotting for time:" << thetime);
        vcrossmanager->setTimeToBestMatch(thetime);

        //expand filename
        if (miutil::contains(priop.fname, "%")) {
          priop.fname = thetime.format(priop.fname);
        }

        if (!raster && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_vcross, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
          startVideo(priop);
#endif
        }

        if (multiple_plots)
          subplot(margin, plotcol, plotrow, deltax, deltay, spacing);

        if (verbose)
          METLIBS_LOG_INFO("- plot");

        if (raster && antialias)
          glpainter->Enable(DiGLPainter::gl_MULTISAMPLE);

        vcrossmanager->plot(painter);

        // --------------------------------------------------------
      } else if (plottype == plot_vprof) {

        if (!multiple_plots)
          createPaintDevice();
        if (not MAKE_CONTROLLER())
          return 99;

        // -- vprof plot
        if (!vprofmanager) {
          vprofmanager = new VprofManager();
          vprofmanager->init();
        }

        // set size of plotwindow
        if (!multiple_plots)
          vprofmanager->setPlotWindow(xsize, ysize);
        else
          vprofmanager->setPlotWindow(deltax, deltay);

        // extract options for plot
        parse_vprof_options(pcom);

        if (verbose)
          METLIBS_LOG_INFO("- sending plotCommands");
        if (vprof_optionschanged)
          vprofmanager->getOptions()->readOptions(vprof_options);
        vprof_optionschanged = false;
        vprofmanager->setSelectedModels(vprof_models, vprof_plotobs);
        vprofmanager->setModel();

        if (ptime.undef()) {
          thetime = vprofmanager->getTime();
          if (verbose)
            METLIBS_LOG_INFO("VPROF has default time:" << thetime);
        } else
          thetime = ptime;
        if (verbose)
          METLIBS_LOG_INFO("- plotting for time:" << thetime);
        vprofmanager->setTime(thetime);

        //expand filename
        if (miutil::contains(priop.fname, "%")) {
          priop.fname = thetime.format(priop.fname);
        }

        if (verbose)
          METLIBS_LOG_INFO("- setting station:" << vprof_stations.size());
        if (vprof_stations.size())
          vprofmanager->setStations(vprof_stations);

        if (!raster && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_vprof, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
          startVideo(priop);
#endif
        }

        if (multiple_plots)
          subplot(margin, plotcol, plotrow, deltax, deltay, spacing);

        if (verbose)
          METLIBS_LOG_INFO("- plot");

        if (raster && antialias)
          glpainter->Enable(DiGLPainter::gl_MULTISAMPLE);
        vprofmanager->plot(glpainter);

        // --------------------------------------------------------
      } else if (plottype == plot_spectrum) {

        if (!multiple_plots)
          createPaintDevice();

        // -- spectrum plot
        if (!spectrummanager) {
          spectrummanager = new SpectrumManager;
        }

        // set size of plotwindow
        if (!multiple_plots)
          spectrummanager->setPlotWindow(xsize, ysize);
        else
          spectrummanager->setPlotWindow(deltax, deltay);

        // extract options for plot
        parse_spectrum_options(pcom);

        if (verbose)
          METLIBS_LOG_INFO("- sending plotCommands");
        if (spectrum_optionschanged)
          spectrummanager->getOptions()->readOptions(spectrum_options);
        spectrum_optionschanged = false;
        spectrummanager->setSelectedModels(spectrum_models);
        spectrummanager->setModel();

        if (ptime.undef()) {
          thetime = spectrummanager->getTime();
          if (verbose)
            METLIBS_LOG_INFO("SPECTRUM has default time:" << thetime);
        } else
          thetime = ptime;
        if (verbose)
          METLIBS_LOG_INFO("- plotting for time:" << thetime);
        spectrummanager->setTime(thetime);

        //expand filename
        if (miutil::contains(priop.fname, "%")) {
          priop.fname = thetime.format(priop.fname);
        }

        if (verbose)
          METLIBS_LOG_INFO("- setting station:" << spectrum_station);
        if (not spectrum_station.empty())
          spectrummanager->setStation(spectrum_station);

        if (!raster && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_spectrum, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
          startVideo(priop);
#endif
        }

        if (multiple_plots)
          subplot(margin, plotcol, plotrow, deltax, deltay, spacing);

        if (verbose)
          METLIBS_LOG_INFO("- plot");

        if (raster && antialias)
          glpainter->Enable(DiGLPainter::gl_MULTISAMPLE);
        spectrummanager->plot(glpainter);

      }
      // --------------------------------------------------------
      // Write output to a file.
      // --------------------------------------------------------

      if (raster) {

        if (verbose)
          METLIBS_LOG_INFO("- Preparing for raster output");
        glpainter->Flush();

        // QWS/QPA output
        if (raster) {
          ensureNewContext();

          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
            image = image.copy(-ox, -oy, xsize, ysize);
            plotAnnotationsOnly = false;
          }

          // Add the input file text as meta-data in the image.
          for (unsigned int i = 0; i < lines.size(); ++i)
            image.setText(QString::number(i), QString::fromStdString(lines[i]));

          image.save(QString::fromStdString(priop.fname));
#if 0
          //milogger::LogHandler::getInstance()->setObjectName("diana.bdiana.parseAndProcess");

          bool empty = true;
          for (int py = 0; py < image.height(); ++py) {
            QRgb *scanLine = (QRgb*)(image.scanLine(py));
            for (int px = 0; px < image.width(); ++px) {
              if (qAlpha(scanLine[px]) != 0) {
                empty = false;
                break;
              }
            }
            if (!empty)
              break;
          }

          if (empty)
            METLIBS_LOG_INFO("# vvv Empty plot (begin)");

          // Write the input file text to the log.
          for (unsigned int i = 0; i < lines.size(); ++i)
            METLIBS_LOG_INFO(lines[i]);

          if (empty)
            METLIBS_LOG_INFO("# ^^^ Empty plot (end)");
#endif
        }
        else {
          imageIO::Image_data img;
          img.width = xsize;
          img.height = ysize;
          img.filename = priop.fname;
          int npixels;

          npixels = img.width * img.height;
          img.nchannels = 4;
          img.data = new unsigned char[npixels * img.nchannels];

          glpainter->ReadPixels(0, 0, img.width, img.height, DiGLPainter::gl_RGBA, DiGLPainter::gl_UNSIGNED_BYTE,
              img.data);

          int result = 0;

          // save as PNG -----------------------------------------------
          if (raster_type == image_png || raster_type == image_unknown) {
            if (verbose) {
              METLIBS_LOG_INFO("- Saving PNG-image to: '" << img.filename << "'");
            }
            result = imageIO::write_png(img);
#ifdef VIDEO_EXPORT
          } else if (raster_type == image_avi) {
            if (verbose) {
              METLIBS_LOG_INFO("- Adding image to: '" << img.filename << "'");
            }
//            result = addVideoFrame(img);
#endif
          }
          if (verbose)
            METLIBS_LOG_INFO(" .." << std::string(result ? "Ok" : " **FAILED!**"));
          else if (!result)
            METLIBS_LOG_ERROR(" ERROR, saving PNG-image to: '" << img.filename << "'");
          // -------------------------------------------------------------

        }

      } else if (shape) { // Only shape output

          if (miutil::contains(priop.fname, "tmp_diana")) {
            METLIBS_LOG_INFO("Using shape option without file name, it will be created automatically");
          } else {
            METLIBS_LOG_INFO("Using shape option with given file name: '" << priop.fname << "'");
          }
          // first stop postscript-generation
          endHardcopy(plot_none);

          // Anything more to be done here ???

      } else if (svg) {

          ensureNewContext();

          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
          }

          // For some reason, QPrinter can determine the correct resolution to use, but
          // QSvgGenerator cannot manage that on its own, so we take the resolution from
          // a QPrinter instance which we do not otherwise use.
          QPrinter sprinter;
          QSvgGenerator svgFile;
          svgFile.setFileName(QString::fromStdString(priop.fname));
          svgFile.setSize(QSize(xsize, ysize));
          svgFile.setViewBox(QRect(0, 0, xsize, ysize));
          svgFile.setResolution(sprinter.resolution());
          painter.begin(&svgFile);
          painter.drawPicture(ox, oy, picture);
          painter.end();

      } else if (pdf) {

        ensureNewContext();

        if (!multiple_plots) {
          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
          }

          printPage(ox, oy);
        }

      } else if (json) {

        ensureNewContext();

        QFile outputFile(QString::fromStdString(priop.fname));
        if (outputFile.open(QFile::WriteOnly)) {
          outputFile.write("{\n");

          unsigned int i = 0;
          for (vector<std::string>::iterator iti = outputTextMapOrder.begin(); iti != outputTextMapOrder.end(); ++iti, ++i) {
            outputFile.write("  \"");
            outputFile.write(QString::fromStdString(*iti).toUtf8());
            outputFile.write("\": {\n");
            map<std::string,std::string> textMap = outputTextMaps[*iti];
            unsigned int j = 0;
            for (map<std::string,std::string>::iterator itj = textMap.begin(); itj != textMap.end(); ++itj, ++j) {
              outputFile.write("    \"");
              outputFile.write(QString::fromStdString(itj->first).toUtf8());
              outputFile.write("\": ");
              outputFile.write(QString::fromStdString(itj->second).toUtf8());
              if (j != textMap.size() - 1)
                outputFile.write(",");
              outputFile.write("\n");
            }
            outputFile.write("  }");
            if (i != outputTextMaps.size() - 1)
              outputFile.write(",");
            outputFile.write("\n");
          }
          outputFile.write("}\n");
          outputFile.close();
        }
      } else { // Postscript

        ensureNewContext();

        if (!multiple_plots) {
          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
          }

          printPage(ox, oy);
        }
      }
      continue;

    } else if (miutil::to_lower(lines[k]) == com_time || miutil::to_lower(lines[k])
        == com_level) {

      // read setup
      if (!setupread) {
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          METLIBS_LOG_ERROR("ERROR, no setupinformation..exiting");
          return 99;
        }
      }

      if (not MAKE_CONTROLLER())
        return 99;

      if (miutil::to_lower(lines[k]) == com_time) {

        if (verbose)
          METLIBS_LOG_INFO("- finding times");

        //Find ENDTIME
        vector<string> pcom;
        FIND_END_COMMAND(com_endtime)

        // necessary to set time before plotCommands()..?
        thetime = miTime::nowTime();
        main_controller->setPlotTime(thetime);

        if (verbose)
          METLIBS_LOG_INFO("- sending plotCommands");
        main_controller->plotCommands(pcom);

        set<miTime> okTimes;
        main_controller->getCapabilitiesTime(okTimes, pcom, time_options == "union", true);

        // open filestream
        ofstream file(priop.fname.c_str());
        if (!file) {
          METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << priop.fname << "'");
          return 1;
        }
        file << "PROG" << endl;
        set<miTime>::iterator p = okTimes.begin();
        for (; p != okTimes.end(); p++) {
          file << (*p).format(time_format) << endl;
        }
        file.close();

      } else if (miutil::to_lower(lines[k]) == com_level) {

        if (verbose)
          METLIBS_LOG_INFO("- finding levels");

        //Find ENDLEVEL
        vector<std::string> pcom;
        for (int i = k + 1; i < linenum && miutil::to_lower(lines[i]) != com_endlevel; i++, k++)
          pcom.push_back(lines[i]);
        k++;

        vector<string> levels;

        // open filestream
        ofstream file(priop.fname.c_str());
        if (!file) {
          METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << priop.fname << "'");
          return 1;
        }

        for (unsigned int i = 0; i < pcom.size(); i++) {
          levels = main_controller->getFieldLevels(pcom[i]);

          for (unsigned int j = 0; j < levels.size(); j++) {
            file << levels[j] << endl;
          }
          file << endl;
        }

        file.close();

      }

      continue;

    } else if (miutil::to_lower(lines[k]) == com_time_vprof) {

      if (verbose)
        METLIBS_LOG_INFO("- finding times");

      //Find ENDTIME
      vector<string> pcom;
      FIND_END_COMMAND(com_endtime)

      if (!vprofmanager) {
        vprofmanager = new VprofManager();
        vprofmanager->init();
      }
      // extract options for plot
      parse_vprof_options(pcom);

      if (vprof_optionschanged)
        vprofmanager->getOptions()->readOptions(vprof_options);

      vprof_optionschanged = false;
      vprofmanager->setSelectedModels(vprof_models, vprof_plotobs);
      vprofmanager->setModel();

      vector<miTime> okTimes = vprofmanager->getTimeList();

      // open filestream
      ofstream file(priop.fname.c_str());
      if (!file) {
        METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << priop.fname << "'");
        return 1;
      }
      file << "PROG" << endl;
      vector<miTime>::iterator p = okTimes.begin();
      for (; p != okTimes.end(); p++) {
        file << (*p).format(time_format) << endl;
      }
      file.close();

      continue;

    } else if (miutil::to_lower(lines[k]) == com_time_spectrum) {

      // read setup
      if (!setupread) {
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          METLIBS_LOG_ERROR("ERROR, no setupinformation..exiting");
          return 99;
        }
      }

      if (not MAKE_CONTROLLER())
        return 99;

      if (verbose)
        METLIBS_LOG_INFO("- finding times");

      //Find ENDTIME
      vector<string> pcom;
      FIND_END_COMMAND(com_endtime)

      if (!spectrummanager)
        spectrummanager = new SpectrumManager;

      // extract options for plot
      parse_spectrum_options(pcom);

      if (spectrum_optionschanged)
        spectrummanager->getOptions()->readOptions(spectrum_options);

      spectrum_optionschanged = false;
      spectrummanager->setSelectedModels(spectrum_models);
      spectrummanager->setModel();

      if (ptime.undef()) {
        thetime = spectrummanager->getTime();
        if (verbose)
          METLIBS_LOG_INFO("SPECTRUM has default time:" << thetime);
      } else
        thetime = ptime;
      if (verbose)
        METLIBS_LOG_INFO("- describing spectrum for time:" << thetime);
      spectrummanager->setTime(thetime);

      if (verbose)
        METLIBS_LOG_INFO("- setting station: '" << spectrum_station << "'");
      if (not spectrum_station.empty())
        spectrummanager->setStation(spectrum_station);

      vector<miTime> okTimes = spectrummanager->getTimeList();
      set<miTime> constTimes;

      // open filestream
      ofstream file(priop.fname.c_str());
      if (!file) {
        METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << priop.fname << "'");
        return 1;
      }
      file << "PROG" << endl;
      vector<miTime>::iterator p = okTimes.begin();
      for (; p != okTimes.end(); p++) {
        file << (*p).format(time_format) << endl;
      }
      file << "CONST" << endl;
/*      p = constTimes.begin();
      for (; p != constTimes.end(); p++) {
        file << (*p).format(time_format) << endl;
      }*/
      file.close();

      continue;

    } else if (miutil::to_lower(lines[k]) == com_print_document) {
      if (raster) {
        METLIBS_LOG_ERROR(" ERROR, trying to print raster-image!");
        continue;
      }
      if (priop.printer.empty()) {
        METLIBS_LOG_ERROR(" ERROR, printing document: '" << priop.fname
               << "'  Printer not defined!");
        continue;
      }
      // first stop postscript-generation
      endHardcopy(plot_none);
      multiple_newpage = true;

      std::string command = printman->printCommand();
      priop.numcopies = 1;

      printman->expandCommand(command, priop);

      if (verbose)
        METLIBS_LOG_INFO("- Issuing print command: '" << command << "'");
      int res = system(command.c_str());
      if (verbose)
        METLIBS_LOG_INFO("Result:" << res);

      continue;

    } else if (miutil::to_lower(lines[k]) == com_wait_for_commands) {
      /*
       ====================================
       ========= TEST - feed commands from files
       ====================================
       */

      if (command_path.empty()) {
        METLIBS_LOG_ERROR("ERROR, wait_for_commands found, but command_path not set");
        continue;
      }
      static int prev_iclock = -1;
      int iclock;
      float diff = 0;
      miTime nowtime = miTime::nowTime();

      // using clock-cycle-command
      iclock = clock();
      if (prev_iclock > 0)
        diff = float(iclock - prev_iclock) / float(CLOCKS_PER_SEC);

      METLIBS_LOG_INFO("================ WAIT FOR COMMANDS, TIME is:" << nowtime
            << ", seconds spent on previous command(s):" << diff);

      std::string pattern = command_path;
      vector<std::string> newlines;
      std::string waitline = com_wait_for_commands;

      diutil::string_v matches;
      while ((matches = diutil::glob(pattern)).empty())
        pu_sleep(1);

      nowtime = miTime::nowTime();
      prev_iclock = clock();
      METLIBS_LOG_INFO("================ FOUND COMMAND-FILE(S), TIME is:" << nowtime);

      vector<std::string> filenames;

      //loop over files
      for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it) {
        const std::string& filename = *it;
        METLIBS_LOG_INFO("==== Reading file: '" << filename << "'");
        filenames.push_back(filename);
        ifstream file(filename.c_str());
        while (file) {
          std::string str;
          if (getline(file, str)) {
            miutil::trim(str);
            if (str.length() > 0 && str[0] != '#') {
              if (miutil::contains(miutil::to_lower(str), com_wait_end))
                waitline = ""; // blank out waitline
              else
                newlines.push_back(str);
            }
          }
        }
      }
      // remove processed files
      for (unsigned int ik = 0; ik < filenames.size(); ik++) {
        ostringstream ost;
        ost << "rm -f " << filenames[ik];
        std::string command = ost.str();

        METLIBS_LOG_INFO("==== Cleaning up with: '" << command << "'");
        int res = system(command.c_str());

        if (res != 0){
          METLIBS_LOG_WARN("Command:" << command << " failed");
        }
      }
      // add new wait-command
      if (waitline.size() > 0)
        newlines.push_back(waitline);
      // insert commandlines into the command-queue
      lines.erase(lines.begin() + k, lines.begin() + k + 1);
      lines.insert(lines.begin() + k, newlines.begin(), newlines.end());
      linenum = lines.size();
      k--;

      METLIBS_LOG_INFO("================ EXECUTING COMMANDS");
      continue;
      // =============================================================

    } else if (miutil::to_lower(lines[k]) == com_field_files) {
      // Read lines until </FIELD_FILES> is read.

      int kk;
      for (kk = k + 1; kk < linenum; ++kk) {
        if (miutil::to_lower(lines[kk]) != com_field_files_end)
           extra_field_lines.push_back(lines[kk]);
        else
          break;
      }

      if (kk < linenum)
        k = kk;         // skip the </FIELD_FILES> line
      else {
          METLIBS_LOG_ERROR("ERROR, no " << com_field_files_end << " found:" << lines[k]
                 << " Linenumber:" << linenumbers[k]);
        return 1;
      }
      continue;

    } else if (miutil::to_lower(lines[k]) == com_field_files_end) {
      METLIBS_LOG_WARN("WARNING, " << com_field_files_end << " found:" << lines[k]
            << " Linenumber:" << linenumbers[k]);
      continue;

    } else if (miutil::to_lower(lines[k]) == com_describe) {

      if (verbose)
        METLIBS_LOG_INFO("- finding information about data sources");

      //Find ENDDESCRIBE
      vector<string> pcom;
      FIND_END_COMMAND(com_describe_end)

      if (not MAKE_CONTROLLER())
        return 99;

      if (verbose)
        METLIBS_LOG_INFO("- sending plotCommands");
      main_controller->plotCommands(pcom);

      thetime = selectTime();

      if (verbose)
        METLIBS_LOG_INFO("- describing field for time: " << thetime);
      main_controller->setPlotTime(thetime);

      if (verbose)
        METLIBS_LOG_INFO("- updatePlots");

      if (main_controller->updatePlots() || !failOnMissingData) {

          if (verbose)
            METLIBS_LOG_INFO("- opening file '" << priop.fname << "'");

          // open filestream
          ofstream file(priop.fname.c_str());
          if (!file) {
            METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << priop.fname << "'");
            return 1;
          }

          vector<FieldPlot*> fieldPlots = main_controller->getFieldPlots();
          std::set<std::string> fieldPatterns;

          FieldManager* fieldManager = main_controller->getFieldManager();
          for (vector<FieldPlot*>::iterator it = fieldPlots.begin(); it != fieldPlots.end(); ++it) {
            std::string modelName = (*it)->getModelName();
            std::vector<std::string> fileNames = fieldManager->getFileNames(modelName);
            fieldPatterns.insert(fileNames.begin(), fileNames.end());
          }

          const SatManager::Prod_t& satProducts = main_controller->getSatelliteManager()->getProductsInfo();
          set<std::string> satPatterns;
          set<std::string> satFiles;

          const vector<SatPlot*>& satellitePlots = main_controller->getSatellitePlots();
          for (vector<SatPlot*>::const_iterator it = satellitePlots.begin(); it != satellitePlots.end(); ++it) {
            Sat* sat = (*it)->satdata;
            const SatManager::Prod_t::const_iterator itp = satProducts.find(sat->satellite);
            if (itp != satProducts.end()) {
              const SatManager::SubProd_t::const_iterator itsp = itp->second.find(sat->satellite);
              if (itsp != itp->second.end()) {
                const SatManager::subProdInfo& satInfo = itsp->second;
                for (vector<SatFileInfo>::const_iterator itsf = satInfo.file.begin(); itsf != satInfo.file.end(); ++itsf) {
                  satFiles.insert(itsf->name);
                  if (itsf->name == sat->actualfile) {
                    for (vector<string>::const_iterator itp = satInfo.pattern.begin(); itp != satInfo.pattern.end(); ++itp)
                      satPatterns.insert(*itp);
                  }
                }
              }
            }
          }

          map<string,ObsManager::ProdInfo> obsProducts = main_controller->getObservationManager()->getProductsInfo();
          set<std::string> obsPatterns;
          set<std::string> obsFiles;

          vector<ObsPlot*> obsPlots = main_controller->getObsPlots();
          for (vector<ObsPlot*>::iterator it = obsPlots.begin(); it != obsPlots.end(); ++it) {
              vector<string> obsFileNames = (*it)->getFileNames();
              for (vector<string>::iterator itf = obsFileNames.begin(); itf != obsFileNames.end(); ++itf) {
                  obsFiles.insert(*itf);

                  for (map<string,ObsManager::ProdInfo>::iterator ito = obsProducts.begin(); ito != obsProducts.end(); ++ito) {
                      for (vector<ObsManager::FileInfo>::iterator itof = ito->second.fileInfo.begin(); itof != ito->second.fileInfo.end(); ++itof) {
                          if (*itf == itof->filename) {
                              for (vector<ObsManager::patternInfo>::iterator itp = ito->second.pattern.begin(); itp != ito->second.pattern.end(); ++itp)
                                  obsPatterns.insert(itp->pattern);
                          }

                      }
                  }
              }
          }

          file << "FILES" << endl;
          for (std::set<std::string>::iterator it = fieldPatterns.begin(); it != fieldPatterns.end(); ++it)
            file << *it << endl;
          for (set<std::string>::iterator it = satFiles.begin(); it != satFiles.end(); ++it)
            file << *it << endl;
          for (set<std::string>::iterator it = satPatterns.begin(); it != satPatterns.end(); ++it)
            file << *it << endl;
          for (set<std::string>::iterator it = obsFiles.begin(); it != obsFiles.end(); ++it)
            file << *it << endl;
          for (set<std::string>::iterator it = obsPatterns.begin(); it != obsPatterns.end(); ++it)
            file << *it << endl;

          file.close();
      }

      continue;

    } else if (miutil::to_lower(lines[k]) == com_describe_spectrum) {

      if (verbose)
        METLIBS_LOG_INFO("- finding information about data sources");

      //Find ENDDESCRIBE
      vector<string> pcom;
      FIND_END_COMMAND(com_describe_end)

      if (not MAKE_CONTROLLER())
        return 99;

      if (!spectrummanager)
        spectrummanager = new SpectrumManager;

      // extract options for plot
      parse_spectrum_options(pcom);

      if (verbose)
        METLIBS_LOG_INFO("- sending plotCommands");

      if (spectrum_optionschanged)
        spectrummanager->getOptions()->readOptions(spectrum_options);

      spectrum_optionschanged = false;
      spectrummanager->setSelectedModels(spectrum_models);
      spectrummanager->setModel();

      if (ptime.undef()) {
        thetime = spectrummanager->getTime();
        if (verbose)
          METLIBS_LOG_INFO("SPECTRUM has default time:" << thetime);
      } else
        thetime = ptime;
      if (verbose)
        METLIBS_LOG_INFO("- describing spectrum for time:" << thetime);
      spectrummanager->setTime(thetime);

      if (verbose)
        METLIBS_LOG_INFO("- setting station: '" << spectrum_station << "'");
      if (not spectrum_station.empty())
        spectrummanager->setStation(spectrum_station);

      if (verbose)
        METLIBS_LOG_INFO("- opening file '" << priop.fname << "'");

      // open filestream
      ofstream file(priop.fname.c_str());
      if (!file) {
        METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << priop.fname << "'");
        return 1;
      }

      vector<std::string> modelFiles = spectrummanager->getModelFiles();
      file << "FILES" << endl;
      for (vector<std::string>::const_iterator it = modelFiles.begin(); it != modelFiles.end(); ++it)
        file << *it << endl;

      file.close();

      continue;

    } else if (miutil::to_lower(lines[k]) == com_describe_end) {
      METLIBS_LOG_ERROR("WARNING, " << com_describe_end << " found:" << lines[k]
             << " Linenumber:" << linenumbers[k]);
      continue;
    }

    // all other options on the form KEY=VALUE

    vs = miutil::split(lines[k], "=");
    int nv = vs.size();
    if (nv < 2) {
      METLIBS_LOG_ERROR("ERROR, unknown command:" << lines[k] << " Linenumber:"
             << linenumbers[k]);
      return 1;
    }
    std::string key = miutil::to_lower(vs[0]);
    int ieq = lines[k].find_first_of("=");
    std::string value = lines[k].substr(ieq + 1, lines[k].length() - ieq - 1);
    miutil::trim(key);
    miutil::trim(value);

    if (key == com_setupfile) {
      if (setupread) {
        METLIBS_LOG_WARN("WARNING, setupfile overrided by command line option. Linenumber:"
            << linenumbers[k]);
        //      return 1;
      } else {
        setupfile = value;
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          METLIBS_LOG_ERROR("ERROR, no setupinformation..exiting");
          return 99;
        }
      }

    } else if (key == com_command_path) {
      command_path = value;

    } else if (key == com_fifo_name) {
      fifo_name = value;

    } else if (key == com_buffersize) {
      vvs = miutil::split(value, "x");
      if (vvs.size() < 2) {
        METLIBS_LOG_ERROR("ERROR, buffersize should be WxH:" << lines[k]
               << " Linenumber:" << linenumbers[k]);
        return 1;
      }
      int tmp_xsize = atoi(vvs[0].c_str());
      int tmp_ysize = atoi(vvs[1].c_str());

      // if requested buffersize identical to current: do nothing
      if (buffermade && tmp_xsize == xsize && tmp_ysize == ysize)
        continue;

      xsize = tmp_xsize;
      ysize = tmp_ysize;

      // first stop ongoing postscript sessions
      endHardcopy(plot_none);

      // for multiple plots
      priop.viewport_x0 = 0;
      priop.viewport_y0 = 0;
      priop.viewport_width = xsize;
      priop.viewport_height = ysize;

      buffermade = true;

    } else if (key == com_papersize) {
      vvvs = miutil::split(value, ","); // could contain both pagesize and papersize
      for (unsigned int l = 0; l < vvvs.size(); l++) {
        if (miutil::contains(vvvs[l], "x")) {
          vvs = miutil::split(vvvs[l], "x");
          if (vvs.size() < 2) {
            METLIBS_LOG_ERROR("ERROR, papersize should be WxH or WxH,PAPERTYPE or PAPERTYPE:"
                << lines[k] << " Linenumber:" << linenumbers[k]);
            return 1;
          }
          priop.papersize.hsize = atoi(vvs[0].c_str());
          priop.papersize.vsize = atoi(vvs[1].c_str());
          priop.usecustomsize = true;
        } else {
          priop.pagesize = printman->getPage(vvvs[l]);
        }
      }

    } else if (key == com_filename) {
      if (value.empty()) {
        METLIBS_LOG_ERROR("ERROR, illegal filename in:" << lines[k] << " Linenumber:"
               << linenumbers[k]);
        return 1;
      } else
        priop.fname = value;

    } else if (key == com_toprinter) {
      toprinter = (miutil::to_lower(value) == "yes");

    } else if (key == com_printer) {
      priop.printer = value;

    } else if (key == com_output) {
      value = miutil::to_lower(value);
      raster = false;
      shape = false;
      json = false;
      postscript = false;
      svg = false;
      pdf = false;
      if (value == "postscript") {
        postscript = true;
        raster = false;
        priop.doEPS = false;
      } else if (value == "eps") {
        raster = false;
        priop.doEPS = true;
      } else if (value == "png" || value == "raster") {
        raster = true;
        if (value == "png")
          raster_type = image_png;
        else
          raster_type = image_unknown;

      } else if (value == "shp") {
        shape = true;
      } else if (value == "avi") {
        raster = true;
        raster_type = image_avi;

      } else if (value == "pdf" || value == "svg" || value == "json") {
        raster = false;
        if (value == "pdf")
            pdf = true;
        else if (value == "svg")
            svg = true;
        else {
            json = true;
            outputTextMaps.clear();
            outputTextMapOrder.clear();
        }
      } else {
        METLIBS_LOG_ERROR("ERROR, unknown output-format:" << lines[k] << " Linenumber:"
               << linenumbers[k]);
        return 1;
      }

      if (raster || shape) {
        // first stop ongoing postscript sessions
        endHardcopy(plot_none);
      }

    } else if (key == com_colour) {
      if (miutil::to_lower(value) == "greyscale")
        priop.colop = d_print::greyscale;
      else
        priop.colop = d_print::incolour;

    } else if (key == com_drawbackground) {
      priop.drawbackground = (miutil::to_lower(value) == "yes");

    } else if (key == com_orientation) {
      value = miutil::to_lower(value);
      if (value == "landscape")
        priop.orientation = d_print::ori_landscape;
      else if (value == "portrait")
        priop.orientation = d_print::ori_portrait;
      else
        priop.orientation = d_print::ori_automatic;

    } else if (key == com_addhour) {
      if (!fixedtime.undef()) {
        ptime = fixedtime;
        ptime.addHour(atoi(value.c_str()));
      }

    } else if (key == com_addminute) {
      if (!fixedtime.undef()) {
        ptime = fixedtime;
        ptime.addMin(atoi(value.c_str()));
      }

    } else if (key == com_settime) {
      if ( value == "nowtime" || value == "current" ) {
        use_nowtime = true;
      } else if ( value == "firsttime" ) {
          use_firsttime = true;
      } else if (miTime::isValid(value)) {
        fixedtime = ptime = miTime(value);
      }

    } else if (key == com_archive) {
      useArchive = (miutil::to_lower(value) == "on");

    } else if (key == com_keepplotarea) {
      keeparea = (miutil::to_lower(value) == "yes");

    } else if (key == com_plotannotationsonly) {
      plotAnnotationsOnly = (miutil::to_lower(value) == "yes");

    } else if (key == com_antialiasing) {
      antialias = (miutil::to_lower(value) == "yes");

    } else if (key == com_fail_on_missing_data) {
      failOnMissingData = (miutil::to_lower(value) == "yes");

    } else if (key == com_multiple_plots) {
      if (miutil::to_lower(value) == "off") {
        multiple_newpage = false;
        multiple_plots = false;
        endHardcopy(plot_none);
        glpainter->Viewport(0, 0, xsize, ysize);

      } else {
        if (multiple_plots) {
          METLIBS_LOG_ERROR("Multiple plots are already enabled at line " << linenumbers[k]);
          endHardcopy(plot_none);
          if (printer && pagePainter.isActive()) {
            pagePainter.end();
            delete printer;
          }
        }
        vector<std::string> v1 = miutil::split(value, ",");
        if (v1.size() < 2) {
          METLIBS_LOG_WARN("WARNING, illegal values to multiple.plots:" << lines[k]
                << " Linenumber:" << linenumbers[k]);
          multiple_plots = false;
          return 1;
        }
        numrows = atoi(v1[0].c_str());
        numcols = atoi(v1[1].c_str());
        if (numrows < 1 || numcols < 1) {
          METLIBS_LOG_WARN("WARNING, illegal values to multiple.plots:" << lines[k]
                << " Linenumber:" << linenumbers[k]);
          multiple_plots = false;
          return 1;
        }
        float fmargin = 0.0;
        float fspacing = 0.0;
        if (v1.size() > 2) {
          fspacing = atof(v1[2].c_str());
          if (fspacing >= 100 || fspacing < 0) {
            METLIBS_LOG_WARN("WARNING, illegal value for spacing:" << lines[k]
                  << " Linenumber:" << linenumbers[k]);
            fspacing = 0;
          }
        }
        if (v1.size() > 3) {
          fmargin = atof(v1[3].c_str());
          if (fmargin >= 100 || fmargin < 0) {
            METLIBS_LOG_WARN("WARNING, illegal value for margin:" << lines[k]
                  << " Linenumber:" << linenumbers[k]);
            fmargin = 0;
          }
        }
        margin = int(xsize * fmargin / 100.0);
        spacing = int(xsize * fspacing / 100.0);
        deltax = (xsize - 2 * margin - (numcols - 1) * spacing) / numcols;
        deltay = (ysize - 2 * margin - (numrows - 1) * spacing) / numrows;
        multiple_plots = true;
        multiple_newpage = true;
        plotcol = plotrow = 0;
        if (verbose)
          METLIBS_LOG_INFO("Starting multiple_plot, rows:" << numrows << " , columns: "
                << numcols);

        // A new multiple plot needs a new paint device to be created.
        createPaintDevice();
      }

    } else if (key == com_plotcell) {
      if (!multiple_plots) {
        METLIBS_LOG_ERROR("ERROR, multiple plots not initialised:" << lines[k]
               << " Linenumber:" << linenumbers[k]);
        return 1;
      } else {
        vector<std::string> v1 = miutil::split(value, ",");
        if (v1.size() != 2) {
          METLIBS_LOG_WARN("WARNING, illegal values to plotcell:" << lines[k]
                << " Linenumber:" << linenumbers[k]);
          return 1;
        }
        plotrow = atoi(v1[0].c_str());
        plotcol = atoi(v1[1].c_str());
        if (plotrow < 0 || plotrow >= numrows || plotcol < 0 || plotcol
            >= numcols) {
          METLIBS_LOG_WARN("WARNING, illegal values to plotcell:" << lines[k]
                << " Linenumber:" << linenumbers[k]);
          return 1;
        }
        // row 0 should be on top of page
        plotrow = (numrows - 1 - plotrow);
      }

    } else if (key == com_trajectory) {
      if (miutil::to_lower(value) == "on") {
        plot_trajectory = true;
        trajectory_started = false;
      } else {
        plot_trajectory = false;
      }

    } else if (key == com_trajectory_opt) {
      trajectory_options = value;

    } else if (key == com_trajectory_print) {
      main_controller->printTrajectoryPositions(value);

    } else if (key == com_time_opt) {
      time_options = miutil::to_lower(value);

    } else if (key == com_time_format) {
      time_format = value;

    } else {
      METLIBS_LOG_WARN("WARNING, unknown command:" << lines[k] << " Linenumber:"
            << linenumbers[k]);
    }
  }

  // finish off any dangling postscript-sessions
  endHardcopy(plot_none);
  if (printer && pagePainter.isActive()) {
    pagePainter.end();
    delete printer;
  }

  return 0;
}

/*
 * public C api of above parseAndProcessString
 */
int diana_parseAndProcessString(const char* string)
{
    stringstream ss;
    ss << string;
    METLIBS_LOG_INFO("start processing");
    miTime undef;
    ptime=fixedtime=undef;
    int retVal = parseAndProcess(ss);
    if (retVal == 0) return DIANA_OK;

    return DIANA_ERROR;
}


/*
 SIGNAL HANDLING ROUTINES
 */

static int dispatchWork(const std::string &file)
{
  METLIBS_LOG_INFO("Reading input file: '" << file << "'");

  // commands in file
  ifstream is(file.c_str());
  if (!is) {
          METLIBS_LOG_ERROR("ERROR, cannot open inputfile '" << batchinput << "'");
    return 99;
  }
  int res = parseAndProcess(is);
  if (res == 0) {
    //Prosessing of file done, remove it!
    unlink(file.c_str());
  }

  // if fifo name set, write response to fifo
  if (!fifo_name.empty()) {
    int fd = open(fifo_name.c_str(), O_WRONLY);
    if (fd == -1) {
        METLIBS_LOG_ERROR("ERROR, can't open the fifo <" << fifo_name << ">!");
      goto ERROR;
    }

    char buf[1];
    if (res == 0)
        buf[0] = 'r';
    else
        buf[0] = 'e';

    if (write(fd, buf, 1) == -1) {
        METLIBS_LOG_ERROR("ERROR, can't write to fifo <" << fifo_name << ">!");
    } else {
      if (verbose)
          METLIBS_LOG_INFO("FIFO client <" << fifo_name << "> notified!");
    }

    close(fd);
    fifo_name = "";
  }

  if (res != 0)
      return 99;

  return 0;

  ERROR: unlink(file.c_str());
  return -1;
}

static void doWork()
{
  if (command_path.empty()) {
    METLIBS_LOG_ERROR("ERROR, trying to scan for commands, but command_path not set!");
    return;
  }

  const diutil::string_v matches = diutil::glob(command_path);
  if (matches.empty()) {
    METLIBS_LOG_WARN("WARNING, scan for commands returned nothing");
    return;
  }

  // loop over files
  for (diutil::string_v::const_iterator it = matches.begin(); it != matches.end(); ++it)
    dispatchWork(*it);
}



/*
 =================================================================
 BDIANA - BATCH PRODUCTION OF DIANA GRAPHICAL PRODUCTS, public C
 =================================================================
 */
int diana_init(int _argc, char** _argv)
{
  diOrderBook *orderbook = NULL;
  std::string xhost = ":0.0"; // default DISPLAY
  std::string sarg;
  int port;

  // get the DISPLAY variable
  char * ctmp = getenv("DISPLAY");
  if (ctmp) {
    xhost = ctmp;
  }

  // get the BDIANA_LOGGER variable
  ctmp = getenv("BDIANA_LOGGER");
  if (ctmp) {
    logfilename = ctmp;
  }

#if defined(Q_WS_QWS)
  application = new QApplication(_argc, _argv, QApplication::GuiServer);
#else
  application = new QApplication(_argc, _argv);
#endif
  QStringList argv = application->arguments();
  int argc = argv.size();

  // check command line arguments
  if (argc < 2) {
    printUsage(false);
  }

  vector<std::string> ks;
  int ac = 1;
  while (ac < argc) {
    sarg = argv[ac].toStdString();
//    cerr << "Checking arg:" << sarg << endl;

    if (sarg == "-display") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      xhost = argv[ac].toStdString();

    } else if (sarg == "-input" || sarg == "-i") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      batchinput = argv[ac].toStdString();

    } else if (sarg == "-setup" || sarg == "-s") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      setupfile = argv[ac].toStdString();
      setupfilegiven = true;

    } else if (sarg == "-logger" || sarg == "-L") {
      ac++;
      if (ac >= argc)
        printUsage(false);
      logfilename = argv[ac].toStdString();

    } else if (sarg == "-v") {
      verbose = true;

    } else if (sarg == "-signal") {
      if (orderbook != NULL) {
        cerr << "ERROR, can't have both -address and -signal" << endl;
        return 1;
      }
      wait_for_signals = true;

    } else if (sarg == "-libinput") {
      wait_for_input = true;
    } else if (sarg == "-example") {
      printUsage(true);

    } else if (sarg == "-use_nowtime") {
        // Use time closest to the current time even if there exists a field
        // and not the timestamps for the future. This corresponds to the
        // default value when using the gui.
        use_nowtime = true;

    } else if (sarg.find("-address=") == 0) {
      if (wait_for_signals) {
        cerr << "ERROR, can't have both -address and -signal" << endl;
        return 1;
      }
      if (orderbook == NULL) {
        orderbook = new diOrderBook();
        orderbook->start();
      }
      ks = miutil::split(sarg, "=");
      if (ks.size() == 2) {
        ks = miutil::split(ks[1], ":");
        if (ks.size() == 2) {
          if (miutil::is_number(ks[1])) {
            port = miutil::to_int(ks[1]);
          } else {
            cerr << "ERROR, " << ks[1] << " is not a valid TCP port number" << endl;
            return 1;
          }
          if (port < 1 || port > 65535) {
            cerr << "ERROR, " << port << "  is not a valid TCP port number" << endl;
            return 1;
          }
        } else {
          port = diOrderListener::DEFAULT_PORT;
        }
        if (!orderbook->addListener(ks[0].c_str(), port)) {
          cerr << "ERROR, unable to listen on " << ks[0] << ":" << port << endl;
          return 1;
        }
      } else {
        cerr << "ERROR, invalid argument to -address" << endl;
        return 1;
      }
    } else {
      ks = miutil::split(sarg, "=");
      if (ks.size() == 2) {
        keyvalue tmp;
        tmp.key = ks[0];
        tmp.value = ks[1];
        keys.push_back(tmp);

        // temporary: force plottime
        if (tmp.key == "TIME") {
          if (miTime::isValid(tmp.value)) {
            fixedtime = ptime = miTime(tmp.value);
          } else {
            cerr << "ERROR, invalid TIME-variable on commandline:" << tmp.value
                << endl;
            return 1;
          }
        }
      } else {
        cerr << "WARNING, unknown argument on commandline:" << sarg << endl;
      }
    }
    ac++;
  } // command line parameters

  if (false and batchinput.empty()) // FIXME removing the 'false' kills perl Metno::Bdiana
    printUsage(false);

  // tell fimex to use log4cpp
  MetNoFimex::Logger::setClass(MetNoFimex::Logger::LOG4CPP);
  milogger::LoggingConfig log4cpp(logfilename);

  METLIBS_LOG_INFO(argv[0].toStdString() << " : DIANA batch version " << VERSION);

  priop.fname = "tmp_diana.ps";
  priop.colop = d_print::greyscale;
  priop.drawbackground = false;
  priop.orientation = d_print::ori_automatic;
  priop.pagesize = d_print::A4;
  // 1.4141
  priop.papersize.hsize = 297;
  priop.papersize.vsize = 420;
  priop.doEPS = false;

  xsize = 1696;
  ysize = 1200;

  hardcopy_started[plot_none] = false;
  hardcopy_started[plot_standard] = false;
  hardcopy_started[plot_vcross] = false;
  hardcopy_started[plot_vprof] = false;
  hardcopy_started[plot_spectrum] = false;

  printman = new printerManager;

  /*
   if setupfile specified on the command-line, parse it now
   */
  if (setupfilegiven) {
    setupread = readSetup(setupfile, *printman);
    if (!setupread) {
      METLIBS_LOG_ERROR("ERROR, unable to read setup:" << setupfile);
      diana_dealloc();
      return 99;
    }
  }

  /*
   Read initial input and process commands...
   */
  if (!batchinput.empty()) {
    METLIBS_LOG_INFO("Reading input file: " << batchinput.c_str());
    ifstream is(batchinput.c_str());
    if (!is) {
        METLIBS_LOG_ERROR("ERROR, cannot open inputfile " << batchinput);
      return 99;
    }
    int res = parseAndProcess(is);
    ensureNewContext();
    if (res != 0)
      return 99;
  }

  /*
   Signal handling
   */
  if (wait_for_signals) {
    ofstream fs;
    bool timeout;
    bool quit = false;

    signalInit();

    if (verbose)
        METLIBS_LOG_INFO("PID: " << getpid());

    fs.open("bdiana.pid");

    if (!fs) {
      METLIBS_LOG_ERROR("ERROR, can't open file <bdiana.pid>!");
      diana_dealloc();
      return 1;
    }

    fs << getpid() << endl;
    fs.close();

    while (!quit) {
      application->processEvents(); // do we actually care in this case?
      switch (waitOnSignal(10, timeout)) {
      case -1:
          METLIBS_LOG_INFO("ERROR, a waitOnSignal error occured!");
        quit = true;
        break;
      case 0:
        if (verbose)
          METLIBS_LOG_INFO("SIGUSR1: received!");
        doWork();
        break;
      case 1:
        if (!timeout) {
          METLIBS_LOG_INFO("SIGTERM, SIGINT: received!");
          quit = true;
        }
      }
    }
  } else if (orderbook != NULL) {
    bool quit = false;
    /*
     * XXX should handle SIGTERM / SIGINT somehow; currently, quit can
     * never be false in this branch, and a SIGTERM or SIGINT will
     * terminate bdiana immediately, with no chance for cleanup.
     */
    while (!quit) {
      QPointer<diWorkOrder> order = orderbook->getNextOrder();
      if (order) {
        istringstream is(order->getText());
        METLIBS_LOG_INFO("processing order...");
        parseAndProcess(is);
        METLIBS_LOG_INFO("done");
        if (order) // may have been deleted (if the client disconnected)
          order->signalCompletion();
        else
          METLIBS_LOG_INFO("diWorkOrder went away");
        application->processEvents();
      } else {
        METLIBS_LOG_INFO("waiting");
        application->processEvents(QEventLoop::WaitForMoreEvents);
      }
    }
  } else if (wait_for_input) {
    // nothing to be done, just a dummy
  } else if (batchinput.empty()) {
    METLIBS_LOG_WARN("Neither -address nor -signal was specified");
  }
  return DIANA_OK;
}

int diana_dealloc()
{
  // clean up structures
#ifdef VIDEO_EXPORT
  if(movieMaker)
    endVideo();
#endif // VIDEO_EXPORT

  delete vprofmanager;
  delete spectrummanager;
  delete main_controller;
  vcrossmanager = vcross::QtManager_p();

  delete glpainter;
  delete glcanvas;

  return DIANA_OK;
}
