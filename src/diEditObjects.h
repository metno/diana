/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2015 met.no

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
#ifndef _diEditObjects_h
#define _diEditObjects_h

#include <diField/diArea.h>
#include <diObjectPlot.h>
#include <diWeatherObjects.h>
#include <diAreaBorder.h>
#include <diUndoFront.h>
#include <diMapMode.h>
#include <diField/diGridConverter.h>

#include <map>
#include <set>
#include <vector>

/**
  \brief WeatherObjects to be edited
*/
class EditObjects:public WeatherObjects{
public:

  EditObjects();

  /// defines object modes and combine modes
  static void defineModes(std::map<int,object_modes>, std::map<int,combine_modes>);
  /// initializes class variables to false, clear strings
  void init();
  /// called when a new object plot to be created
  void createNewObject();
  /// called when new edit mode/tool selected in gui (EditDIalog)
  void setEditMode(const mapMode mmode,const int emode, const std::string etool);
  /// flags comments as saved
  void commentsAreSaved(){commentsSaved=true;}
  /// flags comments as changed
  void commentsHaveChanged(){commentsChanged=true;}
  /// checks if comments saved
  bool areCommentsSaved(){return commentsSaved;}
  /// checks if comments changed
  bool haveCommentsChanged(){return commentsChanged;}
  /// flags labels as saved
  void labelsAreSaved(){labelsSaved=true;}
  /// checks if labels saved
  bool areLabelsSaved(){return labelsSaved;}
  /// sets automatic joining of fronts
  void setAutoJoinOn(bool on){autoJoinOn=on;}
  /// checks if automatic joining of fronts on
  bool isAutoJoinOn(){return autoJoinOn;}
  /// draws points where fronts are joined
  void drawJoinPoints(DiGLPainter* gl);
  /// sets scaleToField variable used for plotting areaBorders
  void setScaleToField(float s);
  /// returns text of marked text object
  std::string getMarkedText();
  /// returns colour of marked colored text object
  Colour::ColourInfo getMarkedTextColour();
  /// returns colour of marked text object
  Colour::ColourInfo getMarkedColour();
  /// changes text of marked text object to newText
  void changeMarkedText(const std::string & newText);
  /// changes colour of marked colored text object to new colour
  void changeMarkedTextColour(const Colour::ColourInfo & newColour);
  /// changes colour of marked text object to newText
  void changeMarkedColour(const Colour::ColourInfo & newColour);
  /// get texts of marked complex colored text object
  void getMarkedComplexTextColored(std::vector<std::string>& symbolText, std::vector<std::string>& xText);
  /// get texts of marked multiline text object
  void getMarkedMultilineText(std::vector<std::string>& symbolText);
  /// get texts of marked complex text object
  void getMarkedComplexText(std::vector<std::string>& symbolText, std::vector<std::string>& xText);
  /// changes texts of marked complex colored text object
  void changeMarkedComplexTextColored(const std::vector<std::string>& symbolText, const std::vector<std::string>& xText);
  /// changes texts of marked multiline text object
  void changeMarkedMultilineText(const std::vector<std::string> & symbolText);
  /// changes texts of marked complex text object
  void changeMarkedComplexText(const std::vector<std::string>& symbolText, const std::vector<std::string>& xText);
  /// returns true of current symbol is simple text
  bool inTextMode();
  /// returns true of current symbol is complext text
  bool inComplexTextMode();
  /// returns true of current symbol is complext text (for colored text)
  bool inComplexTextColorMode();

  /// returns true if current symbol is edittext or textbox
  bool inEditTextMode();
  /// initial text for complex text symbols
  void initCurrentComplexText();
  /// returns true if currently drawing an object
  bool inDrawing;


private:
  mapMode mapmode;
  int editmode; // edit mode index
  std::string drawingtool;

  object_modes objectmode;
  combine_modes combinemode;
  static std::map<int,object_modes> objectModes;
  static std::map<int,combine_modes> combineModes;

  //
  bool createobject;

  bool autoJoinOn;
  float newx,newy;

  bool commentsChanged;   // true if comments changed once
  bool commentsSaved;     // true if comments saved since last edit
  bool labelsSaved;     // true if comments saved since last edit
  std::string itsComments;   // old comment string
  std::string startlines;   // start comment string

  WeatherObjects copyObjects;

 //undo methods
  UndoFront * undoCurrent;
  UndoFront * undoHead;
  UndoFront * undoTemp;

public:
  /// sets mouse coordinates to x and y
  void setMouseCoordinates(const float x,const float y);
  /// sets current status of the objects to passive
  void setAllPassive();
  /// marked object points should stay marked, when mouse moves
  void editStayMarked();
  /// unmark objects marked with editStayMarked
  void editNotMarked();
  /// resume drawing or insert point in marked front
  bool editResumeDrawing(const float x, const float y);
  /// deletes all marked points from objects
  bool editDeleteMarkedPoints();
  /// move marked points, x and y are the distance to be moved
  bool editMoveMarkedPoints(const float x, const float y);
  /// rotate front when one point marked, x and y are the distance to be rotated
  bool editRotateLine(const float x, const float y);
  /// copy marked objects
  void editCopyObjects();
  /// paste copyed objects to location of mouse coordinate
  void editPasteObjects();
  /// flip marked objects (fronts and areas)
  void editFlipObjects();
  /// increase size of marked objects by val
  bool editIncreaseSize(float val);
  /// set marked objects to default size
  void editDefaultSize();
  /// set all objects to default size
  void editDefaultSizeAll();
  /// rotate marked objects by val
  void editRotateObjects(float val);
  /// show or hide box for complex symbols
  void editHideBox();
  /// hide all objects
  void editHideAll();
  /// show all hidden objectts
  void editUnHideAll();
  /// hide all combine objects from region
  void editHideCombineObjects(std::string region);
  /// hide all combine objects from region number ir
  void editHideCombineObjects(int ir);
  /// adds a point at x,y to objects being edited
  bool editAddPoint(const float x, const float y);
  /// join fronts
  /**input parameters
  joinAll = true ->all fronts are joined<br>
          = false->only marked or active fronts are joined<br>
  movePoints = true->points moved to join<br>
             = false->points not moved to join<br>
  joinOnLine = true->join front to line not just end points<br>
             = false->join front only to join- and endpoints..<br>
  */
  bool editJoinFronts(bool joinAll, bool movePoints, bool joinOnLine);
  /// merge two fronts of same type into one
  bool editMergeFronts(bool mergeAll);
  /// joined fronts are unjoined, so they can be moved apart
  void editUnJoinPoints();
  ///set symbol default size to size of last read object !
  void changeDefaultSize();
  /// increase object type by val
  bool editChangeObjectType(int val);
  /// split front in two at x,y
  bool editSplitFront(const float x, const float y);
  /// all marked points unmarked
  void unmarkAllPoints();
  /// returns true if x,y over any symbols,fronts,areas
  bool editCheckPosition(const float x, const float y);
  /// sets rubber flag (to draw rubberband) on last objectPlot
  bool setRubber(bool rubber, const float x, const float y);
  /// remove any front with only one point
  void cleanUp();
  /// check all fronts for join points...
  void checkJoinPoints();
  /// put prefix, name and time at start of comments
  void putCommentStartLines(const std::string name, const std::string prefix, const std::string lines);
  ///returns true if comment differ from startlines
  bool hasComments();
  /// returns the comments
  std::string getComments();
  /// set comments
  void putComments(const std::string & comments);
  /// save labels
  void saveEditLabels(const std::vector<std::string>& labels);

  /// a new UndoFront is created and becomes undoCurrent
  void newUndoCurrent(UndoFront*);
  /// undo last objectPlot edit
  bool undofront();
  /// redo last objectPlot edit
  bool redofront();
  /// Removes everything from UndoFront buffers
  void undofrontClear();
  /// save current objects/operations to undoFront, returns true if anything changed
  bool saveCurrentFronts(operation, UndoFront *);
  /// save objects to undoFront after reading, returns true if anything changed
  bool saveCurrentFronts(int, UndoFront *);
  /// reads the current undoBuffer, and updates objects
  bool changeCurrentFronts();

  /// Methods for finding and marking join points on fronts/borders
  /// recursive routine to find all joined fronts belonging to point x,y
  bool findAllJoined(const float x, const float y,objectType wType);
  /// returns true if x,y on joined point
  bool findJoinedPoints(const float x, const float y,objectType wType);
  /// routine to find fronts joined to pfront
  void findJoinedFronts(ObjectPlot*,objectType wType);
};

#endif
