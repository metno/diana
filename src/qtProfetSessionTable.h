
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
#include <q3frame.h>
#include <q3table.h>
//Added by qt3to4:
#include <QFocusEvent>

#include <vector>
#include <map>

#include <puTools/miString.h>
#include <puTools/miTime.h>

#include <profet/fetParameter.h>
#include <profet/fetObject.h>


using namespace std; 

class ProfetTableCell;

class ProfetSessionTable  : public Q3Table {
  Q_OBJECT
  
private:
  
  vector<fetObject::Signature> allObjects; // quick check if something has been removed

  map<miutil::miTime,int>   times;
  map<miutil::miString,int> parameters;

  miutil::miTime            currentTime;
  miutil::miString          currentParameter;
  
  int               currentRow;
  int               currentCol;
 
  ProfetTableCell*  getCell(miutil::miString par, miutil::miTime tim);
  
protected:
  
 // don't do any of the following 
 
  void swapRows()    {}
  void swapCells()   {}
  void swapColumns() {} 


public:
  ProfetSessionTable(QWidget*);

  void      initialize( const vector<fetParameter> & p,     
			const vector<miutil::miTime>       & t);

  miutil::miString  selectedParameter() const { return currentParameter; }
  miutil::miTime    selectedTime()      const { return currentTime;      }

  void      selectDefault();
  void      setObjectSignatures( vector<fetObject::Signature> s);
 

public slots:
  
  void cellChanged(int row, int col, miutil::miString par, miutil::miTime tim);
  void rowClicked(int);
  void columnClicked(int);
  void cornerClicked();

signals:
  void paramAndTimeChanged(miutil::miString par, miutil::miTime tim);
  void selectedPart(miutil::miString par, miutil::miTime tim);

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
  miutil::miString parametername;            // name of the row
  miutil::miTime   validTime;                // the col name 
  int      odd;                      // odd julian day? for background color...
  set<fetObject::Signature> objects; // what objects on that field;

protected:
  void focusInEvent ( QFocusEvent * ); 
    
public:
  ProfetTableCell(QWidget * parent,int row_, int col_, miutil::miString pname_, miutil::miTime vtime_);
  
  miutil::miString getParameter() const { return parametername; }
  miutil::miTime   getValidTime() const { return validTime; }
    
public slots:
  void dropFocus();
  void setFocus();
  
  void removeObjectSignatures( fetObject::Signature& s );
  void setObjectSignatures(    fetObject::Signature& s );
  void setStatus();
  void setCounter();
  void setTooltip();

signals:
  void newCell(int row_, int col_, miutil::miString pname_, miutil::miTime vtime_);  

};

#endif
