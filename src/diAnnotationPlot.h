/*
 Diana - A Free Meteorological Visualisation Tool

 Copyright (C) 2006-2014 met.no

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
#ifndef diAnnotationPlot_h
#define diAnnotationPlot_h

#include "diPlot.h"
#include <puTools/miTime.h>
#include <vector>
#include <map>

class LegendPlot;

/**
 \brief Plotting text, legends etc on the map

 Includes: text, symbol, image, input, legend, arrow, box
 */
class AnnotationPlot: public Plot {

  enum annoType {
    anno_data, anno_text,
  };

  enum elementType {
    text, symbol, image, input, table, arrow, box
  };

  struct Border {
    float x1, x2, y1, y2;
  };

  struct element {
    std::vector<element> subelement;
    elementType eType;
    std::string eText;
    int eCharacter;
    std::string eFont;
    std::string eFace;
    std::string textcolour;
    std::string eImage;
    LegendPlot* classplot;
    float arrowLength;
    bool arrowFeather;
    std::string eName;
    Alignment eHalign; //where it makes sense
    float eSize;
    float x1, y1, x2, y2;//dimensions and size of element
    bool isInside, inEdit;
    int itsCursor, itsSel1, itsSel2;
    int eAlpha;
    float width, height;
    bool horizontal;
    polyStyle polystyle;
    element():classplot(0)
    {
    }
  };
  typedef std::vector<element> element_v;

public:
  /**

   \brief Annotation text, format, alignment etc

   */
  struct Annotation {
    std::string str;
    std::vector<std::string> vstr;
    Colour col;
    Alignment hAlign;
    polyStyle polystyle;
    Colour bordercolour;
    std::vector<element> annoElements;
    bool spaceLine;
    float wid, hei;
    Rectangle rect;
    bool oldFormat;
  };

private:

  typedef std::vector<Annotation> Annotation_v;
  Annotation_v annotations;
  Annotation_v orig_annotations;
  annoType atype;
  std::vector<Border> borderline;

  //for comparing input text
  std::map<std::string, std::string> inputText;

  // OKstring variables
  float cmargin;
  float cspacing;
  float cxoffset;
  float cyoffset;
  int clinewidth; //line width of border
  float cxratio; //ratio between frame and annotation box in x
  float cyratio; //ratio between frame and annotation box in x
  bool plotRequested;//annotations aligned rel. to frame (not window)
  bool nothingToDo;
  //
  std::string labelstrings; //fixed part of okstrings
  std::string productname;
  bool editable;

  Rectangle bbox;
  bool scaleAnno, plotAnno;
  bool isMarked;
  //float xbox,ybox;
  float fontsizeToPlot;
  float scaleFactor;
  float border;
  float spacing;
  float maxwid;

  bool useAnaTime;
  std::vector<miutil::miTime> fieldAnaTime;
  Colour currentColour;

  //called from constructor
  void init();
  // insert time in text string
  const std::string insertTime(const std::string&, const miutil::miTime&);
  // expand string-variables
  const std::vector<std::string> expanded(const std::vector<std::string>&);
  // decode string, put into elements
  void splitAnnotations();
  bool putElements();
  void addElement2Vector(std::vector<element>& v_e, const element& e, int index);
  bool decodeElement(std::string elementstring, element& el);
  //get size of annotation line
  void getAnnoSize(std::vector<element>& annoEl, float& wid, float& hei,
      bool horizontal = true);
  void getXYBox();
  bool plotElements(std::vector<element>& annoEl, float& x, float& y,
      float annoHeight, bool horizontal = true);
  float plotArrow(float x, float y, float l, bool feather = false);
  void plotBorders();
  std::vector<std::string> split(const std::string, const char, const char);
  std::string writeElement(element& annoEl);

public:
  AnnotationPlot();
  AnnotationPlot(const std::string&);
  ~AnnotationPlot();

  void plot(PlotOrder zorder);

  ///decode plot info strings
  bool prepare(const std::string&);
  ///set data annotations
  bool setData(const std::vector<Annotation>& a,
      const std::vector<miutil::miTime>& fieldAnalysisTime);
  void setfillcolour(std::string colname);
  /// mark editable annotationPlot if x,y inside plot
  bool markAnnotationPlot(int, int);
  /// get text of marked and editable annotationPlot
  std::string getMarkedAnnotation();
  /// change text of marked and editable annotationplot
  void changeMarkedAnnotation(std::string text, int cursor = 0, int sel1 = 0,
      int sel2 = 0);
  /// delete marked and editable annotation
  void DeleteMarkedAnnotation();
  /// start editing annotations
  void startEditAnnotation();
  /// stop editing annotations
  void stopEditAnnotation();
  /// go to next element in annotation being edited
  void editNextAnnoElement();
  /// go to last element in annotation being edited
  void editLastAnnoElement();
  /// put info from saved edit labels into new annotation
  void updateInputLabels(const AnnotationPlot * oldAnno, bool newProduct);
  /// return std::vector std::strings with edited annotation for product prodname
  std::string writeAnnotation(std::string prodname);
  void setProductName(std::string prodname)
  {
    productname = prodname;
  }
  //get raw annotation objects
  const std::vector<Annotation>& getAnnotations();
  //get annotations, change them somewhere else, and put them back
  std::vector<std::vector<std::string> > getAnnotationStrings();
  ///replace annotations
  bool setAnnotationStrings(std::vector<std::vector<std::string> >& vstr);

  Rectangle getBoundingBox() const {return bbox;}
};

#endif
