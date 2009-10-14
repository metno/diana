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
#ifndef _diObjectManager_h
#define _diObjectManager_h

#include <vector>
#include <map>
#include <set>
#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <diField/diArea.h>
#include <diMapMode.h>
#include <diWeatherObjects.h>
#include <diEditObjects.h>
#include <diDisplayObjects.h>
#include <diAreaBorder.h>
#include <diSetupParser.h>
#include <diCommonTypes.h>
#include <diUndoFront.h>
#include <diTimeFilter.h>

using namespace std;


class PlotModule;


  /**
     \brief list of object files
  */
struct ObjectList{
  miString filename;         ///< filename prefix, to be appended with time
  vector<ObjFileInfo> files; ///< filename and time
  bool updated;              ///< true if list updated
  bool archive;              ///< true if list should include archive
  TimeFilter filter;         ///< time filter to apply
  PlotOptions poptions;      ///< plotoptions to apply
};



/**

  \brief Manager for weather objects to be displayed or edited

*/

class ObjectManager {
private:
  PlotModule* plotm;

  bool objectsChanged;
  bool objectsSaved;
  bool doCombine;

  //object dialog stuff
  miTime ztime;

  // from diana.setup:
  vector<miString> objectNames;
  map<miString,ObjectList> objectFiles;

  UndoFront * undoTemp;

  mapMode mapmode;

  miString EditCommentName(const miString region,
			   const miTime& t);

  // list object files (for object dialog)
  vector<ObjFileInfo> listFiles(ObjectList & ol);

  bool autoJoinOn();

  bool getFileName(DisplayObjects &wObjects);
  miTime timeFilterFileName(miString fileName,TimeFilter filter);
  miTime timeFromString(miString timeString);
  //get time string yyyymmddhh or yyyymmddhhmm from time
  miString stringFromTime(const miTime& t,bool addMinutes);

  bool checkFileName(miString &fileName);
  //test to check whether file exist
  bool _isafile(const miString name);




public:
  ObjectManager(PlotModule*);
  ~ObjectManager();

  /// returns true if currently drawing objects
  bool inDrawing();
  /// returns true if objects are changed
  bool haveObjectsChanged(){return objectsChanged;}
  /// set objectsSaved flag to saved
  void setObjectsSaved(bool saved){objectsSaved=saved;}
  /// returns true if objects are saved
  bool areObjectsSaved(){return objectsSaved;}
  /// sets doCombine flag to combine
  void setDoCombine(bool combine){doCombine=combine;}
  /// returns true if objects should be combined
  bool toDoCombine(){return doCombine;}
  /// parse OBJECTS section of setup file
  bool parseSetup(SetupParser& sp);
  /// get ObjectNames from setup file to be used in dialog etc.
  vector<miString> getObjectNames(bool archive);
  /// insert name and file into objectList
  bool insertObjectName(const miString & name, const miString & file );
  /// sets current weather symbol text
  void setCurrentText(const miString &);
  /// sets current weather symbol colour
  void setCurrentColour(const Colour::ColourInfo & newColour);
  /// get current weather symbol text
  miString getCurrentText();
  /// get current weather symbol colour
  Colour::ColourInfo getCurrentColour();
  /// get text from marked text symbols in editObjects
  miString getMarkedText();
  /// get colour from marked text symbols in editObjects
  Colour::ColourInfo getMarkedColour();
  /// change text of marked text symbols in editObjects
  void changeMarkedText(const miString & newText);
  /// change colour of marked text symbols in editObjects
  void changeMarkedColour(const Colour::ColourInfo & newColour);
  /// get text list from weather symbol
  std::set<miString> getTextList();
  /// returns true if currently editing text symbol
  bool inTextMode();
  /// returns true if currently editing complex text symbol
  bool inComplexTextMode();
  /// gets current text of complex text symbols
  void getCurrentComplexText(vector <miString> & symbolText, vector <miString>& xText);
  /// sets current text of complex text symbols
  void setCurrentComplexText(const vector <miString> & symbolText, const vector <miString> & xText);
  /// inits current text of complex text symbols
  void initCurrentComplexText();
   /// get texts of marked complex text object
  void getMarkedComplexText(vector <miString> & symbolText, vector <miString> & xText);
  /// changes texts of marked complex text object
  void changeMarkedComplexText(const vector <miString> & symbolText, const vector <miString> & xText);
  /// get text list from complex weather symbol
  set <miString> getComplexList();
  /// decode string with types of objects to plot
  map <miString,bool> decodeTypeString(miString);
  /// prepare objects for displaying
  bool prepareObjects(const miTime& t,
		      const Area& area,
		      DisplayObjects& objects);
  /// reads the file with weather objectPlots
  bool readEditDrawFile(const miString file,
			const Area& area,
			WeatherObjects& objects);
  /// reads the file with edit comments
  bool readEditCommentFile(const miString file,
			   WeatherObjects& objects);
  /// writes the file with weather objectPlots
  bool writeEditDrawFile(const miString filename,
			 const miString outputString);
  /// writes the string with weather objectPlots
  miString writeEditDrawString(const miTime& t,
			 WeatherObjects& objects);

  // drawing events
  /// called when a new object plot to be created
  void createNewObject();
  /// returns true if x,y over any symbols,fronts,areas
  bool editCheckPosition(const float x, const float y);
  /// adds a point at x,y to objects being edited
  void editAddPoint(const float x, const float y);
  /// move marked points, x and y are the distance to be moved
  bool editMoveMarkedPoints(const float x, const float y);
  /// rotate front when one point marked, x and y are the distance to be rotated
  bool editRotateLine(const float x, const float y);
  /// deletes all marked points from objects
  void editDeleteMarkedPoints();
  /// copy marked objects
  void editCopyObjects();
  /// paste copyed objects to location of mouse coordinate
  void editPasteObjects();
  /// flip marked objects (fronts and areas)
  void editFlipObjects();
  /// all marked points unmarked
  void editUnmarkAllPoints();
  /// increase size of marked objects by val
  void editIncreaseSize(float val);
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
  /// hide combining objects (borders etc)
  void editHideCombining();
  /// unhide combing objects (borders etc)
  void editUnHideCombining();
  /// hide combine objects from region
  void editHideCombineObjects(int);
  /// increase object type by val
  void editChangeObjectType(int val);
  /// Sets current status of the objects to passive
  void setAllPassive();
   /// sets rubber flag (to draw rubberband) on last objectPlot
  bool setRubber(bool rubber, const float x, const float y);
  /// resume drawing or insert point in marked front
  void editResumeDrawing(const float x, const float y);
  /// split front in two at x,y
  void editSplitFront(const float x, const float y);
  /// used for internal testing
  void editTestFront();
  /// joined fronts are unjoined, so they can be moved apart
  void editUnJoinPoints();
  /// marked object points should stay marked, when mouse moves
  void editStayMarked();
  /// unmark objects marked with editStayMarked
  void editNotMarked();
  /// merge two fronts of same type into one
  void editMergeFronts(bool);
  /// remove any front with only one point
  void cleanUp();
  /// check all fronts for join points...
  void checkJoinPoints();
  /// undo last objectPlot edit
  bool undofront();
  /// redo last objectPlot edit
  bool redofront();
  /// Removes everything from UndoFront buffers
  void undofrontClear();
  /// sets automatic joining of fronts active
  void autoJoinToggled(bool on);
  /// makes temporary undo buffer, in case changes occur in objectPlots
  void editPrepareChange(const operation op);
  /// called when mouse released while editing
  void editMouseRelease(bool moved);
  /// called after objectPlot editing to save changes to undo buffer
  void editPostOperation();
  /// save changes to undo buffer when new objects added
  void editNewObjectsAdded(int);
  /// join fronts
  /**input parameters
  joinAll = true ->all fronts are joined<br>
          = false->only marked or active fronts are joined<br>
  movePoints = true->points moved to join<br>
             = false->points not moved to join<br>
  joinOnLine = true->join front to line not just end points<br>
             = false->join front only to join- and endpoints..<br>
  */
  void editCommandJoinFronts(bool joinAll, bool movePoints, bool joinOnLine);
  /// read edit file with objects
  bool editCommandReadDrawFile(const miString filename);
  /// read edit file with comments
  bool editCommandReadCommentFile(miString filename);
  /// return editobjects' comments
  miString getComments();
  /// read the old comments
  miString readComments(bool);
  /// set comments
  void putComments(const miString & comments);
  /// put prefix, name and time at start of comments
  void putCommentStartLines(miString name,miString prefix);
  /// called when new edit mode/tool selected in gui (EditDIalog)
  void setEditMode(const mapMode mmode,const int emode,const miString etool);
  // stop drawing objects
  void editStopDrawing();

  //Object dialog methods
  /// get prefix from a file with name  /.../../prefix_*.yyyymmddhh
  miString prefixFileName(miString fileName);
  /// get time from a file with name *.yyyymmddhh
  miTime timeFileName(miString fileName);
  /// returns list of objectfiles for use in dialog
  vector <ObjFileInfo> getObjectFiles(const miString objectname,
				      bool refresh);
  /// returns list of times
  vector<miTime> getObjectTimes(const vector<miString>&pinfos);
  vector<miTime> getObjectTimes(const miString& pinfo);
  ///returns union or intersection of plot times from all pinfos
  void getCapabilitiesTime(vector<miTime>& normalTimes,
			   miTime& constTime,
			   int& timediff,
			   const miString& pinfo);
  /// returns plot options for object file with name objectname
  PlotOptions getPlotOptions(miString objectName);

};


#endif













