
#ifndef QTPROFETSESSIONTABLE_H
#define QTPROFETSESSIONTABLE_H

/*
  $Id$

  Copyright (C) 2006 met.no

  Contact information:
  Norwegian Meteorological Institute
  Box 43 Blindern
  0313 OSLO
  NORWAY
  email: diana@met.no
  
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.
  
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <qlabel.h>
#include <qframe.h>
#include <qtable.h>

#include <vector>
#include <map>

#include <puTools/miString.h>
#include <puTools/miTime.h>

#include <profetSQL/fetParameter.h>
#include <profet/fetObject.h>


using namespace std; 

class ProfetTableCell;

class ProfetSessionTable  : public QTable {
  Q_OBJECT
  
private:
  
  vector<fetObject::Signature> allObjects; // quick check if something has been removed

  map<miTime,int>   times;
  map<miString,int> parameters;

  miTime            currentTime;
  miString          currentParameter;
  
  int               currentRow;
  int               currentCol;
 
  ProfetTableCell*  getCell(miString par, miTime tim);
  
protected:
  
 // don't do any of the following 
 
  void swapRows()    {}
  void swapCells()   {}
  void swapColumns() {} 


public:
  ProfetSessionTable(QWidget*);

  void      initialize( const vector<fetParameter> & p,     
			const vector<miTime>       & t);

  miString  selectedParameter() const { return currentParameter; }
  miTime    selectedTime()      const { return currentTime;      }

  void      selectDefault();
  void      setObjectSignatures( vector<fetObject::Signature> s);
 

public slots:
  
  void cellChanged(int row, int col, miString par, miTime tim);


signals:
  void paramAndTimeChanged(miString par, miTime tim);


};


// --------------- a cell ----------------

class ProfetTableCell: public QLabel{
  Q_OBJECT
private:
  bool     visited;                  // did you visit that field in that session?
  bool     unvisitedObjects;         // are there any objects which you haven't seen yet 
  bool     current;                  // is this the current field?
  int      row;                      // what row is this cell located in
  int      col;                      // what col     - " -
  miString parametername;            // name of the row
  miTime   validTime;                // the col name 
  int      odd;                      // odd julian day? for background color...
  set<fetObject::Signature> objects; // what objects on that field;



protected:
  void focusInEvent ( QFocusEvent * ); 
    
public:
  ProfetTableCell(QWidget * parent,int row_, int col_, miString pname_, miTime vtime_);
    
public slots:
  void dropFocus();
  void setFocus();
  
  void removeObjectSignatures( fetObject::Signature& s );
  void setObjectSignatures(    fetObject::Signature& s );
  void setStatus();
  void setCounter();
  void setTooltip();

signals:
  void newCell(int row_, int col_, miString pname_, miTime vtime_);  

};




#endif
