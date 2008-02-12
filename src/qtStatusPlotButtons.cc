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

#include <qtStatusPlotButtons.h>
#include <qtooltip.h>
#include <q3grid.h>
#include <qlayout.h>
#include <qlabel.h>
#include <q3scrollview.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <Q3GridLayout>
#include <QFocusEvent>
#include <Q3Frame>
#include <Q3PopupMenu>
#include <QKeyEvent>
#include <iostream>
#include <qtImageGallery.h>
#include <qimage.h>
#include <qpixmap.h>

// #include <earth3.xpm>
// #include <weather_rain.xpm>
#include <question.xpm>
// #include <sun2.xpm>
// #include <shuttle.xpm>
// #include <synop.xpm>
// #include <felt.xpm>
// #include <sat.xpm>
// #include <front.xpm>
// #include <dialoger.xpm>
// #include <balloon.xpm>
// #include <traj.xpm>
// #include <vcross.xpm>
#include <qtooltip.h>

static bool oktoemit;


PlotButton::PlotButton(QWidget * parent,
		       PlotElement& pe,
		       const char * name)
  : QToolButton(parent, name)
{
  setMinimumWidth(30);

  setPlotElement(pe);

  connect(this,SIGNAL(toggled(bool)),SLOT(togg(bool)));
  
  origcolor_= paletteBackgroundColor();
}
  
void PlotButton::setPlotElement(const PlotElement& pe)
{
  QtImageGallery ig;
  miString str= pe.str;
  if (pe.icon.exists()){
    if (plotelement_.icon!=pe.icon ){
      QImage image;
      if(ig.Image(pe.icon,image)){
	QPixmap p(image); 
	setPixmap(p);
      } else { 
	setPixmap(question_xpm);    
      }
    }
  } else {
    setPixmap(question_xpm);
  }

  plotelement_= pe;
  
  tipstr_= QString(str.cStr());
  QToolTip::add(this, tipstr_);
  setToggleButton(true);
  setOn(plotelement_.enabled);
}

void PlotButton::togg(bool b)
{
  plotelement_.enabled= b;
  if (oktoemit) 
    emit enabled(plotelement_);
}

void PlotButton::highlight(bool b)
{
  if (b){
    setPaletteBackgroundColor(origcolor_.dark(200));
  } else {
    setPaletteBackgroundColor(origcolor_);
  }
}


StatusPlotButtons::StatusPlotButtons(QWidget* parent, const char* name)
  : QWidget(parent,name), numbuttons(0), activebutton(0) {
  
  setMaximumHeight(35);
  //setFocusPolicy(QWidget::TabFocus);

  Q3HBoxLayout* hl= new Q3HBoxLayout(this,0); // parent,margin
  sv = new Q3ScrollView(this);
  sv->setFrameStyle(Q3Frame::NoFrame);

  hl->addWidget(sv);

  QWidget *w= new QWidget(sv->viewport());
  // rows, cols, border and space
  grid= new Q3GridLayout(w, 1, MAXBUTTONS, 0, 0);

  sv->addChild(w);

  sv->setResizePolicy(Q3ScrollView::AutoOneFit);
  sv->setVScrollBarMode(Q3ScrollView::AlwaysOff);
  sv->setHScrollBarMode(Q3ScrollView::AlwaysOff);

  PlotElement pe;
  pe.enabled= true;
  oktoemit= false;
  
  for (int i=0; i<MAXBUTTONS; i++){
    buttons[i] = new PlotButton(w, pe);
    connect(buttons[i], SIGNAL(enabled(PlotElement)),
	    this, SLOT(enabled(PlotElement)));
    grid->addWidget(buttons[i], 0, i);
  }
  grid->activate();
  
  showtip= new Q3PopupMenu(this);
  showtip->setPaletteBackgroundColor(QColor(255,250,205));
}

void StatusPlotButtons::calcTipPos()
{
  int fx= x()+width();
  int bx= x()+(numbuttons>0 ? buttons[numbuttons-1]->x() +
	       buttons[numbuttons-1]->width() : 0);

  int tx= (fx < bx ? fx : bx) + 10;
  int ty= y() + 5;
  
  QPoint localtip_pos= QPoint(tx,ty);
  tip_pos = mapToGlobal(localtip_pos);
}

void StatusPlotButtons::setfocus()
{
  grabKeyboard();
  activebutton= 0;

  showActiveButton(true);
}


void StatusPlotButtons::showText(const QString& s)
{
  calcTipPos();
  showtip->popup(tip_pos);

  showtip->clear();
  showtip->insertItem(s);
}

void StatusPlotButtons::showActiveButton(bool b)
{
  if (activebutton>=0 && activebutton<numbuttons){
    buttons[activebutton]->highlight(b);
    
    int x= buttons[activebutton]->x();
    int y= buttons[activebutton]->y();
    sv->ensureVisible(x,y);

    if (b) showText(buttons[activebutton]->tipText());
  }
}

void StatusPlotButtons::releasefocus()
{
  showActiveButton(false);
  activebutton= 0;
  showActiveButton(false);
  releaseKeyboard();
  showtip->hide();
}

void StatusPlotButtons::keyPressEvent ( QKeyEvent * e )
{
  // no modifiers recognized here
  if (e->state() != Qt::NoButton){
    releasefocus();
    e->ignore();
    return;
  }

  if (e->key() == Qt::Key_Left){
    if (activebutton > 0){
      showActiveButton(false);
      activebutton--;
      showActiveButton(true);
    }
  } else if (e->key() == Qt::Key_Right){
    if (activebutton < numbuttons-1){
      showActiveButton(false);
      activebutton++;
      showActiveButton(true);
    }
  } else if (e->key() == Qt::Key_Up || e->key() == Qt::Key_Down){
    if (activebutton>=0 && activebutton < numbuttons){
      buttons[activebutton]->toggle();
    }
  } else if (e->key() == Qt::Key_End){
    releasefocus();
  } else {
    releasefocus();
    e->ignore();
  }
}

void StatusPlotButtons::focusInEvent ( QFocusEvent * )
{
}

void StatusPlotButtons::focusOutEvent ( QFocusEvent * )
{
}

void StatusPlotButtons::reset()
{
  oktoemit= true;
  for (int i=0; i<numbuttons; i++){
    if (!buttons[i]->isOn()){
      buttons[i]->toggle();
      buttons[i]->setDown(true);
    }
  }
  oktoemit= false;
}


void StatusPlotButtons::setPlotElements(const vector<PlotElement>& vpe)
{
  oktoemit= false;

  int n= vpe.size();
  if (n>MAXBUTTONS) n= MAXBUTTONS;

  // first clean up
  for (int i=n; i<MAXBUTTONS; i++){
    buttons[i]->hide();
  }

  // add buttons
  for (int i=0; i<n; i++){
    //if (vpe[i].type=="" || vpe[i].str=="") continue;
    buttons[i]->setPlotElement(vpe[i]);
    buttons[i]->show();
  }
  numbuttons= n;
  grid->invalidate();
  oktoemit= true;

  //sv->updateContents();
}

void StatusPlotButtons::enabled(PlotElement pe)
{
  emit toggleElement(pe);
}

