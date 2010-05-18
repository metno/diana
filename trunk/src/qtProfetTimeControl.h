#ifndef QTPROFETTIMECONTROL_H_
#define QTPROFETTIMECONTROL_H_


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


#include <QLayout>

#include "qtProfetSingleControl.h"
#include <profet/fetObject.h>

#include <map>
#include <vector>
#include <set>

#include <puTools/miString.h>
#include <puTools/miTime.h>
#include <puTools/miRing.h>
using namespace std;


class ProfetTimeControl : public QWidget {
  Q_OBJECT
public:
  enum methodTypes{LINEAR=0,GAUSS=1,COPY=2,RESETLINE=3,RESETSINGLE=4};
  
private:
  QGridLayout * gridl;
  
  
  vector<ProfetSingleControl*>      objects;
  map<miutil::miTime,int>                   time_index;
  set<miutil::miString>                     parameters;
  
  ring< vector<fetObject::TimeValues> >  undobuffer;
  ring< vector<fetObject::TimeValues> >  redobuffer;
  
  
  int                               parenttimestep;
  methodTypes                       method; 
  bool                              changed;
  void clearline(int from,int to,miutil::miString par);
  void interpolate(miutil::miString par, int col);
  void setAll(vector<fetObject::TimeValues>);
  
public:
	ProfetTimeControl(QWidget* parent,vector<fetObject::TimeValues>& obj, vector<miutil::miTime>& times);
	
	void setMethod(ProfetTimeControl::methodTypes);
	bool hasChanged() const { return changed; }
	void setChanged(bool a) { changed=a;      }
	bool undo();
	bool redo();
	void processed(miutil::miTime tim, miutil::miString obj_id);
	
  vector<fetObject::TimeValues>  collect(bool removeDiscardables=false);
	
  ProfetSingleControl* parentObject() const {return objects[parenttimestep];}
  ProfetSingleControl* focusObject()  const;
  
  void toggleParameters(miutil::miString p);
  
  
public slots:
  void buttonAtPressed(int);
  void pushundo();
	
};



#endif /*QTPROFETTIMECONTROL_H_*/
