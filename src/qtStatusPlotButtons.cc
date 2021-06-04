/*
  Diana - A Free Meteorological Visualisation Tool

  Copyright (C) 2006-2021 met.no

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

#include "diana_config.h"

#include "qtStatusPlotButtons.h"

#include <QToolTip>
#include <QLabel>
#include <QHBoxLayout>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QImage>
#include <QPixmap>
#include <QAction>
#include <QMenu>

#include <iostream>
#include "qtImageGallery.h"
#include "question.xpm"

static bool oktoemit;

PlotButton::PlotButton(QWidget* parent, PlotElement& pe)
    : QToolButton(parent)
{
  setMinimumWidth(30);
  setPlotElement(pe);
  setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  connect(this,SIGNAL(toggled(bool)),SLOT(togg(bool)));
}

void PlotButton::setPlotElement(const PlotElement& pe)
{
  QtImageGallery ig;
  if (not pe.icon.empty()) {
    if (plotelement_.icon!=pe.icon ){
      QImage image;
      if(ig.Image(pe.icon,image)){
        setIcon(QPixmap::fromImage(image));
      } else {
        setIcon(QIcon(QPixmap(question_xpm)));
      }
    }
  } else {
    setIcon(QIcon(QPixmap(question_xpm)));
  }

  plotelement_= pe;

  tipstr_ = QString::fromStdString(pe.str);
  QString text = tipstr_.right(1);

  QString statusTip;
  QColor statusFg, statusBg;
  switch (pe.status) {
  case P_UNKNOWN:
    break; // no palette change
  case P_WAITING:
    text += ":";
    statusTip += "WAITING";
    statusBg = Qt::yellow;
    break;
  case P_OK_EMPTY:
    text += "-";
    statusTip += "OK/EMPTY";
    statusFg = Qt::green;
    break;
  case P_OK_DATA:
    text += "+";
    statusTip += "OK/DATA";
    statusFg = Qt::darkGreen;
    break;
  case P_ERROR:
    text += "!";
    statusTip += "ERROR";
    statusBg = Qt::red;
  }
  if (statusBg.isValid() || statusFg.isValid()) {
    QPalette palette;
    if (statusBg.isValid())
      palette.setColor(backgroundRole(), statusBg);
    if (statusFg.isValid())
      palette.setColor(backgroundRole(), statusFg);
    setPalette(palette);
  }
  if (!statusTip.isEmpty()) {
    tipstr_ += QString("\nStatus: %1").arg(statusTip);
  }

  setToolTip(tipstr_);
  setText(text);
  setCheckable(true);
  setChecked(plotelement_.enabled);
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
    setFocus();
  }
}


StatusPlotButtons::StatusPlotButtons(QWidget* parent)
: QWidget(parent), numbuttons(0), activebutton(0) {

  PlotElement pe;
  pe.enabled= true;
  oktoemit= false;

  QHBoxLayout* hl= new QHBoxLayout(this); // parent,margin
  hl->setMargin(1);
  for (int i=0; i<MAXBUTTONS; i++){
    buttons[i] = new PlotButton(this, pe);
    buttons[i]->hide();
    connect(buttons[i], SIGNAL(enabled(PlotElement)),
        this, SLOT(enabled(PlotElement)));
    hl->addWidget(buttons[i]);
  }

  showtip= new QMenu(this);

  //Action
  plotButtonsAction = new QAction( this );
  plotButtonsAction->setShortcut(Qt::Key_End);
  connect( plotButtonsAction, SIGNAL( triggered() ),SLOT(setfocus()));
  addAction( plotButtonsAction );
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
  releasefocus();
  grabKeyboard();
  activebutton= 0;

  showActiveButton(true);
}


void StatusPlotButtons::showText(const QString& s)
{
  calcTipPos();
  showtip->popup(tip_pos);

  showtip->clear();
}

void StatusPlotButtons::showActiveButton(bool b)
{
  if (activebutton>=0 && activebutton<numbuttons){
    buttons[activebutton]->highlight(b);
    
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
  Q_EMIT releaseFocus();
}

void StatusPlotButtons::keyPressEvent ( QKeyEvent * e )
{
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
  } else if (e->key() == Qt::Key_PageUp){
    for (int i=0; i<numbuttons; ++i){
      buttons[i]->setChecked(true);
    }
  } else if (e->key() == Qt::Key_PageDown){
    for (int i=0; i<numbuttons; ++i){
      buttons[i]->setChecked(false);
    }
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
    if (!buttons[i]->isChecked()){
      buttons[i]->toggle();
      buttons[i]->setDown(true);
    }
  }
  oktoemit= false;
}


void StatusPlotButtons::setPlotElements(const std::vector<PlotElement>& vpe)
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
    buttons[i]->setPlotElement(vpe[i]);
    buttons[i]->show();
  }
  numbuttons= n;
  oktoemit= true;
}

void StatusPlotButtons::enabled(PlotElement pe)
{
  emit toggleElement(pe);
}
