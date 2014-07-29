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
#ifndef _diPrintOptions_h
#define _diPrintOptions_h

#include <map>
#include <string>
#include <vector>

namespace d_print {

  // postscript options

  /// postscript paper orientation
  enum Orientation { // paper orientation
    ori_automatic,
    ori_portrait,
    ori_landscape
  };

  /// postscript colour options
  enum ColourOption { // use of colour
    incolour,
    greyscale,
    blackwhite
  };

  /// postscript page sizes
  enum PageSize { // standard pagesizes
    A4, B5, Letter, Legal, Executive,
    A0, A1, A2, A3, A5, A6, A7, A8, A9,
    B0, B1, B10, B2, B3, B4, B6, B7, B8, B9,
    C5E, Comm10E, DLE, Folio, Ledger, Tabloid, NPageSize 
  };

  /// postscript paper size in mm
  struct PaperSize { // size of paper in mm (default A4)
    float hsize;
    float vsize;
    PaperSize(): hsize(210),vsize(297) {}
    PaperSize(float h, float v)
      : hsize(h), vsize(v) {}
  };

}; // end of namespace

/**
   \brief options for one print job

*/

class printOptions {
public:
  std::string fname;                   ///< name of output file
  std::string printer;                 ///< name of printer
  d_print::Orientation orientation; ///< paper-orientation
  d_print::ColourOption colop;      ///< use of colour
  d_print::PageSize pagesize;       ///< pagesize in standard notation
  d_print::PaperSize papersize;     ///< size of paper in mm
  int numcopies;                    ///< number of copies
  bool usecustomsize;               ///< use papersize instead of pagesize
  bool fittopage;                   ///< fit output to page
  bool drawbackground;              ///< fill with background colour
  bool doEPS;                       ///< make encapsulated postscript
  int viewport_x0;                  ///< OpenGL viewport coordinates llcx
  int viewport_y0;                  ///< OpenGL viewport coordinates llcy
  int viewport_width;               ///< OpenGL viewport coordinates width
  int viewport_height;              ///< OpenGL viewport coordinates height
    
  printOptions() :
    orientation(d_print::ori_automatic),
    colop(d_print::incolour),
    pagesize(d_print::A4),
    numcopies(1),
    usecustomsize(false),
    fittopage(true),
    drawbackground(true),
    doEPS(false),
    viewport_x0(0),viewport_y0(0),
    viewport_width(0),viewport_height(0)
  {}

  void printPrintOptions();
};
  
/**
   \brief manager for printing

*/
class printerManager {
private:
  struct printerExtra { // extra commands for postscript
    std::map<std::string,std::string> keys;// keys for matching..
    std::map<std::string,std::string> commands;// Extra output-commands
  };
  static std::vector<printerExtra> printers;
  static std::map<std::string,d_print::PageSize> pages;
  static std::map<d_print::PageSize,d_print::PaperSize> pagesizes;
  static std::string pcommand; // printercommand

public:
  printerManager();
  /// parse the printer section of the setup file
  bool parseSetup();
  /// parse printer-info file
  bool readPrinterInfo(const std::string fname);
  /// page from string
  d_print::PageSize  getPage(const std::string s);
  /// size from page
  d_print::PaperSize getSize(const d_print::PageSize ps);
  /// check if special commands exist for this setup
  bool checkSpecial(const printOptions& po, std::map<std::string,std::string>& mc);
  /// expand variables in print-command
  bool expandCommand(std::string& com, const printOptions& po);
  /// set print command
  void setPrintCommand(const std::string& pc){ pcommand = pc; }
  /// return current print command
  const std::string& printCommand() const { return pcommand; }

private:
  static void initialize();
};

#endif
