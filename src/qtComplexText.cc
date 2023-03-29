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
/*
  Input for adding complex text
 */

#include "diana_config.h"

#include "qtComplexText.h"
#include "diController.h"
#include "qtUtility.h"
#include "qtToggleButton.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPixmap>
#include <QVBoxLayout>
#include <qstring.h>

#include <puTools/miStringFunctions.h>

#define MILOGGER_CATEGORY "diana.ComplexText"
#include <miLogger/miLogging.h>


QValidator::State ComplexText::complexValidator::validate(QString& input, int& pos) const
{
  //validator, only used for zero isoterm input !!!
  if (!input.contains("0°:")) { // was"0<deg>:"
    return QValidator::Invalid;
  }
  return QValidator::Acceptable;
}

// initialize static members
bool               ComplexText::initialized= false;
std::vector<Colour::ColourInfo> ComplexText::colourInfo; // all defined colours
int                ComplexText::nr_colors=0;
QColor*            ComplexText::pixcolor=0;

/*********************************************/
ComplexText::ComplexText(QWidget* parent, Controller* llctrl, std::vector<std::string>& symbolText, std::vector<std::string>& xText,
                         std::set<std::string> cList, bool useColour)
    : QDialog(parent)
    , m_ctrl(llctrl)
{
  METLIBS_LOG_SCOPE();

  setModal(true);
  
  if (!initialized) {
    colourInfo = Colour::getColourInfo();
    
    nr_colors= colourInfo.size();
    pixcolor = new QColor[nr_colors];// this must exist in the objectlifetime
    for(int i=0; i<nr_colors; i++ ){
      pixcolor[i]=QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1],
          colourInfo[i].rgb[2] );
    }
  }
  cv = 0;

  setWindowTitle(tr("Write text"));
  
  //  //horizontal layout for holding grid layouts
  QHBoxLayout * hglayout = new QHBoxLayout();
  //grid layouts
  int ns = symbolText.size();
  int nx = xText.size();
  if (ns){
    QGridLayout* glayout = new QGridLayout();
    hglayout->addLayout(glayout, 0);

    std::set<std::string> complexList = cList;

    for (int i=0;i<ns;i++){
      METLIBS_LOG_DEBUG("symbolText["<<i<<"]"<<symbolText[i]);
      std::string ltext="Text"+miutil::from_number(i+1);
      QString labeltext=ltext.c_str();
      QLabel* namelabel= new QLabel(labeltext, this) ;

      QComboBox *text = new QComboBox(this);
      text->setEditable(true);
      text->setCompleter(NULL);
      if (!cv)
        cv = new complexValidator(this);
      text->addItem(symbolText[i].c_str());
      std::set<std::string>::iterator p = complexList.begin();
      for (; p!=complexList.end(); p++)
        text->addItem(QString::fromStdString(*p));

      connect(text->lineEdit(),
          SIGNAL(selectionChanged()),SLOT(textSelected()));
      connect(text,SIGNAL(activated(const QString &)),
          SLOT(textActivated(const QString &)));

      glayout->addWidget(namelabel, i+1,0);
      glayout->addWidget(text, i+1,1);
      vSymbolEdit.push_back(text);

      if (useColour) {
        QLabel* colourlabel= new QLabel(tr("Colour"), this) ;
        colourbox = ComboBox(this, pixcolor, nr_colors, false, 0 );
        colourbox->setMaximumWidth(50);
        colourbox->setMinimumWidth(50);
        colourbox->setEnabled(true);
        glayout->addWidget(colourlabel,i+1,2);
        glayout->addWidget(colourbox,i+1,3);
      }
      else
        colourbox=0;
    }
  }

  if (nx) {
    QGridLayout* glayout = new QGridLayout();
    hglayout->addLayout(glayout, 0);
    for (int i=0;i<nx;i++){
      METLIBS_LOG_DEBUG("xText["<<i<<"]"<<xText[i]);
      std::string ltext="X"+miutil::from_number(i+1);
      QString labeltext=ltext.c_str();
      QLabel* namelabel= new QLabel(labeltext, this) ;
      QLineEdit *x  = new QLineEdit(this);
      x->setMinimumWidth(100);
      x->setText(xText[i].c_str());
      glayout->addWidget(namelabel, i+1,0);
      glayout->addWidget(x, i+1,1);
      vXEdit.push_back(x);
    }
  }
  
  QPushButton * okb= new QPushButton(tr("OK"),this);
  connect(okb, SIGNAL(clicked()), SLOT(accept()));
  QPushButton* quitb= new QPushButton(tr("Cancel"),this);
  connect(quitb, SIGNAL(clicked()), SLOT(reject()));

  int width  = quitb->sizeHint().width();
  int height = quitb->sizeHint().height();
  //set button size
  okb->setMinimumSize( width, height );
  okb->setMaximumSize( width, height );
  quitb->setMinimumSize( width, height );
  quitb->setMaximumSize( width, height );

  // buttons layout
  QHBoxLayout * hlayout = new QHBoxLayout();
  hlayout->addWidget(okb, 10);
  hlayout->addWidget(quitb, 10);

  //now create a vertical layout to put all the other layouts in
  QVBoxLayout * vlayout = new QVBoxLayout(this);
  vlayout->addLayout(hglayout, 0);
  vlayout->addLayout(hlayout,0);

  //important to avoid endless loop !
  startEdit=true;
}


ComplexText::~ComplexText()
{
  for (size_t i = 0; i<vSymbolEdit.size(); i++)
    delete vSymbolEdit[i];

  for (size_t i = 0; i<vXEdit.size(); i++)
    delete vXEdit[i];

  delete cv;
}

void ComplexText::getComplexText(std::vector<std::string>& symbolText, std::vector<std::string>& xText)
{
  METLIBS_LOG_SCOPE();
  symbolText.clear();
  int ns=vSymbolEdit.size();
  for (int i =0; i<ns;i++)
    symbolText.push_back(vSymbolEdit[i]->currentText().toStdString());
  xText.clear();
  int nx=vXEdit.size();
  for (int i =0; i<nx;i++)
    xText.push_back(vXEdit[i]->text().toStdString());
}


void ComplexText::setColour(Colour::ColourInfo &colour)
{
  METLIBS_LOG_SCOPE();
  int index = getColourIndex(colourInfo,colour);
  colourbox-> setCurrentIndex(index);
}

void ComplexText::getColour(Colour::ColourInfo &colour)
{
  METLIBS_LOG_SCOPE();
  int index=colourbox->currentIndex();
  if (index>-1 && index<int(colourInfo.size()))
    colour=colourInfo[index];
}

int ComplexText::getColourIndex(std::vector<Colour::ColourInfo>& colourInfo, Colour::ColourInfo colour)
{
  int i,index=-1;
  int nr_colors= colourInfo.size();
  for(i=0; i<nr_colors; i++ ){
    if (colourInfo[i].rgb[0]== colour.rgb[0] &&
        colourInfo[i].rgb[1]==colour.rgb[1] &&
        colourInfo[i].rgb[2]==colour.rgb[2] )
      index=i;
  }
  if (index==-1){
    colourInfo.push_back(colour);
    QColor * pcolor=new QColor( colourInfo[i].rgb[0], colourInfo[i].rgb[1],
        colourInfo[i].rgb[2] );
    QPixmap* pmap = new QPixmap( 20, 20 );
    pmap->fill(*pcolor);
    QIcon *icon = new QIcon(*pmap);
    colourbox->addItem ( *icon,"");
    index=nr_colors;
  }
  return index;
}

void ComplexText::textActivated(const QString &textstring)
{
  METLIBS_LOG_SCOPE();
  for (unsigned int i =0;i<vSymbolEdit.size();i++){
    if (!vSymbolEdit[i]->hasFocus()) continue;
    startEdit=true;
    selectText(i);
  }
}

void ComplexText::selectText(int i)
{
  METLIBS_LOG_SCOPE();
  // Special routine to facilitate editing strings with "0<deg>:"

  if (startEdit){
    QComboBox* cb = vSymbolEdit[i];
    startEdit=false;
    const QString& text = cb->currentText();
    if (!text.contains("0°:")) { // was "0<deg>:"
      cb->lineEdit()->setValidator(0);
      return;
    } else {
      cb->lineEdit()->setValidator(cv);
      cb->lineEdit()->setCursorPosition(3);
      int length = text.length()-3;
      cb->lineEdit()->setSelection(3,length);
    }
  }
}

void ComplexText::textSelected()
{
  METLIBS_LOG_SCOPE();
  for (unsigned int i =0;i<vSymbolEdit.size();i++){
    selectText(i);
  }
}
