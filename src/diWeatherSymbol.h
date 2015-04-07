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
#ifndef WeatherSymbol_h
#define WeatherSymbol_h

#include "diObjectPlot.h"
#include "diCommonTypes.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class ComplexSymbolPlot;

/**
   \brief A weather symbol that can be plotted and edited
*/
class WeatherSymbol: public ObjectPlot
{
private:
  float symbolSize;
  std::string symbolString;
  ComplexSymbolPlot * complexSymbol;

  static std::vector<editToolInfo>  allSymbols;
  static std::vector<editToolInfo> allRegions;
  static std::map<std::string,int> symbolTypes;  //finds symbol type number from name
  static std::map<std::string,int> regionTypes;  //finds region type number from name
  static std::map<int,int> indexTypes;  //finds symbol type number
  static std::map<int,int> next;  //finds next symbol type number from number
  static std::map<int,int> last;  //finds last symbol type number from number
  static float defaultSize;
  static float defaultComplexSize;
  static std::string currentText;
  static Colour::ColourInfo currentColour;
  static std::set<std::string> textlist;

public:
 /// default constructor
  WeatherSymbol();
  /// constructor with integer symbol type as argument
  WeatherSymbol(int ty);
  /// constructor with symbol name and type of object as argument
  WeatherSymbol(std::string,int);
  WeatherSymbol(const WeatherSymbol &rhs);
  /// Destructor
  ~WeatherSymbol();

  /// define map to find symbol type number from name
  static void defineSymbols(std::vector<editToolInfo> symbols);
  /// define map to find region type number from name
  static void defineRegions(std::vector<editToolInfo>regions);
  /// set current text for text symbols
  static void setCurrentText(const std::string &);
  /// set current colour for text symbols
  static void setCurrentColour(const Colour::ColourInfo &);
  /// get current text for text symbols
  static std::string getCurrentText();
  /// get current colour for text symbols
  static Colour::ColourInfo getCurrentColour();
  /// returns true if symbol is simple text
  static bool isSimpleText(std::string edittool);
  /// returns true if symbol is complex text
  static bool isComplexText(std::string edittool);
  /// returns true if symbol is colored complex text
  static bool isComplexTextColor(std::string edittool);
  /// returns true if symbol is edit text or textbox
  static bool isTextEdit(std::string edittool);
  /// get std::vectors with text from complex symbols
  static void getCurrentComplexText(std::vector<std::string> & symbolText, std::vector<std::string> & xText);
  /// set std::vectors with text from complex symbols
  static void setCurrentComplexText(const std::vector<std::string>& symbolText,
      const std::vector<std::string>& xText); 
  /// initialise text for complex symbols
  static void initCurrentComplexText(std::string edittool);
  /// sets list of complex texts (used in text dialog)
  static void initComplexList();
  /// get name of region number ir
  static std::string getAllRegions(int ir);
  /// get list of text used in textsymbols (for dialog)
  static std::set<std::string> getTextList();
  /// get list of complex texts (used in text dialog)
  static std::set<std::string> getComplexList();
  /// replace characters like !{ in string
  static void replaceText(std::string& tempString, bool writestring);
  /// set standard size to use for new weathersymbols
  static void setStandardSize(int size1, int size2);
  /// add new point to weather symbol
  void addPoint(float x, float y);
  void plot(PlotOrder zorder);
  /// set type of symbol
  void setType(int ty);
  /// set type of symbol
  bool setType(std::string tystring);
  /// set size of symbol
  void setSize(float si){setSymbolSize(si);}
   /// set size of symbol
  void setSymbolSize(float si);
  /// returns size of symbol
  float getSymbolSize(){return symbolSize;}
  /// returns defaultsize of symbol
  void setDefaultSize( );
  /// changeDefault size of symbol
  void changeDefaultSize( );
  /// increase size of symbol by val
  void increaseSize(float val);
  /// increase type of symbol by val
  void increaseType(int val);
  /// rotate complex symbol by val
  void rotateObject(float val); //only works for complex objects
  /// hide box for complex symbol
  void hideBox(); //only works for complex objects
  /// set white box for complex symbol
  void setWhiteBox(int on); //only works for complex objects
  /// set symbol string
  void setString(const std::string& s);
  /// get symbol string
  std::string getString(){return symbolString;}
  /// returns true if x,y on symbol
  virtual bool isOnObject(float x, float y);
  /// writes a string with Object and Type
  std::string writeTypeString();
  /// apply filter to hide some symbols
  virtual void applyFilters(const std::vector<std::string>&);
  /// gets std::string std::vectors with symboltext or multiline text
  void getMultilineText(std::vector<std::string>& symbolText);
  /// gets std::string std::vectors with symboltext and xText
  void getComplexText(std::vector<std::string>& symbolText, std::vector<std::string> & xText);
  /// reads complex text to be plotted from a string (written by readComplexText)
  void readComplexText(std::string s);
  /// change text to be drawn
  void changeComplexText(const std::vector<std::string>& symbolText, const std::vector<std::string>& xText);
  void changeMultilineText(const std::vector<std::string>& symbolText);

};

#endif
