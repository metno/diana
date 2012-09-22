/*-*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-*/
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
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
#include <QtGui>
#include <QtSvg>
#include "PaintGL/paintgl.h"
#else
#include <QtOpenGL>
#endif

#include <diAnnotationPlot.h>
#include <diController.h>
#include <diFieldPlot.h>
#include <diObsManager.h>
#include <diObsPlot.h>
#include <diSatManager.h>
#include <diSatPlot.h>

#include <puCtools/sleep.h>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diLocalSetupParser.h>
#include <diPrintOptions.h>
#include <diFontManager.h>
#include <diImageIO.h>
#include <puCtools/glob.h>
#include <puCtools/glob_cache.h>

#include <diVcrossManager.h>
#include <diVcrossPlot.h>
#include <diVcrossOptions.h>

#include <diVprofManager.h>
#include <diVprofOptions.h>

#include <diField/diFieldManager.h>
#include <diField/diRectangle.h>

#include <diSpectrumManager.h>
#include <diSpectrumOptions.h>

#ifdef VIDEO_EXPORT
# include <MovieMaker.h>
#endif

#include <signalhelper.h>

#include <miLogger/logger.h>
#include <miLogger/LogHandler.h>

#include <diOrderBook.h>

// Keep X headers last, otherwise Qt will be very unhappy
#ifdef USE_XLIB
//#include <X11/Intrinsic.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

/* Created at Wed May 23 15:28:41 2001 */

using namespace std; using namespace miutil;

bool verbose = false;

// command-strings
const miString com_liststart = "list.";
const miString com_listend = "list.end";

const miString com_loop = "loop";
const miString com_endloop = "endloop";
const miString com_loopend = "loop.end";

const miString com_vprof_opt = "vprof.options";
const miString com_vprof_opt_end = "vprof.options.end";
const miString com_vcross_opt = "vcross.options";
const miString com_vcross_opt_end = "vcross.options.end";
const miString com_spectrum_opt = "spectrum.options";
const miString com_spectrum_opt_end = "spectrum.options.end";

const miString com_plot = "plot";
const miString com_vcross_plot = "vcross.plot";
const miString com_vprof_plot = "vprof.plot";
const miString com_spectrum_plot = "spectrum.plot";
const miString com_endplot = "endplot";
const miString com_plotend = "plot.end";

const miString com_print_document = "print_document";

const miString com_wait_for_commands = "wait_for_commands";
const miString com_wait_end = "wait.end";
const miString com_fifo_name = "fifo";

const miString com_setupfile = "setupfile";
const miString com_command_path = "command_path";
const miString com_buffersize = "buffersize";

const miString com_papersize = "papersize";
const miString com_filname = "filename";
const miString com_toprinter = "toprinter";
const miString com_printer = "printer";
const miString com_output = "output";
const miString com_colour = "colour";
const miString com_drawbackground = "drawbackground";
const miString com_orientation = "orientation";
const miString com_antialiasing = "antialiasing";

const miString com_settime = "settime";
const miString com_addhour = "addhour";
const miString com_addminute = "addminute";
const miString com_archive = "archive";
const miString com_keepplotarea = "keepplotarea";
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
const miString com_plotannotationsonly = "plotannotationsonly";
#endif

const miString com_multiple_plots = "multiple.plots";
const miString com_plotcell = "plotcell";

const miString com_trajectory = "trajectory";
const miString com_trajectory_opt = "trajectory_options";
const miString com_trajectory_print = "trajectory_print";
const miString com_setup_field_info = "setup_field_info";

const miString com_time_opt = "time_options";
const miString com_time_format = "time_format";
const miString com_time = "time";
const miString com_endtime = "endtime";
const miString com_level = "level";
const miString com_endlevel = "endlevel";

const miString com_field_files = "<field_files>";
const miString com_field_files_end = "</field_files>";

const miString com_describe = "describe";
const miString com_describe_end = "enddescribe";

enum canvas {
  x_pixmap, glx_pixelbuffer, qt_glpixelbuffer, qt_glframebuffer, qt_qimage
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
  miString key;
  miString value;
};

// one list of strings with name
struct stringlist {
  miString name;
  vector<miString> l;
};

plot_type plottype = plot_none;// current plot_type
plot_type prevplottype = plot_none;// previous plottype
plot_type multiple_plottype = plot_none;//

bool hardcopy_started[5]; // has startHardcopy been called

// the Controller and Managers
Controller* main_controller = 0;
VprofManager* vprofmanager = 0;
VcrossManager* vcrossmanager = 0;
SpectrumManager* spectrummanager = 0;

#ifdef USE_XLIB
// Attribs for OpenGL visual
static int dblBuf[] = {GLX_DOUBLEBUFFER, GLX_RGBA, GLX_DEPTH_SIZE, 16,
  GLX_RED_SIZE, 1, GLX_GREEN_SIZE, 1, GLX_BLUE_SIZE, 1, GLX_ALPHA_SIZE, 1,
  //GLX_TRANSPARENT_TYPE, GLX_TRANSPARENT_RGB,
  GLX_STENCIL_SIZE, 1, None};
static int *snglBuf = &dblBuf[1];

Display* dpy;
XVisualInfo* pdvi; // X visual
GLXContext cx; // GL drawing context
Pixmap pixmap; // X pixmap
GLXPixmap pix; // GLX pixmap
#ifdef GLX_VERSION_1_3
GLXPbuffer pbuf; // GLX Pixel Buffer
#endif
#endif

QApplication * application = 0; // The Qt Application object
#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
QGLPixelBuffer * qpbuffer = 0; // The Qt GLPixelBuffer used as canvas
QGLFramebufferObject * qfbuffer = 0; // The Qt GLFrameBuffer used as canvas
QGLWidget *qwidget = 0; // The rendering context used for Qt GLFrameBuffer
#else
QPainter painter;
PaintGL wrapper;
PaintGLContext context;
#endif
QPicture picture;
QImage image;
map<miString, map<miString,miString> > outputTextMaps; // output text for cases where output data is XML/JSON
int xsize; // total pixmap width
int ysize; // total pixmap height
bool multiple_plots = false; // multiple plots per page
int numcols, numrows; // for multiple plots
int plotcol, plotrow; // current plotcell for multiple plots
int deltax, deltay; // width and height of plotcells
int margin, spacing; // margin and spacing for multiple plots
bool multiple_newpage = false; // start new page for multiple plots

bool use_double_buffer = true; // use double buffering
#ifdef USE_XLIB
int default_canvas = x_pixmap;
#else
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
int default_canvas = qt_qimage;
#else
int default_canvas = qt_glpixelbuffer;
#endif
#endif
int canvasType = default_canvas; // type of canvas to use
bool use_nowtime = false;
bool antialias = false;

// replaceable values for plot-commands
vector<keyvalue> keys;

miTime thetime, ptime, fixedtime;

miString batchinput;
// diana setup file
miString setupfile = "diana.setup";
bool setupfilegiven = false;
miString command_path;

bool keeparea = false;
bool useArchive = false;
bool toprinter = false;
bool raster = false; // false means postscript
bool shape = false; // false means postscript
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
bool svg = false;
bool pdf = false;
bool json = false;
#endif
int raster_type = image_png; // see enum image_type above

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
bool plotAnnotationsOnly = false;
vector<Rectangle> annotationRectangles;
QTransform annotationTransform;
#endif

/*
 more...
 */
vector<miString> vs, vvs, vvvs;
bool setupread = false;
bool buffermade = false;
vector<miString> lines, tmplines;
vector<int> linenumbers, tmplinenumbers;

bool plot_trajectory = false;
bool trajectory_started = false;

miString trajectory_options;

miString time_options;
miString time_format = "$time";

#ifdef VIDEO_EXPORT
MovieMaker *movieMaker = 0;
#endif

// list of lists..
vector<stringlist> lists;

printerManager * printman;
printOptions priop;

bool wait_for_signals = false;
miString fifo_name;

#define MAKE_CONTROLLER \
    main_controller = new Controller; \
    if (!main_controller->parseSetup()) { \
      cerr \
          << "ERROR, an error occured while main_controller parsed setup: " \
          << setupfile << endl; \
      return 99; \
    }

/*
 clean an input-string: remove preceding and trailing blanks,
 remove comments
 */
void cleanstr(miString& s)
{
  std::string::size_type p;
  if ((p = s.find("#")) != string::npos)
    s.erase(p);

  s.remove('\n');
  s.trim();
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
void unpackloop(vector<miString>& orig, // original strings..
    vector<int>& origlines, // ..with corresponding line-numbers
    unsigned int& index, // original string-counter to update
    vector<miString>& part, // final strings from loop-unpacking..
    vector<int>& partlines) // ..with corresponding line-numbers
{
  unsigned int start = index;

  miString loops = orig[index];
  loops = loops.substr(4, loops.length() - 4);

  vector<miString> vs, vs2;

  vs = loops.split('=');
  if (vs.size() < 2) {
    cerr << "ERROR, missing \'=\' in loop-statement at line:"
        << origlines[start] << endl;
    exit(1);
  }

  miString keys = vs[0]; // key-part
  vector<miString> vkeys = keys.split('|');
  unsigned int nkeys = vkeys.size();

  miString argu = vs[1]; // argument-part
  unsigned int nargu;
  vector<vector<miString> > arguments;

  /* Check if argument is name of list
   Lists are recognized with preceding '@' */
  if (argu.length() > 1 && argu.substr(0, 1) == "@") {
    miString name = argu.substr(1, argu.length() - 1);
    // search for list..
    unsigned int k;
    for (k = 0; k < lists.size(); k++) {
      if (lists[k].name == name)
        break;
    }
    if (k == lists.size()) {
      // list not found
      cerr << "ERROR, reference to unknown list at line:" << origlines[start]
          << endl;
      exit(1);
    }
    nargu = lists[k].l.size();
    // split listentries into separate arguments for loop
    for (unsigned int j = 0; j < nargu; j++) {
      vs = lists[k].l[j].split('|');
      // check if correct number of arguments
      if (vs.size() != nkeys) {
        cerr << "ERROR, number of arguments in loop at:'" << lists[k].l[j]
            << "' line:" << origlines[start] << " does not match key:" << keys
            << endl;
        exit(1);
      }
      arguments.push_back(vs);
    }

  } else {
    // ordinary arguments to loop: comma-separated
    vs2 = argu.split(',');
    nargu = vs2.size();
    for (unsigned int k = 0; k < nargu; k++) {
      vs = vs2[k].split('|');
      // check if correct number of arguments
      if (vs.size() != nkeys) {
        cerr << "ERROR, number of arguments in loop at:'" << vs2[k]
            << "' line:" << origlines[start] << " does not match key:" << keys
            << endl;
        exit(1);
      }
      arguments.push_back(vs);
    }
  }

  // temporary storage of loop-contents
  vector<miString> tmppart;
  vector<int> tmppartlines;

  // go to next line
  index++;

  // start unpacking loop
  for (; index < orig.size(); index++) {
    if (orig[index].downcase() == com_endloop || orig[index].downcase() ==
        com_loopend) { // reached end
      // we have the loop-contents
      for (unsigned int i = 0; i < nargu; i++) { // loop over arguments
        for (unsigned int j = 0; j < tmppart.size(); j++) { // loop over lines
          miString l = tmppart[j];
          for (unsigned int k = 0; k < nkeys; k++) { // loop over keywords
            // replace all variables
            l.replace(vkeys[k], arguments[i][k]);
          }
          part.push_back(l);
          partlines.push_back(tmppartlines[j]);
        }
      }
      break;

    } else if (miString(orig[index].substr(0, 4)).downcase() == com_loop) {
      // start of new loop
      unpackloop(orig, origlines, index, tmppart, tmppartlines);

    } else { // fill loop-contents to temporary vector
      tmppart.push_back(orig[index]);
      tmppartlines.push_back(origlines[index]);
    }
  }
  if (index == orig.size()) {
    cerr << "ERROR, missing \'LOOP.END\' for loop at line:" << origlines[start]
        << endl;
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
void unpackinput(vector<miString>& orig, // original setup
    vector<int>& origlines, // original list of linenumbers
    vector<miString>& final, // final setup
    vector<int>& finallines) // final list of linenumbers
{
  unsigned int i;
  for (i = 0; i < orig.size(); i++) {
    if (miString(orig[i].substr(0, 4)).downcase() == com_loop) {
      // found start of loop - unpack it
      unpackloop(orig, origlines, i, final, finallines);
    } else if (miString(orig[i].substr(0, 5)).downcase() == com_liststart) {
      // save a list
      stringlist li;
      if (orig[i].length() < 6) {
        cerr << "ERROR, missing name for LIST at line:" << origlines[i] << endl;
        exit(1);
      }
      li.name = orig[i].substr(5, orig[i].length() - 5);
      unsigned int start = i;
      i++;
      for (; i < orig.size() && orig[i].downcase() != com_listend; i++)
        li.l.push_back(orig[i]);
      if (i == orig.size() || orig[i].downcase() != com_listend) {
        cerr << "ERROR, missing LIST.END for list starting at line:"
            << origlines[start] << endl;
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

  miString s;
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

  // substitute key-values
  unsigned int nkeys = keys.size();
  if (nkeys > 0)
    for (unsigned int k = 0; k < linenum; k++)
      for (unsigned int m = 0; m < nkeys; m++)
        lines[k].replace("$" + keys[m].key, keys[m].value);

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
  cout << "opening video stream |-->" << endl;
  movieMaker = new MovieMaker(output, format, 0.2f);
}

bool addVideoFrame(const QImage &img)
{
  if (!movieMaker)
    return false;

  return movieMaker->addImage(img);
}

void endVideo()
{
  cout << "-->| video stream closed" << endl;
  delete movieMaker;
  movieMaker = 0;
}
#endif // VIDEO_EXPORT

void startHardcopy(const plot_type pt, const printOptions priop)
{
  if (pt == plot_standard && main_controller) {
    if (verbose)
      cout << "- startHardcopy (standard)" << endl;
    main_controller->startHardcopy(priop);
  } else if (pt == plot_vcross && vcrossmanager) {
    if (verbose)
      cout << "- startHardcopy (vcross)" << endl;
    vcrossmanager->startHardcopy(priop);
  } else if (pt == plot_vprof && vprofmanager) {
    if (verbose)
      cout << "- startHardcopy (vprof)" << endl;
    vprofmanager->startHardcopy(priop);
  } else if (pt == plot_spectrum && spectrummanager) {
    if (verbose)
      cout << "- startHardcopy (spectrum)" << endl;
    spectrummanager->startHardcopy(priop);
  } else {
    if (verbose)
      cout << "- startHardcopy failure (missing manager)" << endl;
  }
  hardcopy_started[pt] = true;
}

void endHardcopy(const plot_type pt)
{
  // finish off postscript-sessions
  if (pt == plot_standard && hardcopy_started[pt] && main_controller) {
    if (verbose)
      cout << "- endHardcopy (standard)" << endl;
    main_controller->endHardcopy();
  } else if (pt == plot_vcross && hardcopy_started[pt] && vcrossmanager) {
    if (verbose)
      cout << "- endHardcopy (vcross)" << endl;
    vcrossmanager->endHardcopy();
  } else if (pt == plot_vprof && hardcopy_started[pt] && vprofmanager) {
    if (verbose)
      cout << "- endHardcopy (vprof)" << endl;
    vprofmanager->endHardcopy();
  } else if (pt == plot_spectrum && hardcopy_started[pt] && spectrummanager) {
    if (verbose)
      cout << "- endHardcopy (spectrum)" << endl;
    spectrummanager->endHardcopy();
  } else if (pt == plot_none) {
    // stop all
    endHardcopy(plot_standard);
    endHardcopy(plot_vcross);
    endHardcopy(plot_vprof);
    endHardcopy(plot_spectrum);
  }
  hardcopy_started[pt] = false;
}

// VPROF-options with parser
miString vprof_station;
vector<miString> vprof_models, vprof_options;
bool vprof_plotobs = true;
bool vprof_optionschanged;

void parse_vprof_options(const vector<miString>& opts)
{
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    miString line = opts[i];
    line.trim();
    if (!line.exists())
      continue;
    miString upline = line.upcase();

    if (upline == "OBSERVATION.ON")
      vprof_plotobs = true;
    else if (upline == "OBSERVATION.OFF")
      vprof_plotobs = false;
    else if (upline.contains("MODELS=") || upline.contains("MODEL=")
        || upline.contains("STATION=")) {
      vector<miString> vs = line.split("=");
      if (vs.size() > 1) {
        miString key = vs[0].upcase();
        miString value = vs[1];
        if (key == "STATION") {
          if (value.contains("\""))
            value.remove('\"');
          vprof_station = value;
        } else if (key == "MODELS" || key == "MODEL") {
          vprof_models = value.split(",");
        }
      }
    } else {
      // assume plot-options
      vprof_options.push_back(line);
      vprof_optionschanged = true;
    }
  }
}

// VCROSS-options with parser
vector<miString> vcross_data, vcross_options;
miString crossection;
bool vcross_optionschanged;

void parse_vcross_options(const vector<miString>& opts)
{
  bool data_exist = false;
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    miString line = opts[i];
    line.trim();
    if (!line.exists())
      continue;
    miString upline = line.upcase();

    if (upline.contains("CROSSECTION=")) {
      vector<miString> vs = line.split("=");
      crossection = vs[1];
      if (crossection.contains("\""))
        crossection.remove('\"');
    } else if (upline.contains("VCROSS ")) {
      if (!data_exist)
        vcross_data.clear();
      vcross_data.push_back(line);
      data_exist = true;
    } else {
      // assume plot-options
      vcross_options.push_back(line);
      vcross_optionschanged = true;
    }
  }
}

// SPECTRUM-options with parser
miString spectrum_station;
vector<miString> spectrum_models, spectrum_options;
bool spectrum_plotobs = false; // not used, yet...
bool spectrum_optionschanged;

void parse_spectrum_options(const vector<miString>& opts)
{
  int n = opts.size();
  for (int i = 0; i < n; i++) {
    miString line = opts[i];
    line.trim();
    if (!line.exists())
      continue;
    miString upline = line.upcase();

    if (upline == "OBSERVATION.ON")
      spectrum_plotobs = true;
    else if (upline == "OBSERVATION.OFF")
      spectrum_plotobs = false;
    else if (upline.contains("MODELS=") || upline.contains("MODEL=")
        || upline.contains("STATION=")) {
      vector<miString> vs = line.split("=");
      if (vs.size() > 1) {
        miString key = vs[0].upcase();
        miString value = vs[1];
        if (key == "STATION") {
          if (value.contains("\""))
            value.remove('\"');
          spectrum_station = value;
        } else if (key == "MODELS" || key == "MODEL") {
          spectrum_models = value.split(",");
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
bool readSetup(const miString& constSetupfile, printerManager& printmanager)
{
  miString setupfile = constSetupfile;
  cout << "Reading setupfile:" << setupfile << endl;

  if (!LocalSetupParser::parse(setupfile)) {
    cerr << "ERROR, an error occured while reading setup: " << setupfile
        << endl;
    return false;
  }
  if (!printmanager.parseSetup()) {
    cerr << "ERROR, an error occured while reading setup: " << setupfile
        << endl;
    return false;
  }
  return true;
}

/*
 Output Help-message:  call-syntax and optionally an example
 input-file for bdiana
 */
void printUsage(bool showexample)
{
  const miString help =
      "***************************************************             \n"
        " DIANA batch version:" + miString(VERSION) + "                  \n"
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
#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
        " - using qtgl: all available raster formats in Qt               \n"
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
#ifdef USE_XLIB
        "-use_pixmap | -use_pbuffer |"
#endif
        " -use_qimage ]"
        " [-use_doublebuffer | -use_singlebuffer]"
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
        "-example          : list example input-file and exit                    \n"
#ifdef USE_XLIB
        "-display          : x-server to use (default: env DISPLAY)              \n"
        "-use_pixmap       : use X Pixmap/GLXPixmap as drawing medium            \n"
        "-use_pbuffer      : use GLX v.1.3 PixelBuffers as drawing medium        \n"
        "-use_qtgl         : use QGLPixelBuffer as drawing medium (default)      \n"
#endif
        "-use_qimage       : use QImage as drawing medium                        \n"
#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
        "-use_doublebuffer : use double buffering OpenGL (default)               \n"
        "-use_singlebuffer : use single buffering OpenGL                         \n"
#endif
        "                                                                        \n"
        "special key/value pairs:                                                \n"
        " - TIME=\"YYYY-MM-DD hh:mm:ss\"      plot-time                          \n"
        "                                                                        \n";

  const miString
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
            " timediff=180 colour=black font=Helvetica face=normal\n"
            "OBJECTS NAME=\"DNMI Bakkeanalyse\" types=front,symbol,area \\\n"
            " timediff=60\n"
            "MAP area=Norge backcolour=white map=Gshhs-AUTO contour=on \\\n"
            " cont.colour=black cont.linewidth=1 cont.linetype=solid cont.zorder=1 \\\n"
            " land=on land.colour=landgul land.zorder=0 latlon=off frame=off\n"
            "LABEL data font=Helvetica\n"
            "LABEL text=\"$day $date $auto UTC\" tcolour=red bcolour=black \\\n"
            " fcolour=white:200 polystyle=both halign=left valign=top \\\n"
            " font=Helvetica fontsize=12\n"

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
            "MODELS=HIRLAM.00,EC.12   # comma-separated list of models         \n"
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
            "#  use settime=YYYY-MM-DD hh:mm:ss                                \n"
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
            "#  scale=1 timediff=180 colour=black font=Helvetica face=normal        \n"
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

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
void getAnnotationsArea(int& ox, int& oy, int& xsize, int& ysize, int number = -1)
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

      for (vector<miString>::iterator itj = iti->vstr.begin(); itj != iti->vstr.end(); ++itj) {

        // Find the table description in the string.
        miString legend = (*itj);
        size_t at = legend.find("table=\"");

        if (at != string::npos) {
          // Find the trailing quote.
          at += 7;
          size_t end = legend.find("\"", at);
          if (end == string::npos)
              end = legend.size();

          // Remove leading and trailing quotes.
          legend = legend.substr(at, end - at);

          map<miString,miString> textMap;
          miString title;
          vector<miString> colors;
          vector<miString> labels;

          bool first = true;
          vector<miString> line;
          miString current;
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
                labels.push_back("\"" + line[1] + "\"");
              }
              line.clear();
            }
          }
          miString value = miString();
          value.join(colors, ", ");
          textMap["colors"] = "[" + value + "]";
          value.join(labels, ", ");
          textMap["labels"] = "[" + value + "]";

          if (!title.empty())
            outputTextMaps[title] = textMap;
        }
      }
    }
  }
}

void ensureNewContext()
{
  plotAnnotationsOnly = false;

  if (canvasType == qt_qimage) {
    if (context.isPainting())
      context.end();
    if (painter.isActive())
      painter.end();
  }

  context.makeCurrent();
}

#endif

int parseAndProcess(istream &is)
{
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
  ensureNewContext();
#endif

  // unpack loops, make lists, merge lines etc.
  int res = prepareInput(is);
  if (res != 0)
    return res;

  vector<miString> extra_field_lines;

  int linenum = lines.size();

  // parse input - and perform plots
  for (int k = 0; k < linenum; k++) {// input-line loop
    // start parsing...
    if (lines[k].downcase() == com_vprof_opt) {
      vector<miString> pcom;
      for (int i = k + 1; i < linenum && lines[i].downcase()
          != com_vprof_opt_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;
      parse_vprof_options(pcom);
      continue;

    } else if (lines[k].downcase() == com_vcross_opt) {
      vector<miString> pcom;
      for (int i = k + 1; i < linenum && lines[i].downcase()
          != com_vcross_opt_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;
      parse_vcross_options(pcom);
      continue;

    } else if (lines[k].downcase() == com_spectrum_opt) {
      vector<miString> pcom;
      for (int i = k + 1; i < linenum && lines[i].downcase()
          != com_spectrum_opt_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;
      parse_spectrum_options(pcom);
      continue;

    } else if (lines[k].downcase() == com_plot || lines[k].downcase()
        == com_vcross_plot || lines[k].downcase() == com_vprof_plot
        || lines[k].downcase() == com_spectrum_plot) {
      // --- START PLOT ---
      if (lines[k].downcase() == com_plot) {
        plottype = plot_standard;
        if (verbose)
          cout << "Preparing new standard-plot" << endl;

      } else if (lines[k].downcase() == com_vcross_plot) {
        plottype = plot_vcross;
        if (verbose)
          cout << "Preparing new vcross-plot" << endl;

      } else if (lines[k].downcase() == com_vprof_plot) {
        plottype = plot_vprof;
        if (verbose)
          cout << "Preparing new vprof-plot" << endl;

      } else if (lines[k].downcase() == com_spectrum_plot) {
        plottype = plot_spectrum;
        if (verbose)
          cout << "Preparing new spectrum-plot" << endl;
      }

      // if new plottype: make sure previous postscript-session is stopped
      if (prevplottype != plot_none && plottype != prevplottype) {
        endHardcopy(prevplottype);
      }
      prevplottype = plottype;

      if (multiple_plots && multiple_plottype != plot_none
          && hardcopy_started[multiple_plottype] && multiple_plottype
          != plottype) {
        cerr
            << "ERROR, you can not mix STANDARD/VCROSS/VPROF/SPECTRUM in multiple plots "
            << "..Exiting.." << endl;
        return 1;
      }
      multiple_plottype = plottype;

      if (multiple_plots && shape ) {
        cerr
            << "ERROR, you can not use shape option for multiple plots "
            << "..Exiting.." << endl;
        return 1;
      }
      if ( !(plottype == plot_none || plottype == plot_standard)  && shape ) {
        cerr
            << "ERROR, you can only use plottype STANDARD when using shape option"
            << "..Exiting.." << endl;
        return 1;
      }

      if (!buffermade) {
        cerr << "ERROR, no buffersize set..exiting" << endl;
        return 1;
      }
      if (!setupread) {
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          cerr << "ERROR, no setupinformation..exiting" << endl;
          return 99;
        }
      }

      vector<miString> pcom;
      for (int i = k + 1; i < linenum && lines[i].downcase() != com_endplot
          && lines[i].downcase() != com_plotend; i++, k++) {
        if (shape ) {
                if ( (lines[i].contains("OBS") || lines[i].contains("SAT") || lines[i].contains("OBJECTS") ||
                        lines[i].contains("EDITFIELD") || lines[i].contains("TRAJECTORY"))) {
                        cerr << "Error, Shape option can not be used for OBS/OBJECTS/SAT/TRAJECTORY/EDITFIELD.. exiting" << endl;
                        return 1;
                }
                if ( lines[i].contains("FIELD") ) {
                        miString shapeFileName=priop.fname;
                        lines[i]+= " shapefilename=" + shapeFileName;
                        if ( lines[i].contains("shapefile=0") )
                                lines[i].replace("shapefile=0","shapefile=1");
                        else if ( !lines[i].contains("shapefile=") )
                                lines[i]+=" shapefile=1";
                }
        }
        pcom.push_back(lines[i]);
      }
      k++;

      if (plottype == plot_standard) {
        // -- normal plot
        // Make Controller
        if (!main_controller) {
          MAKE_CONTROLLER

          vector<miString> field_errors;
          if (!main_controller->getFieldManager()->updateFileSetup(extra_field_lines, field_errors)) {
            cerr << "ERROR, an error occurred while adding new fields:" << endl;
            for (unsigned int kk = 0; kk < field_errors.size(); ++kk)
              cerr << field_errors[kk] << endl;
          }
        }

        // Perform a quick check for newly arrived data.
        main_controller->getFieldManager()->updateSources();

        // turn on/off archive-mode (observations)
        main_controller->archiveMode(useArchive);

        if (verbose)
          cout << "- setPlotWindow" << endl;
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
          cout << "- sending plotCommands" << endl;
        main_controller->plotCommands(pcom);

        vector<miTime> fieldtimes, sattimes, obstimes, objtimes, ptimes;
        main_controller->getPlotTimes(fieldtimes, sattimes, obstimes, objtimes,
            ptimes);

        if (ptime.undef()) {
          if (use_nowtime)
            thetime = selectNowTime(fieldtimes, sattimes, obstimes, objtimes, ptimes);
          else if (fieldtimes.size() > 0)
            thetime = fieldtimes[fieldtimes.size() - 1];
          else if (sattimes.size() > 0)
            thetime = sattimes[sattimes.size() - 1];
          else if (obstimes.size() > 0)
            thetime = obstimes[obstimes.size() - 1];
          else if (objtimes.size() > 0)
            thetime = objtimes[objtimes.size() - 1];
          else if (ptimes.size() > 0)
            thetime = ptimes[ptimes.size() - 1];
        } else
          thetime = ptime;

        if (verbose)
          cout << "- plotting for time:" << thetime << endl;
        main_controller->setPlotTime(thetime);

        if (verbose)
          cout << "- updatePlots" << endl;
        if (!main_controller->updatePlots()) {
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
            cerr << "Failed to update plots." << endl;
            painter.end();
            context.end();
            return 99;
#endif
        }
        cout <<main_controller->getMapArea()<<endl;

        if (!raster && !shape && !json && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_standard, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
            startVideo(priop);
#endif
        }

        if (multiple_plots) {
          glViewport(margin + plotcol * (deltax + spacing), margin + plotrow
              * (deltay + spacing), deltax, deltay);
        }

        if (plot_trajectory && !trajectory_started) {
          vector<miString> vstr;
          vstr.push_back("clear");
          vstr.push_back("delete");
          vstr.push_back(trajectory_options);
          main_controller->trajPos(vstr);
          //main_controller->trajTimeMarker(trajectory_timeMarker);
          main_controller->startTrajectoryComputation();
          trajectory_started = true;
        } else if (!plot_trajectory && trajectory_started) {
          vector<miString> vstr;
          vstr.push_back("clear");
          vstr.push_back("delete");
          main_controller->trajPos(vstr);
          trajectory_started = false;
        }

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        if (canvasType == qt_qimage && raster && antialias)
          painter.setRenderHint(QPainter::Antialiasing);
        else if (svg || pdf)
          painter.setRenderHint(QPainter::Antialiasing);
#endif

        if (verbose)
          cout << "- plot" << endl;

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        if (plotAnnotationsOnly) {
          // Plotting annotations only for the purpose of returning legends to a WMS
          // server front end.
          if (raster) {
            annotationRectangles = main_controller->plotAnnotations();
            annotationTransform = context.transform;
          } else if (json) {
            createJsonAnnotation();
          }
        } else
#endif
        {
          if (shape)
            main_controller->plot(true, false);
          else
            main_controller->plot(true, true);
        }

        // --------------------------------------------------------
      } else if (plottype == plot_vcross) {

        // Make Controller
        if (!main_controller) {
          MAKE_CONTROLLER
        }

        // -- vcross plot
        if (!vcrossmanager) {
          vcrossmanager = new VcrossManager(main_controller);
        }

        // set size of plotwindow
        if (!multiple_plots)
          VcrossPlot::setPlotWindow(xsize, ysize);
        else
          VcrossPlot::setPlotWindow(deltax, deltay);

        // extract options for plot
        parse_vcross_options(pcom);

        if (verbose)
          cout << "- sending plotCommands" << endl;
        if (vcross_optionschanged)
          vcrossmanager->getOptions()->readOptions(vcross_options);
        vcross_optionschanged = false;
        vcrossmanager->setSelection(vcross_data);

        if (ptime.undef()) {
          thetime = vcrossmanager->getTime();
          if (verbose)
            cout << "VCROSS has default time:" << thetime << endl;
        } else
          thetime = ptime;
        if (verbose)
          cout << "- plotting for time:" << thetime << endl;
        vcrossmanager->setTime(thetime);

        if (verbose)
          cout << "- setting cross-section:" << crossection << endl;
        if (crossection.exists())
          vcrossmanager->setCrossection(crossection);

        if (!raster && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_vcross, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
          startVideo(priop);
#endif
        }

        if (multiple_plots) {
          glViewport(margin + plotcol * (deltax + spacing), margin + plotrow
              * (deltay + spacing), deltax, deltay);
        }

        if (verbose)
          cout << "- plot" << endl;

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        if (canvasType == qt_qimage && raster && antialias)
          painter.setRenderHint(QPainter::Antialiasing);
        else if (svg || pdf)
          painter.setRenderHint(QPainter::Antialiasing);
#endif
        vcrossmanager->plot();

        // --------------------------------------------------------
      } else if (plottype == plot_vprof) {
        // Make Controller
        if (!main_controller) {
          MAKE_CONTROLLER
        }

        // -- vprof plot
        if (!vprofmanager) {
          vprofmanager = new VprofManager(main_controller);
        }

        // set size of plotwindow
        if (!multiple_plots)
          vprofmanager->setPlotWindow(xsize, ysize);
        else
          vprofmanager->setPlotWindow(deltax, deltay);

        // extract options for plot
        parse_vprof_options(pcom);

        if (verbose)
          cout << "- sending plotCommands" << endl;
        if (vprof_optionschanged)
          vprofmanager->getOptions()->readOptions(vprof_options);
        vprof_optionschanged = false;
        vprofmanager->setSelectedModels(vprof_models, false, vprof_plotobs,
            vprof_plotobs, vprof_plotobs);
        vprofmanager->setModel();

        if (ptime.undef()) {
          thetime = vprofmanager->getTime();
          if (verbose)
            cout << "VPROF has default time:" << thetime << endl;
        } else
          thetime = ptime;
        if (verbose)
          cout << "- plotting for time:" << thetime << endl;
        vprofmanager->setTime(thetime);

        if (verbose)
          cout << "- setting station:" << vprof_station << endl;
        if (vprof_station.exists())
          vprofmanager->setStation(vprof_station);

        if (!raster && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_vprof, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
          startVideo(priop);
#endif
        }

        if (multiple_plots) {
          glViewport(margin + plotcol * (deltax + spacing), margin + plotrow
              * (deltay + spacing), deltax, deltay);
        }

        if (verbose)
          cout << "- plot" << endl;

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        if (canvasType == qt_qimage && raster && antialias)
          painter.setRenderHint(QPainter::Antialiasing);
        else if (svg || pdf)
          painter.setRenderHint(QPainter::Antialiasing);
#endif
        vprofmanager->plot();

        // --------------------------------------------------------
      } else if (plottype == plot_spectrum) {
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
          cout << "- sending plotCommands" << endl;
        if (spectrum_optionschanged)
          spectrummanager->getOptions()->readOptions(spectrum_options);
        spectrum_optionschanged = false;
        spectrummanager->setSelectedModels(spectrum_models, spectrum_plotobs,
            false);
        spectrummanager->setModel();

        if (ptime.undef()) {
          thetime = spectrummanager->getTime();
          if (verbose)
            cout << "SPECTRUM has default time:" << thetime << endl;
        } else
          thetime = ptime;
        if (verbose)
          cout << "- plotting for time:" << thetime << endl;
        spectrummanager->setTime(thetime);

        if (verbose)
          cout << "- setting station:" << spectrum_station << endl;
        if (spectrum_station.exists())
          spectrummanager->setStation(spectrum_station);

        if (!raster && (!multiple_plots || multiple_newpage)) {
          startHardcopy(plot_spectrum, priop);
          multiple_newpage = false;
#ifdef VIDEO_EXPORT
        } else if (raster && raster_type == image_avi) {
          startVideo(priop);
#endif
        }

        if (multiple_plots) {
          glViewport(margin + plotcol * (deltax + spacing), margin + plotrow
              * (deltay + spacing), deltax, deltay);
        }

        if (verbose)
          cout << "- plot" << endl;

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        if (canvasType == qt_qimage && raster && antialias)
          painter.setRenderHint(QPainter::Antialiasing);
        else if (svg || pdf)
          painter.setRenderHint(QPainter::Antialiasing);
#endif
        spectrummanager->plot();

      }
      // --------------------------------------------------------

      //expand filename
      if (priop.fname.contains("%")) {
        priop.fname = thetime.format(priop.fname);
      }

      if (use_double_buffer) {
        // Double-buffering
        if (canvasType == x_pixmap) {
#ifdef USE_XLIB
          glXSwapBuffers(dpy, pix);
#endif
        } else if (canvasType == glx_pixelbuffer) {
#ifdef USE_XLIB
#ifdef GLX_VERSION_1_3
          glXSwapBuffers(dpy, pbuf);
#endif
#endif
        } else if (canvasType == qt_glpixelbuffer) {
          cerr
              << "WARNING! double buffer swapping not implemented for qt_glpixelbuffer"
              << endl;
        }
      }

#ifdef VIDEO_EXPORT
      if (!raster || !raster_type == image_avi) {
        endVideo();
      }
#endif

      if (raster) {

        if (verbose)
          cout << "- Preparing for raster output" << endl;
        glFlush();

#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
        if (canvasType == qt_glpixelbuffer) {
          if (qpbuffer == 0) {
            cerr << " ERROR. when saving image - qpbuffer is NULL" << endl;
          } else {
            const QImage image = qpbuffer->toImage();

            if (verbose) {
              cout << "- Saving image to:" << priop.fname;
              cout.flush();
            }

            bool result = false;

            if (raster_type == image_png || raster_type == image_unknown) {
              result = image.save(priop.fname.c_str());
              cerr << "--------- write_png: " << priop.fname << endl;
#ifdef VIDEO_EXPORT
            } else if (raster_type == image_avi) {
              result = addVideoFrame(image);
              cerr << "--------- write_avi_frame: " << priop.fname << endl;
#endif
            }

            if (verbose) {
              cout << " .." << miString(result ? "Ok" : " **FAILED!**") << endl;
            } else if (!result) {
              cerr << " ERROR, saving image to:" << priop.fname << endl;
            }
          }
        } else if (canvasType == qt_glframebuffer) {
          if (qfbuffer == 0) {
            cerr << " ERROR. when saving image - qfbuffer is NULL" << endl;
          } else {
            const QImage image = qfbuffer->toImage();

            if (verbose) {
              cout << "- Saving image to:" << priop.fname;
              cout.flush();
            }

            bool result = false;

            if (raster_type == image_png || raster_type == image_unknown) {
              result = image.save(priop.fname.c_str());
#ifdef VIDEO_EXPORT
            } else if (raster_type == image_avi) {
              result = addVideoFrame(image);
#endif
            }
            cerr << "--------- write_png: " << priop.fname << endl;

            if (verbose) {
              cout << " .." << miString(result ? "Ok" : " **FAILED!**") << endl;
            } else if (!result) {
              cerr << " ERROR, saving image to:" << priop.fname << endl;
            }
          }
        }
#else
        if (canvasType == qt_qimage && raster) {
          context.end();
          painter.end();

          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
            image = image.copy(-ox, -oy, xsize, ysize);
          }

          milogger::LogHandler::getInstance()->setObjectName("diana.bdiana.parseAndProcess");

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
            COMMON_LOG::getInstance("common").infoStream() << "# vvv Empty plot (begin)";

          QStringList imageText;
          for (unsigned int i = 0; i < lines.size(); ++i) {
            image.setText(QString::number(i), QString::fromStdString(lines[i]));
            COMMON_LOG::getInstance("common").infoStream() << lines[i];
          }

          if (empty)
            COMMON_LOG::getInstance("common").infoStream() << "# ^^^ Empty plot (end)";
          image.save(QString::fromStdString(priop.fname));
        }
#endif
        else {
          imageIO::Image_data img;
          img.width = xsize;
          img.height = ysize;
          img.filename = priop.fname;
          int npixels;

          npixels = img.width * img.height;
          img.nchannels = 4;
          img.data = new unsigned char[npixels * img.nchannels];

          glReadPixels(0, 0, img.width, img.height, GL_RGBA, GL_UNSIGNED_BYTE,
              img.data);

          int result = 0;

          // save as PNG -----------------------------------------------
          if (raster_type == image_png || raster_type == image_unknown) {
            if (verbose) {
              cout << "- Saving PNG-image to:" << img.filename;
              cout.flush();
            }
            result = imageIO::write_png(img);
#ifdef VIDEO_EXPORT
          } else if (raster_type == image_avi) {
            if (verbose) {
              cout << "- Adding image to:" << img.filename;
              cout.flush();
            }
//            result = addVideoFrame(img);
#endif
          }
          if (verbose)
            cout << " .." << miString(result ? "Ok" : " **FAILED!**") << endl;
          else if (!result)
            cerr << " ERROR, saving PNG-image to:" << img.filename << endl;
          // -------------------------------------------------------------

        }

      } else if (shape) { // Only shape output

          if (priop.fname.contains("tmp_diana")) {
                  cout << "Using shape option without file name, it will be created automatically" << endl;
          } else {
                  cout << "Using shape option with given file name : " << priop.fname << endl;
                  }
          // first stop postscript-generation
          endHardcopy(plot_none);

          // Anything more to be done here ???

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
      } else if (svg) {

          ensureNewContext();

          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
          }

          QSvgGenerator svgFile;
          svgFile.setFileName(QString::fromStdString(priop.fname));
          svgFile.setSize(QSize(xsize, ysize));
          svgFile.setViewBox(QRect(0, 0, xsize, ysize));
          painter.begin(&svgFile);
          painter.drawPicture(ox, oy, picture);
          painter.end();

      } else if (pdf) {

          ensureNewContext();

          int ox = 0, oy = 0;
          if (plotAnnotationsOnly) {
            getAnnotationsArea(ox, oy, xsize, ysize);
          }

          QPrinter printer;
          printer.setOutputFileName(QString::fromStdString(priop.fname));
          printer.setPaperSize(QSizeF(xsize, ysize), QPrinter::DevicePixel);
          painter.begin(&printer);
          painter.drawPicture(ox, oy, picture);
          painter.end();
      } else if (json) {

        ensureNewContext();

        QFile outputFile(QString::fromStdString(priop.fname));
        if (outputFile.open(QFile::WriteOnly)) {
          outputFile.write("{\n");
          unsigned int i = 0;
          for (map<miString, map<miString,miString> >::iterator iti = outputTextMaps.begin(); iti != outputTextMaps.end(); ++iti, ++i) {
            outputFile.write("  \"");
            outputFile.write(QString::fromStdString(iti->first).toUtf8());
            outputFile.write("\": {\n");
            map<miString,miString> textMap = iti->second;
            unsigned int j = 0;
            for (map<miString,miString>::iterator itj = textMap.begin(); itj != textMap.end(); ++itj, ++j) {
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
#endif
      } else { // PostScript only
        if (toprinter) { // automatic print of each page
          // Note that this option works bad for multi-page output:
          // use PRINT_DOCUMENT instead
          if (!priop.printer.exists()) {
            cerr << " ERROR, printing document:" << priop.fname
                << "  Printer not defined!" << endl;
            continue;
          }
          // first stop postscript-generation
          endHardcopy(plot_none);
          multiple_newpage = true;

          miString command = printman->printCommand();
          priop.numcopies = 1;

          printman->expandCommand(command, priop);

          if (verbose)
            cout << "- Issuing print command:" << command << endl;
          int res = system(command.c_str());
          if (verbose)
            cout << " result:" << res << endl;
        }
      }

      continue;

    } else if (lines[k].downcase() == com_time || lines[k].downcase()
        == com_level) {

      // read setup
      if (!setupread) {
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          cerr << "ERROR, no setupinformation..exiting" << endl;
          return 99;
        }
      }

      // Make Controller
      if (!main_controller) {
        MAKE_CONTROLLER
      }

      if (lines[k].downcase() == com_time) {

        if (verbose)
          cout << "- finding times" << endl;

        //Find ENDTIME
        vector<miString> pcom;
        for (int i = k + 1; i < linenum && lines[i].downcase() != com_endtime; i++, k++)
          pcom.push_back(lines[i]);
        k++;

        // Perform a quick check for newly arrived data.
        main_controller->getFieldManager()->updateSources();

        // necessary to set time before plotCommands()..?
        thetime = miTime::nowTime();
        main_controller->setPlotTime(thetime);

        if (verbose)
          cout << "- sending plotCommands" << endl;
        main_controller->plotCommands(pcom);

        set<miTime> okTimes;
        set<miTime> constTimes;
        main_controller->getCapabilitiesTime(okTimes, constTimes, pcom,
            time_options == "union");

        // open filestream
        ofstream file(priop.fname.c_str());
        if (!file) {
          cerr << "ERROR OPEN (WRITE) " << priop.fname << endl;
          return 1;
        }
        file << "PROG" << endl;
        set<miTime>::iterator p = okTimes.begin();
        for (; p != okTimes.end(); p++) {
          file << (*p).format(time_format) << endl;
        }
        file << "CONST" << endl;
        p = constTimes.begin();
        for (; p != constTimes.end(); p++) {
          file << (*p).format(time_format) << endl;
        }
        cerr << endl;
        file.close();

      } else if (lines[k].downcase() == com_level) {

        if (verbose)
          cout << "- finding levels" << endl;

        //Find ENDLEVEL
        vector<miString> pcom;
        for (int i = k + 1; i < linenum && lines[i].downcase() != com_endlevel; i++, k++)
          pcom.push_back(lines[i]);
        k++;

        vector<miString> levels;

        // open filestream
        ofstream file(priop.fname.c_str());
        if (!file) {
          cerr << "ERROR OPEN (WRITE) " << priop.fname << endl;
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

    } else if (lines[k].downcase() == com_print_document) {
      if (raster) {
        cerr << " ERROR, trying to print raster-image!" << endl;
        continue;
      }
      if (!priop.printer.exists()) {
        cerr << " ERROR, printing document:" << priop.fname
            << "  Printer not defined!" << endl;
        continue;
      }
      // first stop postscript-generation
      endHardcopy(plot_none);
      multiple_newpage = true;

      miString command = printman->printCommand();
      priop.numcopies = 1;

      printman->expandCommand(command, priop);

      if (verbose)
        cout << "- Issuing print command:" << command << endl;
      int res = system(command.c_str());
      if (verbose)
        cout << "Result:" << res << endl;

      continue;

    } else if (lines[k].downcase() == com_wait_for_commands) {
      /*
       ====================================
       ========= TEST - feed commands from files
       ====================================
       */

      if (!command_path.exists()) {
        cerr << "ERROR, wait_for_commands found, but command_path not set"
            << endl;
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

      cerr << "================ WAIT FOR COMMANDS, TIME is:" << nowtime
          << ", seconds spent on previous command(s):" << diff << endl;

      miString pattern = command_path;
      vector<miString> newlines;
      miString waitline = com_wait_for_commands;

      glob_t globBuf;
      int number_of_files = 0;
      while (number_of_files == 0) {
        glob(pattern.c_str(), 0, 0, &globBuf);
        number_of_files = globBuf.gl_pathc;
        if (number_of_files == 0) {
          globfree_cache(&globBuf);
          pu_sleep(1);
        }
      }

      nowtime = miTime::nowTime();
      prev_iclock = clock();
      cerr << "================ FOUND COMMAND-FILE(S), TIME is:" << nowtime
          << endl;

      vector<miString> filenames;

      //loop over files
      for (int ij = 0; ij < number_of_files; ij++) {
        miString filename = globBuf.gl_pathv[ij];
        cerr << "==== Reading file:" << filename << endl;
        filenames.push_back(filename);
        ifstream file(filename.c_str());
        while (file) {
          miString str;
          if (getline(file, str)) {
            str.trim();
            if (str.length() > 0 && str[0] != '#') {
              if (str.downcase().contains(com_wait_end))
                waitline = ""; // blank out waitline
              else
                newlines.push_back(str);
            }
          }
        }
      }
      globfree_cache(&globBuf);
      // remove processed files
      for (unsigned int ik = 0; ik < filenames.size(); ik++) {
        ostringstream ost;
        ost << "rm -f " << filenames[ik];
        std::string command = ost.str();

        cerr << "==== Cleaning up with:" << command << endl;
        int res = system(command.c_str());

        if (res != 0){
          cerr << "Command:" << command << " failed" << endl;
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

      cerr << "================ EXECUTING COMMANDS" << endl;
      continue;
      // =============================================================

    } else if (lines[k].downcase() == com_field_files) {
      // Read lines until </FIELD_FILES> is read.

      int kk;
      for (kk = k + 1; kk < linenum; ++kk) {
        if (lines[kk].downcase() != com_field_files_end)
           extra_field_lines.push_back(lines[kk]);
        else
          break;
      }

      if (kk < linenum)
        k = kk;         // skip the </FIELD_FILES> line
      else {
          cerr << "ERROR, no " << com_field_files_end << " found:" << lines[k]
               << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      }
      continue;

    } else if (lines[k].downcase() == com_field_files_end) {
      cerr << "WARNING, " << com_field_files_end << " found:" << lines[k]
           << " Linenumber:" << linenumbers[k] << endl;
      continue;

    } else if (lines[k].downcase() == com_describe) {

      if (verbose)
        cout << "- finding information about data sources" << endl;

      //Find ENDDESCRIBE
      vector<miString> pcom;
      for (int i = k + 1; i < linenum && lines[i].downcase() != com_describe_end; i++, k++)
        pcom.push_back(lines[i]);
      k++;

      // Make Controller
      if (!main_controller) {
        MAKE_CONTROLLER
      }

      // Perform a quick check for newly arrived data.
      main_controller->getFieldManager()->updateSources();

      if (verbose)
        cout << "- sending plotCommands" << endl;
      main_controller->plotCommands(pcom);

      vector<miTime> fieldtimes, sattimes, obstimes, objtimes, ptimes;
      main_controller->getPlotTimes(fieldtimes, sattimes, obstimes, objtimes,
          ptimes);

      if (ptime.undef()) {
        if (use_nowtime)
          thetime = selectNowTime(fieldtimes, sattimes, obstimes, objtimes, ptimes);
        else if (fieldtimes.size() > 0)
          thetime = fieldtimes[fieldtimes.size() - 1];
        else if (sattimes.size() > 0)
          thetime = sattimes[sattimes.size() - 1];
        else if (obstimes.size() > 0)
          thetime = obstimes[obstimes.size() - 1];
        else if (objtimes.size() > 0)
          thetime = objtimes[objtimes.size() - 1];
        else if (ptimes.size() > 0)
          thetime = ptimes[ptimes.size() - 1];
      } else
        thetime = ptime;

      if (verbose)
        cout << "- plotting for time:" << thetime << endl;
      main_controller->setPlotTime(thetime);

      if (verbose)
        cout << "- updatePlots" << endl;
      if (main_controller->updatePlots()) {

          // open filestream
          ofstream file(priop.fname.c_str());
          if (!file) {
            cerr << "ERROR OPEN (WRITE) " << priop.fname << endl;
            return 1;
          }

          vector<FieldPlot*> fieldPlots = main_controller->getFieldPlots();
          std::set<std::string> fieldPatterns;

          for (vector<FieldPlot*>::iterator it = fieldPlots.begin(); it != fieldPlots.end(); ++it) {
            miutil::miString modelName = (*it)->getModelName();

            FieldManager* fieldManager = main_controller->getFieldManager();
            FieldSource* fieldSource = fieldManager->getFieldSource(modelName, true);
            if (fieldSource) {
              std::vector<miutil::miString> fileNames = fieldSource->getFileNames();
              fieldPatterns.insert(fileNames.begin(), fileNames.end());
            } else {
              GridCollection* gridCollection = fieldManager->getGridCollection(modelName, "", true);
              if (gridCollection) {
                std::vector<std::string> fileNames = gridCollection->getRawSources();
                fieldPatterns.insert(fileNames.begin(), fileNames.end());
              }
            }
          }

          map<miutil::miString, map<miutil::miString,SatManager::subProdInfo> > satProducts = main_controller->getSatelliteManager()->getProductsInfo();
          set<miutil::miString> satPatterns;
          set<miutil::miString> satFiles;

          vector<SatPlot*> satellitePlots = main_controller->getSatellitePlots();
          for (vector<SatPlot*>::iterator it = satellitePlots.begin(); it != satellitePlots.end(); ++it) {
              Sat* sat = (*it)->satdata;
              if (satProducts.find(sat->satellite) != satProducts.end() && satProducts[sat->satellite].find(sat->filetype) != satProducts[sat->satellite].end()) {
                  SatManager::subProdInfo satInfo = satProducts[sat->satellite][sat->filetype];
                  for (vector<SatFileInfo>::iterator itsf = satInfo.file.begin(); itsf != satInfo.file.end(); ++itsf) {
                    satFiles.insert(itsf->name);
                    if (itsf->name == sat->actualfile) {
                        for (vector<miutil::miString>::iterator itp = satInfo.pattern.begin(); itp != satInfo.pattern.end(); ++itp)
                            satPatterns.insert(*itp);
                    }
                  }
              }
          }

          map<miutil::miString,ObsManager::ProdInfo> obsProducts = main_controller->getObservationManager()->getProductsInfo();
          set<miutil::miString> obsPatterns;
          set<miutil::miString> obsFiles;

          vector<ObsPlot*> obsPlots = main_controller->getObsPlots();
          for (vector<ObsPlot*>::iterator it = obsPlots.begin(); it != obsPlots.end(); ++it) {
              vector<miutil::miString> obsFileNames = (*it)->getFileNames();
              for (vector<miutil::miString>::iterator itf = obsFileNames.begin(); itf != obsFileNames.end(); ++itf) {
                  obsFiles.insert(*itf);

                  for (map<miutil::miString,ObsManager::ProdInfo>::iterator ito = obsProducts.begin(); ito != obsProducts.end(); ++ito) {
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
          for (set<miutil::miString>::iterator it = satFiles.begin(); it != satFiles.end(); ++it)
            file << *it << endl;
          for (set<miutil::miString>::iterator it = satPatterns.begin(); it != satPatterns.end(); ++it)
            file << *it << endl;
          for (set<miutil::miString>::iterator it = obsFiles.begin(); it != obsFiles.end(); ++it)
            file << *it << endl;
          for (set<miutil::miString>::iterator it = obsPatterns.begin(); it != obsPatterns.end(); ++it)
            file << *it << endl;

          file.close();
      }

      continue;

    } else if (lines[k].downcase() == com_describe_end) {
      cerr << "WARNING, " << com_describe_end << " found:" << lines[k]
           << " Linenumber:" << linenumbers[k] << endl;
      continue;
    }

    // all other options on the form KEY=VALUE

    vs = lines[k].split("=");
    int nv = vs.size();
    if (nv < 2) {
      cerr << "ERROR, unknown command:" << lines[k] << " Linenumber:"
          << linenumbers[k] << endl;
      return 1;
    }
    miString key = vs[0].downcase();
    int ieq = lines[k].find_first_of("=");
    miString value = lines[k].substr(ieq + 1, lines[k].length() - ieq - 1);
    key.trim();
    value.trim();

    if (key == com_setupfile) {
      if (setupread) {
        cerr
            << "WARNING, setupfile overrided by command line option. Linenumber:"
            << linenumbers[k] << endl;
        //      return 1;
      } else {
        setupfile = value;
        setupread = readSetup(setupfile, *printman);
        if (!setupread) {
          cerr << "ERROR, no setupinformation..exiting" << endl;
          return 99;
        }
      }

    } else if (key == com_command_path) {
      command_path = value;

    } else if (key == com_fifo_name) {
      fifo_name = value;

    } else if (key == com_buffersize) {
      vvs = value.split("x");
      if (vvs.size() < 2) {
        cerr << "ERROR, buffersize should be WxH:" << lines[k]
            << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      }
      int tmp_xsize = atoi(vvs[0].cStr());
      int tmp_ysize = atoi(vvs[1].cStr());

      // if requested buffersize identical to current: do nothing
      if (buffermade && tmp_xsize == xsize && tmp_ysize == ysize)
        continue;

      xsize = tmp_xsize;
      ysize = tmp_ysize;

      // first stop ongoing postscript sessions
      endHardcopy(plot_none);

      // create canvas
      if (canvasType == x_pixmap) {
#ifdef USE_XLIB
        // delete old pixmaps
        if (buffermade) {
          if (pix) {
            glXDestroyGLXPixmap(dpy, pix);
          }
          if (pixmap) {
            XFreePixmap(dpy, pixmap);
          }
        }

        //cout << "- Creating X pixmap.." << endl;
        pixmap = XCreatePixmap(dpy, RootWindow(dpy, pdvi->screen),
            xsize, ysize, pdvi->depth);
        if (!pixmap) {
          cerr << "ERROR, could not create X pixmap" << endl;
          return 1;
        }

        //cout << "- Creating GLX pixmap.." << endl;
        pix = glXCreateGLXPixmap(dpy, pdvi, pixmap);
        if (!pix) {
          cerr << "ERROR, could not create GLX pixmap" << endl;
          return 1;
        }

        glXMakeCurrent(dpy, pix, cx);
#endif
      } else if (canvasType == glx_pixelbuffer) {

#ifdef USE_XLIB
#ifdef GLX_VERSION_1_3
        // delete old PixelBuffer
        if (buffermade && pbuf) {
          glXDestroyPbuffer(dpy, pbuf);
        }

        int nelements;
        GLXFBConfig* pbconfig = glXChooseFBConfig(dpy, DefaultScreen(dpy), (use_double_buffer ? dblBuf : snglBuf), &nelements);

        if (nelements == 0) {
          cerr << "glXChooseFBConfig returned no configurations" << endl;
          exit(1);
        }

        static int pbufAttr[5];
        int n = 0;
        pbufAttr[n] = GLX_PBUFFER_WIDTH;
        n++;
        pbufAttr[n] = xsize;
        n++;
        pbufAttr[n] = GLX_PBUFFER_HEIGHT;
        n++;
        pbufAttr[n] = ysize;
        n++;
        pbufAttr[n] = None;
        n++;

        //cout << "- Creating GLX pbuffer.." << endl;
        pbuf = glXCreatePbuffer(dpy, pbconfig[0], pbufAttr);
        if (!pbuf) {
          cerr << "ERROR, could not create GLX pbuffer" << endl;
          return 1;
        }

        pdvi = glXGetVisualFromFBConfig(dpy, pbconfig[0]);
        if (!pdvi) {
          cerr << "ERROR, could not get visual from FBConfig" << endl;
          return 1;
        }

        //cout << "- Create glx rendering context.." << endl;
        cx = glXCreateContext(dpy, pdvi,// display and visual
            0, 0); // sharing and direct rendering
        if (!cx) {
          cerr << "ERROR, could not create rendering context" << endl;
          return 1;
        }

        glXMakeContextCurrent(dpy, pbuf, pbuf, cx);
#endif
#endif
      } else if (canvasType == qt_glpixelbuffer) {

#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
        // delete old pixmaps
        if (buffermade && qpbuffer) {
          delete qpbuffer;
        }

        QGLFormat format = QGLFormat::defaultFormat();
        //TODO: any specific format specifications?
        qpbuffer = new QGLPixelBuffer(xsize, ysize, format, 0);

        qpbuffer->makeCurrent();
        main_controller->restartFontManager();

      } else if (canvasType == qt_glframebuffer) {
          // delete old pixmaps
          if (buffermade && qfbuffer) {
            delete qfbuffer;
          }

          //TODO -- need to set more format attributes than set in the qtwidget context?
          //GLenum target = GL_TEXTURE_2D;
          //QGLFramebufferObjectFormat formatFB;
          //qfbuffer = new QGLFramebufferObject(xsize, ysize, formatFB);
          qfbuffer = new QGLFramebufferObject(xsize, ysize);
          qfbuffer->bind();
          //qfbuffer->release();
#else
      } else if (canvasType == qt_qimage) {
        ensureNewContext();

        image = QImage(xsize, ysize, QImage::Format_ARGB32_Premultiplied);
        image.fill(qRgba(0, 0, 0, 0));
        painter.begin(&image);
        context.begin(&painter);
#endif
      }
      glShadeModel(GL_FLAT);

      glViewport(0, 0, xsize, ysize);

      // for multiple plots
      priop.viewport_x0 = 0;
      priop.viewport_y0 = 0;
      priop.viewport_width = xsize;
      priop.viewport_height = ysize;

      buffermade = true;

    } else if (key == com_papersize) {
      vvvs = value.split(","); // could contain both pagesize and papersize
      for (unsigned int l = 0; l < vvvs.size(); l++) {
        if (vvvs[l].contains("x")) {
          vvs = vvvs[l].split("x");
          if (vvs.size() < 2) {
            cerr
                << "ERROR, papersize should be WxH or WxH,PAPERTYPE or PAPERTYPE:"
                << lines[k] << " Linenumber:" << linenumbers[k] << endl;
            return 1;
          }
          priop.papersize.hsize = atoi(vvs[0].cStr());
          priop.papersize.vsize = atoi(vvs[1].cStr());
          priop.usecustomsize = true;
        } else {
          priop.pagesize = printman->getPage(vvvs[l]);
        }
      }

    } else if (key == com_filname) {
      if (!value.exists()) {
        cerr << "ERROR, illegal filename in:" << lines[k] << " Linenumber:"
            << linenumbers[k] << endl;
        return 1;
      } else
        priop.fname = value;

    } else if (key == com_toprinter) {
      toprinter = (value.downcase() == "yes");

    } else if (key == com_printer) {
      priop.printer = value;

    } else if (key == com_output) {
      value = value.downcase();
      if (value == "postscript") {
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

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
        ensureNewContext();

        image = QImage(xsize, ysize, QImage::Format_ARGB32_Premultiplied);
        image.fill(qRgba(0, 0, 0, 0));
        painter.begin(&image);
        context.begin(&painter);
#endif

      } else if (value == "shp") {
        shape = true;
      } else if (value == "avi") {
        raster = true;
        raster_type = image_avi;
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
      } else if (value == "svg") {
        ensureNewContext();
        svg = true;
        picture.setBoundingRect(QRect(0, 0, xsize, ysize));
        painter.begin(&picture);
        context.begin(&painter);
      } else if (value == "pdf") {
        ensureNewContext();
        pdf = true;
        picture.setBoundingRect(QRect(0, 0, xsize, ysize));
        painter.begin(&picture);
        context.begin(&painter);
      } else if (value == "json") {
        ensureNewContext();
        raster = false;
        json = true;
        outputTextMaps.clear();
        picture.setBoundingRect(QRect(0, 0, xsize, ysize));
        painter.begin(&picture);
        context.begin(&painter);
#endif
      } else {
        cerr << "ERROR, unknown output-format:" << lines[k] << " Linenumber:"
            << linenumbers[k] << endl;
        return 1;
      }
      if (raster && multiple_plots) {
        cerr
            << "ERROR, multiple plots and raster-output can not be used together: "
            << lines[k] << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      }
      if (raster || shape) {
        // first stop ongoing postscript sessions
        endHardcopy(plot_none);
      }

    } else if (key == com_colour) {
      if (value.downcase() == "greyscale")
        priop.colop = d_print::greyscale;
      else
        priop.colop = d_print::incolour;

    } else if (key == com_drawbackground) {
      priop.drawbackground = (value.downcase() == "yes");

    } else if (key == com_orientation) {
      value = value.downcase();
      if (value == "landscape")
        priop.orientation = d_print::ori_landscape;
      else if (value == "portrait")
        priop.orientation = d_print::ori_portrait;
      else
        priop.orientation = d_print::ori_automatic;

    } else if (key == com_addhour) {
      if (!fixedtime.undef()) {
        ptime = fixedtime;
        ptime.addHour(atoi(value.cStr()));
      }

    } else if (key == com_addminute) {
      if (!fixedtime.undef()) {
        ptime = fixedtime;
        ptime.addMin(atoi(value.cStr()));
      }

    } else if (key == com_settime) {
      if (miTime::isValid(value)) {
        fixedtime = ptime = miTime(value);
      }

    } else if (key == com_archive) {
      useArchive = (value.downcase() == "on");

    } else if (key == com_keepplotarea) {
      keeparea = (value.downcase() == "yes");

#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
    } else if (key == com_plotannotationsonly) {
      plotAnnotationsOnly = (value.downcase() == "yes");
#endif

    } else if (key == com_antialiasing) {
      antialias = (value.downcase() == "yes");

    } else if (key == com_multiple_plots) {
      if (raster) {
        cerr
            << "ERROR, multiple plots and raster-output can not be used together: "
            << lines[k] << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      }
      if (value.downcase() == "off") {
        multiple_newpage = false;
        multiple_plots = false;
        glViewport(0, 0, xsize, ysize);

      } else {
        vector<miString> v1 = value.split(",");
        if (v1.size() < 2) {
          cerr << "WARNING, illegal values to multiple.plots:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          multiple_plots = false;
          return 1;
        }
        numrows = atoi(v1[0].cStr());
        numcols = atoi(v1[1].cStr());
        if (numrows < 1 || numcols < 1) {
          cerr << "WARNING, illegal values to multiple.plots:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          multiple_plots = false;
          return 1;
        }
        float fmargin = 0.0;
        float fspacing = 0.0;
        if (v1.size() > 2) {
          fspacing = atof(v1[2].cStr());
          if (fspacing >= 100 || fspacing < 0) {
            cerr << "WARNING, illegal value for spacing:" << lines[k]
                << " Linenumber:" << linenumbers[k] << endl;
            fspacing = 0;
          }
        }
        if (v1.size() > 3) {
          fmargin = atof(v1[3].cStr());
          if (fmargin >= 100 || fmargin < 0) {
            cerr << "WARNING, illegal value for margin:" << lines[k]
                << " Linenumber:" << linenumbers[k] << endl;
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
          cout << "Starting multiple_plot, rows:" << numrows << " , columns: "
              << numcols << endl;
      }

    } else if (key == com_plotcell) {
      if (!multiple_plots) {
        cerr << "ERROR, multiple plots not initialised:" << lines[k]
            << " Linenumber:" << linenumbers[k] << endl;
        return 1;
      } else {
        vector<miString> v1 = value.split(",");
        if (v1.size() != 2) {
          cerr << "WARNING, illegal values to plotcell:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          return 1;
        }
        plotrow = atoi(v1[0].cStr());
        plotcol = atoi(v1[1].cStr());
        if (plotrow < 0 || plotrow >= numrows || plotcol < 0 || plotcol
            >= numcols) {
          cerr << "WARNING, illegal values to plotcell:" << lines[k]
              << " Linenumber:" << linenumbers[k] << endl;
          return 1;
        }
        // row 0 should be on top of page
        plotrow = (numrows - 1 - plotrow);
      }

    } else if (key == com_trajectory) {
      if (value.downcase() == "on") {
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
      time_options = value.downcase();

    } else if (key == com_time_format) {
      time_format = value;

    } else {
      cerr << "WARNING, unknown command:" << lines[k] << " Linenumber:"
          << linenumbers[k] << endl;
    }
  }

  // finish off any dangling postscript-sessions
  endHardcopy(plot_none);

  return 0;
}

void doWork();
int dispatchWork(const std::string &file);

/*
 =================================================================
 BDIANA - BATCH PRODUCTION OF DIANA GRAPHICAL PRODUCTS
 =================================================================
 */
int main(int _argc, char** _argv)
{
  return 17; // simulate premature return to test Jenkins
  diOrderBook *orderbook = NULL;
  miString xhost = ":0.0"; // default DISPLAY
  miString sarg;
  int port;
  milogger::LogHandler * plog = NULL;


  // get the DISPLAY variable
  char * ctmp = getenv("DISPLAY");
  if (ctmp) {
    xhost = ctmp;
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

  vector<miString> ks;
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

    } else if (sarg == "-v") {
      verbose = true;

    } else if (sarg == "-signal") {
      if (orderbook != NULL) {
        cerr << "ERROR, can't have both -address and -signal" << endl;
        return 1;
      }
      wait_for_signals = true;

    } else if (sarg == "-example") {
      printUsage(true);

    } else if (sarg == "-use_pbuffer") {
      canvasType = glx_pixelbuffer;

    } else if (sarg == "-use_pixmap") {
      canvasType = x_pixmap;

    } else if (sarg == "-use_qtgl") {
      canvasType = qt_glpixelbuffer;

    } else if (sarg == "-use_qtgl_fb") {
      canvasType = qt_glframebuffer;

    } else if (sarg == "-use_qimage") {
      canvasType = qt_qimage;

    } else if (sarg == "-use_singlebuffer") {
      use_double_buffer = false;

    } else if (sarg == "-use_doublebuffer") {
      use_double_buffer = true;

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
      ks = sarg.split("=");
      if (ks.size() == 2) {
        ks = ks[1].split(":");
        if (ks.size() == 2) {
          if (ks[1].isNumber()) {
            port = ks[1].toInt();
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
      ks = sarg.split("=");
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


  // prepare font-pack for display
  FontManager::set_display_name(xhost);

  if (!batchinput.empty() && !batchinput.exists())
    printUsage(false);
  // Init loghandler with debug level
  plog = milogger::LogHandler::initLogHandler( 2, "" );
  plog->setObjectName("diana.bdiana.main");
  COMMON_LOG::getInstance("common").infoStream() << argv[0].toStdString() << " : DIANA batch version " << VERSION;

#ifndef USE_XLIB
  if (canvasType == x_pixmap || canvasType == glx_pixelbuffer) {
          COMMON_LOG::getInstance("common").warnStream() << "===================================================" << "\n"
        << " WARNING !" << "\n"
        << " X pixmaps or GLX pixelbuffers not supported" << "\n"
        << " Forcing use of default canvas" << "\n"
        << "===================================================";
    canvasType = default_canvas;
  }
#endif

#ifndef GLX_VERSION_1_3
  if (canvasType == glx_pixelbuffer) {
          COMMON_LOG::getInstance("common").warnStream() << "===================================================" << "\n"
        << " WARNING !" << "\n"
        << " This version of GLX does not support PixelBuffers." << "\n"
        << " Forcing use of default canvas" << "\n"
        << "===================================================";
    canvasType = default_canvas;
  }
#endif

#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
  if (canvasType == qt_glpixelbuffer) {
    cerr <<"qt_glpixelbuffer"<<endl;
    if (!QGLFormat::hasOpenGL() || !QGLPixelBuffer::hasOpenGLPbuffers()) {
      COMMON_LOG::getInstance("common").errorStream() << "This system does not support OpenGL pbuffers.";
      return 1;
    }
  } else if (canvasType == qt_glframebuffer) {
    if (!QGLFormat::hasOpenGL() || !QGLFramebufferObject::hasOpenGLFramebufferObjects()) {
      cerr << "This system does not support OpenGL framebuffers." << endl;
      return 1;
    } else {
      //Create QGL widget as a rendering context
      QGLFormat format = QGLFormat::defaultFormat();
      format.setAlpha(true);
      format.setDirectRendering(true);
      if (use_double_buffer) {
       format.setDoubleBuffer(true);
      }
      qwidget = new QGLWidget(format);
      qwidget->makeCurrent();

      //qwidget->doneCurrent(); // Probably not needed qwidget is deleted furthher down in the code

    }
  }
#endif

  if (canvasType == x_pixmap || canvasType == glx_pixelbuffer) {
#ifdef USE_XLIB

    dpy = XOpenDisplay(xhost.cStr());
    if (!dpy) {
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, could not open X-display:" << xhost;
      return 1;
    }
#endif
  }

  if (canvasType == x_pixmap) {
#ifdef USE_XLIB
    // find an OpenGL-capable RGB visual with depth buffer
    pdvi = glXChooseVisual(dpy, DefaultScreen(dpy),
        (use_double_buffer ? dblBuf : snglBuf));
    if (!pdvi) {
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, no RGB visual with depth buffer";
      return 1;
    }

    // Create glx rendering context..
    cx = glXCreateContext(dpy, pdvi,// display and visual
        0, 0); // sharing and direct rendering
    if (!cx) {
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, could not create rendering context";
      return 1;
    }
#endif
  }

  priop.fname = "tmp_diana.ps";
  priop.colop = d_print::greyscale;
  priop.drawbackground = false;
  priop.orientation = ori_automatic;
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
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, unable to read setup:" << setupfile;
      return 99;
    }
  }

  /*
   Read initial input and process commands...
   */
  if (!batchinput.empty()) {
    cerr << "Reading input file: " << batchinput.c_str() << endl;
    ifstream is(batchinput.c_str());
    if (!is) {
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, cannot open inputfile " << batchinput;
      return 99;
    }
    int res = parseAndProcess(is);
#if defined(Q_WS_QWS) || defined(Q_WS_QPA)
    if (painter.isActive())
        painter.end();
#endif
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
        COMMON_LOG::getInstance("common").infoStream() << "PID: " << getpid();

    fs.open("bdiana.pid");

    if (!fs) {
        COMMON_LOG::getInstance("common").errorStream()<< "ERROR, can't open file <bdiana.pid>!";
      return 1;
    }

    fs << getpid() << endl;
    fs.close();

    while (!quit) {
      application->processEvents(); // do we actually care in this case?
      switch (waitOnSignal(10, timeout)) {
      case -1:
          COMMON_LOG::getInstance("common").infoStream() << "ERROR, a waitOnSignal error occured!";
        quit = true;
        break;
      case 0:
        if (verbose)
                COMMON_LOG::getInstance("common").infoStream() << "SIGUSR1: received!";
        doWork();
        break;
      case 1:
        if (!timeout) {
                COMMON_LOG::getInstance("common").infoStream()<< "SIGTERM, SIGINT: received!";
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
        cerr << "processing order..." << endl;
        parseAndProcess(is);
        cerr << "done" << endl;
        if (order) // may have been deleted (if the client disconnected)
          order->signalCompletion();
        else
          cerr << "diWorkOrder went away" << endl;
        application->processEvents();
      } else {
        cerr << "waiting" << endl;
        application->processEvents(QEventLoop::WaitForMoreEvents);
      }
    }
  } else if (batchinput.empty()) {
    cerr << "Neither -address nor -signal was specified" << endl;
  }

  // clean up structures
#ifdef VIDEO_EXPORT
  if(movieMaker) endVideo();
#endif // VIDEO_EXPORT

#ifdef USE_XLIB
  if (pix) {
    glXDestroyGLXPixmap(dpy, pix);
  }
  if (pixmap) {
    XFreePixmap(dpy, pixmap);
  }
#endif

#if !defined(Q_WS_QWS) && !defined(Q_WS_QPA)
  if (qpbuffer) {
    delete qpbuffer;
  }
  if (qfbuffer) {
    delete qfbuffer;
  }
  if (qwidget) {
    delete qwidget;
  }
#endif

  if (vcrossmanager)
    delete vcrossmanager;
  if (vprofmanager)
    delete vprofmanager;
  if (spectrummanager)
    delete spectrummanager;
  if (main_controller)
    delete main_controller;

  return 0;
}

/*
 SIGNAL HANDLING ROUTINES
 */

void doWork()
{
  if (!command_path.exists()) {
          COMMON_LOG::getInstance("common").errorStream() << "ERROR, trying to scan for commands, but command_path not set!";
    return;
  }

  string pattern = command_path;
  glob_t globBuf;
  int number_of_files = 0;

  glob(pattern.c_str(), 0, 0, &globBuf);
  number_of_files = globBuf.gl_pathc;

  if (number_of_files == 0) {
          COMMON_LOG::getInstance("common").warnStream() << "WARNING, scan for commands returned nothing";
    globfree_cache(&globBuf);
    return;
  }

  // loop over files
  for (int i = 0; i < number_of_files; i++) {
    string filename = globBuf.gl_pathv[i];
    dispatchWork(filename);
  }

  globfree_cache(&globBuf);
}

int dispatchWork(const std::string &file)
{
  cerr << "Reading input file: " << file << endl;

  // commands in file
  ifstream is(file.c_str());
  if (!is) {
          COMMON_LOG::getInstance("common").errorStream() << "ERROR, cannot open inputfile " << batchinput;
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
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, can't open the fifo <" << fifo_name << ">!";
      goto ERROR;
    }

    char buf[1];
    if (res == 0)
        buf[0] = 'r';
    else
        buf[0] = 'e';

    if (write(fd, buf, 1) == -1) {
        COMMON_LOG::getInstance("common").errorStream() << "ERROR, can't write to fifo <" << fifo_name << ">!";
    } else {
      if (verbose)
          COMMON_LOG::getInstance("common").infoStream() << "FIFO client <" << fifo_name << "> notified!";
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

