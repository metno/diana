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
#ifndef _diUndoFront_h
#define _diUndoFront_h

#include <vector>
#include <diObjectPlot.h>
#include <diField/diArea.h>

  //for undo/redo stuff
enum operation{AddPoint,MoveMarkedPoints,DeleteMarkedPoints,FlipObjects,
	       IncreaseSize,ChangeObjectType,RotateLine,ResumeDrawing,
	       SplitFronts,JoinFronts,ReadObjects,DefaultSize,DefaultSizeAll,
	       HideAll,CleanUp,RotateObjects,ChangeText,HideBox,PasteObjects};

enum action{Insert,Replace,Erase,Update};

/**
  \brief one saved weather object for undo operations
*/
struct saveObject{
  ObjectPlot * object; ///<saved weather object
  int place;           ///< where to insert/erase/replace in vector
  action todo;         ///<  insert = 0, replace = 1, erase = 2
};


/**
  \brief Undo information saved after each edit operation
*/
class UndoFront {
public:
  UndoFront();
  ~UndoFront();
  /// add object to undo buffer
  void undoAdd(action,ObjectPlot*,int);
  /// saved undo objects
  std::vector<saveObject> saveobjects;

  /// pointer to last undo
  UndoFront *Last;
  /// pointer to next undo
  UndoFront *Next;

  /// old area kept for changing projections
  Area oldArea;

  /// operation done
  operation iop; 
};

#endif
