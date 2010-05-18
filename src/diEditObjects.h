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
#ifndef _diEditObjects_h
#define _diEditObjects_h
#include <vector>
#include <diField/diArea.h>
#include <diObjectPlot.h>
#include <diWeatherObjects.h>
#include <diAreaBorder.h>
#include <diUndoFront.h>
#include <diMapMode.h>
#include <diField/diGridConverter.h>

using namespace std; 

/**

  \brief WeatherObjects to be edited


*/




class EditObjects:public WeatherObjects{
public:
  
  EditObjects();
  ~EditObjects(){}
  /// defines object modes and combine modes
  static void defineModes(map<int,object_modes>,map<int,combine_modes>);
  /// initializes class variables to false, clear strings
  void init();
  /// called when a new object plot to be created
  void createNewObject();
  /// called when new edit mode/tool selected in gui (EditDIalog)
  void setEditMode(const mapMode mmode,const int emode,const miutil::miString etool);
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
  void drawJoinPoints();
  /// sets scaleToField variable used for plotting areaBorders
  void setScaleToField(float s);
  /// returns text of marked text object
  miutil::miString getMarkedText();
  /// returns colour of marked text object
  Colour::ColourInfo getMarkedColour();
  /// changes text of marked text object to newText
  void changeMarkedText(const miutil::miString & newText);
  /// changes colour of marked text object to newText
  void changeMarkedColour(const Colour::ColourInfo & newColour);
  /// get texts of marked complex text object
  void getMarkedComplexText(vector <miutil::miString> & symbolText, vector <miutil::miString> & xText);
  /// changes texts of marked complex text object
  void changeMarkedComplexText(const vector <miutil::miString> & symbolText, const vector <miutil::miString> & xText);
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
  int edittool; // edit tools index
  miutil::miString drawingtool; 

  object_modes objectmode;
  combine_modes combinemode;
  static map<int,object_modes> objectModes;
  static map<int,combine_modes> combineModes;

  //
  bool createobject;

  bool autoJoinOn;
  float newx,newy;

  bool commentsChanged;   // true if comments changed once
  bool commentsSaved;     // true if comments saved since last edit
  bool labelsSaved;     // true if comments saved since last edit
  miutil::miString itsComments;   // old comment string

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
  void editHideCombineObjects(miutil::miString region);
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
  /// used for internal testing
  void editTestFront();
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
  void putCommentStartLines(miutil::miString name,miutil::miString prefix);
  /// returns the comments
  miutil::miString getComments();
  /// set comments
  void putComments(const miutil::miString & comments);
  /// add comments
  void addComments(const miutil::miString & comments){itsComments+=comments;}
  /// save labels
  void saveEditLabels(vector <miutil::miString> labels);

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

