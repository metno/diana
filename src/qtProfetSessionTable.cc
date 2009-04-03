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

#include "qtProfetSessionTable.h"
#include <qcolor.h>
#include <qtooltip.h>
//Added by qt3to4:
#include <QFocusEvent>
#include <QLabel>
#include <sstream>

namespace TABLECOLORS {

  static const QColor EMPTYODD(255,255,255);
  static const QColor EMPTYEVEN(210,210,210);

  static const QColor VISITEDODD(255,240,220);
  static const QColor VISITEDEVEN(215,200,180);

  static const QColor HASNEWOBJECTS(240,125,31);
  static const QColor HASKNOWNOBJECTS(78,240,89);

  static const QColor CURRENT(230,0,24);

};


ProfetSessionTable::ProfetSessionTable(QWidget* parent) :
  Q3Table(parent)
{
  verticalHeader()->setClickEnabled(true);
  horizontalHeader()->setClickEnabled(true);

  connect(verticalHeader(),SIGNAL(clicked(int)),this,SLOT(rowClicked(int)));
  connect(horizontalHeader(),SIGNAL(clicked(int)),this,SLOT(columnClicked(int)));
}


void ProfetSessionTable::rowClicked(int r){
  miString pname = ((ProfetTableCell*)(cellWidget(r,0)))->getParameter();
  miTime time;
  emit selectedPart(pname, time);
}

void ProfetSessionTable::columnClicked(int c){
  miString pname;
  miTime time = ((ProfetTableCell*)(cellWidget(0,c)))->getValidTime();
  emit selectedPart(pname, time);
}

void ProfetSessionTable::cornerClicked(){
  miString pname;
  miTime time;
  emit selectedPart(pname, time);
}

void ProfetSessionTable::selectDefault()
{
  if(!times.size() || !parameters.size()) return;

  miTime now=miTime::nowTime();

  int col=0;
  int row=0;

  map<miTime,int>::iterator itr=times.begin();
  for(;itr!=times.end();itr++)
    if(itr->first >= now ) {
      col=itr->second;
      break;
    }

  ProfetTableCell* p =(ProfetTableCell*)cellWidget(row,col);
  if(p)
    p->setFocus();
  setCurrentCell(row,col);
}

void ProfetSessionTable::cellChanged(int row, int col,miString par, miTime tim)
{
  currentParameter = par;
  currentTime      = tim;

  if(currentRow >= 0 && currentCol >= 0 && currentRow < numRows() && currentCol < numCols() ) {

    ProfetTableCell* p =(ProfetTableCell*)cellWidget(currentRow,currentCol);

    if(p)
      p->dropFocus();
  }
  currentRow=row;
  currentCol=col;
  setCurrentCell(row,col);

  emit paramAndTimeChanged(par,tim);
}


ProfetTableCell* ProfetSessionTable::getCell(miString par, miTime tim)
{
  if(!times.count(tim))      return 0;
  if(!parameters.count(par)) return 0;

  ProfetTableCell* p =(ProfetTableCell*)cellWidget(parameters[par],times[tim]);

  return p;
}

void ProfetSessionTable::setObjectSignatures( vector<fetObject::Signature> s)
{
  set<fetObject::Signature> rm;

  cerr << "NEW SIGNATURES: " << s.size() << endl;

  for(int i=0; i<s.size();i++) {
    rm.insert(s[i]);

    ProfetTableCell* p =getCell(s[i].parameter,s[i].validTime);

    if(p) p->setObjectSignatures(s[i]);
  }

  // check if something has to be removed .....

  for(int i=0; i<allObjects.size();i++)
    if(!rm.count(allObjects[i])) {
      ProfetTableCell* p =getCell(allObjects[i].parameter,allObjects[i].validTime);
      if(p) p->removeObjectSignatures(allObjects[i]);
    }

  allObjects=s;
}

void ProfetSessionTable::initialize(const vector<fetParameter> & p,
				    const vector<miTime> & t)
{
  if(!p.size() || !t.size()) return;

  setNumCols(t.size());
  setNumRows(p.size());

  for(int col=0;col<t.size();col++){
    miString tstr=t[col].format("%a %k");
    horizontalHeader()->setLabel(col, tstr.cStr());
    setColumnWidth(col,50);
    times[t[col]]=col;
    adjustColumn(col);
  }

  for(int row=0;row<p.size();row++) {
    verticalHeader()->setLabel(row, p[row].description().cStr());
    parameters[p[row].name()]=row;
  }



  for(int row=0;row<p.size();row++)
    for(int col=0;col<times.size();col++) {
      setCellWidget(row,col,new ProfetTableCell(this,row,col, p[row].name(),t[col]));
      connect( cellWidget(row,col), SIGNAL (newCell(int,int,miString,miTime)),
	       this, SLOT (cellChanged(int,int,miString,miTime)));

    }


}





/////////////////////// profetTableCell --------------------------------------------------------

ProfetTableCell::ProfetTableCell(QWidget * parent,int row_, int col_, miString pname_, miTime vtime_)
  : QLabel(" ",parent)
{
  col             = col_;
  row             = row_;
  parametername   = pname_;
  validTime       = vtime_;
  odd             = validTime.date().julianDay()%2;
  unvisitedObjects= false;
  visited         = false;
  current         = false;

  setFocusPolicy(Qt::StrongFocus);
  setAlignment(Qt::AlignHCenter);
  setTextFormat(Qt::RichText);
  setStatus();
}


void ProfetTableCell::focusInEvent ( QFocusEvent * )
{
  setFocus();
}

void ProfetTableCell::setFocus()
{
  current          = true;
  unvisitedObjects = false;
  setStatus();
  emit newCell(row,col,parametername,validTime);
}

void ProfetTableCell::dropFocus()
{
  current=false;
  visited=true;
  setStatus();
}


void ProfetTableCell::setStatus()
{
  if(current) {
    setPaletteBackgroundColor(TABLECOLORS::CURRENT);
    return;
  }

  if(objects.size()) {
    if(unvisitedObjects)
      setPaletteBackgroundColor(TABLECOLORS::HASNEWOBJECTS);
    else
      setPaletteBackgroundColor(TABLECOLORS::HASKNOWNOBJECTS);
    return;
  }

  if(odd) {
    if(visited)
      setPaletteBackgroundColor(TABLECOLORS::VISITEDODD);
    else
      setPaletteBackgroundColor(TABLECOLORS::EMPTYODD);
    return;
  }


  if(visited)
    setPaletteBackgroundColor(TABLECOLORS::VISITEDEVEN);
  else
    setPaletteBackgroundColor(TABLECOLORS::EMPTYEVEN);

}

void ProfetTableCell::setObjectSignatures(fetObject::Signature& s)
{
  if(!objects.count(s))
    objects.insert(s);



  if( !current ) unvisitedObjects=true;
  setCounter();
  setTooltip();
}

void ProfetTableCell::removeObjectSignatures(fetObject::Signature& s)
{
  if(!objects.count(s)) return;

  objects.erase(s);
  setCounter();
}

void ProfetTableCell::setCounter()
{
  ostringstream ost;
  ost << "<b>" << objects.size() << "</b>";
 // qt4 fix: setText takes QString as argument
   setText(QString(ost.str().c_str()));
  setStatus();
}

void ProfetTableCell::setTooltip()
{
  QToolTip::remove(this);

  if(objects.size()) {

    ostringstream ost;
    set<fetObject::Signature>::iterator itr=objects.begin();

    for(;itr!=objects.end();itr++ ) {
      ost <<  "<b>Object:</b> "     << itr->name     << "<br>"
	  <<  "<b>Changed by</b>: " << itr->user     << "<br>"
	  <<  "<b>at: </b>"         << itr->editTime << "<br>"
	  <<  "<hl>";
	}


 // qt4 fix: QToolTip::add takes QString as argument
    QToolTip::add(this, QString(ost.str().c_str()));
  }
}
