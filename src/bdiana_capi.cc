/*-*- mode: C++; c-basic-offset: 2; indent-tabs-mode: nil; -*-*/
/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2018 met.no

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

#include "bdiana_capi.h"
#include "bdiana_graphics.h"
#include "bdiana_main.h"
#include "bdiana_spectrum.h"
#include "bdiana_vcross.h"
#include "bdiana_vprof.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "diQuickMenues.h"
#include <diAnnotationPlot.h>
#include <diController.h>
#include <diFieldPlot.h>
#include <diObsManager.h>
#include <diObsPlot.h>
#include <diPlotModule.h>
#include <diSatManager.h>
#include <diSatPlot.h>

#include "diLocalSetupParser.h"
#include "diPlotCommandFactory.h"
#include "diUtilities.h"
#include "miSetupParser.h"
#include <diOrderBook.h>

#include "diField/diFieldManager.h"
#include "diField/diRectangle.h"

#include "util/charsets.h"
#include "util/diLineMerger.h"
#include "util/fimex_logging.h"
#include "util/misc_util.h"
#include "util/string_util.h"

#include <puTools/miStringFunctions.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/locale/generator.hpp>

#include <QApplication>
#include <QtCore>

#include <miLogger/miLoggingSystem.h>
#define MILOGGER_CATEGORY "diana.bdiana"
#include <miLogger/miLogging.h>


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

const std::string com_setupfile = "setupfile";
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

// types of plot
enum plot_type {
  plot_none = 0,
  plot_standard = 1, // standard diana map-plot
  plot_vcross = 2,   // vertical cross-section
  plot_vprof = 3,    // profiles
  plot_spectrum = 4, // wave spectrum
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

namespace {
QApplication * application = 0; // The Qt Application object

std::string format_time(const miutil::miTime& time)
{
  if (time.undef())
    return "--no time found--";
  else
    return time.isoTime();
}

struct Bdiana
{
  enum output_format_t { output_graphics, output_shape, output_json };

  Bdiana();
  ~Bdiana();

  FimexLoggingAdapter fla;
  BdianaGraphics go;
  BdianaMain main;
  DianaVcross vc; // cannot use "vcross" because of namespace name
  BdianaVprof vprof;
  BdianaSpectrum wavespec;

  std::string logfilename;
  std::string setupfile; //! diana setup file
  bool setupread;

  output_format_t output_format;
  std::string outputfilename; // except for graphics output

  plot_type plottype; // current plot_type
  bool failOnMissingData;
  bool useArchive;
  bool time_union;
  std::string time_format;
  miTime fixedtime, ptime;
  int addhour, addminute;

  map<std::string, map<std::string, std::string>> outputTextMaps; // output text for cases where output data is XML/JSON
  vector<std::string> outputTextMapOrder;                         // order of legends in output text

  //! replaceable values for plot-commands
  vector<keyvalue> keys;
  vector<std::string> lines, tmplines;
  vector<int> linenumbers;
  vector<stringlist> lists; // list of lists..

  //! Write one miTime per line to an output stream. At present, this producses utf-8.
  template <class C> void writeTimes(std::ostream& out, const C& times);

  void unpackloop(vector<std::string>& orig, // original strings..
                  vector<int>& origlines,    // ..with corresponding line-numbers
                  unsigned int& index,       // original string-counter to update
                  vector<std::string>& part, // final strings from loop-unpacking..
                  vector<int>& partlines);   // ..with corresponding line-numbers

  void unpackinput(vector<std::string>& orig,  // original setup
                   vector<int>& origlines,     // original list of linenumbers
                   vector<std::string>& final, // final setup
                   vector<int>& finallines);   // final list of linenumbers

  int prepareInput(istream& is);
  bool ensureSetup();
  void setTimeChoice(BdianaSource::TimeChoice tc);
  void set_ptime(BdianaSource& src);
  void createJsonAnnotation();
  std::vector<std::string> FIND_END_COMMAND(int& k, const std::string& end1, const std::string& end2, bool* found_end = 0);
  std::vector<std::string> FIND_END_COMMAND(int& k, const std::string& end, bool* found_end = 0);

  void handleVprofOpt(int& k);
  void handleVcrossOpt(int& k);
  void handleSpectrumOpt(int& k);
  int handlePlotCommand(int& k);
  int handleTimeCommand(int& k);
  int handleLevelCommand(int& k);
  int handleTimeVprofCommand(int& k);
  int handleTimeSpectrumCommand(int& k);
  int handleFieldFilesCommand(int& k);
  int handleDescribeCommand(int& k);
  int handleDescribeSpectrumCommand(int& k);
  int handleBuffersize(int& k, const std::string& value);
  int handleOutputCommand(int& k, const std::string& value);
  int handleMultiplePlotsCommand(int& k, const std::string& value);
  int handlePlotCellCommand(int& k, const std::string& value);

  int parseAndProcess(istream& is);
};

Bdiana::Bdiana()
    : setupfile("diana.setup")
    , setupread(false)
    , output_format(output_graphics)
    , outputfilename("tmp_diana.png")
    , plottype(plot_none)
    , failOnMissingData(false)
    , useArchive(false)
    , time_union(false)
    , time_format("$time")
    , addhour(0)
    , addminute(0)
{
}

Bdiana::~Bdiana()
{
}

template <class C> void Bdiana::writeTimes(std::ostream& out, const C& times)
{
  for (const miutil::miTime& t : times) {
    out << t.format(time_format, "", true) << std::endl;
  }
}

/*!
 * Expand % using miTime. At present, this inserts utf-8.
 */
static void expandTime(std::string& text, const miutil::miTime& time)
{
  if (miutil::contains(text, "%"))
    text = time.format(text, "", true);
}

} // namespace

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
void Bdiana::unpackloop(vector<std::string>& orig, // original strings..
                        vector<int>& origlines,    // ..with corresponding line-numbers
                        unsigned int& index,       // original string-counter to update
                        vector<std::string>& part, // final strings from loop-unpacking..
                        vector<int>& partlines)    // ..with corresponding line-numbers
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
void Bdiana::unpackinput(vector<std::string>& orig,  // original setup
                         vector<int>& origlines,     // original list of linenumbers
                         vector<std::string>& final, // final setup
                         vector<int>& finallines)    // final list of linenumbers
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

int Bdiana::prepareInput(istream& is)
{
  vector<std::string> tmplines;
  vector<int> tmplinenumbers;
  lines.clear();
  linenumbers.clear();

  /*
   read inputfile
   - skip blank lines
   - strip lines for comments and left/right whitespace
   - merge lines (ending with \)
   */
  diutil::GetLineConverter convertline("#");
  diutil::LineMerger lm;
  std::string line;
  while (convertline(is, line)) {
    diutil::remove_comment_and_trim(line);
    if (lm.push(line) && !lm.mergedline().empty()) {
      tmplines.push_back(lm.mergedline());
      tmplinenumbers.push_back(lm.lineno());
    }
  }
  if (!lm.complete() && !lm.mergedline().empty()) {
    tmplines.push_back(lm.mergedline());
    tmplinenumbers.push_back(lm.lineno());
  }

  // unpack loops and lists
  unpackinput(tmplines, tmplinenumbers, lines, linenumbers);

  // substitute environment variables and key-values
  for (std::string& line : lines) {
    SetupParser::checkEnvironment(line);
    for (const keyvalue& kv : keys)
      miutil::replace(line, "$" + kv.key, kv.value);
  }
  return 0;
}

/*
 parse setupfile
 perform other initialisations based on setup information
 */
static bool readSetup(const std::string& constSetupfile)
{
  std::string setupfile = constSetupfile;
  METLIBS_LOG_INFO("Reading setupfile: '" << setupfile << "'");

  if (!LocalSetupParser::parse(setupfile)) {
    METLIBS_LOG_ERROR("ERROR, an error occured while reading setup: '" << setupfile << "'");
    return false;
  }

  // language from setup
  const std::string language = LocalSetupParser::basicValue("language");
  if (!language.empty()) {
    miTime::setDefaultLanguage(language);
  }

  return true;
}

bool Bdiana::ensureSetup()
{
  if (!setupread)
    setupread = readSetup(setupfile);
  if (!setupread) {
    METLIBS_LOG_ERROR("ERROR, no setupinformation..exiting");
    return false;
  }
  return true;
}

/*
 Output Help-message:  call-syntax and optionally an example
 input-file for bdiana
 */
static void printUsage(bool showexample)
{
  const char* help[] = {
      "***************************************************",
      " DIANA batch version:" VERSION,
      " plot products in batch",
      "***************************************************",
      " Available products:",
      " - All standard map-products from DIANA",
      " - Vertical cross sections",
      " - Vertical profiles",
      " - WaveSpectrum plots",
      " Available output-formats:",
      " - as PDF, PS and EPS (using pdf2ps)",
      " - as PNG, SVG",
      " - as AVI MPG, MP4",
      "***************************************************",
      "",
      "Usage: bdiana -i <job-filename> [-s <setup-filename>] [-v] [-example] [key=value key=value]",
      "",
      "-i                   : job-control file. See '-example' below",
      "-s                   : setupfile for diana",
      "-v                   : (verbose) for more job-output",
      "-address=addr[:port] : production triggered by TCP connection",
      "                        * addr is a hostname or IP address",
      "                        * port is an optional port number, default is 3190", // diOrderListener::DEFAULT_PORT
      "-example             : list example input-file and exit",
      "",
      "special key/value pairs:",
      " - TIME=\"YYYY-MM-DD hh:mm:ss\"      plot-time",
  };
  const char** help_end = help + sizeof(help)/sizeof(help[0]);

  const char* example[] = {
    "#--------------------------------------------------------------",
    "# inputfile for bdiana",
    "# - '#' marks start of comment.",
    "# - you may split long lines by adding '\\' at the end.",
    "#--------------------------------------------------------------",
    "",
    "buffersize=1696x1200     # plotbuffer (WIDTHxHEIGHT)",
    "                         # For output=RASTER: size of plot.",
    "                         # For output=POSTSCRIPT: size of buffer",
    "                         #  affects output-quality. TIP: make",
    "                         #  sure width/height ratio = width/height",
    "                         #  ratio of MAP-area (defined under PLOT)",
    "setupfile=diana.setup    # use a standard setup-file",
    "output=PDF               # PDF/POSTSCRIPT/EPS/PNG/RASTER/AVI/MPG/MP4/SHP",
    "                         # RASTER: format from filename-suffix",
    "                         # PDF/SVG/JSON",
    "                         # JSON (only for annotations)",
    "filename=diana.pdf       # output filename",
    "",
    "# the next options only apply to map-based plots:",
    "keepPlotArea=NO          # YES=try to keep plotarea for several plots",
    "plotAnnotationsOnly=NO   # YES=only plot annotations/legends",
    "",
    "# Extra fields: including field sources not in the setup file",
    "<FIELD_FILES>",
    "m=Hirlam12 t=fimex f=hirlam12/hirlam12ml.nc format=netcdf",
    "</FIELD_FILES>",
    "",
    "#--------------------------------------------------------------",
    "# Product-examples:",
    "# Products are made by one or more PLOT-sections seen below,",
    "# in between two PLOT-sections you may change any of the options",
    "# described above.",
    "# The data-time will be set from the TIME=\"isotime-string\"",
    "# commandline parameter.",
    "# Output filename may contain data-time, format see man date",
    "# Example: filename=diana_%Y%M%dT%H.ps",
    "#--------------------------------------------------------------",
    "# STANDARD MAP-PRODUCT SECTION:",
    "PLOT                     # start of plot-command for map-product",
    "# paste in commands from quick-menues (one line for each element)",
    "",
    "FIELD HIRLAM.00 DZ(500-850) colour=yellow linetype=solid \\",
    " linewidth=1 line.interval=40 extreme.type=None extreme.size=1 \\",
    " extreme.radius=1 line.smooth=0 value.label=1 label.size=1  \\",
    " field.smooth=0 grid.lines=0 undef.masking=0 undef.colour=white \\",
    " undef.linewidth=1 undef.linetype=solid",
    "FIELD DNMI.ANA MSLP colour=blue linetype=dash linewidth=2 \\",
    " line.interval=1 extreme.type=None extreme.size=1 \\",
    " extreme.radius=1 line.smooth=0 value.label=1 label.size=1 \\",
    " field.smooth=0 grid.lines=0 undef.masking=0 undef.colour=white \\",
    " undef.linewidth=1 undef.linetype=solid",
    "OBS plot=Synop data=Synop parameter=Vind,TTT,TdTdTd,PPPP,ppp,a,h,\\",
    " VV,N,RRR,ww,W1,W2,Nh,Cl,Cm,Ch,vs,ds,TwTwTw,PwaHwa,Dw1Dw1,Pw1Hw1,\\",
    " TxTn,sss,911ff,s,fxfx,Kjtegn  tempprecision=true density=1 scale=1 \\",
    " timediff=180 colour=black font=BITMAPFONT face=normal",
    "OBJECTS NAME=\"DNMI Bakkeanalyse\" types=front,symbol,area \\",
    " timediff=60",
    "MAP area=Norge backcolour=white map=Gshhs-AUTO contour=on \\",
    " cont.colour=black cont.linewidth=1 cont.linetype=solid cont.zorder=1 \\",
    " land=on land.colour=landgul land.zorder=0 latlon=off frame=off",
    "LABEL data font=BITMAPFONT",
    "LABEL text=\"$day $date $auto UTC\" tcolour=red bcolour=black \\",
    " fcolour=white:200 polystyle=both halign=left valign=top \\",
    " font=BITMAPFONT fontsize=12",

    "",
    "ENDPLOT                  # End of plot-command",
    "#--------------------------------------------------------------",
    "# VERTICAL CROSSECTION SECTION:",
    "filename=vcross.pdf",
    "",
    "# detailed options for plot",
    "#VCROSS.OPTIONS",
    "#text=on textColour=black",
    "#frame=on frameColour=black frameLinetype=solid frameLinewidth=1",
    "#etc...  (see DIANA documentation or diana.log)",
    "#VCROSS.OPTIONS.END",
    "",
    "VCROSS.PLOT              # start of vertical crossection plot",
    "",

    "VCROSS model=HIRLAM.00 field=Temp(C) colour=black linetype=solid \\",
    " linewidth=1 line.interval=4 line.smooth=0 value.label=1 \\",
    " label.size=1",
    "VCROSS model=HIRLAM.00 field=Temp(C) colour=red linetype=solid \\",
    " linewidth=1 line.interval=4 line.smooth=0 value.label=1 \\",
    " label.size=1",
    "",
    "CROSSECTION=A.(70N,30W)-(50N,30W) # name of crossection",
    "",
    "ENDPLOT                  # End of plot-command",
    "#--------------------------------------------------------------",
    "# VERTICAL PROFILE SECTION:",
    "filename=vprof.pdf",
    "",
    "# detailed options for plot",
    "#VPROF.OPTIONS",
    "#tttt=on",
    "#tdtd=on",
    "#etc...  (see DIANA documentation or diana.log)",
    "#VPROF.OPTIONS.END",
    "",
    "VPROF.PLOT               # start of vertical profile plot",
    "",
    "MODELS=AROME-MetCoOp.00, HIRLAM.12KM.00  # comma-separated list of models",
    "STATION=KIRKENES         # station-name",
    "",
    "ENDPLOT                  # End of plot-command",
    "#--------------------------------------------------------------",
    "# SPECTRUM SECTION:",
    "filename=spectrum.pdf",
    "",
    "# detailed options for plot",
    "#SPECTRUM.OPTIONS",
    "#freqMax=0.3",
    "#backgroundColour=white",
    "#etc...  (see DIANA documentation or diana.log)",
    "#SPECTRUM.OPTIONS.END",
    "",
    "SPECTRUM.PLOT            # start of spectrum plot",
    "",
    "MODEL=WAM.50KM.00        # model",
    "STATION=\"60.1N 5.3W\"   # station-name",
    "",
    "ENDPLOT                  # End of plot-command",
    "#--------------------------------------------------------------",
    "# ADDITIONAL:",
    "#--------------------------------------------------------------",
    "#- You can add LOOPS with one or more variables:",
    "#  LOOP [X]|[Y] = X_value1 | Y_value1 , X_value2 | Y_value2",
    "#   <any other input lines, all \"[X]\" and \"[Y]\" will be",
    "#   replaced by the values after '=' for each iteration>",
    "#  LOOP.END",
    "#  The example shows a loop with two variables ([X] and [Y],",
    "#  separated by '|') and two iterations (separated with ',')",
    "#  Loops kan be nested",
    "#--------------------------------------------------------------",
    "#- Make a LIST for use in loops:",
    "#  LIST.stations           # A new list with name=stations",
    "#  OSLO                    # May contain any strings..",
    "#  KIRKENES                #",
    "#  LIST.END                # Marks End of list",
    "#",
    "#  To use in a loop:",
    "#  LOOP [VAR]=@stations    # The key here is the \'@\' with the",
    "#                          # list-name.",
    "#  LOOP.END                # This will loop over all list-entries",
    "#",
    "#  NOTES:",
    "#  - To make a list with multiple variables, convenient for",
    "#    multiple-variable loops, just add \'|\'s in the list-strings.",
    "#    Example:",
    "#    LIST.name             # new list",
    "#    OSLO | blue           # two variables for each entry",
    "#    KIRKENES | red        #",
    "#    LIST.END",
    "#",
    "#    LOOP [POS] | [COL] = @name # Loop using two variables in list",
    "#    LOOP.END",
    "#  - Lists must be defined OUTSIDE all loop statements",
    "#--------------------------------------------------------------",
    "#- alternative to TIME=.. commandline option:",
    "#  (default time is the last available time)",
    "#  use settime=YYYY-MM-DD hh:mm:ss",
    "#  use settime=currenttime / nowtime / firsttime",
    "#- use addhour=<value> or addminute=<value> to increment datatime",
    "#  (offset from TIME=\"\" variable). Useful in loops",
    "#--------------------------------------------------------------",
    "#- \"key=value\" pairs given on the commandline controls variables",
    "#  in the inputfile: Any \"$key\" found in the text will be",
    "#  substituted by \"value\".",
    "#--------------------------------------------------------------",
    "#- toggle archive mode (for observations)",
    "#  archive=ON/OFF  (default=OFF)",
#if 0
    "#--------------------------------------------------------------",
    "#- Making TRAJECTORIES",
    "#  to create and plot trajectories from any vector-field use:",
    "#  TRAJECTORY_OPTIONS=<options>    # see available options below",
    "#  TRAJECTORY=ON                   # Set this before PLOT command",
    "#  TRAJECTORY=OFF                  # Turn off trajectories when finished",
    "#  Valid options:",
    "#  latitudelongitude=<lat>,<lng>   # start position, repeat if necessary",
    "#  field=\"<full fieldname>\"        # Example: \"HIRLAM.00 VIND10.M\"",
    "#  colour=<colourname>             # Colour of trajectory lines",
    "#  line=<linewidth>                # width of trajectory lines",
    "#  numpos=1,5 or 9                 # number of sub-positions",
    "#  radius=<spreadth in km>         # if numpos>1, spreadth in km",
    "#  timemarker=<minutes>            # marker for each specified minute",
    "#",
    "#  Example:",
    "#    TRAJECTORY_OPTIONS=latitudelongitude=58.0,11.0 latitudelongitude=60.0,9.0 \\",
    "#    latitudelongitude=66.0,0.0 field=\"HIRLAM.00 VIND.10M\" colour=red \\",
    "#    line=2 numpos=5 radius=50 timemarker=0",
    "#  specifies 3 start positions, each with 5 sub-pos., using a HIRLAM",
    "#  wind-field as input (must also be specified in PLOT section)",
    "#  Prepare for trajectories",
    "#    TRAJECTORY=ON",
    "#  followed by a series of PLOT sections where the above field must be",
    "#  specified, running through the desired timesteps:",
    "#    LOOP [HOUR]=0,3,6,9",
    "#    ADDHOUR=[HOUR]",
    "#    FILENAME=traj_[HOUR].png",
    "#    PLOT",
    "#     FIELD HIRLAM.00 VIND.10M ........ etc.",
    "#    ENDPLOT",
    "#    LOOP.END",
    "#  Finally, turn off trajectories with:",
    "#    TRAJECTORY=OFF",
    "#--------------------------------------------------------------",
#endif
    "#",
    "#- MULTIPLE PLOTS PER PAGE",
    "#  You can put several plots in one postscript page by using the \'multiple.plots\'",
    "#  and \'plotcell\' commands. Start with describing the layout you want:",
    "#",
    "#  MULTIPLE.PLOTS=<rows>,<columns> # set the number of rows and columns",
    "#",
    "#  In the same command, you can specify the spacing between plots and",
    "#  the page-margin (given as percent of page width/height [0-100]):",
    "#  MULTIPLE.PLOTS=<rows>,<columns>,<spacing>,<margin>",
    "#",
    "#  Then, for each separate plot use the plotcell command to place plot on page:",
    "#  PLOTCELL=<row>,<column>         # the row and column, starting with 0",
    "#",
    "#  Finally, end the page with:",
    "#  MULTIPLE.PLOTS=OFF",
    "#",
    "#--------------------------------------------------------------",
    "#* Get Capabilities *",
    "## Developed for use in WMS",
    "## The syntax of the TIME-, LEVEL- and DESCRIBE-sections are equal to",
    "## the PLOT-sections. The plot options will be ignored.",
    "## The TIME-sections give available times,",
    "## both normal and constant times.",
    "## The LEVEL-sections give available levels.",
    "## The DESCRIBE-sections give information about the files read.",
    "## Valid options:",
    "## Normal times common to all products and  all constant times:",
    "#  time_options = intersection",
    "## All normal times from all products:",
    "#  time_options = union",
    "## Time format in outputfile",
    "#  time_format=%Y-%m-%dT%H:%M:%S",
    "#  TIME",
    "#  FIELD HIRLAM.00 DZ(500-850) colour=yellow linetype=solid \\",
    "#   linewidth=1 line.interval=40 extreme.type=None extreme.size=1 \\",
    "#   extreme.radius=1 line.smooth=0 value.label=1 label.size=1  \\",
    "#   field.smooth=0 grid.lines=0 undef.masking=0 undef.colour=white \\",
    "#   undef.linewidth=1 undef.linetype=solid",
    "#  FIELD DNMI.ANA MSLP",
    "#  OBS plot=Synop data=Synop parameter=Vind,TTT,TdTdTd,PPPP,ppp,a,h,\\",
    "#   VV,N,RRR,ww,W1,W2,Nh,Cl,Cm,Ch,vs,ds,TwTwTw,PwaHwa,Dw1Dw1,Pw1Hw1,\\",
    "#   TxTn,sss,911ff,s,fxfx,Kjtegn  tempprecision=true density=1 \\",
    "#   scale=1 timediff=180 colour=black font=BITMAPFONT face=normal",
    "#  OBJECTS NAME=\"DNMI Bakkeanalyse\" types=front,symbol,area",
    "#  ENDTIME",
    "#  LEVEL",
    "#  FIELD HIRLAM.20KM.00 Z",
    "#  ENDLEVEL",
    "#--------------------------------------------------------------",
    ""
  };
  const char** example_end = example + sizeof(example)/sizeof(example[0]);

  const char** txt_begin, **txt_end;
  if (!showexample) {
    txt_begin = help;
    txt_end = help_end;
  } else {
    txt_begin = example;
    txt_end = example_end;
  }
  std::copy(txt_begin, txt_end,
      std::ostream_iterator<std::string>(std::cout, "\n"));

  exit(1);
}

void Bdiana::setTimeChoice(BdianaSource::TimeChoice tc)
{
  main.setTimeChoice(tc);
  vc.setTimeChoice(tc);
  vprof.setTimeChoice(tc);
  wavespec.setTimeChoice(tc);
}

void Bdiana::set_ptime(BdianaSource& src)
{
  if (fixedtime.undef()) {
    fixedtime = src.getTime();
    if (verbose)
      METLIBS_LOG_INFO("using default time:" << format_time(fixedtime));
  }
  ptime = fixedtime;
  if (!ptime.undef()) {
    ptime.addHour(addhour);
    ptime.addMin(addminute);
  }
  if (verbose)
    METLIBS_LOG_INFO("- using time: " << format_time(ptime));
  src.setTime(ptime);
}

void Bdiana::createJsonAnnotation()
{
  for (AnnotationPlot* ap : main.controller->getAnnotations()) {
    for (const AnnotationPlot::Annotation& ann : ap->getAnnotations()) {
      for (const string& ans : ann.vstr) {

        // Find the table description in the string.
        std::string legend = ans;
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
  //  main.controller->getPlotTime(thetime);
  //  if ( !ptime.undef() ) {
  //    metaDataMap["request time"] = std::string("\"") + ptime.isoTime() + std::string("\"");
  //  }
  //  metaDataMap["time used"] = std::string("\"") + thetime + std::string("\"");
  //  outputTextMaps["metadata"] = metaDataMap;
  //  outputTextMapOrder.push_back("metadata");
}

std::vector<std::string> Bdiana::FIND_END_COMMAND(int& k, const std::string& end1, const std::string& end2, bool* found_end)
{
  std::vector<std::string> pcom;
  const int linenum = lines.size();
  while (++k < linenum) {
    const std::string& line = lines[k];
    if (line.empty())
      continue;
    const size_t nospace = line.find_first_not_of(" \t");
    if (nospace == std::string::npos || line[nospace] == '#')
      continue;

    const std::string line_lc = miutil::to_lower(line);
    if (line_lc == end1 || line_lc == end2)
      break;
    pcom.push_back(line);
  }
  if (found_end)
    *found_end = (k < linenum);
  return pcom;
}

std::vector<std::string> Bdiana::FIND_END_COMMAND(int& k, const std::string& end, bool* found_end)
{
  return FIND_END_COMMAND(k, end, std::string(), found_end);
}

void Bdiana::handleVprofOpt(int& k)
{
  const std::vector<std::string> pcom = FIND_END_COMMAND(k, com_vprof_opt_end);
  vprof.set_options(pcom);
}

void Bdiana::handleVcrossOpt(int& k)
{
  const std::vector<std::string> pcom = FIND_END_COMMAND(k, com_vcross_opt_end);
  main.MAKE_CONTROLLER(); // required for loading kml files
  vc.MAKE_VCROSS();
  vc.commands(pcom);
}

void Bdiana::handleSpectrumOpt(int& k)
{
  const std::vector<std::string> pcom = FIND_END_COMMAND(k, com_spectrum_opt_end);
  wavespec.set_options(pcom);
}

int Bdiana::handlePlotCommand(int& k)
{
  // --- START PLOT ---
  const std::string command = miutil::to_lower(lines[k]);
  if (command == com_plot) {
    plottype = plot_standard;
    if (verbose)
      METLIBS_LOG_INFO("Preparing new standard-plot");

  } else if (command == com_vcross_plot) {
    plottype = plot_vcross;
    if (verbose)
      METLIBS_LOG_INFO("Preparing new vcross-plot");

  } else if (command == com_vprof_plot) {
    plottype = plot_vprof;
    if (verbose)
      METLIBS_LOG_INFO("Preparing new vprof-plot");

  } else if (command == com_spectrum_plot) {
    plottype = plot_spectrum;
    if (verbose)
      METLIBS_LOG_INFO("Preparing new spectrum-plot");
  }

  if (output_format == output_shape && plottype != plot_standard) {
    METLIBS_LOG_ERROR("ERROR, you can only use plottype STANDARD when using shape option"
        << "..Exiting..");
    return 1;
  }

  if (!ensureSetup())
    return 99;

  std::vector<std::string> pcom = FIND_END_COMMAND(k, com_endplot, com_plotend);
  if (output_format == output_shape) {
    for (std::string& line : pcom) {
      if (diutil::startswith(line, "FIELD ")) {
        std::string shapeFileName = outputfilename;
        line += " shapefilename=" + shapeFileName;
        if (miutil::contains(line, "shapefile=0"))
          miutil::replace(line, "shapefile=0", "shapefile=1");
        else if (not miutil::contains(line, "shapefile="))
          line += " shapefile=1";
      } else {
        METLIBS_LOG_ERROR("Error, Shape option cannot be used for OBS/OBJECTS/SAT/TRAJECTORY/EDITFIELD.. exiting");
        return 1;
      }
    }
  }

  if (plottype == plot_standard) {
    // -- normal plot

    if (not main.MAKE_CONTROLLER())
      return 99;

    vector<std::string> field_errors;
    if (!main.controller->getFieldManager()->updateFileSetup(main.extra_field_lines, field_errors)) {
      METLIBS_LOG_ERROR("ERROR, an error occurred while adding new fields:");
      for (const std::string& fe : field_errors)
        METLIBS_LOG_ERROR(fe);
    }
    main.extra_field_lines.clear();

    // turn on/off archive-mode (observations)
    main.controller->archiveMode(useArchive);

    // keeparea= false: use selected area or field/sat-area
    // keeparea= true : keep previous used area (if possible)
    main.controller->keepCurrentArea(main.keeparea);

    // necessary to set time before plotCommands()..?
    miTime thetime = miTime::nowTime();
    main.controller->setPlotTime(thetime);
    main.commands(pcom);

    set_ptime(main);

    if (verbose)
      METLIBS_LOG_INFO("- updatePlots");
    if (!main.controller->updatePlots() && failOnMissingData) {
      METLIBS_LOG_WARN("Failed to update plots.");
      go.endOutput();
      return 99;
    }
    METLIBS_LOG_INFO("map area = " << main.controller->getMapArea());

    expandTime(outputfilename, ptime);
    if (output_format == output_graphics) {
      go.setOutputFile(outputfilename);
      go.render(main);
    }

    if (main.plot_trajectory && !main.trajectory_started) {
      vector<string> vstr;
      vstr.push_back("clear");
      vstr.push_back("delete");
      vstr.push_back(main.trajectory_options);
      main.controller->trajPos(vstr);
      // main.controller->trajTimeMarker(trajectory_timeMarker);
      main.controller->startTrajectoryComputation();
      main.trajectory_started = true;
    } else if (!main.plot_trajectory && main.trajectory_started) {
      vector<string> vstr;
      vstr.push_back("clear");
      vstr.push_back("delete");
      main.controller->trajPos(vstr);
      main.trajectory_started = false;
    }

    if (verbose)
      METLIBS_LOG_INFO("- plot");

    if (output_format == output_shape)
      main.controller->plot(nullptr /*go.glpainter*/, true, false); // FIXME

    // Create JSON annotations irrespective of the value of plotAnnotationsOnly.
    if (output_format == output_json)
      createJsonAnnotation();

    // --------------------------------------------------------
  } else if (plottype == plot_vcross) {

    // -- vcross plot
    if (!vc.manager)
      vc.manager = std::make_shared<vcross::QtManager>();
    else
      vc.manager->cleanup(); // needed to clear zoom history

    if (verbose)
      METLIBS_LOG_INFO("- sending vcross plot commands");
    vc.commands(pcom);

    set_ptime(vc);

    expandTime(outputfilename, ptime);
    go.setOutputFile(outputfilename);
    if (verbose)
      METLIBS_LOG_INFO("- plot");
    go.render(vc);

    // --------------------------------------------------------
  } else if (plottype == plot_vprof) {

    vprof.MAKE_VPROF();

    if (verbose)
      METLIBS_LOG_INFO("- sending plotCommands");
    vprof.commands(pcom);

    set_ptime(vprof);

    expandTime(outputfilename, ptime);
    go.setOutputFile(outputfilename);

    if (verbose)
      METLIBS_LOG_INFO("- plot");
    if (!go.render(vprof) && failOnMissingData) {
      METLIBS_LOG_WARN("Failed to plot vprofdata.");
      return 99;
    }

    // --------------------------------------------------------
  } else if (plottype == plot_spectrum) {

    wavespec.MAKE_SPECTRUM();
    wavespec.set_spectrum(pcom);

    set_ptime(wavespec);

    wavespec.set_station();

    expandTime(outputfilename, ptime);
    go.setOutputFile(outputfilename);
    if (verbose)
      METLIBS_LOG_INFO("- plot");
    go.render(wavespec);
  }
  // --------------------------------------------------------
  // Write output to a file.
  // --------------------------------------------------------

  if (output_format == output_shape) { // Only shape output

    if (miutil::contains(outputfilename, "tmp_diana")) {
      METLIBS_LOG_INFO("Using shape option without file name, it will be created automatically");
    } else {
      METLIBS_LOG_INFO("Using shape option with given file name: '" << outputfilename << "'");
    }

  } else if (output_format == output_json) {

    QFile outputFile(QString::fromStdString(outputfilename));
    if (outputFile.open(QFile::WriteOnly)) {
      outputFile.write("{");
      bool firstItem = true;
      for (const std::string& iti : outputTextMapOrder) {
        if (!firstItem)
          outputFile.write(",");
        firstItem = false;
        outputFile.write("\n  \"");
        outputFile.write(QString::fromStdString(iti).toUtf8());
        outputFile.write("\": {");
        bool firstMap = true;
        for (auto itj : outputTextMaps[iti]) {
          if (!firstMap)
            outputFile.write(",");
          firstMap = false;
          outputFile.write("\n    \"");
          outputFile.write(QString::fromStdString(itj.first).toUtf8());
          outputFile.write("\": ");
          outputFile.write(QString::fromStdString(itj.second).toUtf8());
        }
        outputFile.write("\n  }");
      }
      outputFile.write("\n}\n");
      outputFile.close();
    }
  }
  return 0;
}

int Bdiana::handleTimeCommand(int& k)
{
  if (!ensureSetup())
    return 99;

  if (not main.MAKE_CONTROLLER())
    return 99;

  if (verbose)
    METLIBS_LOG_INFO("- finding times");

  std::vector<std::string> pcom = FIND_END_COMMAND(k, com_endtime);
  set<miTime> okTimes;
  main.controller->getCapabilitiesTime(okTimes, makeCommands(pcom), time_union);

  ofstream file(outputfilename.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << outputfilename << "'");
    return 1;
  }
  file << "PROG" << endl;
  writeTimes(file, okTimes);
  return 0;
}

int Bdiana::handleLevelCommand(int& k)
{
  if (!ensureSetup())
    return 99;

  if (not main.MAKE_CONTROLLER())
    return 99;

  if (verbose)
    METLIBS_LOG_INFO("- finding levels");

  const vector<std::string> pcom = FIND_END_COMMAND(k, com_endlevel);

  ofstream file(outputfilename.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << outputfilename << "'");
    return 1;
  }

  for (const std::string& pcs : pcom) {
    for (const std::string& level : main.controller->getFieldLevels(makeCommand(pcs)))
      file << level << endl;
    file << endl;
  }

  return 0;
}

int Bdiana::handleTimeVprofCommand(int& k)
{
  if (verbose)
    METLIBS_LOG_INFO("- finding times");

  vprof.MAKE_VPROF();
  vprof.commands(FIND_END_COMMAND(k, com_endtime));

  ofstream file(outputfilename.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << outputfilename << "'");
    return 1;
  }
  file << "PROG" << endl;
  writeTimes(file, vprof.manager->getTimeList());
  return 0;
}

int Bdiana::handleTimeSpectrumCommand(int& k)
{
  // read setup
  if (!ensureSetup())
    return 99;

  if (verbose)
    METLIBS_LOG_INFO("- finding times");

  wavespec.MAKE_SPECTRUM();
  wavespec.set_spectrum(FIND_END_COMMAND(k, com_endtime));

  set_ptime(wavespec);

  wavespec.set_station();

  ofstream file(outputfilename.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << outputfilename << "'");
    return 1;
  }
  file << "PROG" << endl;
  writeTimes(file, wavespec.manager->getTimeList());

  file << "CONST" << endl;
  /* writeTimes(file, constTimes); */
  return 0;
}

int Bdiana::handleFieldFilesCommand(int& k)
{
  bool found_end = false;
  const std::vector<std::string> pcom = FIND_END_COMMAND(k, com_field_files_end, &found_end);
  if (!found_end) {
    METLIBS_LOG_ERROR("no " << com_field_files_end << " found:" << lines[k] << " Linenumber:" << linenumbers[k]);
    return 1;
  }
  diutil::insert_all(main.extra_field_lines, pcom);
  return 0;
}

int Bdiana::handleDescribeCommand(int& k)
{
  if (verbose)
    METLIBS_LOG_INFO("- finding information about data sources");

  //Find ENDDESCRIBE
  std::vector<std::string> pcom = FIND_END_COMMAND(k, com_describe_end);

  if (not main.MAKE_CONTROLLER())
    return 99;

  main.commands(pcom);

  set_ptime(main);

  if (verbose)
    METLIBS_LOG_INFO("- updatePlots");

  if (main.controller->updatePlots() || !failOnMissingData) {

    if (verbose)
      METLIBS_LOG_INFO("- opening file '" << outputfilename << "'");

    // open filestream
    ofstream file(outputfilename.c_str());
    if (!file) {
      METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << outputfilename << "'");
      return 1;
    }

    FieldManager* fieldManager = main.controller->getFieldManager();
    std::set<std::string> fieldPatterns;
    for (FieldPlot* fp : main.controller->getFieldPlots()) {
      diutil::insert_all(fieldPatterns, fieldManager->getFileNames(fp->getModelName()));
    }

    const SatManager::Prod_t& satProducts = main.controller->getSatelliteManager()->getProductsInfo();
    set<std::string> satPatterns;
    set<std::string> satFiles;

    for (SatPlot* sp : main.controller->getSatellitePlots()) {
      Sat* sat = sp->satdata;
      const SatManager::Prod_t::const_iterator itp = satProducts.find(sat->satellite);
      if (itp != satProducts.end()) {
        const SatManager::SubProd_t::const_iterator itsp = itp->second.find(sat->satellite);
        if (itsp != itp->second.end()) {
          const SatManager::subProdInfo& satInfo = itsp->second;
          for (const SatFileInfo& sfi : satInfo.file) {
            satFiles.insert(sfi.name);
            if (sfi.name == sat->actualfile)
              satPatterns.insert(satInfo.pattern.begin(), satInfo.pattern.end());
          }
        }
      }
    }

    set<std::string> obsPatterns;
    set<std::string> obsFiles;

#if 0
    const std::map<std::string, ObsManager::ProdInfo>& obsProducts = main.controller->getObservationManager()->getProductsInfo();
    for (ObsPlot* op : main.controller->getObsPlots()) {
      for (const string& opf : op->getFileNames()) {
        obsFiles.insert(opf);

        for (auto obsprod : obsProducts) {
          const ObsManager::ProdInfo& pi = obsprod.second;
          for (const ObsManager::FileInfo& fi : pi.fileInfo) {
            if (opf == fi.filename) {
              for (const ObsManager::patternInfo& pin : pi.pattern)
                obsPatterns.insert(pin.pattern);
            }
          }
        }
      }
    }
#endif

    file << "FILES" << endl;
    for (const std::string& p : fieldPatterns)
      file << p << endl;
    for (const std::string& p : satFiles)
      file << p << endl;
    for (const std::string& p : satPatterns)
      file << p << endl;
    for (const std::string& p : obsFiles)
      file << p << endl;
    for (const std::string& p : obsPatterns)
      file << p << endl;

    file.close();
  }
  return 0;
}

int Bdiana::handleDescribeSpectrumCommand(int& k)
{
  if (verbose)
    METLIBS_LOG_INFO("- finding information about data sources");

  wavespec.MAKE_SPECTRUM();
  wavespec.set_spectrum(FIND_END_COMMAND(k, com_describe_end));

  set_ptime(wavespec);
  wavespec.set_station();

  if (verbose)
    METLIBS_LOG_INFO("- opening file '" << outputfilename << "'");

  // open filestream
  ofstream file(outputfilename.c_str());
  if (!file) {
    METLIBS_LOG_ERROR("ERROR OPEN (WRITE) '" << outputfilename << "'");
    return 1;
  }

  file << "FILES" << endl;
  for (const std::string& f : wavespec.manager->getModelFiles())
    file << f << endl;

  return 0;
}

int Bdiana::handleBuffersize(int& k, const std::string& value)
{
  const std::vector<std::string> vvs = miutil::split(value, "x");
  if (vvs.size() < 2) {
    METLIBS_LOG_ERROR("ERROR, buffersize should be WxH:" << lines[k]
        << " Linenumber:" << linenumbers[k]);
    return 1;
  }
  const int w = miutil::to_int(vvs[0]);
  const int h = miutil::to_int(vvs[1]);
  return go.setBufferSize(w, h) ? 0 : 1;
}

int Bdiana::handleOutputCommand(int& k, const std::string& value)
{
  const std::string lvalue = miutil::to_lower(value);
  output_format = output_graphics;
  if (lvalue == "auto") {
    go.setImageType(image_auto);
  } else if (lvalue == "postscript") {
    go.setImageType(image_ps);
  } else if (lvalue == "eps") {
    go.setImageType(image_eps);
  } else if (lvalue == "png" || lvalue == "raster") {
    go.setImageType(image_raster);
  } else if (lvalue == "shp") {
    output_format = output_shape;
  } else if (lvalue == "avi") {
    go.setMovieFormat("avi");
  } else if (lvalue == "mpg") {
    go.setMovieFormat("mpg");
  } else if (lvalue == "mp4") {
    go.setMovieFormat("mp4");
  } else if (lvalue == "pdf") {
    go.setImageType(image_pdf);
  } else if (lvalue == "svg") {
    go.setImageType(image_svg);
  } else if (lvalue == "json") {
    output_format = output_json;
    outputTextMaps.clear();
    outputTextMapOrder.clear();
  } else {
    METLIBS_LOG_ERROR("ERROR, unknown output-format:" << lines[k] << " Linenumber:" << linenumbers[k]);
    return 1;
  }

  return 0;
}

int Bdiana::handleMultiplePlotsCommand(int& k, const std::string& value)
{
  if (miutil::to_lower(value) == "off") {
    go.disableMultiPlot();

  } else {
    const vector<std::string> v1 = miutil::split(value, ",");
    if (v1.size() < 2) {
      METLIBS_LOG_WARN("WARNING, illegal values to multiple.plots:" << lines[k] << " Linenumber:" << linenumbers[k]);
      return 1;
    }
    const int rows = miutil::to_int(v1[0]);
    const int cols = miutil::to_int(v1[1]);
    float fmargin = 0.0;
    float fspacing = 0.0;
    if (v1.size() > 2)
      fspacing = miutil::to_float(v1[2]);
    if (v1.size() > 3)
      fmargin = miutil::to_float(v1[3]);
    go.enableMultiPlot(rows, cols, fspacing, fmargin);
  }

  return 0;
}

int Bdiana::handlePlotCellCommand(int& k, const std::string& value)
{
  vector<std::string> v1 = miutil::split(value, ",");
  if (v1.size() != 2) {
    METLIBS_LOG_WARN("WARNING, illegal values to plotcell:" << lines[k] << " Linenumber:" << linenumbers[k]);
    return 1;
  }
  const int row = miutil::to_int(v1[0]);
  const int col = miutil::to_int(v1[1]);
  if (!go.setPlotCell(row, col)) {
    METLIBS_LOG_WARN("WARNING, illegal values to plotcell:" << lines[k] << " Linenumber:" << linenumbers[k]);
    return 1;
  }
  return 0;
}

int Bdiana::parseAndProcess(istream& is)
{
  // unpack loops, make lists, merge lines etc.
  int res = prepareInput(is);
  if (res != 0)
    return res;

  const int linenum = lines.size();

  // parse input - and perform plots
  for (int k = 0; k < linenum; k++) {// input-line loop
    // start parsing...
    const std::string command = miutil::to_lower(lines[k]);
    if (command == com_vprof_opt) {
      handleVprofOpt(k);
      continue;

    } else if (command == com_vcross_opt) {
      handleVcrossOpt(k);
      continue;

    } else if (command == com_spectrum_opt) {
      handleSpectrumOpt(k);
      continue;

    } else if (command == com_plot || command == com_vcross_plot || command == com_vprof_plot || command == com_spectrum_plot) {
      if (handlePlotCommand(k))
        return 99;
      continue;

    } else if (command == com_time) {
      if (handleTimeCommand(k))
        return 99;
      continue;

    } else if (command == com_time_vprof) {
      if (handleTimeVprofCommand(k))
        return 99;
      continue;

    } else if (command == com_time_spectrum) {
      if (handleTimeSpectrumCommand(k) != 0)
        return 99;
      continue;

    } else if (command == com_level) {
      if (handleLevelCommand(k))
        return 99;
      continue;

    } else if (command == com_field_files) {

      if (handleFieldFilesCommand(k) != 0)
        return 99;
      continue;

    } else if (command == com_describe) {
      if (handleDescribeCommand(k) != 0)
        return 99;
      continue;

    } else if (command == com_describe_spectrum) {
      if (handleDescribeSpectrumCommand(k) != 0)
        return 99;
      continue;

    } else if (command == com_print_document) {
      METLIBS_LOG_ERROR("the bdiana command '" << command << "' is not valid any more");
      return 99;

    } else if (command == com_field_files_end || command == com_describe_end) {
      METLIBS_LOG_ERROR("WARNING, " << command << " found:" << lines[k] << " Linenumber:" << linenumbers[k]);
      continue;
    }

    // all other options on the form KEY=VALUE

    const vector<std::string> vs = miutil::split(lines[k], "=");
    const int nv = vs.size();
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
        METLIBS_LOG_WARN("setupfile overrided by command line option. Linenumber:" << linenumbers[k]);
      } else {
        setupfile = value;
        if (!ensureSetup())
          return 99;
      }

    } else if (key == com_buffersize) {
      if (handleBuffersize(k, value))
        return 99;

    } else if (key == com_papersize || key == com_toprinter || key == com_printer || key == com_colour || key == com_drawbackground || key == com_orientation ||
               key == com_antialiasing) {
      METLIBS_LOG_WARN("the bdiana option '" << key << "' has no effect any more");

    } else if (key == com_filename) {
      if (value.empty()) {
        METLIBS_LOG_ERROR("ERROR, illegal filename in:" << lines[k] << " Linenumber:"
               << linenumbers[k]);
        return 1;
      } else
        outputfilename = value;

    } else if (key == com_output) {
      if (handleOutputCommand(k, value))
        return 99;

    } else if (key == com_addhour) {
      addhour=atoi(value.c_str());

    } else if (key == com_addminute) {
      addminute = atoi(value.c_str());

    } else if (key == com_settime) {
      ptime = miTime(); // undef
      if (value == "nowtime" || value == "current" || value == "currenttime") {
        setTimeChoice(BdianaSource::USE_NOWTIME);
      } else if (value == "firsttime") {
        setTimeChoice(BdianaSource::USE_FIRSTTIME);
      } else if (value == "lasttime") {
        setTimeChoice(BdianaSource::USE_LASTTIME);
      } else if (value == "referencetime") {
        setTimeChoice(BdianaSource::USE_REFERENCETIME);
      } else if (miTime::isValid(value)) {
        setTimeChoice(BdianaSource::USE_FIXEDTIME);
        fixedtime = miTime(value);
      } else {
        METLIBS_LOG_ERROR("command " << com_settime << " has invalid argument '" << value << "'");
        setTimeChoice(BdianaSource::USE_LASTTIME);
        fixedtime = miTime();
      }

    } else if (key == com_archive) {
      useArchive = (miutil::to_lower(value) == "on");

    } else if (key == com_keepplotarea) {
      main.keeparea = (miutil::to_lower(value) == "yes");

    } else if (key == com_plotannotationsonly) {
      main.setAnnotationsOnly(miutil::to_lower(value) == "yes");

    } else if (key == com_fail_on_missing_data) {
      failOnMissingData = (miutil::to_lower(value) == "yes");

    } else if (key == com_multiple_plots) {
      if (handleMultiplePlotsCommand(k, value))
        return 99;

    } else if (key == com_plotcell) {
      if (handlePlotCellCommand(k, value))
        return 99;

    } else if (key == com_trajectory) {
      if (miutil::to_lower(value) == "on") {
        main.plot_trajectory = true;
        main.trajectory_started = false;
      } else {
        main.plot_trajectory = false;
      }

    } else if (key == com_trajectory_opt) {
      main.trajectory_options = value;

    } else if (key == com_trajectory_print) {
      main.controller->printTrajectoryPositions(value);

    } else if (key == com_time_opt) {
      time_union = (miutil::to_lower(value) == "union");

    } else if (key == com_time_format) {
      time_format = value;

    } else {
      METLIBS_LOG_WARN("WARNING, unknown command:" << lines[k] << " Linenumber:"
            << linenumbers[k]);
    }
  }

  return 0;
}

static std::unique_ptr<Bdiana> bdiana_instance;

static Bdiana* bdiana()
{
  if (!bdiana_instance)
    bdiana_instance.reset(new Bdiana);
  return bdiana_instance.get();
}

// ========================================================================

/*
 * public C-version of above readSetup
 */
int diana_readSetupFile(const char* setupFilename)
{
  std::string setupfile(setupFilename);
  if (!(bdiana()->setupread = readSetup(setupfile))) {
    METLIBS_LOG_ERROR("ERROR, unable to read setup: '" << setupfile << "'");
    return DIANA_ERROR;
  }
  return DIANA_OK;
}

/*
 * public C api of above parseAndProcessString
 */
int diana_parseAndProcessString(const char* string)
{
  // reset time (before next wms request)
  bdiana()->ptime = bdiana()->fixedtime = miTime();

  stringstream ss;
  ss << string;
  METLIBS_LOG_INFO("start processing");
  if (bdiana()->parseAndProcess(ss) == 0)
    return DIANA_OK;
  else
    return DIANA_ERROR;
}


/*
 =================================================================
 BDIANA - BATCH PRODUCTION OF DIANA GRAPHICAL PRODUCTS, public C
 =================================================================
 */
int diana_init(int _argc, char** _argv)
{
  if (!application) {
#if defined(Q_WS_QWS)
    application = new QApplication(_argc, _argv, QApplication::GuiServer);
#else
    application = new QApplication(_argc, _argv);
#endif
  }

  setlocale(LC_NUMERIC, "C");
  setlocale(LC_MEASUREMENT, "C");
  setlocale(LC_TIME, "C");

  boost::locale::generator locale_gen;
  std::locale::global(locale_gen(""));

  bool setupfilegiven = false;
  std::string logfilename;
  std::string batchinput;
  diOrderBook* orderbook = nullptr;
  int port;

  // get the BDIANA_LOGGER variable
  if (char* ctmp = getenv("BDIANA_LOGGER")) {
    logfilename = ctmp;
  }

  QStringList argv = application->arguments();
  int argc = argv.size();

  // check command line arguments
  if (argc < 2) {
    printUsage(false);
  }

  SetupParser::replaceUserVariables("PVERSION", PVERSION);
  SetupParser::replaceUserVariables("SYSCONFDIR", SYSCONFDIR);

  for (int ac = 1; ac < argc;) {
    const std::string sarg = argv[ac].toStdString();
    ac += 1;

    if (sarg == "-display") {
      if (ac >= argc)
        printUsage(false);

      cerr << "WARN, -display <..> is ignored and deprecated, please remove from command invocation" << endl;
      ac += 1;

    } else if (sarg == "-input" || sarg == "-i") {
      if (ac >= argc)
        printUsage(false);

      batchinput = argv[ac].toStdString();
      ac += 1;

    } else if (sarg == "-setup" || sarg == "-s") {
      if (ac >= argc)
        printUsage(false);

      setupfilegiven = true;
      bdiana()->setupfile = argv[ac].toStdString();
      ac += 1;

    } else if (sarg == "-logger" || sarg == "-L") {
      if (ac >= argc)
        printUsage(false);

      logfilename = argv[ac].toStdString();
      ac += 1;

    } else if (sarg == "-v") {
      verbose = true;

    } else if (sarg == "-example") {
      printUsage(true);

    } else if (sarg == "-use_nowtime") {
      // Use time closest to the current time even if there exists a field
      // and not the timestamps for the future. This corresponds to the
      // default value when using the gui.
      bdiana()->setTimeChoice(BdianaSource::USE_NOWTIME);

    } else if (sarg.find("-address=") == 0) {
      if (!orderbook) {
        orderbook = new diOrderBook();
        orderbook->start();
      }
      const vector<std::string> ks = miutil::split(sarg, "=");
      if (ks.size() != 2) {
        cerr << "ERROR, invalid -address argument '" << sarg << "'" << endl;
        return 1;
      }
      const vector<std::string> ads = miutil::split(ks[1], ":");
      if (ads.size() == 2) {
        const std::string& port_txt = ads[1];
        if (miutil::is_number(port_txt)) {
          port = miutil::to_int(port_txt);
        } else {
          cerr << "ERROR, " << port_txt << " is not a valid TCP port number" << endl;
          return 1;
        }
        if (port < 1 || port > 65535) {
          cerr << "ERROR, " << port << "  is not a valid TCP port number" << endl;
          return 1;
        }
      } else {
        port = diOrderListener::DEFAULT_PORT;
      }
      const std::string& host_txt = ads[0];
      if (!orderbook->addListener(QString::fromStdString(host_txt), port)) {
        cerr << "ERROR, unable to listen on " << host_txt << ":" << port << endl;
        return 1;
      }
    } else {
      const vector<std::string> ks = miutil::split(sarg, "=");
      if (ks.size() != 2) {
        cerr << "WARNING, unknown argument on commandline:" << sarg << endl;
        continue;
      }

      keyvalue tmp;
      tmp.key = ks[0];
      tmp.value = ks[1];
      bdiana()->keys.push_back(tmp);

      // temporary: force plottime
      if (tmp.key == "TIME") {
        if (miTime::isValid(tmp.value)) {
          bdiana()->fixedtime = miTime(tmp.value);
        } else {
          cerr << "ERROR, invalid TIME-variable on commandline: '" << tmp.value << "'" << endl;
          return 1;
        }
      }
    }
  } // command line parameters

  milogger::system::selectedSystem()->configure(logfilename);

  METLIBS_LOG_INFO(argv[0].toStdString() << " : DIANA batch version " << VERSION);

  /*
   if setupfile specified on the command-line, parse it now
   */
  if (setupfilegiven) {
    if (!(bdiana()->setupread = readSetup(bdiana()->setupfile))) {
      METLIBS_LOG_ERROR("ERROR, unable to read setup:" << bdiana()->setupfile);
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
    int res = bdiana()->parseAndProcess(is);
    if (res != 0)
      return 99;
  }

  if (orderbook != NULL) {
    bool quit = false;
    /*
     * XXX should handle SIGTERM / SIGINT somehow; currently, quit can
     * never be false in this branch, and a SIGTERM or SIGINT will
     * terminate bdiana immediately, with no chance for cleanup.
     */
    while (!quit) {
      QPointer<diWorkOrder> order = orderbook->getNextOrder();
      if (order) {
        std::istringstream is(order->getText());
        METLIBS_LOG_INFO("processing order...");
        bdiana()->parseAndProcess(is);
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
  } else if (batchinput.empty()) {
    METLIBS_LOG_WARN("No -address was specified");
  }
  return DIANA_OK;
}

int diana_dealloc()
{
  bdiana_instance.reset(0);
  milogger::system::selectSystem(milogger::system::SystemPtr());
  return DIANA_OK;
}
